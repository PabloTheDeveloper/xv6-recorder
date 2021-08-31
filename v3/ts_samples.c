//
// changed from wc.c
//
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"


char buf[200];

int
wc(int fd, char *name)
{
  int i, n;
  int l, w, c, inword;

  l = w = c = 0;
  inword = 0;
  while((n = read(fd, buf, sizeof(buf))) > 0){
    for(i=0; i<n; i++){
      c++;
      if(buf[i] == '\n')
        l++;
      if(strchr(" \r\t\n\v", buf[i]))
        inword = 0;
      else if(!inword){
        w++;
        inword = 1;
      }
    }
  }
  if(n < 0){
    printf(1, "wc: read error\n");
    return -1;
  }
  printf(1, "%d %d %d %s\n", l, w, c, name);
  return w;
}

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1] = { 0, };
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  //memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
deleteall(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  char * name;

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "unlink(%s)\n", fmtname(path));
	break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
	  name = fmtname(buf);
	  if( name[0] == '.' ) continue;
      printf(1, "unlink(%s)\n", name);
    }
    break;
  }
  close(fd);
}


int test1(int argc, char *argv[]){
  printf(1, "\nTEST 1:\n");
  int fd, i, totalw = 0;

  if(argc <= 1){
    return wc(0, "");
  }

  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "wc: cannot open %s\n", argv[i]);
      return -1;
    }
    totalw += wc(fd, argv[i]);
    close(fd);
  }
  
  switch( totalw % 5 ) {
	case 0:
	  printf(1, "device: %d\n", uptime());
	  break;
	case 1:
	  printf(1, "License: %d\n", totalw);
	  break;
	case 2:
	  printf(1, "embedded: %d\n", uptime()+totalw);
	  break;
	case 3:
	  printf(1, "designed: %d\n", totalw - uptime());
	  break;
	case 4:
	  printf(1, "inner: %d\n", uptime());
	  break;
  }
	return -1;
}

void test2(){
  printf(1, "\nTEST 2:\n");
	int x = uptime();
	
	printf(1, "Current uptime: %d\n", x);	
	
	if( (x % 10) == 0 ) {
		printf(1, "You win!\n");
	} else {
		printf(1, "You lose...\n");
	}
}

void test3(){
  printf(1, "\nTEST 3:\n");
	int x = getpid();
	
	printf(1, "Current PID: %d\n", x);	
	
	if( (x*uptime() % 10) == 0 ) {
		printf(1, "You win!\n");
	} else {
		printf(1, "You lose...\n");
	}
}

void test4(){
  printf(1, "\nTEST 4:\n");
	char dirname[20] = { 'd', 'i', 'r', 0, };
	int x = uptime();
	
	dirname[3] = 'a' + x % 20;
	dirname[4] = 'A' + x % 21;
	dirname[5] = 'a' + x % 22;
	dirname[6] = 'A' + x % 23;
	
	
	printf(1, "folder name: %s\n", dirname);	
	mkdir(dirname);
}

void test5(){
  printf(1, "\nTEST 5:\n");
  deleteall("/");
}

void test6(){
  printf(1, "\nTEST 6:\n");
	int fd, ret1, ret2;
	fd = open("file.txt", 0);
	ret1 = close(fd);
	ret2 = close(fd);
  printf(1, "fd: %d, ret1: %d, ret2: %d\n",fd,ret1,ret2);
}

int main(int argc, char *argv[])
{
  /* record tests */
  printf(1, "****************************\n");
  printf(1, "RECORDING:\n");
  if(argc != 1){
  // test 1
  	record_start("./rr1.txt");
    test1(argc, argv);
  	record_stop();
  }

  // test 2
  record_start("./rr2.txt");
  test2();
  record_stop();

  // test 3
  record_start("./rr3.txt");
  test3();
  record_stop();

  // test 4
  record_start("./rr4.txt");
  test4();
  record_stop();

  // test 5
  record_start("./rr5.txt");
  test5();
  record_stop();

  // test 6
  record_start("./rr6.txt");
  test6();
  record_stop();

  /* replay tests */
  printf(1, "\n****************************\n");
  printf(1, "REPLAYING:\n");

  // test 1
  if(argc != 1){
    replay_start("./rr1.txt");
    test1(argc, argv);
	  replay_stop();
  }
  // test 2
  replay_start("./rr2.txt");
  test2();
  replay_stop();

  // test 3
  replay_start("./rr3.txt");
  test3();
  replay_stop();

  // test 4
  replay_start("./rr4.txt");
  test4();
  replay_stop();

  // test 5
  replay_start("./rr5.txt");
  test5();
  replay_stop();

  //test 6
  replay_start("./rr6.txt");
  test6();
  replay_stop();
 
  exit();
}
