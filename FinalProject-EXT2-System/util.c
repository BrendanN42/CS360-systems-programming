/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern MOUNT mountTable[8];
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd,dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}
MINODE *mialloc()//allocate a FREE minode for use
{
   int i;
   for(i=0;i<NMINODE;i++)
   {
      MINODE *mp = &minode[i];
      if(mp->refCount==0){
         mp->refCount=1;
         return mp;
      }
   }
   printf("FS panic: out of minodes\n");
   return 0;

} 
int midalloc(MINODE *mip){//release a used minode
   mip->refCount=0; //setting it to 0 means that viewed is unclaimed
   return 0;
} 


// return minode pointer to loaded INODE
/*this function returns a pointer to the in-memory minode containing the inode of (dev,ino).
the returned minode is unique, i.e. only one copy of the INODe exists in memory*/
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

   char buf2[BLKSIZE];
   int iblk2;
   get_block(dev,2,buf2);
   GD *gp2=(GD *)buf2;
   iblk2=gp2->bg_inode_table;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk    = (ino-1)/8 + iblk2;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + offset;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}
/* this function releases a used minode pointed by mip. Each minode has a refCount, which is represents the number of usesr 
that are using the minode. iput() decrements the refcount by 1. If the refCount is nonzero, meaning that  the minode still 
has other users, the caller simply returns. if the caller is the last user of the minode (refCount=0), the INODE is writeen back to disk
if it is modified.*/
void iput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
  char buf2[BLKSIZE];
 INODE *ip;
 int iblk2;
   get_block(dev,2,buf2);
   GD *gp2=(GD *)buf2;
   iblk2=gp2->bg_inode_table;
 if (mip==0) 
     return;
//printf("********refcount1: %d \n",mip->refCount);
 mip->refCount--;
 //printf("********refcount2: %d \n",mip->refCount);
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write INODE back to disk */
 /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WORK: MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 *****************************************************/
   //write INODE back to disk
   block=(mip->ino-1)/8 + iblk2;
   offset=(mip->ino-1)%8;

   //get block containing this inode
   get_block(mip->dev,block,buf);
   ip=(INODE *)buf + offset; //ip points at INODE
   ip = (INODE *)buf + offset; //ip points at INODE
   *ip=mip->INODE; //copy INODE to inode in block
   put_block(mip->dev,block,buf); //write back to disk
   midalloc(mip); //mip->refCount=0;
} 

