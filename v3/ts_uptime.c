#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {
  // single uptime
  record_start("/rr1.txt");
  int a = uptime();
  record_stop();

  // two uptime
  record_start("/rr2.txt");
  int b = uptime();
  int c = uptime();
  record_stop();

  // replay first uptime
  replay_start("/rr1.txt");
  int aa = uptime();
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second uptime
  replay_start("/rr2.txt");
  int bb = uptime();
  int cc = uptime();
  replay_stop();
  printf(1, "bc test: %d\n", b==bb && c==cc);

  // forked test
  record_start("/rr_forked.txt");

  if (0 == fork()) {
    uptime(); // ignored syscall
    exit();
  }
  wait();
  int d = uptime(); // only one uptime
  record_stop();

  replay_start("/rr_forked.txt"); // functionally the same program
  int dd = uptime();
  replay_stop();

  printf(1, "d test: %d\n", d==dd);

	exit();
}
