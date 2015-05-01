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

#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
    /*NOT_YET_IMPLEMENTED("VM: do_mmap");
    return -1;*/
	if (!(flags & MAP_PRIVATE) && !(flags & MAP_SHARED) && !(flags & MAP_FIXED) && !(flags & MAP_ANON) ){
			return -EINVAL;
	}
	/*addr = (uintptr_t) addr;*/

	/* invalid file descriptor */
	if((fd < 0 && fd == -1) || fd >= NFILES || curproc->p_files[fd]==NULL) {
	     return -EBADF;
	}
	/* args not valid */
	if(len == 0 || (PAGE_ALIGNED(addr)==0)) {
		return -EINVAL;
	}
	if( (!(flags & MAP_PRIVATE) && !(flags & MAP_SHARED)) || ((flags & MAP_PRIVATE) && (flags & MAP_SHARED)) ){
		return -EINVAL;
	}

	if((flags & MAP_PRIVATE) && !(curproc->p_files[fd]->f_mode & FMODE_READ)){
		return -EACCES;
	}

	if((flags & MAP_SHARED) && (prot & PROT_WRITE)) {
		if( !(curproc->p_files[fd]->f_mode & FMODE_READ) && !(curproc->p_files[fd]->f_mode & FMODE_WRITE) ){
			return -EACCES;
		}
		if(curproc->p_files[fd]->f_mode == FMODE_APPEND){
			return -EACCES;
		}
	}
	struct vnode* vnod = curproc->p_files[fd]->f_vnode;
	if (fd == -1) {
		file_t * new_file = fget(fd);
		vnod = new_file->f_vnode;
	}
	tlb_flush((uintptr_t)addr);
	KASSERT(NULL != curproc->p_pagedir);
	dbg(DBG_PRINT, "(GRADING3A 2.a)\n");
   	/*tlb_flush((uintptr_t)addr);*/
	/*what should be lopage and npage*/
	uint32_t lopage = ADDR_TO_PN(addr);
	uint32_t npages = (PAGE_SIZE + len-1)/PAGE_SIZE;
	vmarea_t *new_area;
	int i = vmmap_map(curproc->p_vmmap, vnod, lopage, npages, prot, flags, off, VMMAP_DIR_HILO, &new_area);

	*ret = (uint32_t *)PN_TO_ADDR(new_area->vma_start);
	return i;

}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
       /* NOT_YET_IMPLEMENTED("VM: do_munmap");
        return -1;*/
       /* if (len==0)
        		return -EINVAL;

        	if(PAGE_ALIGNED(addr)==0)
        		return -EINVAL;
        	tlb_flush((uintptr_t)addr);
        	uint32_t lopage = ADDR_TO_PN(addr);
        	uint32_t npages = len / PAGE_SIZE + 1;
        	vmmap_remove(curproc->p_vmmap,lopage,npages);
        	return 0;*/
    if (len==0)
      	return -EINVAL;
      	if (addr<(void*)USER_MEM_LOW || (size_t)addr+len>(size_t)USER_MEM_HIGH)
      		return -EINVAL;
      	if(PAGE_ALIGNED(addr)==0)
      		return -EINVAL;
       	uint32_t lopage = ADDR_TO_PN(addr);
      	uint32_t npages = (PAGE_SIZE + len-1)/PAGE_SIZE;
      	vmmap_remove(curproc->p_vmmap,lopage,npages);
      	tlb_flush((uintptr_t)addr);
      	return 0;
}

