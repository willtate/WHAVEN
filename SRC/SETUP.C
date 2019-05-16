/***************************************************************************
 *   SETUP.C  - Setup utility for TEKWAR & WITCHAVEN game                  *
 *                                                                         *
 *   use "WCL SETUP.C" to compile (no need for 32 bit)                     *
 *                                                                         *
 *                                                     06/25/95 Les Bird   *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <setjmp.h>
#include <stdarg.h>
#include <graph.h>
#include <io.h>
#include <process.h>
#include "keycodes.h"

#define   FONTHEIGHT     8
#define   FONTWIDTH      8

//#define   TEKWAR

#ifdef TEKWAR
#define   NUMOPTS        8
#else
#define   NUMOPTS        12
#endif

#define   NUMKEYS        27
#define   MOREOPTS       7         // mouse buttons, joystick on/off/buttons

char option[NUMOPTS+NUMKEYS+MOREOPTS];

volatile
char keyc;

int  curx,cury,
     menupos,
     mouse,
     mouseon,
     mx,oldmx,
     my,oldmy,
     mb,oldmb,
     selopt;

struct videoconfig vc;
struct textsettings ts;

char textbuf[160];

#if 0
char defkeys[]={
     0xC8,0xD0,0xCB,0xCD,0x2A,0x9D,0x1D,0x39,
     0x2D,0x2E,0xC9,0xD1,0xC7,0x33,0x34,
     0x0F,0x1C,0x0D,0x0C,0x9C,0x13,0x16,
     0x23,0x1E,0x14,0x1F,0x35
};
#endif
char defkeys[]={
     0xC8,0xD0,0xCB,0xCD,0x2A,0x38,0x1d,0x39,
     0x2d,0x2e,0xd1,0xc9,0xC7,0x33,0x34,
     0x0F,0x1C,0x0D,0x0C,0x9C,0x1C,0x29,
     0x23,0x21,0x14,0xd2,0xd3
};


#define   MAINMENU            0
#define   CONTROLMENU         1
#define   SOUNDMENU           2
#define   VIDEOMENU           3
#define   EXITMENU            4
#define   MOUSEMENU           5
#define   JSTICKMENU          6
#define   ACTIONMENU          7

#define   MAINMENUOPTIONS     4
#define   MAINMENUSTART       0
#define   MAINMENUEND         (MAINMENUSTART+MAINMENUOPTIONS)
#ifdef TEKWAR
#define   CONTROLMENUOPTIONS  29
#else
#define   CONTROLMENUOPTIONS  29
#endif
#define   CONTROLMENUSTART    MAINMENUEND
#define   CONTROLMENUEND      (CONTROLMENUSTART+CONTROLMENUOPTIONS)
#ifdef TEKWAR
#define   SOUNDMENUOPTIONS    2
#else
#define   SOUNDMENUOPTIONS    4
#endif
#define   SOUNDMENUSTART      CONTROLMENUEND
#define   SOUNDMENUEND        (SOUNDMENUSTART+SOUNDMENUOPTIONS)
#define   MUSICMENUOPTIONS    0
#define   MUSICMENUSTART      SOUNDMENUEND
#define   MUSICMENUEND        (MUSICMENUSTART+MUSICMENUOPTIONS)
#ifdef TEKWAR
#define   VIDEOMENUOPTIONS    4
#else
#define   VIDEOMENUOPTIONS    2
#endif
#define   VIDEOMENUSTART      MUSICMENUEND
#define   VIDEOMENUEND        (VIDEOMENUSTART+VIDEOMENUOPTIONS)
#define   EXITMENUOPTIONS     3
#define   EXITMENUSTART       VIDEOMENUEND
#define   EXITMENUEND         (EXITMENUSTART+EXITMENUOPTIONS)
#define   MOUSEMENUOPTIONS    4
#define   MOUSEMENUSTART      EXITMENUEND
#define   MOUSEMENUEND        (MOUSEMENUSTART+MOUSEMENUOPTIONS)
#define   JSTICKMENUOPTIONS   6
#define   JSTICKMENUSTART     MOUSEMENUEND
#define   JSTICKMENUEND       (JSTICKMENUSTART+JSTICKMENUOPTIONS)
//#define   ACTIONMENUOPTIONS   19
#define   ACTIONMENUOPTIONS   6
#define   ACTIONMENUSTART     JSTICKMENUEND
#define   ACTIONMENUEND       (ACTIONMENUSTART+ACTIONMENUOPTIONS)

#define   MAINMENUY      24
#define   SOUNDMENUY     142
#define   CONTROLMENUY   142
#define   VIDEOMENUY     200
#define   EXITMENUY      142
#define   MOUSEMENUY     142
#define   JSTICKMENUY    142
#define   ACTIONMENUY    126

struct menu {
     int  x;
     int  y;
     int  fore;
     int  back;
     char *label;
} menu[]={
     { -1,MAINMENUY+00    ,14, 1,"CONTROL CONFIGURATION"},
     { -1,MAINMENUY+16    ,14, 1,"MUSIC/SOUND CONFIGURATION"},
     { -1,MAINMENUY+32    ,14, 1,"VIDEO MODE CONFIGURATION"},
     { -1,MAINMENUY+48    ,12, 1,"EXIT SETUP UTILITY"},
     { 48,CONTROLMENUY-14 ,10, 8,"CONFIGURE MOUSE    "},
     { 48,CONTROLMENUY+00 ,10, 8,"CONFIGURE  JOYSTICK"},
     { 48,CONTROLMENUY+14 ,10, 8,"MOVE FORWARD......."},
     { 48,CONTROLMENUY+28 ,10, 8,"MOVE BACKWARD......"},
     { 48,CONTROLMENUY+42 ,10, 8,"TURN LEFT.........."},
     { 48,CONTROLMENUY+56 ,10, 8,"TURN RIGHT........."},
     { 48,CONTROLMENUY+70 ,10, 8,"RUN................"},
     { 48,CONTROLMENUY+84 ,10, 8,"STRAFE MODE........"},
     { 48,CONTROLMENUY+98 ,10, 8,"USE WEAPON........."},
     { 48,CONTROLMENUY+112,10, 8,"OPEN/CLOSE........."},
     { 48,CONTROLMENUY+126,10, 8,"JUMP..............."},
     { 48,CONTROLMENUY+140,10, 8,"CROUCH............."},
     { 48,CONTROLMENUY+154,10, 8,"LOOK UP............"},
     { 48,CONTROLMENUY+168,10, 8,"LOOK DOWN.........."},
     {344,CONTROLMENUY+00 ,10, 8,"CENTER VIEW........"},
     {344,CONTROLMENUY+14 ,10, 8,"STRAFE LEFT........"},
     {344,CONTROLMENUY+28 ,10, 8,"STRAFE RIGHT......."},
     {344,CONTROLMENUY+42 ,10, 8,"MAP MODE..........."},
     {344,CONTROLMENUY+56 ,10, 8,NULL},
     {344,CONTROLMENUY+56 ,10, 8,"MAP MODE ZOOM IN..."},
     {344,CONTROLMENUY+70 ,10, 8,"MAP MODE ZOOM OUT.."},
#ifdef TEKWAR
     {344,CONTROLMENUY+84 ,10, 8,NULL},
     {344,CONTROLMENUY+84 ,10, 8,"REAR VIEW HEADSET.."},
     {344,CONTROLMENUY+98 ,10, 8,"CURRENT ITEM VIEW.."},
     {344,CONTROLMENUY+112,10, 8,"HEALTH METER......."},
     {344,CONTROLMENUY+126,10, 8,"AIMING RETICULE...."},
     {344,CONTROLMENUY+140,10, 8,"ELAPSED TIME......."},
     {344,CONTROLMENUY+154,10, 8,"SCORE TOGGLE......."},
     {344,CONTROLMENUY+168,10, 8,"CONCEAL WEAPON....."},
#else
     {344,CONTROLMENUY+84 ,10, 8,NULL},
     {344,CONTROLMENUY+84 ,10, 8,"USE POTION........."},
     {344,CONTROLMENUY+98 ,10, 8,"CAST SPELL........."},
     {344,CONTROLMENUY+112,10, 8,NULL},
     {344,CONTROLMENUY+112,10, 8,NULL},
     {344,CONTROLMENUY+126,10, 8,NULL},
     {344,CONTROLMENUY+112,10, 8,"FLY UP............."},
     {344,CONTROLMENUY+126,10, 8,"FLY DOWN..........."},
#endif
#ifdef TEKWAR
     { -1,SOUNDMENUY+00   ,10, 8,"EXECUTE SOUND CONFIGURATION PROGRAM"},
     { -1,SOUNDMENUY+16   ,10, 8,"RETURN TO MAIN MENU"},
#else
     { -1,SOUNDMENUY+00   ,10, 8,"MUSIC........... ON"},
     { -1,SOUNDMENUY+16   ,10, 8,"SOUND EFFECTS... ON"},
     { -1,SOUNDMENUY+32   ,10, 8,"EXECUTE SOUND CONFIGURATION PROGRAM"},
     { -1,SOUNDMENUY+48   ,10, 8,"RETURN TO MAIN MENU"},
#endif
     {232,VIDEOMENUY+00   ,10, 8,"320 x 200 - 256 colors"},
#ifdef TEKWAR
     {232,VIDEOMENUY+16   ,10, 8,"360 x 360 - 256 colors"},
     {232,VIDEOMENUY+32   ,10, 8,"640 x 400 - 256 colors"},
#endif
     {232,VIDEOMENUY+48   ,10, 8,"640 x 480 - 256 colors"},
     { -1,EXITMENUY+00    ,10, 8,"EXIT AND SAVE"},
     { -1,EXITMENUY+16    ,10, 8,"EXIT WITHOUT SAVING"},
     { -1,EXITMENUY+32    ,10, 8,"CANCEL AND RETURN TO MENU"},
     { 48,MOUSEMENUY+00   ,10, 8,"MOUSE ENABLED......"},
     { 48,MOUSEMENUY+16   ,10, 8,"BUTTON 1...."},
     { 48,MOUSEMENUY+32   ,10, 8,"BUTTON 2...."},
     { 48,MOUSEMENUY+64   ,10, 8,"BACK TO CONTROL MENU"},
     { 48,JSTICKMENUY+00  ,10, 8,"JOYSTICK ENABLED..."},
     { 48,JSTICKMENUY+16  ,10, 8,"BUTTON 1.."},
     { 48,JSTICKMENUY+32  ,10, 8,"BUTTON 2.."},
     { 48,JSTICKMENUY+48  ,10, 8,"BUTTON 3.."},
     { 48,JSTICKMENUY+64  ,10, 8,"BUTTON 4.."},
     { 48,JSTICKMENUY+96  ,10, 8,"BACK TO CONTROL MENU"},
     
     {344,ACTIONMENUY+10  ,10, 8,"RUN MODE"},
     {344,ACTIONMENUY+30  ,10, 8,"STRAFE MODE"},
     {344,ACTIONMENUY+50  ,10, 8,"USE WEAPON"},
     {344,ACTIONMENUY+70  ,10, 8,"OPEN/CLOSE"},
     
     {344,ACTIONMENUY+90  ,10, 8,"JUMP"},
     {344,ACTIONMENUY+110  ,10, 8,"CROUCH"},

     
     {344,ACTIONMENUY+60  ,10, 8,NULL},
     {344,ACTIONMENUY+70  ,10, 8,NULL},
     {344,ACTIONMENUY+80  ,10, 8,NULL},
     {344,ACTIONMENUY+90  ,10, 8,NULL},
     {344,ACTIONMENUY+100 ,10, 8,NULL},
     {344,ACTIONMENUY+110 ,10, 8,NULL},
     {344,ACTIONMENUY+120 ,10, 8,NULL},
     {344,ACTIONMENUY+130 ,10, 8,NULL},
     {344,ACTIONMENUY+140 ,10, 8,NULL},
     {344,ACTIONMENUY+150 ,10, 8,NULL},
     {344,ACTIONMENUY+160 ,10, 8,NULL},
     {344,ACTIONMENUY+170 ,10, 8,NULL},
     {344,ACTIONMENUY+180 ,10, 8,NULL}
     
     #if 0
     {344,ACTIONMENUY+40  ,10, 8,"JUMP"},
     {344,ACTIONMENUY+50  ,10, 8,"CROUCH"},
     {344,ACTIONMENUY+60  ,10, 8,"LOOK UP"},
     {344,ACTIONMENUY+70  ,10, 8,"LOOK DOWN"},
     {344,ACTIONMENUY+80  ,10, 8,"CENTER VIEW"},
     {344,ACTIONMENUY+90  ,10, 8,"STRAFE LEFT"},
     {344,ACTIONMENUY+100 ,10, 8,"STRAFE RIGHT"},
     {344,ACTIONMENUY+110 ,10, 8,"VIEW 2D MAP"},
     {344,ACTIONMENUY+120 ,10, 8,"ZOOM IN"},
     {344,ACTIONMENUY+130 ,10, 8,"ZOOM OUT"},
     {344,ACTIONMENUY+140 ,10, 8,"USE POTION"},
     {344,ACTIONMENUY+150 ,10, 8,"CAST SPELL"},
     {344,ACTIONMENUY+160 ,10, 8,"LOOK UP/DOWN MODE"},
     {344,ACTIONMENUY+170 ,10, 8,"FLY UP"},
     {344,ACTIONMENUY+180 ,10, 8,"FLY DOWN"}
     #endif
};

int  menustarts[]={
     MAINMENUSTART,CONTROLMENUSTART,SOUNDMENUSTART,VIDEOMENUSTART,
     EXITMENUSTART,MOUSEMENUSTART,JSTICKMENUSTART,ACTIONMENUSTART
},menuends[]={
     MAINMENUEND,CONTROLMENUEND,SOUNDMENUEND,VIDEOMENUEND,
     EXITMENUEND,MOUSEMENUEND,JSTICKMENUEND,ACTIONMENUEND
};


int  musicon=1,
     soundon=1;

char *musiconstr={"MUSIC........... ON"},
     *musicoffstr={"MUSIC........... OFF"},
     *soundonstr={"SOUND EFFECTS... ON"},
     *soundoffstr={"SOUND EFFECTS... OFF"};


void (far *oldkeyint)(void);

int
getBIOSvmode(void)
{
     union REGPACK regs;

     memset(&regs,0,sizeof(union REGPACK));
     regs.h.ah=0x0F;
     regs.h.al=0x00;
     intr(0x10,&regs);
     return(regs.h.al);
}

void
setBIOSvmode(int m)
{
     union REGPACK regs;

     memset(&regs,0,sizeof(union REGPACK));
     regs.h.ah=0x00;
     regs.h.al=m;
     intr(0x10,&regs);
}

void interrupt far
newkeyint(void)
{
     if (inp(0x64)&0x01) {         /* read the status reg, output buf full?*/
          keyc=inp(0x60);          /* read the scan code..                 */
          outp(0x20,0x20);         /* send EOI code (end of interrupt)     */
     }
     _enable();                    /* turn on interrupt processing.             */
}

