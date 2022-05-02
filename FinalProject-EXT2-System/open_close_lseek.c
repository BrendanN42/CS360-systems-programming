#include "type.h"



extern char line[128], cmd[32], pathname[128];
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
 //truncate function is based on partial code from lab 5
int truncate(MINODE *mip){
    char buf[BLKSIZE];
    char buf1[BLKSIZE];
    char buf2[BLKSIZE]; 
    //deallocating direct blocks
    for(int i=0;i<12;i++){
        if(mip->INODE.i_block[i]==0)
            break;
        //printf("before %d\n",mip->INODE.i_block[i]);
        bdalloc(dev,mip->INODE.i_block[i]);
       // printf("after %d\n",mip->INODE.i_block[i]);
    }
    //deallocating indirect blocks
    if(mip->INODE.i_block[12]){
        get_block(dev,mip->INODE.i_block[12],buf);
        u32 *temp=(u32 *)buf;
        int i=0;
        while(i<256){
            if(temp==0)
                break;
            bdalloc(dev,temp);   
            temp++;
            i++;
        }
        bdalloc(dev,mip->INODE.i_block[12]);
    }
      
    //deallocating double indirect blocks 256*256
    if(mip->INODE.i_block[13]){
        get_block(dev,mip->INODE.i_block[13],buf1);
        u32 *temp1 = (u32 *)buf1;
        get_block(dev,*temp1,buf2);
        u32 *up =(u32 *)buf2; 

        for(int i=0; i<256 && *temp1;i++)
        {
            for(int j=0;j<256 && *up;j++)
            {
                bdalloc(dev,up); 
                up++;
            }
            temp1++;
            memset(buf2,0,BLKSIZE);
            get_block(fd,*temp1,buf2);//read in the next block to buf2
            up =(u32 *)buf2;
        }
        bdalloc(dev,mip->INODE.i_block[13]);
    }
    //checking to make sure there are no blocks in 12 and 13
    if(mip->INODE.i_block[12]){
        printf("FALSE12\n");
        return -1;
    }
    if(mip->INODE.i_block[13]){
        printf("FALSE13\n");
        return -1;
    }


    mip->INODE.i_size=0;
    mip->dirty=1;
    return 0;
}
int open_file(char *filename, int flag){

    OFT *oftp;
     
    //mode = 0(RD) or 1(WR) or 2(RW) or 3(APPEND)
    printf("mode %d\n",flag);
    if(flag!=0 && flag!=1 && flag!=2 && flag!=3){
        printf("incorrect mode\n");
        return -1;
    }
    
  
    //get file's minode
    int ino = getino(filename);
    if(ino==0){// file does not exist
    printf("File doesnt exist creating file now...\n");
        ccreat(filename);
        ino=getino(filename);
    }
   
    
    //get minode pointer
    MINODE *mip=iget(dev,ino);
      printf("2file size: %d\n",mip->INODE.i_size);
    //verify it is a REGULAR file
    if(!S_ISREG(mip->INODE.i_mode)){
        printf("Not a regular file\n");
        return -1;
    }
    //check if prmission is OK
    if(mip->INODE.i_uid!=running->uid)
    {
        printf("Permission NOT ok\n");
        return -1;
    }
     //check whether the file is ALREADY opened with INCOMPATIBLE MODE
    for(int i=0;i<10;i++){
        if(running->fd[i]!=NULL){
             //have to add running->fd[i]->minodePtr==mip to make sure it was pointing to the same minode pointer
            if(running->fd[i]->minodePtr==mip){ //check if its already opened for W, RW, APPEND
            //printf("%d=[%d,%d]\n",i, running->fd[i]->minodePtr->dev,running->fd[i]->minodePtr->ino);
                if(running->fd[i]->mode!=0 && flag!=0){
                    printf("file is open with incompatible mode\n");
                    return -1;
                }
               
            }
        }
        else   //end of open files
            break;
    }
   
    //allocate a FREE openFileTable (OFT) and fille in values
    oftp=(OFT *)malloc (sizeof(OFT));
    oftp->mode=flag; //mode = 0|1|2|3 for R|W|RW|APPEND 
    oftp->refCount=1;
    oftp->minodePtr=mip; //point at the file's minode[]
    printf("1file size: %d\n",mip->INODE.i_size);
    printf("file size: %d\n",oftp->minodePtr->INODE.i_size);
    
    //Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:
    switch(flag){
        case 0: oftp->offset=0; //R: offset =0
                break;
        case 1: truncate(mip); //W: truncate file to 0 size
                break;
        case 2: oftp->offset=0; //RW: do NOT Truncate file
                break;
        case 3:oftp->offset=mip->INODE.i_size; //APPEND mode
                break;
        default: printf("invalid mode\n");
                return -1;
    }
    //find the smallest i in the running PROC's fd[] such that fd[i] is NULL
    int small_i=0;
    for(int i=0;i<10;i++){
       
        if(running->fd[i]==NULL){
            small_i=i;
            running->fd[i]=oftp; //let running->fd[i] point at the OFT entry
            break;
        }
        else  
         printf("[%d,%d]\n",running->fd[i]->minodePtr->dev,running->fd[i]->minodePtr->ino);
    }
    //update time field based on mode
    if(flag==0){ //R
        mip->INODE.i_atime=time(0L);
    }
    else{ //W|RW|APPEND 
        mip->INODE.i_atime=time(0L);
        mip->INODE.i_mtime=time(0L);
    }
    //mark minode dirty
    mip->dirty=1;

    //iput(mip);
    printf("small_i: %d\n",small_i);
    //return i as the file descriptor
    return small_i;


}

int close_file(int fd) 
{
    //verify fd is within range
    if(fd<0 || fd>=10){
        printf("File Descriptor is out of range\n");
        return -1;
    }
    //verify running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd]==NULL){
        printf("File is not pointing at a OFT and is not open\n");
        return -1;
    }

    OFT *oftp;
    oftp = running->fd[fd];
    running->fd[fd]=0;
    oftp->refCount--;
    if(oftp->refCount>0) 
        return 0;
    //last user of this OFT entry ==> dispose of the minode
    MINODE *mip=oftp->minodePtr;
    iput(mip);
    return 0;


}
int llseek(int fd, int position){

    if(fd<0 || fd>=10){
        printf("File Descriptor is out of range\n");
        return -1;
    }
    if(running->fd[fd]==NULL){
        printf("File is not pointing at a OFT and is not open\n");
        return -1;
    }
   
    if(position<0){
        printf("position is out of bounds\n");
        return -1;
    }
    if(position>running->fd[fd]->minodePtr->INODE.i_size){
         printf("position is out of bounds\n");
        return -1;
    }

    int original_pos=running->fd[fd]->offset;
    running->fd[fd]->offset=position;
    return original_pos;


}

int pfd(){

printf("fd\tmode\toffset\tINODE\n");
printf("-----------------------------\n");
char mode[64];
for(int i=0;i<10;i++){
    if(running->fd[i]!=NULL){
        if(running->fd[i]->mode==0)
            strcpy(mode,"READ");
        else if(running->fd[i]->mode==1)
             strcpy(mode,"WRITE");
        else if(running->fd[i]->mode==2)
            strcpy(mode,"RDWR");
         else if(running->fd[i]->mode==3)
             strcpy(mode,"APPEND");
        printf("%d\t%s\t%d\t[%d, %d]\n",i,mode,running->fd[i]->offset,running->fd[i]->minodePtr->dev,running->fd[i]->minodePtr->ino);
    }
    else
        break;
}

}