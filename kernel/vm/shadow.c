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

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
 * number of shadow objects with a single parent, that is another shadow
 * object in the shadow objects tree(singletons)
 */
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int  shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int  shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int  shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = {
        .ref = shadow_ref,
        .put = shadow_put,
        .lookuppage = shadow_lookuppage,
        .fillpage  = shadow_fillpage,
        .dirtypage = shadow_dirtypage,
        .cleanpage = shadow_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * shadow page sub system. Currently it only initializes the
 * shadow_allocator object.
 */
void
shadow_init()
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_init");
    */
	shadow_allocator = slab_allocator_create("shadow", sizeof(mmobj_t));
    KASSERT(shadow_allocator);
    dbg(DBG_PRINT, "(GRADING3A 6.a)\n");
}

/*
 * You'll want to use the shadow_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
shadow_create()
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_create");
        return NULL;
    */
	mmobj_t *new_mmobj = (mmobj_t*)slab_obj_alloc(shadow_allocator);
	if(new_mmobj) {
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		mmobj_init(new_mmobj, &shadow_mmobj_ops);
		new_mmobj->mmo_un.mmo_bottom_obj = NULL;
		new_mmobj->mmo_refcount++;
	}
	return new_mmobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
shadow_ref(mmobj_t *o)
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_ref");
    */
	KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "(GRADING3A 6.b)\n");
	o->mmo_refcount++;
	return;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is a shadow object, it will never
 * be used again. You should unpin and uncache all of the object's
 * pages and then free the object itself.
 */
static void
shadow_put(mmobj_t *o)
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_put");
    */
	KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "(GRADING3A 6.c)\n");
	if((o->mmo_refcount-1) == o->mmo_nrespages) { /* mmobj no longer in use */
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		if(!(list_empty(&(o->mmo_respages)))) {
			dbg(DBG_PRINT, "(GRADING3 D.2)\n");
			pframe_t *page = NULL;
			list_iterate_begin(&o->mmo_respages, page, pframe_t, pf_olink) {
				while(pframe_is_busy(page)) { /* if the object is busy wait for it */
					dbg(DBG_PRINT, "(shadow4)\n");
					sched_sleep_on(&(page->pf_waitq)); /* wait for it to become not busy*/
				}
				if(pframe_is_pinned(page)) {
					dbg(DBG_PRINT, "(GRADING3 D.2)\n");
					pframe_unpin(page);
				} else if(pframe_is_dirty(page)){
					dbg(DBG_PRINT, "(shadow6)\n");
					pframe_clean(page);
				} else {
					dbg(DBG_PRINT, "(GRADING3 D.2)\n");
					/* free the page */
					pframe_free(page);/* uncache all the pages */
				}
			}list_iterate_end();
			/*pframe_free(page);*/
		}
	}
	o->mmo_refcount--;
	if(o->mmo_refcount == 0) {
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		slab_obj_free(shadow_allocator, o); /* free the object */
	}
	return;
}

/* This function looks up the given page in this shadow object. The
 * forwrite argument is true if the page is being looked up for
 * writing, false if it is being looked up for reading. This function
 * must handle all do-not-copy-on-not-write magic (i.e. when forwrite
 * is false find the first shadow object in the chain which has the
 * given page resident). copy-on-write magic (necessary when forwrite
 * is true) is handled in shadow_fillpage, not here. It is important to
 * use iteration rather than recursion here as a recursive implementation
 * can overflow the kernel stack when looking down a long shadow chain */
