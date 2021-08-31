#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "fcntl.h"
#include "rr.h"
#include "stat.h"
#include "syscall.h"

extern uint record_syscalls;
extern uint replay_syscalls;
extern char record_filename[256];
extern char replay_filename[256];

extern int my_open(char *, int);
extern int my_close(int);
extern int my_write(int, char *, int);
extern int my_read(int, char *, int);

struct Syscalls syscalls;
int syscall_idx = 0;
int rr_pid = -1;


void
initNone(struct Call * rr_syscall, int pos)
{
	struct Field field;
#if INTERNAL_DEVELOPMENT_LOG
	if(pos >= MAX_FIELD_SIZE)
		panic("In sysproc.c, initNone, you have attempted to add more fields than allowed");
#endif
	struct Byte bytes;
	for (int i = 0; i < MAX_BYTE_SIZE; i ++)
		bytes.content[i] = '\0';
	
	field.type = NONE;
	field.data.bytes = bytes;
	rr_syscall->fields[pos] = field;
}

void
setInt(struct Call * rr_syscall, int num)
{
	struct Field field;
	field.data.num = num;
	field.type = INT;

#if INTERNAL_DEVELOPMENT_LOG
	if(rr_syscall->size >= MAX_FIELD_SIZE)
		panic("In sysproc.c, SetInt, you have attempted to add more fields than allowed");
#endif
	rr_syscall->fields[rr_syscall->size] = field;
	rr_syscall->size = rr_syscall->size + 1;
}

void
setUint(struct Call * rr_syscall, uint unum)
{
	struct Field field;
	field.data.num = unum;
	field.type = UINT;

#if INTERNAL_DEVELOPMENT_LOG
	if(rr_syscall->size >= MAX_FIELD_SIZE)
		panic("In sysproc.c, SetUint, you have attempted to add more fields than allowed");
#endif
	rr_syscall->fields[rr_syscall->size] = field;
	rr_syscall->size = rr_syscall->size + 1;
}
struct Byte transformNULLTerminatedStrToByte(char * buffer)
{
	struct Byte bytes;
	if((bytes.size = strlen(buffer)) > MAX_BYTE_SIZE)
	{
		cprintf("\nERR: attempted to read more bytes (>512) into a Byte field than allowed in record & replay session\n");
		replay_syscalls = -1;
		record_syscalls = -1;
		exit();
	}

	for (int i = 0; i < MAX_BYTE_SIZE; i ++)
		bytes.content[i] = '\0';
	
	for (int i =0; i < bytes.size; i++)
		bytes.content[i] = buffer[i];
	
	return bytes;
}

struct Byte transformCharPointerToByte(char * buffer, int size)
{
	struct Byte bytes;
	if(size > MAX_BYTE_SIZE)
	{
		cprintf("\nERR: attempted to read more bytes (>512) into a Byte field than allowed in record & replay session\n");
		replay_syscalls = -1;
		record_syscalls = -1;
		exit();
	}

	for (int i = 0; i < MAX_BYTE_SIZE; i ++)
		bytes.content[i] = '\0';

	memmove(&bytes.content, buffer, size);
	bytes.size = size;
	return bytes;
}

void
setByte(struct Call * rr_syscall, struct Byte bytes)
{
	struct Field field;
	field.data.bytes = bytes;
	field.type = BYTE;

#if INTERNAL_DEVELOPMENT_LOG
	if(rr_syscall->size >= MAX_FIELD_SIZE)
		panic("In sysproc.c, SetByte, you have attempted to add more fields than allowed");
#endif
	rr_syscall->fields[rr_syscall->size] = field;
	rr_syscall->size = rr_syscall->size + 1;
}

struct PackedStat
transformStatPointerToPackedStat(struct stat * st)
{
	struct PackedStat pstat;
	pstat.type = st->type;
	pstat.dev = st->dev;
	pstat.ino = st->ino;
	pstat.nlink = st->nlink;
	pstat.size = st->size;
	return pstat;
}

void
setPackedStat(struct Call * rr_syscall, struct PackedStat pstat)
{
	struct Field field;
	field.data.pstat = pstat;
	field.type = PSTAT;

#if INTERNAL_DEVELOPMENT_LOG
	if(rr_syscall->size >= MAX_FIELD_SIZE)
		panic("In sysproc.c, setPackedStat, you have attempted to add more fields than allowed");
#endif
	rr_syscall->fields[rr_syscall->size] = field;
	rr_syscall->size = rr_syscall->size + 1;

}

