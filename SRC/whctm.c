/***************************************************************************
 *   CTM.C  - CyberMAXX support routines                                   *
 *            uses: SERIAL1.LIB functions                                  *
 *   Written by: Les Bird                                                  *
 ***************************************************************************/

#define   DOS4G

#include "icorp.h"
#include <time.h>
#include "3d.h"
#include "commlib.h"

PORT *sd;

int  iglassenabled;

void
ctm_deinit(void)
{
     if (cyberenabled) {
          PortClose(sd);
          cyberenabled=0;
     }
}

int
ctm_init(int port)
{
     long cybcode=0,delayclock;

     sd=PortOpenGreenleafFast(port,9600L,'N',8,1);
     if (sd->status == ASSUCCESS) {
          cyberenabled=1;
          WriteChar(sd,'X');
          delayclock=totalclock+CLKIPS;
          while (totalclock < delayclock && cybcode < 3) {
               if (SpaceUsedInRXBuffer(sd) == 0) {
                    continue;
               }
               switch (cybcode) {
               case 0:
                    if (ReadChar(sd) == 'C') {
                         cybcode++;
                    }
                    break;
               case 1:
                    if (ReadChar(sd) == 'T') {
                         cybcode++;
                    }
                    break;
               case 2:
                    if (ReadChar(sd) == 'M') {
                         cybcode++;
                    }
                    break;
               }
          }
          if (cybcode != 3) {
               ctm_deinit();
               return(0);
          }
          WriteChar(sd,'C');       // CyberMAXX absolute coordinate mode
          WriteChar(sd,'F');       // CyberMAXX binary transfer mode
          WriteChar(sd,'M');       // CyberMAXX checksum byte
          WriteChar(sd,'G');       // CyberMAXX continuous sample mode
          cyberenabled=port+1;
          return(1);
     }
     return(0);
}

#if 0
short
getserch(PORT *s)
{
     return((short)ReadChar(s));
}
#endif

void
vio_deinit(void)
{
     if (iglassenabled) {
          PortClose(sd);
          iglassenabled=0;
     }
}

void
vio_reset(void)
{
     WriteChar(sd,'!');
     WriteChar(sd,'R');
     WriteChar(sd,0x0D);
}

void
vio_setup(void)
{
     WriteChar(sd,'!');
     WriteChar(sd,'M');
     WriteChar(sd,'2');  // cooked data mode
     WriteChar(sd,',');
     WriteChar(sd,'P');  // polled
     WriteChar(sd,',');
     WriteChar(sd,'B');  // binary
     WriteChar(sd,0x0D);
}

int
vio_init(int port)
{
     long delayclock,tries=0;

     sd=PortOpenGreenleafFast(port,9600L,'N',8,1);
     if (sd->status == ASSUCCESS) {
          iglassenabled=0;
          vio_reset();
          delayclock=totalclock+(CLKIPS>>1);
          while (totalclock < delayclock && iglassenabled == 0 && tries < 10) {
               if (SpaceUsedInRXBuffer(sd) == 0) {
                    continue;
               }
               if (ReadChar(sd) == 'O') {
                    iglassenabled=port+1;
               }
               else {
                    vio_reset();
                    delayclock=totalclock+(CLKIPS>>1);
                    tries++;
               }
          }
          if (iglassenabled == 0) {
               vio_deinit();
               return(0);
          }
          vio_setup();
          return(1);
     }
     return(0);
}

void
vio_read(short *yaw,short *pitch,short *roll)
{
     short c,chk,chk2,yaw2,pitch2,roll2;
     long delayclock;
     PORT *s;

     s=sd;
     if (SpaceUsedInTXBuffer(s) == 0) {
          WriteChar(s,'S');
     }
     if (iglassenabled) {
          if (SpaceUsedInRXBuffer(s) < 8) {
               return;
          }
     }
     else if (cyberenabled) {
          if (SpaceUsedInRXBuffer(s) < 7) {
               return;
          }
     }
     chk=0;
     pitch2=yaw2=roll2=0;
     if ((c=ReadChar(s)) == 0xFF || cyberenabled) {
          if (iglassenabled) {
               chk+=c;
               yaw2=((c=ReadChar(s))<<8);    // yaw high byte
          }
          else {
               yaw2=c<<8;
          }
          chk+=c;
          yaw2+=(c=ReadChar(s));        // yaw low byte
          chk+=c;
          pitch2=((c=ReadChar(s))<<8);  // pitch high byte
          chk+=c;
          pitch2+=(c=ReadChar(s));      // pitch low byte
          chk+=c;
          roll2=((c=ReadChar(s))<<8);   // roll high byte
          chk+=c;
          roll2+=(c=ReadChar(s));       // roll low byte
          chk+=c;
          chk2=ReadChar(s);             // checksum
          if ((chk&0xFF) == chk2 || cyberenabled) {
               *yaw=yaw2;
               *pitch=pitch2;
               *roll=roll2;
          }
     }
     ClearTXBuffer(s);
     ClearRXBuffer(s);
}

extern
short pitch,
     roll,
     yaw;

void
whvrmon(void)
{
#if 0
     char tmpbuf[80];

     sprintf(tmpbuf,"Y: %05d P: %05d R: %05d C: %05d",yaw,pitch,roll,
          sd.rx.q->count);
     printext256(0L,8L,31,-1,tmpbuf,1);
#endif
}

