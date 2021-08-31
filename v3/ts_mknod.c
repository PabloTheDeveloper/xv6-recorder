#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

  // single mknod
  record_start("/rr1.txt");
  int a = mknod("/a.txt", 1, 1);
  record_stop();

  // replay first mknod
  replay_start("/rr1.txt");
  int aa = mknod("/a.txt", 1, 1);
  replay_stop();
  printf(1, "a test: %d\n", a==aa);

  // replay second mknod
  
  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    mknod("/b.txt", 1, 1);
    replay_stop();
    printf(1, "c test: 0\n");
    exit();
  }
  wait();
  printf(1, "c test: 1 (error should appear above)\n");

  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    mknod("/a.txt", 0, 1);
    replay_stop();
    printf(1, "d test: 0\n");
    exit();
  }
  wait();
  printf(1, "d test: 1 (error should appear above)\n");

  if (0 == fork()){
    // mismatched args (suppose to throw error)
    replay_start("/rr1.txt");
    mknod("/a.txt", 1, 0);
    replay_stop();
    printf(1, "e test: 0\n");
    exit();
  }
  wait();
  printf(1, "e test: 1 (error should appear above)\n");
	exit();
}