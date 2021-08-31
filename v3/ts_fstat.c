#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

  // /* following 2 tests if executed correctly test for IO */
	int fd = open("README", O_RDWR);
	struct stat st1;
  // single record
  record_start("rr1.txt");
  int a = fstat(fd, &st1);
  record_stop();

  // single replay
  replay_start("rr1.txt");
	struct stat st2;
  int aa = fstat(fd, &st2);
  replay_stop();
  printf(1, "a test: %d, a=%d\n", a==aa, a);

	printf(1,
	"\nst: (stat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)",
	st1.type,
	st1.dev,
	st1.ino,
	st1.nlink,
	st1.size
	);

	printf(1,
	"\nst: (stat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)",
	st2.type,
	st2.dev,
	st2.ino,
	st2.nlink,
	st2.size
	);

	if (st2.dev != st1.dev || st2.ino != st1.ino || st2.nlink != st1.nlink || st2.type != st1.type || st2.size != st1.size)
	{
		printf(1, "Error. both values are not the same");
	} else {
    printf(1, "\nb test: (passed)\n");
  }
  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("rr1.txt");
    fstat(fd+1, &st1); 
    replay_stop();
    printf(1, "c test: 0\n");
    exit();
  }
  wait();
  printf(1, "c test: 1 (error should appear above)\n");
	exit();
}