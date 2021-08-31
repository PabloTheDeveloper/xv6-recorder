#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "fcntl.h"
#include "recordreplay.h"

/* ********************** Err Messages *********************** */

char *getErrorMsg(int errCode)
{
	switch (errCode)
	{
	case VALID:
		return "Valid\n";

	case ADDITIONAL_SYS_CALL:
		return "ERR: there is an additional system call being logged\n";

	case INVALID_RECORD_SYS_CALL:
		return "ERR: invalid record SYS_CALL.\n";

	case INVALID_MAX_BOUND:
		return "ERR: attempted to access field past MAX_FIELD_SIZE.\n";

	case MISMATCH_SYS_CALLS:
		return "ERR: System calls do not match.\n";

	case NO_FIELDS_IN_RECORD:
		return "ERR: no fields in record (there must be at least one field struct in a record).\n";

	case MISTMATCH_INPUT_OUTPUT_DECLARATION:
		return "ERR: field accessed is declared as Input or Output when it is the opposite.\n";

	case INPROPER_ORDERING_OF_INPUT_OUTPUT:
		return "ERR: field is being set as isInput=True, yet the prior field was isInput=False.\n";

	case FIELD_UNINITIALIZED:
		return "ERR: field was never initialized to a valid field.\n";

	case FIELD_TYPE_MISMATCH:
		return "ERR: field type mismatched with passed in field's type.\n";

	case INT_VALUE_MISMATCH:
		return "ERR: int field's value mismatched with passed in int field's value.\n";

	case STR_SIZE_MISMATCH:
		return "ERR: string field's size mismatch with passed in field's size.\n";

	case STR_VALUE_MISMATCH:
		return "ERR: string field's content mismatch with passed in field's contents.\n";

	case STR_VALUE_MARKED_INVALID:
		return "ERR: string field had isValid attribute set to 0.\n";

	case STR_ATTEMPT_CONVERT_EXCEED_MAX:
		return "ERR: string field attempted to convert past the maximum string threshold.\n";

	case INVALID_MAX_RECORD_BOUND:
		return "ERR: attempted to access more records than available in MAX_RECORD_GROUP_SIZE\n";

	case INVALID_RECORD_ACCESS:
		return "ERR: attempted to access uninitialized Record idx in Record Group\n";

	case FIELD_TYPE_UNKNOWN:
		return "ERR: non-declared field type present in field.type.\n";

	default:
		return "ERR: UNKNOWN ERROR CODE PASSED INTO 'getErrorMsg'.\n";
	}
}

/* *********************** Constructors ***************************** */
struct String stringConstructor()
{
	struct String str;
	str.isValid = 0;
	str.strSize = 0;
	for (int i = 0; i < STR_MAX_SIZE; i++)
		str.content[i] = '\0';
	return str;
}

int convertToStr(char *original, struct String *str)
{
	// set to zero
	str->isValid = 0;
	for (int i = 0; i < STR_MAX_SIZE; i++)
		str->content[i] = '\0';
	str->strSize = 0;

	// if (original == NULL) // Here is where I can account for the NULL value. I
	// 					  //could just force program panic as well..
	// 	return STR_ATTEMPT_CONVERT_IS_NULL;

	int originalSize = 0;
	for (originalSize = 0; original[originalSize]; originalSize++) // inspired by strlen function in xv6-master/ulib.c
		;

	if (originalSize > STR_MAX_SIZE)
		return STR_ATTEMPT_CONVERT_EXCEED_MAX;

	str->strSize = originalSize;
	for (int i = 0; i < str->strSize; i++)
	{
		str->content[i] = original[i];
	}

	str->isValid = 1;
	return VALID;
}

struct Record recordConstructor(int syscall)
{
	struct Record record;
	record.syscall = syscall;
	record.fieldCount = 0;
	for (int i = 0; i < MAX_FIELD_SIZE; i++)
	{
		record.fields[i].isInput = 0;
		record.fields[i].type = NONTYPE;
		record.fields[i].intVal = -1;
		record.fields[i].strVal = stringConstructor();
	}
	return record;
}

void recordGroupConstructor(struct RecordGroup *rg)
{
	for (int i = 0; i < MAX_RECORD_GROUP_SIZE; i++)
	{
		rg->records[i] = recordConstructor(0);
	}
	rg->size = 0;
	rg->counter = 0;
}

