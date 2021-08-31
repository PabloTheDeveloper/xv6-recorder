#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
  int o_status = 0;
  char buffer[10];
  // if (argc != 2)
  //   exit();

  // int flag = atoi(argv[1]);
  // if (flag)
  // {
  record_start("filename.txt");
  o_status = open("filenames.txt", O_CREATE); // 2
  getpid();                                   // 1
  o_status = open("filenames.txt", O_CREATE); // 2
                                              // 1
  read(o_status, buffer, 8); // 3
  close(o_status); // 4
  close(o_status); // 4

  // for(i = 1; i < argc; i++)
  //   printf(1, "%s%s", argv[i], i+1 < argc ? " " : "\n");

  record_stop();
  replay_stop();
  // }
  // else
  // {
  replay_start("filename.txt");
  o_status = open("filenames.txt", O_CREATE); // 2
  getpid();                                   // 1
  o_status = open("filenames.txt", O_CREATE); // 2

  read(o_status, buffer, 8);                  // 3
  close(o_status); // 4
  close(o_status); // 4

  replay_stop();
  // }
  exit();
}
