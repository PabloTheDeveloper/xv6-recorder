#include "types.h"
#include "x86.h"
#include "date.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"
#include "stat.h"
#include "rr.h"
#include "syscall.h"
#include "fcntl.h"

extern uint record_syscalls;
extern uint replay_syscalls;
extern char record_filename[256];
extern char replay_filename[256];

extern int my_open(char *, int);
extern int my_close(int);
extern int my_write(int, char *, int);
extern int my_read(int, char *, int);

// used to check record and replay status
int isRecOrReplay(){
	return record_syscalls == 1 && replay_syscalls == 1 && rr_pid == myproc()->pid;
}

int isRecording(){
	return record_syscalls == 1 && rr_pid == myproc()->pid;
}

int isReplaying(){
	return replay_syscalls == 1 && rr_pid == myproc()->pid;
}

void RecRepErr(char * msg){
	cprintf("%s\n", msg);
	myproc()->killed = 1;
	replay_syscalls = -1;
	record_syscalls = -1;
}

char* getSyscallName(int syscall) {
  switch (syscall) {
  case UNITIALIZED_SCR:
    return "uninitialized (invalid)";
  case SYS_getpid:
  	return "getpid";
	case SYS_uptime:
		return "uptime";
	case SYS_close:
		return "close";
	case SYS_open:
		return "open";
	case SYS_read:
		return "read";
	case SYS_fstat:
		return "fstat";
	case SYS_mkdir:
		return "mkdir";
	case SYS_chdir:
		return "chdir";
	case SYS_mknod:
		return "mknod";
	case SYS_unlink:
		return "unlink";
	case SYS_link:
		return "link";
  default:
    break;
  };
  return "unknown syscall!";
}

int get1stOutputPos(int syscall) {
  switch (syscall) {
	case SYS_uptime:
  case SYS_getpid:
  	return 0;
	case SYS_fstat:
	case SYS_close:
	case SYS_mkdir:
	case SYS_chdir:
	case SYS_unlink:
		return 1;
	case SYS_read:
	case SYS_open:
	case SYS_link:
		return 2;
	case SYS_mknod:
		return 3;
  default:
    break;
  };
  return -1; // intentionally cause error in program execution
}

void printSCR(struct SCR scr) {
  char *syscall_name = getSyscallName(scr.syscall);
  int inputsEnd = get1stOutputPos(scr.syscall);
  cprintf("------------------------------------");
  cprintf("\nSyscall counter: %d, syscall: %s", scrs_idx, syscall_name);
  for (int i = 0; i < inputsEnd; i++) {
    switch (scr.args[i].type) {
    case INT_TYPE:
      cprintf("\nInput: (int: %d)", scr.args[i].val.num);
      break;
		case BYTE_TYPE:
      cprintf("\nInput: (raw: (size: %d, data: %s)", scr.args[i].val.raw.size, scr.args[i].val.raw.data);
			break;
    default:
      break;
    }
  }

  for (int i = inputsEnd; i < scr.argc; i++) {
    switch (scr.args[i].type) {
    case INT_TYPE:
      cprintf("\nOutput: (int: %d)", scr.args[i].val.num);
      break;
		case UINT_TYPE:
      cprintf("\nOutput: (uint: %d)", scr.args[i].val.unum);
      break;
		case BYTE_TYPE:
      cprintf("\nOUtput: (raw: (size: %d, data: %s)", scr.args[i].val.raw.size, scr.args[i].val.raw.data);
      break;
		case STAT_TYPE:
			cprintf(
      "\nOutput: (stat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)",
      scr.args[i].val.st.type,
      scr.args[i].val.st.dev,
      scr.args[i].val.st.ino,
      scr.args[i].val.st.nlink,
      scr.args[i].val.st.size
    	);
			break;
    default:
      break;
    }
  }
  cprintf("\n------------------------------------\n");
}