/* ********************** String Method *********************** */

int strsMatch(struct String str1, struct String str2)
{
	if (!str1.isValid || !str2.isValid)
		return STR_VALUE_MARKED_INVALID;

	if (str1.strSize != str2.strSize)
		return STR_SIZE_MISMATCH;

	for (int i = 0; i < str1.strSize; i++)
	{
		if (str1.content[i] != str2.content[i])
		{
			return STR_VALUE_MISMATCH;
		}
	}
	return VALID;
}

/* ********************** Record Methods *********************** */

int recordField(struct Record *record, char type, char isInput, union dataVal val)
{
	if (record->syscall <= 0 || record->syscall > 27)
		return INVALID_RECORD_SYS_CALL;

	if (record->fieldCount >= MAX_FIELD_SIZE)
		return INVALID_MAX_BOUND;

	if (record->fieldCount > 0 && (isInput && (record->fields[record->fieldCount - 1].isInput == 0)))
		return INPROPER_ORDERING_OF_INPUT_OUTPUT;

	struct Field newField;
	switch (type)
	{
	case INTTYPE:
		newField.intVal = val.intVal;
		break;

	case STRTYPE:
		// marked as such when error in creating it
		if (val.strVal.isValid == 0)
			return STR_VALUE_MARKED_INVALID;

		newField.strVal = val.strVal;
		break;

	default:
		return FIELD_TYPE_UNKNOWN;
	}

	newField.type = type;
	newField.isInput = isInput;
	record->fields[record->fieldCount] = newField;
	record->fieldCount++;

	return VALID;
}

int validateInputField(struct Record *record, char type, int fieldIdx, union dataVal val)
{
	if (record->syscall <= 0 || record->syscall > 27)
		return INVALID_RECORD_SYS_CALL;

	if (record->fieldCount > MAX_FIELD_SIZE)
		return INVALID_MAX_BOUND;

	if (record->fieldCount == 0)
		return NO_FIELDS_IN_RECORD;

	struct Field currField = record->fields[fieldIdx];

	if (currField.isInput == 0)
		return MISTMATCH_INPUT_OUTPUT_DECLARATION;

	if (currField.type == NONTYPE)
		return FIELD_UNINITIALIZED;

	if (currField.type != type)
		return FIELD_TYPE_MISMATCH;

	switch (currField.type)
	{
	case INTTYPE:
		if (currField.intVal != val.intVal)
			return INT_VALUE_MISMATCH;
		return VALID;
	case STRTYPE:
		if (!currField.strVal.isValid)
			return STR_VALUE_MARKED_INVALID;
		return strsMatch(currField.strVal, val.strVal);
	default:
		return FIELD_TYPE_UNKNOWN;
	}
}

int validateOutputField(struct Record *record, char type, int fieldIdx, union dataVal *val)
{
	if (record->syscall <= 0 || record->syscall > 27)
		return INVALID_RECORD_SYS_CALL;

	if (record->fieldCount > MAX_FIELD_SIZE)
		return INVALID_MAX_BOUND;

	if (record->fieldCount == 0)
		return NO_FIELDS_IN_RECORD;

	struct Field currField = record->fields[fieldIdx];

	if (currField.isInput == 1)
		return MISTMATCH_INPUT_OUTPUT_DECLARATION;

	if (currField.type == NONTYPE)
		return FIELD_UNINITIALIZED;

	if (currField.type != type)
		return FIELD_TYPE_MISMATCH;

	switch (currField.type)
	{
	case INTTYPE:
		val->intVal = currField.intVal;
		return VALID;
	case STRTYPE:
		val->strVal = currField.strVal;
		return VALID;
	default:
		return FIELD_TYPE_UNKNOWN;
	}
}

/* ********************** Record Group Methods *********************** */

int addRecord(struct RecordGroup *rg, struct Record record)
{
	if (record.syscall <= 0 || record.syscall > 27)
		return INVALID_RECORD_SYS_CALL;

	if (rg->size >= MAX_RECORD_GROUP_SIZE)
	{
		return INVALID_MAX_RECORD_BOUND;
	}
	rg->records[rg->size] = record;
	rg->size = rg->size + 1;
	return VALID;
}

