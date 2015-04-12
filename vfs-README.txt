Documentation for Kernel Assignment 2
=====================================

+-------------+
| BUILD & RUN |
+-------------+

Comments: No comments

+-----------------+
| SKIP (Optional) |
+-----------------+

Every test in the standard test-suite runs properly.

+---------+
| GRADING |
+---------+

(A.1) In fs/vnode.c:
    (a) In special_file_read(): 6 out of 6 pts
    (b) In special_file_write(): 6 out of 6 pts

(A.2) In fs/namev.c:
    (a) In lookup(): 6 out of 6 pts
    (b) In dir_namev(): 10 out of 10 pts
    (c) In open_namev(): 2 out of 2 pts

(3) In fs/vfs_syscall.c:
    (a) In do_write(): 6 out of 6 pts
    (b) In do_mknod(): 2 out of 2 pts
    (c) In do_mkdir(): 2 out of 2 pts
    (d) In do_rmdir(): 2 out of 2 pts
    (e) In do_unlink(): 2 out of 2 pts
    (f) In do_stat(): 2 out of 2 pts

(B) vfstest: 39 out of 39 pts
    Comments:  Please invoke vfs test from kshell using 'vfstest' command
	invoke faber_fs_thread_test using 'fs_thread_test' 
	invoke faber_directory_test using 'directory_test' commands in kshell

(C.1) faber_fs_thread_test (3 out of 3 pts)
(C.2) faber_directory_test (2 out of 2 pts)

(D) Self-checks: (10 out of 10 pts)
    Comments: By running all the tests in the above sections, all code paths are exercised

Missing required section(s) in README file (vfs-README.txt): Not missing any sections
Submitted binary file :  No binary files submitted
Submitted extra (unmodified) file : None
Wrong file location in submission : None
Use dbg_print(...) instead of dbg(DBG_PRINT, ...) : Nowhere
Not properly indentify which dbg() printout is for which item in the grading guidelines : Every dbg() printout properly identifies the item in grading guidelines
Cannot compile : Kernel is properly compiling
Compiler warnings : None
"make clean" : Works
Useless KASSERT : None
Insufficient/Confusing dbg : None
Kernel panic : No
Cannot halt kernel cleanly : Kernel halts cleanly

+------+
| BUGS |
+------+

None

+---------------------------+
| CONTRIBUTION FROM MEMBERS |
+---------------------------+

Equal Contribution from all members (hansalsh@usc.edu, yshankar@usc.edu, kmanek@usc.edu, mukhin@usc.edu)

+------------------+
| OTHER (Optional) |
+------------------+

Special DBG setting in Config.mk for certain tests: No
Comments on deviation from spec (you will still lose points, but it's better to let the grader know): There is no deviation from spec
General comments on design decisions: No
