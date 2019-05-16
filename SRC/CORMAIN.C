/***************************************************************************
 *   CORMAIN.C - main module for experimental game                         *
 *                                                                         *
 *                                                     08/06/95 Les Bird   *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include "build.h"
#include "names.h"
#include "les.h"

//#define   DEBUGOUTPUT

#define   NUMOPTIONS          8
#define   MOVEFIFOSIZE        256
#define   AVERAGEFRAMES       16
#define   MAXTYPES            8
#define   MINHORIZ            0
#define   MAXHORIZ            200
#define   CTRHORIZ            100
#define   STEPHORIZ           4
#define   MAXDOORS            200
#define   MESSAGETIME         (CLKIPS*4)
#define   STATBARHEIGHT       32
#define   MAXSECTOREFFECTORS  100
#define   PULSECLOCK          16
#define   FLICKERCLOCK        16
#define   DIECLOCK            20
#define   SPARKCLOCK          15
#define   FLASHCLOCK          15
#define   EXPLOSIONDIST       2048L
#define   MAXRANDOMSPOTS      32
#define   MAXHEALTH           100

#define   AMMOSTATX           5
#define   AMMOSTATY           (ydim-STATBARHEIGHT+5)
#define   HEALTHSTATX         55
#define   HEALTHSTATY         (ydim-STATBARHEIGHT+5)
#define   FRAGSTATX           106
#define   FRAGSTATY           (ydim-STATBARHEIGHT+5)

#define   GRAVITYCONSTANT     (TICSPERFRAME<<5)
#define   JUMPVEL             (TICSPERFRAME<<9)
#define   GROUNDBIT           1
#define   PLATFORMBIT         2

#define   EXPDELAYRATE        8L

#define   MAXPARMS            5

#define   DEBUGPLAYERS        0x01
#define   DEBUGNEWPLAYER      0x02
#define   DEBUGNEWVIEW        0x04
#define   DEBUGNEWCONTROL     0x08

#ifdef DEBUGOUTPUT
FILE *dbgfp;
#endif

//** ENGINE variables

void (__interrupt __far *oldtimerhandler)();

long chainxres[4]={256,320,360,400};
long chainyres[11]={200,240,256,270,300,350,360,400,480,512,540};
long vesares[7][2]={320,200,640,400,640,480,800,600,1024,768,1280,1024,1600,1200};
long digihz[7]={6000,8000,11025,16000,22050,32000,44100};
long globlohit,globloz,globhihit,globhiz;
long frameval[AVERAGEFRAMES],framecnt;

char option[NUMOPTIONS];

struct sectortype *sectptr[MAXSECTORS];
struct spritetype *sprptr[MAXSPRITES];
struct walltype *wallptr[MAXWALLS];

short brightness;

char scantoasc[128]={
     0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
     'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
     'd','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v',
     'b','n','m',',','.','/',0,'*',0,32,0,0,0,0,0,0,
     0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
     '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

char scantoascwithshift[128]={
     0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
     'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,'A','S',
     'D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V',
     'B','N','M','<','>','?',0,'*',0,32,0,0,0,0,0,0,
     0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
     '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

//** MULTIPLAYER variables

char tempbuf[576];

extern
short connecthead,
     connectpoint2[MAXPLAYERS],
     myconnectindex,
     numplayers;

long fakeclock,
     lockclock,
     movefifoend,
     movefifoplc;

//** GAME variables

jmp_buf jmpenv;

enum {
     MOVEFWD,
     MOVEBACK,
     MOVELEFT,
     MOVERIGHT,
     MOVERUN,
     MOVESTRAFE,
     MOVESHOOT,
     MOVEUSE,
     MOVEJUMP,
     MOVECROUCH,
     MOVELOOKUP,
     MOVELOOKDN,
     MOVECENTER,
     MOVESTRAFEL,
     MOVESTRAFER,
     MOVEMAP,
     UNDEFINED1,
     MOVEZOOMI,
     MOVEZOOMO,
     UNDEFINED2,
     UNDEFINED3,
     UNDEFINED4,
     UNDEFINED5,
     UNDEFINED6,
     UNDEFINED7,
     UNDEFINED8,
     UNDEFINED9,
     NUMKEYS
};

int  curvidmode,
     debugflags,
     engineinitialized,
     multiinitialized,
     soundinitialized,
     timerinitialized;

long msgclock,
     randomspota[MAXRANDOMSPOTS],
     randomspots[MAXRANDOMSPOTS],
     randomspotx[MAXRANDOMSPOTS],
     randomspoty[MAXRANDOMSPOTS],
     randomspotz[MAXRANDOMSPOTS],
     starta,
     starts,
     startx,
     starty,
     startz,
     weapclock[MAXPLAYERS];

unsigned
long oplrmove[MAXPLAYERS],
     plrflags[MAXPLAYERS],
     plrmove[MAXPLAYERS];

enum {
     G_ROCKET,
     G_GATLING,
     G_SHOTGUN,
     G_PLASMA,
     MAXWEAPONS
};

short defammomax[]={
     20,
     200,
     50,
     200
};

struct weaptype {
     char selectable;
     short basepic,firepic,firestartpic,fireendpic;
     char action[8];
     char delay;
     char reloaddelay;
} weaptype[MAXWEAPONS]={
     {1,GUN5ONBOTTOM,GUN5FIREPIC,GUN5FIRESTART,GUN5FIREEND,
          1,0,0,0,0,0,0,0,8,60},
     {1,GUN4ONBOTTOM,GUN4FIRE1,GUN4FIRE1,GUN4FIRE2,
          1,0,0,0,0,0,0,0,4,0},
     {1,GUN3ONBOTTOM,GUN3ONBOTTOM,GUN3FIREPIC,GUN3COCKEND,
          1,0,0,0,0,0,0,0,18,0},
     {1,GUN6ONBOTTOM,GUN6ONBOTTOM,GUN6FIREPIC,GUN6FIREEND,
          1,0,0,0,0,0,0,0,4,0}
};

short curplyr,
     curview,
     defvisibility,
     doorsect[MAXSECTORS],
     ingame=1,
     lasthitclock[MAXPLAYERS],
     mousxspeed=1,
     mousyspeed=1,
     npyrs,
     numdoors,
     oldvmode,
     plrammo[MAXPLAYERS][MAXWEAPONS],
     plrammomax[MAXPLAYERS][MAXWEAPONS],
     plrdead[MAXPLAYERS],
     plrfrags[MAXPLAYERS],
     plrmousx[MAXPLAYERS],
     plrmousy[MAXPLAYERS],
     plrscreen[MAXPLAYERS],
     plrzoom[MAXPLAYERS],
     pyrn,
     ready2send,
     screensize,
     secnt,
     sectfx[MAXSECTORS],
     totrandomspots,
     typemsgflag,
     typemsglen,
     weap[MAXPLAYERS],
     weapstep[MAXPLAYERS],
     xmax,xmin,
     ymax,ymin;

short maxmvel[MAXTYPES],
     maxsvel[MAXTYPES],
     maxtvel[MAXTYPES],
     minmvel[MAXTYPES],
     minsvel[MAXTYPES],
     mintvel[MAXTYPES],
     mvel[MAXPLAYERS],
     plrsprite[MAXPLAYERS],
     svel[MAXPLAYERS],
     tvel[MAXPLAYERS];

char keys[NUMKEYS],
     mapname[80],
     msgbuf[80],
     syncpack,
     typebuf[80],
     weapflags[MAXPLAYERS][MAXWEAPONS];

char *parms[MAXPARMS]={
     "MAP",
     "PLAYERS",
     "COM",
     "BAUD",
     "IPX"
};

char comtable[]={0x00,0x01,0x02,0x03,0x04};

struct spritedata {
     char type;
     char onsomething;
     short oldsectnum;
     short height;
     short health;
     short forcea;
     short forcev;
     short horiz;
     unsigned short misc;
     long hvel;
     long z;                       // view & world Z
     long fallz;
     long aclock;
} spritedata[MAXSPRITES],
     *sprdta[MAXSPRITES];

enum {
     D_NOTHING,
     D_OPENDOOR,
     D_CLOSEDOOR,
     D_OPENSOUND,
     D_CLOSESOUND,
     D_OPENING,
     D_CLOSING,
     D_WAITING,
     D_DISABLED
};

struct doortype {
     short type;
     short state;
     short sector;
     long startz;
     long destz;
     long clock;
     short stepz;
     long centerx;
     long centery;
} doortype[MAXDOORS],
     *doorptr[MAXDOORS];

struct sectoreffect {
     short type;
     short sector;
     short lolight;
     short hilight;
     short pdelay;
     long pclock;
} sectoreffect[MAXSECTOREFFECTORS],
     *septr[MAXSECTOREFFECTORS];

enum {
     M_MSYNC=0,
     M_SSYNC,
     M_MESSAGE,
     M_LOGOUT=255
};

struct multipack {
     unsigned long plrmove;
     short mousx;
     short mousy;
     char weap;
} multipack[MOVEFIFOSIZE][MAXPLAYERS],
     rcvpack[MAXPLAYERS],
     sndpck;

//** Function prototypes

void monitor(void);
short movesprite(short,long,long,long,long,long,char);
void initplayer(short);
void drawstatbarnum(short,short,short);
short getspriteangle(short,short);
void drawscreen(short,long);
long getspritedist(short,short);
void drawscreenfx(void);
void spawnflash(long,long,long,short);

void (interrupt far *oldGPhandler)();
void (interrupt far *oldPGhandler)();

//*************************************************************

void
setvmode(short mode)
{
     union REGS regs;

     regs.h.ah=0x00;
     regs.h.al=(char)mode;
     int386(0x10,&regs,&regs);
}

int
getvmode(void)
{
     union REGS regs;

     regs.h.ah=0x0F;
     regs.h.al=0x00;
     int386(0x10,&regs,&regs);
     return(regs.h.al);
}

#pragma aux gpsetmode=\
     "int 10h",     \
     parm [eax]     \

void gpsetmode(short);

void interrupt far
newGPhandler(union INTPACK r)
{
     gpsetmode(oldvmode);
     _chain_intr(oldGPhandler);
}

void interrupt far
newPGhandler(union INTPACK r)
{
     gpsetmode(oldvmode);
     _chain_intr(oldPGhandler);
}

void far *
dpmi_getexception(int no)
{
     void far *fp;
     union REGS regs;

     regs.x.eax=0x202;
     regs.x.ebx=no;
     int386(0x31,&regs,&regs);
     fp=MK_FP(regs.w.cx,regs.x.edx);
     return(fp);
}

void
dpmi_setexception(int no,void far *func)
{
     union REGS regs;

     regs.x.eax=0x203;
     regs.x.ebx=no;
     regs.x.ecx=FP_SEG(func);
     regs.x.edx=FP_OFF(func);
     int386(0x31,&regs,&regs);
}

void
installGPhandler(void)
{
     oldGPhandler=(void interrupt far *)dpmi_getexception(0x0D);
     oldPGhandler=(void interrupt far *)dpmi_getexception(0x0E);
     dpmi_setexception(0x0D,(void far *)newGPhandler);
     dpmi_setexception(0x0E,(void far *)newPGhandler);
}

void
printextf(short x,short y,char *fmt,...)
{
     char buf[80];
     va_list argptr;

     va_start(argptr,fmt);
     vsprintf(buf,fmt,argptr);
     va_end(argptr);
     printext256(x,y,31,-1,buf,1);
}

void
uninittimer(void)
{
     outp(0x43,54);
     outp(0x40,255);
     outp(0x40,255);               //18.2 times/sec
     _disable();
     _dos_setvect(0x08,oldtimerhandler);
     _enable();
}

void
shutdown(void)
{
     if (timerinitialized) {
          uninittimer();
     }
     if (soundinitialized) {
          uninitsb();
     }
     if (multiinitialized) {
          sendlogoff();
          uninitmultiplayers();
     }
     if (engineinitialized) {
          uninitengine();
          setvmode(oldvmode);
          showengineinfo();
     }
}

void
crash(char *fmt,...)
{
     va_list argptr;

     shutdown();
     va_start(argptr,fmt);
     vprintf(fmt,argptr);
     va_end(argptr);
     printf("\n");
     longjmp(jmpenv,1);
}

void
showmessage(char *fmt,...)
{
     va_list argptr;

     va_start(argptr,fmt);
     vsprintf(msgbuf,fmt,argptr);
     va_end(argptr);
     msgclock=lockclock+MESSAGETIME+(strlen(msgbuf)<<2);
}

void
keytimerstuff(void)
{
     char ch,keystate;
     short bstatus=0,i,mousx=0,mousy=0,ptype,s,strafing;

     plrmove[curplyr]=0;
     if (option[3] != 0) {
          getmousevalues(&mousx,&mousy,&bstatus);
     }
     plrmousx[curplyr]=mousx*mousxspeed;
     plrmousy[curplyr]=mousy*mousyspeed;
     if (typemsgflag) {
          while (keyfifoplc != keyfifoend) {
               ch=keyfifo[keyfifoplc];
               keystate=keyfifo[(keyfifoplc+1)&(KEYFIFOSIZ-1)];
               keyfifoplc=((keyfifoplc+2)&(KEYFIFOSIZ-1));
               if (keystate != 0) {
                    if (ch == 0x0E) {   // backspace key
                         if (typemsglen > 0) {
                              typebuf[typemsglen]=0;
                              typemsglen--;
                         }
                    }
                    else if (ch == 0x1C || ch == 0x9C) {
                         keystatus[0x1C]=keystatus[0x9C]=0;
                         if (typemsglen > 0) {
                              tempbuf[0]=M_MESSAGE;
                              for (i=0 ; i < typemsglen ; i++) {
                                   tempbuf[1+i]=typebuf[i];
                              }
                              for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
                                   if (i != curplyr) {
                                        sendpacket(i,tempbuf,typemsglen+1);
                                   }
                              }
                              showmessage("(%d): %s",curplyr,typebuf);
                              typemsglen=0;
                         }
                         typemsgflag=0;
                    }
                    else if (typemsglen < 75 && ch < 128) {
                         if (keystatus[0x2A] || keystatus[0x36]) {
                              ch=scantoascwithshift[ch];
                         }
                         else {
                              ch=scantoasc[ch];
                         }
                         if (ch != 0) {
                              typebuf[typemsglen]=toupper(ch);
                              typemsglen++;
                              typebuf[typemsglen]=0;
                         }
                    }
               }
          }
          return;
     }
     keyfifoplc=keyfifoend;
     for (i=0 ; i < NUMKEYS ; i++) {
          if (keystatus[keys[i]] != 0) {
               plrmove[curplyr]|=(1<<i);
          }
     }
     if (bstatus&0x01) {
          plrmove[curplyr]|=(1<<MOVESHOOT);
     }
     if (bstatus&0x06) {
          plrmove[curplyr]|=(1<<MOVESTRAFE);
     }
     for (i=0 ; i < MAXWEAPONS ; i++) {
          if (weaptype[i].selectable) {
               if (keystatus[0x02+i] != 0) {
                    if (weapstep[curplyr] == 0) {
                         weap[curplyr]=i;
                         drawstatusbar(curplyr);
                         break;
                    }
               }
          }
     }
     if (keystatus[0x0E]) {                       // backspace
          if (keystatus[0x19]) {                  // "P"
               if (debugflags&DEBUGPLAYERS) {
                    debugflags&=~DEBUGPLAYERS;
               }
               else {
                    debugflags|=DEBUGPLAYERS;
               }
               keystatus[0x19]=0;
          }
          if (keystatus[0x1B]) {                  // "]"
               keystatus[0x1B]=0;
               pskybits<<=1;
               if (pskybits == 0) {
                    pskybits=1;
               }
               showmessage("PSKYBITS=%X",pskybits);
          }
          else if (keystatus[0x1A]) {             // "["
               keystatus[0x1A]=0;
               pskybits>>=1;
               showmessage("PSKYBITS=%X",pskybits);
          }
          if (keystatus[0x1F]) {                  // "S"
               parallaxtype++;
               if (parallaxtype > 2) {
                    parallaxtype=0;
               }
               keystatus[0x1F]=0;
          }
          if (keystatus[0x32]) {                  // "M"
               for (i=0 ; i < (MAXSECTORS>>3) ; i++) {
                    show2dsector[i]=0xFF;
               }
               for (i=0 ; i < (MAXWALLS>>3) ; i++) {
                    show2dwall[i]=0xFF;
               }
               for (i=0 ; i < (MAXSPRITES>>3) ; i++) {
                    show2dsprite[i]=0xFF;
               }
          }
          if (option[4] == 0) {
               if (keystatus[59]) {               // F1
                    debugflags|=DEBUGNEWPLAYER;
                    keystatus[59]=0;
               }
               if (keystatus[60]) {               // F2
                    debugflags|=DEBUGNEWVIEW;
                    keystatus[60]=0;
               }
               if (keystatus[61]) {               // F3
                    debugflags|=DEBUGNEWCONTROL;
                    keystatus[61]=0;
               }
          }
     }
     else {
          if (keystatus[0x43]) {                  // F9
               if (keystatus[0x0C]) {             // "-"
                    keystatus[0x0C]=0;
                    mousxspeed--;
                    if (mousxspeed < 1) {
                         mousxspeed=1;
                    }
               }
               else if (keystatus[0x0D]) {        // "+"
                    keystatus[0x0D]=0;
                    mousxspeed++;
                    if (mousxspeed > 16) {
                         mousxspeed=16;
                    }
               }
               showmessage("MOUSE X SPEED %d",mousxspeed);
          }
          else if (keystatus[0x44]) {             // F10
               if (keystatus[0x0C]) {             // "-"
                    keystatus[0x0C]=0;
                    mousyspeed--;
                    if (mousyspeed < 1) {
                         mousyspeed=1;
                    }
               }
               else if (keystatus[0x0D]) {        // "+"
                    keystatus[0x0D]=0;
                    mousyspeed++;
                    if (mousyspeed > 16) {
                         mousyspeed=16;
                    }
               }
               showmessage("MOUSE Y SPEED %d",mousyspeed);
          }
          else if (keystatus[0x57]) {             // F11
               brightness++;
               if (brightness > 8) {
                    brightness=0;
               }
               setbrightness(brightness);
          }
          if (keystatus[0x29]) {                  // "`"
               keystatus[0x29]=0;
               typemsgflag=1;
          }
     }
}

void __interrupt __far
timerhandler(void)
{
     totalclock++;
     outp(0x20,0x20);
}

void
parseargs(int argc,char *argv[])
{
     short comirq,comport,i,j=0,k;
     long combaud;
     char multitype[16];

     npyrs=1;
     strcpy(mapname,"NEWBOARD.MAP");
     option[4]=option[5]=0;
     while (j < argc) {
          strupr(argv[j]);
          for (i=0 ; i < MAXPARMS ; i++) {
               if (strcmp(argv[j],parms[i]) == 0) {
                    switch (i) {
                    case 0:        // map name
                         strcpy(mapname,argv[j+1]);
                         if (strstr(mapname,".MAP") == NULL) {
                              strcat(mapname,".MAP");
                         }
                         j++;
                         break;
                    case 1:        // players
                         npyrs=atoi(argv[j+1]);
                         j++;
                         break;
                    case 2:        // com
                         k=atoi(argv[j+1]);
                         if (k > 0 && k < 5) {
                              option[4]|=comtable[k];
                              comport=k;
                         }
                         j++;
                         strcpy(multitype,"COM");
                         break;
                    case 3:        // baud rate
                         k=atoi(argv[j+1]);
                         switch (k) {
                         case 2400:
                              combaud=2400L;
                              break;
                         case 4800:
                              option[5]|=0x01;
                              combaud=4800L;
                              break;
                         case 14400:
                              option[5]|=0x03;
                              combaud=14400L;
                              break;
                         case 19200:
                              option[5]|=0x04;
                              combaud=19200L;
                              break;
                         case 28800:
                              option[5]|=0x05;
                              combaud=28800L;
                              break;
                         default:
                         case 9600:
                              option[5]|=0x02;
                              combaud=9600L;
                              break;
                         }
                         j++;
                         break;
                    case 4:        // ipx
                         option[4]=5;
                         strcpy(multitype,"IPX");
                         break;
                    }
                    break;
               }
          }
          j++;
     }
     if (option[4] == 1 || option[4] == 3) {
          option[5]|=0x20;
          comirq=4;
     }
     else if (option[4] == 2 || option[4] == 4) {
          option[5]|=0x10;
          comirq=3;
     }
     else if (option[4] == 5) {
          if (npyrs < 2) {
               npyrs=2;
          }
          else {
               option[4]+=(npyrs-2);
          }
     }
     fprintf(stderr," Number of players = %d\n",npyrs);
     if (option[4] != 0) {
          fprintf(stderr," Setting up for a multiplayer %s game\n",multitype);
          if (option[4] < 5) {
               fprintf(stderr,"   COM: %d BUAD: %-6ld IRQ: %d\n",comport,
                    combaud,comirq);
          }
     }
}

void
readsetup(void)
{
     int  fil;

     if ((fil=open("setup.dat",O_BINARY|O_RDWR,S_IREAD)) != -1) {
          read(fil,&option[0],NUMOPTIONS);
          read(fil,&keys[0],NUMKEYS);
          close(fil);
     }
}

void
inittimer(void)
{
     outp(0x43,54);
     outp(0x40,9942&255);
     outp(0x40,9942>>8);           //120 times/sec
     oldtimerhandler=_dos_getvect(0x08);
     _disable();
     _dos_setvect(0x08,timerhandler);
     _enable();
}

void
initpaltable(void)
{
     short i,j,k,l;
     FILE *fp;

     if ((fp=fopen("lookup.dat","rb")) != NULL) {
          l=getc(fp);
          for (j=0 ; j < l ; j++) {
               k=getc(fp);
               for (i=0 ; i < 256 ; i++) {
                    tempbuf[i]=getc(fp);
               }
               makepalookup(k,tempbuf,0,0,0,1);
          }
          fclose(fp);
     }
}

void
drawstatbarnum(short x,short y,short n)
{
     short i,pic;
     char tmpbuf[5];

     sprintf(tmpbuf,"%3d",n);
     for (i=0 ; i < strlen(tmpbuf) ; i++) {
          if (tmpbuf[i] >= '0' && tmpbuf[i] <= '9') {
               pic=BIGNUM0+(tmpbuf[i]-'0');
               permanentwritesprite(x,y,pic,0,0L,ydim-STATBARHEIGHT,
                    xdim-1,ydim-1,0);
               x+=tilesizx[pic];
          }
          else if (tmpbuf[i] == '-') {
               pic=BIGNUMMINUS;
               permanentwritesprite(x,y+4,pic,0,0L,ydim-STATBARHEIGHT,
                    xdim-1,ydim-1,0);
               x+=tilesizx[pic];
          }
          else {
               x+=tilesizx[BIGNUM0+1];
          }
     }
}

void
drawstatusbar(short p)
{
     short n,x,y;

     if (p != curview || screensize > xdim) {
          return;
     }
     permanentwritesprite((xdim-320)>>1,ydim-STATBARHEIGHT,STATUSBARPIC,0,
          0L,0L,xdim-1,ydim-1,0);
     x=AMMOSTATX;
     y=AMMOSTATY;
     drawstatbarnum(x,y,plrammo[p][weap[p]]);
     x=HEALTHSTATX;
     y=HEALTHSTATY;
     n=sprdta[plrsprite[p]]->health;
     if (n < 0) {
          n=0;
     }
     drawstatbarnum(x,y,n);
     x=FRAGSTATX;
     y=FRAGSTATY;
     drawstatbarnum(x,y,plrfrags[p]);
}

int
changeammo(short p,short delta,short gun)
{
     short a;

     if (delta > 0 && plrammo[p][gun] >= plrammomax[p][gun]) {
          return(0);
     }
     a=plrammo[p][gun]+delta;
     if (a < 0) {
          a=0;
     }
     else if (a > plrammomax[p][gun]) {
          a=plrammomax[p][gun];
     }
     plrammo[p][gun]=a;
     drawstatusbar(p);
     return(1);
}

void
changehealth(short s,short delta)
{
     short p;

     sprdta[s]->health+=delta;
     if ((p=sprptr[s]->owner) >= MAXSPRITES) {
          if (delta < 0 && sprdta[s]->health > 0) {
               if (lockclock > lasthitclock[p]) {
                    wsayfollow("dsplpain.wav",4096L+(krand()&255)-128,256L,
                         &sprptr[s]->x,&sprptr[s]->y,0);
                    lasthitclock[p]=lockclock+(CLKIPS*2);
               }
          }
          drawstatusbar(p-MAXSPRITES);
     }
}

void
setup3dscreen(void)
{
     if (screensize <= xdim) {
          if (screensize < xdim) {
               permanentwritesprite(0L,0L,BACKGROUNDPIC,0,0L,0L,xdim-1,ydim-1,0);
          }
          drawstatusbar(curview);
          xmin=(xdim>>1)-(screensize>>1);
          xmax=xmin+screensize-1;
          ymin=((ydim-STATBARHEIGHT)>>1)-(((screensize*(ydim-STATBARHEIGHT))/xdim)>>1);
          ymax=ymin+((screensize*(ydim-STATBARHEIGHT))/xdim)-1;
     }
     else {
          xmin=0;
          ymin=0;
          xmax=xdim;
          ymax=ydim;
     }
     setview(xmin,ymin,xmax,ymax);
}

void
initboard(void)
{
     short i,n,s,w,wallend,wallstart;
     long dax,day;

     fprintf(stderr," Initializing sectors");
     for (i=0 ; i < MAXSECTORS ; i++) {
          sectptr[i]=&sector[i];
          sectfx[i]=-1;
     }
     fprintf(stderr,", sprites");
     for (i=0 ; i < MAXSPRITES ; i++) {
          sprptr[i]=&sprite[i];
          sprdta[i]=&spritedata[i];
     }
     fprintf(stderr,", walls");
     for (i=0 ; i < MAXWALLS ; i++) {
          wallptr[i]=&wall[i];
     }
     fprintf(stderr,", doors\n");
     numdoors=0;
     for (i=0 ; i < MAXDOORS ; i++) {
          doorptr[i]=&doortype[i];
          memset(doorptr[i],0,sizeof(struct doortype));
          doorsect[i]=-1;
     }
     secnt=0;
     for (i=0 ; i < MAXSECTOREFFECTORS ; i++) {
          septr[i]=&sectoreffect[i];
          memset(septr[i],0,sizeof(struct sectoreffect));
     }
     fprintf(stderr," loadboard() - mapname: %s\n",mapname);
     if (loadboard(mapname,&startx,&starty,&startz,&starta,&starts) == -1) {
          crash("initboard: Board \"%s\" not found!",mapname);
     }
     fprintf(stderr," Scanning for sector tags");
     for (i=0 ; i < MAXSECTORS ; i++) {
          if (sectptr[i]->wallnum > 0) {
               dax=day=0L;
               wallstart=sectptr[i]->wallptr;
               wallend=wallstart+sectptr[i]->wallnum;
               for (w=wallstart ; w < wallend ; w++) {
                    dax+=wallptr[w]->x;
                    day+=wallptr[w]->y;
               }
               dax/=(wallend-wallstart);
               day/=(wallend-wallstart);
          }
          switch (sectptr[i]->lotag) {
          case DOORUPTAG:
               if (numdoors >= MAXDOORS) {
                    crash("initboard: Maximum doors exceeded");
               }
               n=numdoors++;
               doorsect[i]=n;
               doorptr[n]->type=sectptr[i]->lotag;
               doorptr[n]->state=D_NOTHING;
               doorptr[n]->sector=i;
               s=nextsectorneighborz(i,sectptr[i]->floorz,-1,-1);
               doorptr[n]->destz=sectptr[s]->ceilingz;
               doorptr[n]->startz=sectptr[i]->floorz;
               doorptr[n]->stepz=DOORSPEED;
               doorptr[n]->centerx=dax;
               doorptr[n]->centery=day;
               break;
          case DOORSPLITVER:
               if (numdoors >= MAXDOORS) {
                    crash("initboard: Maximum doors exceeded");
               }
               n=numdoors++;
               doorsect[i]=n;
               doorptr[n]->type=sectptr[i]->lotag;
               doorptr[n]->state=D_NOTHING;
               doorptr[n]->sector=i;
               wallstart=sectptr[i]->wallptr;
               wallend=wallstart+sectptr[i]->wallnum;
               break;
          case PLATFORMELEVTAG:
          case PLATFORMDELAYTAG:
          case PLATFORMDROPTAG:
               if (numdoors >= MAXDOORS) {
                    crash("initboard: Maximum doors exceeded");
               }
               n=numdoors++;
               doorsect[i]=n;
               doorptr[n]->type=sectptr[i]->lotag;
               doorptr[n]->state=D_NOTHING;
               doorptr[n]->sector=i;
               s=nextsectorneighborz(i,sectptr[i]->floorz,1,1);
               if (s >= 0) {
                    doorptr[n]->destz=sectptr[s]->floorz;
               }
               else {
                    s=nextsectorneighborz(i,sectptr[i]->floorz,-1,-1);
                    doorptr[n]->destz=sectptr[s]->ceilingz;
               }
               doorptr[n]->startz=sectptr[i]->floorz;
               doorptr[n]->stepz=DOORSPEED;
               doorptr[n]->centerx=dax;
               doorptr[n]->centery=day;
               break;
          }
     }
     fprintf(stderr,"\n");
     fprintf(stderr,"   total doors: %d\n",numdoors);
     fprintf(stderr," Scanning sprites\n");
     totrandomspots=0;
     randomspota[totrandomspots]=starta;
     randomspotx[totrandomspots]=startx;
     randomspoty[totrandomspots]=starty;
     randomspotz[totrandomspots]=startz;
     randomspots[totrandomspots]=starts;
     totrandomspots++;
     for (i=0 ; i < MAXSPRITES ; i++) {
          if (sprptr[i]->statnum < MAXSTATUS) {
               switch (sprptr[i]->picnum) {
               case SECTOREFFECT:
                    if (sprptr[i]->lotag >= 1000
                       && sprptr[i]->lotag < 1000+MAXSECTOREFFECTS) {
                         if (secnt >= MAXSECTOREFFECTORS) {
                              crash("initboard: Sector effect limit exceeded");
                         }
                         n=secnt++;
                         s=sprptr[i]->sectnum;
                         sectfx[s]=n;
                         septr[n]->sector=s;
                         septr[n]->type=sprptr[i]->lotag-1000;
                         switch (septr[n]->type) {
                         case PULSELIGHT:
                              septr[n]->lolight=sectptr[s]->floorshade;
                              septr[n]->hilight=sectptr[s]->ceilingshade;
                              if (septr[n]->pdelay == 0) {
                                   septr[n]->pdelay=PULSECLOCK;
                              }
                              wallstart=sectptr[s]->wallptr;
                              wallend=wallstart+sectptr[s]->wallnum;
                              for (w=wallstart ; w < wallend ; w++) {
                                   wallptr[w]->shade=septr[n]->lolight;
                              }
                              sectptr[s]->ceilingshade=septr[n]->lolight;
                              deletesprite(i);
                              break;
                         case FLICKERLIGHT:
                              septr[n]->lolight=sectptr[s]->floorshade;
                              septr[n]->hilight=sectptr[s]->ceilingshade;
                              if (septr[n]->pdelay == 0) {
                                   septr[n]->pdelay=FLICKERCLOCK;
                              }
                              wallstart=sectptr[s]->wallptr;
                              wallend=wallstart+sectptr[s]->wallnum;
                              for (w=wallstart ; w < wallend ; w++) {
                                   wallptr[w]->shade=septr[n]->hilight;
                              }
                              sectptr[s]->floorshade=septr[n]->hilight;
                              deletesprite(i);
                              break;
                         }
                    }
                    break;
               case RANDOMSPOTPIC:
                    if (totrandomspots >= MAXRANDOMSPOTS) {
                         break;
                    }
                    randomspota[totrandomspots]=sprptr[i]->ang;
                    randomspots[totrandomspots]=sprptr[i]->sectnum;
                    randomspotx[totrandomspots]=sprptr[i]->x;
                    randomspoty[totrandomspots]=sprptr[i]->y;
                    randomspotz[totrandomspots]=sprptr[i]->z;
                    totrandomspots++;
                    deletesprite(i);
                    break;
               }
               if (sprptr[i]->statnum == ACTIVESTAT) {
                    sprdta[i]->z=sectptr[sprptr[i]->sectnum]->floorz;
               }
          }
     }
     for (i=0 ; i < MAXSPRITES ; i++) {
          if (sprptr[i]->statnum < MAXSTATUS) {
               switch (sprptr[i]->picnum) {
               case TIMEREFFECT:
                    if (sprptr[i]->lotag >= 1000
                       && sprptr[i]->lotag < 1000+MAXSECTOREFFECTS) {
                         s=sprptr[i]->sectnum;
                         n=sectfx[s];
                         switch (sprptr[i]->lotag-1000) {
                         case PULSELIGHT:
                         case FLICKERLIGHT:
                              if (septr[n]->type == sprptr[i]->lotag-1000) {
                                   septr[n]->pdelay=sprptr[i]->hitag;
                              }
                              break;
                         }
                         deletesprite(i);
                    }
                    break;
               case SECTOREFFECT:
                    if (sprptr[i]->lotag >= 1000
                       && sprptr[i]->lotag < 1000+MAXSECTOREFFECTS) {
                         switch (sprptr[i]->lotag-1000) {
                         case DOORSPEEDEFFECT:
                              n=doorsect[sprptr[i]->sectnum];
                              if (n >= 0) {
                                   doorptr[n]->stepz=sprptr[i]->hitag;
                              }
                              deletesprite(i);
                              break;
                         }
                    }
                    break;
               }
          }
     }
     fprintf(stderr,"   total sector effects: %d\n",secnt);
     fprintf(stderr," precache()\n");
     precache();
     for (i=0 ; i < MAXTYPES ; i++) {
          minmvel[i]=-127;
          maxmvel[i]=128;
          mintvel[i]=-24;
          maxtvel[i]=24;
          minsvel[i]=-127;
          maxsvel[i]=128;
     }
     automapping=1;
     fprintf(stderr,"  Press RETURN:");
     while (keystatus[0x1C] == 0 && keystatus[0x9C] == 0);
}

void
pickrandomspot(long *a,long *x,long *y,long *z,short *s)
{
     short i,j,r,stat,tryagain;

     do {
          r=krand()%totrandomspots;
          *x=randomspotx[r];
          *y=randomspoty[r];
          *z=randomspotz[r];
          *s=randomspots[r];
          *a=randomspota[r];
          tryagain=0;
          i=headspritesect[*s];
          while (i >= 0) {
               j=nextspritesect[i];
               stat=sprptr[i]->statnum;
               if (stat > INACTIVESTAT && stat < MAXSTATUS) {
                    if (sprptr[i]->x == *x && sprptr[i]->y == *y) {
                         tryagain=1;
                         break;
                    }
               }
               i=j;
          }
     } while (tryagain);
}

void
initplayer(short p)
{
     short i,s;
     long a,x,y,z;

     if (numplayers > 1) {
          pickrandomspot(&a,&x,&y,&z,&s);
          spawnflash(x,y,z,s);
     }
     else {
          a=starta;
          x=startx;
          y=starty;
          z=startz;
          s=starts;
     }
     spawnsprite(plrsprite[p],x,y,z,1+256,0,p,32,64,64,0,0,DOOMGUYPIC,
          (short)a,0,0,0,p+MAXSPRITES,s,8,0,0,0);
     memset(sprdta[plrsprite[p]],0,sizeof(struct spritedata));
     for (i=0 ; i < MAXWEAPONS ; i++) {
          plrammo[p][i]=0;
          plrammomax[p][i]=defammomax[i];
          if (p == pyrn) {
               weaptype[i].selectable=0;
          }
          weapflags[p][i]=0;
     }
     changeammo(p,50,G_GATLING);
     weaptype[G_GATLING].selectable=1;
     weap[p]=G_GATLING;
     weapflags[p][G_GATLING]=1;
     sprdta[plrsprite[p]]->health=0;
     changehealth(plrsprite[p],MAXHEALTH);
     sprdta[plrsprite[p]]->type=0;
     sprdta[plrsprite[p]]->height=40<<8;
     sprdta[plrsprite[p]]->z=sprptr[plrsprite[p]]->z;
     sprdta[plrsprite[p]]->horiz=CTRHORIZ;
     plrscreen[p]=3;
     plrzoom[p]=768L;
     plrdead[p]=0;
     visibility=defvisibility;
}

void
initplayers(short n)
{
     short i,opyr;

     pyrn=curplyr=curview=myconnectindex;
     if (option[4] != 0) {
          sendlogon();
          while (numplayers < n) {
               getpacket(&opyr,tempbuf);
               showmessage("%d OF %d PLAYERS READY!",numplayers,npyrs);
               overwritesprite(0,0,TITLEPIC,0,0,0);
               drawscreenfx();
               nextpage();
               if (keystatus[0x01] != 0) {
                    sendlogoff();
                    crash("Multiplayer game aborted!");
               }
          }
          pyrn=curview=curplyr=myconnectindex;
     }
     for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
          initplayer(i);
     }
}

void
killplayer(short p)
{
     short killedby,s;

     plrdead[p]=1;
     s=plrsprite[p];
     if (sprdta[s]->health < -25) {
          changespritestat(s,DIE2STAT);
          wsayfollow("dsslop.wav",4096L+(krand()&255)-128,256L,
               &sprptr[s]->x,&sprptr[s]->y,0);
          sprptr[s]->extra=0;
     }
     else {
          changespritestat(s,DIESTAT);
          wsayfollow("dspldeth.wav",4096L+(krand()&255)-128,256L,
               &sprptr[s]->x,&sprptr[s]->y,0);
          sprptr[s]->extra=0;
     }
     sprptr[s]->cstat=0;
     if ((killedby=sprptr[s]->hitag) >= MAXSPRITES) {
          killedby-=MAXSPRITES;
          if (killedby == p) {
               plrfrags[killedby]--;
          }
          else {
               plrfrags[killedby]++;
          }
          drawstatusbar(killedby);
     }
}

void
newsector(short s)
{
}

//wango
void
dophysics(short s,long goalz,long *xvect,long *yvect)
{
     long xv,yv;
     struct spritedata *spr;

     spr=sprdta[s];
     if (spr->z < goalz) {
          spr->hvel+=GRAVITYCONSTANT;
          spr->onsomething&=~(GROUNDBIT|PLATFORMBIT);
          spr->fallz+=spr->hvel;
     }
     else if (spr->z > goalz) {
          spr->hvel-=((spr->z-goalz)>>6);
          spr->onsomething|=GROUNDBIT;
     }
     else {
          spr->fallz=0L;
     }
     spr->z+=spr->hvel;
     if (spr->hvel > 0 && spr->z > goalz) {
          spr->hvel>>=2;
     }
     else if (spr->onsomething != 0) {
          if (spr->hvel < 0 && spr->z < goalz) {
               spr->hvel=0;
               spr->z=goalz;
          }
     }
     if (spr->z < globhiz) {
          spr->z=globhiz+(spr->height>>4);
          spr->hvel=0;
     }
     else if (spr->z > globloz) {
          spr->z=globloz-(spr->height>>4);
          spr->hvel=0;
     }
     if (spr->forcev != 0) {
          *xvect=(long)((spr->forcev*(long)sintable[(spr->forcea+512)&2047])>>3);
          *yvect=(long)((spr->forcev*(long)sintable[spr->forcea])>>3);
          spr->forcev>>=1;
     }
}

void
operatedoor(short sectnum)
{
     short d;

     d=doorsect[sectnum];
     if (d == -1 || d >= numdoors) {
          crash("OPERATEDOOR: Invalid door number (%d)",d);
     }
     switch (doorptr[d]->state) {
     case D_OPENDOOR:
     case D_OPENING:
     case D_OPENSOUND:
     case D_WAITING:
          doorptr[d]->state=D_CLOSEDOOR;
          break;
     default:
          doorptr[d]->state=D_OPENDOOR;
          break;
     }
}

void
operatesector(short sectnum)
{
     switch (sectptr[sectnum]->lotag) {
     case DOORUPTAG:
     case PLATFORMELEVTAG:
     case PLATFORMDELAYTAG:
     case PLATFORMDROPTAG:
          operatedoor(sectnum);
          break;
     }
}

void
operatewall(short wallnum)
{
}

void
operatesprite(short spritenum)
{
}

void
plruse(short s)
{
     short nearsector,nearwall,nearsprite;
     long neardist;
     struct spritetype *spr;

     spr=sprptr[s];
     neartag(spr->x,spr->y,sprdta[s]->z,spr->sectnum,spr->ang,
          &nearsector,&nearwall,&nearsprite,&neardist,1024L);
     showmessage("S: %05d W: %05d SP: %05d",nearsector,nearwall,nearsprite);
     if (nearsector >= 0) {
          operatesector(nearsector);
     }
     if (nearwall >= 0) {
          operatewall(nearwall);
     }
     if (nearsprite >= 0) {
          operatesprite(nearsprite);
     }
}

void
processinput(short p)
{
     short moving=0,onground,ptype,s,strafing=0,turning=0;
     long goalz,ps,px,py,pz,sv,v,xvect,yvect;

     s=plrsprite[p];
     if (plrdead[p] == 0) {
          if (sprdta[s]->health <= 0) {
               killplayer(p);
          }
          switch (sprptr[s]->picnum) {
          case DOOMGUYSHOOT2:
          case DOOMGUYPAIN:
               if (sprptr[s]->extra > 0) {
                    sprptr[s]->extra-=TICSPERFRAME;
                    if (sprptr[s]->extra <= 0) {
                         sprptr[s]->extra=0;
                         sprptr[s]->picnum=DOOMGUYPIC;
                    }
               }
               break;
          }
     }
     else {
          mvel[p]=0;
          svel[p]=0;
          tvel[p]=0;
          plrdead[p]+=(TICSPERFRAME<<7);
          if (plrdead[p] > sprdta[s]->height-(8<<8)) {
               plrdead[p]=sprdta[s]->height-(8<<8);
          }
          v=sprptr[s]->hitag-MAXSPRITES;
          if (v >= 0 && v < MAXPLAYERS) {
               v=plrsprite[v];
               sprptr[s]->ang=getangle(sprptr[v]->x-sprptr[s]->x,
                    sprptr[v]->y-sprptr[s]->y);
          }
          goto playerdead;
     }
     ptype=sprdta[s]->type;
     if (plrmove[p]&(1<<MOVEFWD)) {
          if (mvel[p] < maxmvel[ptype]) {
               mvel[p]+=(TICSPERFRAME<<2);
               if (mvel[p] > maxmvel[ptype]) {
                    mvel[p]=maxmvel[ptype];
               }
          }
          moving=1;
     }
     else if (plrmove[p]&(1<<MOVEBACK)) {
          if (mvel[p] > minmvel[ptype]) {
               mvel[p]-=(TICSPERFRAME<<2);
               if (mvel[p] < minmvel[ptype]) {
                    mvel[p]=minmvel[ptype];
               }
          }
          moving=1;
     }
     if (moving == 0 && mvel[p] != 0) {
          if (mvel[p] > 0) {
               mvel[p]-=(TICSPERFRAME<<1);
               if (mvel[p] < 0) {
                    mvel[p]=0;
               }
          }
          else {
               mvel[p]+=(TICSPERFRAME<<1);
               if (mvel[p] > 0) {
                    mvel[p]=0;
               }
          }
     }
     if (moving == 0 && plrmousy[p] != 0) {
          mvel[p]=-(plrmousy[p]<<2);
     }
     if (plrmove[p]&(1<<MOVELEFT)) {
          if (plrmove[p]&(1<<MOVESTRAFE)) {
               if (svel[p] < maxsvel[ptype]) {
                    svel[p]+=(TICSPERFRAME<<3);
                    if (svel[p] > maxsvel[ptype]) {
                         svel[p]=maxsvel[ptype];
                    }
               }
               strafing=1;
          }
          else if (tvel[p] > mintvel[ptype]) {
               tvel[p]-=TICSPERFRAME;
               if (tvel[p] < mintvel[ptype]) {
                    tvel[p]=mintvel[ptype];
               }
               turning=1;
          }
     }
     else if (plrmove[p]&(1<<MOVERIGHT)) {
          if (plrmove[p]&(1<<MOVESTRAFE)) {
               if (svel[p] > minsvel[ptype]) {
                    svel[p]-=(TICSPERFRAME<<3);
                    if (svel[p] < minsvel[ptype]) {
                         svel[p]=minsvel[ptype];
                    }
               }
               strafing=1;
          }
          else if (tvel[p] < maxmvel[ptype]) {
               tvel[p]+=TICSPERFRAME;
               if (tvel[p] > maxtvel[ptype]) {
                    tvel[p]=maxtvel[ptype];
               }
               turning=1;
          }
     }
     if (turning == 0 && tvel[p] != 0) {
          if (tvel[p] < 0) {
               tvel[p]+=TICSPERFRAME;
               if (tvel[p] > 0) {
                    tvel[p]=0;
               }
          }
          else if (tvel[p] > 0) {
               tvel[p]-=TICSPERFRAME;
               if (tvel[p] < 0) {
                    tvel[p]=0;
               }
          }
     }
     if (plrmove[p]&(1<<MOVESTRAFEL)) {
          if (svel[p] < maxsvel[ptype]) {
               svel[p]+=(TICSPERFRAME<<3);
               if (svel[p] > maxsvel[ptype]) {
                    svel[p]=maxsvel[ptype];
               }
          }
          strafing=1;
     }
     else if (plrmove[p]&(1<<MOVESTRAFER)) {
          if (svel[p] > minsvel[ptype]) {
               svel[p]-=(TICSPERFRAME<<3);
               if (svel[p] < minsvel[ptype]) {
                    svel[p]=minsvel[ptype];
               }
          }
          strafing=1;
     }
     if (strafing == 0 && svel[p] != 0) {
          if (svel[p] < 0) {
               svel[p]+=(TICSPERFRAME<<3);
               if (svel[p] > 0) {
                    svel[p]=0;
               }
          }
          else if (svel[p] > 0) {
               svel[p]-=(TICSPERFRAME<<3);
               if (svel[p] < 0) {
                    svel[p]=0;
               }
          }
     }
     if (turning == 0 && plrmousx[p] != 0) {
          if (plrmove[p]&(1<<MOVESTRAFE)) {
               svel[p]=-(plrmousx[p]<<2);
          }
          else {
               tvel[p]=plrmousx[p]>>2;
          }
     }
     if ((plrmove[p]&(1<<MOVESHOOT)) && !(plrflags[p]&(1<<MOVESHOOT))) {
          if (plrammo[p][weap[p]] > 0) {
               plrflags[p]|=(1<<MOVESHOOT);
               weapclock[p]=lockclock+weaptype[weap[p]].delay;
               weapstep[p]=0;
          }
     }
     if (plrmove[p]&(1<<MOVELOOKUP)) {
          if (sprdta[s]->horiz < MAXHORIZ) {
               sprdta[s]->horiz+=STEPHORIZ;
               if (sprdta[s]->horiz > MAXHORIZ) {
                    sprdta[s]->horiz=MAXHORIZ;
               }
          }
          plrflags[p]|=(1<<MOVELOOKUP);
     }
     else if (plrmove[p]&(1<<MOVELOOKDN)) {
          if (sprdta[s]->horiz > MINHORIZ) {
               sprdta[s]->horiz-=STEPHORIZ;
               if (sprdta[s]->horiz < MINHORIZ) {
                    sprdta[s]->horiz=MINHORIZ;
               }
          }
          plrflags[p]|=(1<<MOVELOOKDN);
     }
     else if (plrmove[p]&(1<<MOVECENTER)) {
          plrflags[p]|=(1<<MOVECENTER);
     }
     if (plrflags[p]&(1<<MOVECENTER)) {
          if (sprdta[s]->horiz < CTRHORIZ) {
               sprdta[s]->horiz+=STEPHORIZ;
               if (sprdta[s]->horiz >= CTRHORIZ) {
                    sprdta[s]->horiz=CTRHORIZ;
                    plrflags[p]&=~((1<<MOVELOOKUP)|(1<<MOVELOOKDN));
                    plrflags[p]&=~(1<<MOVECENTER);
               }
          }
          else if (sprdta[s]->horiz > CTRHORIZ) {
               sprdta[s]->horiz-=STEPHORIZ;
               if (sprdta[s]->horiz <= CTRHORIZ) {
                    sprdta[s]->horiz=CTRHORIZ;
                    plrflags[p]&=~((1<<MOVELOOKUP)|(1<<MOVELOOKDN));
                    plrflags[p]&=~(1<<MOVECENTER);
               }
          }
     }
playerdead:
     if ((plrmove[p]&(1<<MOVEUSE)) && !(oplrmove[p]&(1<<MOVEUSE))) {
          if (plrdead[p] == 0) {
               plruse(s);
          }
          else {
               initplayer(p);
          }
     }
     if (plrmove[p]&(1<<MOVEZOOMI) && !(oplrmove[p]&(1<<MOVEZOOMI))) {
          switch (plrscreen[p]) {
          case 2:                  // 2D zoom in mode
               if (plrzoom[p] < 4096L) {
                    plrzoom[p]+=(plrzoom[p]>>4);
                    if (plrzoom[p] > 4096L) {
                         plrzoom[p]=4096L;
                    }
               }
               break;
          case 3:                  // 3D screen size enlarge
               if (p == curplyr && screensize <= xdim) {
                    screensize+=4;
                    if (screensize >= xdim+4) {
                         screensize=xdim+4;
                    }
                    setup3dscreen();
               }
               break;
          }
     }
     else if (plrmove[p]&(1<<MOVEZOOMO) && !(oplrmove[p]&(1<<MOVEZOOMO))) {
          switch (plrscreen[p]) {
          case 2:                  // 2D zoom out mode
               if (plrzoom[p] > 48L) {
                    plrzoom[p]-=(plrzoom[p]>>4);
                    if (plrzoom[p] < 48L) {
                         plrzoom[p]=48L;
                    }
               }
               break;
          case 3:                  // 3D screen size shrink
               if (p == curplyr && screensize > 128) {
                    screensize-=4;
                    if (screensize <= 128) {
                         screensize=128;
                    }
                    setup3dscreen();
               }
               break;
          }
     }
     if ((plrmove[p]&(1<<MOVEMAP)) && !(oplrmove[p]&(1<<MOVEMAP))) {
          if (plrscreen[p] == 3) {
               plrscreen[p]=2;
          }
          else {
               plrscreen[p]=3;
          }
     }
     if (tvel[p] != 0) {
          sprptr[s]->ang=(sprptr[s]->ang+tvel[p])&2047;
     }
     ps=sprptr[s]->sectnum;
     px=sprptr[s]->x;
     py=sprptr[s]->y;
     pz=sprdta[s]->z;
     xvect=yvect=0L;
     v=sprdta[s]->forcev*TICSPERFRAME;
     if (v != 0) {
          xvect=(long)((v*(long)sintable[(sprdta[s]->forcea+512)&2047])>>3);
          yvect=(long)((v*(long)sintable[sprdta[s]->forcea])>>3);
     }
     if (mvel[p] != 0 || svel[p] != 0) {
          v=mvel[p]*TICSPERFRAME;
          if (plrmove[p]&(1<<MOVERUN)) {
               v=(v<<1)+(v>>2);
          }
          xvect+=(long)((v*(long)sintable[(sprptr[s]->ang+512)&2047])>>3);
          yvect+=(long)((v*(long)sintable[sprptr[s]->ang])>>3);
          if (svel[p] != 0) {
               sv=svel[p]*TICSPERFRAME;
               xvect+=(long)((sv*(long)sintable[sprptr[s]->ang])>>3);
               yvect+=(long)((sv*(long)sintable[(sprptr[s]->ang-512)&2047])>>3);
          }
     }
     if (xvect != 0L || yvect != 0L) {
          clipmove(&px,&py,&pz,&ps,xvect,yvect,128L,4<<8,4<<8,0);
     }
     if (plrdead[p] == 0) {
          sprptr[s]->cstat^=1;
          getzrange(px,py,pz,ps,&globhiz,&globhihit,&globloz,&globlohit,128L,0);
          sprptr[s]->cstat^=1;
          if (ps != sprdta[s]->oldsectnum) {
               newsector(s);
          }
     }
     else {
          globloz=sectptr[ps]->floorz;
          globhiz=sectptr[ps]->ceilingz;
     }
     goalz=globloz-sprdta[s]->height;
     if (plrdead[p] == 0) {
          if (plrmove[p]&(1<<MOVEJUMP) && !(oplrmove[p]&(1<<MOVEJUMP))) {
               if (sprdta[s]->onsomething) {
                    sprdta[s]->hvel-=JUMPVEL;
                    sprdta[s]->onsomething=0;
               }
          }
          else if (plrmove[p]&(1<<MOVECROUCH)) {
               goalz+=sprdta[s]->height>>1;
          }
          if (goalz < globhiz) {
               goalz=globhiz+(8<<8);
          }
     }
     onground=sprdta[s]->onsomething;
     dophysics(s,goalz,&xvect,&yvect);
     if (!onground && sprdta[s]->onsomething) {
          if (sprdta[s]->fallz > 32768L) {
               wsayfollow("dsplpain.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               changehealth(s,-(sprdta[s]->fallz>>12));
          }
          else if (sprdta[s]->fallz > 8192L) {
               wsayfollow("dsoof.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
          }
     }
     if ((plrflags[p]&((1<<MOVELOOKUP)|(1<<MOVELOOKDN)|(1<<MOVECENTER))) == 0) {
          sprdta[s]->horiz=100-(sprdta[s]->hvel>>6);
     }
     setsprite(s,px,py,sprdta[s]->z+sprdta[s]->height);
     sprdta[s]->oldsectnum=sprptr[s]->sectnum;
     oplrmove[p]=plrmove[p];
}

void
processtouch(short p)
{
     short i,j,k,s;

     s=plrsprite[p];
     i=headspritesect[sprptr[s]->sectnum];
     while (i != -1) {
          j=nextspritesect[i];
          if (getspritedist(s,i) < 512L) {
               switch (sprptr[i]->picnum) {
               case GUN3ONGROUND:
                    if (sprptr[i]->hitag != 0 && weapflags[p][G_SHOTGUN] != 0) {
                         break;
                    }
                    weapflags[p][G_SHOTGUN]=1;
                    if (changeammo(p,4,G_SHOTGUN)) {
                         weaptype[G_SHOTGUN].selectable=1;
                         wsayfollow("dssgcock.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         if (sprptr[i]->hitag == 0) {
                              deletesprite(i);
                         }
                    }
                    break;
               case GUN4ONGROUND:
                    if (sprptr[i]->hitag != 0 && weapflags[p][G_GATLING] != 0) {
                         break;
                    }
                    weapflags[p][G_GATLING]=1;
                    if (changeammo(p,25,G_GATLING)) {
                         weaptype[G_GATLING].selectable=1;
                         wsayfollow("dssgcock.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         if (sprptr[i]->hitag == 0) {
                              deletesprite(i);
                         }
                    }
                    break;
               case GUN5ONGROUND:
                    if (sprptr[i]->hitag != 0 && weapflags[p][G_ROCKET] != 0) {
                         break;
                    }
                    weapflags[p][G_ROCKET]=1;
                    if (changeammo(p,4,G_ROCKET)) {
                         weaptype[G_ROCKET].selectable=1;
                         wsayfollow("dssgcock.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         if (sprptr[i]->hitag == 0) {
                              deletesprite(i);
                         }
                    }
                    break;
               case GUN6ONGROUND:
                    if (sprptr[i]->hitag != 0 && weapflags[p][G_PLASMA] != 0) {
                         break;
                    }
                    weapflags[p][G_PLASMA]=1;
                    if (changeammo(p,50,G_PLASMA)) {
                         weaptype[G_PLASMA].selectable=1;
                         wsayfollow("dssgcock.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         if (sprptr[i]->hitag == 0) {
                              deletesprite(i);
                         }
                    }
                    break;
               case ROCKETBOXPIC:
                    if (changeammo(p,10,G_ROCKET)) {
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case ROCKETSHELLPIC:
                    if (changeammo(p,1,G_ROCKET)) {
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case AMMOBOXPIC:
                    if (changeammo(p,50,G_GATLING)) {
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case SHELL4PIC:
                    if (changeammo(p,4,G_SHOTGUN)) {
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case SHELLBOXPIC:
                    if (changeammo(p,16,G_SHOTGUN)) {
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case POWERCELLPIC:
                    if (changeammo(p,25,G_PLASMA)) {
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case POWERUNITPIC:
                    if (changeammo(p,100,G_PLASMA)) {
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case MEDICBOXPIC:
                    if (sprdta[s]->health < MAXHEALTH) {
                         changehealth(s,25);
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case AUTOMAPPERPIC:
                    if (p == pyrn) {
                         for (k=0 ; k < (MAXSECTORS>>3) ; k++) {
                              show2dsector[k]=0xFF;
                         }
                         for (k=0 ; k < (MAXWALLS>>3) ; k++) {
                              show2dwall[k]=0xFF;
                         }
                    }
                    wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,256L,
                         &sprptr[s]->x,&sprptr[s]->y,0);
                    deletesprite(i);
                    break;
               case BERZERKPIC:
                    if (sprdta[s]->health < MAXHEALTH) {
                         sprdta[s]->health=MAXHEALTH;
                    }
                    wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,256L,
                         &sprptr[s]->x,&sprptr[s]->y,0);
                    deletesprite(i);
                    break;
               case NIGHTVISORPIC:
                    if (p == pyrn) {
                         visibility=128;
                    }
                    wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,256L,
                         &sprptr[s]->x,&sprptr[s]->y,0);
                    deletesprite(i);
                    break;
               case LOSTSOULPIC:
                    if (sprdta[s]->health < (MAXHEALTH<<1)) {
                         changehealth(s,MAXHEALTH<<1);
                    }
                    wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,256L,
                         &sprptr[s]->x,&sprptr[s]->y,0);
                    deletesprite(i);
                    break;
               case STIMPACKPIC:
                    if (sprdta[s]->health < MAXHEALTH) {
                         changehealth(s,10);
                         wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,
                              256L,&sprptr[s]->x,&sprptr[s]->y,0);
                         deletesprite(i);
                    }
                    break;
               case REDKEYPIC:
               case BLUEKEYPIC:
               case YELLOWKEYPIC:
               case RADIATIONSUITPIC:
               case GREENARMORPIC:
               case BLUEARMORPIC:
               case BACKPACKPIC:
                    wsayfollow("dsitemup.wav",4096L+(krand()&255)-128,256L,
                         &sprptr[s]->x,&sprptr[s]->y,0);
                    deletesprite(i);
                    break;
               }
          }
          i=j;
     }
}

void
spawnprojectile(short s,short gun)
{
     short pic,stat;
     long j,x,xvect,y,yvect,z;

     switch (gun) {
     case G_ROCKET:
          pic=GUN5PROJECTILE;
          xvect=sintable[(sprptr[s]->ang+512)&2047]>>5;
          yvect=sintable[sprptr[s]->ang]>>5;
          break;
     case G_PLASMA:
          pic=GUN6PROJECTILE;
          xvect=sintable[(sprptr[s]->ang+512)&2047]>>4;
          yvect=sintable[sprptr[s]->ang]>>4;
          break;
     }
     x=sprptr[s]->x; //+xvect;
     y=sprptr[s]->y; //+yvect;
     z=sprptr[s]->z-(sprdta[s]->height>>1);
     spawnsprite(j,x,y,z,1+256,0,0,16,64,64,0,0,pic,sprptr[s]->ang,
          xvect,yvect,(100-sprdta[s]->horiz)<<6,
          sprptr[s]->owner,sprptr[s]->sectnum,MOVEPROJECTILESTAT,0,0,0);
     sprdta[j]->type=gun;
}

void
spawnblood(long x,long y,long z,short sect)
{
     short i;

     spawnsprite(i,x,y,z,2,0,0,16,64,64,0,0,BLOODPIC,0,
          0L,0L,0L,0,sect,SPLASHSTAT,0,0,0);
     sprdta[i]->aclock=lockclock+SPARKCLOCK;
}

void
spawnspark(long x,long y,long z,short sect)
{
     short i;

     spawnsprite(i,x,y,z,2,0,0,16,64,64,0,0,SPARKSTART,0,
          0L,0L,0L,0,sect,SMOKEWALLSTAT,0,0,0);
     sprdta[i]->aclock=lockclock+SPARKCLOCK;
}

void
spawnflash(long x,long y,long z,short sect)
{
     short i;

     spawnsprite(i,x,y,z,0,0,0,16,32,32,0,0,FLASHPIC,0,
          0L,0L,0L,0,sect,FLASHSTAT,0,0,0);
     sprdta[i]->aclock=lockclock+SPARKCLOCK;
     wsayfollow("dstelept.wav",4096L+(krand()&255)-128,256L,&x,&y,0);
}

void
showpainframe(short s)
{
     switch (sprptr[s]->picnum) {
     case DOOMGUYPIC:
     case DOOMGUYSHOOT:
     case DOOMGUYSHOOT2:
          sprptr[s]->picnum=DOOMGUYPAIN;
          sprptr[s]->extra=TICSPERFRAME<<3;
          break;
     }
}

void
hitscangun(short s,short gun)
{
     short ang,hitsect,hitsprite,hitwall,reps=1;
     long daz,hitx,hity,hitz,x,y,z;

     switch (gun) {
     case G_SHOTGUN:
          reps=4;
     case G_GATLING:
          do {
               ang=(sprptr[s]->ang+2048+(krand()&31)-16)&2047;
               x=sprptr[s]->x;
               y=sprptr[s]->y;
               z=sprdta[s]->z;
               daz=((100-sprdta[s]->horiz)*2000)+((krand()-32768)>>1);
               hitscan(x,y,z,sprptr[s]->sectnum,sintable[(ang+512)&2047],
                    sintable[ang],daz,&hitsect,&hitwall,&hitsprite,
                    &hitx,&hity,&hitz);
               if (hitsprite >= 0 && sprptr[hitsprite]->statnum < MAXSTATUS) {
                    switch (sprptr[hitsprite]->picnum) {
                    case DOOMGUYPIC:
                    case DOOMGUYSHOOT:
                    case DOOMGUYSHOOT2:
                    case DOOMGUYPAIN:
                         sprptr[hitsprite]->hitag=sprptr[s]->owner;
                         changehealth(hitsprite,-((krand()&0x07)+2));
                         spawnblood(hitx,hity,hitz,hitsect);
                         showpainframe(hitsprite);
                         break;
                    }
                    sprdta[hitsprite]->forcev=512L-(krand()&0x1FF);
                    sprdta[hitsprite]->forcea=getspriteangle(hitsprite,s);
               }
               else {
                    spawnspark(hitx,hity,hitz,hitsect);
               }
          } while (--reps > 0);
          break;
     }
}

void
shootgun(short p)
{
     short gun,s;
     long x,y;

     s=plrsprite[p];
     gun=weap[p];
     x=sprptr[s]->x;
     y=sprptr[s]->y;
     switch (gun) {
     case G_SHOTGUN:               // shot gun
          changeammo(p,-1,weap[p]);
          hitscangun(s,gun);
          wsayfollow("dsshotgn.wav",4096L+(krand()&255)-128,256L,&x,&y,0);
          break;
     case G_GATLING:               // chain gun
          changeammo(p,-1,weap[p]);
          hitscangun(s,gun);
          wsayfollow("dspistol.wav",4096L+(krand()&255)-128,256L,&x,&y,0);
          break;
     case G_ROCKET:                // rocket launcher
          changeammo(p,-1,weap[p]);
          spawnprojectile(s,gun);
          wsayfollow("dsrlaunc.wav",4096L+(krand()&255)-128,256L,&x,&y,0);
          break;
     case G_PLASMA:                // plasma gun
          changeammo(p,-1,weap[p]);
          spawnprojectile(s,gun);
          wsayfollow("dsplasma.wav",4096L+(krand()&255)-128,256L,&x,&y,0);
          break;
     }
}

void
showshootframe(short s)
{
     switch (sprptr[s]->picnum) {
     case DOOMGUYPIC:
     case DOOMGUYSHOOT:
     case DOOMGUYPAIN:
          sprptr[s]->picnum=DOOMGUYSHOOT2;
          sprptr[s]->extra=TICSPERFRAME<<1;
          break;
     }
}

void
processweaps(short p)
{
     short endpic,s,startpic;
     struct weaptype *weapptr;

     if (plrflags[p]&(1<<MOVESHOOT)) {
          if (plrdead[p]) {
               plrflags[p]&=~(1<<MOVESHOOT);
               return;
          }
          s=plrsprite[p];
          if (plrammo[p][weap[p]] <= 0) {
               if (p == curplyr) {
                    showmessage("OUT OF AMMO");
               }
               weapstep[p]=0;
               plrflags[p]&=~(1<<MOVESHOOT);
               sprptr[s]->picnum=DOOMGUYPIC;
               return;
          }
          if (sprptr[s]->extra == 0) {
               sprptr[s]->picnum=DOOMGUYSHOOT;
          }
          if (lockclock < weapclock[p]) {
               return;
          }
          weapptr=&weaptype[weap[p]];
          weapclock[p]=lockclock+weapptr->delay;
          if (weapptr->action[weapstep[p]]) {
               showshootframe(s);
               shootgun(p);
          }
          weapstep[p]++;
          startpic=weapptr->firestartpic;
          endpic=weapptr->fireendpic;
          if (weapstep[p] > endpic-startpic) {
               weapstep[p]=0;
               if (plrmove[p]&(1<<MOVESHOOT)) {
                    weapclock[p]+=weapptr->reloaddelay;
                    return;
               }
               plrflags[p]&=~(1<<MOVESHOOT);
               sprptr[s]->picnum=DOOMGUYPIC;
          }
     }
}

long
getspritedist(short s1,short s2)
{
     long dist;

     dist=labs(sprptr[s1]->x-sprptr[s2]->x)+labs(sprptr[s1]->y-sprptr[s2]->y);
     return(dist);
}

short
getspriteangle(short s1,short s2)
{
     short ang;

     ang=getangle(sprptr[s1]->x-sprptr[s2]->x,sprptr[s1]->y-sprptr[s2]->y);
     return(ang);
}

void
checknearexplosion(short i,short s)
{
     long dax;

     if ((dax=getspritedist(i,s)) < EXPLOSIONDIST) {
          if (cansee(sprptr[i]->x,sprptr[i]->y,sprptr[i]->z,sprptr[i]->sectnum,
             sprptr[s]->x,sprptr[s]->y,sprdta[s]->z,sprptr[s]->sectnum)) {
               sprdta[s]->forcea=getspriteangle(s,i);
               sprdta[s]->forcev=(short)EXPLOSIONDIST-dax-(krand()&0x7F);
               if (sprdta[s]->forcev < 0) {
                    sprdta[s]->forcev=0;
               }
               else {
                    changehealth(s,-(sprdta[s]->forcev>>4));
                    sprptr[s]->hitag=sprptr[i]->owner;
                    showpainframe(s);
               }
          }
     }
}

void
processstats(void)
{
     short endpic,i,j,k,s,startpic;
     unsigned short hit;
     long dax,day,daz;

     i=headspritestat[ACTIVESTAT];
     while (i != -1) {
          j=nextspritestat[i];
          dax=day=0L;
          globloz=sectptr[sprptr[i]->sectnum]->floorz;
          globhiz=sectptr[sprptr[i]->sectnum]->ceilingz;
          dophysics(i,globloz,&dax,&day);
          daz=sprdta[i]->z-sprptr[i]->z;
          movesprite(i,dax,day,daz,4L<<8,4L<<8,1);
          i=j;
     }
     i=headspritestat[SPLASHSTAT];
     while (i != -1) {
          j=nextspritestat[i];
          if (lockclock >= sprdta[i]->aclock) {
               sprdta[i]->aclock=lockclock+SPARKCLOCK;
               sprptr[i]->extra++;
               if (sprptr[i]->extra > BLOODENDPIC-BLOODPIC) {
                    deletesprite(i);
               }
               sprptr[i]->picnum=BLOODPIC+sprptr[i]->extra;
          }
          sprptr[i]->z+=(TICSPERFRAME<<2);
          i=j;
     }
     i=headspritestat[SMOKEWALLSTAT];
     while (i != -1) {
          j=nextspritestat[i];
          if (lockclock >= sprdta[i]->aclock) {
               sprdta[i]->aclock=lockclock+SPARKCLOCK;
               sprptr[i]->extra++;
               if (sprptr[i]->extra > SPARKEND-SPARKSTART) {
                    deletesprite(i);
               }
               sprptr[i]->picnum=SPARKSTART+sprptr[i]->extra;
          }
          sprptr[i]->z-=TICSPERFRAME;
          i=j;
     }
     i=headspritestat[DIESTAT];
     while (i != -1) {
          j=nextspritestat[i];
          dax=day=0L;
          globloz=sectptr[sprptr[i]->sectnum]->floorz;
          globhiz=sectptr[sprptr[i]->sectnum]->ceilingz;
          daz=globloz-sprdta[i]->height;
          dophysics(i,daz,&dax,&day);
          movesprite(i,dax,day,0L,4L<<8,4L<<8,1);
          if (lockclock >= sprdta[i]->aclock) {
               sprdta[i]->aclock=lockclock+DIECLOCK;
               sprptr[i]->extra++;
               if (sprptr[i]->extra > DOOMGUYDIEEND-DOOMGUYDIE) {
                    changespritestat(i,INACTIVESTAT);
               }
               else {
                    sprptr[i]->picnum=DOOMGUYDIE+sprptr[i]->extra;
               }
          }
          i=j;
     }
     i=headspritestat[DIE2STAT];
     while (i != -1) {
          j=nextspritestat[i];
          dax=day=0L;
          globloz=sectptr[sprptr[i]->sectnum]->floorz;
          globhiz=sectptr[sprptr[i]->sectnum]->ceilingz;
          daz=globloz-sprdta[i]->height;
          dophysics(i,daz,&dax,&day);
          movesprite(i,dax,day,0L,4L<<8,4L<<8,1);
          if (lockclock >= sprdta[i]->aclock) {
               sprdta[i]->aclock=lockclock+DIECLOCK;
               sprptr[i]->extra++;
               if (sprptr[i]->extra > DOOMGUYDIE2END-DOOMGUYDIE2) {
                    changespritestat(i,INACTIVESTAT);
               }
               else {
                    sprptr[i]->picnum=DOOMGUYDIE2+sprptr[i]->extra;
               }
          }
          i=j;
     }
     i=headspritestat[FLASHSTAT];
     while (i != -1) {
          j=nextspritestat[i];
          if (lockclock >= sprdta[i]->aclock) {
               sprdta[i]->aclock=lockclock+FLASHCLOCK;
               sprptr[i]->extra++;
               if (sprptr[i]->extra > FLASHEND-FLASHPIC) {
                    deletesprite(i);
               }
               else {
                    sprptr[i]->picnum=FLASHPIC+sprptr[i]->extra;
               }
          }
          i=j;
     }
     i=headspritestat[MOVEPROJECTILESTAT];
     while (i != -1) {
          j=nextspritestat[i];
          dax=(((long)sprptr[i]->xvel)*TICSPERFRAME)<<11;
          day=(((long)sprptr[i]->yvel)*TICSPERFRAME)<<11;
          daz=(((long)sprptr[i]->zvel)*TICSPERFRAME)>>3;
          hit=movesprite(i,dax,day,daz,4L<<8,4L<<8,1);
          if (hit != 0) {
               changespritestat(i,EXPLOSIONSTAT);
               sprdta[i]->misc=hit;
          }
          i=j;
     }
     i=headspritestat[EXPLOSIONSTAT];
     while (i != -1) {
          j=nextspritestat[i];
          switch (sprdta[i]->type) {
          case G_ROCKET:
               startpic=EXPLOSIONSTART;
               endpic=EXPLOSIONEND;
               break;
          case G_PLASMA:
               startpic=PLASMAEXPLODEPIC;
               endpic=PLASMAEXPLODEEND;
               break;
          }
          if (sprptr[i]->extra == 0) {
               switch (sprdta[i]->type) {
               case G_ROCKET:
                    wsayfollow("dsbarexp.wav",4096L+(krand()&255)-128,256L,
                         &dax,&day,0);
                    break;
               }
               dax=sprptr[i]->x;
               day=sprptr[i]->y;
               spawnsprite(k,dax,day,sprptr[i]->z,
                    128,0,0,16,64,64,0,0,startpic,0,0,0,0,
                    sprptr[i]->owner,sprptr[i]->sectnum,EXPLOSIONSTAT,0,0,1);
               sprdta[k]->aclock=lockclock+EXPDELAYRATE;
               sprdta[k]->misc=sprdta[i]->misc;        // object that was hit
               sprdta[k]->type=sprdta[i]->type;        // type of projectile
               deletesprite(i);
          }
          else {
               if (lockclock < sprdta[i]->aclock) {
                    i=j;
                    continue;
               }
               sprdta[i]->aclock=lockclock+EXPDELAYRATE;
               if (sprptr[i]->extra == 1) {
                    switch (sprdta[i]->type) {
                    case G_ROCKET:
                         s=headspritestat[ACTIVESTAT];
                         while (s != -1) {
                              k=nextspritestat[s];
                              sprdta[s]->z=sprptr[s]->z;
                              checknearexplosion(i,s);
                              s=k;
                         }
                         s=headspritestat[PLAYERSTAT];
                         while (s != -1) {
                              k=nextspritestat[s];
                              checknearexplosion(i,s);
                              s=k;
                         }
                         break;
                    case G_PLASMA:
                         if (sprdta[i]->misc >= 49152) {
                              hit=sprdta[i]->misc-49152;
                              changehealth(hit,-((krand()&0x0F)+10));
                              sprptr[hit]->hitag=sprptr[i]->owner;
                         }
                         break;
                    }
               }
               sprptr[i]->extra++;
               if (sprptr[i]->extra > endpic-startpic) {
                    deletesprite(i);
               }
               else {
                    sprptr[i]->picnum=startpic+sprptr[i]->extra;
               }
          }
          i=j;
     }
}

void
setsectorlight(short s,short brightness)
{
     short w,wallend,wallstart;

     wallstart=sectptr[s]->wallptr;
     wallend=wallstart+sectptr[s]->wallnum;
     sectptr[s]->floorshade=brightness;
     for (w=wallstart ; w < wallend ; w++) {
          wallptr[w]->shade=brightness;
     }
     sectptr[s]->ceilingshade=brightness;
}

void
processtags(void)
{
     short goalshade,i,k,s;

     for (i=0 ; i < secnt ; i++) {
          s=septr[i]->sector;
          switch (septr[i]->type) {
          case PULSELIGHT:
               if (lockclock < septr[i]->pclock) {
                    continue;
               }
               septr[i]->pclock=lockclock+septr[i]->pdelay;
               goalshade=septr[i]->hilight;
               if (septr[i]->lolight < goalshade) {
                    sectptr[s]->floorshade++;
                    if (sectptr[s]->floorshade == goalshade) {
                         k=septr[i]->lolight;
                         septr[i]->lolight=septr[i]->hilight;
                         septr[i]->hilight=k;
                    }
               }
               else {
                    sectptr[s]->floorshade--;
                    if (sectptr[s]->floorshade == goalshade) {
                         k=septr[i]->lolight;
                         septr[i]->lolight=septr[i]->hilight;
                         septr[i]->hilight=k;
                    }
               }
               setsectorlight(s,sectptr[s]->floorshade);
               break;
          case FLICKERLIGHT:
               if (lockclock < septr[i]->pclock) {
                    continue;
               }
               septr[i]->pclock=lockclock+(krand()%septr[i]->pdelay);
               if (sectptr[s]->floorshade == septr[i]->lolight) {
                    goalshade=septr[i]->hilight;
               }
               else if (krand() < 16384) {
                    goalshade=septr[i]->lolight;
               }
               setsectorlight(s,goalshade);
               break;
          }
     }
     if (visibility < defvisibility) {
          visibility++;
     }
     else if (visibility > defvisibility) {
          visibility--;
     }
}

void
dodoor(short d)
{
     short sect;
     long newz,px,py;
     struct doortype *door;

     door=doorptr[d];
     sect=door->sector;
     px=door->centerx;
     py=door->centery;
     switch (door->state) {
     case D_NOTHING:
          break;
     case D_OPENDOOR:
          switch (door->type) {
          case DOORUPTAG:
               wsayfollow("dsdoropn.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               break;
          case PLATFORMELEVTAG:
          case PLATFORMDELAYTAG:
          case PLATFORMDROPTAG:
               wsayfollow("dspstart.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               break;
          }
          door->state=D_OPENING;
          break;
     case D_CLOSEDOOR:
          switch (door->type) {
          case DOORUPTAG:
               wsayfollow("dsdorcls.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               break;
          case PLATFORMELEVTAG:
          case PLATFORMDELAYTAG:
               wsayfollow("dspstart.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               break;
          }
          door->state=D_CLOSING;
          break;
     case D_OPENSOUND:
          switch (door->type) {
          case DOORUPTAG:
               door->clock=lockclock+DOORDELAY;
               door->state=D_WAITING;
               break;
          case PLATFORMELEVTAG:
          case PLATFORMDELAYTAG:
               wsayfollow("dspstop.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               door->clock=lockclock+DOORDELAY;
               door->state=D_WAITING;
               break;
          case PLATFORMDROPTAG:
               wsayfollow("dspstop.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               door->state=D_DISABLED;
               break;
          }
          break;
     case D_CLOSESOUND:
          switch (door->type) {
          case PLATFORMELEVTAG:
          case PLATFORMDELAYTAG:
               wsayfollow("dspstop.wav",4096L+(krand()&255)-128,256L,&px,&py,0);
               break;
          }
          door->state=D_NOTHING;
          break;
     case D_OPENING:
          switch (door->type) {
          case DOORUPTAG:
               newz=sectptr[sect]->ceilingz-door->stepz;
               if (newz <= door->destz) {
                    newz=door->destz;
                    door->state=D_OPENSOUND;
               }
               sectptr[sect]->ceilingz=newz;
               break;
          case PLATFORMELEVTAG:
          case PLATFORMDELAYTAG:
          case PLATFORMDROPTAG:
               if (door->destz > door->startz) {  // drop
                    newz=sectptr[sect]->floorz+door->stepz;
                    if (newz >= door->destz) {
                         newz=door->destz;
                         door->state=D_OPENSOUND;
                    }
               }
               else {                        // rise
                    newz=sectptr[sect]->floorz-door->stepz;
                    if (newz <= door->destz) {
                         newz=door->destz;
                         door->state=D_OPENSOUND;
                    }
               }
               sectptr[sect]->floorz=newz;
          }
          break;
     case D_CLOSING:
          switch (door->type) {
          case DOORUPTAG:
               newz=sectptr[sect]->ceilingz+door->stepz;
               if (newz >= door->startz) {
                    newz=door->startz;
                    door->state=D_CLOSESOUND;
               }
               sectptr[sect]->ceilingz=newz;
               break;
          case PLATFORMELEVTAG:
          case PLATFORMDELAYTAG:
               if (door->destz > door->startz) {  // drop
                    newz=sectptr[sect]->floorz-door->stepz;
                    if (newz <= door->startz) {
                         newz=door->startz;
                         door->state=D_CLOSESOUND;
                    }
               }
               else {                        // rise
                    newz=sectptr[sect]->floorz+door->stepz;
                    if (newz >= door->destz) {
                         newz=door->destz;
                         door->state=D_CLOSESOUND;
                    }
               }
               sectptr[sect]->floorz=newz;
               break;
          }
          break;
     case D_WAITING:
          if (lockclock >= door->clock) {
               door->state=D_CLOSEDOOR;
          }
          break;
     case D_DISABLED:
          break;
     }
}

void
processdoors(void)
{
     short i;

     for (i=0 ; i < numdoors ; i++) {
          dodoor(i);
     }
}

void
processdebug(void)
{
     if (debugflags&DEBUGNEWPLAYER) {
          if (numplayers < MAXPLAYERS) {
               showmessage("ADDING PLAYER %d",numplayers);
               connectpoint2[numplayers-1]=numplayers;
               connectpoint2[numplayers]=-1;
               initplayer(numplayers);
               numplayers++;
          }
          debugflags&=~DEBUGNEWPLAYER;
     }
     if (debugflags&DEBUGNEWVIEW) {
          curview=connectpoint2[curview];
          if (curview == -1) {
               curview=connecthead;
          }
          drawstatusbar(curview);
          showmessage("SWITCHING VIEW TO PLAYER %d",curview);
          debugflags&=~DEBUGNEWVIEW;
     }
     if (debugflags&DEBUGNEWCONTROL) {
          curplyr=connectpoint2[curplyr];
          if (curplyr == -1) {
               curplyr=connecthead;
          }
          showmessage("SWITCHING CONTROL TO PLAYER %d",curplyr);
          debugflags&=~DEBUGNEWCONTROL;
     }
}

#ifdef DEBUGOUTPUT
void
debugout(short p)
{
     short s;

     s=plrsprite[p];
     fprintf(dbgfp,"%3d %3ld %5ld %07X %7ld %7ld %7ld %4d %4d %4d %4d\n",
          p,movefifoplc,lockclock,multipack[movefifoplc][p].plrmove,
          sprptr[s]->x,sprptr[s]->y,sprptr[s]->z,sprptr[s]->ang,
          mvel[p],tvel[p],sprdta[s]->forcev);
}
#endif

void
domovethings(void)
{
     short i,m;

     m=movefifoplc;
     lockclock+=TICSPERFRAME;
     for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
#ifdef DEBUGOUTPUT
          debugout(i);
#endif
          plrmove[i]=multipack[m][i].plrmove;
          plrmousx[i]=multipack[m][i].mousx;
          plrmousy[i]=multipack[m][i].mousy;
          weap[i]=multipack[m][i].weap;
          processinput(i);
          processtouch(i);
          processweaps(i);
     }
     processstats();
     processtags();
     processdoors();
     processdebug();
     movefifoplc=(m+1)&(MOVEFIFOSIZE-1);
}

void
analyzesprites(long dax,long day,long smoothratio)
{
     long i,j,k;

     for (i=0 ; i < spritesortcnt ; i++) {
          j=thesprite[i];
          switch (sprptr[j]->picnum) {
          case DOOMGUYPIC:         // 4 frame angles
               k=getangle(sprptr[j]->x-dax,sprptr[j]->y-day);
               k=(((sprptr[j]->ang+3072+128-k)&2047)>>8)&7;
               if (k <= 4) {
                    spritepicnum[i]+=(k<<2);
                    sprptr[j]->cstat&=~4;
               }
               else {
                    spritepicnum[i]+=((8-k)<<2);
                    sprptr[j]->cstat|=4;
               }
               break;
          case DOOMGUYSHOOT:       // 2 frame angles
          case DOOMGUYSHOOT2:
               k=getangle(sprptr[j]->x-dax,sprptr[j]->y-day);
               k=(((sprptr[j]->ang+3072+128-k)&2047)>>8)&7;
               if (k <= 4) {
                    spritepicnum[i]+=(k<<1);
                    sprptr[j]->cstat&=~4;
               }
               else {
                    spritepicnum[i]+=((8-k)<<1);
                    sprptr[j]->cstat|=4;
               }
               break;
          case DOOMGUYPAIN:        // 1 frame angles
               k=getangle(sprptr[j]->x-dax,sprptr[j]->y-day);
               k=(((sprptr[j]->ang+3072+128-k)&2047)>>8)&7;
               if (k <= 4) {
                    spritepicnum[i]+=k;
                    sprptr[j]->cstat&=~4;
               }
               else {
                    spritepicnum[i]+=(8-k);
                    sprptr[j]->cstat|=4;
               }
               break;
          case GUN5PROJECTILE:
               k=getangle(sprptr[j]->x-dax,sprptr[j]->y-day);
               k=(((sprptr[j]->ang+2048+128-k)&2047)>>8)&7;
               if (k <= 4) {
                    spritepicnum[i]+=k;
                    sprptr[j]->cstat&=~4;
               }
               else {
                    spritepicnum[i]+=(8-k);
                    sprptr[j]->cstat|=4;
               }
               break;
          }
     }
}

void
drawscreenfx(void)
{
     long i;

     i=lockclock;
     if (i != frameval[framecnt]) {
          sprintf(tempbuf,"%ld",(CLKIPS*AVERAGEFRAMES)/(i-frameval[framecnt]));
          printext256(xmin,ymax-8,31,-1,tempbuf,1);
          frameval[framecnt]=i;
     }
     framecnt=((framecnt+1)&(AVERAGEFRAMES-1));
     if (lockclock < msgclock) {
          printext256(xmin,ymin,31,-1,msgbuf,1);
     }
     if (typemsgflag) {
          printextf(xmin,ymin+8,"%s_",typebuf);
     }
}

void
drawweapon(short p)
{
     short s,weapframe,weapnum;
     long x,y;
     struct weaptype *weapptr;

     if (plrdead[p] != 0) {
          return;
     }
     weapnum=weap[p];
     s=sprptr[plrsprite[p]]->sectnum;
     weapptr=&weaptype[weapnum];
     if (plrflags[p]&(1<<MOVESHOOT)) {
          switch (weapnum) {
          case G_SHOTGUN:
               weapframe=weapptr->firestartpic+weapstep[p];
               if (weapstep[p] < 2) {
                    x=xdim>>1;
                    y=ydim-tilesizy[weapframe];
                    overwritesprite(x,y,weapframe,sectptr[s]->floorshade,1|2,0);
                    weapframe=weapptr->firepic;
               }
               y=ydim-(tilesizy[weapframe]>>2);
               break;
          case G_GATLING:
               weapframe=weapptr->firepic+(weapstep[p]&0x01);
               x=xdim>>1;
               y=ydim-tilesizy[weapframe];
               overwritesprite(x,y,weapframe,sectptr[s]->floorshade,1|2,0);
               weapframe=weapptr->basepic+(weapstep[p]&0x01);
               y=ydim-(tilesizy[weapframe]>>3);
               break;
          case G_ROCKET:
               weapframe=weapptr->firestartpic+weapstep[p];
               x=xdim>>1;
               y=ydim-(tilesizy[weapframe]>>1);
               overwritesprite(x,y,weapframe,sectptr[s]->floorshade,1|2,0);
               weapframe=weapptr->firepic;
               y=ydim-(tilesizy[weapframe]>>3);
               break;
          case G_PLASMA:
               weapframe=weapptr->firestartpic+weapstep[p];
               y=ydim-(tilesizy[weapframe]>>1);
               break;
          }
     }
     else {
          weapframe=weapptr->basepic;
          switch (weapnum) {
          case G_SHOTGUN:
               y=ydim-(tilesizy[weapframe]>>2);
               break;
          default:
               y=ydim-(tilesizy[weapframe]>>3);
               break;
          }
     }
     x=xdim>>1;
     overwritesprite(x,y,weapframe,sectptr[s]->floorshade,1|2,0);
}

void
drawscreen(short view,long smoothratio)
{
     short s;
     long pa,ph,ps,px,py,pz;

     s=plrsprite[view];
     pa=sprptr[s]->ang;
     ph=sprdta[s]->horiz;
     ps=sprptr[s]->sectnum;
     px=sprptr[s]->x;
     py=sprptr[s]->y;
     pz=sprdta[s]->z;
     if (plrdead[view] != 0) {
          pz+=plrdead[view];
     }
     setears(px,py,(long)sintable[(pa+512)&2047]<<14,(long)sintable[pa]<<14);
     if (plrscreen[view] == 3) {
          if (curvidmode != 3) {
               setgamemode();
               curvidmode=3;
          }
          drawrooms(px,py,pz,pa,ph,ps);
          analyzesprites(px,py,smoothratio);
          drawmasks();
          drawscreenfx();
          drawweapon(view);
          if (debugflags&DEBUGPLAYERS) {
               monitor();
          }
     }
     else if (plrscreen[view] == 2) {
          if (curvidmode != 2) {
               qsetmode640480();
               curvidmode=2;
          }
          draw2dscreen(px,py,pa,(long)plrzoom[view],0);
     }
     nextpage();
}

void
main(int argc,char *argv[])
{
     if (setjmp(jmpenv) != 0) {
          exit(1);
     }
     oldvmode=getvmode();
     installGPhandler();
     readsetup();
     parseargs(argc,argv);
     if (option[3] != 0) {
          initmouse();
     }
     switch (option[0]) {
     case 0:
          initengine(0,chainxres[option[6]&15],chainyres[option[6]>>4]);
          break;
     case 1:
          initengine(1,vesares[option[6]&15][0],vesares[option[6]&15][1]);
          break;
     case 2:
          initengine(2,320L,200L);
          break;
     case 3:
          initengine(3,320L,200L);
          break;
     case 4:
          initengine(4,320L,200L);
          break;
     case 5:
          initengine(5,320L,200L);
          break;
     case 6:
          initengine(6,320L,200L);
          break;
     }
     engineinitialized=1;
     initmultiplayers(option[4],option[5]);
     multiinitialized=1;
     pskyoff[0]=pskyoff[1]=pskyoff[2]=0;
     pskybits=2;
     screensize=xdim;
     inittimer();
     timerinitialized=1;
     initsb(option[1],option[2],digihz[option[7]>>4],((option[7]&4)>0)+1,
          ((option[7]&2)>0)+1,60,option[7]&1);
     soundinitialized=1;
     initpaltable();
     loadpics("tiles000.art");
     initboard();
     setgamemode();
     setup3dscreen();
#ifdef DEBUGOUTPUT
     if (option[4] != 0) {
          dbgfp=fopen("debug.out","wt");
          fprintf(dbgfp," P  PLC  CLK    MOV      X       Y       Z     AN  MVEL TVEL FRCV\n");
          fprintf(dbgfp,"=== === ===== ======= ======= ======= ======= ==== ==== ==== ====\n");
     }
#endif
     randomseed=17L;
     initplayers(npyrs);
     resettiming();
     syncpack=0;
     lockclock=0L;
     fakeclock=totalclock+TICSPERFRAME;
     movefifoplc=movefifoend=0L;
     ready2send=1;
     defvisibility=visibility;
     while (ingame) {
          while (movefifoplc != movefifoend) {
               domovethings();
          }
          drawscreen(curview,0L);
          if (keystatus[1] != 0) {
               ingame=0;
          }
     }
#ifdef DEBUGOUTPUT
     if (option[4] != 0) {
          fclose(dbgfp);
     }
#endif
     shutdown();
}

void
movethings(void)
{
     short i,m;

     m=movefifoend;
     for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
          memmove(&multipack[m][i],&rcvpack[i],sizeof(struct multipack));
     }
     movefifoend=(m+1)&(MOVEFIFOSIZE-1);
}

void
faketimerhandler(void)
{
     short i,j,m,opyr,tmpbuflen;

     if (totalclock < fakeclock) {
          return;
     }
     if (ready2send == 0) {
          return;
     }
     fakeclock=totalclock+TICSPERFRAME;
     if (option[4] != 0) {
          if (pyrn == connecthead) {
               while ((tmpbuflen=getpacket(&opyr,tempbuf)) > 0) {
                    switch (tempbuf[0]) {
                    case M_SSYNC:
                         memmove(&rcvpack[opyr],&tempbuf[1],sizeof(struct multipack));
                         break;
                    case M_MESSAGE:
                         showmessage("(%d): %s",opyr,&tempbuf[1]);
                         wsayfollow("dstink.wav",4096L+(krand()&255)-128,256L,
                              &sprptr[plrsprite[curplyr]]->x,
                              &sprptr[plrsprite[curplyr]]->y,0);
                         break;
                    case M_LOGOUT:
                         changespritestat(plrsprite[opyr],DIESTAT);
                         showmessage("PLAYER %d LEFT THE GAME",opyr);
                         break;
                    }
               }
               if (getoutputcirclesize() < 16) {
                    keytimerstuff();
                    rcvpack[pyrn].plrmove=plrmove[pyrn];
                    rcvpack[pyrn].weap=weap[pyrn];
                    rcvpack[pyrn].mousx=plrmousx[pyrn];
                    rcvpack[pyrn].mousy=plrmousy[pyrn];
                    j=0;
                    tempbuf[j++]=M_MSYNC;
                    tempbuf[j++]=syncpack;
                    for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
                         memmove(&tempbuf[j],&rcvpack[i],sizeof(struct multipack));
                         j+=sizeof(struct multipack);
                    }
                    for (i=connectpoint2[connecthead] ; i >= 0 ; i=connectpoint2[i]) {
                         sendpacket(i,tempbuf,j);
                    }
                    syncpack++;
                    movethings();
               }
          }
          else {
               while ((tmpbuflen=getpacket(&opyr,tempbuf)) > 0) {
                    switch (tempbuf[0]) {
                    case M_MSYNC:
                         j=1;
                         if (tempbuf[j] != syncpack) {
                              crash("Lost packet! (m=%d,s=%d)",tempbuf[j],syncpack);
                         }
                         j++;
                         for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
                              memmove(&rcvpack[i],&tempbuf[j],sizeof(struct multipack));
                              j+=sizeof(struct multipack);
                         }
                         syncpack++;
                         movethings();
                         break;
                    case M_MESSAGE:
                         showmessage("(%d): %s",opyr,&tempbuf[1]);
                         wsayfollow("dstink.wav",4096L+(krand()&255)-128,256L,
                              &sprptr[plrsprite[curplyr]]->x,
                              &sprptr[plrsprite[curplyr]]->y,0);
                         break;
                    case M_LOGOUT:
                         changespritestat(plrsprite[opyr],DIESTAT);
                         showmessage("PLAYER %d LEFT THE GAME",opyr);
                         if (opyr == connecthead && connectpoint2[connecthead] == pyrn) {
                              connecthead=pyrn;
                              connectpoint2[pyrn]=connectpoint2[numplayers-1];
                         }
                         break;
                    }
               }
               if (getoutputcirclesize() < 16) {
                    keytimerstuff();
                    j=0;
                    tempbuf[j++]=M_SSYNC;
                    sndpck.plrmove=plrmove[pyrn];
                    sndpck.weap=weap[pyrn];
                    sndpck.mousx=plrmousx[pyrn];
                    sndpck.mousy=plrmousy[pyrn];
                    memmove(&tempbuf[j],&sndpck,sizeof(struct multipack));
                    j+=sizeof(struct multipack);
                    sendpacket(connecthead,tempbuf,j);
               }
          }
     }
     else {
          keytimerstuff();
          rcvpack[curplyr].plrmove=plrmove[curplyr];
          rcvpack[curplyr].weap=weap[curplyr];
          rcvpack[curplyr].mousx=plrmousx[curplyr];
          rcvpack[curplyr].mousy=plrmousy[curplyr];
          movethings();
     }
}

void
monitor(void)
{
     short p;

     for (p=connecthead ; p >= 0 ; p=connectpoint2[p]) {
          printextf(0L,8L+(p*8L),"P: %02d X: %06d Y: %06d Z: %06d/%06d PM: %08X FV: %06d",
               p,sprptr[plrsprite[p]]->x,sprptr[plrsprite[p]]->y,
               sprptr[plrsprite[p]]->z,sprdta[plrsprite[p]]->z,plrmove[p],
               sprdta[plrsprite[p]]->forcev);
     }
}

short
movesprite(short spritenum,long dx,long dy,long dz,long ceildist,long flordist,
     char cliptype)
{
     long daz,zoffs,templong;
     short retval,dasectnum,tempshort;
     struct spritetype *spr;

     spr=sprptr[spritenum];
     if ((spr->cstat&128) == 0) {
          zoffs=-((tilesizy[spr->picnum]*spr->yrepeat)<<1);
     }
     else {
          zoffs=0;
     }
     dasectnum=spr->sectnum;
     daz=spr->z+zoffs;
     retval=clipmove(&spr->x,&spr->y,&daz,&dasectnum,dx,dy,
          ((long)spr->clipdist)<<2,ceildist,flordist,cliptype);
     if ((dasectnum != spr->sectnum) && (dasectnum >= 0)) {
          changespritesect(spritenum,dasectnum);
     }
     tempshort=spr->cstat;
     spr->cstat&=~1;
     getzrange(spr->x,spr->y,spr->z-1,spr->sectnum,&globhiz,&globhihit,
          &globloz,&globlohit,((long)spr->clipdist)<<2,cliptype);
     spr->cstat=tempshort;
     daz=spr->z+zoffs+dz;
     if ((daz <= globhiz) || (daz > globloz)) {
          if (retval != 0) {
               return(retval);
          }
          return(16384+dasectnum);
     }
     spr->z=daz-zoffs;
     return(retval);
}