union Data
getFieldData(struct Call * replayed, int pos)
{
#if INTERNAL_DEVELOPMENT_LOG
	if(pos >= MAX_FIELD_SIZE)
		panic("In sysproc.c, getFieldData, you have attempted to get more fields than allowed");
#endif
	return replayed->fields[pos].data;
}

struct Call
createSyscall(int syscall)
{
	struct Call call;
	call.syscall = syscall; 
	call.size = 0;
	for (int i = 0; i < MAX_FIELD_SIZE; i++)
		initNone(&call, i);
	return call;
}

void
resetSyscalls(struct Syscalls * SYSCALLS, int * IDX)
{
	*IDX = 0;
	for(int i = 0; i < MAX_SYS_CALLS; i++)
		SYSCALLS->calls[i] = createSyscall(UNINITIALIZED);
}

void
addSyscall(struct Syscalls * SYSCALLS, struct Call recorded, int *IDX)
{
	if(*IDX >= MAX_SYS_CALLS)
	{
		cprintf("\nERR: attempted to add more than %d records\n", MAX_SYS_CALLS);
		record_syscalls = -1;
		exit(); // safe to use so long as addSyscall is called only on the pid matching correctly
		// Proffessor Yonghwi Kwon @8:43 of the first video on Project C said to "print error and exit".
	}
	else
	{
		SYSCALLS->calls[*IDX] = recorded;
		*IDX = *IDX + 1;
	}
}
void
compareSyscalls(struct Call * replayed, struct Call * recorded, int *IDX)
{

	for (int i = 0; i < MAX_FIELD_SIZE; i++)
	{
		if(replayed->fields[i].type != recorded->fields[i].type)
		{
			cprintf("\nERR: Mismatched field type's at syscall counter: %d, field position: %d\n", *IDX, i+1);
			cprintf("Argument position: %d, replayed type: %d, record type: %d\n", replayed->fields[i].type, recorded->fields[i].type);
			replay_syscalls = -1;
			exit();
			break;
		}
		else
		{
			int type = replayed->fields[i].type;
			union Data recordedData = recorded->fields[i].data;
			union Data replayedData = replayed->fields[i].data;

			if(type == NONE)
			{
			}
			else if (type == INT)	
			{
				if(replayedData.num != recordedData.num)
				{
					cprintf("\nERR: Mismatched field values at syscall counter: %d, field position: %d\n", *IDX, i+1);
					cprintf("Type of values are: %d\n", INT);
					cprintf("Recorded field value: %d, Replayed field value: %d\n", recordedData.num, replayedData.num);
					replay_syscalls = -1;
					exit();
					break;
				}
			}
			else if (type == UINT)
			{
				if(replayedData.num != recordedData.num)
				{
					cprintf("\nERR: Mismatched field type's at syscall counter: %d, field position: %d\n", *IDX, i+1);
					cprintf("Type of values are: %d, (value will be presented in int form)\n", UINT);	
					cprintf("Recorded field value: %d, Replayed field value: %d\n", (int) recordedData.unum, (int) replayedData.unum);
					replay_syscalls = -1;
					exit();
					break;
				}
			}
			// I have this just in case, a byte input is passed
			else if (type == BYTE)
			{
				if(replayedData.bytes.size != recordedData.bytes.size)
				{
					cprintf("\nERR: Mismatched Byte field sizes, at syscall counter: %d, field position: %d\n", *IDX, i+1);
					cprintf("Type of values are: %d, (value will be presented in int form)\n", BYTE);	
					cprintf("Recorded Byte field size: %d, Replayed Byte field size: %d\n", recordedData.bytes.size, replayedData.bytes.size);
					replay_syscalls = -1;
					exit();
					break;
				}
				int break_all = 0;
				for (int i = 0; i < replayedData.bytes.size; i++)
				{
					if(replayedData.bytes.content[i] != recordedData.bytes.content[i])
					{
						cprintf("\nERR: Mismatched Byte field content values, at syscall counter: %d, field position: %d\n", *IDX, i+1);
						cprintf("Type of values are: %d, (value will be presented in int form)\n", BYTE);	
						cprintf("Recorded Byte field content: %s, Replayed Byte field content: %s\n", recordedData.bytes.content, replayedData.bytes.content);
						replay_syscalls = -1;
						break_all = 1;
						exit();
						break;
					}
				}
				if(break_all)
					break;
			}
			else if (type == PSTAT)
			{
				; // does nothing this value is only an output in all syscall
			}
			else
			{
				cprintf("\nERR: Unknown field type: %d, at syscall counter: %d, field position: %d\n",replayed->fields[i].type, syscall_idx, i+1);
				replay_syscalls = -1;
				exit();
				break;
			}
		}
	}
	
}

