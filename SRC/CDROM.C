/***************************************************************************
 *    CDROM.C  - CD-ROM interface code (audio routines)                    *
 *                                                                         *
 ***************************************************************************/

#include "stdio.h"
#include "dos.h"
#include "malloc.h"
#include "sys\types.h"
#include "sys\stat.h"
#include "fcntl.h"

#ifdef __386__
#define   LES_MKFP(s,o)  (void far *)(s<<4)
#else
#define   LES_MKFP(s,o)  MK_FP(s,o)
#endif

#define   CMDBUFSIZE     22

#define   IOCMD2         0x4402
#define   IOCMD3         0x4403

//** IOCTL3 commands
#define   CD_OPENDOORCMD 0x00
#define   CD_CLOSEDOORCMD 0x05
#define   CD_RESETCMD    0x02

//** IOCTL2 commands
#define   CD_STATUSCMD   0x06

short cddrive,
     cddrives,
     cdhdl,
     cdhitrack,
     cdlotrack,
     cdtracktoplay;

long leadout;

unsigned short bufoff[100],bufseg[100],
     cmdoff,cmdseg,
     devoff,devseg,
     nameoff,nameseg,
     rmoff,rmseg;

char cddevname[9],
     far *namebuf;

char far *cddevadr;

struct redbook {
     char frame;
     char second;
     char minute;
     char unused;
     char control;
} redbook[20];

struct rmregs {
     unsigned long edi,esi,ebp;
     unsigned long reserved;
     unsigned long ebx,edx,ecx,eax;
     unsigned short flags,es,ds,fs,gs,ip,cs,sp,ss;
} rmregs;

#ifdef __386__
void
rmint86(short intno,struct rmregs *rmptr)
{
     union REGS regs;
     struct SREGS sregs;

     memset(&regs,0,sizeof(union REGS));
     segread(&sregs);
     regs.x.eax=0x0300;
     regs.x.ebx=intno;
     regs.x.ecx=0x0000;
     sregs.es=FP_SEG(rmptr);
     regs.x.edi=FP_OFF(rmptr);
     rmptr->ss=rmptr->sp=0;
     rmptr->flags=0;
     int386x(0x31,&regs,&regs,&sregs);
}
#else
void
rmint86(short intno,struct rmregs *rmptr)
{
     union REGS regs;
     struct SREGS sregs;

     regs.x.ax=rmptr->eax;
     regs.x.bx=rmptr->ebx;
     regs.x.cx=rmptr->ecx;
     regs.x.dx=rmptr->edx;
     regs.x.di=rmptr->edi;
     regs.x.si=rmptr->esi;
     regs.x.cflag=rmptr->flags;
     segread(&sregs);
     if (rmptr->es != 0) {
          sregs.es=rmptr->es;
     }
     if (rmptr->cs != 0) {
          sregs.cs=rmptr->cs;
     }
     if (rmptr->ss != 0) {
          sregs.ss=rmptr->ss;
     }
     if (rmptr->ds != 0) {
          sregs.ds=rmptr->ds;
     }
     int86x(intno,&regs,&regs,&sregs);
     rmptr->eax=regs.x.ax;
     rmptr->ebx=regs.x.bx;
     rmptr->ecx=regs.x.cx;
     rmptr->edx=regs.x.dx;
     rmptr->edi=regs.x.di;
     rmptr->esi=regs.x.si;
     rmptr->flags=regs.x.cflag;
     rmptr->es=sregs.es;
     rmptr->cs=sregs.cs;
     rmptr->ss=sregs.ss;
     rmptr->ds=sregs.ds;
}
#endif

#ifdef __386__
void far *
alcmem(short bytes)
{
     union REGS regs;

     regs.x.eax=0x0100;
     regs.x.ebx=((bytes+16)/16);
     int386(0x31,&regs,&regs);
     if (regs.x.cflag == 0) {
          rmseg=regs.x.eax;
          rmoff=0x0000;
          return((void far *)(rmseg<<4));
     }
     return(NULL);
}
#else
void far *
alcmem(short bytes)
{
     char far *ptr;

     ptr=_fmalloc(bytes);
     rmseg=FP_SEG(ptr);
     rmoff=FP_OFF(ptr);
     return(ptr);
}
#endif