char
getkey(void)
{
     keyc=0;
     while (keyc == 0);
     return(keyc);
}

char
newgetch(void)
{
     int  extended=0;

     oldkeyint=_dos_getvect(0x09);
     _dos_setvect(0x09,newkeyint);
more:
     getkey();
     if (keyc == 224) {
          extended=1;
          getkey();
          if (keyc == 42 || keyc == 170) {
               goto more;
          }
     }
     else if (keyc > 128) {
          goto more;
     }
     _dos_setvect(0x09,oldkeyint);
     return(keyc+(extended<<7));
}

int
resetmouse(void)
{
     union REGS regs;

     regs.x.ax=0x0000;
     int86(0x33,&regs,&regs);
     if (regs.x.ax != 0) {
          regs.x.ax=0x0021;
          int86(0x33,&regs,&regs);
          return(1);
     }
     return(0);
}

void
showmouse(void)
{
     union REGS regs;

     if (mouse) {
          if (!mouseon) {
               regs.x.ax=0x0001;
               int86(0x33,&regs,&regs);
               mouseon=1;
          }
     }
}

void
hidemouse(void)
{
     union REGS regs;

     if (mouse) {
          if (mouseon) {
               regs.x.ax=0x0002;
               int86(0x33,&regs,&regs);
               mouseon=0;
          }
     }
}