struct Call
getSyscall(struct Syscalls * SYSCALLS, int *IDX)
{
	int idx = *IDX;
	if(*IDX >= MAX_SYS_CALLS)
	{
		cprintf("\nERR: attempted to get more than 1000 records\n");
		record_syscalls = -1; // maked both as failure as getSyscall can be called on either record ore replay session
		replay_syscalls = -1;
		exit();
	}
	else
		*IDX = *IDX + 1;

	return SYSCALLS->calls[idx];
}

int
compress(struct Call call, char * buffer, int *bfileSize)
{
	if(call.syscall == UNINITIALIZED)
		return -1;

	memmove(&buffer[*bfileSize], &call.syscall , CHAR_SIZE);
	*bfileSize += CHAR_SIZE;

	memmove(&buffer[*bfileSize], &call.size , CHAR_SIZE);
	*bfileSize += CHAR_SIZE;

	for (int i = 0; i < call.size; i++)
	{
		if(call.fields[i].type == NONE)
			return 0;
		
		memmove(&buffer[*bfileSize], &call.fields[i].type, CHAR_SIZE);
		*bfileSize += CHAR_SIZE;

		switch (call.fields[i].type)
		{
			case INT:
			{
				memmove(&buffer[*bfileSize], &call.fields[i].data.num, INT_SIZE);
				*bfileSize += INT_SIZE;
				break;
			}
			case UINT:
			{
				memmove(&buffer[*bfileSize], &call.fields[i].data.unum, UINT_SIZE);
				*bfileSize += UINT_SIZE;
				break;
			}
			case PSTAT:
			{
				memmove(&buffer[*bfileSize], &call.fields[i].data.pstat.type, SHORT_SIZE);
				*bfileSize += SHORT_SIZE;
				memmove(&buffer[*bfileSize], &call.fields[i].data.pstat.dev, INT_SIZE);
				*bfileSize += INT_SIZE;
				memmove(&buffer[*bfileSize], &call.fields[i].data.pstat.ino, UINT_SIZE);
				*bfileSize += UINT_SIZE;
				memmove(&buffer[*bfileSize], &call.fields[i].data.pstat.nlink, SHORT_SIZE);
				*bfileSize += SHORT_SIZE;
				memmove(&buffer[*bfileSize], &call.fields[i].data.pstat.size, UINT_SIZE);
				*bfileSize += UINT_SIZE;
				break;
			}
			case BYTE:
			{
				memmove(&buffer[*bfileSize], &call.fields[i].data.bytes.size, INT_SIZE);
				*bfileSize += INT_SIZE;
				memmove(&buffer[*bfileSize], &call.fields[i].data.bytes.content, call.fields[i].data.bytes.size);
				*bfileSize += call.fields[i].data.bytes.size;
				break;
			}
			default:
				break;
		}
	}
	return 0;
}

