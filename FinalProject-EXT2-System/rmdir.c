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
//1 is not empty, 0 is empty
int is_empty(MINODE *mip){ //algorithm is very similar to search but we have to look through each data block 
    char *cp, c, buf[BLKSIZE], temp[256];
    DIR *dp;
    if(mip->INODE.i_links_count>2){ //check if dir is empty 
        printf("Directory %s is NOT empty\n",pathname);
        return 1;
    }    
    if(mip->INODE.i_links_count==2){

        for(int i=0;i<12;i++){
            if(mip->INODE.i_block[i]==0)
                break;
            get_block(mip->dev,mip->INODE.i_block[i],buf);
            dp=(DIR *)buf;
            cp=buf;
            while(cp<buf + BLKSIZE){
                strncpy(temp,dp->name,dp->name_len);
                temp[dp->name_len]=0;
                //printf("%4d  %4d  %4d    %s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
                if(strcmp(temp,".")!=0 && strcmp(temp,"..")!=0){
                     printf("Directory %s is NOT empty\n",pathname);
                    return 1;
                }
                cp+=dp->rec_len;
                dp = (DIR *)cp;
            
            }
        }
    }
    return 0; //yay empty
}
int rm_child(MINODE *pmip,char *name){
    char *cp, *cp2;
    DIR *dp,*dp2,*dpPrev;
    char buf[BLKSIZE], temp[256];

    for(int i=0;i<12;i++){
        get_block(pmip->dev,pmip->INODE.i_block[i],buf);
        dp=(DIR *)buf;
        cp=buf;
        dp2=(DIR *)buf;
        cp2=buf;
        while(cp<buf+BLKSIZE){
            strncpy(temp,dp->name,dp->name_len);
            temp[dp->name_len]=0;
            if(strcmp(temp,name)==0){
                if(cp==buf && dp->rec_len==BLKSIZE){ //first and only entry in a data block
                    bdalloc(pmip->dev,pmip->INODE.i_block[i]); //deallocate the data block
                    pmip->INODE.i_size-=BLKSIZE;//reduce parents file size by BLKSIZE

                    //compact parent's i_block[] array to eliminate the deleted entry if its between nonzero entries
                    int j=i;
                    while(j+1<12 && pmip->INODE.i_block[j+1]!=0)
                    {
                        j++;
                        get_block(pmip->dev,pmip->INODE.i_block[j],buf);
                        put_block(pmip->dev,pmip->INODE.i_block[j-1],buf);

                    }
                    return 0;
                }
                else if(cp+dp->rec_len==buf+BLKSIZE){ //last entry in block
                    dpPrev->rec_len += dp->rec_len; //Absorb its rec_len to the predecessor entry
                }
                else{ //Else the entry is first but not the only entry or  the entry is in the middle of the block
                    while(cp2 +dp2->rec_len<buf+BLKSIZE){ //find the last entry in block
                        cp2+=dp2->rec_len;
                        dp2=(DIR*)cp2;
                    }
                    //calculate the difference between the end of copy block to the start of copy block (size will later be used to shift left)
                    int size=(buf+BLKSIZE)-(cp+dp->rec_len);
                    dp2->rec_len+=dp->rec_len; //add the deleted rec_len to the last entry
                    memmove(cp,cp+dp->rec_len,size); //shift all trailing entries LEFT overlay the deleted entry
                }
                put_block(pmip->dev,pmip->INODE.i_block[i],buf);
                return 0;
            }
            dpPrev=dp;
            cp+=dp->rec_len;
            dp = (DIR *)cp;
               
            }
        }
        printf("No child was found\n");
        return 1; //error
}

int rrmdir(char *pathname){
    char *dir,*base;
    char temp[256];
    int pino;
    MINODE *pmip;
    strcpy(temp,pathname);
     //get dev
    if(pathname[0]=='/'){
        dev=root->dev;
    }
    else{
        dev=running->cwd->dev;
    }
    //checking ownership 
    if(checkowner(pathname) == 0)
    {
       printf("Not the owner cannot rmdir\n");
       return -1; 
    }

    int ino=getino(pathname);
    if(ino==0)
        return -1;
    //step 1: get the in-memory INODE of pathname
    MINODE *mip=iget(dev,ino);
    //step2: verify INODE is a DIR, minode is not busy, and DIR is empty
    if(!S_ISDIR(mip->INODE.i_mode)){
         printf("dirname %s is not a directory\n",pathname);
        return -1;
    }
    if(mip->refCount>2){
        printf("Error: node is BUSY, refcount: %d\n",mip->refCount);
        return -1;
    }
    if(is_empty(mip)==1){
        //not empty
        return -1;
    }
    //Deallocate disk blocks and the inode
    for(int i=0;i<12;i++){
        if(mip->INODE.i_block[i]==0)
            continue;
        bdalloc(mip->dev,mip->INODE.i_block[i]);
    }
    idalloc(mip->dev,mip->ino);
    
    iput(mip);
    dir=dirname(temp);
    base=basename(pathname);
   
    pino=getino(dir);
    pmip=iget(mip->dev,pino);

    rm_child(pmip,base);
    pmip->INODE.i_links_count--;
    pmip->dirty=1;

    iput(pmip);


}