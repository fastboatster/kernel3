/******************************************************************************/
/* Important Spring 2015 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page (don't forget
 * about shadow objects, especially copy-on-write magic!). Make
 * sure that if the user writes to the page it will be handled
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{/*
        NOT_YET_IMPLEMENTED("VM: handle_pagefault");
*/
	/* Faults to be handled
	 * #define FAULT_PRESENT  0x01 -->  The fault was caused by a page-level protection violation
		#define FAULT_WRITE    0x02
		#define FAULT_USER     0x04 --> fault caused, when the processor was executing in user mode.
		#define FAULT_RESERVED 0x08
		#define FAULT_EXEC     0x10
	 */

		dbg(DBG_PRINT, "Page fault executing..");

	 /* get the base address of the page */
	uint32_t vaddr_vfn = ADDR_TO_PN(vaddr); /* Since everything is a 4KB page, we divide the virtual address/4096 to get the actual base address */

	/* get the corresponding vmarea of the current process */
	vmarea_t *vmarea = vmmap_lookup(curproc->p_vmmap, vaddr_vfn);
	if(vmarea) {
		if((cause & FAULT_RESERVED) && (vmarea->vma_prot == PROT_NONE)) {
			proc_kill(curproc, EFAULT);
			return;
		}
		/* fault happened because of exec but it does not have exec permission */
		else if((cause & FAULT_EXEC) && !(vmarea->vma_prot & PROT_EXEC)) {
			proc_kill(curproc, EFAULT);
			return;
		} else if((cause & FAULT_WRITE) && !(vmarea->vma_prot & PROT_WRITE)) {
			proc_kill(curproc, EFAULT);
			return;
		} else if((cause & FAULT_PRESENT) && !(vmarea->vma_prot & PROT_READ)) {
			proc_kill(curproc, EFAULT);
			return;
		} else { /* not sure what to do with FAULT_USER/ FAULT_PRESENT */
			/*pframe_get(struct mmobj *o, uint32_t pagenum, pframe_t **result) */
			pframe_t *new_frame = NULL;
			int pagenum = (vmarea->vma_start - vaddr_vfn)/* + vmarea->vma_off*/;
			if(pframe_get(vmarea->vma_obj, pagenum, &new_frame) >= 0){
				uintptr_t paddr = pt_virt_to_phys((uintptr_t)new_frame->pf_addr); /* gives the physical address */
				if(pt_map(curproc->p_pagedir, PAGE_ALIGN_DOWN(vaddr), paddr, PD_PRESENT|PD_WRITE|PD_USER, PT_PRESENT|PT_WRITE|PT_USER) < 0) {
					return;
				}
				sched_broadcast_on(&new_frame->pf_waitq);
				return;/* this helps the waiting process to wake up */
			} else {
				return;
			}
		}
	}
	/*vmarea is NULL --> no mapping */
	proc_kill(curproc, EFAULT);
	return;
}
