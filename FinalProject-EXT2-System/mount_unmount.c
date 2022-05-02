#include "type.h"
#include <stdio.h>
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
//this function just displays the current mounted filesystems if there are no parameters
int mounthelper(){

    for(int i=0;i<8;i++){
            if(mountTable[i].dev==0)
                return 0;
            printf("%s mount on %s ",mountTable[i].name,mountTable[i].mount_name);
            MINODE *mip=mountTable[i].mounted_inode;
            printf("[%d,%d]\n",mip->dev,mip->ino);
            
        }


}
//this function carries out the mount function with parameters
/*mounts a file system to a mount_point directory. It allows the 
file system to include other file systems as parts of the existing 
file system. The data structures used in mount are the MOUNT table and the 
in-memory minode of the mount_point directory*/
int my_mount(char *filesys, char *mount_point){
   
    printf("filesys: %s, mount_point: %s\n",filesys,mount_point);
    MOUNT *mt;
    SUPER *sp2;
    GD *gp2;
    char buf[BLKSIZE];
    
    //first check if the filesystem is already mounted, else allocate a free MOUNT table entry
    for(int i=0;i<8;i++){
        if(strcmp(filesys,mountTable[i].name)==0 && mountTable[i].dev!=0){
            printf("Reject Filesystem: %s is already mounted on mountTable[%d]\n",filesys,i);
            return -1;
        }
    }
    //allocate a free mount table entry
    for(int i=0;i<8;i++){
        if(mountTable[i].dev==0){
           mt=&mountTable[i];
            break;
        }
    }
    //open filesys for RW and use the fd numbers as the new dev
     if ((fd = open(filesys, 2)) < 0){
    printf("open %s failed\n", filesys);
    return -1;
  }
    get_block(fd, 1, buf);
    sp2=(SUPER *)buf;
    //have to get the SUPER block to check if its an EXT2 filey system
    if (sp2->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp2->s_magic);
      return -1;
  }  
  //for mount_point: find its ino then get its minode
    int pno=getino(mount_point);
    MINODE *mip=iget(dev,pno);
    //verify mount_point is a DIR //can only mount on a DIR, not a file
   if(!S_ISDIR(mip->INODE.i_mode)){
        printf("%s is not a DIR: REJECT\n",mount_point);
        return -1;
    }
    //also verify the mount_point is NOT busy, do this by checking the refcount
    if(mip->refCount>2){
        printf("%s is busy: REJECT\n",mount_point);
        return -1;
    }
    //Record new DEV, ninodes, nblocks, bmap, imap, iblk in mountTable[] 
    
    int ninodes2= sp2->s_inodes_count;
    int nblocks2=sp2->s_blocks_count;
    mt->dev=fd;
    mt->ninodes=ninodes2;
    mt->nblocks=nblocks2;

    get_block(fd,2,buf);
    gp2=(GD *)buf;

    int bmap2 = gp2->bg_block_bitmap;
    int imap2 = gp2->bg_inode_bitmap;
    int iblk2 = gp2->bg_inode_table;

    mt->bmap=bmap2;
    mt->imap=imap2;
    mt->iblk=iblk2;
   
   strcpy(mt->name,filesys);
   strcpy(mt->mount_name,mount_point); 
    //mark mount_point's minode as being mounted and let it point at the Mount table entry
    mip->mounted=1;
    mip->mptr=mt;
     mt->mounted_inode=mip;
    return 0;

}

/*un-mounts a mounted file system. It detaches a mounted file system from the mounting point,
x where filesys may be a virtual diak name or a mounting point directory name*/
int umount(char *filesys){

//check to see if filesystem is mounted
int check=0;
int m_dev;
MOUNT *mt;
int index=0;
//check to see if filesystem is mounted -- do this by looping through the MOUNT table 
//and checking the if the name is equal to the parameter and the cooresponding dev is not 0
for(int i=0;i<8;i++){
        if(mountTable[i].dev!=0 && strcmp(filesys,mountTable[i].name)==0){
            m_dev=mountTable[i].dev;
            mt=&mountTable[i];
            check=1;
            index=i;
            break;
        }
    }
    if(check==0){
        printf("filesys: %s is not mounted\n",filesys);
        return -1;
    }
    //check whether any file is still active in the mounted filesys
    //do this by looping through all the current MINODE and checking the dev
    for(int i=0;i<NMINODE;i++){
        if(m_dev==minode[i].dev){
            printf("ERROR: Active file still exist\n");
            return -1;
        }
    }
    //find the mount_point's inode (found in the first for loop)
    MINODE *mip=mt->mounted_inode;
    //reset it to not mounted
    mip->mounted=0;
    //iput() the minode.  (because it was iget()ed during mounting)
    iput(mip);
    mountTable[index].dev=0;

    return 0;


}