void addInt(struct SCR *scr, int val) {
  if (scr->argc >= MAX_ARGC)
    RecRepErr("ERR: exceeded MAX_ARGC when adding a int to a syscall.\n");

  struct Arg arg;
  arg.type = INT_TYPE;
  arg.val.num = val;
  scr->args[scr->argc] = arg;
  scr->argc++;
}

void addUInt(struct SCR *scr, uint val) {
  if (scr->argc >= MAX_ARGC)
    RecRepErr("ERR: exceeded MAX_ARGC when adding a uint to a syscall.\n");

  struct Arg arg;
  arg.type = UINT_TYPE;
  arg.val.unum = val;
  scr->args[scr->argc] = arg;
  scr->argc+=1;
}


void addStr(struct SCR *scr, char * buffer) {
	struct Raw raw;
	if((raw.size = strlen(buffer)) > MAX_RAW_SIZE)
		RecRepErr("\nERR: attempted to read more bytes into a Raw field than allowed in record & replay session\n");

	for (int i = 0; i < MAX_RAW_SIZE; i++)
		raw.data[i] = '\0';
	
	for (int i =0; i < raw.size; i++)
		raw.data[i] = buffer[i];
	
  if (scr->argc >= MAX_ARGC)
    RecRepErr("ERR: exceeded MAX_ARGC when adding a raw to a syscall.\n");
 
  struct Arg arg;
  arg.type = BYTE_TYPE;
  arg.val.raw = raw;
  scr->args[scr->argc] = arg;
  scr->argc+=1;
}

void addRaw(struct SCR *scr, char * buffer, int size)
{
	struct Raw raw;
	if(size > MAX_RAW_SIZE)
		RecRepErr("\nERR: attempted to read more bytes into a Raw field than allowed in record & replay session\n");

	for (int i = 0; i < MAX_RAW_SIZE; i++)
		raw.data[i] = '\0';

	memmove(&raw.data, buffer, size);
	raw.size = size;

  if (scr->argc >= MAX_ARGC)
    RecRepErr("ERR: exceeded MAX_ARGC when adding a raw to a syscall.\n");
 
  struct Arg arg;
  arg.type = BYTE_TYPE;
  arg.val.raw = raw;
  scr->args[scr->argc] = arg;
  scr->argc+=1;
}

void addStat(struct SCR *scr, struct stat st)
{
	struct myStat val;
  if (scr->argc >= MAX_ARGC)
    RecRepErr("ERR: exceeded MAX_ARGC when adding a stat struct to a syscall.\n");
	
	val.nlink = st.nlink;
	val.ino = st.ino;
	val.size = st.size;
	val.dev = st.dev;
	val.type = st.type;

  struct Arg arg;
  arg.type = STAT_TYPE;
  arg.val.st = val;
  scr->args[scr->argc] = arg;
  scr->argc+=1;;
}

struct SCR createSCR(int syscall) {
  struct SCR scr;
  scr.syscall = syscall;
  scr.argc = 0;
  for (int i = 0; i < MAX_ARGC; i++) {
    struct Arg arg;
    arg.type = NONE_TYPE;
		// empties data (makes strings print cleaner)
		for (int j = 0; j < MAX_RAW_SIZE; j++)
			arg.val.raw.data[j] = '\0';
		
    scr.args[i] = arg;
  }
  return scr;
}

void addSRC(struct SCR scr) {
#if PRINTLOG
  printSCR(scr);
#endif
  if (scrs_idx >= MAX_SCRS){
    RecRepErr("ERR: exceeded scrs_idx (More than 1000 records attempted to be added)\n");
		return;
	}
  recorded_scrs[scrs_idx] = scr;
  scrs_idx++;
}

struct SCR compareSCRS(struct SCR scr) {
	// gets scr from recorded_scrs.
  if (scrs_idx >= MAX_SCRS){
    RecRepErr("ERR: exceeded scrs_idx (More than 1000 records attempted to be gotten)\n");
		return createSCR(UNITIALIZED_SCR);
	}
  struct SCR recorded_scr = recorded_scrs[scrs_idx];
#if PRINTLOG
  printSCR(recorded_scr);
#endif

scrs_idx++;

