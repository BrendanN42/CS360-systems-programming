/************** lab5base.c file ******************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;
typedef unsigned int u32;
SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define BLK 1024

int fd;             // opened vdisk for READ
int inodes_block;   // inodes start block number

char gpath[128];    // token strings
char *name[32];
int n;
// get_block() reads a disk block into a buf[ ]
int get_block(int fd, int blk, char *buf)
{
   lseek(fd, blk*BLK, 0);
   read(fd, buf, BLK);
}   

int search(INODE *ip, char *name)
{
  // Chapter 11.4.4  int i; 
  // Exercise 6
  char *cp; //char pointer
  char buf[BLK],temp[256];
  DIR *dp; //dir pointer
  
  for(int i=0;i<12;i++)
  {
   printf("i = %d i_block[0] = %d\n",i, ip->i_block[i]);
     get_block(fd,ip->i_block[i],buf);//get data block into buf
     dp=(DIR *)buf; //as Dir
     cp=buf;
     printf("    i_number    rec_len   name_len    name\n");
     while(cp<buf+BLK)
     {
        strncpy(temp,dp->name,dp->name_len); //make name a string
        temp[dp->name_len]=0; //ensure NULL at end
        printf("%8d%12d%10d        %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
        if(strcmp(name,temp)==0) //check if name was found
            return dp->inode;
         cp+=dp->rec_len; //advance cp by rec_len
         dp=(DIR *)cp; //pull dp to next entry

     }
   
  
 }
  return 0;
}

/*************************************************************************/
int tokenize(char *pathname)
{
  n=0;
  strcpy(gpath,pathname);
  char *token=strtok(gpath,"/");
  while(token)
  {
     name[n++]=token;
     token=strtok(0,"/");
  }
  return 0;
} 

char *disk = "vdisk";

int main(int argc, char *argv[])   // a.out pathname
{
   
  char buf[BLK];  // use more bufs if you need them

//   1. open vdisk for READ: print fd value
   fd=open("vdisk", O_RDONLY);
   if(fd<0){
      printf("FAILED to open disk\n");
      exit(1);
   }
   printf("1. open vdisk for READ: fd = %d\n",fd);
  // printf("%d\n",fd);
 // 2. read SUPER block #1 to verify EXT2 fs : print s_magic in HEX

   get_block(fd,1,buf);
   sp = (SUPER *)buf; // as a super block structure

 
   printf("2. read SUPER block #1 to verify EXT2 fs : s_magic = %X \n",sp->s_magic);
   if(sp->s_magic!=0xEF53){
      printf("NOT AN EXT2 FS\n");
      exit(2);
   }
//   3. read GD block #2 to get inodes_block=bg_inode_table: print inodes_block 
   get_block(fd,2,buf);
   gp = (GD *)buf;
   inodes_block=gp->bg_inode_table;
   printf("3. read GD block #2 to get inodes_block=bg_inode_table: inodes_block = %d\n",inodes_block);
//   4. read inodes_block into buf[] to get root INODE #2 : set ino=2 
   int ino=2;
   printf("4. read inodes_block into buf[] to get root INODE #2 : ino = %d\n",ino);
   get_block(fd,inodes_block,buf);
   INODE *ip = (INODE *)buf + 1;
			    
//   5. tokenize pathame=argv[1]);

if(argc>1)
{
   printf("5. tokenize %s : ",argv[1]);
   tokenize(argv[1]);
   for(int i=0;i<n;i++)
   {
      printf("%s ", name[i]);
   }
   printf("\n");
   //   6. 
      for (int i=0; i<n; i++){
        printf("===========================================\n");
        printf("search name[%d]=%s in ino=%d\n", i, name[i], ino);
        ino = search(ip, name[i]);
         printf("found %s ino = %d\n",name[i],ino);

        if (ino==0){
           printf("name %s does not exist\n", name[i]);
           exit(1);
        }
        // MAILman's algrithm:
        int blk    = (ino-1) / 8 + inodes_block;
        int offset = (ino-1) % 8;
        printf("blk = %d offset = %d\n",blk,offset);
        get_block(fd, blk, buf);

        ip = (INODE *)buf + offset;
        printf("enter a key to continue: ");
        getchar();
   } 
  
// //7. HERE, ip->INODE of pathname
   printf("****************  DISK BLOCKS  *******************\n");
   for (int i=0; i<15; i++){
      if (ip->i_block[i]) 
         printf("block[%2d] = %d\n", i, ip->i_block[i]);
   }

   printf("================ Direct Blocks ===================\n");
   for(int i=0;i<12;i++)
   {
      if(ip->i_block[i])
         printf("%d ", ip->i_block[i]);
   }
   printf("\n");
   if (ip->i_block[12]){
      printf("===============  Indirect blocks   ===============\n"); 
      char buf3[BLK];
      get_block(fd,ip->i_block[12],buf3);//read i_block[12] into buf3
      
      u32 *temp = (u32 *)buf3; //temp is the first disk block number
      int i=0;
      while(i<256)
      {
        printf("%d ",*temp);
         temp++;//iterate through the block and print out the disk blocks
        
         i++;
     }
     printf("\n");
   }

   if (ip->i_block[13]){
      printf("===========  Double Indirect blocks   ============\n"); 
    
      char buf1[BLK];
      get_block(fd,ip->i_block[13],buf1); //read i_block[13] into buf1


      u32 *temp1 = (u32 *)buf1; //temp1 points to the first block
      int i=0;
      char buf2[BLK];
      get_block(fd,*temp1,buf2);
      u32 *up =(u32 *)buf2; //up points to the first disk block
    
      
      for(int i=0;i<256 && *temp1;i++)//iterate through temp1 to get the next block
      {
         for(int j=0;j<256 && *up;j++) //iterate through up to print out the disk blocks
         {
         
            printf("%d ",*up);
            up++;
         }
      
         temp1++;
         memset(buf2,0,BLK);
         get_block(fd,*temp1,buf2);//read in the next block to buf2
         up =(u32 *)buf2;//re-initialize up to retrieve the next group of disk blocks

      }

      printf("\n");
   }
}

  close(fd);

}
