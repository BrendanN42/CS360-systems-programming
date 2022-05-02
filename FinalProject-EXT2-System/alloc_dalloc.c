

#include "type.h"
//comment
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd,dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int ialloc(int dev){
   int i;
   char buf[BLKSIZE];
   //use imap,ninodes in mount table of dev
   int imap2,ninodes2;
   char buf2[BLKSIZE];
   
   get_block(dev,1,buf2);
   SUPER *sp2=(SUPER *)buf2;
   ninodes2=sp2->s_inodes_count;

   get_block(dev,2,buf2);
   GD *gp2=(GD *)buf2;
   imap2=gp2->bg_inode_bitmap;

  get_block(dev,imap2,buf);
  for(i=0;i<ninodes2;i++)
  {
     if(tst_bit(buf,i)==0)
     {
        set_bit(buf,i);
        put_block(dev,imap2,buf);
        //update free inode in SUPER and GD
        decFreeInodes(dev);
        return (i+1);
        
     }
  }
   return 0; //out of free nodes
}
int idalloc(int dev, int ino){
   int i;
   char buf[BLKSIZE];

   int imap2,ninodes2;
   char buf2[BLKSIZE];
   
   get_block(dev,1,buf2);
   SUPER *sp2=(SUPER *)buf2;
   ninodes2=sp2->s_inodes_count;

   get_block(dev,2,buf2);
   GD *gp2=(GD *)buf2;
   imap2=gp2->bg_inode_bitmap;

   if(ino>ninodes2){
      printf("inumber %d is out of range\n",ino);
   }
   get_block(dev,imap2,buf);
   clr_bit(buf,ino-1);
   //write buf back
   put_block(dev,imap2,buf);
   //update free inode count in SUPER and GD
   incFreeInodes(dev);
}
int bdalloc(int dev, int ino){
   int i;
   char buf[BLKSIZE];
   int bmap2;
   char buf2[BLKSIZE];

   get_block(dev,2,buf2);
   GD *gp2=(GD *)buf2;
   bmap2=gp2->bg_block_bitmap;

   get_block(dev,bmap2,buf);
   clr_bit(buf,ino-1);
   put_block(dev,bmap2,buf);
   incFreeBlocks(dev);
}
int balloc(int dev){
   int i;
   char buf[BLKSIZE];
   int bmap2,nblocks2;
   char buf2[BLKSIZE];
  
   get_block(dev,1,buf2);
    SUPER *sp2=(SUPER *)buf2;
   nblocks2=sp2->s_blocks_count;

     get_block(dev,2,buf2);
   GD *gp2=(GD *)buf2;
   bmap2=gp2->bg_block_bitmap;
  get_block(dev,bmap,buf);
  for(i=0;i<nblocks2;i++)
  {
     if(tst_bit(buf,i)==0)
     {
        set_bit(buf,i);
        //update free inode in SUPER and GD
         put_block(dev,bmap2,buf);
        decFreeBlocks(dev);
        put_block(dev,bmap2,buf);
        return (i+1);
        
     }
  }
   return 0; //out of free nodes
}