int
ismscdex(void)
{
     memset(&rmregs,0,sizeof(struct rmregs));
     rmregs.eax=0x1500;
     rmregs.ebx=0x0000;
     rmint86(0x2F,&rmregs);
     if (rmregs.ebx != 0) {
          cddrive='A'+rmregs.ecx;
          return('A'+rmregs.ecx);
     }
     return(0);
}

void
cd_getname(void)
{
     if (alcmem(cddrives*5) != NULL) {
          devseg=rmseg;
          devoff=rmoff;
          namebuf=LES_MKFP(devseg,devoff);
     }
     memset(&rmregs,0,sizeof(struct rmregs));
     rmregs.eax=0x1501;
     rmregs.ebx=devoff;
     rmregs.es=devseg;
     rmint86(0x2F,&rmregs);
}

void
sendIOcmd(short hdl,short bytes,short cmd)
{
     memset(&rmregs,0,sizeof(rmregs));
     rmregs.ebx=hdl;
     rmregs.ds=cmdseg;
     rmregs.edx=cmdoff;
     rmregs.ecx=bytes;
     rmregs.eax=cmd;
     rmint86(0x21,&rmregs);
}

void
cd_opendoor(short cdhdl)
{
     char far *ptr;

     ptr=LES_MKFP(cmdseg,cmdoff);
     ptr[0]=CD_OPENDOORCMD;
     sendIOcmd(cdhdl,1,IOCMD3);
}

void
cd_closedoor(short cdhdl)
{
     char far *ptr;

     ptr=LES_MKFP(cmdseg,cmdoff);
     ptr[0]=CD_CLOSEDOORCMD;
     sendIOcmd(cdhdl,1,IOCMD3);
}

void
cd_reset(short cdhdl)
{
     char far *ptr;

     ptr=LES_MKFP(cmdseg,cmdoff);
     ptr[0]=CD_RESETCMD;
     sendIOcmd(cdhdl,1,IOCMD3);
}

void
cd_getaudiodisk(short cdhdl)
{
     int  i;
     char far *ptr;

     ptr=LES_MKFP(cmdseg,cmdoff);
     ptr[0]=0x0A;
     sendIOcmd(cdhdl,8,IOCMD2);
     cdlotrack=ptr[1];
     cdhitrack=ptr[2];
     leadout=ptr[3]+(ptr[4]<<8)+(ptr[5]<<16)+(ptr[6]<<24);
}

void
cd_getaudiotrack(short cdhdl,char tno)
{
     char far *ptr;

     ptr=LES_MKFP(cmdseg,cmdoff);
     ptr[0]=0x0B;
     ptr[1]=tno;
     sendIOcmd(cdhdl,8,IOCMD2);
     redbook[tno].frame=ptr[2];
     redbook[tno].second=ptr[3];
     redbook[tno].minute=ptr[4];
     redbook[tno].unused=ptr[5];
     redbook[tno].control=ptr[6];
}

int
cd_init(void)
{
     int  i;

     if ((cddrives=ismscdex()) == 0) {
          return(0);
     }
     if (alcmem(CMDBUFSIZE) == NULL) {
          return(0);
     }
     cmdseg=rmseg;
     cmdoff=rmoff;
     cd_getname();
     nameseg=(unsigned short)(namebuf[4]<<8)+namebuf[3];
     nameoff=(unsigned short)(namebuf[2]<<8)+namebuf[1];
     cddevadr=LES_MKFP(nameseg,nameoff);
     _fmemmove((char far *)&cddevname,cddevadr+10,8);
     if ((cdhdl=open(cddevname,O_RDWR)) == -1) {
          return(0);
     }
     cd_reset(cdhdl);
     cd_getaudiodisk(cdhdl);
     for (i=cdlotrack ; i <= cdhitrack ; i++) {
          cd_getaudiotrack(cdhdl,i);
     }
     return(1);
}

void
cd_uninit(void)
{
     close(cdhdl);
}

void
cd_stopplay(void)
{
     char far *ptr;

     ptr=LES_MKFP(cmdseg,cmdoff);
     memset(ptr,0,CMDBUFSIZE);
     ptr[0]=0x16;
     ptr[1]=0x00;
     ptr[2]=0x85;
     memset(&rmregs,0,sizeof(struct rmregs));
     rmregs.es=cmdseg;
     rmregs.ebx=cmdoff;
     rmregs.ecx=cddrive-'A';
     rmregs.eax=0x1510;
     rmint86(0x2F,&rmregs);
}

