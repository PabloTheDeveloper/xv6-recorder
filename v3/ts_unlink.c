#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {
	
  // single unlink
  record_start("/rr1.txt");
  int a = unlink("/grep");
  record_stop();

  // two unlink
  record_start("/rr2.txt");
  int b = unlink("/non_file");
  int c = unlink("/grep");
  record_stop();

  // replay first unlink
  replay_start("/rr1.txt");
  int aa = unlink("/grep");
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second unlink
  replay_start("/rr2.txt");
  int bb = unlink("/non_file");
  int cc = unlink("/grep");
  replay_stop();
  printf(1, "bc test: %d\n", b==bb && c==cc);

  // forked test
  record_start("/rr_forked.txt");

  if (0 == fork()) {
    unlink("/aa.txt" ); // ignored syscall
    exit();
  }
  wait();
  int d = unlink("/bb.txt" ); 
  record_stop();

  replay_start("/rr_forked.txt"); // functionally the same program
  int dd = unlink("/bb.txt" );
  replay_stop();

  printf(1, "d test: %d\n", d==dd);

  
  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    unlink("/non_file");
    replay_stop();
    printf(1, "e test: 0\n");
    exit();
  }
  wait();
  printf(1, "e test: 1 (error should appear above)\n");

	exit();
}