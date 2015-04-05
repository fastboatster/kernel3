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

/*
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND or O_CREAT.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int
do_open(const char *filename, int oflags)
{
	/*long if conditional to test the file access flags*/
	/*if(oflags!= O_RDONLY && oflags != O_WRONLY && oflags != O_RDWR && oflags != (O_RDONLY|O_CREAT)
			&& oflags != (O_WRONLY|O_CREAT) && oflags!=(O_RDWR|O_CREAT) && oflags!=(O_RDONLY|O_APPEND)
			&& oflags!= (O_WRONLY|O_APPEND) && oflags!= (O_RDWR|O_APPEND)) {

		return -EINVAL;
	}*/
	int new_fd = get_empty_fd(curproc);
	if(new_fd == -EMFILE) {
		/**/
		return -EMFILE;
	}
	if(strlen(filename) > NAME_LEN) return -ENAMETOOLONG;

	file_t* new_file = fget(-1);
	if(new_file == NULL) {
		/**/
		dbg(DBG_VFS, "Null file (%s), (%d)\n", filename,new_fd);
		return -ENOMEM;
	}
	/*set the mode*/
	/*O_RDONLY, O_WRONLY, O_RDWR, O_RDONLY|O_APPEND,O_WRONLY|O_APPEND,O_RDWR|O_APPEND, O_RDONLY|O_CREAT,O_WRONLY|O_CREAT,O_RDWR|O_CREAT*/
	int write_found  =0;
	dbg(DBG_PRINT, "Trying to open %s\n", filename);
	dbg(DBG_PRINT, "Flags (%d)\n",oflags);
	if(oflags == O_RDONLY || oflags == (O_RDONLY|O_CREAT))
		new_file->f_mode = FMODE_READ;
	else if(oflags == O_WRONLY || oflags == (O_WRONLY|O_CREAT)){
		write_found = 1;
		new_file->f_mode = FMODE_WRITE;
	}
	else if(oflags == O_RDWR || oflags == (O_RDWR|O_CREAT)){
		write_found = 1;
		new_file->f_mode = FMODE_READ | FMODE_WRITE;
	}
	else if(oflags == (O_RDONLY|O_APPEND))
		new_file->f_mode = FMODE_READ | FMODE_APPEND;
	else if(oflags == (O_WRONLY|O_APPEND)){
		write_found = 1;
		new_file->f_mode = FMODE_WRITE | FMODE_APPEND;
	}
	else if(oflags == (O_RDWR|O_APPEND)){
		write_found = 1;
		new_file->f_mode = FMODE_READ | FMODE_WRITE | FMODE_APPEND;
	}
	else {
		fput(new_file);
		return -EINVAL;
	}

	vnode_t *file_vnode = NULL;
	int open_resp = open_namev(filename, oflags, &file_vnode, NULL);
	if(open_resp < 0) {
		fput(new_file);
		return open_resp;
	}

	if(S_ISDIR(file_vnode->vn_mode) && write_found){
		fput(new_file);
		vput(file_vnode);
		return -EISDIR;
	}

	if((S_ISCHR(file_vnode->vn_mode) || S_ISBLK(file_vnode->vn_mode)) && NULL == file_vnode->vn_devid){
		fput(new_file);
		vput(file_vnode);
		return -ENXIO;
	}

	new_file->f_vnode = file_vnode;
	new_file->f_pos = 0;

	curproc->p_files[new_fd] = new_file;
	return new_fd;
      /*  NOT_YET_IMPLEMENTED("VFS: do_open");
        return -1;*/
}
