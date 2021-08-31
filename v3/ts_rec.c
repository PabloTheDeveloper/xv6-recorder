#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

enum {
	GETPID,
	UPTIME,
	OPEN,
	READ,
	FSTAT,
	CLOSE,
	MKDIR,
	CHDIR,
	MKNOD,
	LINK,
	UNLINK,
};

void callSC(int printFlag, int syscall, int intarg1, int intarg2, char *buffer1, char *buffer2, struct stat * st){
	int output = 0;
	switch (syscall){
	case GETPID:
		output = getpid();	
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "getpid", output);
			printf(1, "input(s): N/A\n", intarg1);
			}
		break;
	case UPTIME:
		output = uptime();
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "uptime", output);
			printf(1, "input(s): N/A\n", intarg1);
			}
		break;
	case OPEN:
		output = open(buffer1, intarg1);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "open", output);
			printf(1, "input(s): (path: %s)(omode: %d)\n", buffer1, intarg1);
			}
		break;
	case CLOSE:
		output = close(intarg1);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "close", output);
			printf(1, "input(s): (fd: %d)\n", intarg1);
			}
		break;
	case READ:
		output = read(intarg1, buffer1, intarg2);
		if(printFlag){
			printf(1, "\nsyscall: %s, output(s): (read#: %d) (buffer: %s)\n", "read", output, buffer1);
			printf(1, "input(s): (fd: %d)(size: %d)\n", intarg1, intarg2);
			}
		break;
	case FSTAT:
		output = fstat(intarg1, st);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: (fstat status: %d)\n", "fstat", output);
			printf(1, "input(s): (fd: %d)\n", intarg1);
			}
		break;
	case MKDIR:
		output = mkdir(buffer1);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "mkdir", output);
			printf(1, "input(s):(path: %s)\n", buffer1);
			}
		break;
	case CHDIR:
		output = chdir(buffer1);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "chdir", output);
			printf(1, "input(s):(path: %s)\n", buffer1);
			}
		break;
	case MKNOD:
		output = mknod(buffer1, intarg1, intarg1);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "mknod", output);
			printf(1, "input(s):(path: %s)(major: %d)(minor: %d)\n", buffer1, intarg1, intarg2);
			}
		break;
	case LINK:
		output = link(buffer1, buffer2);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "link", output);
			printf(1, "input(s): (old: %s)(new: %s)\n", buffer1, buffer2);
			}
		break;
	case UNLINK:
		output = unlink(buffer1);
		if(printFlag){
			printf(1, "\nsyscall: %s, output: %d\n", "unlink", output);
			printf(1, "input(s):(path: %s)\n", buffer1);
			}
		break;
	default:
		break;
	}
}
int
main(int argc, char *argv[])
{
	// some variables to pass to functions
	int readme_fd = open("echo", O_RDONLY);

	// used to set which test to run
	int all = 0;
	int test = 0;
	if (argc == 1){
		;
	} else if (argc == 2){
		test = atoi(argv[1]);
	} else if (argc == 3){
		test = atoi(argv[1]);
		all = atoi(argv[2]);
	} else {
		printf(1,"You aren't allowed more than two args in this program!");
		exit();
	}
	if(all || test == -4){
		// should create a session. It should be empty beside the # of syscalls
  	record_start("/null_rr.txt");
  	record_stop();
	}

	if(all || test == 0){
		printf(1, "WRITING ONE RECORD OF EACH SYSCALL:\n\n");
		record_start("/rr1.txt");
		for (int i = 0; i < 11; i++){
			struct stat st;
			char buff[200];
			if(i == OPEN)
				callSC(1, i, O_CREATE, -1, "a.txt", "", &st);

			else if(i == READ || i == FSTAT || i == CLOSE)
				callSC(1, i, readme_fd, 100, buff, "", &st);

			else if(i == MKDIR || i == CHDIR)
				callSC(1, i, -1, -1, "some_dir", "", &st);

			else
				callSC(1, i, 10, 10, "README", "echo", &st);
		}
		record_stop();
	}

	if(all || test == 1){
		printf(1, "\n\nWRITING ONLY 1000 RECORD OF EACH SYSCALL:\n\n");
		const char * scs[] = {
			"/k_getpid",
			"/k_uptime",
			"/k_open",
			"/k_close",
			"/k_read",
			"/k_fstat",
			"/k_mkdir",
			"/k_chdir",
			"/k_mknod",
			"/k_link",
			"/k_unlink",
		};
		for (int i = 0; i < 11; i++){

			printf(1, "recording %s\n", scs[i]);
			record_start(scs[i]);
			for (int j = 0; j < 1000; j++){
				struct stat st;
				char buff[200];
				if(i == READ || i == FSTAT){
					callSC(0, i, readme_fd, 4, buff, "", &st);
				}
				else if(i == OPEN){
					j = j==0?1000 - 15 + 3 + 1 + 1:j;
					// only 16 files allowed to be open at once (removing ones used by use and my own).
					// Leaving one for writing the remaining files
					callSC(0, i, O_RDONLY, -1, "README", "", &st);
				}
				else if(i == UNLINK){
					callSC(0, i, -1, -1, "README", "", &st);
				}
				else{
					callSC(0, i, 10, 10, "README", "echo", &st);
				}
			}
			record_stop();
		}
	}

	if(all || test == 2){
		printf(1, "\n\nWRITING 1001 RECORD OF EACH SYSCALL(should fail each):\n\n");
		const char * scs[] = {
			"k1_getpid",
			"k1_uptime",
			"k1_open",
			"k1_close",
			"k1_read",
			"k1_fstat",
			"k1_mkdir",
			"k1_chdir",
			"k1_mknod",
			"k1_link",
			"k1_unlink",
		};

		for (int i = 0; i < 11; i++){
			printf(1, "TEST MSG: should exceeding records error down below\n");
			if(fork() == 0){
				printf(1, "recording %s\n", scs[i]);
				record_start(scs[i]);
				for (int j = 0; j < 1000 + 1; j++){
					struct stat st;
					char buff[200];
					if(i == READ || i == FSTAT){
						callSC(0, i, readme_fd, 4, buff, "", &st);
					}
					else if(i == OPEN){
					// removing limit from open as program will try to write
					// a rr sessions of > 1000 syscalls
					callSC(0, i, O_RDONLY, -1, "README", "", &st);
					}
					else if(i == UNLINK){
						callSC(0, i, -1, -1, "README", "", &st);
					}
					else{
						callSC(0, i, 10, 10, "README", "echo", &st);
					}
				}
				record_stop();
				printf(1, "failed test\n"); // this line is only callled when there wasn't any error
				exit();
			}
			wait();
		}
	}
















	exit();
}