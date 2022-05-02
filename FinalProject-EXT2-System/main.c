/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

extern MINODE *iget();

MINODE minode[NMINODE];
MOUNT mountTable[8];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int fd,dev;
int nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128];

#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "misc1.c"
#include "alloc_dalloc.c"
#include "rmdir.c"
#include "symlink.c"
#include "write_cp_mov.c"
#include "mount_unmount.c"
 
int switchP1() 
{
   running = running->next;
   running->cwd = iget(dev, 2);
   printf("Switched to P1, pid %d\n", running->pid);
   return 0;
} 

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;
  MOUNT *mt;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = i;
    p->cwd = 0;
    for(j=0;j<10;j++){
      p->fd[i]=0;
    }
    
  }
  for(i=0;i<8;i++){
    mt=&mountTable[i];
    mt->dev=0;
  }
  
  proc[0].next = &proc[1];
  proc[1].next = &proc[0];
  root=NULL;
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
  mountTable[0].mounted_inode=root;
  mountTable[0].dev=dev;
}

char *disk = "mydisk";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  //char disk[128];
  // printf("enter rootdev name (RETURN for diskimage) : ");
  // fgets(disk, 128, stdin);
  // if(strcmp(disk,"diskimage")!=0)
  //   disk = strtok(pathname, " ");
  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  
  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();  
  mount_root();
  mountTable[0].ninodes=ninodes;
  mountTable[0].nblocks=nblocks;
  mountTable[0].iblk=iblk;
  mountTable[0].bmap=bmap;
  mountTable[0].imap=imap;
  strcpy(mountTable[0].name,disk);
  strcpy(mountTable[0].mount_name,"/");

  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  
  while(1){
    // printf("GLObAL DEV: %d\n",dev);
    printf("input command : [ls|cd|pwd|mkdir|rmdir|creat|link|unlink|symlink|readlink|stat|utime|open|close|lseek|pfd|read|write|cat|cp|mv|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %[^\n]", cmd, pathname);
    printf("cmd=%s pathname=%s\n", cmd, pathname);
  
    if (strcmp(cmd, "ls")==0)
       ls();
    else if (strcmp(cmd, "cd")==0)
       cd();
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
     else if (strcmp(cmd, "mkdir")==0)
     {
        if(strcmp(pathname,"")==0)
        {
           printf("MKDIR: NO pathname given\n");
        }
        else
          mmkdir(pathname);
     }
     else if (strcmp(cmd, "rmdir")==0)
     {
        if(strcmp(pathname,"")==0)
        {
           printf("RMDIR: NO pathname given\n");
        }
        else
          rrmdir(pathname);
     }

     else if (strcmp(cmd, "creat")==0)
     {
        if(strcmp(pathname,"")==0)
        {
           printf("CREAT: NO pathname given\n");
        }
        else
          ccreat(pathname);
     }
      else if (strcmp(cmd, "stat")==0)
     {
        if(strcmp(pathname,"")==0)
        {
           printf("STAT: NO pathname given\n");
        }
        else
          statt(pathname);
     }
      else if (strcmp(cmd, "utime")==0)
     {
        if(strcmp(pathname,"")==0)
        {
           printf("UTIME: NO pathname given\n");
        }
        else
          utimee(pathname);
     }
      else if (strcmp(cmd, "quit")==0)
       quit();
    else if(strcmp(cmd,"link") == 0)
    {
      //printf("\n in link\n");
      if(strcmp(pathname,"")==0)
        {
           printf("LINK: NO pathname given\n");
        }
      else
      {
        char *old_file = strtok(pathname, " ");
        char *new_file = strtok(NULL, " ");
        my_link(old_file,new_file);
      }
      
    }
    else if(strcmp(cmd,"unlink") == 0)
    {
      if(strcmp(pathname,"")==0)
        {
           printf("UNLINK: NO pathname given\n");
        }
        else
          unlink(pathname);
      
    }
    else if(strcmp(cmd,"symlink") == 0)
    {//printf("\n in symlink\n");
      if(strcmp(pathname,"")==0)
        {
           printf("SYMLINK: NO pathname given\n");
        }
       else
        { 
          char *old_file = strtok(pathname, " ");
          char *new_file = strtok(NULL, " ");
          symlink(old_file,new_file);
        }
      
      
    }
    else if(strcmp(cmd,"readlink") == 0)
    {//printf("\n in symlink\n");
      if(strcmp(pathname,"")==0)
        {
           printf("READLINK: NO pathname given\n");
        }
      else
      {
        char buffer[256];
        int size = readlink(pathname,buffer);
        if(size!=-1)
          printf("results of readlink: \n%s\n", buffer);
      }
      
      
    }

     else if(strcmp(cmd,"open") == 0)
    {
      if(strcmp(pathname,"")==0)
        {
           printf("open: NO pathname given\n");
        }
        else
        {
          char *file = strtok(pathname, " ");
          char *mode = strtok(NULL, " ");
          open_file(file,atoi(mode));
        }
          
      
    }

     else if(strcmp(cmd,"close") == 0)
    {
      if(strcmp(pathname,"")==0)
        {
           printf("close: NO pathname given\n");
        }
        else
        {
          close_file(atoi(pathname));
        }
          
      
    }

     else if(strcmp(cmd,"lseek") == 0)
    {
      if(strcmp(pathname,"")==0)
        {
           printf("lseek: NO pathname given\n");
        }
        else
        {
          char *fd = strtok(pathname, " ");
          char *pos = strtok(NULL, " ");
          llseek(atoi(fd),atoi(pos));
        }
          
      
    }
    else if(strcmp(cmd,"pfd") == 0){
    pfd();
  }
  else if(strcmp(cmd,"write") == 0)
    {
        char *fd = strtok(pathname, " ");
        char *buf = strtok(NULL, " ");
        mywrite(atoi(fd),buf,strlen(buf));

        //write_file();  
    }
  else if(strcmp(cmd,"read") == 0)
    {
        
          read_file();
    }
  else if(strcmp(cmd,"cp") == 0)
    {
      if(strcmp(pathname,"")==0)
        {
           printf("close: NO pathname given\n");
        }
        else
        {
          char *src = strtok(pathname, " ");
          char *dest = strtok(NULL, " ");
          my_cp(src, dest);
        }
          
      
    }
  else if(strcmp(cmd,"cat") == 0)
    {
      if(strcmp(pathname,"")==0)
        {
           printf("close: NO pathname given\n");
        }
        else
        {
          mycat(pathname);
        }
          
      
    }

  else if(strcmp(cmd,"mv") == 0)
    {
      if(strcmp(pathname,"")==0)
        {
           printf("close: NO pathname given\n");
        }
        else
        {
          char *src = strtok(pathname, " ");
          char *dest = strtok(NULL, " ");
          my_mv(src, dest);
        }
    }
    else if(strcmp(cmd,"mount") == 0){
      if(strcmp(pathname,"")==0)
      {
       mounthelper();
       continue;
      }
       char *file = strtok(pathname, " ");
        char *mount = strtok(NULL, " ");
        my_mount(file,mount);
    }
    else if(strcmp(cmd,"umount") == 0){
        if(strcmp(pathname,"")==0)
        {
           printf("umount: NO pathname given\n");
        }
        else
          umount(pathname);
     }
    else if(strcmp(cmd,"switch") == 0){
      switchP1(); 
    
      }
  }
  
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
