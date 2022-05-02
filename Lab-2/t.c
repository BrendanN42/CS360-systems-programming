#include <stdio.h>             // for I/O
#include <stdlib.h>            // for I/O
#include <libgen.h>            // for dirname()/basename()
#include <string.h>

typedef struct node{
         char  name[64];       // node's name string
         char  type;           // 'D' for DIR; 'F' for file
   struct node *child, *sibling, *parent;
}NODE;

// cwd is current working directory
NODE *root, *cwd, *start;
char line[128];
char dname[64], bname[64];
char command[16], pathname[64];
char gpath[128];       // holder of token strings
char *name[32];        // token string pointers
int  n;                // number of token strings

//               0       1      2    
char *cmd[] = {"menu","mkdir", "ls", "cd","rmdir","rm","creat","pwd","quit", 0};
NODE *search_child(NODE *parent, char *name)
{
  NODE *p;
  printf("search parent DIR %s for %s\n", parent->name, name);

  if((strcmp(parent->name,name)==0 &&strcmp(parent->name,"/")==0 || strcmp(parent->name,name)==0)) 
  {
    return parent;
  }
  
  p = parent->child;
  if (p==0)
    return 0;
  while(p){
    if (strcmp(p->name, name)==0)
      return p;
    p = p->sibling;
  }
  return 0;
}
int tokenize(char *pathname)
{
  n=0;
  if(pathname[0]=='/')
  {
        name[0]="/";
        n++;
  }
  char *token;
  token = strtok(pathname, "/"); // first call to strtok() 
  while(token){
      name[n]=token;
      printf("name[%d]: %s\n",n, token);
      strcat(gpath,token);
      n++;

      token = strtok(0, "/"); // call strtok() until it returns NULL }

      if(token!=NULL)
        {
            strcat(gpath,"0");
        }
  }
  return 0;
}

NODE *path2node(char*pathname)
{
    
    char c[2]="/";
    strcpy(gpath, "");

  
    //absolute is when pathname[0]='/'
    if( pathname[0]=='/') //absolute path
    {
        start = root;
    }
    else //relative path
    {
        start = cwd;
    }
    
    //tokenize the path name
    tokenize(pathname);
  
    NODE *p=start;
    for (int i=0; i < n; i++)
    {
        p = search_child(p, name[i]);

        if (p==0)
        { 
          printf("ERROR: No node found\n");
          return 0;
        }

    }

       return p;
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



int insert_child(NODE *parent, NODE *q)
{
  NODE *p;
  printf("insert NODE %s into END of parent child list\n", q->name);
  p = parent->child;
  q->parent = parent;
  q->child = 0;
  q->sibling = 0;
 
  if (p==0)
    parent->child = q;
  else{
    while(p->sibling)
    { 
        p = p->sibling;
    }
    p->sibling = q;
  }
  
}
int dbname(char *pathname)
{

    char temp[128]; // dirname(), basename() destroy original pathname strcpy(temp, pathname);
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));

}

void createNode(char *pathname, char type)
{
  NODE *q = (NODE *)malloc(sizeof(NODE));
    q->type=type;
    strcpy(q->name, name[n-1]);
    
    NODE *k;
    if( pathname[0]=='/') //absolute path
    {
        k = root;
        
    }
    else //relative path
    {
        k = cwd;
    }
       for (int i=0; i < n-1; i++){ //for loop is finding the parent node of the file that wants to be created starting at the root
      k = search_child(k, name[i]);
      }
    insert_child(k, q);

}
int mkdir(char *pathname)
{
  
    NODE *p=path2node(pathname);
    
    printf("check whether %s already exists\n", name[n-1]);
    if(p!=0)
    {
        printf("name %s already exists, mkdir FAILED\n", name[n-1]);
        return -1;
    }
    
    printf("--------------------------------------\n");
    printf("ready to mkdir %s\n", name[n-1]);

    createNode(pathname, 'D');

    printf("mkdir %s OK\n", name[n-1]);
    printf("--------------------------------------\n");
    
    return 0;

}
int creat(char *pathname)
{
  NODE *p=path2node(pathname);
    
     printf("check whether %s already exists\n", name[n-1]);
    if(p!=0)
    {
        printf("name %s already exists, creat FAILED\n", name[n-1]);
        return -1;
    }
    
    printf("--------------------------------------\n");
    printf("ready to creat %s\n", name[n-1]);
    createNode(pathname,'F');
  
    printf("creat %s OK\n", name[n-1]);
    printf("--------------------------------------\n");
    
    return 0;

}



int ls(char *pathname)
{
    NODE *p=path2node(pathname)->child;
  while(p){
    printf("[%c %s] ", p->type, p->name);
    p = p->sibling;
  }
  printf("\n");
  return 0;

}

