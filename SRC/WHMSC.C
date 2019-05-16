/*
        WHMSC.C
*/

#include "icorp.h"

static
int  cursbot,curstop,
     cursx,cursy,
     winx1,winy1,
     winx2,winy2,
     winatr;

#if MULTIPLAYER
int  ipxflag;

int  totstarts;

long fakepersec,
     gotlastpacketclock,
     movefifoend,
     movefifoplc,
     ototalclock,
     ready2send;

struct baksync baksync[MAXPLAYERS],
     *baksyncptr[MAXPLAYERS];

struct multistart multistart[MAXMULTISTARTS],
     *multistartptr[MAXMULTISTARTS];

char synckeys[MAXSYNCKEYS]={
          KEYFWD,KEYBACK,KEYLEFT,KEYRIGHT,KEYRUN,KEYSTRAFE,KEYFIRE,KEYUSE,
          KEYELEVUP,KEYELEVDN,KEYSTAND,KEYLAY,KEYUP,KEYDOWN,KEYGUN1,KEYGUN2,
          KEYGUN3,KEYGUN4,KEYGUN5,KEYGUN6
     },
     tempbuf[576];


//   DPMI stuff

struct rmreg {                // real mode register pack
     unsigned long edi;
     unsigned long esi;
     unsigned long ebp;
     unsigned long rsv;
     unsigned long ebx;
     unsigned long edx;
     unsigned long ecx;
     unsigned long eax;
     unsigned short flags;
     unsigned short es;
     unsigned short ds;
     unsigned short fs;
     unsigned short gs;
     unsigned short ip;
     unsigned short cs;
     unsigned short sp;
     unsigned short ss;
} rmreg;

struct ipxentry {
     unsigned short cs;
     unsigned short ip;
} ipxentry;
#endif


