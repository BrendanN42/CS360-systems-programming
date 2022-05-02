
#include "type.h"


extern char line[128], cmd[32], pathname[128];
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MOUNT mountTable[8];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern int fd, dev;
extern char gpath[128];
extern char *name[64];
extern int n;
/************* cd_ls_pwd.c file **************/
int cd()
{
  int ino;
  
  printf("cd %s\n", pathname);
  
  if(pathname[0]=='/'){
        dev=root->dev;
    }
    else{
        dev=running->cwd->dev;
    }
   
  if(strcmp(pathname, "") == 0)
  {
    ino = 2;
    dev=root->dev;
  }

  else
  {
      if(myaccess(pathname, 'x') == 0)
      {
        printf("You do not have the permission to do this");
        return -1;
      }
      ino = getino(pathname);//get INODE number (ino) of the pathname
  }
    
  
  if (ino == 0)
  {
    printf("Oops - ino can't be found\n");
    return 0;
  }
  
  MINODE *mip = iget(dev, ino);  //returns MINODE cooresponding to the ino number of pathname

  if (S_ISDIR(mip->INODE.i_mode))
  {
    iput(running->cwd); //releases the current cwd minode
    running->cwd = mip; //chang the cwd to the MINODE 
   
    return 1;
  }

  printf("Oops - mip is not a directory\n");
  return 0;
 
 
}

int ls_file(MINODE *mip, char *name) //refer to ls_file function inn lab 4
{
  //printf("ls_file: to be done: READ textbook!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  char ftime[256];
  char *t1 = "xwrxwrxwr-------"; 
  char *t2 = "----------------";
  
  if(S_ISREG(mip->INODE.i_mode))
    printf("%c", '-');
  if(S_ISDIR(mip->INODE.i_mode))
    printf("%c",'d');
  if(S_ISLNK(mip->INODE.i_mode))
    printf("%c", 'l');
  
  for(int i=8;i>=0;i--)
  {
    if(mip->INODE.i_mode & (1<<i)) //print r|w|x
      printf("%c",t1[i]);
    else 
      printf("%c",t2[i]);
  }

  printf("%4d", mip->INODE.i_links_count); //link count
  printf("%4d", mip->INODE.i_gid); //gid
  printf("%4d ", mip->INODE.i_uid);//uid
  
  strcpy(ftime, ctime(&(mip->INODE.i_mtime))); //printing time in calendar form
  ftime[strlen(ftime)-1]=0;
  printf("%s ",ftime);
  printf("%8d ", mip->INODE.i_size); //file size
  printf("%s ", name); //print file base name
  //print linkname if symbolic file
  if(S_ISLNK(mip->INODE.i_mode))
  {
    printf(" -> %s", (char *)mip->INODE.i_block);
  }

  printf("[%d %d]\n",mip->dev,mip->ino);
  
  return 0;
  
}

int ls_dir(MINODE *mip,int dev)
{
 
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE *t;
  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
  
  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     t=iget(dev,dp->inode);
     ls_file(t,temp);
     t->dirty=1;
     iput(t);
     cp += dp->rec_len;
     dp = (DIR *)cp;
  }
  printf("\n");
}

int ls()
{
  if(pathname[0]=='/'){
        dev=root->dev;
    }
    else{
        dev=running->cwd->dev;
    }
  
  if(strcmp(pathname,"")!=0)
  {
    
      int ino= getino(pathname); //get ino of file/dir
      if(ino==0) //if ino is zero that means INODE doesn't exist
      {
        printf("The inode does not exist or you do not have the correct permissions\n");
        return -1;
      }
      else{
       
        MINODE *mip=iget(dev,ino);  
        if(S_ISDIR(mip->INODE.i_mode)){ // check to see if it is a dir, if so call ls_dir
          ls_dir(mip,dev);
        }
        else{
          ls_file(mip,pathname); //this means the pathname is a file
        }
       iput(mip);
      }
  }
  else{
    ls_dir(running->cwd,dev);
  }
 
  
}

char *pwd(MINODE *dir) 
{
  printf("========================================\n");
  
  if (dir == root){ //base case if the MINODE is equal to the root MINODE
    printf("/\n");
    
  }
  else //else we must use recursion 
    {
        printf("CWD = ");
        pwdHelper(dir);
        printf("\n");
    }
  printf("========================================\n");
  return;
}

void pwdHelper(MINODE *dir)
{
  int myino;
  char buf[BLKSIZE], myname[256];
   MINODE *temp,*pip;
  if (dir == root) 
  {
      return;
  }
  //cross mounting check: if the current dir dev is different then root AND ino is currently 2 then we need to cross mounts
  if(dir->ino==2 && dir->dev!=root->dev){
    for(int i=0;i<8;i++){
      if(dir->dev==mountTable[i].dev){ //need to get pointer to mounted inode; find this by looping through table entries till we find the devs equal
        temp=mountTable[i].mounted_inode;
        break;
      }
    }
    
    int d=temp->dev;
   
    int p=findino(temp,&myino); //use the mounted inode to find ino number of parent and and child

    pip=iget(d,p); //get the minode cooresspinding to the parent ino number


  }
  else
  {

  int parent = findino(dir, &myino); //find parent inode and my inode EX: dir1/dir3 -> parent=dir1 and my inode=dir3
  pip = iget(dir->dev, parent);//returns MINODE cooresponding to the parent ino number

  }
  findmyname(pip, myino, myname);
  pwdHelper(pip);
  iput(pip);//release the  minode
  printf("/%s", myname);
  return;
}


