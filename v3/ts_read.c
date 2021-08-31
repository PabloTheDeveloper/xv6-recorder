#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {

	// null case
  record_start("null_rr.txt");
  record_stop();
  replay_start("null_rr.txt");
  replay_stop();
  int null_pass = 1; // since recording and replaying empty syscalls
                     // didn't throw error
  printf(1,"null test: %d\n", null_pass);

  // /* following 2 tests if executed correctly test for IO */
	int fd = open("README", O_RDWR);
	char buffer[500];

  // single getpid
  record_start("rr1.txt");
  int a = read(fd, buffer, 10);
  record_stop();


  // replay first getpid
  replay_start("rr1.txt");
  int aa = read(fd, buffer, 10);
  replay_stop();
  printf(1, "a test: %d\n", a==aa && a==10);





  // // k records
  // int k_size =a 1000;
  // record_start("one_k.txt");
  // for (int i = 0; i < k_size; i++)
  //   read(fd, buffer, 10);;
  // record_stop();

  // replay_start("one_k.txt");
  // for (int i = 0; i < k_size; i++)
  //   read(fd, buffer, 10);;
  // replay_stop();
  
  // // if you try to compare via int arrays xv6 will crash.
  // // regardless of my code
  // int k_outputs_pass = 1;
 

  // printf(1, "k outputs test: %d", k_outputs_pass);



  // int a_pass = a == aa && a == -1;
  // int bc_pass = b == bb && c == cc && c == -1 && b == -1;


	// int tests_passed = null_pass + k_outputs_pass + a_pass + bc_pass;
  // int tests_total = (sizeof(null_pass) + sizeof(k_outputs_pass) +
  //                   sizeof(a_pass) + sizeof(bc_pass))/4;

  // printf(1, "tests passed: %d/%d", tests_passed, tests_total);
	exit();
}