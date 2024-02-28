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
#include "mmap.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

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

int
sys_mmap(void)
{
  cprintf("System mmap called\n");
  int addr;
  int length;
  int prot;
  int flags;
  int fd;
  int offset;

  if(argint(0, &addr) < 0) {
    cprintf("Failed to get addr arg\n");
    return -1;
  }

  if(argint(1, (int*)&length) < 0) {
    cprintf("Failed to get length arg\n");
    return -1;
  }

  if(argint(2, (int*)&prot) < 0) {
    cprintf("Failed to get prot arg\n");
    return -1;
  }

  if(argint(3, (int*)&flags) < 0) {
    cprintf("Failed to get flag arg\n");
    return -1;
  }

  if(argint(4, (int*)&fd) < 0) {
    cprintf("Failed to get fd arg\n");
    return -1;
  }

  // if(argfd(4, &fd, &f) < 0) {
  //   cprintf("Failed to get file pointer\n");
  //   return -1;
  // }

  if(argint(5, (int*)&offset) < 0) {
    cprintf("Failed to get offset arg\n");
    return -1;
  }

  

  struct proc *curproc = myproc();
    
    if (length == 0 || length > KERNBASE - MMAPBASE) {
        cprintf("Invalid length\n");
        return MAP_FAIL;
    }

    if (flags & MAP_FIXED && ((uint)addr % PGSIZE != 0)) {
        cprintf("Address is not page-aligned\n");
        return MAP_FAIL;
    }

    if (flags & MAP_FIXED && ((uint)addr < MMAPBASE || (uint)addr >= KERNBASE)) {
        cprintf("Address is not within bounds\n");
        return MAP_FAIL;
    }

    uint start = (flags & MAP_FIXED) ? (uint)addr : PGROUNDUP(MMAPBASE);
    struct map_mem *m = 0;
    
    if (!(flags & MAP_FIXED)) {
        for (int i = 0; i < MAX_MAPS; i++) {
            m = &curproc->map[i];
            if (!m->mapped) {
                start = PGROUNDUP(start);
                if (start + length >= KERNBASE) {
                    cprintf("Out of memory\n");
                    return MAP_FAIL;
                }
                break;
            }
            if (start >= m->addr + m->length) {
                break; // Found a suitable gap
            }
            start = m->addr + m->length;
        }
    }

    // Initialize the new mapping
    m = &curproc->map[curproc->num_mappings];
    m->addr = start;
    m->length = length;
    m->prot = prot;
    m->flags = flags;
    m->offset = offset;
    m->mapped = 1;

    // Adjust protection bits
    if (prot & PROT_READ) {
        m->prot |= PTE_U;
    }
    if (prot & PROT_WRITE) {
        m->prot |= PTE_W;
    }
    
    // Handle file-backed mapping
    if (!(flags & MAP_ANONYMOUS)) {
        if (fd < 0 || fd >= NOFILE || (m->f = curproc->ofile[fd]) == 0) {
            cprintf("Invalid file descriptor\n");
            return MAP_FAIL;
        }
        filedup(m->f); // Increment the reference count of the file
        m->fd = fd;
    }

    // Update the process's mapping count
    curproc->num_mappings++;

    cprintf("Mapped at address %x\n", start);
  return (void *)start;
  
}

int
sys_munmap(void)
{
  cprintf("System munmap called\n");
  int addr;
  size_t length;

  if(argint(0, &addr) < 0) {
    cprintf("Failed to get addr arg\n");
    return -1;
  }

  if(argint(1, (int*)&length) < 0) {
    cprintf("Failed to get length arg\n");
    return -1;
  }

  cprintf("Going into munmap\n");
  cprintf("Addr being passed in: %x\n", addr);
  struct proc *curproc = myproc();

    if(((uint)addr % PGSIZE)){
        cprintf("Address is not page-aligned\n");
        return -1;
    }

    for(int i = 0; i < MAX_MAPS; i++){
        if(curproc -> map[i].addr == (uint)addr){
            
            uint starting_addr = curproc -> map[i].addr;

            uint anon = curproc->map[i].flags & MAP_ANON;
            uint shared = curproc->map[i].flags & MAP_SHARED;
            // Check for file backing
            if(shared && !anon){
                curproc->map[i].f->off = 0;
                if(filewrite(curproc -> map[i].f, (char *) starting_addr, length) < 0){
                    cprintf("filewrite failed\n");
                    return -1; 
                }
            }



            for(uint j = starting_addr; j < starting_addr + length; j+=PGSIZE){
                // Get the pte
                pte_t *pte = walkpgdir(curproc ->pgdir, (char*)j, 1);
                char *v;
                
                // Check if the pte is present in the pgtable
                if(*pte & PTE_P){
                    
                    uint pa = PTE_ADDR(*pte);

                    v = P2V(pa);
                    
                    // Free memory
                    kfree(v);

                    // Indicate the page is not present
                    *pte = *pte & ~PTE_P;

                }
            }

            curproc -> map[i].addr += length;
            curproc -> map[i].length -= length; 
        }
    }


    return 0;
}

