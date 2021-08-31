#define PRINTLOG 0
#define MAX_SCRS 1000
#define MAX_ARGC 4
#define MAX_RAW_SIZE 200
#define MAX_FILE_SIZE 60000

// types for Arg
#define UNITIALIZED_SCR 0
#define NONE_TYPE 0
#define INT_TYPE 1
#define UINT_TYPE 2
#define BYTE_TYPE 3
#define RAW_TYPE 4
#define STAT_TYPE 5


struct myStat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
  uint size;   // Size of file in bytes
};


struct Raw {
  int size;
  char data[MAX_RAW_SIZE];
};
union Val {
  int num;
	uint unum;
  struct Raw raw;
  struct myStat st;
};

struct Arg {
  unsigned char type;
  union Val val;
};

// SCR: SysCall Records
struct SCR {
  unsigned char syscall;
  unsigned char argc;
  struct Arg args[MAX_ARGC];
};


int get1stOutputPos(int syscall);

void printSCR(struct SCR scr);

// Transforms 'val' into a 'Arg' struct.
// Appends to scr's args. increments src's argc
void addInt(struct SCR *scr, int val);

// Transforms 'val' into a 'Arg' struct.
// Appends to scr's args. increments src's argc
void addUInt(struct SCR *scr, uint val);


// Transforms 'val' into a 'Arg' struct.
// Appends to scr's args. increments src's argc
void addStr(struct SCR *scr, char * buffer);

// Transforms 'val' into a 'Arg' struct.
// Appends to scr's args. increments src's argc
void addRaw(struct SCR *scr, char * buffer, int size);

// Transforms 'val' into a 'Arg' struct.
// Appends to scr's args. increments src's argc
void addStat(struct SCR *scr, struct stat val);

// creates a SRC.
// The SRC has all args set  to type: NONE_TYPE
struct SCR createSCR(int syscall);

// Adds a SRC to 'recorded_scrs'
void addSRC(struct SCR scr);

// gets a SCR from 'recorded_scrs'
// increments src's argc
// compares it with passed int scr
// if passes, it returns recorded SCR
struct SCR compareSCRS(struct SCR scr);

int isRecOrReplay();
int isRecording();
int isReplaying();
// kills program as I should
void RecRepErr(char *msg);

struct SCR recorded_scrs[MAX_SCRS];
int scrs_idx;
char buffer[MAX_FILE_SIZE];
int buffer_pos;
int rr_pid;

