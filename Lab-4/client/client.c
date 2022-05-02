#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>     // for dirname()/basename()
#include <time.h> 

#define MAX 256
#define PORT 1234

char line[MAX], ans[MAX], linetok[MAX];
char* arg[64]; //store args 
char buff[MAX]; //to store get stuff
int n;

struct sockaddr_in saddr; 
int sfd;
char*cmd[]={"lcat","lls",  "lcd",  "lpwd",  "lmkdir",  "lrmdir", "lrm"};

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

char stream_end[] = "ENDOFSTREAM\n"; //$ is ASCII code for 

void tokenize(char *cmdline)
{
  n = 0;
  char *s = strtok(cmdline, " ");
  while (s != NULL)
  {
    arg[n++] = s;
    s = strtok(NULL, " ");
  }

}
int findCmd(char *command)
{
   int i = 0;
   while(cmd[i]){
     if (strcmp(command, cmd[i])==0)
         return i;
     i++;
   }
   return -1;
}
int l_mkdir(char *pathname)
{
    int r=mkdir(pathname,0755);
    return r;
}
int l_rmdir(char *pathname)
{
    int r=rmdir(pathname);
    return r;
}
int l_rm(char *pathname)
{
    int r = unlink(pathname);
    return r;
}
int l_cd(char *pathname)
{
  int r = chdir(pathname);
  return r;
}
int l_cat(char *pathname)
{

  int temp;
  char buf[512];
  FILE *fd = fopen(pathname,"r");
  if(fd == NULL)
  {
    printf("cat %s unsuccessful\n", pathname);
    return 1; 
  }
  else{
    while(fgets(buf, 512,fd) !=NULL)
    {
      puts(buf);
    }
  }
  return 0; 
}
int ls_file(char *fname)
{
  char linkname[MAX];
  struct stat fstat, *sp;
  int r, i;
  char ftime[64];
  sp = &fstat;
  if ((r = lstat(fname, &fstat)) < 0)
  {
    printf("can’t stat %s\n", fname);
    exit(1);
  }
  if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
  printf("%c",'-');
  if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
  printf("%c",'d');
  if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
  printf("%c",'l');
  for (i=8; i >= 0; i--){
  if (sp->st_mode & (1 << i)) // print r|w|x
  printf("%c", t1[i]);
  else
  printf("%c", t2[i]);
  // or print -
  }
  printf("%4ld ",sp->st_nlink); // link count
  printf("%4d ",sp->st_gid);
  // gid
  printf("%4d ",sp->st_uid);
  // uid
  printf("%8ld ",sp->st_size);
  // file size
  // print time
  strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
  ftime[strlen(ftime)-1] = 0;
  // kill \n at end
  printf("%s ",ftime);
  // print name
  printf("%s", basename(fname)); // print file basename
  // print -> linkname if symbolic file
  if ((sp->st_mode & 0xF000)== 0xA000){
  // use readlink() to read linkname
  readlink(fname, linkname, MAX);
  printf(" -> %s", linkname); // print linked name
  }
  //printf("\n");
  return 0;
}
int ls_dir(char *dname)
{
// use opendir(), readdir(); then call ls_file(name)
 
  struct dirent *ep;
  DIR *cdir; //current dir
  char path[MAX];
  char temp[MAX];
  cdir = opendir(dname);
  if(cdir==NULL)
  {
    printf("ERROR: Couldnt open path\n");
    strcpy(temp,"ERROR: Couldnt open path\n");
    n = write(sfd, temp, MAX);
    printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, temp);
    return 1;
  }
  ep=readdir(cdir);
  while(ep)
  {
    
      strcpy(path, dname);
      strcat(path, "/");
      strcat(path, ep->d_name);
      ls_file(path);
      printf("\n");
    
    ep=readdir(cdir);
  }
  
  closedir(cdir);
}
//NOTE - CODE FOR LS BASED ON TEXTBOOK SECTION 8.6.7 
int l_ls(char* pathname)
{
  
  if(strcmp(pathname, "") == 0)
  {
    ls_dir("./");
    return 1;
  }
  ls_dir(pathname);
  return 0; 
}
int l_pwd()
{
  char store_path[MAX];
  getcwd(store_path, MAX);
  printf("%s\n", store_path);
  
}
int client_init()
{
    int n;
    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET; 
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    saddr.sin_port = htons(PORT); 
  
    printf("3. connect to server\n");
    if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
}

