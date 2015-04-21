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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void vmmap_init(void) {
	vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
	KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
	vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
	KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void) {
	vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
	if (newvma) {
		newvma->vma_vmmap = NULL;
	}
	return newvma;
}

void vmarea_free(vmarea_t *vma) {
	KASSERT(NULL != vma);
	slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t vmmap_mapping_info(const void *vmmap, char *buf, size_t osize) {
	KASSERT(0 < osize);
	KASSERT(NULL != buf);
	KASSERT(NULL != vmmap);

	vmmap_t *map = (vmmap_t *) vmmap;
	vmarea_t *vma;
	ssize_t size = (ssize_t) osize;

	int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n", "VADDR RANGE",
			"PROT", "FLAGS", "MMOBJ", "OFFSET", "VFN RANGE");

	list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
				{
					size -= len;
					buf += len;
					if (0 >= size) {
						goto end;
					}

					len =
							snprintf(buf, size,
									"%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
									vma->vma_start << PAGE_SHIFT,
									vma->vma_end << PAGE_SHIFT,
									(vma->vma_prot & PROT_READ ? 'r' : '-'),
									(vma->vma_prot & PROT_WRITE ? 'w' : '-'),
									(vma->vma_prot & PROT_EXEC ? 'x' : '-'),
									(vma->vma_flags & MAP_SHARED ?
											" SHARED" : "PRIVATE"),
									vma->vma_obj, vma->vma_off, vma->vma_start,
									vma->vma_end);
				}list_iterate_end();

	end: if (size <= 0) {
		size = osize;
		buf[osize - 1] = '\0';
	}
	/*
	 KASSERT(0 <= size);
	 if (0 == size) {
	 size++;
	 buf--;
	 buf[0] = '\0';
	 }
	 */
	return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void) {
	/*  NOT_YET_IMPLEMENTED("VM: vmmap_create");
	 return NULL;*/
	vmmap_t* map = (vmmap_t*) slab_obj_alloc(vmmap_allocator);
	map->vmm_proc = NULL;
	list_init(&(map->vmm_list));
	return map;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void vmmap_destroy(vmmap_t *map) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_destroy");*/
	list_link_t *link = (&(map->vmm_list))->l_next;
	for (; link != &(map->vmm_list); link = link->l_next) {
		vmarea_t* area = list_item(link, vmarea_t, vma_plink);
		list_remove(link);
		vmarea_free(area);
	};
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void vmmap_insert(vmmap_t *map, vmarea_t *newvma) {

	uint32_t new_vma_end = newvma->vma_end;
	uint32_t new_vma_start = newvma->vma_start;
	/*find the spot in the vmareas list*/
	list_link_t *link;
	for (link = (&(map->vmm_list))->l_next; link != &(map->vmm_list); link =
			link->l_next) {
		vmarea_t* area = list_item(link, vmarea_t, vma_plink);
		uint32_t vma_start = area->vma_start;
		uint32_t vma_end = area->vma_end;
		if (link->l_next != &(map->vmm_list) && vma_end <= new_vma_start
				&& new_vma_end
						<= (list_item(link->l_next, vmarea_t, vma_plink))->vma_start) {
			list_insert_before(link->l_next, &(newvma->vma_plink));
			newvma->vma_vmmap = map;
			return;
		}
	};
	/*if it can't be inserted between any vmareas, append to tail*/
	list_insert_tail(&(map->vmm_list), &(newvma->vma_plink));
	newvma->vma_vmmap = map;

	/* NOT_YET_IMPLEMENTED("VM: vmmap_insert");*/
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int vmmap_find_range(vmmap_t *map, uint32_t npages, int dir) {

	list_link_t *link;
	uint32_t num_free = 0; /*number of contiguous free pages*/
	if (dir == VMMAP_DIR_HILO) {
		for (link = (&(map->vmm_list))->l_prev; link != &(map->vmm_list); link =
				link->l_prev) {
			vmarea_t* area = list_item(link, vmarea_t, vma_plink);
			uint32_t vfn_start = area->vma_start;
			int page_num = area->vma_obj->mmo_nrespages;
			int i = 0;
			pframe_t* pf;
			for (i = page_num - 1; i >= 0; i--) {
				area->vma_obj->mmo_ops->lookuppage(area->vma_obj, i, 1, &pf);
				if (pframe_is_free(pf)) {
					num_free++;
				} else {
					num_free = 0;
				};
				if (num_free == npages) {
					return vfn_start + i;
				}
			}
		}
	};

	if (dir == VMMAP_DIR_LOHI) {
		for (link = (&(map->vmm_list))->l_next; link != &(map->vmm_list); link =
				link->l_next) {
			vmarea_t* area = list_item(link, vmarea_t, vma_plink);
			uint32_t vfn_start = area->vma_start;
			int page_num = area->vma_obj->mmo_nrespages;
			int i = 0;
			pframe_t* pf;
			for (i = 0; i < page_num; i++) {
				area->vma_obj->mmo_ops->lookuppage(area->vma_obj, i, 1, &pf);
				if (pframe_is_free(pf)) {
					num_free++;
				} else {
					num_free = 0;
				};
				if (num_free == npages) {
					return (vfn_start + i - npages);
				}
			}
		}
	};
	return -1;

	/*	NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
	 return -1;*/
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn) {
	/* NOT_YET_IMPLEMENTED("VM: vmmap_lookup");*/
	list_link_t *link;
	for (link = (&(map->vmm_list))->l_next; link != &(map->vmm_list); link =
			link->l_next) {
		vmarea_t* area = list_item(link, vmarea_t, vma_plink);
		uint32_t vma_start = area->vma_start;
		uint32_t vma_end = area->vma_end;
		if (vma_start <= vfn && vfn < vma_end) {
			return area;
		}
	};
	return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_clone");*/

	/*allocate a new vmmap:*/
	vmmap_t *new_map = vmmap_create(); /*vmarea_alloc();*/
	if (new_map == NULL) {
		return NULL; /*couldn't allocate a new map, returning null*/
	};
	/*iterate entire list of vmareas of a given memory map, and allocate a new vmarea */
	list_link_t *link;
	for (link = &(map->vmm_list); link != &(map->vmm_list); link =
			link->l_next) {
		/*allocate a new vmarea and insert it into the list in new vmmap*/
		vmarea_t *new_area = vmarea_alloc();
		if (new_area == NULL) {
			return NULL; /*couldn't allocate a new vmarea, return null, maybe need to do a cleanup here*/
		};
		list_insert_tail(&(new_map->vmm_list), &(new_area->vma_plink));
	}
	return new_map;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
		int prot, int flags, off_t off, int dir, vmarea_t **new) {
	/*
	NOT_YET_IMPLEMENTED("VM: vmmap_map");
	return -1;
	*/

	KASSERT(map);
	/*KASSERT(new);*/
	vmarea_t* new_area  = vmarea_alloc();
	new_area->vma_off = off;
	new_area->vma_flags = flags;
	new_area->vma_prot = prot;

	if(!lopage) { /* lopage == 0 */
		int start_vfn = vmmap_find_range(map, npages, dir);

		if( start_vfn >= 0) {
			new_area->vma_start = start_vfn;
			new_area->vma_end = npages + start_vfn; /* what about 4KB ?? */
		vmmap_insert(map, new_area);
		if(file) {
				int mmobj_ret = file->vn_ops->mmap(file, new_area, &new_area->vma_obj);
				if(mmobj_ret >= 0) {
					new_area->vma_obj->mmo_ops->ref(new_area->vma_obj);
					vref(file);
				}
			}/* else if(flags & MAP_PRIVATE) {
				new_area->vma_obj =  shadow_create();
			} */else { /* anon obj */
				new_area->vma_obj =  anon_create();
			}

			new_area->vma_vmmap = map;
			new = &new_area;
			return 0;
		}
	} else { /* lopage != 0 */
		int is_range_empty = vmmap_is_range_empty(map, lopage, npages);
		if(is_range_empty) { /* range is empty */
			new_area->vma_start = lopage;
			new_area->vma_end = npages + lopage; /* what about 4KB ?? */
		vmmap_insert(map, new_area);
		if(file) {
				int mmobj_ret = file->vn_ops->mmap(file, new_area, &new_area->vma_obj);
				if(mmobj_ret >= 0) {
					new_area->vma_obj->mmo_ops->ref(new_area->vma_obj);
					vref(file);
				}
			}/* else if(flags & MAP_PRIVATE) {
				new_area->vma_obj =  shadow_create();
			} */else { /* anon obj */
				new_area->vma_obj =  anon_create();
			}

			new_area->vma_vmmap = map;
			new = &new_area;
			return 0;
		} else { /* address is being used */
			vmmap_remove(map, lopage, npages);
			new_area->vma_start = lopage;
			new_area->vma_end = npages + lopage; /* what about 4KB ?? */
		vmmap_insert(map, new_area);
		if(file) {
				int mmobj_ret = file->vn_ops->mmap(file, new_area, &new_area->vma_obj);
				if(mmobj_ret >= 0) {
					new_area->vma_obj->mmo_ops->ref(new_area->vma_obj);
					vref(file);
				}
			}/* else if(flags & MAP_PRIVATE) {
				new_area->vma_obj =  shadow_create();
			} */else { /* anon obj */
				new_area->vma_obj =  anon_create();
			}

			new_area->vma_vmmap = map;
			new = &new_area;
			return 0;
		}
	}

	return -1;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_remove");*/
	vmarea_t *area = vmmap_lookup(map, lopage);
	uint32_t vmarea_start = area->vma_start;
	uint32_t vmarea_end = area->vma_end;
	uint32_t lopage_end = lopage + npages;
	/*case 2:*/
	if (vmarea_start < lopage && vmarea_end < lopage_end) {
		area->vma_end = lopage_end;
		return 1;
	}
	/*case 4*/
	if (lopage <= vmarea_start && lopage_end >= vmarea_end) {
		list_remove(&area->vma_plink);
		/*list_remove(&area->vma_olink);*/
		vmarea_free(area);
		return 1;
	}
	/*case 3*/
	if (lopage < vmarea_start && lopage_end < vmarea_end) {
		area->vma_start = lopage_end;
		uint32_t old_offset = area->vma_off;
		area->vma_off = old_offset + (lopage_end - vmarea_start);
		return 1;
	}
	/*case 1*/
	if (vmarea_start < lopage && lopage_end < vmarea_end) {
		/*allocate 2 new vmareas instead of the old one*/
		vmarea_t* l_area = vmarea_alloc(); /*area to the left of a removed region*/
		vmarea_t *r_area = vmarea_alloc(); /*area to the right*/
		/*copy stuff from original vmarea to l_area*/
		l_area->vma_start = vmarea_start;
		l_area->vma_end = lopage;
		l_area->vma_flags = area->vma_flags;
		l_area->vma_off = area->vma_off;
		l_area->vma_obj = area->vma_obj;
		l_area->vma_prot = area->vma_prot;
		l_area->vma_vmmap = map;
		vmmap_insert(map, l_area); /*that takes care of plink, and what do we do with olink*/
		list_insert_before(&(area->vma_olink), &(l_area->vma_olink));
		/*do the same for the r_area*/
		r_area->vma_start = lopage_end;
		r_area->vma_end = vmarea_end;
		r_area->vma_flags = area->vma_flags;
		r_area->vma_off = (area->vma_off) + (lopage_end - vmarea_start);
		r_area->vma_obj = area->vma_obj;
		r_area->vma_prot = area->vma_prot;
		r_area->vma_vmmap = map;
		vmmap_insert(map, r_area); /*that takes care of plink, and what do we do with olink*/
		list_insert_before((&(area->vma_olink))->l_next, &(r_area->vma_olink));
		/*increment the ref count on mmobj:*/
		area->vma_obj->mmo_ops->ref(area->vma_obj);
		/*remove and deallocate the old vmarea*/
		list_remove(&area->vma_plink);
		list_remove(&area->vma_olink);
		vmarea_free(area);
		return 1;
	}
	return -1;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages) {
	vmarea_t *area = vmmap_lookup(map, startvfn);
	if(area) {
		int page_num = area->vma_obj->mmo_nrespages; /*number of pages in vmarea*/
		int i = startvfn - (area->vma_start);
		uint32_t num_free = 0;
		if ((uint32_t) page_num < npages) {
			return 1;
		}
		pframe_t* pf;
		for (; i < page_num; i++) {
			area->vma_obj->mmo_ops->lookuppage(area->vma_obj, i, 1, &pf);
			if(pf) {
				if (pframe_is_free(pf)) {
					num_free++;
				} else {
					return 0;
				};
				if (num_free == npages) {
					return 0;
				}
			} else {
				return 1;
			}
		};
	}
	/*NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");*/
	return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_read");*/
	/*convert virtual address to vfn*/
	uintptr_t addr = (uintptr_t) vaddr;
	uint32_t vfn = ADDR_TO_PN(addr);
	vmarea_t *area = vmmap_lookup(map, vfn); /* look up the vmarea vaddr belongs to*/
	if (area == NULL) {
		return -1;
	}
	/*look up the page*/
	struct mmobj *memobj = area->vma_obj;
	int pagenum = vfn - (area->vma_start)/* + (area->vma_off)*/; /*pagenum based on the vfn of the vaddr*/
	pframe_t* pg_frame = NULL;
	int write_count = 0;
		uintptr_t offset = PAGE_OFFSET(addr);/*(addr << 20) >> 20;*//*get the offset in the physical page*/
		uint32_t rem_count = count;
		while (rem_count > 0) {
			int result = pframe_get(memobj, pagenum, &pg_frame);
			if (result < 0) {
				return result;
			};
			void *pf_addr = pg_frame->pf_addr;
			void * new_addr = (void*) ((uintptr_t) pf_addr + offset); /*just so gcc doesn't complain*/
			int num_to_write = rem_count;
			if ((PAGE_SIZE - offset) < rem_count) {
				num_to_write = PAGE_SIZE - offset;
				pagenum++;
				offset = 0;
			}
			memcpy((void*)((uintptr_t)buf + write_count),new_addr,num_to_write);
			write_count += num_to_write;
			rem_count -= num_to_write;
		}
	return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count) {
	/* NOT_YET_IMPLEMENTED("VM: vmmap_write");*/
	uintptr_t addr = (uintptr_t) vaddr;
	uint32_t vfn = ADDR_TO_PN(addr);
	vmarea_t *area = vmmap_lookup(map, vfn); /* look up the vmarea vaddr belongs to*/
	/*look up the page*/
	int pagenum = vfn - (area->vma_start)/* + (area->vma_off)*/; /*pagenum based on the vfn of the vaddr*/
	struct mmobj *memobj = area->vma_obj;
	pframe_t* pg_frame = NULL;
	int write_count = 0;
	uintptr_t offset = PAGE_OFFSET(addr);/*(addr << 20) >> 20;*//*get the offset in the physical page*/
	uint32_t rem_count = count;
	while (rem_count > 0) {
		int result = pframe_get(memobj, pagenum, &pg_frame);
		if (result < 0) {
			return result;
		};
		void *pf_addr = pg_frame->pf_addr;
		void * new_addr = (void*) ((uintptr_t) pf_addr + offset); /*just so gcc doesn't complain*/
		int num_to_write = rem_count;
		if ((PAGE_SIZE - offset) < rem_count) {
			num_to_write = PAGE_SIZE - offset;
			pagenum++;
			offset = 0;
		}
		memcpy(new_addr, (void*)((uintptr_t)buf + write_count), num_to_write);
		write_count += num_to_write;
		rem_count -= num_to_write;
	}
	return 0;
}
