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

  int curr_pid = myproc()->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
    struct Call recorded = getSyscall(&syscalls, &syscall_idx);
    if(recorded.syscall == UNINITIALIZED)
    {
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			exit();
    }
    else
    {
      int fd = -1;
      struct Call replayed = createSyscall(SYS_read);
 #if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 0).num);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 1).num);
		cprintf("\nOutput: (int: %d)", getFieldData(&recorded, 2).num);
		cprintf("\nOutput: (bytes: %s)\n", getFieldData(&recorded, 3).bytes.content);
		cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argint(0, &fd) < 0)
        {
          cprintf("\nERR: unable to get argint. for sys_read. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }
        // get second input
        if(argint(2, &n) < 0)
        {
          cprintf("\nERR: unable to get argint. for sys_read. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }
        // get first output
        if(argptr(1, &p, n) < 0)
        {
          cprintf("\nERR: unable to get argptr. for sys_read. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        // sets inputs
        setInt(&replayed, fd);
        setInt(&replayed, n);

        // sets outputs
        setInt(&replayed, getFieldData(&recorded, 2).num);
        setByte(&replayed, getFieldData(&recorded, 3).bytes);

        compareSyscalls(&replayed, &recorded, &syscall_idx);

        // returning outputs
        union Data recorded_output = getFieldData(&recorded, 3);
        memmove(p, &recorded_output.bytes.content, n);
        return getFieldData(&recorded, 2).num;
      }

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
    
  }
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int read_status = -1; // assuming failure condition
    int fd = -1;
    struct Call recorded = createSyscall(SYS_read);
    
    // get first input
    if(argint(0, &fd) < 0)
    {
      cprintf("\nERR: unable to get argint. for sys_read. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }
    // get second input
    if(argint(2, &n) < 0)
    {
      cprintf("\nERR: unable to get argint. for sys_read. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }
    // sets first two inputs
    setInt(&recorded, fd);
    setInt(&recorded, n);

    // original snippet of but modified to get output value
    for (int flag = 0; flag < 1; flag++)
    {
      if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
        break; // read_status = -1

      read_status = fileread(f, p, n); 
      break;
    }
    // sets outputs
    setInt(&recorded, read_status);
    setByte(&recorded, transformCharPointerToByte(p, n));

#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 0).num);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 1).num);
		cprintf("\nOutput: (int: %d)", getFieldData(&recorded, 2).num);
		cprintf("\nOutput: (bytes: %s)\n", getFieldData(&recorded, 3).bytes.content);
		cprintf("------------------------------------");
#endif

    addSyscall(&syscalls, recorded, &syscall_idx);
    return read_status;
  }

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

  
	int curr_pid = myproc()->pid;
	if(curr_pid == rr_pid && replay_syscalls == 1)
	{
		struct Call recorded = getSyscall(&syscalls, &syscall_idx);
		if (recorded.syscall == UNINITIALIZED)
		{
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			kill(rr_pid);
		}
		else
		{
			struct Call replayed = createSyscall(SYS_close);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nInput: (int: %d)", getFieldData(&recorded, 0).num);
      cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 1).num);
			cprintf("------------------------------------");
#endif
			if(recorded.syscall == replayed.syscall)
			{
        // Get First argument
        if(argint(0, &fd) < 0)
        {
          cprintf("\nERR: unable to get argint. for sys_close. replay syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
          replay_syscalls = -1;
          exit();
          return -1;
        }
 				setInt(&replayed, fd);
				setInt(&replayed, getFieldData(&recorded, 1).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
				return getFieldData(&recorded, 1).num;	
			}

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
		}
	}

  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int close_status = -1; // assumes failure condition
		struct Call recorded = createSyscall(SYS_close);
    // get first input
    if(argint(0, &fd) < 0)
    {
      cprintf("\nERR: unable to get argint. for sys_close. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }


    // original snippet of code but modified to get output value
    for (int flag = 0; flag < 1; flag++)
    {
      if(argfd(0, &fd, &f) < 0)
        break;
      myproc()->ofile[fd] = 0;
      fileclose(f);
      close_status = 0;
      break;
  }
    // set first input
    setInt(&recorded, fd);    
    // set first output
    setInt(&recorded, close_status);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 0).num);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 1).num);
		cprintf("------------------------------------");
#endif
    return close_status;
  }

  // Original Code
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

  int curr_pid = myproc()->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
		struct Call recorded = getSyscall(&syscalls, &syscall_idx);
		if (recorded.syscall == UNINITIALIZED)
		{
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			kill(rr_pid);
      return -1;
		}
		else
		{
      int fd = -1;
      struct Call replayed = createSyscall(SYS_fstat);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 0).num);
		cprintf("\nOutput: (int: %d)", getFieldData(&recorded, 1).num);
		cprintf(
      "\nOutput: (stat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)\n",
      getFieldData(&recorded, 2).pstat.type,
      getFieldData(&recorded, 2).pstat.dev,
      getFieldData(&recorded, 2).pstat.ino,
      getFieldData(&recorded, 2).pstat.nlink,
      getFieldData(&recorded, 2).pstat.size
    );
		cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argint(0, &fd) < 0)
        {
          cprintf("\nERR: unable to get argint. for sys_fstat. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        // 'get' first output
        if(argptr(1, (void*)&st, sizeof(*st)) < 0)
        {
          cprintf("\nERR: unable to get argptr. for sys_fstat. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        setInt(&replayed, fd);

        // set outputs
        setInt(&replayed, getFieldData(&recorded, 1).num);
        setPackedStat(&replayed, getFieldData(&recorded, 2).pstat);
 
        compareSyscalls(&replayed, &recorded, &syscall_idx);

        // returning outputs
        union Data recorded_output = getFieldData(&recorded, 2);
        st->type = recorded_output.pstat.type;
        st->dev = recorded_output.pstat.dev;
        st->ino = recorded_output.pstat.ino;
        st->nlink = recorded_output.pstat.nlink;
        st->size = recorded_output.pstat.size;
        return getFieldData(&recorded, 1).num;
      }
			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
  }
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int fstat_status = -1; // assuming failure condition
    int fd = -1;
    struct Call recorded = createSyscall(SYS_fstat);

    // get first input
    if(argint(0, &fd) < 0)
    {
      cprintf("\nERR: unable to get argint. for sys_fstat. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // sets input
    setInt(&recorded, fd);
    
    // original snippet of but modified to get output value
    for (int flag = 0; flag < 1; flag++)
    {
      if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
        break; // fstat_status = -1

      fstat_status = filestat(f, st);
      break;
    }
    
    //set outputs
    setInt(&recorded, fstat_status);
    setPackedStat(&recorded, transformStatPointerToPackedStat(st));

#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 0).num);
		cprintf("\nOutput: (int: %d)", getFieldData(&recorded, 1).num);
		cprintf(
      "\nOutput: (stat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)\n",
      getFieldData(&recorded, 2).pstat.type,
      getFieldData(&recorded, 2).pstat.dev,
      getFieldData(&recorded, 2).pstat.ino,
      getFieldData(&recorded, 2).pstat.nlink,
      getFieldData(&recorded, 2).pstat.size
    );
		cprintf("------------------------------------");
#endif

    addSyscall(&syscalls, recorded, &syscall_idx);
    return fstat_status;
  }

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

  int curr_pid = myproc()->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
    struct Call recorded = getSyscall(&syscalls, &syscall_idx);
    if(recorded.syscall == UNINITIALIZED)
    {
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			exit();
    }
    else
    {
      struct Call replayed = createSyscall(SYS_link);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 1).bytes.content);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 2).num);
		cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argstr(0, &old) < 0)
        {
          cprintf("\nERR: unable to get 1st argstr. for sys_link. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        // get second input
        if(argstr(1, &new) < 0)
        {
          cprintf("\nERR: unable to get 2nd argstr. for sys_link. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        // set first two inputs
        setByte(&replayed, transformNULLTerminatedStrToByte(old));
        setByte(&replayed, transformNULLTerminatedStrToByte(new));
        setInt(&replayed, getFieldData(&recorded, 2).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
        return getFieldData(&recorded, 2).num;
      }

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
  }
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int link_status = -1; // assumes fail state
		struct Call recorded = createSyscall(SYS_link);

    // get first input
    if(argstr(0, &old) < 0)
    {
      cprintf("\nERR: unable to get 1st argstr. for sys_link. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // get second input
    if(argstr(1, &new) < 0)
    {
      cprintf("\nERR: unable to get 2nd argstr. for sys_link. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // set first two inputs
    setByte(&recorded, transformNULLTerminatedStrToByte(old));
    setByte(&recorded, transformNULLTerminatedStrToByte(new));

    // original snippet of but modified to get output value
    for(int flag = 0; flag < 1; flag++)
    {
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

      link_status = 0;
    }

    // set first output
    setInt(&recorded, link_status);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 1).bytes.content);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 2).num);
		cprintf("------------------------------------");
#endif
    return link_status;
  }

  // original source code
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

  int curr_pid = myproc()->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
    struct Call recorded = getSyscall(&syscalls, &syscall_idx);
    if(recorded.syscall == UNINITIALIZED)
    {
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			exit();
    }
    else
    {
      struct Call replayed = createSyscall(SYS_unlink);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
			cprintf("\nInput: (int: %d)\n", getFieldData(&recorded, 1).num);
			cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argstr(0, &path) < 0)
        {
          cprintf("\nERR: unable to get argstr. for sys_unlink. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        setByte(&replayed, transformNULLTerminatedStrToByte(path));
        setInt(&replayed, getFieldData(&recorded, 1).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
        return getFieldData(&recorded, 1).num;
      }

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
  }
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int unlink_status = -1; // assumes fail state
		struct Call recorded = createSyscall(SYS_unlink);

    // get first input
    if(argstr(0, &path) < 0)
    {
      cprintf("\nERR: unable to get argstr, for unlink while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // set first input
    setByte(&recorded, transformNULLTerminatedStrToByte(path));

    // original snippet of but modified to get output value
    for(int flag = 0; flag < 1; flag++)
    {
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
      {
        // goto bad;
        iunlockput(dp);
        end_op();
        break;
      }

      if((ip = dirlookup(dp, name, &off)) == 0)
      {
        // goto bad;
        iunlockput(dp);
        end_op();
        break;
      }
      ilock(ip);

      if(ip->nlink < 1)
        panic("unlink: nlink < 1");
      if(ip->type == T_DIR && !isdirempty(ip)){
        iunlockput(ip);
        // goto bad;
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

      unlink_status = 0;
      break;
    }

    // set first output
    setInt(&recorded, unlink_status);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 1).num);
		cprintf("------------------------------------");
#endif
    return unlink_status;
  }
  
  // original source code
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


  int curr_pid = myproc()->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
    struct Call recorded = getSyscall(&syscalls, &syscall_idx);
    if(recorded.syscall == UNINITIALIZED)
    {
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			exit();
    }
    else
    {
      struct Call replayed = createSyscall(SYS_open);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
			cprintf("\nInput: (int: %d)", getFieldData(&recorded, 1).num);
      cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 2).num);
			cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argstr(0, &path) < 0)
        {
          cprintf("\nERR: unable to get argstr. for sys_open. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }
        // get second input
        if(argint(1, &omode) < 0)
        {
          cprintf("\nERR: unable to get argint. for sys_open. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }
        setByte(&replayed, transformNULLTerminatedStrToByte(path));
        setInt(&replayed, omode);
        setInt(&replayed, getFieldData(&recorded, 2).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
        return getFieldData(&recorded, 2).num;
      }

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
  }
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int open_status = 0;
		struct Call recorded = createSyscall(SYS_open);

    // get first input
    if(argstr(0, &path) < 0)
    {
      cprintf("\nERR: unable to get argstr. for sys_open. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }
    // get second input
    if(argint(1, &omode) < 0)
    {
      cprintf("\nERR: unable to get argint. for sys_open. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }
    // set first two inputs
    setByte(&recorded, transformNULLTerminatedStrToByte(path));
    setInt(&recorded, omode);

    // original snippet of but modified to get output value
    for(int flag = 0; flag < 1; flag++)
    {
      if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
      {
        open_status = -1;
        break;
      }

      begin_op();

      if(omode & O_CREATE){
        ip = create(path, T_FILE, 0, 0);
        if(ip == 0){
          end_op();
          open_status = -1;
          break;
        }
      } else {
        if((ip = namei(path)) == 0){
          end_op();
          open_status = -1;
          break;
        }
        ilock(ip);
        if(ip->type == T_DIR && omode != O_RDONLY){
          iunlockput(ip);
          end_op();
          open_status = -1;
          break;
        }
      }

      if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
        if(f)
          fileclose(f);
        iunlockput(ip);
        end_op();
        open_status = -1;
        break;
      }
      iunlock(ip);
      end_op();

      f->type = FD_INODE;
      f->ip = ip;
      f->off = 0;
      f->readable = !(omode & O_WRONLY);
      f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
      open_status = fd;
    }

    // set first output
    setInt(&recorded, open_status);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 1).num);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 2).num);
		cprintf("------------------------------------");
#endif
    return open_status;
  }

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
  int curr_pid = myproc()->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
    struct Call recorded = getSyscall(&syscalls, &syscall_idx);
    if(recorded.syscall == UNINITIALIZED)
    {
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			exit();
    }
    else
    {
      struct Call replayed = createSyscall(SYS_mkdir);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
			cprintf("\nInput: (int: %d)\n", getFieldData(&recorded, 1).num);
			cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argstr(0, &path) < 0)
        {
          cprintf("\nERR: unable to get argstr. for sys_mkdir. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        setByte(&replayed, transformNULLTerminatedStrToByte(path));
        setInt(&replayed, getFieldData(&recorded, 1).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
        return getFieldData(&recorded, 1).num;
      }

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
  }
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int mkdir_status = -1; // assumes fail state
		struct Call recorded = createSyscall(SYS_mkdir);

    // get first input
    if(argstr(0, &path) < 0)
    {
      cprintf("\nERR: unable to get argstr. for sys_mkdir. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // set first input
    setByte(&recorded, transformNULLTerminatedStrToByte(path));

    // original snippet of but modified to get output value
    for(int flag = 0; flag < 1; flag++)
    {
      begin_op();
      if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
        end_op();
        break;
      }
      iunlockput(ip);
      end_op();
      mkdir_status = 0;
    }

    // set first output
    setInt(&recorded, mkdir_status);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 1).num);
		cprintf("------------------------------------");
#endif
    return mkdir_status;
  }

  // original source code
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

  int curr_pid = myproc()->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
    struct Call recorded = getSyscall(&syscalls, &syscall_idx);
    if(recorded.syscall == UNINITIALIZED)
    {
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			exit();
    }
    else
    {
      struct Call replayed = createSyscall(SYS_mknod);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
			cprintf("\nInput: (int: %d)", getFieldData(&recorded, 1).num);
			cprintf("\nInput: (int: %d)", getFieldData(&recorded, 2).num);
      cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 3).num);
			cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argstr(0, &path) < 0)
        {
          cprintf("\nERR: unable to get argstr. for sys_mknod. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }
        // get second input
        if(argint(1, &major) < 0)
        {
          cprintf("\nERR: unable to get argint. for sys_mknod. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        // get third input
        if(argint(2, &minor) < 0)
        {
          cprintf("\nERR: unable to get argint. for sys_mknod. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        // set first two inputs
        setByte(&replayed, transformNULLTerminatedStrToByte(path));
        setInt(&replayed, major);
        setInt(&replayed, minor);
        setInt(&replayed, getFieldData(&recorded, 3).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
        return getFieldData(&recorded, 3).num;
      }

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
  }
 
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int mknod_status = -1;
		struct Call recorded = createSyscall(SYS_mknod);

    // get first input
    if(argstr(0, &path) < 0)
    {
      cprintf("\nERR: unable to get argstr. for sys_mknod. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }
    // get second input
    if(argint(1, &major) < 0)
    {
      cprintf("\nERR: unable to get argint. for sys_mknod. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // get third input
    if(argint(2, &minor) < 0)
    {
      cprintf("\nERR: unable to get argint. for sys_mknod. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // set first two inputs
    setByte(&recorded, transformNULLTerminatedStrToByte(path));
    setInt(&recorded, major);
    setInt(&recorded, minor);

    // original snippet of but modified to get output value
    for(int flag = 0; flag < 1; flag++)
    {
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
      mknod_status = 0;
      break;
    }
    // set first output
    setInt(&recorded, mknod_status);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 1).num);
		cprintf("\nInput: (int: %d)", getFieldData(&recorded, 2).num);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 3).num);
		cprintf("------------------------------------");
#endif
    return mknod_status;
  }


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

  int curr_pid = curproc->pid;
  if(curr_pid == rr_pid && replay_syscalls == 1)
  {
    struct Call recorded = getSyscall(&syscalls, &syscall_idx);
    if(recorded.syscall == UNINITIALIZED)
    {
			cprintf("\nERR: Attempted to replay more records than records available\n");
			replay_syscalls = -1;
			exit();
    }
    else
    {
      struct Call replayed = createSyscall(SYS_chdir);
#if PRINTLOG
			cprintf("\n------------------------------------");
			cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
			cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
			cprintf("\nInput: (int: %d)\n", getFieldData(&recorded, 1).num);
			cprintf("------------------------------------");
#endif
      if(recorded.syscall == replayed.syscall)
      {
        // get first input
        if(argstr(0, &path) < 0)
        {
          cprintf("\nERR: unable to get argstr. for sys_chdir. while record syscall #:%d\n", recorded.syscall);
          record_syscalls = -1;
          exit();
          return -1;
        }

        setByte(&replayed, transformNULLTerminatedStrToByte(path));
        setInt(&replayed, getFieldData(&recorded, 1).num);

        compareSyscalls(&replayed, &recorded, &syscall_idx);
        return getFieldData(&recorded, 1).num;
      }

			cprintf("\nERR: replayed syscall #: %d, while record syscall #:%d\n",replayed.syscall, recorded.syscall);
			replay_syscalls = -1;
			exit();
			return -1;
    }
  }
  if(curr_pid == rr_pid && record_syscalls == 1)
  {
    int chdir_status = -1; // assumes fail state
		struct Call recorded = createSyscall(SYS_chdir);

    // get first input
    if(argstr(0, &path) < 0)
    {
      cprintf("\nERR: unable to get argstr. for sys_chdir. while record syscall #:%d\n", recorded.syscall);
      record_syscalls = -1;
      exit();
      return -1;
    }

    // set first input
    setByte(&recorded, transformNULLTerminatedStrToByte(path));

    // original snippet of but modified to get output value
    for(int flag = 0; flag < 1; flag++)
    {
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
      chdir_status = 0;
      break;
    }

    // set first output
    setInt(&recorded, chdir_status);
		addSyscall(&syscalls, recorded, &syscall_idx);
#if PRINTLOG
		cprintf("\n------------------------------------");
		cprintf("\nSyscall counter: %d, syscall: %d",syscall_idx, recorded.syscall);
		cprintf("\nInput: (str: %s)", getFieldData(&recorded, 0).bytes.content);
		cprintf("\nOutput: (int: %d)\n", getFieldData(&recorded, 1).num);
		cprintf("------------------------------------");
#endif
    return chdir_status;
  }

  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
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

static int
my_argfd(int fd, struct file **pf)
{
  struct file *f;

  if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == 0)
    return -1;

  if (pf)
    *pf = f;
  return 0;
}

int
my_read(int fd, char *p, int n)
{
  struct file *f;

  if (my_argfd(fd, &f) < 0)
    return -1;

  return fileread(f, p, n);
}

int
my_write(int fd, char *p, int n)
{
  struct file *f;

  if (my_argfd(fd, &f) < 0)
  {
    return -1;
  }
  // cprintf("writable: %d\n", f->writable != 0);
  return filewrite(f, p, n);
}

int
my_open(char *path, int omode)
{
  int fd;
  struct file *f;
  struct inode *ip;

  begin_op();

  if (omode & O_CREATE)
  {
    ip = create(path, T_FILE, 0, 0);
    if (ip == 0)
    {
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


int
my_close(int fd)
{
  struct file *f;

  if (my_argfd(fd, &f) < 0)
    return -1;

  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

// **********************************************************************