int
cd_playtrack(char tno)
{
     char status;
     long frames,hsgfmt;
     char far *ptr;

     if ((redbook[tno].control&0x40) == 0x40) {   // don't play data tracks
          return(0);
     }
     cd_stopplay();
     ptr=LES_MKFP(cmdseg,cmdoff);
     memset(ptr,0,CMDBUFSIZE);
     ptr[0]=0x16;
     ptr[1]=0x00;   // sub-unit
     ptr[2]=0x84;   // play-audio command
     ptr[13]=0x00;  // addressing mode
     hsgfmt=((long)redbook[tno].minute*4500L)+((long)redbook[tno].second*75L)
          +((long)redbook[tno].frame)-150L;
     ptr[14]=(char)hsgfmt&0x0FF;
     ptr[15]=(char)(hsgfmt>>8)&0xFF;
     ptr[16]=(char)(hsgfmt>>16)&0xFF;
     ptr[17]=(char)(hsgfmt>>24)&0xFF;
     if (tno+1 > cdhitrack) {
          frames=leadout;
     }
     else {
          frames=((long)redbook[tno+1].minute*4500L)+((long)redbook[tno+1].second*75L)
               +((long)redbook[tno+1].frame)-150L;
     }
     frames-=hsgfmt;
     ptr[18]=(char)frames&0xFF;
     ptr[19]=(char)(frames>>8)&0xFF;
     ptr[20]=(char)(frames>>16)&0xFF;
     ptr[21]=(char)(frames>>24)&0xFF;
     memset(&rmregs,0,sizeof(struct rmregs));
     rmregs.es=cmdseg;
     rmregs.ebx=cmdoff;
     rmregs.ecx=cddrive-'A';
     rmregs.eax=0x1510;
     rmint86(0x2F,&rmregs);
     status=ptr[4];
     if (status&0x01) {
          return(1);
     }
     return(0);
}

void
cd_resumeplay(void)
{
     char far *ptr;

     ptr=LES_MKFP(cmdseg,cmdoff);
     memset(ptr,0,CMDBUFSIZE);
     ptr[0]=0x16;
     ptr[1]=0x00;
     ptr[2]=0x88;
     memset(&rmregs,0,sizeof(struct rmregs));
     rmregs.es=cmdseg;
     rmregs.ebx=cmdoff;
     rmregs.ecx=cddrive-'A';
     rmregs.eax=0x1510;
     rmint86(0x2F,&rmregs);
}

#if 0
void
main(void)
{
     short i;
     unsigned short nameoff,nameseg;
     char far *ptr;

     if ((cddrives=ismscdex())) {
          if (alcmem(CMDBUFSIZE) == NULL) {
               exit(0);
          }
          cmdseg=rmseg;
          cmdoff=rmoff;
          ptr=LES_MKFP(cmdseg,cmdoff);
          printf("CD-ROM drive is %c\n",cddrive);
          cd_getname();
          nameseg=(unsigned short)(namebuf[4]<<8)+namebuf[3];
          nameoff=(unsigned short)(namebuf[2]<<8)+namebuf[1];
          cddevadr=LES_MKFP(nameseg,nameoff);
          _fmemmove((char far *)&cddevname,cddevadr+10,8);
          if ((cdhdl=open(cddevname,O_RDWR)) != -1) {
               //cd_opendoor(cdhdl);
               //printf("Insert CD then press a key\n");
               //getch();
               //cd_closedoor(cdhdl);
               //printf("Door closed.  Resetting drive\n");
               cd_reset(cdhdl);
               printf("Getting audio disk info\n");
               cd_getaudiodisk(cdhdl);
               printf("Getting and playing audio tracks\n");
               for (i=cdlotrack ; i <= cdhitrack ; i++) {
                    cd_getaudiotrack(cdhdl,i);
               }
               for (i=cdlotrack ; i <= cdhitrack ; i++) {
                    if (cd_playtrack(i)) {
                         printf("Playing track %d...any key to stop\n",i);
                         getch();
                    }
               }
               cd_reset(cdhdl);
               close(cdhdl);
          }
     }
}
#endif