int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;
   
   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(mip->dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     printf("%4d  %4d  %4d    %s\n", 
           dp->inode, dp->rec_len, dp->name_len, dp->name);
     if (strcmp(temp, name)==0){
        printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }
     cp += dp->rec_len; //advance to the next rec_len which is the next file
     dp = (DIR *)cp; //pull dp to next entry
   }
   return 0;
}
/* the getino() function implements the file system tree traversal algorithm. it returns the INODE number (ino) of a speccified pathname
The function first uses the tokenize() function to break up the pathname into component strings. Then it calls teh search() function
to search for the token strings in successive directories.*/
int getino(char *pathname)
{
   int i, ino, blk, offset;
   char buf[BLKSIZE];
   INODE *ip;
   MINODE *mip;

   printf("getino: pathname=%s\n", pathname);
   if (strcmp(pathname, "/")==0)
      return 2;
  
   if (pathname[0]=='/')
     mip = root;
   else
     mip = running->cwd;

   mip->refCount++;         
  
   tokenize(pathname);

   for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
   //upward traversal e,g cd ../../
   //upward traversal when current dev is diff from root dev and ino is 2
      if(mip->dev!=root->dev && ino==2){
         printf("upward\n");
         //using the dev, looop thorugh the mount entries till the dev's are equal
         //the mount entry found will be point to the mounted inode
         for(int i=0;i<8;i++){ 
            if(mountTable[i].dev==mip->dev){
               iput(mip); //release the current mip to get a new one
               mip=mountTable[i].mounted_inode; //the mount entry found will be point to the mounted inode
               //added line below because without it i was having issues with changing cwd
              dev=mip->dev; //change global dev to mip->dev //added this line bcause had issue with cd ../../
               break;
            }
         }
      }
     
      else{
         iput(mip);
         mip = iget(dev, ino);
         
          //downward traversal
         if(mip->mounted==1 ){
            printf("downrad\n");
            MOUNT *mt=mip->mptr;//follow the minode's mountTable pointer to get the mount tabel entry
            //change ino to 2 since we have now just crossed into another mounted system
            ino=2;
            //change global dev to the mounted dev
            dev=mt->dev;

            iput(mip);
            //from the new dev num and root ino=2 do iget to get INODE into memory
            mip=iget(dev,ino);
            
            
         }
      }
      
   }

   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ]) //similar to search function in lab5
{
  // WRITE YOUR code here
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]

    MINODE *mip = parent;
    char *cp, c, sbuf[BLKSIZE], temp[256];
    DIR *dp;
   

    for (int i = 0; i < 12; i++)
    { 
        if (mip->INODE.i_block[i] == 0) //means no more info
            return;
        get_block(mip->dev, mip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            if (dp->inode == myino)
            {
                strncpy(myname, dp->name, dp->name_len);
                myname[dp->name_len] = 0;
                return 0;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return;


}
/* findino() returns the inode number of . and that of .. in myino*/
//note: pretty much returns the parent inode 
int findino(MINODE *mip, u32 *myino) // myino = i# of . (myinode) return i# of .. (parent)
{
  // mip points at a DIR minode
  // WRITE your code here: myino = ino of .  return ino of ..
  // all in i_block[0] of this DIR INODE.

   char *t;
   DIR *dp;
   char buf[BLKSIZE];

   get_block(mip->dev, mip->INODE.i_block[0], buf);
   t = buf;
   dp = (DIR *)buf; 

   *myino = dp->inode;

   t += dp->rec_len; //add rec_len to get the next file in the dir
   dp = (DIR *)t; //pull dp to next entry
   return dp->inode;
  
}

int tst_bit(char *buf, int bit){
   return buf[bit/8] & (1 << (bit%8));
}
int set_bit(char *buf, int bit){
   buf[bit/8] |= (1 << (bit%8));
}
int clr_bit(char *buf, int bit){//clear bit in char buf[]{
   buf[bit/8]&=~(1 << (bit%8));
} 
int incFreeInodes(int dev){
   char buf[BLKSIZE];
   //inc free inodes count in SUPER and GD
   get_block(dev,1,buf);
   sp = (SUPER *) buf;
   sp->s_free_inodes_count++;
   put_block(dev,1,buf);
   get_block(dev,2,buf);
   gp=(GD*)buf;
   gp->bg_free_inodes_count++;
   put_block(dev,2,buf);
}
int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

int decFreeInodes(int dev){
   //dec free inodes count in SUPER AND GD
   char buf[BLKSIZE];
   get_block(dev,1,buf);
   sp=(SUPER*)buf;
   sp->s_free_inodes_count--;
   put_block(dev,1,buf);
   get_block(dev,2,buf);
   gp=(GD*)buf;
   gp->bg_free_inodes_count--;
   put_block(dev,2,buf);

}
int decFreeBlocks(int dev){
   //dec free inodes count in SUPER AND GD
   char buf[BLKSIZE];
   get_block(dev,1,buf);
   sp=(SUPER*)buf;
   sp->s_free_blocks_count--;
   put_block(dev,1,buf);
   get_block(dev,2,buf);
   gp=(GD*)buf;
   gp->bg_free_blocks_count--;
   put_block(dev,2,buf);

}


int myaccess(char *filename, char mode)  // mode = r|w|x:
{
   int r;
   //SuperUser always okay 
   if (running->uid == 0)
      return 1; 

   int ino = getino(filename);
   MINODE *mip = iget(dev, ino);

   int bitsarray[16] = {0};
   int decimal = mip->INODE.i_mode;
   int index = 15; 
   while(decimal)
   {
      
      if(decimal % 2 == 1)
      {
         bitsarray[index] = 1;
      }  
      index --; 
      decimal = decimal / 2; 
   }
   //owner
   if(mip->INODE.i_uid == running->uid)
   {
      switch(mode){
         case 'r':   r = bitsarray[7];
                     break; 
         case 'w':   r = bitsarray[8];
                     break; 
         case 'x':   r = bitsarray[9];
                     break; 
      }
   }
   else // Other 
   {
      switch(mode){
         case 'r':   r = bitsarray[13];
                     break; 
         case 'w':   r = bitsarray[14];
                     break; 
         case 'x':   r = bitsarray[15];
                     break; 
      }
   }

   iput(mip);
  
   return r;
}

int checkowner(char *filename)
{
   //SuperUser always okay 
   if (running->uid == 0)
      return 1; 

   int ino = getino(filename);
   MINODE *mip = iget(dev, ino);

   //owner
   if(mip->INODE.i_uid == running->uid)
   {
      return 1;
   }
   else
   {
      return 0; 
   }

}
int enter_name(MINODE *pip, int ino, char *name){
    
   char buf[BLKSIZE];
   int blk;
   DIR *dp; char *cp;
   
   //in order to enter a new entry of name with n_len, we need to get the needed length
   int need_length=4*((8+strlen(name)+3)/4); 
   //assume only 12 direct blocks
  for(int i=0;i<12;i++)
  {
    
     if(pip->INODE.i_block[i]==0)
     {
       
        break;

     }
         
      ///step 4: code below gets the last entry in the data block
      get_block(pip->dev,pip->INODE.i_block[i],buf);
      dp=(DIR *)buf;
      cp=buf;

      while(cp + dp->rec_len < buf + BLKSIZE){
         cp+=dp->rec_len;
         dp = (DIR *)cp;
      }
         //dp now points at last entry in block
      int ideal_length= 4 * ((8+dp->name_len+3)/4);
      int remain = dp->rec_len-ideal_length;
      if(remain >= need_length) //refer to diagram on page 335
      { //don't need to allocate a new block sitll room
         //enter the new entry as the last entry and trim the previous entry rec_len to its ideal_length
         
         //first: trim the previous entry rec_len to its ideal_length
         dp->rec_len=ideal_length;
         //go to next entry
         cp+=dp->rec_len;
         dp = (DIR *)cp;

          //enter the new entry as the last entry 
         dp->name_len=strlen(name); //length of name
         strcpy(dp->name,name); //string copy of name
         dp->rec_len=remain; //make rec len equal to dp->rec_len-ideallength;
         dp->inode = ino; //inode number equals to paremeter ino
         put_block(pip->dev,pip->INODE.i_block[i],buf); //step 6: write back to disk
        

      }
      else{ //no room, step 5
      
        
         //allocate a new data block
         blk=balloc(pip->dev);
        
         //get block blk into buf
         get_block(pip->dev,blk,buf);
         dp=(DIR *)buf;

         dp->name_len=strlen(name);
         strcpy(dp->name,name);
         dp->rec_len=BLKSIZE;
         dp->inode=ino;
         
         put_block(pip->dev,blk,buf);//step 6: write back to disk
         
      }

      

  }
   
}
MOUNT *getmptr(int dev){

   MOUNT *ret;
   for(int i=0;i<8;i++){
      if(mountTable[i].dev==dev)
            ret=&mountTable[i];
            break;
               }

return ret;


}