  if (recorded_scr.syscall == UNITIALIZED_SCR){
    RecRepErr("ERR: Attempted to replay more records than records available\n");
		return recorded_scr;
	}

  if (recorded_scr.syscall != scr.syscall) {
    cprintf("\nrecorded syscall: %s,\nwhile attempted replayed syscall: %s.\nAt replayed syscall invokation #: %d\n",
            getSyscallName(recorded_scr.syscall), getSyscallName(scr.syscall), scrs_idx);
		RecRepErr("ERR: mismatch syscalls\n");
		return recorded_scr;
  }
  for (int i = 0;
       i < get1stOutputPos(recorded_scr.syscall);
       i++) {
    
		switch (recorded_scr.args[i].type){
		case INT_TYPE:
			if (recorded_scr.args[i].val.num != scr.args[i].val.num) {
				cprintf("\nERR: Mismatched input field values at syscall "
							"counter: %d\n"
							"field position: %d\n"
							"Type of values are: INT_TYPE\n"
							"Recorded field value: %d\n"
							"Replayed field value: %d\n",
							scrs_idx - 1,
							i,
							recorded_scr.args[i].val.num,
							scr.args[i].val.num);
				RecRepErr("");
				return recorded_scr;
			}
			break;

		case BYTE_TYPE:
			if (recorded_scr.args[i].val.raw.size != scr.args[i].val.raw.size) {
				cprintf("\nERR: Mismatched input field values at a syscall "
							"counter: %d\n"
							"field position: %d\n"
							"Type of values are: BYTE_TYPE\n"
							"Recorded field value (size) : %d\n"
							"Replayed field value (size): %d\n"
							"Recorded field value (data[]): %s\n"
							"Replayed field value (data[]): %s\n",
							scrs_idx-1,
							i,
							recorded_scr.args[i].val.raw.size,
							scr.args[i].val.raw.size,
							recorded_scr.args[i].val.raw.data,
							scr.args[i].val.raw.data
							);
				RecRepErr("");
				return recorded_scr;
				break;
			}

			int isSame = 1;
			for (int j = 0; j < scr.args[i].val.raw.size; j++){
				isSame = scr.args[i].val.raw.data[j] == recorded_scr.args[i].val.raw.data[j];
				if(isSame == 0){
					cprintf("\nERR: Mismatched input field values at aaa syscall "
							"counter: %d\n"
							"field position: %d\n"
							"Type of values are: BYTE_TYPE\n"
							"Recorded field value (data[%d](char)): %d\n"
							"Replayed field value (data[%d](char)): %d\n",

							scrs_idx - 1,
							i,
							j,
							recorded_scr.args[i].val.raw.data[j],
							j,
							scr.args[i].val.raw.data[j]
							);
					break;
				}
			}
			if(isSame == 0){
				RecRepErr("");
				return recorded_scr;
				break;
			}
			break;
		default:
			RecRepErr("unhandle type in comparison function.");
			return recorded_scr;
			break;
		}
	}
	return recorded_scr;
}

void memmove_and_increment(void * dst, void *src, int size){
	if(replay_syscalls == 1){
		memmove(src, dst, size);
	}
	if(record_syscalls == 1){
		memmove(dst, src, size);
	}
	buffer_pos += size;

}

