#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

  // single getpid
  record_start("/rr1.txt");
  int a = getpid();
  record_stop();

  // two getpid
  record_start("/rr2.txt");
  int b = getpid();
  int c = getpid();
  record_stop();

  // replay first getpid
  replay_start("/rr1.txt");
  int aa = getpid();
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second getpid
  replay_start("/rr2.txt");
  int bb = getpid();
  int cc = getpid();
  replay_stop();
  printf(1, "bc test: %d\n", b==bb && c==cc);


  // forked test
  record_start("/rr_forked.txt");

  if (0 == fork()) {

    getpid(); // ignored syscall
    exit();
  }
  wait();
  int d = getpid(); // only one getpid
  record_stop();

  replay_start("/rr_forked.txt"); // functionally the same program
  int dd = getpid();
  replay_stop();

  printf(1, "d test: %d\n", d==dd);

	exit();
}
