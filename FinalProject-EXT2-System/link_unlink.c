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

int my_link(char* old_file, char*new_file)
{

    MINODE* omip, *pmip;
    if (pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    //1. Verfiy old file exists and is not a dir 

    int oino = getino(old_file);
    if(oino==0)
        return -1;
    // set dev up for iget 
    
    omip = iget(dev, oino);

    if(S_ISDIR(omip->INODE.i_mode)){
         printf("filename %s is a directory\n", old_file);
        return -1;
    }

    //new file must not exist yet 
    if(getino(new_file) != 0)
    {
        printf("filename %s already exists\n",new_file);
        return -1;
    }
    // creat new_file with the same inode as the old_file 
    char* parent = dirname(new_file);
    char* child = basename(new_file);
    int pino = getino(parent);
    pmip = iget(dev,pino);
    enter_name(pmip,oino,child);

    omip->INODE.i_links_count++; // inc INODE’s links_count by 1
    omip->dirty = 1;// for write back by iput(omip)
    iput(omip);
    iput(pmip);

}


int unlink(char* filename)
{   
    
    if (pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    //get filename's minode
    if(checkowner(pathname) == 0)
    {
       printf("Not the owner cannot unlink\n");
       return -1; 
    }

    int ino = getino(filename);
    if(ino==0)
        return -1;
    
    MINODE *mip = iget(dev, ino);
    
    if(S_ISDIR(mip->INODE.i_mode)){
         printf("filename %s is a directory\n", filename);
        return -1;
    }
    
    char* parent = dirname(filename);
    char* child = basename(filename);

    int pino = getino(parent);
    MINODE *pmip = iget(dev,pino);

    rm_child(pmip, child);
    pmip->dirty = 1;
    iput(pmip);
    // decrement INODE’s link_count by 1
    mip->INODE.i_links_count--;
    if (mip->INODE.i_links_count > 0)
    mip->dirty = 1; // for write INODE back to disk
    else
    { // if links_count = 0: remove filename
        INODE *ip = &mip->INODE;
        //deallocate all data blocks in INODE 
        for (int i = 0; i < 12; i++) 
        {
            if (ip->i_block[i] == 0)
                break;
            // now deallocate block
        balloc(ip->i_block[i]);
        ip->i_block[i] = 0;
        }
        idalloc(mip->dev,mip->ino);
    }
 
    iput(mip);
    // release mip


}

