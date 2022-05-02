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

int mmkdir(char *pathname)
{
    char *dir,*base;
    char temp[256];
    int pino;
    MINODE *pmip;
    strcpy(temp,pathname);
   //get dev: if pathaname is absolute get the root dev if not get the cwd dev
    if(pathname[0]=='/'){
        dev=root->dev;
    }
    else{
        dev=running->cwd->dev;
    }
    
    //step1: divide pathanme into dirname and basename 
    printf("step1: divide pathanme into dirname and basename\n");
    dir=dirname(temp);
    base=basename(pathname);
   
    //step2: get ino number of directory of pathname (this has to exist)
    pino=getino(dir);
    printf("__________- %d\n",dev);
    //check if dirname exists
    printf("Step 2: checking if dirname exists\n");
    if(pino==0){
        printf("dirname %s does not exist\n",dir);
        return -1;
    }
    //step 2: get the inode corresponding to the dirname ino number
    
    pmip=iget(dev,pino);
    //step 2: check if the inode is a directory and not a file
     printf("check if the inode is a directory and not a file\n");
    if(!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("dirname %s is not a directory\n",dir);
        return -1;
    }
    // step3:basename must not exist alreadu in parent DIR
    printf("Step 3:checking that the basename does not already exist\n");
    if(search(pmip,base)!=0)
    {
        printf("Directory %s already exists\n",base);
        return -1;
    }



    mkdir_helper(pmip,base);
    pmip->INODE.i_links_count++;
    pmip->dirty=1;
    iput(pmip);
    
return 0;
}
/*This function creates a Directory*/
int mkdir_helper(MINODE *pip, char *base)
{
    printf("mkdir hellper: %d\n",dev);
    //alocaate an INODE and a disk block
    int ino=ialloc(dev);
    int blk=balloc(dev);
    MINODE *mip=iget(dev,ino); //load INODE into a minode;

    INODE *ip = &mip->INODE; 
    ip->i_mode=0x41ED; //040755: DIR type and persmissions
    ip->i_uid=running->uid; //owner uid
    ip->i_gid=running->gid; //group id
    ip->i_size=BLKSIZE; //size in bytes
    ip->i_links_count=2; //links count=2 becuase of . and ..
    ip->i_atime= ip->i_ctime=ip->i_mtime=time(0L);
    ip->i_blocks=2; //linux: Blocks ocunt in 512 byte chunks
    ip->i_block[0]=blk; //new IDR has one data block
    
    for(int i=1; i<=14;i++)
    {
        ip->i_block[i]=0;
    } 
    mip->dirty=1; //mark minode dirty
    iput(mip); //write INODE to disk

    //create data block for new DIR containing . and .. entries
    char buf[BLKSIZE];
    bzero(buf,BLKSIZE); 
    DIR *dp=(DIR *)buf;
    //make . entry
    dp->inode =ino;
    dp->rec_len=12;
    dp->name_len=1;
    dp->name[0]='.';
    //make .. entry : pino = parent DIR ino, blk = allocated block
    dp  = (char *)dp +12;
    dp->inode=pip->ino;
    dp->rec_len=BLKSIZE-12; //rec_len spans block
    dp->name_len=2;
    dp->name[0]=dp->name[1]='.';
    put_block(dev,blk,buf); //write blks on disks
    
    enter_name(pip,ino,base);




}

int ccreat(char *pathname){

    char *dir,*base;
    char temp[256];
    int pino;
    MINODE *pmip;
    strcpy(temp,pathname);
    
   //get dev: if pathaname is absolute get the root dev if not get the cwd dev
    if(pathname[0]=='/'){
        dev=root->dev;
    }
    else{
        dev=running->cwd->dev;
    }
    //step1: divide pathanme into dirname and basename 
    printf("step1: divide pathanme into dirname and basename\n");
    dir=dirname(temp);
    base=basename(pathname);
   
    //step2: get ino number of directory of pathname (this has to exist)
    pino=getino(dir);
    //check if dirname exists
    printf("Step 2: checking if dirname exists\n");
    if(pino==0){
        printf("dirname %s does not exist\n",dir);
        return -1;
    }
    //step 2: get the inode corresponding to the dirname ino number
    pmip=iget(dev,pino);
    //step 2: check if the inode is a directory and not a file
     printf("check if the inode is a directory and not a file\n");
    if(!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("dirname %s is not a directory\n",dir);
        return -1;
    }
    // step3:basename must not exist alreadu in parent DIR
    printf("Step 3:checking that the basename does not already exist\n");
    if(search(pmip,base)!=0)
    {
        printf("File %s already exists\n",base);
        return -1;
    }
     creat_helper(pmip,base);
    pmip->dirty=1;
    iput(pmip);
    return 0;

}

int creat_helper(MINODE *pip, char *base){
     //alocaate an INODE and a disk block
    int ino=ialloc(dev);
    int blk=balloc(dev);
    MINODE *mip=iget(dev,ino); //load INODE into a minode;

    INODE *ip = &mip->INODE; 
    ip->i_mode=0x81A4; //FILE type and persmissions
    ip->i_uid=running->uid; //owner uid
    ip->i_gid=running->gid; //group id
    ip->i_size=0; //size in bytes
    ip->i_links_count=1; 
    ip->i_atime= ip->i_ctime=ip->i_mtime=time(0L);
    ip->i_blocks=2; //linux: Blocks ocunt in 512 byte chunks
    ip->i_block[0]=0; 
    
    for(int i=1; i<=14;i++)
    {
        ip->i_block[i]=0;
    } 
    mip->dirty=1; //mark minode dirty
    iput(mip); //write INODE to disk

     enter_name(pip,ino, base);
}