int getRecord(struct RecordGroup *rg, struct Record *record)
{
	if (record->syscall <= 0 || record->syscall > 27)
		return INVALID_RECORD_SYS_CALL;

	if (rg->counter >= MAX_RECORD_GROUP_SIZE)
	{
		return INVALID_MAX_RECORD_BOUND;
	}
	if (rg->counter >= rg->size)
	{
		return ADDITIONAL_SYS_CALL;
	}
	// means I'm trying to access
	if (rg->records[rg->counter].syscall == 0)
	{
		return ADDITIONAL_SYS_CALL;
	}
	if (rg->records[rg->counter].syscall != record->syscall)
	{
		return MISMATCH_SYS_CALLS;
	}

	*record = rg->records[rg->counter];
	rg->counter = rg->counter + 1;
	return VALID;
}

void printRecords(struct RecordGroup *rg, int recordsToIterateUpto)
{

	for (int i = 0; i < recordsToIterateUpto; i++)
	{
		struct Record record = rg->records[i];
		cprintf("-----------");
		cprintf("RECORD (%d):%d", i, record.syscall);
		cprintf("-----------\n");
		int j;
		cprintf("\tINPUT:\n");
		for (j = 0; j < record.fieldCount && record.fields[j].isInput; j++)
		{
			struct Field field;
			field = record.fields[j];
			switch (field.type)
			{
			case INTTYPE:
				cprintf("\t\tint: %d\n", field.intVal);
				break;
			case STRTYPE:
				cprintf("\t\tstr: \"%s\" , str_size: %d\n", field.strVal.content, field.strVal.strSize);
				break;
			default:
				cprintf("\t\tNULL\n");
				break;
			}
		}
		cprintf("\tOUTPUT:\n");

		for (j = j; j < record.fieldCount && !record.fields[j].isInput; j++)
		{
			struct Field field;
			field = record.fields[j];
			switch (field.type)
			{
			case INTTYPE:
				cprintf("\t\tint: %d\n", field.intVal);
				break;
			case STRTYPE:
				cprintf("\t\tstr: \"%s\"\n", field.strVal);
				break;
			default:
				cprintf("\t\tNULL\n");
				break;
			}
		}
	}
}

/* ********************** Externally defined variables *********************** */

// record_syscalls - 1 means in record
extern uint record_syscalls;
// replay_syscalls - 1 means in replay
extern uint replay_syscalls;

extern char record_filename[256];
extern char replay_filename[256];

extern int my_open(char *, int);
extern int my_close(int);
extern int my_write(int, char *, int);
extern int my_read(int, char *, int);

#define PRINTLOG 1

void __strcpy(char *dest, char *src, int maxlen)
{
	int i = 0;
	for (i = 0; i < maxlen; i++)
	{
		dest[i] = src[i];
		if (src[i] == 0)
		{
			break;
		}
	}
	dest[maxlen - 1] = 0;
}

// SYS_record_start - records with a filename
// if filename already exists, overwrite it
int sys_record_start(void)
{
	char *filename;
	if (argstr(0, &filename) < 0)
	{
		return -1;
	}
	__strcpy(record_filename, filename, 255);
	record_syscalls = 1;
	recordGroupConstructor(&rg);

#if PRINTLOG
	cprintf("**********");
	cprintf(" sys_record_start: %d, %s ||| records: %d, records replayed: %d", record_syscalls, record_filename, rg.size, rg.counter);
	cprintf("**********\n");
	cprintf("PRINTF commands from terminal...:\n");
	cprintf("----------------\n");
#endif
	return record_syscalls;
}

int sys_record_stop(void)
{
	if (record_syscalls == 1)
	{
		record_syscalls = 0;

		// open new file
		int fd = my_open(record_filename, O_CREATE | O_RDWR);

		// write to file
		char buffer[sizeof(rg) + 1];
		memmove(&buffer, &rg, sizeof(rg));
		int w_status = my_write(fd, buffer, sizeof(buffer) - 1);

		// close file
		int c_status = my_close(fd);

#if PRINTLOG
		cprintf("\n----------------\n");
		cprintf("FILE STATUS - fd:%d, write:%d, close:%d:\n", fd, w_status, c_status);
		cprintf("----------------\n");
		cprintf("RECORDS:\n");
		printRecords(&rg, rg.size);
		cprintf("\n***********");
		cprintf(" sys_record_stop: %d, %s ||| records: %d, records replayed: %d", record_syscalls, record_filename, rg.size, rg.counter);
		cprintf("***********\n\n");
		;
#endif
		recordGroupConstructor(&rg);
	}

	return record_syscalls;
}