void
getmouse(int *x,int *y,int *b)
{
     union REGS regs;

     if (mouse) {
          regs.x.ax=0x0003;
          int86(0x33,&regs,&regs);
          *x=regs.x.cx;
          *y=regs.x.dx;
          *b=regs.x.bx;
     }
}

void
gprintf(short x,short y,short color,char *stg,...)
{
     va_list valist;

     va_start(valist,stg);
     vsprintf(textbuf,stg,valist);
     va_end(valist);
     if (x == -1) {
          x=(vc.numxpixels>>1)-(strlen(textbuf)<<2);
     }
     if (y == -1) {
          y=(vc.numypixels>>1);
     }
     _setcolor(0);
     _moveto(x,y);
     _outgtext(textbuf);
     if (color > 0) {
          _setcolor(color);
          _moveto(x+1,y+1);
          _outgtext(textbuf);
     }
     curx=x;
     cury=y;
}

void
drawheader(void)
{
     _setcolor(7);
     _rectangle(_GFILLINTERIOR,1,1,vc.numxpixels-2,16);
     _setcolor(15);
     _rectangle(_GBORDER,0,0,vc.numxpixels-1,16);
#ifdef TEKWAR
     gprintf(-1,4,0,"TEKWAR SETUP UTILITY");
#else
     gprintf(-1,4,0,"WITCHAVEN SETUP UTILITY");
#endif
}

int
centerx(int menopt)
{
     return((vc.numxpixels>>1)-((strlen(menu[menopt].label)*FONTWIDTH)>>1));
}

void
menuopthilite(int opt,int onoff)
{
     int  x1,y1,x2,y2;

     if (menu[opt].label != NULL) {
          x1=menu[opt].x;
          if (x1 == -1) {
               x1=centerx(opt);
          }
          x2=x1+(strlen(menu[opt].label)*FONTWIDTH);
          y1=menu[opt].y;
          y2=y1+FONTHEIGHT;
          if (onoff) {
               _setcolor(15);
          }
          else {
               _setcolor(menu[opt].back);
          }
          if (opt >= ACTIONMENUSTART && opt < ACTIONMENUEND) {
               _rectangle(_GBORDER,x1-3,y1-2,x2+3,y2+2);
               _rectangle(_GBORDER,x1-2,y1-1,x2+2,y2+1);
          }
          else {
               _rectangle(_GBORDER,x1-3,y1-3,x2+3,y2+3);
               _rectangle(_GBORDER,x1-2,y1-2,x2+2,y2+2);
          }
     }
}

