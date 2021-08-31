#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

  // single link
  record_start("/rr1.txt");
  int a = link("/grep", "README");
  record_stop();

  // two link
  record_start("/rr2.txt");
  int b = link("/non_file", "nonfile");
  int c = link("/grep", "README");
  record_stop();

  // replay first link
  replay_start("/rr1.txt");
  int aa = link("/grep", "README");
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second link
  replay_start("/rr2.txt");
  int bb = link("/non_file", "nonfile");
  int cc = link("/grep", "README");
  replay_stop();
  printf(1, "bc test: %d\n", b==bb && c==cc);

  // forked test
  record_start("/rr_forked.txt");

  if (0 == fork()) {
    link("/aa.txt", "something"); // ignored syscall
    exit();
  }
  wait();
  int d = link("/bb.txt", "nonefile" ); 
  record_stop();

  replay_start("/rr_forked.txt"); // functionally the same program
  int dd = link("/bb.txt", "nonefile" );
  replay_stop();

  printf(1, "d test: %d\n", d==dd);

  
  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    link("/non_file", "README");
    replay_stop();
    printf(1, "e test: 0\n");
    exit();
  }
	wait();
  printf(1, "e test: 1 (error should appear above)\n");

  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    link("/grep", "nonfile");
    replay_stop();
    printf(1, "f test: 0\n");
    exit();
  }
  wait();
  printf(1, "f test: 1 (error should appear above)\n");

	exit();
}