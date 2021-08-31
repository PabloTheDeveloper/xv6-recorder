//
// changed from wc.c
//
#include "types.h"
#include "stat.h"
#include "user.h"

char buf[512];

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
  //if(n < 0){
    //printf(1, "wc: read error\n");
    //exit();
  //}
  //printf(1, "%d %d %d %s\n", l, w, c, name);
  return w;
}

int
main(int argc, char *argv[])
{
  int fd, i, totalw = 0;

  if(argc <= 1){
    wc(0, "");
    exit();
  }

  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "wc: cannot open %s\n", argv[i]);
      exit();
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
  
  
  exit();
}
