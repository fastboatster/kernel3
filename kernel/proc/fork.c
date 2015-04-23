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
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
	/* NOT_YET_IMPLEMENTED("VM: do_fork");
	     return 0;*/

		KASSERT(regs != NULL);
		dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
		KASSERT(curproc != NULL);
		dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
		KASSERT(curproc->p_state == PROC_RUNNING);
		dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

		/*Give current process and thead to parent*/
		proc_t * parent = curproc;
		kthread_t * parent_thr = curthr;

		/*Create child process and copy the contents of file table and cwd into child*/
		proc_t * child = proc_create("CHILD_PROC");
		KASSERT(child->p_state == PROC_RUNNING);
		dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
		int i;
		for (i = 0;i < NFILES; i++){
			child->p_files[i] = parent->p_files[i];
			fref(child->p_files[i]);
		}
		child->p_cwd = parent->p_cwd;
		vref(child->p_cwd);

		/*I dont know whether or not to do these things??*/
		child->p_pagedir = parent->p_pagedir;
		KASSERT(child->p_pagedir != NULL);
		dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
		child->p_brk = parent->p_brk;
		child->p_start_brk = parent->p_start_brk;

		/*Clone the vmmap and adju0st the shadow objects*/
		child->p_vmmap = vmmap_clone(parent->p_vmmap);
		vmarea_t *p_area, *c_area;
		list_link_t *link = &(child->p_vmmap->vmm_list);
		list_iterate_begin(&(parent->p_vmmap->vmm_list), p_area, vmarea_t, vma_plink){

			if (p_area->vma_flags & MAP_SHARED){ /*Shared object*/
				c_area = list_item(link , vmarea_t, vma_plink);
				c_area->vma_obj = p_area->vma_obj;
				p_area->vma_obj->mmo_ops->ref(p_area->vma_obj);
			}
			else{/*Private Obj*/
				mmobj_t *p_old_mmobj=p_area->vma_obj;
				mmobj_t *p_shadow_obj=shadow_create();
				p_area->vma_obj=p_shadow_obj;
				p_shadow_obj->mmo_shadowed=p_old_mmobj;
				mmobj_t *c_shadow_obj=shadow_create();
				c_area->vma_obj=c_shadow_obj;
				c_shadow_obj->mmo_shadowed=p_old_mmobj;
				p_old_mmobj->mmo_ops->ref(p_old_mmobj);



			}
			link = link->l_next;

		}list_iterate_end();
		/*need to clone the thread, set up the stack, return appropriate value, change eax register, make runnable*/
		kthread_t * child_thr=kthread_clone(parent_thr);
		KASSERT(child_thr->kt_kstack != NULL);
		pt_unmap_range(parent->p_pagedir, 0, 0);
		tlb_flush_all();

		child_thr->kt_ctx.c_pdptr=parent_thr->kt_ctx.c_pdptr;
		child_thr->kt_ctx.c_kstack=parent_thr->kt_ctx.c_kstack;
		child_thr->kt_ctx.c_kstacksz=parent_thr->kt_ctx.c_kstacksz;
		child_thr->kt_ctx.c_esp=fork_setup_stack((const regs_t*)regs, (void *)child_thr->kt_kstack);
		child_thr->kt_ctx.c_eip=(uint32_t)userland_entry;
		regs->r_eax=NULL;
		sched_make_runnable(child_thr);
		if (curthr==child_thr)
			return 0;
		else
			return child->p_pid;
}
