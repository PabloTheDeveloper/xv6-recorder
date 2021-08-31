#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"


void test6_wrong_arg(){
	printf(1, "\n-----------------------------------------------");
  printf(1, "\nTEST 6: (Should throw error about wrong arg)\n");
	printf(1, "-----------------------------------------------");
	int fd, ret1, ret2;
	fd = open("file2.txt", 0);
	ret1 = close(fd);
	ret2 = close(fd);
  printf(1, "fd: %d, ret1: %d, ret2: %d\n",fd,ret1,ret2);
}

void test6_wrong_order(){
	printf(1, "\n-----------------------------------------------");
  printf(1, "\nTEST 6: (Should throw error about wrong order)\n");
	printf(1, "-----------------------------------------------");
	int fd, fd2, ret1, ret2;
	fd = open("file.txt", 0);
	fd2 = open("file1.txt", 0);
	ret1 = close(fd);
	ret2 = close(fd);
  printf(1, "fd: %d, fd1: %d, ret1: %d, ret2: %d\n",fd, fd2, ret1,ret2);
}

void test6_wrong_order2(){
	printf(1, "\n-----------------------------------------------");
  printf(1, "\nTEST 6: (Should throw error about wrong order (v2))\n");
	printf(1, "-----------------------------------------------");
	int fd, ret1, ret2;
	fd = 0; // to avoid makefile compilation error
	ret1 = close(fd);
	fd = open("file2.txt", 0);
	ret2 = close(fd);
  printf(1, "fd: %d, ret1: %d, ret2: %d\n",fd, ret1,ret2);
}


void test6(){
  printf(1, "\nTEST 6:\n");
	int fd, ret1, ret2;
	fd = open("file.txt", 0);
	ret1 = close(fd);
	ret2 = close(fd);
  printf(1, "fd: %d, ret1: %d, ret2: %d\n",fd,ret1,ret2);
}

int
main(int argc, char *argv[])
{
  /* record tests */
  printf(1, "****************************\n");
  printf(1, "RECORDING:\n");
  
	// test 6
  record_start("./rr6.txt");
  test6();
  record_stop();

  /* replay tests (forking to failed case from ending test program)*/
  printf(1, "\n****************************\n");
  printf(1, "REPLAYING:\n");

  //test 6 (wrong arg)
	if(fork() == 0){
  	replay_start("./rr6.txt");
  	test6_wrong_arg();
 	 	replay_stop();
		exit();
	}
	wait();

	// test 6 (wrong order)
	if(fork() == 0){
  	replay_start("./rr6.txt");
  	test6_wrong_order();
 	 	replay_stop();
		exit();
	}
	wait();

	// test 6 (wrong order)
	if(fork() == 0){
  	replay_start("./rr6.txt");
  	test6_wrong_order2();
 	 	replay_stop();
		exit();
	}

	wait();

  exit(); 
}
