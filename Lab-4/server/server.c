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
#include <libgen.h>
#include <time.h>

#define MAX 256
#define PORT 1234

int n;
int sfd, cfd;
char ans[MAX];
char* arg[64];
char line[MAX];
char*cmd[]={"get","put","ls",  "cd",  "pwd",  "mkdir",  "rmdir", "rm"};
char stream_end[] = "ENDOFSTREAM\n";  

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

int s_mkdir(char *pathname)
{
    int r=mkdir(pathname,0755);
    return r;
}
int s_rmdir(char *pathname)
{
    int r=rmdir(pathname);
    return r;
}
int s_rm(char *pathname)
{
    int r = unlink(pathname);
    return r;
}
int s_cd(char *pathname)
{
  int r = chdir(pathname);
  return r;
}
int s_get(char *name)
{
   
   
    struct stat fstat, *sp;
    sp = &fstat;
    char buffer[MAX];
    int k;
    if ((k = lstat(name, &fstat)) < 0) {
        printf("can’t stat %s\n", name);
        return -1;
    }
    //First write file size to client as we are using that to terminate loop 
    int size = sp->st_size;
    sprintf(buffer, "%d", size);
    write(cfd, buffer, MAX);
    int fp = open(name, O_RDONLY); //opening file in server
    if (fp > 0) {
        char buf[MAX];
        memset(buf, '\0', sizeof(buf));
        int n = read(fp, buf, MAX); // read 256 bytes from the file
        while(n > 0){
            write(cfd, buf, MAX); //writing contents of file to client
            n = read(fp, buf, MAX);
        }
    }
    close(fp);
    return 0;
}
int s_put(char *name)
{
  
    char buffer[MAX];
    int fd;
    int b = read(cfd, buffer, MAX);
    if(strcmp(buffer,"can't stat\n")==0)
    {
      printf("Can't stat\n");
      return -1;
    }
    int size = atoi(buffer);
    memset(buffer, 0, sizeof(buffer));
    fd = open(name, O_WRONLY|O_CREAT, 0644); //creating file in server
    if (fd > 0) {
       
        while (size > 0) 
        {
            read(cfd, buffer, MAX); //reading the file contents that are to be copied in client
            
            if(size < MAX){
                write(fd, buffer, size); //writing to the file
                size -= size;
            }
            else{
                write(fd, buffer, MAX); //writing to the file
                size -= MAX;
            }
        }
        close(fd);
    }
  
}
int s_ls(char *file)
{
  
  char temp[MAX];
    getcwd(temp, MAX-1);
   
    if (!strcmp(file, ""))
    {
      
        ls_dir(temp);
        return 1;
    }
   
    ls_dir(file);
    return 0;
}
int ls_dir(char *dname)
{
  struct dirent *ep;
  DIR *cdir; //current dir
  char path[MAX];
  char temp[MAX];
  cdir = opendir(dname);
  if(cdir==NULL)
  {
    printf("ERROR: Couldnt open path\n");
    strcpy(temp,"ERROR: Couldnt open path\n");
    n = write(cfd, temp, MAX);
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
      write(cfd, "\n", MAX);
      ep=readdir(cdir);
  }
  closedir(cdir);

}
int ls_file(char *fname)
{
  struct stat fstat, *sp;
  char temp[MAX];
  char final_output[MAX];
  memset(final_output, 0, MAX);
  char linkname[MAX];
  int r,i;
  char ftime[64];
  char *t1 = "xwrxwrxwr-------"; 
  char *t2 = "----------------";
  sp=&fstat;
  
  if((r=lstat(fname,&fstat))<0)
  {
    printf("can't stat %s\n",fname);
    sprintf(temp,"can't stat %s\n",fname);
    write(cfd,temp,MAX);
    exit(1);

  }
  if ((sp->st_mode & 0xF000) == 0x8000)// if (S_ISREG())
  {
    printf("%c",'-');
    sprintf(temp,"%c",'-' );
    strcat(final_output,temp);
  } 
 
if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
{
  printf("%c",'d'); 
  sprintf(temp,"%c",'d' );
  strcat(final_output,temp);
  
}

if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
{
  printf("%c",'l');
  sprintf(temp,"%c",'l');
  strcat(final_output,temp);
}
 
for(i=8;i>=0;i--)
{
  
if (sp->st_mode & (1 << i)) // print r|w|x 
{
  printf("%c", t1[i]);
  sprintf(temp,"%c", t1[i]);
  strcat(final_output,temp);
}
else
{
  printf("%c", t2[i]); 
  sprintf(temp,"%c", t2[i]);
  strcat(final_output,temp);
}

}

printf("%4d ",sp->st_nlink); // link count
sprintf(temp,"%4d ",sp->st_nlink );
strcat(final_output,temp);



printf("%4d ",sp->st_gid);// gid
sprintf(temp,"%4d ",sp->st_gid);
strcat(final_output,temp);

printf("%4d ",sp->st_uid);// uid
sprintf(temp, "%4d ",sp->st_uid);
strcat(final_output,temp);


printf("%8d ",sp->st_size);// file size
sprintf(temp,"%8d ",sp->st_size);
strcat(final_output,temp);


strcpy(ftime,ctime(&sp->st_ctime)); //print time in calender form
ftime[strlen(ftime)-1]=0;
sprintf(temp, "%s ", ftime);                // prints the time
strcat(final_output, temp);

printf("%s",basename(fname));//print file basename
sprintf(temp,"%s",basename(fname));
strcat(final_output,temp);

//print -> linkname if simbolic file
if((sp->st_mode & 0xF000) == 0xA000)
{
  //use readlink() to read linkname
  readlink(fname,linkname,MAX);
  printf(" -> %s\n",linkname);
  sprintf(temp," -> %s\n",linkname);
  strcat(final_output,temp);
  
}

  write(cfd,final_output,MAX);

}
int s_pwd()
{
   char cwd[MAX];
   getcwd(cwd,MAX-1);
   strcat(cwd,"\n");
   printf("CWD SERVER: %s\n",cwd);
   write(cfd,cwd,MAX);
}
int main() 
{ 
    char buff[MAX];
    getcwd(buff,MAX);
    chroot(buff);
    int len, temp_debuggy; 
    struct sockaddr_in saddr, caddr; 
    int i, length,index;
    
    
    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET; 
    //saddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    saddr.sin_port = htons(PORT);
    
    printf("3. bind socket to server\n");
    if ((bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) != 0) { 
        printf("socket bind failed\n"); 
        exit(0); 
    }
      
    // Now server is ready to listen and verification 
    if ((listen(sfd, 5)) != 0) { 
        printf("Listen failed\n"); 
        exit(0); 
    }
    while(1){
        // Try to accept a client connection as descriptor newsock
        length = sizeof(caddr);
        cfd = accept(sfd, (struct sockaddr *)&caddr, &length);
        if (cfd < 0){
            printf("server: accept error\n");
            exit(1);
        }

        printf("server: accepted a client connection from\n");
        printf("-----------------------------------------------\n");
        printf("    IP=%s  port=%d\n", "127.0.0.1", ntohs(caddr.sin_port));
        printf("-----------------------------------------------\n");
       
        // Processing loop
        while(1){
         
          memset(arg, 0, sizeof(arg));
          printf("server ready for next request ....\n");
          n = read(cfd, line, MAX);
          if (n==0){
            printf("server: client died, server loops\n");
            close(cfd);
            break;
          }

          // show the line string
          printf("server: read  n=%d bytes; line=[%s]\n", n, line);
          char *linetok[MAX];
         strcpy(linetok,line); 
          tokenize(linetok);
          printf("COMMAND: %s, ARG: %s\n",arg[0],arg[1]);
           
          index=findCmd(arg[0]);
          int r;
          if(index==-1)
            printf("ERROR: INVALID COMMAND\n");
            else{
              
                switch(index)
                {
                  //{"get","put","ls",  "cd",  "pwd",  "mkdir",  "rmdir", "rm"};
                  
                  case 0: r=s_get(arg[1]); break;
                  case 1: r=s_put(arg[1]);break;
                  case 2: 
                  if(arg[1] != NULL)
                    s_ls(arg[1]);
                  else
                    s_ls("");
                    break;
                  case 3: r=s_cd(arg[1]); break;
                  case 4: r=s_pwd(); break;
                  case 5: r=s_mkdir(arg[1]); break;
                  case 6: r=s_rmdir(arg[1]); break;
                  case 7: r=s_rm(arg[1]); break;

                }


            }
          strcat(line, " ECHO");

          // send the echo line to client 
          n= write(cfd, stream_end, MAX);
          printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
          printf("server: ready for next request\n");
        }
    }
}



/********************* YOU DO ***********************
    1. The assignment is the Project in 13.17.1 of Chapter 13

    2. Implement 2 sets of commands:

      ********************** menu ***********************
      * get  put  ls   cd   pwd   mkdir   rmdir   rm  *  // executed by server (this file)
      * lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm  *  // executed LOACLLY
      ***************************************************

    3. EXTRA Credits: make the server MULTI-threaded by processes
    
    Note: The client and server are in different folders on purpose.
          Get and put should work when cwd of client and cwd of server are different.
****************************************************/