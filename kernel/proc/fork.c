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
		/*child->p_cwd = parent->p_cwd;
		vref(child->p_cwd); */ /* parent is always associated with CWD */

		/*I dont know whether or not to do these things??*/
		/*memcpy(child->p_pagedir, parent->p_pagedir, sizeof(child->p_pagedir));*/
		/*child->p_pagedir = parent->p_pagedir;*/
		KASSERT(child->p_pagedir != NULL);
		dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
		child->p_brk = parent->p_brk;
		child->p_start_brk = parent->p_start_brk;

		/*Clone the vmmap and adjust the shadow objects*/
		child->p_vmmap = vmmap_clone(parent->p_vmmap);
		if(!child->p_vmmap) {
			pt_destroy_pagedir(child->p_pagedir);
			list_remove(&(child->p_list_link));
			list_remove(&(child->p_child_link));
			vput(child->p_cwd);
			return -1;
		}

		vmarea_t *p_area = NULL;
		vmarea_t *c_area = NULL;
		list_link_t *link = (&child->p_vmmap->vmm_list)->l_next;
		list_iterate_begin(&(parent->p_vmmap->vmm_list), p_area, vmarea_t, vma_plink){
			c_area = list_item(link , vmarea_t, vma_plink);
			if((p_area->vma_flags & MAP_PRIVATE)) { /*Private Obj*/
				/* create a shadow object for parent and adjust the pointers */
				mmobj_t *p_old_mmobj = p_area->vma_obj;
				mmobj_t *p_shadow_obj = shadow_create();
				if(!p_shadow_obj) {
					pt_destroy_pagedir(child->p_pagedir);
					list_remove(&(child->p_list_link));
					list_remove(&(child->p_child_link));
					vput(child->p_cwd);
					vmmap_destroy(child->p_vmmap);
					return -1;
				}
				p_shadow_obj->mmo_shadowed = p_old_mmobj;
				p_shadow_obj->mmo_un.mmo_bottom_obj = p_old_mmobj->mmo_un.mmo_bottom_obj;
				p_area->vma_obj = p_shadow_obj;

				mmobj_t *c_shadow_obj = shadow_create();
				if(!c_shadow_obj) {
					pt_destroy_pagedir(child->p_pagedir);
					list_remove(&(child->p_list_link));
					list_remove(&(child->p_child_link));
					vput(child->p_cwd);
					vmmap_destroy(child->p_vmmap);
					return -1;
				}
				c_shadow_obj->mmo_shadowed = p_old_mmobj;
				c_shadow_obj->mmo_un.mmo_bottom_obj = p_old_mmobj->mmo_un.mmo_bottom_obj;
				c_area->vma_obj = c_shadow_obj;
				/* increment the refcount of parent shadow */
				p_old_mmobj->mmo_ops->ref(p_old_mmobj);
				/* decrement the refcount of bottom object */
				/*p_old_mmobj->mmo_un.mmo_bottom_obj->mmo_ops->put(p_old_mmobj->mmo_un.mmo_bottom_obj);*/
				list_insert_tail(mmobj_bottom_vmas(p_old_mmobj->mmo_un.mmo_bottom_obj), &(c_area->vma_olink));
			} else{
				c_area->vma_obj = p_area->vma_obj;
				c_area->vma_obj->mmo_ops->ref(c_area->vma_obj);
			}
			link = link->l_next;
		}list_iterate_end();

		/*
		list_iterate_begin(&(child->p_vmmap->vmm_list), c_area, vmarea_t, vma_plink){
			if((c_area->vma_flags & MAP_PRIVATE)) {
				mmobj_t *c_old_mmobj = c_area->vma_obj;
				mmobj_t *c_shadow_obj = shadow_create();
				if(!c_shadow_obj) {
					pt_destroy_pagedir(child->p_pagedir);
					list_remove(&(child->p_list_link));
					list_remove(&(child->p_child_link));
					vput(child->p_cwd);
					vmmap_destroy(child->p_vmmap);
					return -1;
				}
				c_shadow_obj = c_old_mmobj;
				c_area->vma_obj = c_shadow_obj;
			}
		}list_iterate_end();
*/
		pt_unmap_range(parent->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
		tlb_flush_all();

		int i;
		for (i = 0;i < NFILES; i++) {
			child->p_files[i] = parent->p_files[i];
			if(child->p_files[i]) { /* not all the file descriptors point to a file, some may be NULL */
				fref(child->p_files[i]);
			}
		}
		/*need to clone the thread, set up the stack, return appropriate value, change eax register, make runnable*/
		kthread_t * child_thr = kthread_clone(parent_thr);
		if(!child_thr) {
			pt_destroy_pagedir(child->p_pagedir);
			list_remove(&(child->p_list_link));
			list_remove(&(child->p_child_link));
			vput(child->p_cwd);
			vmmap_destroy(child->p_vmmap);
			return -1;
		}
		list_insert_tail(&(child->p_threads), &(child_thr->kt_plink));
		KASSERT(child_thr->kt_kstack != NULL);
		dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
		child_thr->kt_proc = child;

		/* copy page table pointer */
		child_thr->kt_ctx.c_pdptr = child->p_pagedir;
		child_thr->kt_ctx.c_kstack = (uintptr_t)child_thr->kt_kstack;
		/* set the size for new thread kernel stack */
		child_thr->kt_ctx.c_kstacksz = /*parent_thr->kt_ctx.c_kstacksz*/ DEFAULT_STACK_SIZE;
		/* set return val for the child process */
		regs->r_eax = 0;
		/* set ESP */
		child_thr->kt_ctx.c_eip = (uint32_t)userland_entry;
		child_thr->kt_ctx.c_esp = fork_setup_stack(regs, child_thr->kt_kstack);
		/* set EIP */

		/* make the child thread runnable */
		sched_make_runnable(child_thr);
		return child->p_pid;
}
