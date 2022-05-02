#include <stdio.h>
typedef unsigned int u32;
int *ip;
char *ctable = "0123456789ABCDEF";
int  BASE = 10; 


int rpu_base(u32 x, int base)
{
 char c;
    if (x){
       c = ctable[x % base];
       rpu_base(x / base, base);
       putchar(c);
    }
}
int printu(u32 x) //prints unsigned integers
{
    (x==0)? putchar('0') : rpu_base(x,10);
    putchar(' ');
}

int printx(u32 x) //prints unsigned integers in HEX
{
    (x==0)? putchar('0') : rpu_base(x, 16);
    putchar(' ');
}

int  printd(int x){  //prints any integers (x may be negative)

    if (x==0)
    {
        putchar('0');
        return;
    }
    if(x<0)//check if x is a negative number
    {
        putchar('-');
        printu(-1*x);
    }
    else{
        printu(x);
    }
        


 }
int prints(char *s)
{
    int i=0;
    while(s[i]!='\0')
    {
        putchar(s[i]);
        i++;
    }
}

int myprintf(char *fmt, ...)      // C compiler requires the 3 DOTs
{
    char *cp=fmt;
    int * ip = (int *)&fmt; 
	ip++;
    
    int i=0;
    while(cp[i]!='\0')
    {
        switch(cp[i])
        {
            case '\n':
                putchar(cp[i]);
                putchar('\r');
            break;

            case '%':
              i++;  
                switch(cp[i])
                {
                    case 'c':
                    
                        putchar(*ip);
                    break;

                    case 's':
                        prints(*ip);
                    break;

                    case 'u':
                        printu(*ip);
                    break;

                    case 'x':
                        printx(*ip);
                    break;

                    case 'd':
                        printd(*ip);
                    break;


                }
                
                ip++;
            break;
        default:
            putchar(cp[i]);
        break;
        }
        
        i++;
    }

}
int main(int argc, char *argv[ ], char *env[ ])
{
  myprintf("************** int main ******************\n\n");
  myprintf("argc=%d\n\n", argc);
      int i=0;
    while(i<argc)
    {
        myprintf("argv[%d] = %s\n", i, argv[i]);
        i++;
    }
    printf("\n");
    i=0;
     while(env[i]){
    myprintf("env[%d] = %s\n", i, env[i]);
    i++;
  }
    printf("\n\n");
      myprintf("char= %c string= %s       dec= %u  hex= %x neg= %d\n",  'A', "this is a test",  100,   100,   -100);

    return 0;
}