void
drawmenuopt(int oldopt,int newopt)
{
     menuopthilite(oldopt,0);
     menuopthilite(newopt,1);
}

void
drawmainmenu(void)
{
     int  i;

     for (i=MAINMENUSTART ; i < MAINMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
          }
     }
}

void
showmouseline(int i)
{
     int  a;

     if (i == MOUSEMENUSTART) {
          _setcolor(menu[i].back);
          _rectangle(_GFILLINTERIOR,216,menu[i].y,216+48,
               menu[i].y+FONTHEIGHT);
          gprintf(menu[i].x,menu[i].y,menu[i].fore,
               menu[i].label);
          gprintf(216,menu[i].y,14,option[3] == 0 ? "NO" : "YES");
     }
     else if (i < MOUSEMENUEND-1) {
          a=NUMOPTS+NUMKEYS+(i-MOUSEMENUSTART)-2;//was 1
          a=option[a]+ACTIONMENUSTART-4;
          gprintf(216,menu[i].y,14,menu[a].label);
     }
}

void
showmousesettings(void)
{
     int  i;

     for (i=MOUSEMENUSTART ; i < MOUSEMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
               if (i < MOUSEMENUEND-1) {
                    showmouseline(i);
               }
          }
     }
}

void
showjstickline(int i)
{
     int  a;

     if (i == JSTICKMENUSTART) {
          _setcolor(menu[i].back);
          _rectangle(_GFILLINTERIOR,216,menu[i].y,216+48,menu[i].y+FONTHEIGHT);
          gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
          gprintf(216,menu[i].y,14,
               option[NUMOPTS+NUMKEYS+2] == 0 ? "NO" : "YES");
     }
     else {
          a=NUMOPTS+NUMKEYS+3+(i-JSTICKMENUSTART)-1;
          a=option[a]+ACTIONMENUSTART-4;
          gprintf(216,menu[i].y,14,menu[a].label);
     }
}

void
showjsticksettings(void)
{
     int  i;

     for (i=JSTICKMENUSTART ; i < JSTICKMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
               if (i < JSTICKMENUEND-1) {
                    showjstickline(i);
               }
          }
     }
}

void
showcontrolline(int i)
{
     char c;

     c=option[i-CONTROLMENUSTART+NUMOPTS-2];
     if (strcmp(keys[c&0x7F],"CTRL") == 0
        || strcmp(keys[c&0x7F],"ALT") == 0) {
          gprintf(menu[i].x+168,menu[i].y,14,"%s%s",
               (c&0x80) != 0 ? "R" : "L",keys[c&0x7F]);
     }
     else if (strcmp(keys[c&0x7F],"RETURN") == 0) {
          gprintf(menu[i].x+168,menu[i].y,14,"%s",
               (c&0x80) != 0 ? "KP ENTER" : "ENTER");
     }
     else {
          gprintf(menu[i].x+168,menu[i].y,14,keys[c&0x7F]);
     }
}

void
showcontrolsettings(void)
{
     int  i,n;
     char c;

     _setcolor(0);
     _moveto(vc.numxpixels>>1,116);
     _lineto(vc.numxpixels>>1,vc.numypixels-25);
     for (i=CONTROLMENUSTART ; i < CONTROLMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
               if (i >= CONTROLMENUSTART+2) {
                    showcontrolline(i);
               }
          }
     }
}

void
docontrolaction(int domenu,int opt)
{
     int  x1,x2,y1,y2;

     menuopthilite(opt,0);
     x1=menu[opt].x+168;
     y1=menu[opt].y;
     x2=x1+(9*FONTWIDTH);
     y2=y1+FONTHEIGHT;
     _setcolor(0);
     _rectangle(_GFILLINTERIOR,x1,y1,x2,y2);
     option[opt-menustarts[domenu]+NUMOPTS-2]=newgetch();
     _setcolor(menu[opt].back);
     _rectangle(_GFILLINTERIOR,x1,y1,x2,y2);
     menuopthilite(opt,1);
     showcontrolline(opt);
}

void
showsoundsettings(void)
{
     int  i;

     for (i=SOUNDMENUSTART ; i < SOUNDMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
          }
     }
}

void
showmusicsettings(void)
{
     int  i;

     for (i=MUSICMENUSTART ; i < MUSICMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
          }
     }
}

void
showvideosettings(void)
{
     int  i,x1,x2,y1,y2;

     if (option[0] == 1 && option[6] == 0x62) {
          strcpy(textbuf,"VIDEO MODE........... 640 x 480 - 256 colors");
     }
     else if (option[0] == 0 && option[6] == 0x62) {
          strcpy(textbuf,"VIDEO MODE........... 360 x 360 - 256 colors");
     }
     else if (option[0] == 1 && option[6] == 0x61) {
          strcpy(textbuf,"VIDEO MODE........... 640 x 400 - 256 colors");
     }
     else {
          strcpy(textbuf,"VIDEO MODE........... 320 x 200 - 256 colors");
          option[0]=1;
          option[6]=0x60;
     }
     x1=(vc.numxpixels>>1)-((strlen(textbuf)*FONTWIDTH)>>1);
     x2=x1+(strlen(textbuf)*FONTWIDTH);
     y1=152;
     y2=y1+FONTHEIGHT;
     _setcolor(menu[VIDEOMENUSTART].back);
     _rectangle(_GFILLINTERIOR,x1,y1,x2,y2);
     gprintf(x1,y1,14,textbuf);
     for (i=VIDEOMENUSTART ; i < VIDEOMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
          }
     }
}

void
showexitsettings(void)
{
     int  i;

     for (i=EXITMENUSTART ; i < EXITMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
          }
     }
}

void
showactionsettings(void)
{
     int  i;

     for (i=ACTIONMENUSTART ; i < ACTIONMENUEND ; i++) {
          if (menu[i].label != NULL) {
               gprintf(menu[i].x,menu[i].y,menu[i].fore,menu[i].label);
          }
     }
}

