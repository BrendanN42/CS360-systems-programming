/***** LAB3 base code *****/ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>


// ** TA Added includes to get source code to compile:
#include <unistd.h>



char gpath[128];    // hold token strings 
char *arg[64];      // token string pointers
int  n;             // number of token strings

char dpath[128];    // hold dir strings in PATH
char *dir[64];      // dir string pointers
int  ndir;          // number of dirs   

char *head;
char *tail;
int pd2[2];

int tokenize(char *pathname) // YOU have done this in LAB2
{                            // YOU better know how to apply it from now on
  char *s;
  strcpy(gpath, pathname);   // copy into global gpath[]
  s = strtok(gpath, " ");    
  n = 0;
  while(s){
    arg[n++] = s;           // token string pointers   
    s = strtok(0, " ");
  }
  arg[n] =0;                // arg[n] = NULL pointer 
  
  return 0;
}

int check_pipe(char *pathname)
{
  int k=0;
  char path[64];
  strcpy(path, pathname);
  int size=strlen(path);
  int j=0;
  char temp[64]="";

   for(int i = size-1; i>0 ; i--){

    if(path[i] == '|' ){
     
      j=i+1;
      
      while(j<size)
      { 
        temp[k]=path[j];
        k++;
        j++;
      }
      
      tail=temp;
      path[i] = 0;
      head = path;
      printf("HEAD: %s, TAIL: %s\n", head, tail);
      return 1;
    }
  }
  head = path;
  tail = NULL;
  printf("NO PIPE: HEAD: %s\n", head);
  return 0;

  
}
void redirect()
{

  for(int i=0;i<n;i++)
  {
    int fd;
    if(strcmp(arg[i],">")==0)//OUTFILE, sends output
    {
      arg[i]=0;
      close(1);
      fd = open(arg[i + 1], O_WRONLY | O_CREAT, 0644);
      dup2(fd,1);

    }
    else if(strcmp(arg[i],"<")==0) //INFILE, takes input
    {
      arg[i]=0;
      close(0);
      fd = open(arg[i + 1], O_RDONLY);
      dup2(fd,0);
    }
    else if(strcmp(arg[i],">>")==0)//OUTFILE, appends output
    {
      arg[i]=0;
      close(1);
      fd = open(arg[i + 1], O_WRONLY | O_APPEND, 0644);
      dup2(fd,1);
    }
  }

}

void exec(char *command,char *env[ ])
{
  char t1[128],t2[128];
  int r;
  
  printf("COMMAND: %s\n",command);
  tokenize(command);
  redirect();
  for (int i=0; i<n; i++){  // show token strings   
      printf("arg[%d] = %s\n", i, arg[i]);
  }
  printf("DO COMMAND: %s\n", command);
  if(arg[0][0]=='.' && arg[0][1]=='/')
  {
    getcwd(t1,128);
    strcat(t1,"/");
    strcat(t1, arg[0]);
    execve(t1,arg,env);
  }
  for(int i=0;i<ndir;i++)
  {
    strcpy(t2,dir[i]);
    strcat(t2,"/");
    strcat(t2,arg[0]);
    
    printf("i=%d  cmd = %s\n",i,t2);
    
    r = execve(t2, arg, env);
  }
  printf("execve failed r = %d\n", r);
  exit(1);

}
char* cPipe(char *line, int *pd, char * env[], char tail1[64], char head1[64])
{

 
  if(pd) //parent as pipe writer
  {
   //printf("****** WRITER *******\n");
    close(pd[0]);
    close(1);
    dup(pd[1]);
    close(pd[1]);
  
  }
  int check = check_pipe(line);
  
  if(check==1)
  {
    strcpy(head1,head);
    strcpy(tail1,tail);
  
    pipe(pd2);
    int pid=fork();
    if(pid)
    {
        // printf("****** READER *******\n");
      close(pd2[1]);
      close(0);
      dup(pd2[0]);
      close(pd2[0]);
      exec(tail1,env);
    }
    else{
      cPipe(head1,pd2,env,tail1,head1); //using recursion to satisfy 1 or more pipes
    }

  }
  else{
    exec(line,env);
  }

}

int main(int argc, char *argv[ ], char *env[ ])
{
  int i,r;
  int pid, status;
  char *cmd;
  char head1[64], tail1[64];
  char line[28];
  ndir=0;
  i=0;
  char *home = getenv("HOME");
  
  printf("************* Welcome to snsh **************\n");
  printf("1. show HOME directory: HOME = %s\n",home);

   char*path=getenv("PATH");
   printf("2. show PATH:\n%s\n", path);
   strcpy(dpath,path);

    ndir = 0;
      printf("3.decompose PATH into dir strings: \n");
      char *token = strtok(dpath, ":");

      while (token)
      {
        dir[ndir++] = token;
        token = strtok(NULL, ":");
      }
  
  //show dir
  i=0;
  while(i<ndir)
  {
    printf("dir[%d] = %s\n", i, dir[i]);
    i++;
  }
  
  printf("4: *********** snsh processing loop **********\n");
  
  while(1){
    printf("sh %d running\n", getpid());
    printf("snsh%% : ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0; 
    if (line[0]==0)
      continue;
    
    tokenize(line);

    
    for (i=0; i<n; i++){  // show token strings   
        printf("arg[%d] = %s\n", i, arg[i]);
    }
    
    cmd = arg[0];         // line = arg0 arg1 arg2 ... 

    if (strcmp(cmd, "cd")==0){
      if(arg[1]==NULL)
      {
        printf("HOME: %s\n",home);
        chdir(home);
      }
      else{
        chdir(arg[1]);
      }
      
      continue;
    }
    
    if (strcmp(cmd, "exit")==0)
      exit(0); 
    
     pid = fork();
     
     if (pid){
       printf("sh %d forked a child sh %d\n", getpid(), pid);
       printf("sh %d wait for child sh %d to terminate\n", getpid(), pid);
       pid = wait(&status);
       printf("ZOMBIE child=%d dead exitStatus=%x\n", pid, status); 
       printf("main sh %d repeat loop\n", getpid());
     }
     else{
       printf("child sh %d running\n", getpid());
       int check= check_pipe(line);
      
       if(check==1) //if command consist of a pipe connect the pipes and execute commands
       {
        strcpy(head1,head);
        strcpy(tail1,tail);
      
        cPipe(line,0,env,tail1,head1);
       }
       else{

         exec(line,env);
         
       }
      
     }
  }
}





/********************* YOU DO ***********************
1. I/O redirections:

Example: line = arg0 arg1 ... > argn-1

  check each arg[i]:
  if arg[i] = ">" {
     arg[i] = 0; // null terminated arg[ ] array 
     // do output redirection to arg[i+1] as in Page 131 of BOOK
  }
  Then execve() to change image


2. Pipes:

Single pipe   : cmd1 | cmd2 :  Chapter 3.10.3, 3.11.2

Multiple pipes: Chapter 3.11.2
****************************************************/

    
