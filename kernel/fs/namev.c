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
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
	/* NOT_YET_IMPLEMENTED("VFS: lookup"); */
	/*
	 * I assume that this function is only called from directory
	 */
		KASSERT(NULL != dir);
		dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
		KASSERT(NULL != name);
		dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

	/*	if(dir->vn_ops->lookup == NULL || !S_ISDIR(dir->vn_mode)){
			dbg(DBG_VFS, "INFO: lookup(): not a dir\n"); this doesn't ever get executed
			return -ENOTDIR;
		}*/
		if(len > NAME_LEN) {
			dbg(DBG_VFS, "INFO: lookup(): name too long\n");
			dbg(DBG_PRINT, "(GRADING2B)\n");
			return -ENAMETOOLONG;
		}

		if(0 == strcmp(name, ".")) { /* current directory */
			dbg(DBG_VFS, "INFO: lookup(): vnode requested for the current directory\n");
			vref(dir);
			*result = dir;
			dbg(DBG_PRINT, "(GRADING2B)\n");
			return 0; /* success */
		}
		/* how to get the vnode for the parent directory ???*/

		int lookup_res = dir->vn_ops->lookup(dir, name, len, result);
		if(lookup_res < 0) {
			dbg(DBG_VFS, "INFO: lookup(): error in FS lookup. Couldn't find the file (%s)\n", name);
			dbg(DBG_PRINT, "(GRADING2A)\n");
			return lookup_res;
		}

		KASSERT(NULL != result);
		dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

		return 0; /* success */
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: dir_namev");
        return 0;
    */
		KASSERT(NULL != pathname);
		dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
		KASSERT(NULL != namelen);
		dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
		KASSERT(NULL != name);
		dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
		KASSERT(NULL != res_vnode);
		dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

		vnode_t *dir = NULL;
		vnode_t *result = NULL;
		char *pname = (char *)pathname; /* component name */

	/*	if(strlen(pathname) > MAXPATHLEN) {
			return -ENAMETOOLONG; doesn't get executed
		}*/

		if(pathname[0] == '/') {
			dbg(DBG_VFS, "INFO: dir_namev(): root path\n");
			dir = vfs_root_vn;
			vref(dir);
			pname++;
			dbg(DBG_PRINT, "(GRADING2A)\n");
		} else if(NULL == base) {
			dbg(DBG_VFS, "INFO: dir_namev(): NULL base, use current directory\n");
			dir = curproc->p_cwd;
			dbg(DBG_PRINT, "(GRADING2A)\n");
			vref(dir);
		} /* else {  base not null doesn't execute
			dir = base;
			vref(dir);
		}*/

		char *separator = pname;

		int prev_sep_pos = 0;
		int plen = strlen(pname); /* component length */

		while (1) {
			pname = separator;
			separator = strchr(separator, '/');
			dbg(DBG_PRINT, "(GRADING2A)\n");
			if(separator!=NULL) {
				dbg(DBG_PRINT, "Separator found at : %d", separator-pathname+1);
				plen = (separator-pathname)-prev_sep_pos;
				if(pathname[0] == '/') {
					dbg(DBG_PRINT, "(GRADING2A)\n");
					plen--;
				}
				prev_sep_pos  = separator-pathname+1;
				separator = separator+1;
				dbg(DBG_PRINT, "(GRADING2A)\n");/* moving ahead of the matched character */
			}else { /* regular string */
				plen = strlen(pname);
				dbg(DBG_PRINT, "(GRADING2A)\n");
				break;
			}

			dbg(DBG_VFS, "INFO: calling lookup() (%s) (%d)\n", pname, plen);

        /*    if(NULL == dir){ doesn't get executed
            	dbg(DBG_VFS, "INFO: dir_namev(): lookup failed. a path element does not exist.\n");
                return -ENOENT;
            }*/
        /*    if (!S_ISDIR(dir->vn_mode)){ doesn't get executed
            	dbg(DBG_VFS, "INFO: dir_namev(): lookup failed. a path element is not a directory.\n");
                vput(dir);
                return -ENOTDIR;
            }*/
		/*	if(plen > NAME_LEN){ doesn't get executed
				dbg(DBG_VFS, "INFO: dir_namev(): lookup failed. Path component too long.\n");
				vput(dir);
				return -ENAMETOOLONG;
			}*/

			if(plen > 0) { /* empty name */
				int lookup_resp = lookup(dir, pname, plen, &result);
				dbg(DBG_PRINT, "(GRADING2A)\n");
				if(lookup_resp < 0){
					dbg(DBG_VFS, "INFO: lookup() failed with ret code (%d)\n", lookup_resp);
					vput(dir);
					dbg(DBG_PRINT, "(GRADING2B)\n");
					return lookup_resp;
				}
				vput(dir);
				KASSERT(NULL != result);
				dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
				dir = result;
			}

			/*if(1 ==  time_to_break) break;*/
		}
		if(!S_ISDIR(dir->vn_mode)){
			vput(dir);
			dbg(DBG_PRINT, "(GRADING2B)\n");
			return -ENOTDIR;
		}
	    *namelen = plen;
	    *res_vnode = dir;
	    *name = (const char*)pname;
	    dbg(DBG_VFS, "INFO: dir_namev(): call succeeded with ret code(len of comp) (%d)(%s).\n", *namelen, *name);

	    KASSERT(NULL!=*res_vnode);
		KASSERT(NULL != namelen);
	    KASSERT(NULL != name);

	    return 0; /* success */
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: open_namev");
        return 0;
    */
	/*
	 * It is called by open() to get the vnode of the respective file
	 */
		KASSERT(NULL != pathname);
        size_t namelen = 0;
        vnode_t *dir_res_vnode = NULL;
        const char *filename = NULL; dbg(DBG_PRINT, "(GRADING2A)\n");
        int dir_namev_resp = dir_namev(pathname, &namelen, &filename, base, &dir_res_vnode);
        if(dir_namev_resp<0) {
        	dbg(DBG_VFS, "INFO: open_namev(): call to dir_namev() failed with ret code (%d).\n", dir_namev_resp);
        	dbg(DBG_PRINT, "(GRADING2B)\n");
        	return dir_namev_resp;
        }
       /* if(!S_ISDIR(dir_res_vnode->vn_mode)){
        	dbg(DBG_VFS, "INFO: open_namev(): call to dir_namev() doesn't return dir.\n");
        	vput(dir_res_vnode); because dir_namev increments and returns
        	return -ENOTDIR;doesn't get executed
        }*/
        /* Look up whether the file exists */
        int file_lookup_res = lookup(dir_res_vnode, filename, namelen, res_vnode);
        if(file_lookup_res < 0) {
        	/* we may need to create a new file based on the flag */
        	if(flag & O_CREAT) {
        		dbg(DBG_VFS, "INFO: open_namev(): creating the requested file\n");
        		KASSERT(NULL != dir_res_vnode->vn_ops->create);
        		dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
        		int file_creation_res = dir_res_vnode->vn_ops->create(dir_res_vnode, filename, namelen, res_vnode);
        		if(file_creation_res < 0)
        		{
        			dbg(DBG_VFS, "INFO: open_namev(): file creation failed with ret code (%d)\n", file_creation_res);
        			vput(dir_res_vnode);
        			dbg(DBG_PRINT, "(GRADING2C 1)\n");
        			return file_creation_res;
        		}
        	}else { /* no request for file create */
				dbg(DBG_VFS, "INFO: open_namev(): call to lookup() failed.\n");
				vput(dir_res_vnode);
				dbg(DBG_PRINT, "(GRADING2B)\n");
				return file_lookup_res;
        	}
        }
        /*vput(res_vnode);*/
        vput(dir_res_vnode);
		return 0;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
