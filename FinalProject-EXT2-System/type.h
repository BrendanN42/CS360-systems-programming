#ifndef TYPE_H
#define TYPE_H
/*************** type.h file for LEVEL-1 ****************/
#include <ext2fs/ext2_fs.h>
#include <time.h>
#include <libgen.h>
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;
typedef unsigned int u32;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NPROC       2

typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified

  int mounted;           // for level-3
  struct Mount *mptr;  // for level-3
}MINODE;
//open file table
typedef struct oft{
  int mode; //mode of opened file
  int refCount; //number of PROCS sharing this instance
  MINODE *minodePtr; //pointer to minode of FILE
  int offset; //byte offset for R|W
}OFT;
typedef struct proc{
  struct proc *next;
  int          pid;      // process ID  
  int         ppid;
  int          uid;      // user ID
  int          gid;
  MINODE      *cwd;      // CWD directory pointer  
  OFT *fd[10];
}PROC;

typedef struct Mount{
  int    dev;       // dev (opened vdisk fd number) 0 means FREE 
  int    ninodes;   // from superblock
  int    nblocks;
  int    bmap;      // from GD block  
  int    imap;
  int    iblk;
  struct Minode *mounted_inode;
  char   name[64];  // device name, e.g. mydisk
  char   mount_name[64]; // mounted DIR pathname
} MOUNT;
#endif