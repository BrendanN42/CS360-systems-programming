#include "type.h"


extern char line[128], cmd[32], pathname[128];
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
//iblk corresponds to inode_start 
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];


int read_file()
{
    //Preparations: 
    //ASSUME: file is opened for RD or RW;
    //ask for a fd  and  nbytes to read;
    
    int fd = 0, nbytes = 0;
    printf("Enter a file descriptor");
    scanf("%d", &fd);
    printf("Enter  number of bytes to read\n");
    scanf("%d",&nbytes);
    OFT *oftp = running->fd[fd];
    //verify that fd is indeed opened for RD or RW;
    if (oftp->mode != 0 && oftp->mode != 2)
    {
        printf("fd is not open for RD or RW\n");
        return -1;
    }
    char buf[BLKSIZE];
    myread(fd, buf, nbytes);
    printf("%s", buf);
    printf("\n");
}

int myread(int fd, char *buf, int nbytes)
{
    //Accessing all the variables
    
    
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    INODE *ip = &mip->INODE;
    //1. initialize all the necessary variables
    int count = 0;
    int avil = mip->INODE.i_size - oftp->offset;
    int ibuf[BLKSIZE] = { 0 };
    int dbuf[BLKSIZE] = { 0 };
    char *cq = buf;
    int lbk, startByte, blk;
    if (nbytes > avil)
        nbytes = avil;
    
    //2. Loop to run while there is still stuff to read and nbytes still left. 
    while (nbytes && avil) 
    {
        
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        if (lbk < 12) 
        {                             
            // direct blocks
            blk = ip->i_block[lbk];
        } 
        else if (lbk >= 12 && lbk < 256 + 12) 
        {   // indirect blocks
            // Read INODE.i_block[12] into int ibuf[256]
            get_block(mip->dev, ip->i_block[12], ibuf);
            blk = ibuf[lbk - 12]; // offset of 12
        } 
        else 
        {                                    
            // doubly indirect blocks
            // read block INODE.i_block[13] into int ibuf[256]
            get_block(mip->dev, ip->i_block[13], (char *)ibuf);
            //mailman's algorithm 
            lbk = lbk - (BLKSIZE/ sizeof(int)) - 12; 
            blk = ibuf[lbk / (BLKSIZE/ sizeof(int))]; 
            get_block(mip->dev, blk, dbuf);
            blk = dbuf[lbk % (BLKSIZE/ sizeof(int))];
        }

        /* get the data block into readbuf[BLKSIZE] */
        char readbuf[BLKSIZE];
        get_block(mip->dev, blk, readbuf);

        /* copy from startByte to buf[ ], at most remain bytes in this block */
        char *cp = readbuf + startByte;
        int remainder = BLKSIZE - startByte; // number of bytes remain in readbuf[]

        // Instead of copying byte by byte le us use nbytes and remainder to efficiently read 
        if (nbytes > remainder) 
        {
            //If space left is less than total bytes to be read, read in as much as possible i.e size of remainder 
            strncpy(cq, cp, remainder);
            //Adjust remainder and avil 
            count += remainder;
            oftp->offset += remainder;
            avil -= remainder;
            nbytes -= remainder; 
            remainder = 0;
        } 
        else 
        {
            //vice a versa for nbytes < remainder
            strncpy(cq, cp, nbytes);
            //Adjust remainder and avil
            count += nbytes;
            oftp->offset += nbytes;
            avil -= nbytes; 
            remainder -= nbytes;
            nbytes = 0;
        }
    }
    return count;
}


int mycat(char *filename)
{
    char mybuf[BLKSIZE], dummy = 0;// a null char at end of mybuf[]
    int n; 

    //1. open filename for READ;
    int fd = open_file(filename, 0);
    //2. 
    while (n = myread(fd, mybuf, BLKSIZE)) {
        mybuf[n] = 0; //as a null char at end of mybuf
        // printf("%s", mybuf);   <=== THIS works but not good
        //spit out chars from mybuf[ ] but handle \n properly
        char *letter = mybuf;
        while (*letter != '\0')
        {
            //Handling \n
            if (*letter == '\n') 
            {
                printf("\n");
            } 
            else if (*letter == '\t')
            {
                printf("\t");
            }
            else 
            {
                // if not \n or \t just print the character as is 
                printf("%c", *letter);
            }
            letter++;
        }
    }
    close_file(fd);
    return 0; 
}