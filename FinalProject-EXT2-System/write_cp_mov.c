#include "read_cat.c"
#include "link_unlink.c"
#include "open_close_lseek.c"

extern char line[128], cmd[32], pathname[128];
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int write_file()
{
    int fd = 0, nbytes = 0;
    char wbuf[BLKSIZE] = {0};
    
    printf("Enter a file descriptor and data to be written to the file : ");
    scanf("%d %s", &fd, &wbuf);

    OFT *oftp = running->fd[fd];
    // verify fd is open for WR or RW or Append 
    if (oftp->mode != 1 && oftp->mode != 2 && oftp->mode != 3) {
        printf("File not in wr or rw or append mode \n");
        return -1;
    }
    nbytes = sizeof(wbuf);
    return mywrite(fd, wbuf, nbytes);

}


int mywrite(int fd, char *buf, int nbytes)
{
        OFT *oftp = running->fd[fd];
        MINODE *mip = oftp->minodePtr;
        INODE *ip = &mip->INODE;
        int lbk, start, blk, dblk, remain;
        int ibuf[256], dbuf[256], dbuf2[265], dbuf3[256];
        char writebuf[BLKSIZE], *cp, *cq = buf;
        int doubleflag = 0;
        int count = 0;
       
        while(nbytes>0)
        {
            doubleflag = 0; 
             
            //compute LOGICAL BLOCK (lbk) and the startByte in that lbk:
            lbk = oftp->offset / BLKSIZE;
            start = oftp->offset % BLKSIZE;
           
            if(lbk < 12 ) 
            {
                
                if(ip->i_block[lbk]==0) // if no data block yet
                {
                    ip->i_block[lbk] = balloc(mip->dev); // must allocate a block 
                }
                 
                blk = ip->i_block[lbk];
               
            }
            //indirect blocks - refer to write.html 
            else if(lbk >= 12 && lbk < 256 + 12) 
            {
                 
            if(ip->i_block[12]==0) // if no data block yet
            {
             
                ip->i_block[12] = balloc(mip->dev);
                get_block(mip->dev, ip->i_block[12], (char *)ibuf);
                memset(ibuf,0,256);
                put_block(mip->dev, ip->i_block[12], (char *)ibuf);
                

            }
            //overwrtiing ibuf?
             
            get_block(mip->dev, ip->i_block[12], (char *)ibuf);

            blk = ibuf[lbk - 12];

            if (blk==0){
                 
                ibuf[lbk-12]=balloc(mip->dev);
                get_block(mip->dev, ibuf[lbk-12], (char *)ibuf);
                memset(ibuf,0,256);
                 put_block(mip->dev, ibuf[lbk-12], (char *)ibuf);
                  put_block(mip->dev, ip->i_block[12], (char *)ibuf);
                 }
            }
            //double indirect blocks
            else 
            {
                doubleflag=1;
                 
                if(ip->i_block[13]==0) // if no data block yet
                {
                     
                    ip->i_block[13] = balloc(mip->dev); // must allocate a block
                    
                    get_block(mip->dev, ip->i_block[13], (char *)dbuf);
                    
                    memset(dbuf, 0, 256);
                    

                    put_block(mip->dev, ip->i_block[13], (char *)dbuf);
                }
                 
                get_block(mip->dev, ip->i_block[13], (char *)dbuf);
                // printf("--------------------IN WRITE DD\n");
                // printf("%s\n", (char *)dbuf);
                //blk = ip->i_block[lbk];
                //12 for the first 12 i_block numbers and 256 for the 256 entries in i_block[13]
                lbk -= 268; // 256 + 12 

                //get the correct blk number from dbuf 
                dblk = dbuf[lbk / 256];
                if(dblk == 0)
                { 
                    dblk = balloc(mip->dev);
                    get_block(mip->dev, dblk, (char *)dbuf2);
                    memset(dbuf2, 0, 256);
                    put_block(mip->dev, dblk, (char *)dbuf2);
                }
                
                
                get_block(mip->dev, dblk, (char *)dbuf2);
                blk = dbuf2[lbk % 256];
                if(blk==0){
                    blk=balloc(mip->dev);
                    get_block(mip->dev,blk,(char *)dbuf3);
                    memset(dbuf3, 0, 256);
                    put_block(mip->dev, blk, (char *)dbuf3);

                }
            }
                        /* all cases come to here : write to the data block */
            //read disk block into wbuf[ ]  
           
            get_block(mip->dev, blk, writebuf);
            //cp points at startByte in wbuf[]
            cp = writebuf + start;
            //number of BYTEs remain in this block
            remain = BLKSIZE - start;
            //Optimization of code 
            //The idea is to read in chunks instead of a byte at a time 
            if(remain < nbytes)
            {
                //cp remain amount of bytes from buf into cp 
                
                //printf("%s\n", );
                strncpy(cq, cp, remain);
                //adjust nbytes and remain 
                count += remain;
                nbytes -= remain;
                oftp->offset += remain;
                
                ip->i_size += remain;
                remain -= remain;
            }
            else
            {              
                //cp remain amount of bytes from buf into cp
                strncpy(cp, cq, nbytes);
                //adjust nbytes and remain 
                count += nbytes;
                remain -= nbytes;
                running->fd[fd]->offset += nbytes;
                
                ip->i_size += nbytes;
                
                nbytes -= nbytes;
            } 


            put_block(mip->dev, blk, writebuf);
            
              
        }
    mip->dirty = 1; 
    return count;    
}

int my_cp(char *src, char *dest)
{
    int n = 0;
    char buf[BLKSIZE] = {0};
    //open src and dst 
    int fd_src = open_file(src, 0); // src for read (0)
    int fd_dest = open_file(dest, 2); // dst for read-write 

    if (fd_src == -1 || fd_dest == -1) 
    {
		printf("Error in opening files");
        return -1;
	}
    // pfd();
    while ( n = myread(fd_src, buf, BLKSIZE))
    {
        //  pfd();
        //read source into buf 
        buf[n] = 0;
        //write from buf to dst 
        mywrite(fd_dest, buf, n);
        //reset buf for next iteration 
        memset(buf, '\0', BLKSIZE);
    }
   // pfd();
    close_file(fd_src);
    close_file(fd_dest);
    return 0;
}
int my_mv(char* src, char* dest)
{
    int fd_src = open_file(src, 0);
    printf("fd src : %d \n", fd_src);
    if(fd_src == -1)
    {
        printf("Error in opening file. Please check that the file exists");
    }
    OFT *oftp_src = running->fd[fd_src];
    MINODE *mip_src = oftp_src->minodePtr;
    INODE *i_src = &mip_src->INODE;

    int fd_dest = open_file(dest, 1);
    if(fd_dest == -1)
    {
        printf("Error in opening file. Please check that the file exists");
    }
    OFT *oftp_dest = running->fd[fd_src];
    MINODE *mip_dest = oftp_dest->minodePtr;
    INODE *i_dest = &mip_dest->INODE;
    printf("fd dest : %d \n", fd_dest);
    if(mip_dest->dev == mip_src->dev)
    {
        
        printf("\nSame Dev\n");
        // As open file creates the destination file, we first delte the created file. 
        //This is not the most efficient way to do it, but I couldn't think of a better way to get the dev for dest 
        unlink(dest);
        my_link(src, dest);
        unlink(src);
    }
    else
    {
        printf("\nDIFF DEV\n");
        my_cp(src, dest);
        unlink(src);
    }


    
}