#include <stdio.h>

#define MAX_RECORD_GROUP_SIZE 10

// denotes maximum size for fields for any record
#define MAX_FIELD_SIZE 3

/* constants for different types regarding record's fields*/
#define NONTYPE 0
#define INTTYPE 1
#define STRTYPE 2
#define STR_MAX_SIZE 200

/* **********************  Error Related Code *********************** */
enum ERR_CODE
{
	VALID,
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
	// STR_ATTEMPT_CONVERT_IS_NULL,	// Error for convertToStr
	STR_ATTEMPT_CONVERT_EXCEED_MAX, // Likewise as above
	INVALID_MAX_RECORD_BOUND,
	INVALID_RECORD_ACCESS,
	FIELD_TYPE_UNKNOWN,
};

char *getErrorMsg(int errCode)
{
	switch (errCode)
	{
	case VALID:
		return "Valid\n";

	case INVALID_RECORD_SYS_CALL:
		return "ERR: invalid record SYS_CALL.\n";

	case INVALID_MAX_BOUND:
		return "ERR: attempted to access field past MAX_FIELD_SIZE.\n";

	case NO_FIELDS_IN_RECORD:
		return "ERR: no fields in record (there must be at least one field struct in a record.\n";

	case MISTMATCH_INPUT_OUTPUT_DECLARATION:
		return "ERR: field accessed is declared as Input or Output when it is the opposite.\n";

	case TRYING_TO_READ_OUTPUT_WHEN_VALIDATING:
		return "ERR: the validateInputField function is attempting to validate an output marked field struct.\n";

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

		// case STR_ATTEMPT_CONVERT_IS_NULL:
		// 	return "ERR: string field char* was NULL. This is an invalid value to pass.\n";

	case STR_ATTEMPT_CONVERT_EXCEED_MAX:
		return "ERR: string field attempted to convert past the maximum string threshold.\n";

	case INVALID_MAX_RECORD_BOUND:
		return "ERR: attempted to add more records than available in MAX_RECORD_GROUP_SIZE\n";

	case INVALID_RECORD_ACCESS:
		return "ERR: attempted to access uninitialized Record idx in Record Group\n";

	case FIELD_TYPE_UNKNOWN:
		return "ERR: non-declared field type present in field.type.\n";

	default:
		return "ERR: UNKNOWN ERROR CODE PASSED INTO 'getErrorMsg'.\n";
	}
}

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
	while (original[originalSize]) // inspired by strlen function in xv6-master/ulib.c
		originalSize++;

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

struct RecordGroup recordGroupConstructor()
{
	struct RecordGroup rg;
	for (int i = 0; i < MAX_RECORD_GROUP_SIZE; i++)
	{
		rg.records[i] = recordConstructor(0);
	}
	rg.size = 0;
	rg.counter = 0;
	return rg;
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

	if (record->fieldCount != 0 && (isInput && (record->fields[record->fieldCount - 1].isInput == 0)))
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
		if (currField.strVal.isValid)
			return STR_VALUE_MARKED_INVALID;
		return strsMatch(currField.strVal, val.strVal);
	default:
		return FIELD_TYPE_UNKNOWN;
	}
}

/* ********************** Record Group Methods *********************** */

int AddRecord(struct RecordGroup *rg, struct Record record)
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

int GetRecord(struct RecordGroup *rg, struct Record *record)
{
	if (record->syscall <= 0 || record->syscall > 27)
		return INVALID_RECORD_SYS_CALL;

	if (rg->counter >= MAX_RECORD_GROUP_SIZE)
	{
		return INVALID_MAX_RECORD_BOUND;
	}
	if (rg->counter >= rg->size)
	{
		return INVALID_RECORD_ACCESS;
	}
	*record = rg->records[rg->counter];
	rg->counter = rg->counter + 1;
	return VALID;
}


/* ********************** Testing/Logging Code *********************** */
char *starDivider = "\n*************************************************************************\n";
char *lineDivider = "\n-------------------------------------------------------------------------\n";

void test_create_header(char *msg)
{
	printf("%s%s\n%s", starDivider, msg, starDivider);
}

void test_create_footer(int passed, int total)
{
	printf("PASSED %d/%d%s", passed, total, lineDivider);
}

int test_error(int *total, char *msg, int expected, int errorCode)
{
	*total = *total + 1;
	int didPass = errorCode == VALID ? 1 == expected : 0 == expected;
	printf("\t(%s) %s:\n\t\tMSG:%s\n", didPass ? "PASSED" : "FAILED", msg, getErrorMsg(errorCode));
	return didPass;
}