void
drawoptsettings(int opt)
{
     _setcolor(8);
     if (opt+1 == ACTIONMENU) {
          _rectangle(_GFILLINTERIOR,vc.numxpixels>>1,117,
               vc.numxpixels-24,vc.numypixels-24);
     }
     else {
          _rectangle(_GFILLINTERIOR,24,100,vc.numxpixels-24,vc.numypixels-24);
     }
     _setcolor(15);
     _rectangle(_GBORDER,24,100,vc.numxpixels-24,vc.numypixels-24);
     if (opt+1 != ACTIONMENU) {
          gprintf(-1,104,15,"CURRENT SETTINGS");
          _setcolor(0);
          _moveto(25,116);
          _lineto(vc.numxpixels-25,116);
     }
     switch (opt+1) {
     case CONTROLMENU:
          showcontrolsettings();
          break;
     case SOUNDMENU:
          showsoundsettings();
          break;
     case VIDEOMENU:
          showvideosettings();
          break;
     case EXITMENU:
          showexitsettings();
          break;
     case MOUSEMENU:
          showmousesettings();
          break;
     case JSTICKMENU:
          showjsticksettings();
          break;
     case ACTIONMENU:
          showactionsettings();
          break;
     }
}

void
drawmain(int domenu)
{
     _setcolor(1);
     _rectangle(_GFILLINTERIOR,0,0,vc.numxpixels-1,vc.numypixels-1);
     _setcolor(15);
     _rectangle(_GBORDER,0,0,vc.numxpixels-1,vc.numypixels-1);
     drawheader();
     drawmainmenu();
     drawoptsettings(domenu);
     drawmenuopt(selopt,selopt);
}

void
runaudioexe(int domenu)
{
     hidemouse();
     _setvideomode(_TEXTC80);
     spawnl(P_WAIT,"audio.exe","audio.exe",NULL);
     _setvideomode(_ERESCOLOR);
     drawmain(domenu-1);
     showmouse();
}