int sys_replay_start(void)
{
	char *filename;
	if (argstr(0, &filename) < 0)
	{
		return -1;
	}
	__strcpy(replay_filename, filename, 255);
	replay_syscalls = 1;

	recordGroupConstructor(&rg);

	// open file
	int fd = my_open(record_filename, O_CREATE | O_RDWR);

	// read from file
	char buffer[sizeof(rg) + 1];
	int r_status = my_read(fd, buffer, sizeof(buffer) - 1);

	// write from buffer into rg
	memmove(&rg, &buffer, sizeof(rg));

	// close file
	int c_status = my_close(fd);

#if PRINTLOG
	cprintf("**********");
	cprintf(" sys_replay_start: %d, %s ||| records: %d, records replayed: %d", replay_syscalls, replay_filename, rg.size, rg.counter);
	cprintf("**********\n");
	cprintf("FILE STATUS - fd:%d, read:%d, close:%d:\n", fd, r_status, c_status);
	cprintf("----------------\n");
	cprintf("PRINTF commands from terminal...:\n");
#endif

	return replay_syscalls;
}

int sys_replay_stop(void)
{
	if (replay_syscalls == 1)
	{
		replay_syscalls = 0;
#if PRINTLOG
		cprintf("\n----------------\n");
		cprintf("RECORDS:\n");
		printRecords(&rg, rg.size);
		cprintf("\n\n***********");
		cprintf(" sys_replay_stop: %d, %s ||| records: %d, records replayed: %d", replay_syscalls, replay_filename, rg.size, rg.counter);
		cprintf("***********\n\n");

		;

#endif
		recordGroupConstructor(&rg);
	}
	return replay_syscalls;
}

int sys_fork(void)
{
	return fork();
}

int sys_exit(void)
{
	sys_replay_stop();
	sys_record_stop();
	exit();
	return 0; // not reached
}

int sys_wait(void)
{
	return wait();
}

int sys_kill(void)
{
	int pid;

	if (argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

int sys_getpid(void)
{
	union dataVal val;
	struct Record record = recordConstructor(11);

	if (replay_syscalls == 1)
	{
		cprintf("syscall: %d, %d, rg.counter: %d\n", record.syscall, rg.records[rg.counter].syscall, rg.counter);
		errorHandling(&rg, getRecord(&rg, &record));
		errorHandling(&rg, validateOutputField(&record, INTTYPE, 0, &val));
		return val.intVal;
	}

	val.intVal = myproc()->pid; // this value is what is being return.
	// (original program is a one liner)

	if (record_syscalls == 1)
	{
		errorHandling(&rg, recordField(&record, INTTYPE, IS_OUTPUT, val));
		errorHandling(&rg, addRecord(&rg, record));
	}

	return val.intVal;
}

int sys_sbrk(void)
{
	int addr;
	int n;

	if (argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1;
	return addr;
}

int sys_sleep(void)
{
	int n;
	uint ticks0;

	if (argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n)
	{
		if (myproc()->killed)
		{
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
int sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

int sys_yield(void)
{
	yield();
	return 0;
}

int sys_shutdown(void)
{
	shutdown();
	return 0;
}

void errorHandling(struct RecordGroup *rg, int errCode)
{
	if (errCode != VALID)
	{
		cprintf("\nERROR!*********************************\n");
		cprintf("MSG: %s\n", getErrorMsg(errCode));
		if (record_syscalls == 1)
		{
			cprintf("HAPPENED RIGHT BEFORE SYSCALL INVOKATION (zeroth indexed) #%d\n", rg->size - 1);
			struct Record record = rg->records[rg->size - 1];

			if (record.fieldCount > 0)
			{
				cprintf("HAPPENED RIGHT BEFORE FIELD ACCESS (zeroth indexed) #%d\n", record.fieldCount - 1);
			}
		}

		if (replay_syscalls == 1)
		{
			int syscallPos = rg->counter - 1;
			if (errCode == MISMATCH_SYS_CALLS)
				syscallPos = syscallPos + 1;

			cprintf("HAPPENED AT REPLAY'S SYSCALL INVOKATION (zeroth indexed) #%d\n", syscallPos);
		}
		sys_replay_stop();
		sys_record_stop();
		exit();
	}
}