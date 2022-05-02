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

int symlink(char *old_file, char *new_file)
{

     if(pathname[0]=='/'){
        dev=root->dev;
    }
    else{
        dev=running->cwd->dev;
    }
    //1. Verfiy old file exists 
    int oino = getino(old_file);
    // set dev up for iget 
   
    if(getino(old_file) == 0)
    {
        printf("filename %s doesn't exist\n",old_file);
        return -1;
    }
    
    //new file must not exist yet 
    if(getino(new_file) != 0)
    {
        printf("filename %s already exists\n",new_file);
        return -1;
    }

    //creat new_file 
    ccreat(new_file);
    int nino = getino(new_file);
    if(nino == 0) 
    {
        printf("filename %s doesn't exist\n",new_file);
        return -1;
    }

    MINODE *mip = iget(dev, nino);
    mip->INODE.i_mode = 0xA1FF; //0xA000 EXT2_S_IFLNK; 
    mip->dirty = 1;

    strncpy(mip->INODE.i_block, old_file, 84);

    mip->INODE.i_size = strlen(old_file) + 1; 

    iput(mip);
}



int readlink(char* file_name, char* buffer)
{
    int ino = getino(file_name);

    if(ino==0)
        return -1;
    // set dev up for iget 
    if (pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    
    MINODE *mip = iget(dev, ino); 
    if(mip->INODE.i_mode != 0xA1FF) 
    {
        printf("%s is not a LNK file\n", file_name);
        return -1;
    }
    
    strncpy(buffer, mip->INODE.i_block,84);
    iput(mip);
    return(strlen(buffer));


}