int
uncompress(struct Call * call, char * buffer, int*bfileRead, int bufferSize)
{
		if(*bfileRead >= bufferSize){
			cprintf("bfileRead > : %d", *bfileRead);
			return -1;
		}

		memmove(&call->syscall, &buffer[*bfileRead], CHAR_SIZE);
		*bfileRead += CHAR_SIZE;
	
		memmove(&call->size, &buffer[*bfileRead], CHAR_SIZE);
		*bfileRead += CHAR_SIZE;
	
		// knowing when to stop (shouldn't be called however since we don't record unit calls)
		if(call->syscall == UNINITIALIZED){
			cprintf("\n uninit syscall: %d\n", call->syscall);
			return -1;
		}
		
		// if some error happened on recording of file
		if (call->syscall < 0 || call->syscall > 27)
		{
			cprintf("\nINVALID SYS_* number. number is: %d\n", call->syscall);
			return -1;
		}
		// cprintf("\nfieldsize is: %d\n", call->size);
		for (int i = 0; i < call->size; i++)
		{
			memmove(&call->fields[i].type, &buffer[*bfileRead], CHAR_SIZE);
			*bfileRead += CHAR_SIZE;
			switch (call->fields[i].type)
			{
				case INT:
				{
					memmove(&call->fields[i].data.num, &buffer[*bfileRead], INT_SIZE);
					*bfileRead += INT_SIZE;
					break;
				}
				case UINT:
				{
					memmove(&call->fields[i].data.unum, &buffer[*bfileRead], UINT_SIZE);
					*bfileRead += UINT_SIZE;
					break;
				}
				case BYTE:
				{
					memmove(&call->fields[i].data.bytes.size, &buffer[*bfileRead], INT_SIZE);
					*bfileRead += INT_SIZE;
					memmove(&call->fields[i].data.bytes.content, &buffer[*bfileRead], call->fields[i].data.bytes.size);
					*bfileRead += call->fields[i].data.bytes.size;
					break;
				}
				case PSTAT:
				{
					memmove(&call->fields[i].data.pstat.type, &buffer[*bfileRead], SHORT_SIZE);
					*bfileRead += SHORT_SIZE;
					memmove(&call->fields[i].data.pstat.dev, &buffer[*bfileRead], INT_SIZE);
					*bfileRead += INT_SIZE;
					memmove(&call->fields[i].data.pstat.ino, &buffer[*bfileRead], UINT_SIZE);
					*bfileRead += UINT_SIZE;
					memmove(&call->fields[i].data.pstat.nlink, &buffer[*bfileRead], SHORT_SIZE);
					*bfileRead += SHORT_SIZE;
					memmove(&call->fields[i].data.pstat.size, &buffer[*bfileRead], UINT_SIZE);
					*bfileRead += UINT_SIZE;
					break;
				}
				default:
					break;
			}
		}

		return 0;
}

void __strcpy(char* dest, char* src, int maxlen)
{
	int i = 0;
	for( i = 0; i < maxlen; i++ ) {
		dest[i] = src[i];
		if( src[i] == 0 ) {
			break;
		}
	}
	dest[maxlen-1] = 0;
}

int sys_record_start(void)
{
	char* filename;
	if( argstr(0, &filename) < 0 ) {
		return -1;
	}
	__strcpy(record_filename, filename, 255);
	record_syscalls = 1;
	replay_syscalls = 0;
#if PRINTLOG
	cprintf("sys_record_start: %d, %s", record_syscalls, record_filename);
#endif
	resetSyscalls(&syscalls, &syscall_idx);
	rr_pid = myproc()->pid;
	return record_syscalls;
}

int sys_record_stop(void)
{
	if( record_syscalls == 1 ) {
		record_syscalls = 0;
		//
		// you probably need to save all the recorded system calls here
		//
		int fd, w_status, c_status;

		// open file
		if ((fd = my_open(record_filename, O_CREATE | O_RDWR)) == -1)
		{
			cprintf("\nERR: (Record Stop). Error opening file: %d\n", fd);
			record_syscalls = -1;
			cprintf("sys_record_stop: %d\n\n", record_syscalls);
			exit();
			return record_syscalls;
		}

		// storing each syscall in buffer
		char buffer[60000];
		int bfileSize = 0;
		for (int i = 0; i < syscall_idx; i++)
		{
			if(compress(syscalls.calls[i], buffer, &bfileSize)  == -1)
				break;
		}
		// cprintf("bfileSize:%d\n", bfileSize);
		// write to file
		if ((w_status = my_write(fd, buffer, bfileSize)) == -1)
		{
			cprintf("\nERR: (Record Stop). Error in writing file: %d, write_status: %d\n", fd, w_status);
			record_syscalls = -1;
			cprintf("sys_record_stop: %d\n\n", record_syscalls);
			exit();
			return record_syscalls;
		}

		// close file
		if ((c_status = my_close(fd)) == -1)
		{
			cprintf("\nERR: (Record Stop). Error close file: %d, close_status: %d\n", fd, c_status);
			record_syscalls = -1;
			cprintf("sys_record_stop: %d\n\n", record_syscalls);
			exit();
			return record_syscalls;
		}

		rr_pid = -1;

#if PRINTLOG
	cprintf("\n(Record Stop) record_filename_fd: %d, write: %d, close: %d\n", fd, w_status, c_status);
	cprintf("sys_record_stop: %d\n", record_syscalls);
	cprintf("\n**************************************************************\n");
#endif
	}

	return record_syscalls;
}