void
faketimerhandler(void)
{
#if MULTIPLAYER
     short other,tempbufleng;
     long i,j,k,l;
     struct player *plr;
     struct playerext *plrx;

     if (totalclock < ototalclock+TICSPERFRAME) {
          return;
     }
     if (ready2send == 0) {
          return;
     }
     fakepersec++;
     ototalclock=totalclock;
     if ((pyrn == connecthead) || (option[4] == 0)) {
          if (option[4] != 0) {
               while ((tempbufleng=getpacket(&other,tempbuf)) > 0) {
                    plr=plptrlist[other];
                    plrx=(struct playerext *)plr->ext1;
                    switch (tempbuf[0]) {
                    case 1:        //[1] (receive slave sync buffer)
                         j=2;
                         k=tempbuf[1];
                         if (k&1) {
                              plrx->vel=tempbuf[j++];
                         }
                         if (k&2) {
                              plrx->svel=tempbuf[j++];
                         }
                         if (k&4) {
                              plrx->angvel=tempbuf[j++];
                         }
                         if (k&8) {
                              plrx->syncbits=((plrx->syncbits&0xFFFFFF00)|((long)tempbuf[j++]));
                         }
                         if (k&16) {
                              plrx->syncbits=((plrx->syncbits&0xFFFF00FF)|((long)tempbuf[j++]<<8));
                         }
                         if (k&32) {
                              plrx->syncbits=((plrx->syncbits&0xFF00FFFF)|((long)tempbuf[j++]<<16));
                         }
                         if (k&64) {
                              plrx->syncbits=((plrx->syncbits&0x00FFFFFF)|((long)tempbuf[j++]<<24));
                         }
                         if (k&128) {
                              plrx->weapon=tempbuf[j++];
                         }
                         break;
                    case 2:        // typed message text
                         break;
                    case 3:        // voice transmission
                         break;
                    case 255:      //[255] (logout)
                         showmessage("Player %d left the game",other);
                         deletesprite(plr->spritenum);
                         break;
                    }
               }
          }
          if (getoutputcirclesize() < 16) {
               getinput(plptrlist[pyrn]);
               if (option[4] != 0) {
                    tempbuf[0]=0;
                    tempbuf[1]=numplayers;
                    j=2;
                    for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
                         plr=plptrlist[i];
                         plrx=(struct playerext *)plr->ext1;
                         tempbuf[j]=(plr->x&0x000000FF);
                         tempbuf[j+1]=(plr->x>>8)&0x000000FF;
                         tempbuf[j+2]=(plr->x>>16)&0x000000FF;
                         tempbuf[j+3]=(plr->x>>24)&0x000000FF;
                         j+=4;
                         tempbuf[j]=(plr->y&0x000000FF);
                         tempbuf[j+1]=(plr->y>>8)&0x000000FF;
                         tempbuf[j+2]=(plr->y>>16)&0x000000FF;
                         tempbuf[j+3]=(plr->y>>24)&0x000000FF;
                         j+=4;
                         tempbuf[j]=(plr->z&0x000000FF);
                         tempbuf[j+1]=(plr->z>>8)&0x000000FF;
                         tempbuf[j+2]=(plr->z>>16)&0x000000FF;
                         tempbuf[j+3]=(plr->z>>24)&0x000000FF;
                         j+=4;
                         tempbuf[j]=(plr->ang&0x000000FF);
                         tempbuf[j+1]=(plr->ang>>8)&0x000000FF;
                         j+=2;
                         tempbuf[j]=(plr->sector&0x000000FF);
                         tempbuf[j+1]=(plr->sector>>8)&0x000000FF;
                         j+=2;
                         tempbuf[j]=(plrx->syncbits&0xFF);
                         tempbuf[j+1]=(plrx->syncbits>>8)&0xFF;
                         tempbuf[j+2]=(plrx->syncbits>>16)&0xFF;
                         tempbuf[j+3]=(plrx->syncbits>>24)&0xFF;
                         j+=4;
                         tempbuf[j]=plrx->weapon;
                         j+=1;
                         tempbuf[j]=(plrx->health&0x00FF);
                         tempbuf[j+1]=(plrx->health>>8)&0x00FF;
                         j+=2;
                         plrx->oldvel=plrx->vel;
                         plrx->oldsvel=plrx->svel;
                         plrx->oldangvel=plrx->angvel;
                         plrx->oldsyncbits=plrx->syncbits;
                         if (plrx->weapon != plrx->oldweapon) {
                              memmove(&plrx->weapontype,wptptr[plrx->weapon],
                                                  sizeof(struct weapontype));
                              plrx->oldweapon=plrx->weapon;
                         }
                    }
                    for (i=connectpoint2[connecthead] ; i >= 0 ; i=connectpoint2[i]) {
                         sendpacket(i,tempbuf,j);
                    }
               }
               movethings();       //Move EVERYTHING (you too!)
          }
     }
     else {                        //I am a SLAVE
          while ((tempbufleng=getpacket(&other,tempbuf)) > 0) {
               switch (tempbuf[0]) {
               case 0:             //[0] (receive master sync buffer)
                    if (numplayers != tempbuf[1]) {
                         crash("faketimer: numplayers mismatch - "
                              "master=%d,slave=%d",tempbuf[1],numplayers);
                    }
                    j=2;
                    for (i=connecthead ; i >= 0 ; i=connectpoint2[i]) {
                         plr=plptrlist[i];
                         plrx=(struct playerext *)plr->ext1;
                         plr->x=tempbuf[j]+(tempbuf[j+1]<<8)
                              +(tempbuf[j+2]<<16)+(tempbuf[j+3]<<24);
                         j+=4;
                         plr->y=tempbuf[j]+(tempbuf[j+1]<<8)
                              +(tempbuf[j+2]<<16)+(tempbuf[j+3]<<24);
                         j+=4;
                         plr->z=tempbuf[j]+(tempbuf[j+1]<<8)
                              +(tempbuf[j+2]<<16)+(tempbuf[j+3]<<24);
                         j+=4;
                         plr->ang=tempbuf[j]+(tempbuf[j+1]<<8);
                         j+=2;
                         plr->sector=tempbuf[j]+(tempbuf[j+1]<<8);
                         j+=2;
                         plrx->syncbits=tempbuf[j]+(tempbuf[j+1]<<8)
                              +(tempbuf[j+2]<<16)+(tempbuf[j+3]<<24);
                         j+=4;
                         if (i != pyrn) {
                              plrx->weapon=tempbuf[j];
                              if (plrx->weapon != plrx->oldweapon) {
                                   memmove(&plrx->weapontype,wptptr[plrx->weapon],
                                                       sizeof(struct weapontype));
                                   plrx->oldweapon=plrx->weapon;
                              }
                         }
                         j+=1;
                         plrx->health=tempbuf[j]+(tempbuf[j+1]<<8);
                         j+=2;
                    }
                    movethings();  //Move all players and sprites
                    break;
               case 2:
                    break;
               case 3:
                    break;
               case 255:           //[255] (logout)
                    showmessage("Player %d left the game",other);
                    deletesprite(plptrlist[other]->spritenum);
                    break;
               }
          }
          if (getoutputcirclesize() < 16) {
               getinput(plptrlist[pyrn]);
               tempbuf[0]=1;
               k=0;
               j=2;
               plr=plptrlist[pyrn];
               plrx=(struct playerext *)plr->ext1;
               if (plrx->vel != plrx->oldvel) {
                    tempbuf[j++]=plrx->vel;
                    k|=1;
               }
               if (plrx->svel != plrx->oldsvel) {
                    tempbuf[j++]=plrx->svel;
                    k|=2;
               }
               if (plrx->angvel != plrx->oldangvel) {
                    tempbuf[j++]=plrx->angvel;
                    k|=4;
               }
               if ((plrx->syncbits&0x000000FF) != (plrx->oldsyncbits&0x000000FF)) {
                    tempbuf[j++]=(plrx->syncbits&255);
                    k|=8;
               }
               if ((plrx->syncbits&0x0000FF00) != (plrx->oldsyncbits&0x0000FF00)) {
                    tempbuf[j++]=((plrx->syncbits>>8)&255);
                    k|=16;
               }
               if ((plrx->syncbits&0x00FF0000) != (plrx->oldsyncbits&0x00FF0000)) {
                    tempbuf[j++]=((plrx->syncbits>>16)&255);
                    k|=32;
               }
               if ((plrx->syncbits&0xFF000000) != (plrx->oldsyncbits&0xFF000000)) {
                    tempbuf[j++]=((plrx->syncbits>>24)&255);
                    k|=64;
               }
               if (plrx->weapon != plrx->oldweapon) {
                    tempbuf[j++]=(char)plrx->weapon;
                    k|=128;
               }
               tempbuf[1]=k;
               plrx->oldvel=plrx->vel;
               plrx->oldsvel=plrx->svel;
               plrx->oldangvel=plrx->angvel;
               plrx->oldsyncbits=plrx->syncbits;
               sendpacket(connecthead,tempbuf,j);
          }
     }