int test_assert(int *total, char *msg, int assertedStmt)
{
	*total = *total + 1;
	printf("\t(%s) %s:\n\n", assertedStmt ? "PASSED" : "FAILED", msg);
	return assertedStmt;
}

int test_suit_convertToStr(int *globalTotal)
{
	test_create_header("TEST SUITE convertToStr(char * original, struct String* str):");
	int passed = 0;
	int total = 0;
	struct String str1 = stringConstructor();
	struct String str2 = stringConstructor();
	struct String str3 = stringConstructor();
	struct String str4 = stringConstructor();
	struct String str5 = stringConstructor();

	// For oneBelowMaxStr, atMaxStr, exceedMaxStr, I added an extra char to null terminate the string
	// This is done to simulate a  char *.

	// Actual Size: STR_MAX_SIZE -1
	char oneBelowMaxStr[STR_MAX_SIZE];
	for (int i = 0; i < STR_MAX_SIZE; i++)
		oneBelowMaxStr[i] = 'a';
	oneBelowMaxStr[STR_MAX_SIZE - 1] = '\0';

	// Actual Size: STR_MAX_SIZE
	char atMaxStr[STR_MAX_SIZE + 1];
	for (int i = 0; i < STR_MAX_SIZE + 1; i++)
		atMaxStr[i] = 'a';
	atMaxStr[STR_MAX_SIZE] = '\0';

	// Actual Size: STR_MAX_SIZE + 1
	char exceedMaxStr[STR_MAX_SIZE + 2];
	for (int i = 0; i < STR_MAX_SIZE + 2; i++)
		exceedMaxStr[i] = 'a';

	exceedMaxStr[STR_MAX_SIZE + 1] = '\0';

	// passed += test_error(&total, "CASE (NULL, str1)", 0, convertToStr(NULL, &str1));
	passed += test_error(&total, "CASE (\"\", str1)", 1, convertToStr("", &str1));
	passed += test_assert(&total, "CASE assert str1.isValid as true", str1.isValid);
	passed += test_assert(&total, "CASE assert str1.strSize is 0", str1.strSize == 0);
	passed += test_error(&total, "CASE (\" \", str2)", 1, convertToStr(" ", &str2));
	passed += test_assert(&total, "CASE assert str2.strSize is 1", str2.strSize == 1);

	passed += test_error(&total, "CASE (\"max str size-1\", str3)", 1, convertToStr(oneBelowMaxStr, &str3));
	passed += test_assert(&total, "CASE assert str2.strSize is 199", str3.strSize == STR_MAX_SIZE - 1);

	passed += test_error(&total, "CASE (\"at max str size\", str4)", 1, convertToStr(atMaxStr, &str4));
	passed += test_assert(&total, "CASE assert str4.strSize is 200", str4.strSize == STR_MAX_SIZE);

	passed += test_error(&total, "CASE (\">max str size\", str5)", 0, convertToStr(exceedMaxStr, &str5));
	passed += test_assert(&total, "CASE assert str5.isValid is false", str5.isValid == 0);

	// These are bad tests to setup since str5.strSize was never initialized.
	// printf("%d\n", str5.strSize);

	test_create_footer(passed, total);

	*globalTotal = *globalTotal + total;
	return passed;
}