void
main(void)
{
     int  domenu=0,done=0,i,len,mousemenu=0,omode,oldselopt,x,x1,x2,y1,y2;
     int  butopt,savdomenu,savselopt;
     char *crashstr=NULL;
     char temp[10]={"ver1"};
     FILE *fp;
     void func1(void);

     if (access("ver1.dat",F_OK) != 0) {
        fp=fopen("ver1.dat","w");
        if( fp != NULL ) {
            fwrite(temp,sizeof(temp),1,fp);
            fclose(fp);
        }
     }

     if (access("setup.dat",F_OK) == 0) {
          fp=fopen("setup.dat","rb");
          fread(option,sizeof(char),NUMOPTS+NUMKEYS+MOREOPTS,fp);
          fclose(fp);
     }
     else {
          option[0]=1;
          option[1]=1;
          option[2]=1;
          option[3]=1;
          option[4]=0;
          option[5]=0;
          option[6]=0x60;
          option[7]=0x16;
#ifndef TEKWAR
          option[8]=0;
          option[9]=0;
          option[10]=0;
          option[11]=0;
#endif
          for (i=0 ; i < NUMKEYS ; i++) {
               option[i+NUMOPTS]=defkeys[i];
          }
     }
/*
#ifndef TEKWAR
     musicon=option[8];
     if (option[8] == 0) {
          menu[menustarts[SOUNDMENU]].label=musicoffstr;
     }
     else {
          menu[menustarts[SOUNDMENU]].label=musiconstr;
     }
     soundon=option[10];
     if (option[10] == 0) {
          menu[menustarts[SOUNDMENU]+1].label=soundoffstr;
     }
     else {
          menu[menustarts[SOUNDMENU]+1].label=soundonstr;
     }
#endif
*/
#ifndef TEKWAR
     musicon=option[9];
     if (option[9] == 0) {
          menu[menustarts[SOUNDMENU]].label=musicoffstr;
     }
     else {
          menu[menustarts[SOUNDMENU]].label=musiconstr;
     }               
     soundon=option[11];
     if (option[11] == 0) {
          menu[menustarts[SOUNDMENU]+1].label=soundoffstr;
     }
     else {
          menu[menustarts[SOUNDMENU]+1].label=soundonstr;
     }
#endif

     omode=getBIOSvmode();
     _setvideomode(_ERESCOLOR);
     _getvideoconfig(&vc);
     _gettextsettings(&ts);
     drawheader();
     gprintf(0,20,7,"Initializing...");
     mouse=resetmouse();
     drawmain(domenu);
     showmouse();
     oldselopt=selopt=0;
     do {
          if (!kbhit()) {
               goto skipkeys;
          }
          if (mouseon) {
               hidemouse();
          }
          switch (getch()) {
          case 0:
               switch (getch()) {
               case 75:            // left arrow
                    break;
               case 77:            // right arrow
                    break;
               case 72:            // up arrow
                    do {
                         selopt--;
                         if (selopt < menustarts[domenu]) {
                              selopt=menuends[domenu]-1;
                         }
                    } while (menu[selopt].label == NULL);
                    drawmenuopt(oldselopt,selopt);
                    if (domenu == 0) {
                         drawoptsettings(selopt);
                    }
                    oldselopt=selopt;
                    break;
               case 80:            // down arrow
                    do {
                         selopt++;
                         if (selopt >= menuends[domenu]) {
                              selopt=menustarts[domenu];
                         }
                    } while (menu[selopt].label == NULL);
                    drawmenuopt(oldselopt,selopt);
                    if (domenu == 0) {
                         drawoptsettings(selopt);
                    }
                    oldselopt=selopt;
                    break;
               case 73:            // pgup
                    break;
               case 81:            // pgdn
                    break;
               case 68:            // F10
                    done=1;
                    break;
               default:
                    break;
               }
               break;
          case 0x20:               // spacebar
               break;
          case 0x0D:               // enter key
               switch (domenu) {
               case MAINMENU:      // main menu
                    if (selopt < MAINMENUOPTIONS) {
                         domenu=selopt+1;
                         selopt=menustarts[domenu];
                         drawmenuopt(oldselopt,selopt);
                         oldselopt=selopt;
                    }
                    break;
               case CONTROLMENU:   // control menu
                    if (selopt < menustarts[domenu]+2) {
                         _setcolor(8);
                         _rectangle(_GFILLINTERIOR,24,100,vc.numxpixels-24,
                              vc.numypixels-24);
                         _setcolor(15);
                         _rectangle(_GBORDER,24,100,vc.numxpixels-24,
                              vc.numypixels-24);
                         gprintf(-1,104,15,"CURRENT SETTINGS");
                         _setcolor(0);
                         _moveto(25,116);
                         _lineto(vc.numxpixels-25,116);
                         if (selopt == menustarts[domenu]) {
                              domenu=MOUSEMENU;
                         }
                         else {
                              domenu=JSTICKMENU;
                         }
                         drawoptsettings(domenu-1);
                         oldselopt=selopt=menustarts[domenu];
                         menuopthilite(selopt,1);
                    }
                    else {
                         docontrolaction(domenu,selopt);
                    }
                    break;
               case SOUNDMENU:     // sound menu
                    switch (selopt) {
#ifdef TEKWAR
                    case SOUNDMENUSTART:
                         runaudioexe(domenu);
                         break;
                    case SOUNDMENUSTART+1:
                         menuopthilite(selopt,0);
                         selopt=domenu-1;
                         menuopthilite(selopt,1);
                         oldselopt=selopt;
                         domenu=0;
                         break;
#else
                    case SOUNDMENUSTART:
                         _setcolor(8);
                         _rectangle(_GFILLINTERIOR,234,136,415,157);
                         if (musicon) {
                              menu[selopt].label=musicoffstr;
                              musicon=0;
                         }
                         else {
                              menu[selopt].label=musiconstr;
                              musicon=1;
                         }
                         option[9]=musicon;
                         showsoundsettings();
                         drawmenuopt(selopt,selopt);
                         break;
                    case SOUNDMENUSTART+1:
                         _setcolor(8);
                         _rectangle(_GFILLINTERIOR,234,152,415,172);
                         if (soundon) {
                              menu[selopt].label=soundoffstr;
                              soundon=0;
                         }
                         else {
                              menu[selopt].label=soundonstr;
                              soundon=1;
                         }
                         option[11]=soundon;
                         showsoundsettings();
                         drawmenuopt(selopt,selopt);
                         break;
                    case SOUNDMENUSTART+2:
                         runaudioexe(domenu);
                         break;
                    case SOUNDMENUSTART+3:
                         menuopthilite(selopt,0);
                         selopt=domenu-1;
                         menuopthilite(selopt,1);
                         oldselopt=selopt;
                         domenu=0;
                         break;
#endif
                    }
                    break;
//               case MUSICMENU:     // music menu
//                    break;
               case VIDEOMENU:     // video menu
                    switch (selopt) {
                    case VIDEOMENUSTART:
                         option[0]=1;
                         option[6]=0x60;
                         break;
#ifdef TEKWAR
                    case VIDEOMENUSTART+1:
                         option[0]=0;
                         option[6]=0x62;
                         break;
                    case VIDEOMENUSTART+2:
                         option[0]=1;
                         option[6]=0x61;
                         break;
                    case VIDEOMENUSTART+3:
                         option[0]=1;
                         option[6]=0x62;
                         break;
#else                         
                    case VIDEOMENUSTART+1:
                         option[0]=1;
                         option[6]=0x62;
                         break;
#endif
                    }
                    menuopthilite(selopt,0);
                    selopt=domenu-1;
                    menuopthilite(selopt,1);
                    oldselopt=selopt;
                    showvideosettings();
                    domenu=0;
                    break;
               case EXITMENU:
                    switch (selopt) {
                    case EXITMENUSTART:
                         //here
                         fp=fopen("setup.dat","wb");
                         if (fp != NULL) {
                              fwrite(option,sizeof(char),NUMOPTS+NUMKEYS+MOREOPTS,fp);
                              fclose(fp);
                              done=1;
                              atexit( func1 );
                         }
                         else {
                              done=2;
                         }
                         break;
                    case EXITMENUSTART+1:
                         done=1;
                         break;
                    default:
                         menuopthilite(selopt,0);
                         selopt=domenu-1;
                         menuopthilite(selopt,1);
                         oldselopt=selopt;
                         domenu=0;
                         break;
                    }
                    break;
               case MOUSEMENU:
                    switch (selopt) {
                    case MOUSEMENUSTART:
                         option[3]^=1;
                         _setcolor(menu[selopt].back);
                         _rectangle(_GFILLINTERIOR,216,menu[selopt].y,
                              216+48,menu[selopt].y+FONTHEIGHT);
                         gprintf(menu[selopt].x,menu[selopt].y,
                              menu[selopt].fore,menu[selopt].label);
                         gprintf(216,menu[selopt].y,14,
                              option[3] == 0 ? "NO" : "YES");
                         break;
                    case MOUSEMENUSTART+1:
                    case MOUSEMENUSTART+2:
                         butopt=NUMOPTS+NUMKEYS;
                         if (selopt == MOUSEMENUSTART+2) {
                              butopt++;
                         }
                         menuopthilite(selopt,0);
                         savdomenu=domenu;
                         domenu=ACTIONMENU;
                         savselopt=selopt;
                         oldselopt=selopt=menustarts[domenu];
                         drawoptsettings(domenu-1);
                         menuopthilite(selopt,1);
                         break;
                    case MOUSEMENUSTART+3:
                         domenu=CONTROLMENU;
                         oldselopt=selopt=menustarts[domenu];
                         drawoptsettings(domenu-1);
                         menuopthilite(selopt,1);
                         break;
                    }
                    break;
               case JSTICKMENU:
                    switch (selopt) {
                    case JSTICKMENUSTART:
                         option[NUMOPTS+NUMKEYS+2]^=1;
                         _setcolor(menu[selopt].back);
                         _rectangle(_GFILLINTERIOR,216,menu[selopt].y,
                              216+48,menu[selopt].y+FONTHEIGHT);
                         gprintf(menu[selopt].x,menu[selopt].y,
                              menu[selopt].fore,menu[selopt].label);
                         gprintf(216,menu[selopt].y,14,
                              option[NUMOPTS+NUMKEYS+2] == 0 ? "NO" : "YES");
                         break;
                    case JSTICKMENUSTART+1:
                    case JSTICKMENUSTART+2:
                    case JSTICKMENUSTART+3:
                    case JSTICKMENUSTART+4:
                         butopt=(NUMOPTS+NUMKEYS)+(selopt-(JSTICKMENUSTART+1))+3;
                         menuopthilite(selopt,0);
                         savdomenu=domenu;
                         domenu=ACTIONMENU;
                         savselopt=selopt;
                         oldselopt=selopt=menustarts[domenu];
                         drawoptsettings(domenu-1);
                         menuopthilite(selopt,1);
                         break;
                    case JSTICKMENUSTART+5:
                         domenu=CONTROLMENU;
                         oldselopt=selopt=menustarts[domenu];
                         drawoptsettings(domenu-1);
                         menuopthilite(selopt,1);
                         break;
                    }
                    break;
               case ACTIONMENU:
                    i=(selopt-ACTIONMENUSTART)+4;
                    option[butopt]=i;
                    _setcolor(8);
                    _rectangle(_GFILLINTERIOR,vc.numxpixels>>1,117,
                         vc.numxpixels-24,vc.numypixels-24);
                    domenu=savdomenu;
                    oldselopt=selopt=savselopt;
                    drawoptsettings(domenu-1);
                    menuopthilite(selopt,1);
                    break;
               }
               break;
          case 0x1B:               // ESC key
               if (domenu != 0) {
                    menuopthilite(selopt,0);
                    if (domenu == MOUSEMENU || domenu == JSTICKMENU
                       || domenu == ACTIONMENU) {
                         if (domenu == ACTIONMENU) {
                              domenu=savdomenu;
                         }
                         else {
                              domenu=CONTROLMENU;
                         }
                         selopt=menustarts[domenu];
                         drawoptsettings(domenu-1);
                    }
                    else {
                         selopt=domenu-1;
                         domenu=0;
                    }
                    menuopthilite(selopt,1);
                    oldselopt=selopt;
               }
               else {
                    done=1;
               }
               break;
          default:
               break;
          }
skipkeys:
          if (mouse) {
               getmouse(&mx,&my,&mb);
               if (mx != oldmx || my != oldmy || mb != oldmb) {
                    if (mouseon) {
                         hidemouse();
                    }
                    _setcolor(1);
                    _rectangle(_GFILLINTERIOR,1,vc.numypixels-12,
                         vc.numxpixels-1,vc.numypixels-1);
//                    gprintf(0,vc.numypixels-10,15,"mx=%d my=%d mb=%X",
//                         mx,my,mb);
                    oldmx=mx;
                    oldmy=my;
                    oldmb=mb;
                    if ((mb&1) != 0) {
                         switch (mousemenu) {
                         case MAINMENU:
                              if (selopt < menuends[mousemenu]) {
                                   drawoptsettings(selopt);
                                   domenu=selopt+1;
                              }
                              break;
                         case CONTROLMENU:
                              if (selopt < menustarts[domenu]+2) {
                                   _setcolor(8);
                                   _rectangle(_GFILLINTERIOR,24,100,vc.numxpixels-24,
                                        vc.numypixels-24);
                                   _setcolor(15);
                                   _rectangle(_GBORDER,24,100,vc.numxpixels-24,
                                        vc.numypixels-24);
                                   gprintf(-1,104,15,"CURRENT SETTINGS");
                                   _setcolor(0);
                                   _moveto(25,116);
                                   _lineto(vc.numxpixels-25,116);
                                   if (selopt == menustarts[domenu]) {
                                        domenu=MOUSEMENU;
                                   }
                                   else {
                                        domenu=JSTICKMENU;
                                   }
                                   mousemenu=domenu;
                                   drawoptsettings(domenu-1);
                                   oldselopt=selopt=menustarts[domenu];
                                   menuopthilite(selopt,1);
                              }
                              else {
                                   docontrolaction(domenu,selopt);
                              }
                              break;
                         case SOUNDMENU:
                              switch (selopt) {
#ifdef TEKWAR
                              case SOUNDMENUSTART:
                                   runaudioexe(mousemenu);
                                   break;
                              case SOUNDMENUSTART+1:
                                   menuopthilite(selopt,0);
                                   selopt=domenu-1;
                                   menuopthilite(selopt,1);
                                   oldselopt=selopt;
                                   domenu=0;
                                   break;
#else
                              case SOUNDMENUSTART:
                                   _setcolor(8);
                                   _rectangle(_GFILLINTERIOR,234,136,415,157);
                                   if (musicon) {
                                        menu[selopt].label=musicoffstr;
                                        musicon=0;
                                   }
                                   else {
                                        menu[selopt].label=musiconstr;
                                        musicon=1;
                                   }
                                   option[8]=musicon;
                                   showsoundsettings();
                                   drawmenuopt(selopt,selopt);
                                   break;
                              case SOUNDMENUSTART+1:
                                   _setcolor(8);
                                   _rectangle(_GFILLINTERIOR,234,152,415,172);
                                   if (soundon) {
                                        menu[selopt].label=soundoffstr;
                                        soundon=0;
                                   }
                                   else {
                                        menu[selopt].label=soundonstr;
                                        soundon=1;
                                   }
                                   option[10]=soundon;
                                   showsoundsettings();
                                   drawmenuopt(selopt,selopt);
                                   break;
                              case SOUNDMENUSTART+2:
                                   runaudioexe(mousemenu);
                                   break;
                              case SOUNDMENUSTART+3:
                                   menuopthilite(selopt,0);
                                   selopt=domenu-1;
                                   menuopthilite(selopt,1);
                                   oldselopt=selopt;
                                   domenu=0;
                                   break;
#endif
                              }
                              break;
//                         case MUSICMENU:
//                              break;
                         case VIDEOMENU:
                              switch (selopt) {
                              case VIDEOMENUSTART:
                                   option[0]=1;
                                   option[6]=0x60;
                                   break;
#ifdef TEKWAR
                              case VIDEOMENUSTART+1:
                                   option[0]=0;
                                   option[6]=0x62;
                                   break;
                              case VIDEOMENUSTART+2:
                                   option[0]=1;
                                   option[6]=0x61;
                                   break;
                              case VIDEOMENUSTART+3:
                                   option[0]=1;
                                   option[6]=0x62;
                                   break;
#else
                              case VIDEOMENUSTART+1:
                                   option[0]=1;
                                   option[6]=0x62;
                                   break;
#endif
                              }
                              showvideosettings();
                              break;
                         case EXITMENU:
                              switch (selopt) {
                              case EXITMENUSTART:
                                   //here
                                   fp=fopen("setup.dat","wb");
                                   if (fp != NULL) {
                                        fwrite(option,sizeof(char),NUMOPTS+NUMKEYS+MOREOPTS,fp);
                                        fclose(fp);
                                        done=1;
                                        atexit( func1 );
                                   }
                                   else {
                                        done=2;
                                   }
                                   break;
                              case EXITMENUSTART+1:
                                   done=1;
                                   break;
                              default:
                                   menuopthilite(selopt,0);
                                   selopt=domenu-1;
                                   menuopthilite(selopt,1);
                                   oldselopt=selopt;
                                   domenu=0;
                                   break;
                              }
                              break;
                         case MOUSEMENU:
                              switch (selopt) {
                              case MOUSEMENUSTART:
                                   option[3]^=1;
                                   _setcolor(menu[selopt].back);
                                   _rectangle(_GFILLINTERIOR,216,menu[selopt].y,
                                        216+48,menu[selopt].y+FONTHEIGHT);
                                   gprintf(menu[selopt].x,menu[selopt].y,
                                        menu[selopt].fore,menu[selopt].label);
                                   gprintf(216,menu[selopt].y,14,
                                        option[3] == 0 ? "NO" : "YES");
                                   break;
                              case MOUSEMENUSTART+1:
                              case MOUSEMENUSTART+2:
                                   butopt=NUMOPTS+NUMKEYS;
                                   if (selopt == MOUSEMENUSTART+2) {
                                        butopt++;
                                   }
                                   menuopthilite(selopt,0);
                                   savdomenu=domenu;
                                   domenu=ACTIONMENU;
                                   savselopt=selopt;
                                   oldselopt=selopt=menustarts[domenu];
                                   drawoptsettings(domenu-1);
                                   menuopthilite(selopt,1);
                                   break;
                              case MOUSEMENUSTART+3:
                                   domenu=CONTROLMENU;
                                   oldselopt=selopt=menustarts[domenu];
                                   drawoptsettings(domenu-1);
                                   menuopthilite(selopt,1);
                                   break;
                              }
                              break;
                         case JSTICKMENU:
                              switch (selopt) {
                              case JSTICKMENUSTART:
                                   option[NUMOPTS+NUMKEYS+2]^=1;
                                   _setcolor(menu[selopt].back);
                                   _rectangle(_GFILLINTERIOR,216,menu[selopt].y,
                                        216+48,menu[selopt].y+FONTHEIGHT);
                                   gprintf(menu[selopt].x,menu[selopt].y,
                                        menu[selopt].fore,menu[selopt].label);
                                   gprintf(216,menu[selopt].y,14,
                                        option[NUMOPTS+NUMKEYS+2] == 0 ? "NO" : "YES");
                                   break;
                              case JSTICKMENUSTART+1:
                              case JSTICKMENUSTART+2:
                              case JSTICKMENUSTART+3:
                              case JSTICKMENUSTART+4:
                                   butopt=(NUMOPTS+NUMKEYS)+(selopt-(JSTICKMENUSTART+1))+3;
                                   menuopthilite(selopt,0);
                                   savdomenu=domenu;
                                   domenu=ACTIONMENU;
                                   savselopt=selopt;
                                   oldselopt=selopt=menustarts[domenu];
                                   drawoptsettings(domenu-1);
                                   menuopthilite(selopt,1);
                                   break;
                              case JSTICKMENUSTART+5:
                                   domenu=CONTROLMENU;
                                   oldselopt=selopt=menustarts[domenu];
                                   drawoptsettings(domenu-1);
                                   menuopthilite(selopt,1);
                                   break;
                              }
                              break;
                         case ACTIONMENU:
                              i=(selopt-ACTIONMENUSTART)+4;
                              option[butopt]=i;
                              _setcolor(8);
                              _rectangle(_GFILLINTERIOR,vc.numxpixels>>1,100,
                                   vc.numxpixels-24,vc.numypixels-24);
                              domenu=savdomenu;
                              oldselopt=selopt=savselopt;
                              drawoptsettings(domenu-1);
                              menuopthilite(selopt,1);
                              break;
                         }
                    }
                    for (i=menustarts[0] ; i < menuends[0] ; i++) {
                         if (menu[i].label == NULL) {
                              continue;
                         }
                         len=strlen(menu[i].label)*FONTWIDTH;
                         if ((x=menu[i].x) == -1) {
                              x=centerx(i);
                         }
                         if (mx > x && mx < x+len) {
                              if (my > menu[i].y && my < menu[i].y+FONTHEIGHT) {
                                   if (i != selopt) {
                                        selopt=i;
                                        drawmenuopt(oldselopt,selopt);
                                        oldselopt=selopt;
                                        mousemenu=0;
                                   }
                              }
                         }
                    }
                    if (domenu != 0) {
                         for (i=menustarts[domenu] ; i < menuends[domenu] ; i++) {
                              if (menu[i].label == NULL) {
                                   continue;
                              }
                              len=strlen(menu[i].label)*FONTWIDTH;
                              if ((x=menu[i].x) == -1) {
                                   x=centerx(i);
                              }
                              if (mx > x && mx < x+len) {
                                   if (my > menu[i].y && my < menu[i].y+FONTHEIGHT) {
                                        if (i != selopt) {
                                             selopt=i;
                                             drawmenuopt(oldselopt,selopt);
                                             oldselopt=selopt;
                                             mousemenu=domenu;
                                        }
                                   }
                              }
                         }
                    }
                    showmouse();
               }
          }
     } while (!done);
     if (mouseon) {
          hidemouse();
     }
     setBIOSvmode(omode);
     if (crashstr != NULL) {
          printf("\nERROR: %s\n",crashstr);
          getch();
          exit(1);
     }

     //printf("\nType WHAVEN to run the game.\n");

     exit(0);
}

void func1(void) {

     printf("\nType WHAVEN to run the game.\n");

}