int bidirectionalTransform( struct SCR *scr, int fileSize){
	if (buffer_pos > MAX_FILE_SIZE){
		cprintf("ERR: the program with record and replay exceeds the maximum file size!\n");
		return -1;
	}
	
	if(replay_syscalls == 1 && buffer_pos > fileSize){
		cprintf("ERR: Size exceed of file trying to read");
		return -1;
	}
	if (record_syscalls == 1 && scr->syscall == UNITIALIZED_SCR)
		return 0;
	
	memmove_and_increment(&buffer[buffer_pos], &scr->syscall, 1);

	if (replay_syscalls == 1 && scr->syscall == UNITIALIZED_SCR)
		return 0;

	memmove_and_increment(&buffer[buffer_pos], &scr->argc, 1);

	for (int i = 0; i < scr->argc; i++){

		if(record_syscalls == 1 && scr->args[i].type == NONE_TYPE){
			cprintf("ERR: trying to write a null type");
			return -1;
		}

		memmove_and_increment(&buffer[buffer_pos], &scr->args[i].type, 1);

		if(replay_syscalls == 1 && scr->args[i].type == NONE_TYPE){
			cprintf("ERR: trying to read a null type");
			return -1;
		}

		switch (scr->args[i].type){
		case INT_TYPE:
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.num, 4);
			break;

		case UINT_TYPE:
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.unum, 4);
			break;

		case BYTE_TYPE:
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.raw.size, 4);
			memmove_and_increment(
				&buffer[buffer_pos],
				&scr->args[i].val.raw.data,
				scr->args[i].val.raw.size
			);
			break;
		case STAT_TYPE:
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.st.type, 2);
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.st.dev, 4);
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.st.ino, 4);
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.st.nlink, 2);
			memmove_and_increment(&buffer[buffer_pos], &scr->args[i].val.st.size, 4);
			break;

		default:
			RecRepErr("ERR: using unknown type in arg\n"); // shouldn't ever be called. 
			break;
		}
	}
	return 1;
}

void __strcpy(char *dest, char *src, int maxlen) {
  int i = 0;
  for (i = 0; i < maxlen; i++) {
    dest[i] = src[i];
    if (src[i] == 0) {
      break;
    }
  }
  dest[maxlen - 1] = 0;
}

int sys_record_start(void) {
  char *filename;
  if (argstr(0, &filename) < 0) {
    return -1;
  }
  __strcpy(record_filename, filename, 255);
  record_syscalls = 1;
#if PRINTLOG
  cprintf("sys_record_start: %d, %s\n", record_syscalls, record_filename);
#endif
	// resets all global values
	for (int i = 0; i < MAX_SCRS; i++){
		recorded_scrs[i] = createSCR(UNITIALIZED_SCR);
	}
	scrs_idx = 0;
	rr_pid = myproc()->pid;
	buffer_pos = 0;

  return record_syscalls;
}

int sys_record_stop(void) {
  if (isRecording()) {
#if PRINTLOG
    cprintf("sys_record_stop: %d\n", record_syscalls);
#endif
		// /* efficiently storing recorded_scrs */
		buffer_pos = 0;
		memmove_and_increment(buffer, &scrs_idx, 4);
		for (int i = 0; i < scrs_idx; i++)
		{
			int compress_status = bidirectionalTransform(&recorded_scrs[i], -1);
			if(compress_status == -1){
				RecRepErr("");
				return -1;
			}
// 
			if(compress_status == 0)
				break;
		}
		record_syscalls = 0;
#if PRINTLOG
		cprintf("amount of bytes attempting to write: %d\n", buffer_pos);
#endif
// 
		/* IO */
		int fd, w_status, c_status;
// 
		// open file
		if ((fd = my_open(record_filename, O_CREATE | O_RDWR)) == -1){
			cprintf("\nERR: (Record Stop). Error opening file: %d\n", fd);
			record_syscalls = -1;
			cprintf("sys_record_stop: %d\n\n", record_syscalls);
			RecRepErr("");
			return record_syscalls;
		}
// 
		// write to file
		if ((w_status = my_write(fd, buffer, buffer_pos)) == -1){
			cprintf("\nERR: (Record Stop). Error in writing file: %d, write_status: %d\n", fd, w_status);
			record_syscalls = -1;
			cprintf("sys_record_stop: %d\n\n", record_syscalls);
			RecRepErr("");
			return record_syscalls;
		}
// 
		// close file
		if ((c_status = my_close(fd)) == -1){
			cprintf("\nERR: (Record Stop). Error close file: %d, close_status: %d\n", fd, c_status);
			record_syscalls = -1;
			cprintf("sys_record_stop: %d\n\n", record_syscalls);
			RecRepErr("");
			return record_syscalls;
		}


#if PRINTLOG
	cprintf("\n(Record Stop) record_filename_fd: %d, write: %d, close: %d\n", fd, w_status, c_status);
	cprintf("sys_record_stop: %d\n", record_syscalls);
	cprintf("\n**************************************************************\n");
#endif
	}

  return record_syscalls;
}