static int
shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_lookuppage");
        return 0;
    */
	if(forwrite) { /* copy-on-write */
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		return pframe_get(o, pagenum, pf);
	} else { /* no copy on write */
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		pframe_t* page = NULL;
		mmobj_t* temp_mmobj = o;
		if(temp_mmobj->mmo_shadowed) { /* look for all shadowed object */
			dbg(DBG_PRINT, "(GRADING3 D.2)\n");
			while(temp_mmobj->mmo_shadowed) {
				dbg(DBG_PRINT, "(GRADING3 D.2)\n");
				page = pframe_get_resident(temp_mmobj, pagenum);
				if(page) {
					dbg(DBG_PRINT, "(GRADING3 D.2)\n");
					 while (pframe_is_busy(page)) {
						 dbg(DBG_PRINT, "(shadow14)\n");
						 sched_sleep_on(&page->pf_waitq);
					 }
					 *pf = page;
					return 0;
				}
				temp_mmobj = temp_mmobj->mmo_shadowed;
			}
			/* object not found in any of the shadowed object, check in bottom object */
			int temp = pframe_get(temp_mmobj, pagenum, &page);
			if(temp < 0) {
				return temp;
			}
			if(page) {
				dbg(DBG_PRINT, "(GRADING3 D.2)\n");
				 *pf = page;
				return 0;
			}
		} else { /* not a shadowed object */
			dbg(DBG_PRINT, "(shadow16)\n");
			int temp = pframe_get(temp_mmobj, pagenum, &page);
			if(temp < 0) {
				dbg(DBG_PRINT, "(GRADING3 D.4)\n");
				return temp;
			}
			if(page) {
				dbg(DBG_PRINT, "(shadow17)\n");
				 *pf = page;
				return 0;
			}
		}
	}
	return -1;
}

/* As per the specification in mmobj.h, fill the page frame starting
 * at address pf->pf_addr with the contents of the page identified by
 * pf->pf_obj and pf->pf_pagenum. This function handles all
 * copy-on-write magic (i.e. if there is a shadow object which has
 * data for the pf->pf_pagenum-th page then we should take that data,
 * if no such shadow object exists we need to follow the chain of
 * shadow objects all the way to the bottom object and take the data
 * for the pf->pf_pagenum-th page from the last object in the chain).
 * It is important to use iteration rather than recursion here as a 
 * recursive implementation can overflow the kernel stack when 
 * looking down a long shadow chain */
static int
shadow_fillpage(mmobj_t *o, pframe_t *pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_fillpage");
        return 0;
    */
	KASSERT(pframe_is_busy(pf));
	dbg(DBG_PRINT, "(GRADING3A 6.d)\n");
	KASSERT(!pframe_is_pinned(pf));
	dbg(DBG_PRINT, "(GRADING3A 6.d)\n");

	pframe_t* page = NULL;
	mmobj_t *temp = o->mmo_shadowed;
	if(temp->mmo_shadowed) { /* shadowed object */
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		while(temp->mmo_shadowed) {
			dbg(DBG_PRINT, "(GRADING3 D.2)\n");
			page = pframe_get_resident(temp, pf->pf_pagenum);
			if(page) {
				dbg(DBG_PRINT, "(GRADING3 D.2)\n");
				break;
			}
			temp = temp->mmo_shadowed;
		}
	}
	/* page not found from shadow objects */
	/* lookup the page, not temp is the bottom most object */
	if(!page) { /* look for a page in the bottom object */
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		page = pframe_get_resident(temp, pf->pf_pagenum);
	}
	if(page) {
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		pframe_set_dirty(pf);
		pframe_pin(pf);
		memcpy(pf->pf_addr, page->pf_addr, PAGE_SIZE);
		return 0;
	} else {
		dbg(DBG_PRINT, "(GRADING3 D.2)\n");
		memset(pf->pf_addr, 0, PAGE_SIZE);
		return 0;
	}
	return -1;
}

/* These next two functions are not difficult. */

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_dirtypage");
        return -1;
    */
	if(!pframe_is_dirty(pf)) {
		dbg(DBG_PRINT, "(shadow24)\n");
		pframe_set_dirty(pf);
		return 0;
	}
	return -1;
}

static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: shadow_cleanpage");
        return -1;
    */
	pframe_t* page = NULL; /* probably this is the destination page */
	mmobj_t *temp = o;
	int found_page = shadow_lookuppage(temp, pf->pf_pagenum, 0, &page);
	if(found_page < 0) {
		dbg(DBG_PRINT, "(shadow25)\n");
		return -1;
	}
	if(page) {
		dbg(DBG_PRINT, "(shadow26)\n");
		while(pframe_is_busy(pf)){ /* wait for it to become free */
			dbg(DBG_PRINT, "(shadow27)\n");
			sched_broadcast_on(&pf->pf_waitq);
		}
		if(pframe_is_pinned(pf)){
			dbg(DBG_PRINT, "(shadow28)\n");
			pframe_unpin(pf);
			memcpy(page->pf_addr, pf->pf_addr, PAGE_SIZE);
			pframe_free(pf);
			return 0;
		}
	}
	return -1;
}