int test_suit_strsMatch(int *globalTotal)
{
	test_create_header("TEST SUITE strsMatch(struct String* str, struct String* str):");
	int passed = 0;
	int total = 0;
	struct String str1 = stringConstructor();
	struct String str2 = stringConstructor();
	struct String str3 = stringConstructor();
	struct String str4 = stringConstructor();
	struct String str5 = stringConstructor();

	// For oneBelowMaxStr, atMaxStr, exceedMaxStr, I added an extra char to null terminate the string
	// This is done to simulate a  char *.

	// Actual Size: STR_MAX_SIZE -1
	char oneBelowMaxStr[STR_MAX_SIZE];
	for (int i = 0; i < STR_MAX_SIZE; i++)
		oneBelowMaxStr[i] = 'a';
	oneBelowMaxStr[STR_MAX_SIZE - 1] = '\0';

	// Actual Size: STR_MAX_SIZE
	char atMaxStr[STR_MAX_SIZE + 1];
	for (int i = 0; i < STR_MAX_SIZE + 1; i++)
		atMaxStr[i] = 'a';
	atMaxStr[STR_MAX_SIZE] = '\0';

	// Actual Size: STR_MAX_SIZE + 1
	char exceedMaxStr[STR_MAX_SIZE + 2];
	for (int i = 0; i < STR_MAX_SIZE + 2; i++)
		exceedMaxStr[i] = 'a';

	exceedMaxStr[STR_MAX_SIZE + 1] = '\0';

	passed += test_error(&total, "CASE (empty str1, empty str2)", 0, strsMatch(str1, str2));
	convertToStr(" ", &str2);
	passed += test_error(&total, "CASE (empty str1, \" \" str2)", 0, strsMatch(str1, str2));
	passed += test_error(&total, "CASE ( \" \" str2, empty str1)", 0, strsMatch(str2, str1));

	convertToStr(" ", &str1);
	passed += test_error(&total, "CASE (\" \" empty str1, \" \" str2)", 1, strsMatch(str1, str2));

	convertToStr(oneBelowMaxStr, &str3);
	convertToStr(oneBelowMaxStr, &str4);
	passed += test_error(&total, "CASE (\"max str size -1\" str3, \"max str size-1\" str4)", 1, strsMatch(str3, str4));

	convertToStr(atMaxStr, &str4);
	passed += test_error(&total, "CASE (\"max str size -1\" str3, \"max str size\" str4)", 0, strsMatch(str3, str4));
	passed += test_error(&total, "CASE (\"max str size\" str4, \"max str size-1\" str3)", 0, strsMatch(str4, str3));

	convertToStr(atMaxStr, &str3);
	passed += test_error(&total, "CASE (\"max str size \" str3, \"max str size\" str4)", 1, strsMatch(str3, str4));

	convertToStr(exceedMaxStr, &str5);
	passed += test_error(&total, "CASE (\"max str size \" str4, \">max str size\" str5)", 0, strsMatch(str4, str5));
	passed += test_error(&total, "CASE (\">max str size \" str5, \"max str size\" str4)", 0, strsMatch(str5, str4));

	test_create_footer(passed, total);
	*globalTotal = *globalTotal + total;
	return passed;
}

int test_suit_recordField(int *globalTotal)
{
	test_create_header("TEST SUITE recordField(struct Record* record, char type, char isInput, union dataValue val):");
	int passed = 0;
	int total = 0;
	struct Record r1 = recordConstructor(0);
	struct Record r2 = recordConstructor(27);
	struct Record r3 = recordConstructor(28);
	union dataVal val;
	val.intVal = 5;

	// for the following 3 tests, i don't need to try different datatypes for val
	passed += test_error(&total, "CASE r1(syscall=0)", 0, recordField(&r1, INTTYPE, 1, val));
	passed += test_assert(&total, "CASE assert r1.fieldCount=0", r1.fieldCount == 0);

	passed += test_error(&total, "CASE r2(syscall=27)", 1, recordField(&r2, INTTYPE, 1, val));
	passed += test_assert(&total, "CASE assert r2.fieldCount=1", r2.fieldCount == 1);

	passed += test_error(&total, "CASE r3(syscall=28)", 0, recordField(&r3, INTTYPE, 1, val));
	passed += test_assert(&total, "CASE assert r3.fieldCount=0", r3.fieldCount == 0);

	struct Record r4 = recordConstructor(1);
	for (int i = 0; i < MAX_FIELD_SIZE - 1; i++)
	{
		recordField(&r4, INTTYPE, 1, val);
	}
	passed += test_assert(&total, "CASE r4.fieldCount == MAX_FIELD_SIZE-1", r4.fieldCount == MAX_FIELD_SIZE - 1);
	passed += test_error(&total, "CASE r4(max_field_size-1)+1", 1, recordField(&r4, INTTYPE, 1, val));
	passed += test_assert(&total, "CASE r4.fieldCount == MAX_FIELD_SIZE", r4.fieldCount == MAX_FIELD_SIZE);
	passed += test_error(&total, "CASE r4(max_field_size)+1", 0, recordField(&r4, INTTYPE, 1, val));

	//tests ordering
	struct Record r5 = recordConstructor(2);
	recordField(&r5, INTTYPE, 0, val);
	passed += test_error(&total, "CASE ordering (output following input)", 0, recordField(&r5, INTTYPE, 1, val));
	passed += test_assert(&total, "CASE r5 did not change size from last operation", r5.fieldCount == 1);

	// test String types (valid and invalid)
	// Actual Size: STR_MAX_SIZE + 1

	struct Record r6 = recordConstructor(3);

	char exceedMaxStr[STR_MAX_SIZE + 2];
	for (int i = 0; i < STR_MAX_SIZE + 2; i++)
		exceedMaxStr[i] = 'a';

	exceedMaxStr[STR_MAX_SIZE + 1] = '\0';

	struct String str1 = stringConstructor();
	struct String str2 = stringConstructor();
	struct String str3 = stringConstructor();
	struct String str4 = stringConstructor();

	//convertToStr(NULL, &str1);
	convertToStr("", &str2);
	convertToStr(" ", &str3);
	convertToStr(exceedMaxStr, &str4);

	union dataVal str1val;
	union dataVal str2val;
	union dataVal str3val;
	union dataVal str4val;

	str1val.strVal = str1;
	str2val.strVal = str2;
	str3val.strVal = str3;
	str4val.strVal = str4;

	passed += test_error(&total, "r6 (null string)", 0, recordField(&r6, STRTYPE, 1, str1val));
	passed += test_assert(&total, "CASE r6 did not change size from last operation", r6.fieldCount == 0);

	passed += test_error(&total, "r6 (empty string)", 1, recordField(&r6, STRTYPE, 1, str2val));
	passed += test_assert(&total, "CASE r6.fieldCount = 1", r6.fieldCount == 1);

	passed += test_error(&total, "r6 (\" \" string)", 1, recordField(&r6, STRTYPE, 1, str3val));
	passed += test_assert(&total, "CASE r6.fieldCount = 2", r6.fieldCount == 2);

	passed += test_error(&total, "r6 (exceed max size string)", 0, recordField(&r6, STRTYPE, 1, str4val));
	passed += test_assert(&total, "CASE r6 remained = 2", r6.fieldCount == 2);

	test_create_footer(passed, total);

	*globalTotal = *globalTotal + total;
	return passed;
}

