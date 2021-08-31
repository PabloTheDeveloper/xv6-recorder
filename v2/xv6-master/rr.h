#define PRINTLOG 1
#define INTERNAL_DEVELOPMENT_LOG 1

// Defining Maximum values
#define MAX_SYS_CALLS 1000
#define MAX_FIELD_SIZE 5
#define MAX_BYTE_SIZE 250

/* constants for different types regarding record's fields*/
#define NONE 0
#define BYTE 1
#define INT 2
#define UINT 3
#define PSTAT 4

#define CHAR_SIZE 1
#define SHORT_SIZE 2
#define INT_SIZE 4
#define UINT_SIZE 4

#define IS_INPUT 1
#define IS_OUTPUT 0

#define UNINITIALIZED 0

// **************** STRUCTS **********************
struct __attribute__((packed)) Byte {
  int size;
  char content[MAX_BYTE_SIZE];
};

// a packed version of the stat struct
// ref: 'stat.h'
struct __attribute__((packed)) PackedStat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
  uint size;   // Size of file in bytes
};

union Data {
  int num;
  uint unum;
  struct Byte bytes;
  struct PackedStat pstat;
};

struct __attribute__((packed)) Field {
  unsigned char type;
  union Data data;
};

struct __attribute__((packed)) Call {
  unsigned char syscall;
  unsigned char size;
  struct Field fields[MAX_FIELD_SIZE];
};

struct __attribute__((packed)) Syscalls {
  struct Call calls[MAX_SYS_CALLS];
};

// **************** Global Variables **********************

// Holds all the Syscalls
extern struct Syscalls syscalls;

// Used to keep track of idx of syscall recorded or replayed
extern int syscall_idx;

// Used to track pid of process using record replay functionality
extern int rr_pid;

// **************** Functions **********************

// initializes a None typed Field from some pos
void initNone(struct Call *, int);

// for a syscall, adds a int type Field of value 'num'
void setInt(struct Call *, int);

// for a syscall, adds a uint type Field of value 'unum'
void setUint(struct Call *, uint);

// to transform strings into the Byte format
struct Byte transformNULLTerminatedStrToByte(char *);

// to transform any buffer of bytes into Byte format
struct Byte transformCharPointerToByte(char *, int);

// for a syscall, adds a byte type Field of value 'bytes'
void setByte(struct Call *, struct Byte);

// to transform any stat pointer into PackedStat format
struct PackedStat transformStatPointerToPackedStat(struct stat *);

// for a syscall, adds a packedstat type Field of value 'pstat'
void setPackedStat(struct Call *, struct PackedStat);

// get's a field Data
union Data getFieldData(struct Call *, int);

// creates a syscall with some syscall #, sets all fields to None type
struct Call createSyscall(int);

// resets syscalls global variable by using createSyscall with UNINITIALIZED
void resetSyscalls(struct Syscalls *, int *);

// adds recorded syscall, to syscalls, and increments syscall_idx
// potentially throws error when recording > 1000 syscalls
void addSyscall(struct Syscalls *, struct Call, int *);

// gets a syscall using the syscall_idx. increments it as well
// potentially throws error when replaying more than 1000 syscalls? (TODO: check with prof)
struct Call getSyscall(struct Syscalls *, int *);

// compare's two syscalls. If there is an error in type or value for any field, throws error
void compareSyscalls(struct Call *, struct Call *, int *);

// compress global variable syscalls. Called only in record_stop
int compress(struct Call, char *, int *);

// uncompress into global variable syscalls from buffer. Called only in replay_start
int uncompress(struct Call *, char *, int*, int);