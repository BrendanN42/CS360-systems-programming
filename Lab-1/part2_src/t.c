#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

 
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
struct partition {
   u8 drive;             // drive number FD=0, HD=0x80, etc.

	u8  head;             // starting head 
	u8  sector;           // starting sector
	u8  cylinder;         // starting cylinder

	u8  sys_type;         // partition type: NTFS, LINUX, etc.

	u8  end_head;         // end head 
	u8  end_sector;       // end sector
	u8  end_cylinder;     // end cylinder

	u32 start_sector;     // starting sector counting from 0 
	u32 nr_sectors;       // number of of sectors in partition  
};

char *dev = "vdisk";
int fd;
    
// read a disk sector into char buf[512]
int read_sector(int fd, int sector, char *buf)
{
    lseek(fd, (long) (sector*512), SEEK_SET);  // lseek to byte sector*512
    read(fd, buf, 512);               // read 512 bytes into buf[ ]
}

int main()
{
  struct partition *p;
  char buf[512];

  fd = open(dev, O_RDONLY);   // open dev for READ
  read_sector(fd, 0, buf);    // read in MBR at sector 0    
  p = (struct partition *)&buf[0x1BE]; // p points at partition 1 in buf
  //Write C code to let p point at Partition 1 in buf[];
   
 // print P1's start_sector, nr_sectors, sys_type;
    
  //Write code to print all 4 partitions;
  int i=0;
  while(i!=4)
  {
    printf("P%d:\n",(i+1));
    printf("sys_type = '%d\n", p->sys_type);
    printf("start_sector = %u\n", p->start_sector);
    printf("end_sector %u\n", p->start_sector + p->nr_sectors - 1);
    printf("nr_sectors = %u\n\n", p->nr_sectors);

    
    if(i!=3) //don't increment p on the fourth partition
    {
      p++;
    }
      
      i++;
  }
    
//at partition 4 now, printing out the EXTEND portion 
    printf("********** Look for Extend Partition ************\n");
    int extStart=p->start_sector;
    int localMBR;


    printf("Ext Partition %d: %d\n\n", i, extStart);

    while(p->start_sector!=0)
    {
      if(p->start_sector==extStart)
      {
         localMBR=extStart;

      printf("Next localMPR = %d \n",p->start_sector);
      }
      else{
        localMBR=extStart+p->start_sector;
        printf("Next localMPR = %d + %d = %d\n", extStart,p->start_sector,localMBR);
      }

      read_sector(fd, localMBR, buf);
      p = (struct partition *)&buf[0x1BE];// access local ptable

      printf("P%d: ",(i+1));
      printf("start_sector = %u end_sector %u nr_sectors = %u\n", localMBR+p->start_sector,localMBR + p->start_sector + p->nr_sectors - 1,p->nr_sectors);
  
      printf("--------------------------------\n");
      printf("P%d: Entry1: start_sector = %u nr_sectors = %u\n", (i+1),p->start_sector, p->nr_sectors);

      p++; //move to the second entry

      printf("P%d: Entry2: start_sector = %u nr_sectors = %u\n",(i+1), p->start_sector, p->nr_sectors);
      printf("--------------------------------\n\n");
      i++;
   }
    printf("End of Extend partitions\n");
  
  }
   