int test_suit_validateInputField(int *globalTotal)
{
	test_create_header("TEST SUITE validateInputField(struct Record* record, char type, int fieldIdx, union dataValue val):");
	int passed = 0;
	int total = 0;
	struct Record r1 = recordConstructor(0);
	struct Record r2 = recordConstructor(27);
	struct Record r3 = recordConstructor(28);
	union dataVal val;
	val.intVal = 5;
	// for the following 3 tests, i don't need to try different datatypes for val
	passed += test_error(&total, "CASE r1(syscall=0)", 0, validateInputField(&r1, INTTYPE, 0, val));
	passed += test_error(&total, "CASE r2(syscall=27)", 0, validateInputField(&r2, INTTYPE, 0, val));
	passed += test_error(&total, "CASE r3(syscall=28)", 0, validateInputField(&r3, INTTYPE, 0, val));

	recordField(&r2, INTTYPE, 1, val);

	passed += test_error(&total, "CASE r2 access 0th field input", 1, validateInputField(&r2, INTTYPE, 0, val));
	passed += test_error(&total, "CASE r2 acess 1th field input ", 0, validateInputField(&r2, INTTYPE, 1, val));
	val.intVal = 4;
	passed += test_error(&total, "CASE r2 access 0th field input (nonmatching values)", 0, validateInputField(&r2, INTTYPE, 0, val));
	passed += test_error(&total, "CASE r2 access 0th field input (nonmatching types)", 0, validateInputField(&r2, STRTYPE, 0, val));

	struct Record r4 = recordConstructor(1);
	for (int i = 0; i < MAX_FIELD_SIZE; i++)
	{
		recordField(&r4, INTTYPE, 1, val);
	}
	passed += test_assert(&total, "CASE r4.fieldCount == MAX_FIELD_SIZE (ensure that record is filled with fields)", r4.fieldCount == MAX_FIELD_SIZE);
	passed += test_error(&total, "CASE r4 access MAX_FIELD_SIZE -1", 1, validateInputField(&r4, INTTYPE, MAX_FIELD_SIZE - 1, val));
	passed += test_error(&total, "CASE r4 access MAX FIELD_SIZE (should fail)", 0, validateInputField(&r4, INTTYPE, MAX_FIELD_SIZE, val) != VALID);

	test_create_footer(passed, total);
	*globalTotal = *globalTotal + total;
	return passed;
}

