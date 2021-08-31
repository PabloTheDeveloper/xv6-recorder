#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

  // single close
  record_start("/rr1.txt");
  int a = close(10);
  record_stop();

  // two close
  record_start("/rr2.txt");
  int b = close(10);
  int c = close(10);
  record_stop();

  // replay first close
  replay_start("/rr1.txt");
  int aa = close(10);
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second close
  replay_start("/rr2.txt");
  int bb = close(10);
  int cc = close(10);
  replay_stop();
  printf(1, "bc test: %d\n", b==bb && c==cc);

  // forked test
  record_start("/rr_forked.txt");

  if (0 == fork()) {
    close(11); // ignored syscall
    exit();
  }
  wait();
  int d = close(10); // only one close
  record_stop();

  replay_start("/rr_forked.txt"); // functionally the same program
  int dd = close(10);
  replay_stop();

  printf(1, "d test: %d\n", d==dd);

  
  if (0 == fork()){
    // mismatched syscall (suppose to throw error)
    replay_start("/rr1.txt");
    close(11);
    replay_stop();
    printf(1, "e test: 0\n");
    exit();
  }
  wait();
  printf(1, "e test: 1 (error should appear above)\n");

  // passing valid fd 
  int fd = open("cat", O_RDWR);
  record_start("/rr_open_valid_fd.txt");
  int f = close(fd);
  record_stop();

  replay_start("/rr_open_valid_fd.txt");
  int ff = close(fd); // even though close, it should still run correclty
  printf(1, "f test: %d\n", f==ff);

	exit();
}