int cd(char *pathname)
{

  NODE *p=path2node(pathname);

  if(strcmp(name[0],"..")==0)
  {
    for(int i=0;i<n;i++)
    {
      cwd=search_child(cwd->parent,cwd->parent->name);
    }
    printf("cd SUCCESS: Current Working Directory is: %s\n", cwd->name);
  }
  
  else{
  
    if(p==0)
    {
        printf("ERROR: Path does not exist. CD failed\n");
    }
    else if(p->type=='D')
    {
        cwd=p;
        printf("cd SUCCESS: Current Working Directory is: %s\n", cwd->name);
    }
    else
    {
      printf("ERROR: Path is not a directory\n");
    }
      
  }
    return 0;

}

void delete(NODE *parent, NODE *p)
{
      NODE *temp;
      if(parent->child==p)
        {
           if(p->sibling == NULL)
           {
            parent->child = NULL;
           }
        else
            {
            parent->child = p->sibling;
            }
        }
        else  
        {
        temp = parent->child;
        while(temp->sibling != p)
        {
            temp = temp->sibling;
        }
        temp->sibling = p->sibling;
        
         }
       
        printf("Delete %s succesful.\n", p->name);
        free(p);
}
int rmdir(char *pathname)
{
    NODE *p=path2node(pathname);
    NODE *parent, *temp;

    if(p==0)
    {
        printf("ERROR: Path does not exist. rmdir failed\n");
    }
    else if(p->type=='D') //checking to make sure node type is a directory
    {
      
      if(strcmp(pathname, "/")==0) //check to see if user is trying to delete the root
      {
        printf("ERROR: cannot delete the root direcotry / \n");
        return;
      }
      parent=p->parent;
      
      if(p->child==NULL)
      {
       
        delete(parent, p);
      }
      else
        printf("ERROR: Dir is not empty. rmdir failed\n");
    }
    else
      printf("ERROR: Path is not a directory. rmdir failed\n");

  return 1;
}

int rm(char *pathname)
{
  NODE *p=path2node(pathname);
  NODE *parent,*temp;
  
    if(p==0)
    {
        printf("ERROR: Path does not exist. rm failed\n");
    }
    else if(p->type=='F') //checking to see if the node type is a file
    {

      parent=p->parent;

      delete(parent,p);

    }
   else
      printf("ERROR: Path is not a file. rm failed\n");
  return 0;
}
void recursion_pwd(NODE *p)
{
  if(strcmp(p->parent->name,"/")!=0) //finding the start of the path
  {
    recursion_pwd(p->parent); //using recursion to keep going to the parent in the path till "/" is found
  }
  printf("%s/",p->name); 
  
}
int pwd()
{
  
  start=cwd;

  if(strcmp(start->parent->name,start->name)!=0)
  {
   
    printf("/");
    recursion_pwd(start);
    printf("\n");
    return 1;
    

  }
  //else its already at the root
  printf("%s\n",start->name);

  return 1;

}

int quit()
{
  printf("Program exit\n");
  exit(0);
}

int initialize()  // create the root DIR / and set CWD
{
    root = (NODE *)malloc(sizeof(NODE));
    strcpy(root->name, "/");
    root->parent = root;
    root->sibling = 0;
    root->child = 0;
    root->type = 'D';
    cwd = root;
    printf("Root initialized OK\n");
}

int main()
{
  int index;

  initialize();
  char *m;
  printf("Enter menu for help menu\n");
  
  while(1){
    //printf("CWD: %s\n",cwd->name);
      printf("Enter command line [menu|mkdir|ls|cd|rmdir|rm|creat|pwd|quit] : ");
      fgets(line, 128, stdin);
      line[strlen(line)-1] = 0;

      command[0] = pathname[0] = 0;
      sscanf(line, "%s %s", command, pathname);
    printf("command=%s pathname=%s\n", command, pathname);
      
      if (command[0]==0) 
         continue;

      index = findCmd(command);
      if(index==-1)
      {
        printf("Invalid Command\n");
        
      }
      
  else
      {switch (index){
        case 0: 
        printf("====================== MENU =======================\n");
        printf("[mkdir|ls|cd|rmdir|rm|creat|pwd|quit]\n");
        printf("===================================================\n");
        break;
        case 1: mkdir(pathname); break;
        case 2: ls(pathname);            break;
        case 3: cd(pathname);          break;
        case 4: rmdir(pathname);          break;
        case 5: rm(pathname);          break;
        case 6: creat(pathname);          break;
        case 7: pwd();          break;
        case 8: quit();          break;
      }
      }
  }
}