int test_suit_addRecord(int *globalTotal)
{
	test_create_header("TEST SUITE addRecord(struct RecordGroup * rg, struct Record record):");
	int passed = 0;
	int total = 0;
	struct RecordGroup rg1 = recordGroupConstructor();
	struct Record r1 = recordConstructor(27);
	union dataVal val;
	val.intVal = 5;

	recordField(&r1, INTTYPE, 1, val);

	passed += test_error(&total, "CASE: adding first Record to rg1", 1, AddRecord(&rg1, r1));
	passed += test_error(&total, "CASE: adding second Record to rg1", 1, AddRecord(&rg1, r1));
	passed += test_assert(&total, "CASE: size of rg1 is 2", rg1.size == 2);

	for (int i = rg1.size; i < MAX_RECORD_GROUP_SIZE - 2; i++)
	{
		AddRecord(&rg1, r1);
	}
	passed += test_assert(&total, "CASE: size of rg1 is MAX_RECORD_GROUP_SIZE-2", rg1.size == MAX_RECORD_GROUP_SIZE - 2);
	passed += test_error(&total, "CASE: adding second to last Record to rg1", 1, AddRecord(&rg1, r1));
	passed += test_assert(&total, "CASE: size of rg1 is MAX_RECORD_GROUP_SIZE-1", rg1.size == MAX_RECORD_GROUP_SIZE - 1);
	passed += test_error(&total, "CASE: adding last (valid) Record to rg1", 1, AddRecord(&rg1, r1));
	passed += test_assert(&total, "CASE: size of rg1 is MAX_RECORD_GROUP_SIZE", rg1.size == MAX_RECORD_GROUP_SIZE);
	passed += test_error(&total, "CASE: add past the first record to rg1", 0, AddRecord(&rg1, r1));
	passed += test_assert(&total, "CASE: size of rg1 is not MAX_RECORD_GROUP_SIZE+1", rg1.size != MAX_RECORD_GROUP_SIZE + 1);
	test_create_footer(passed, total);
	*globalTotal = *globalTotal + total;
	return passed;
}

int test_suit_GetRecord(int *globalTotal)
{
	test_create_header("TEST SUITE getRecord(struct RecordGroup * rg, struct Record* record):");
	int passed = 0;
	int total = 0;
	struct RecordGroup rg1 = recordGroupConstructor();
	struct Record r2 = recordConstructor(26);
	struct Record r1 = recordConstructor(27);
	union dataVal val;
	val.intVal = 5;
	printf("counter is: %d\n", rg1.counter);
	printf("size is : %d\n", rg1.size);

	passed += test_error(&total, "CASE: get second to last record (supposed to fail)", 0, GetRecord(&rg1, &r2));

	for (int i = 0; i < MAX_RECORD_GROUP_SIZE - 1; i++)
	{
		AddRecord(&rg1, r1);
	}

	for (int i = 0; i < MAX_RECORD_GROUP_SIZE - 2; i++)
	{
		if (GetRecord(&rg1, &r2) != VALID)
		{
			printf("CASE: error getting elements from 0 to MAX_RECORD_GROUP_SIZE -2:");
		}
	}

	passed += test_error(&total, "CASE: get second to last record", 1, GetRecord(&rg1, &r2));
	passed += test_assert(&total, "CASE: size of rg1 is MAX_RECORD_GROUP_SIZE-1", rg1.size == MAX_RECORD_GROUP_SIZE - 1);
	passed += test_error(&total, "CASE: adding last (valid) Record to rg1", 1, AddRecord(&rg1, r1));
	passed += test_error(&total, "CASE: get last record", 1, GetRecord(&rg1, &r2));

	passed += test_assert(&total, "CASE: size of current size is MAX_RECORD_GROUP_SIZE", rg1.size == MAX_RECORD_GROUP_SIZE);
	passed += test_assert(&total, "CASE: size of current counter is MAX_RECORD_GROUP_SIZE", rg1.counter == MAX_RECORD_GROUP_SIZE);

	passed += test_error(&total, "CASE: get last record", 0, GetRecord(&rg1, &r2));
	passed += test_assert(&total, "CASE: size of current counter is MAX_RECORD_GROUP_SIZE", rg1.counter == MAX_RECORD_GROUP_SIZE);

	test_create_footer(passed, total);
	*globalTotal = *globalTotal + total;
	return passed;
}

int main()
{
	int globalTotal = 0;
	int globalPassed = 0;
	globalPassed += test_suit_convertToStr(&globalTotal);
	globalPassed += test_suit_strsMatch(&globalTotal);
	globalPassed += test_suit_recordField(&globalTotal);
	globalPassed += test_suit_validateInputField(&globalTotal);
	globalPassed += test_suit_addRecord(&globalTotal);
	globalPassed += test_suit_GetRecord(&globalTotal);
	test_create_header("ALL TESTS:");
	test_create_footer(globalPassed, globalTotal);
	return 0;
}