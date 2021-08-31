# XV6-Recorder

## Goal
Record and Replay is a powerful feature for debugging.
The goal in this project was to add record and replay functionality onto the xv6 operating system.

## Stakes
This project was my final for my Operating Systems class.
It accounted for a large majority of my final grade.
We had three weeks to complete it.
It was due on the last day of UVA's finals week.

## Restrictions
Collaboration was not permitted.
However, we were allowed to ask clarifying questions on our piazza forum.
Additionally, only a few selected systemcalls needed to have record and replay functionality.

### What I Did
I began by collecting all the restrictions and limitations the project was going to have.
I collected all the details the professor gave in class, in the assignment, piazza, in discord, etc.
You can see those details in the v1/documentation and other related files.
After I learned as much as I could, I began with an outline for an algorithm.

I stored the solution I created in their own seperate directory so it would be easier to access from this github README.

### High Level Algorithm Overview
Have a variable to record when a record or replay session is occurring.
If it is in record mode, then capture and store inputs and outputs to system calls.
If it is in replay mode, replay the inputs and outputs for system calls.
Store both the inputs and outputs in some kind of way where you can retrieve them.
For each system call, add the right arguments and outputs.

## Result
By the end, I had created three seperate functioning versions.
After completing it the first time, I wanted to see how it could be improved.
After two more iterations and much testing I found my best version.


I ended up with a 100% for the final.
I was very happy as a result.
But I was just as much, if not more happy for the effort and thought I put into the last version.

## Highly Relevant Files
---
* **v1**
    * [recordreplay.h](v1/xv6-master/recordreplay.h) has the structures I am using to record/replay the systemcalls and their relevant functions.
    * [sysproc.h](v1/xv6-master/sysproc.c) uses the functions defined in the recordreplay.h
    * [sysfile.c](v1/xv6-master/sysfile.c) uses the functions defined in the recordreplay.h
    * [main.c](v1/main.c) has unit tests for the structures I would create and modify.

* **v2**
    * [rr.h](v2/xv6-master/rr.h) has the structures I am using to record/replay the systemcalls and their relevant functions signatures.
    * [sysproc.h](v2/xv6-master/sysproc.c) implements and uses the functions defined in the rr.h
    * [sysfile.c](v2/xv6-master/sysfile.c) uses the functions defined in the rr.h
    * [ts.c](v2/ts.c) has unit tests for the structures I would create and modify. More tests and more robust than before.
* **v3**
    * [rr.h](v3/rr.h) has the structures I am using to record/replay the systemcalls and their relevant functions signatures.
    * [sysproc.h](v3/sysproc.c) implements and uses the functions defined in the rr.h.
    * [sysfile.c](v3/sysfile.c) uses the functions defined in the rr.h.
		* [ts_samples.c](v3/ts_samples.c) tests samples test cases instructor provided.

		* [ts_rec.c](v3/ts_rec.c) tests record system call
		* [ts_rep.c](v3/ts_rep.c) tests replay system call

		* [ts_failures.c](v3/ts_failures.c) tests possible fail cases

		* [ts_getpid.c](v3/ts_getpid.c) tests getpid system call
		* [ts_uptime.c](v3/ts_uptime.c) tests uptime system call

		* [ts_open.c](v3/ts_open.c) tests open system call
		* [ts_read.c](v3/ts_read.c) tests read system call
		* [ts_close.c](v3/ts_close.c) tests close system call

		* [ts_fstat.c](v3/ts_fstat.c) tests fstat system call
		
		* [ts_mkdir.c](v3/ts_mkdir.c) tests mkdir system call
		* [ts_chdir.c](v3/ts_chdir.c) tests chdir system call

		* [ts_mknod.c](v3/ts_mknod.c) tests mknod system call
		* [ts_link.c](v3/unlink.c) tests link system call
		* [ts_unlink.c](v3/ts_unlink.c) tests unlink system call
---