#endif
}


char buf[80];

void
setvmode(int m)
{
     union REGS regs;

     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x00;
     regs.h.al=m;
     int386(0x10,&regs,&regs);
}

void
setatr(int fore,int back)
{
     winatr=(((back&0x0F)<<4)|(fore&0x0F));
}

void
setcsize(int t,int b)
{
     union REGS regs;

     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x01;
     regs.h.ch=t;
     regs.h.cl=b;
     int386(0x10,&regs,&regs);
}

void
gotoxy(int x,int y)
{
     union REGS regs;

     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x02;
     regs.h.bh=0;
     regs.h.dh=y;
     regs.h.dl=x;
     int386(0x10,&regs,&regs);
     cursx=x;
     cursy=y;
}

void
getcurs(void)
{
     union REGS regs;

     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x03;
     regs.h.bh=0;
     int386(0x10,&regs,&regs);
     if (curstop == -1) {
          curstop=regs.h.ch;
     }
     if (cursbot == -1) {
          cursbot=regs.h.cl;
     }
     cursy=regs.h.dh;
     cursx=regs.h.dl;
}

void
setregion(int x1,int y1,int x2,int y2)
{
     winx1=x1;
     winy1=y1;
     winx2=x2;
     winy2=y2;
}

void
scrollup(void)
{
     union REGS regs;

     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x06;
     regs.h.al=1;
     regs.h.bh=winatr;
     regs.h.ch=winy1;
     regs.h.cl=winx1;
     regs.h.dh=winy2;
     regs.h.dl=winx2;
     int386(0x10,&regs,&regs);
}

void
scrolldn(void)
{
     union REGS regs;

     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x07;
     regs.h.al=1;
     regs.h.bh=winatr;
     regs.h.ch=winy1;
     regs.h.cl=winx1;
     regs.h.dh=winy2;
     regs.h.dl=winx2;
     int386(0x10,&regs,&regs);
}

void
clrregion(int fore,int back)
{
     union REGS regs;

     setatr(fore,back);
     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x06;
     regs.h.al=0;
     regs.h.bh=winatr;
     regs.h.ch=winy1;
     regs.h.cl=winx1;
     regs.h.dh=winy2;
     regs.h.dl=winx2;
     int386(0x10,&regs,&regs);
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

#if MULTIPLAYER
int
dpmi_callrealint(int no,struct rmreg *r)
{
     union REGS regs;
     struct SREGS sregs;
     void far *fp;

     fp=(void far *)r;
     segread(&sregs);
     regs.x.eax=0x300;
     regs.x.ebx=no;
     regs.x.ecx=0;
     sregs.es=FP_SEG(fp);
     regs.x.edi=FP_OFF(fp);
     r->ss=0;
     r->sp=0;
     r->flags=0;
     int386x(0x31,&regs,&regs,&sregs);
     return(regs.x.cflag);
}

int
isipx(void)
{
     rmreg.eax=0x7A00;
     dpmi_callrealint(0x2F,&rmreg);
     if ((rmreg.eax&0x000000FF) != 0xFF) {
          ipxentry.cs=0;
          ipxentry.ip=0;
          ipxflag=0;
     }
     else {
          ipxentry.cs=rmreg.es;
          ipxentry.ip=rmreg.edi&0xFFFF;
          ipxflag=1;
     }
     return(ipxflag);
}
#endif

