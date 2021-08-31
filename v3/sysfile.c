//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "rr.h"
#include "syscall.h"

char * myargstr(int pos, char * val){
    if(argstr(pos, &val) < 0){
      cprintf("\nERR: unable to get argstr. while record syscall #:%d\n", scrs_idx);
      RecRepErr("");
      return ""; 
    }
    return val;
}

int myargint(int pos, int * val){
  if(argint(pos, val) < 0){
      cprintf("\nERR: unable to get argint. while record syscall #:%d\n", scrs_idx);
      RecRepErr("");
      return -1;
  }
  return *val;
}

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  struct SCR scr = createSCR(SYS_read);
  if(isReplaying()){
    int fd = -1;
    addInt(&scr, myargint(0, &fd));
    addInt(&scr, myargint(2, &n));
    struct SCR recorded_scr = compareSCRS(scr);
    // get first output
    if(argptr(1, &p, n) < 0){
      //said in Q&A we can trust these 'getter' functions
      //these would only fail when the developer screws up on programming
      // not really possible from a user program
      // placing for being complete and following what xv6 code does already
      RecRepErr("\nERR: unable to get argptr. for sys_read\n");
      return -1;
    }
     // returning output buffer
    memmove(
      p,
      &recorded_scr.args[get1stOutputPos(recorded_scr.syscall)+1].val.raw.data,
      recorded_scr.args[get1stOutputPos(recorded_scr.syscall)-1].val.num // get's recorded size
    );
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    int fd = -1;
    addInt(&scr, myargint(0, &fd));
    addInt(&scr, myargint(2, &n));
    int read_output = -1;
    while(1){
      if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
        break;
      read_output = fileread(f, p, n);
      break;
    }
    addInt(&scr,read_output);
    addRaw(&scr, p, n);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;
  
  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;

  struct SCR scr = createSCR(SYS_close);
  if(isReplaying()){
    addInt(&scr, myargint(0, &fd));
    struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    addInt(&scr, myargint(0, &fd));
    int close_output = -1;
    while(1){
      if(argfd(0, &fd, &f) < 0)
        break;
      myproc()->ofile[fd] = 0;
      fileclose(f);
      close_output = 0;
      break;
    }
    addInt(&scr, close_output);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
 	/* original */
  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

 struct SCR scr = createSCR(SYS_fstat);
  if(isReplaying()){
    int fd = -1;
    addInt(&scr, myargint(0, &fd));
    struct SCR recorded_scr = compareSCRS(scr);
    // get first output
    if(argptr(1, (void*)&st, sizeof(*st)) < 0)
    {
      //said in Q&A we can trust these 'getter' functions
      //these would only fail when the developer screws up on programming
      // not really possible from a user program
      // placing for being complete and following what xv6 code does already
      RecRepErr("\nERR: unable to get argptr. for sys_fstat\n");
      return -1;
    }
    // 'returning' fstat
    st->type = recorded_scr.args[get1stOutputPos(recorded_scr.syscall)+1].val.st.type;
    st->size = recorded_scr.args[get1stOutputPos(recorded_scr.syscall)+1].val.st.size;
    st->ino = recorded_scr.args[get1stOutputPos(recorded_scr.syscall)+1].val.st.ino;
    st->nlink = recorded_scr.args[get1stOutputPos(recorded_scr.syscall)+1].val.st.nlink;
    st->dev = recorded_scr.args[get1stOutputPos(recorded_scr.syscall)+1].val.st.dev;

		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    int fd = -1;
    addInt(&scr, myargint(0, &fd));
    int fstat_output = -1;
    while(1){
      if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
        break;
      fstat_output = filestat(f, st);
      break;
    }
    addInt(&scr,fstat_output);
    addStat(&scr, *st);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;
  struct SCR scr = createSCR(SYS_link);
  if(isReplaying()){
    addStr(&scr, myargstr(0, old));
    addStr(&scr, myargstr(1, new));
    struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    addStr(&scr, myargstr(0, old));
    addStr(&scr, myargstr(1, new));
    int link_output = -1;
    while (1){
      if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
        break;

      begin_op();
      if((ip = namei(old)) == 0){
        end_op();
        break;
      }

      ilock(ip);
      if(ip->type == T_DIR){
        iunlockput(ip);
        end_op();
        break;
      }

      ip->nlink++;
      iupdate(ip);
      iunlock(ip);

      if((dp = nameiparent(new, name)) == 0){
        // substituting bad
        ilock(ip);
        ip->nlink--;
        iupdate(ip);
        iunlockput(ip);
        end_op();
        break;
      }
      ilock(dp);
      if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
        iunlockput(dp);
        // substituting bad 
        ilock(ip);
        ip->nlink--;
        iupdate(ip);
        iunlockput(ip);
        end_op();
        break;
      }
      iunlockput(dp);
      iput(ip);

      end_op();

      link_output = 0;
      break;
    // bad:
    //   ilock(ip);
    //   ip->nlink--;
    //   iupdate(ip);
    //   iunlockput(ip);
    //   end_op();
    //   return -1;
    }
    addInt(&scr, link_output);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], *path;
  uint off;

  struct SCR scr = createSCR(SYS_unlink);
  if(isReplaying()){
    addStr(&scr, myargstr(0, path));
    struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    addStr(&scr, myargstr(0, path));
    int unlink_output = -1;
    while (1){
      if(argstr(0, &path) < 0)
        break;

      begin_op();
      if((dp = nameiparent(path, name)) == 0){
        end_op();
        break;
      }

      ilock(dp);

      // Cannot unlink "." or "..".
      if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0){
        // where bad label went
        iunlockput(dp);
        end_op();
        break;
      }

      if((ip = dirlookup(dp, name, &off)) == 0){
        // where bad label went
        iunlockput(dp);
        end_op();
        break;
      }

      ilock(ip);

      if(ip->nlink < 1)
        panic("unlink: nlink < 1");
      if(ip->type == T_DIR && !isdirempty(ip)){
        iunlockput(ip);
        // where bad label went
        iunlockput(dp);
        end_op();
        break;
      }

      memset(&de, 0, sizeof(de));
      if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
        panic("unlink: writei");
      if(ip->type == T_DIR){
        dp->nlink--;
        iupdate(dp);
      }
      iunlockput(dp);

      ip->nlink--;
      iupdate(ip);
      iunlockput(ip);

      end_op();

      unlink_output = 0;
      break;
    // bad:
      // iunlockput(dp);
      // end_op();
      // return -1;
  }
    addInt(&scr, unlink_output);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  if(argstr(0, &path) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

int
sys_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  struct SCR scr = createSCR(SYS_open);
  if(isReplaying()){
    addStr(&scr, myargstr(0, path));
    addInt(&scr, myargint(1, &fd));
    struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    addStr(&scr, myargstr(0, path));
    addInt(&scr, myargint(1, &fd));
    int open_output = -1;
    while(1){
      if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
        break;

      begin_op();

      if(omode & O_CREATE){
        ip = create(path, T_FILE, 0, 0);
        if(ip == 0){
          end_op();
          break;
        }
      } else {
        if((ip = namei(path)) == 0){
          end_op();
          break;
        }
        ilock(ip);
        if(ip->type == T_DIR && omode != O_RDONLY){
          iunlockput(ip);
          end_op();
          break;
        }
      }

      if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
        if(f)
          fileclose(f);
        iunlockput(ip);
        end_op();
        break;
      }
      iunlock(ip);
      end_op();

      f->type = FD_INODE;
      f->ip = ip;
      f->off = 0;
      f->readable = !(omode & O_WRONLY);
      f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
      open_output = fd;
      break;
    }
    addInt(&scr, open_output);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  struct SCR scr = createSCR(SYS_mkdir);
  if(isReplaying()){
    addStr(&scr, myargstr(0, path));
    struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    addStr(&scr, myargstr(0, path));
    int mkdir_output = -1;
    while (1){
      begin_op();
      if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
        end_op();
        break;
      }
      iunlockput(ip);
      end_op();
      mkdir_output = 0;
      break;
    }
    addInt(&scr,mkdir_output);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  begin_op();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  struct SCR scr = createSCR(SYS_mknod);
  if(isReplaying()){
    addStr(&scr, myargstr(0, path));
    addInt(&scr, myargint(1, &major));
    addInt(&scr, myargint(2, &minor));
    struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){

    addStr(&scr, myargstr(0, path));
    addInt(&scr, myargint(1, &major));
    addInt(&scr, myargint(2, &minor));
    int mknod_output = -1;
    while(1){
      begin_op();
      if((argstr(0, &path)) < 0 ||
        argint(1, &major) < 0 ||
        argint(2, &minor) < 0 ||
        (ip = create(path, T_DEV, major, minor)) == 0){
        end_op();
        break;
      }
      iunlockput(ip);
      end_op();
      mknod_output = 0;
      break;

    }
    addInt(&scr, mknod_output);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
	/* original */
  begin_op();
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();

  struct SCR scr = createSCR(SYS_mkdir);
  if(isReplaying()){
    addStr(&scr, myargstr(0, path));
    struct SCR recorded_scr = compareSCRS(scr);
		return recorded_scr.args[get1stOutputPos(recorded_scr.syscall)].val.num;
  }
  if(isRecording()){
    addStr(&scr, myargstr(0, path));
    int chdir_output = -1;
    while (1){
      begin_op();
      if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
        end_op();
        break;
      }
      ilock(ip);
      if(ip->type != T_DIR){
        iunlockput(ip);
        end_op();
        break;
      }
      iunlock(ip);
      iput(curproc->cwd);
      end_op();
      curproc->cwd = ip;
      chdir_output = 0;
      break;
    }
    addInt(&scr,chdir_output);
    addSRC(scr);
    return scr.args[get1stOutputPos(scr.syscall)].val.num;
  }
  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
	/* original */
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op();
  curproc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

// **********************************************************************
// Custom File Helper Methods

static int my_argfd(int fd, struct file **pf){
  struct file *f;

  if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == 0)
    return -1;

  if (pf)
    *pf = f;
  return 0;
}

int my_read(int fd, char *p, int n){
  struct file *f;

  if (my_argfd(fd, &f) < 0)
    return -1;

  return fileread(f, p, n);
}

int my_write(int fd, char *p, int n){
  struct file *f;

  if (my_argfd(fd, &f) < 0)
    return -1;

  return filewrite(f, p, n);
}

int my_open(char *path, int omode){
  int fd;
  struct file *f;
  struct inode *ip;

  begin_op();

  if (omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if (ip == 0){
      end_op();
      return -1;
    }
  }
  else
  {
    if ((ip = namei(path)) == 0)
    {
      end_op();
      return -1;
    }
    ilock(ip);
    if (ip->type == T_DIR && omode != O_RDONLY)
    {
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0)
  {
    if (f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}


int my_close(int fd) {
  struct file *f;

  if (my_argfd(fd, &f) < 0)
    return -1;

  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

// **********************************************************************