int sys_replay_start(void)
{
	char* filename;
	if( argstr(0, &filename) < 0 ) {
		return -1;
	}	
	__strcpy(replay_filename, filename, 255);
	replay_syscalls = 1;
	record_syscalls = 0;
#if PRINTLOG
	cprintf("sys_replay_start: %d, %s", replay_syscalls, replay_filename);	
#endif	
	resetSyscalls(&syscalls, &syscall_idx);
	rr_pid = myproc()->pid;

	//
	// open the recorded file, and get the data ready for replay.
	// then close the file.
	//
	int fd, r_status, c_status;


	// open file
	if ((fd = my_open(replay_filename, O_RDWR)) == -1)
	{
		cprintf("\nERR: (Replay Start). Error opening file: %d\nLikely, file \'%s\' does not exist\n", fd, replay_filename);
		replay_syscalls = -1;
		exit();
		return replay_syscalls;
	}

	// read file
	char buffer[60000];	
	if ((r_status = my_read(fd, buffer, sizeof(buffer))) == -1)
	{
		cprintf("\nERR: (Replay Start). Error reading file: %d, r_status: %d\n\n", fd, r_status);
		replay_syscalls = -1;
		exit();
		return replay_syscalls;;
	}
	// uncompress file into global variable syscalls 
	int uncompressed_bytes = 0;
	for (int i = 0; i < MAX_SYS_CALLS; i++)
	{
		if(uncompress(&syscalls.calls[i], buffer, &uncompressed_bytes, r_status) == -1)
			break;
	}
	cprintf("\nuncompressed bytes: %d\n", uncompressed_bytes);
	
	// close file
	if ((c_status = my_close(fd)) == -1)
	{
		cprintf("\nERR: (Replay Start). Error close file: %d, close_status: %d\n\n", fd, c_status);
		replay_syscalls = -1;
		exit();
		return replay_syscalls;;
	}

#if PRINTLOG
	cprintf("\n(Replay Start) replay_filename_fd: %d, read: %d, close: %d", fd, r_status, c_status);
#endif

	return replay_syscalls;
}

int sys_replay_stop(void)
{
	if( replay_syscalls == 1 ) {
		replay_syscalls = 0;
#if PRINTLOG		
		cprintf("\nsys_replay_stop: %d\n", replay_syscalls);
		cprintf("\n**************************************************************\n");

#endif		
	}
	rr_pid = -1;
	return replay_syscalls;
}



int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
	sys_replay_stop();
	sys_record_stop();
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
	int curr_pid = myproc()->pid;

	if(curr_pid == rr_pid && replay_syscalls == 1)
	{
		struct Call recorded = getSyscall(&syscalls, &syscall_idx);
		if (recorded.syscall == UNINITIALIZED)
		{
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			kill(rr_pid);
		}
		else
		{
			struct Call replayed = createSyscall(SYS_getpid);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 0).num);
			cprintf("------------------------------------");
#endif
			if(recorded.syscall == replayed.syscall)
			{
				setInt(&replayed, getFieldData(&recorded, 0).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
				return getFieldData(&recorded, 0).num;	
			}

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
		}
	}
	// using already declared gotten value.
  int pid = curr_pid;

	if(curr_pid == rr_pid && record_syscalls == 1)
	{
		struct Call recorded = createSyscall(SYS_getpid);
		setInt(&recorded, pid);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 0).num);
		cprintf("------------------------------------");
#endif
	}
	return pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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
int
sys_uptime(void)
{
  uint xticks;

	int curr_pid = myproc()->pid;
	if(curr_pid == rr_pid && replay_syscalls == 1)
	{
		struct Call recorded = getSyscall(&syscalls, &syscall_idx);
		if (recorded.syscall == UNINITIALIZED)
		{
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			kill(rr_pid);
		}
		else
		{
			struct Call replayed = createSyscall(SYS_uptime);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 0).unum);
			cprintf("------------------------------------");
#endif
			if(recorded.syscall == replayed.syscall)
			{
				setUint(&replayed, getFieldData(&recorded, 0).unum);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
				return getFieldData(&recorded, 0).unum;	
			}

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
		}
	}
	// below three lines are the remaining part of the code
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);

	if(curr_pid == rr_pid && record_syscalls == 1)
	{
		struct Call recorded = createSyscall(SYS_uptime);
		setUint(&recorded, xticks);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 0).unum);
		cprintf("------------------------------------");
#endif
	}

  return xticks;
}

int
sys_yield(void)
{
  yield();
  return 0;
}

int sys_shutdown(void)
{
  shutdown();
  return 0;
}