int main(int argc, char *argv[], char *env[]) 
{ 
  
    int n; 
    client_init();
    printf("********  processing loop  *********\n");
     printf("********************** menu ***********************\n"); 
     printf( "* get  put  ls   cd   pwd   mkdir   rmdir   rm  *\n");  // executed by server (this file)
     printf(" * lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm  *\n");  // execut
     printf("***************************************************\n");
    while (1){
      printf("input a line : ");
      memset(arg, 0, sizeof(arg));
      bzero(line, MAX);                // zero out line[ ]
      fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

      line[strlen(line)-1] = 0;        // kill \n at end
      if (line[0]==0)                  // exit if NULL line
         exit(0);

      strcpy(linetok,line); 
      tokenize(linetok);
      int index=findCmd(arg[0]);
      if( index!= -1)
      {
        //Execute local commands.
        //"lcat","lls",  "lcd",  "lpwd",  "lmkdir",  "lrmdir", "lrm" 
          switch(index)
          {
                  case 0: l_cat(arg[1]);; break;
                  case 1: 
                  if(arg[1] != NULL)
                    l_ls(arg[1]);
                  else
                    l_ls("");
                  break;
                  case 2: l_cd(arg[1]); break;
                  case 3: l_pwd(); break;
                  case 4: l_mkdir(arg[1]); break;
                  case 5: l_rmdir(arg[1]); break;
                  case 6: l_rm(arg[1]); break;
          }
      
         continue;
      }
      else
      {
        //Execute commands on server. 
        n = write(sfd, line, MAX);

        if(strcmp(arg[0], "put") == 0)
        {
         
            //We will use the file size to coordinate
           
            struct stat fstat, *sp;
            sp = &fstat;
            char buffer[MAX];
            int k;
            
           
            if ((k = lstat(arg[1], &fstat)) < 0) {
                printf("can’t stat %s\n", arg[1]);
                write(sfd,"can't stat\n",MAX);
               
            }
            else
            {int size = sp->st_size;
            sprintf(buffer, "%d", size);
            write(sfd, buffer, MAX);
            int fp = open(arg[1], O_RDONLY); //opening file from client
            if (fp > 0) 
            {
                char temp[MAX];
                memset(temp, '\0', sizeof(temp));
                int n = read(fp, temp, MAX); 
                while(n > 0){
                    write(sfd, temp, n); //writing contents of file to the server to be copied
                    n = read(fp, temp, MAX);
                }
            }
            close(fp);
            
         
              }
           
        }

        else if(strcmp(arg[0],"get") == 0)
        {
            char buffer[MAX];
            int fd;
            read(sfd, buffer, MAX);
            int size = atoi(buffer);
            memset(buffer, 0, sizeof(buffer));
            fd = open(arg[1], O_WRONLY|O_CREAT, 0644); //creating file in client
            
            if(fd > 0)
            {
                while (size > 0) {
                    read(sfd, buffer, MAX); //reading the file contents that are to be copied in server
                   
                    if(size < MAX){
                        write(fd, buffer, size); //writing to file
                       
                        size -= size;
                    }
                    else{
                        write(fd, buffer, MAX); //writing to file
                        
                        size -= MAX;
                    }
                }
               
            
            }
          
           close(fd);
        }
        else{ //else statements are for server commands
        bzero(ans, sizeof(ans));
       
           n = read(sfd, ans, sizeof(ans));
           if(strcmp(ans,stream_end) == 0)
           {
             printf("%s",ans);
             continue;
           }
          while(strcmp(ans,stream_end) != 0)
          {
             printf("%s", ans);
              bzero(ans, sizeof(ans));
            n = read(sfd, ans, sizeof(ans));
          }
          continue;

       
      }
       char buffer[MAX];
        read(sfd, buffer, MAX);
        
      }
}

}

/********************* YOU DO ***********************
    1. The assignment is the Project in 13.17.1 of Chapter 13

    2. Implement 2 sets of commands:

      ********************** menu ***********************
      * get  put  ls   cd   pwd   mkdir   rmdir   rm  *  // executed by server
      * lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm  *  // executed LOACLLY (this file)
      ***************************************************

    3. EXTRA Credits: make the server MULTI-threaded by processes

    Note: The client and server are in different folders on purpose.
          Get and put should work when cwd of client and cwd of server are different.
****************************************************/