int sys_replay_start(void) {
  char *filename;
  if (argstr(0, &filename) < 0) {
    return -1;
  }
  __strcpy(replay_filename, filename, 255);
  replay_syscalls = 1;
#if PRINTLOG
  cprintf("sys_replay_start: %d, %s\n", replay_syscalls, replay_filename);
#endif

	// resets all global values
	for (int i = 0; i < MAX_SCRS; i++){
		recorded_scrs[i] = createSCR(UNITIALIZED_SCR);
	}
	scrs_idx = 0;
	rr_pid = myproc()->pid;
	buffer_pos = 0;

	int fd, r_status, c_status;

	/* IO */
	// open file
	if ((fd = my_open(replay_filename, O_RDWR)) == -1){
		cprintf("\nERR: (Replay Start). Error opening file: %d\nLikely, file \'%s\' does not exist\n", fd, replay_filename);
		replay_syscalls = -1;
		RecRepErr("");
		return replay_syscalls;
	}

	// read file
	if ((r_status = my_read(fd, buffer, sizeof(buffer))) == -1){
		cprintf("\nERR: (Replay Start). Error reading file: %d, r_status: %d\n\n", fd, r_status);
		replay_syscalls = -1;
		RecRepErr("");
		return replay_syscalls;;
	}

	// close file
	if ((c_status = my_close(fd)) == -1){
		cprintf("\nERR: (Replay Start). Error close file: %d, close_status: %d\n\n", fd, c_status);
		replay_syscalls = -1;
		RecRepErr("");
		return replay_syscalls;;
	}

	/* retrieving stored (compressed) recorded_scrs */

	memmove_and_increment(buffer, &scrs_idx, 4);

	// uncompress recorded_scrs
	for (int i = 0; i < scrs_idx; i++)
	{
		int compress_status = bidirectionalTransform(&recorded_scrs[i], r_status);
		if(compress_status == -1)
			return -1;

		if(compress_status == 0)
			break;
	}
	scrs_idx = 0; // (used in other functions

#if PRINTLOG
	cprintf("\n(Replay Start) replay_filename_fd: %d, read: %d, close: %d\n", fd, r_status, c_status);
#endif

  return replay_syscalls;
}

int sys_replay_stop(void) {
  if (isReplaying()) {
    replay_syscalls = 0;
#if PRINTLOG
    cprintf("sys_replay_stop: %d\n", replay_syscalls);
		cprintf("\n**************************************************************\n");
#endif
  }

  return replay_syscalls;
}

int sys_fork(void) { return fork(); }

int sys_exit(void) {
  sys_replay_stop();
  sys_record_stop();
  exit();
  return 0; // not reached
}

int sys_wait(void) { return wait(); }

int sys_kill(void) {
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int sys_getpid(void) {

  struct SCR scr = createSCR(SYS_getpid);
	if(isReplaying()){
		struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
	}
  if (isRecording()) {
  	int output = myproc()->pid;
 		addInt(&scr, output);
    addSRC(scr);
		return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  return myproc()->pid;
}

int sys_sbrk(void) {
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int sys_sleep(void) {
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void) {
  uint xticks;

  struct SCR scr = createSCR(SYS_uptime);
	if(isReplaying()){
		struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
	}
  if (isRecording()) {
		acquire(&tickslock);
		xticks = ticks;
		release(&tickslock);
 		addUInt(&scr, xticks);
    addSRC(scr);
		return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
	return xticks;
}

int sys_yield(void) {
  yield();
  return 0;
}

int sys_shutdown(void) {
  shutdown();
  return 0;
}
