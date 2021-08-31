#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

  // single mkdir
  record_start("/rr1.txt");
  int a = mkdir("/a" );
  record_stop();

  // two mkdir
  record_start("/rr2.txt");
  int b = mkdir("/b" );
  int c = mkdir("/a" );
  record_stop();

  // replay first mkdir
  replay_start("/rr1.txt");
  int aa = mkdir("/a" );
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second mkdir
  replay_start("/rr2.txt");
  int bb = mkdir("/b" );
  int cc = mkdir("/a" );
  replay_stop();
  printf(1, "bc test: %d\n", b==bb && c==cc);

  // forked test
  record_start("/rr_forked.txt");

  if (0 == fork()) {
    mkdir("/aa.txt" ); // ignored syscall
    exit();
  }
  wait();
  int d = mkdir("/bb.txt" ); 
  record_stop();

  replay_start("/rr_forked.txt"); // functionally the same program
  int dd = mkdir("/bb.txt" );
  replay_stop();

  printf(1, "d test: %d\n", d==dd);

  
  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    mkdir("/b" );
    replay_stop();
    printf(1, "e test: 0\n");
    exit();
  }
  wait();
  printf(1, "e test: 1 (error should appear above)\n");

	exit();
}