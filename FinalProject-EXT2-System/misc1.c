#include "type.h"
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd,dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];


int statt(char *pathname){
    
    if (pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    char ftime[256];
    int ino=getino(pathname);
    if(ino==0)
        return;
    MINODE *mip = iget(dev, ino);

    strcpy(ftime, ctime(&(mip->INODE.i_mtime))); //printing time in calendar form
    ftime[strlen(ftime)-1]=0;
    printf("dev=%d ino=%d mod=%d\n", mip->dev,mip->ino,mip->INODE.i_mode);
    printf("uid=%d gid=%d nlink=%d\n",mip->INODE.i_uid, mip->INODE.i_gid, mip->INODE.i_links_count);
    printf("size=%d time=%s\n",mip->INODE.i_size,ftime);
    iput(mip);
}
int utimee(char *pathname){
   
    if (pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    int ino=getino(pathname);
    if(ino==0)
        return;
     
    MINODE *mip = iget(dev, ino);
    
    time_t var=mip->INODE.i_atime;
    char *btime=ctime(&var);
   
    printf("Access time before: %s",btime);
    mip->INODE.i_atime=time(0L);
    time_t var2=mip->INODE.i_atime;
    char *atime=ctime(&var2);
    printf("Access time after: %s",atime);
    iput(mip);

}
// int chmod(char *pathname){



// }


