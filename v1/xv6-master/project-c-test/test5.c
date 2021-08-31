#include "types.h"
#include "stat.h"
#include "user.h"

#include "fs.h"

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
    //printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    //printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "unlink(%s)\n", fmtname(path));
	break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      //printf(1, "ls: path too long\n");
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
        //printf(1, "ls: cannot stat %s\n", buf);
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

int
main(int argc, char *argv[])
{ 
  deleteall("/");

  exit();
}
