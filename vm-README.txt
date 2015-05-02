Documentation for Kernel Assignment 3
=====================================

+-------------+
| BUILD & RUN |
+-------------+

Comments: In Config.mk, set CS402INITCHOICE to 0 (user shell) or 1(kshell) depending on
 which test you want to run. To test Part B, set to 1, else set it to 0.
 sbin/halt doesn't work

+-----------------+
| SKIP (Optional) |
+-----------------+
Part D: Skip stress test, memtest
Part E: skip completely


+---------+
| GRADING |
+---------+

(A.1) In mm/pframe.c:
    (a) In pframe_pin(): 1 out of 1 pt
    (b) In pframe_unpin(): 1 out of 1 pt

(A.2) In vm/mmap.c:
    (a) In do_mmap(): 2 out of 2 pts
    (b) In do_munmap(): 2 out of 2 pts

(A.3) In vm/vmmap.c:
    (a) In vmmap_destroy(): 2 out of 2 pts
    (b) In vmmap_insert(): 2 out of 2 pts
    (c) In vmmap_find_range(): 2 out of 2 pts
    (d) In vmmap_lookup(): 1 out of 1 pt
    (e) In vmmap_is_range_empty(): 1 out of 1 pt
    (f) In vmmap_map(): 7 out of 7 pts

(A.4) In vm/anon.c:
    (a) In anon_init(): 1 out of 1 pt
    (b) In anon_ref(): 1 out of 1 pt
    (c) In anon_put(): 1 out of 1 pt
    (d) In anon_fillpage(): 1 out of 1 pt

(A.5) In fs/vnode.c:
    (a) In special_file_mmap(): 2 out of 2 pts

(A.6) In vm/shadow.c:
    (a) In shadow_init(): 1 out of 1 pt
    (b) In shadow_ref(): 1 out of 1 pt
    (c) In shadow_put(): 1 out of 1 pts
    (d) In shadow_fillpage(): 2 out of 2 pts

(A.7) In proc/fork.c:
    (a) In do_fork(): 6 out of 6 pts

(A.8) In proc/kthread.c:
    (a) In kthread_clone(): 2 out of 2 pts

(B.1) /usr/bin/hello (3 out of 3 pts)
(B.2) /bin/uname -a (3 out of 3 pts)
(B.3) /usr/bin/args ab cde fghi j (3 out of 3 pts)
(B.4) /usr/bin/fork-and-wait (5 out of 5 pts)

(C.1) /usr/bin/segfault (.5 out of 1 pt) (only runs once, second time prompt doesn/t return)

(D.2) /usr/bin/vfstest (7 out of 7 pts) (s5fs_vm test takes some time but runs, please don't be warned)
(D.3) /usr/bin/memtest (0 out of 7 pts)
(D.4) /usr/bin/eatmem (3.5 out of 7 pts) (runs once, so deserves 3.5 pts, don't be warned it takes very long time to free the pages and get the prompt back)
(D.5) /usr/bin/forkbomb (7 out of 7 pts) (runs for 1 minute, prompt returns by pressing enter)
(D.6) /usr/bin/stress (0 out of 7 pts)

(E.1) /usr/bin/vfstest (0 out of 1 pt)
(E.2) /usr/bin/memtest (0 out of 1 pt)
(E.3) /usr/bin/eatmem (0 out of 1 pt)
(E.4) /usr/bin/forkbomb (0 out of 1 pt)
(E.5) /usr/bin/stress (0 out of 1 pt)

(F) Self-checks: (10 out of 10 pts)
    Comments: (please provide details, add subsections and/or items as needed)

Missing required section(s) in README file (vm-README.txt): No missing sections
Submitted binary file : No
Submitted extra (unmodified) file : No
Wrong file location in submission : No
Use dbg_print(...) instead of dbg(DBG_PRINT, ...) : No
Not properly indentify which dbg() printout is for which item in the grading guidelines : No
Cannot compile : Compiles fine
Compiler warnings : 1 warning
"make clean" : works fine
Useless KASSERT : No useless kasserts
Insufficient/Confusing dbg : all dbg statements are consistent
Kernel panic : Yes, panics when shuts down due to vnode reference count problem
Cannot halt kernel cleanly : See above

+------+
| BUGS |
+------+

Comments: ?

+---------------------------+
| CONTRIBUTION FROM MEMBERS |
+---------------------------+

Equal contributions from mukin@usc.edu, yshankar@usc.edu, hansalsh@usc.edu, kmanek@usc.edu

+------------------+
| OTHER (Optional) |
+------------------+

Special DBG setting in Config.mk for certain tests: n/a
Comments on deviation from spec (you will still lose points, but it's better to let the grader know): N/a
General comments on design decisions: N/a

