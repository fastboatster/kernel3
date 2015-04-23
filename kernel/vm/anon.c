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

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage  = anon_fillpage,
        .dirtypage = anon_dirtypage,
        .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void
anon_init()
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_init");
    */
	anon_allocator = slab_allocator_create("anon", sizeof(mmobj_t));
    KASSERT(anon_allocator);
    dbg(DBG_PRINT, "(GRADING3A 4.a)\n");
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
anon_create()
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_create");
        return NULL;
    */
	mmobj_t *new_anon_obj = (mmobj_t*)slab_obj_alloc(anon_allocator);
	if(new_anon_obj) {
		mmobj_init(new_anon_obj, &anon_mmobj_ops); /* initialize the object */
		/*anon_ref(new_anon_obj);*/
		new_anon_obj->mmo_un.mmo_bottom_obj = NULL;
		new_anon_obj->mmo_refcount++; /*do we need this at all?*/
	};
	return new_anon_obj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_ref");
    */
	KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "(GRADING3A 4.b)\n");
	o->mmo_refcount++;
	return;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_put");
    */
	KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "(GRADING3A 4.c)\n");
	/*o->mmo_ops->put(o);*/
	o->mmo_refcount--;
	if(o->mmo_refcount == o->mmo_nrespages) { /* mmobj no longer in use */
		pframe_t *page = NULL;
		list_iterate_begin(&o->mmo_respages, page, pframe_t, pf_olink) {
		    while(pframe_is_busy(page)) {
		    	sched_sleep_on(&(page->pf_waitq));
		    }
			if(pframe_is_pinned(page)) {
				pframe_unpin(page);
			} else if(pframe_is_dirty(page)){
				pframe_clean(page);
			} else {
				pframe_free(page); /* uncache all the pages */
			}
		}list_iterate_end();
		slab_obj_free(anon_allocator, o); /* free the object */
	}
	return;
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_lookuppage");
        return -1;
    */
	pframe_t* page = NULL;
	pframe_get(o, pagenum, &page);
	if(page) {
	    *pf = page;
	    return 0;
	}
	/* page not found  */
	return -1;
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_fillpage");
        return 0;
    */
	KASSERT(pframe_is_busy(pf));
	KASSERT(!pframe_is_pinned(pf));

	/* get the page from the given frame */
	/*pframe_t *page = pframe_get_resident(pf->pf_obj,pf->pf_pagenum);
	if(page) {
		memcpy(pf->pf_addr, page->pf_addr, PAGE_SIZE);
		if(!pframe_is_pinned(page)) {
			pframe_pin(page);
		}
		return 0;
	}
	return -1; */
	pframe_set_busy(pf);
	pframe_pin(pf);
	memset(pf->pf_addr, 0, PAGE_SIZE);
	pframe_clear_busy(pf);
	return 0;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_dirtypage");
        return -1;
    */
	if(!pframe_is_dirty(pf)) {
		pframe_set_dirty(pf);
	}
	if(pframe_is_dirty(pf)) {
		return 0;
	}
	return -1;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
	/*
        NOT_YET_IMPLEMENTED("VM: anon_cleanpage");
        return -1;
    */
	KASSERT(o);
	KASSERT(pf);
	/*
	pframe_t* page = NULL;
	list_iterate_begin(&o->mmo_respages, page, pframe_t, pf_olink) {
		if(page->pf_pagenum == pf->pf_pagenum) {
			pframe_clean(pf);
			return 0;
		}
	}list_iterate_end();
	return -1;
	*/
	pframe_clear_dirty(pf);
	return 0;
}
