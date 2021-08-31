#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

  // single open
  record_start("/rr1.txt");
  int a = open("/a.txt", O_CREATE);
  record_stop();

  // two open
  record_start("/rr2.txt");
  int b = open("/b.txt", O_CREATE);
  int c = open("/a.txt", O_RDWR);
  record_stop();

  // replay first open
  replay_start("/rr1.txt");
  int aa = open("/a.txt", O_CREATE);
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second open
  replay_start("/rr2.txt");
  int bb = open("/b.txt", O_CREATE);
  int cc = open("/a.txt", O_RDWR);
  replay_stop();
  printf(1, "bc test: %d\n", b==bb && c==cc);

  // forked test
  record_start("/rr_forked.txt");

  if (0 == fork()) {
    open("/aa.txt", O_CREATE); // ignored syscall
    exit();
  }
  wait();
  int d = open("/bb.txt", O_CREATE); 
  record_stop();

  replay_start("/rr_forked.txt"); // functionally the same program
  int dd = open("/bb.txt", O_CREATE);
  replay_stop();

  printf(1, "d test: %d\n", d==dd);

  
  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    open("/b.txt", O_CREATE);
    replay_stop();
    printf(1, "e test: 0\n");
    exit();
  }
  wait();
  printf(1, "e test: 1 (error should appear above)\n");

  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr2.txt");
    open("/b.txt", O_RDWR);
    replay_stop();
    printf(1, "f test: 0\n");
    exit();
  }
  wait();
  printf(1, "f test: 1 (error should appear above)\n");
	exit();
}