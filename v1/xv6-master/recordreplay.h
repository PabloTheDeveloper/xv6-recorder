#define MAX_RECORD_GROUP_SIZE 6

// denotes maximum size for fields for any record
#define MAX_FIELD_SIZE 4

/* constants for different types regarding record's fields*/
#define NONTYPE 0
#define INTTYPE 1
#define STRTYPE 2
#define STR_MAX_SIZE 200
#define IS_INPUT 1
#define IS_OUTPUT 0

/* **********************  Error Related Code *********************** */
enum ERR_CODE
{
	VALID,
	ADDITIONAL_SYS_CALL,
	MISMATCH_SYS_CALLS,
	INVALID_RECORD_SYS_CALL,
	INVALID_MAX_BOUND,
	NO_FIELDS_IN_RECORD,
	MISTMATCH_INPUT_OUTPUT_DECLARATION,
	TRYING_TO_READ_OUTPUT_WHEN_VALIDATING,
	INPROPER_ORDERING_OF_INPUT_OUTPUT,
	FIELD_UNINITIALIZED,
	FIELD_TYPE_MISMATCH,
	INT_VALUE_MISMATCH,
	STR_SIZE_MISMATCH,
	STR_VALUE_MISMATCH,
	STR_VALUE_MARKED_INVALID,
	STR_ATTEMPT_CONVERT_EXCEED_MAX, // Likewise as above
	INVALID_MAX_RECORD_BOUND,
	INVALID_RECORD_ACCESS,
	FIELD_TYPE_UNKNOWN,
};

/* **********************  String Struct *********************** */

struct __attribute__((packed)) String
{
	char isValid;
	int strSize;
	char content[STR_MAX_SIZE];
};

/* **********************  dataVal Union *********************** */

union dataVal
{
	int intVal;
	struct String strVal;
};

/* ********************** Field & Record Struct *********************** */
struct __attribute__((packed)) Field
{
	char isInput;
	char type;
	int intVal;
	struct String strVal;
};

struct __attribute__((packed)) Record
{
	char syscall;
	int fieldCount; //use in functions to validate size (it is modified and not static)
	struct Field fields[MAX_FIELD_SIZE];
};

/* ********************** Record Group Struct *********************** */
struct __attribute__((packed)) RecordGroup
{
	int counter; // it is modifable
	int size;
	struct Record records[MAX_RECORD_GROUP_SIZE];
};

char *getErrorMsg(int);
void errorHandling(struct RecordGroup *, int);

struct String stringConstructor(void);
int convertToStr(char *, struct String *);
struct Record recordConstructor(int);
void recordGroupConstructor(struct RecordGroup *);

int strsMatch(struct String, struct String);

int recordField(struct Record *, char, char, union dataVal);
int validateInputField(struct Record *, char, int, union dataVal);
int validateOutputField(struct Record *, char, int, union dataVal *);

int addRecord(struct RecordGroup *, struct Record);
int getRecord(struct RecordGroup *, struct Record *);

void printRecords(struct RecordGroup *, int);

extern struct RecordGroup rg;
struct RecordGroup rg;
