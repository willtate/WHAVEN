#include "cd_heads.h"


void Init_CD( void )
{

   asm {
      mov   ax,1500h
      mov   bx,0
      int   2Fh
      mov   [flag],bx
      }
	if (!flag) {
		printf("MSCDEX not installed\n");
		exit(1);
		}

     devptr=&drv_lst[0];
     asm {
          mov  ax,1501h
          les  bx,dword ptr [devptr]
          int  2Fh
     }
     hdrptr=(struct dev_hdr far *)drv_lst[0].dev_hdr;
     printf("\nCD-ROM header contents at address %Fp\n",hdrptr);
     printf("\n...next driver = %Fp",hdrptr->nxtdrv);
     printf("\n...attributes  = %X",hdrptr->devattr);
     printf("\n...strategy ep = %X",hdrptr->devstrategy);
     printf("\n...interrupt ep= %X",hdrptr->devinterrupt);
     printf("\n...device name = %Fs",hdrptr->devname);
     printf("\n...reserved    = %X",hdrptr->reserved);
     printf("\n...drive letter= %c",'@'+hdrptr->driveletter);
     printf("\n...units       = %d",(int)hdrptr->units);
     printf("\n");
}


void read_toc( struct devlst *drv )
{
struct TnoInfo_Rec far	*t;         /* info on each track */
struct DiskInfo_Rec	DiskInfo;		/* Disk information for CD	*/
unsigned char		*s;
unsigned int		i;
unsigned long     num;

	if (ioctl(drv,(unsigned char *) &DiskInfo, IO_disk_info,
		sizeof(struct DiskInfo_Rec)))
		printf("	Failed to read TOC\n");

	printf("DiskInfo.lo_tno %d ", DiskInfo.lo_tno);
	printf("DiskInfo.hi_tno %d ", DiskInfo.hi_tno);
	printf("DiskInfo.lead_out ");
	dsp_addr(DiskInfo.lead_out);
   printf("\n");

#if 1

/* The entry after the last track has as it's
** starting address the beginning of the lead-out
** track which is the end of the audio on the disc.
** This is why we have 99+1 records in TnoInfo
*/
	TnoInfo[DiskInfo.hi_tno + 1].start_addr = DiskInfo.lead_out;

	t = &TnoInfo[DiskInfo.lo_tno];
	for (i = DiskInfo.lo_tno; i <= DiskInfo.hi_tno; i++,t++) {
		t->tno = (unsigned char) i;
		ioctl(drv,(unsigned char *) t, IO_track_info, sizeof(struct TnoInfo_Rec));
		printf("tno %2d ", t->tno);
		printf("start_addr ");
		dsp_addr(t->start_addr);
//		printf(" ctrl 0x%02x ", t->ctrl);
//		dsp_ctrl(t->ctrl);
		}

//to find out how many frames the track is
//add 150 to i start_add to skip pops ?
	for (i = DiskInfo.lo_tno; i <= DiskInfo.hi_tno; i++) {
   	num = red2hsg(TnoInfo[i+1].start_addr) - red2hsg(TnoInfo[i].start_addr);
      printf("%d TF=%ld \n ", i, num);
      }

#endif
}

/* ioctl() -
**
** DESCRIPTION
**	Sends an IOCTL request to the device driver in drv. The
**	ioctl command is cmd and the ioctl cmd length is cmdlen.
**	The buffer for the command is pointed to by xbuf.
*/
unsigned int ioctl(struct devlst far *drv, unsigned char *xbuf, 
            unsigned char cmd, unsigned char cmdlen)
{
struct Ioctl_Hdr	*io = &Ioctl_Rec;

	io->ioctl_rqh.rqh_len	= sizeof(struct Ioctl_Hdr);
	io->ioctl_rqh.rqh_unit	= drv->drive;
	io->ioctl_rqh.rqh_cmd	= IOCTL;
	io->ioctl_rqh.rqh_status = 0;

	io->ioctl_media		= 0;
	io->ioctl_xfer		= (char far *) xbuf;
	*xbuf			= cmd;
	io->ioctl_nbytes	= cmdlen;
	io->ioctl_sector	= 0;
	io->ioctl_volid		= 0;

#if 1
	send_req((struct ReadWriteL_Hdr far *) io, drv->dev_hdr);

	if (io->ioctl_rqh.rqh_status & 0x8000) {
		printf("	Error on play - status = 0x%r\n", io->ioctl_rqh.rqh_status);
		return(21);
		}
#endif
	return(0);
}                   

void send_req( struct ReadWriteL_Hdr far *io, struct dev_hdr far *devptr )
{
void far *Req_Ptr;
void far ( *dev_strat )();
void far ( *dev_int )();
unsigned int devseg;


   devseg =  HIWORD(devptr);
   dev_strat = MK_FP( devseg, devptr->devstrategy );
   dev_int = MK_FP( devseg, devptr->devinterrupt );

     Req_Ptr=io;

     asm {
          les  bx,dword ptr [Req_Ptr]
          call dword ptr [dev_strat]
          call dword ptr [dev_int]
     }

}


/* play() -
**
** DESCRIPTION
**	Sends the request to play num frames at address start on
**	drive drv. Mode determines whether the starting address is to
**	be interpreted as high sierra or red book addressing.
**	If num == 0, then instead of sending a PLAY-AUDIO command,
**	we issue a STOP-AUDIO command.
*/
unsigned int play(struct devlst far *drv, unsigned long start, unsigned long num, 
            unsigned char mode)
{
struct PlayReq_Hdr  far 	*req = &Play_Rec;

	if (mode == ADDR_HSG)
		printf(" HSG start %ld num %ld\n", start, num);
	else {
		printf(" start PLAY ");
		dsp_addr(start);
		printf(" num FRAMES %ld\n", num);
		}

	req->pl_rqh.rqh_len	= sizeof(struct PlayReq_Hdr);
	req->pl_rqh.rqh_unit	= drv->drive;
	req->pl_rqh.rqh_cmd	= (unsigned char) (num ? DEVPLAY : DEVSTOP);
	req->pl_rqh.rqh_status	= 0;

	req->pl_addrmd	= mode;
	req->pl_start	= start;
	req->pl_num	= num;

	send_req((struct ReadWriteL_Hdr far *) req, drv->dev_hdr); 
	if (req->pl_rqh.rqh_status & 0x8000) {
		printf("	Error on play - status = 0x%r\n", req->pl_rqh.rqh_status, 0);
		return(21);
		}

	return(0);
}


/* play_tracks() -
**
** DESCRIPTION
**	If WHOLE is defined, then we play the entire disc.
**	Otherwise we play about 10 seconds (the amount of time
**	to do 200 qchannel queries) from each track on the
**	disk
*/
void play_tracks(struct devlst far *drv, int start, int end)
{
struct TnoInfo_Rec	*t;
unsigned long		num;

//start dictates which track 
	t = &TnoInfo[DiskInfo.lo_tno+start];

	num = red2hsg(TnoInfo[DiskInfo.lo_tno+end].start_addr) - red2hsg(t->start_addr);
	play(drv, t->start_addr, num, ADDR_RED);
}


unsigned int getCD_Stat( struct devlst *drv )
{
struct Status_Hdr *stat = &Status_Rec;

   ioctl( drv, (unsigned char *) stat, IO_status_info, sizeof(struct Status_Hdr) );
   if( (Ioctl_Rec.ioctl_rqh.rqh_status) & 512 )
      return(1);
   else return(0);
}

unsigned int getCD_Qchan( struct devlst *drv )
{
struct QchanInfo_Rec *q = &Q_Info;


   ioctl( drv, (unsigned char *) q, IO_qchan_info, sizeof(struct QchanInfo_Rec) );
   if( (Ioctl_Rec.ioctl_rqh.rqh_status) & 512 )
      return(1);
   else return(0);

}

/* dsp_addr() -
**
** DESCRIPTION
**	Prints red book address in MM:SS:FF Min/Sec/Frame format
*/
void dsp_addr(unsigned long addr)
{
   printf( "RED %ld \n", addr );
//	printf("%2d:",	HIWORD(addr) & 0xff);
//	printf("%02d.",	LOWORD(addr) >> 8);
//	printf("%02d",	LOWORD(addr) & 0xff);
}

/* red2hsg() -
**
** DESCRIPTION
**	Converts a binary red book address to high sierra addressing
**	The msb of the red book is always zero, the next less significant
**	byte is the minute (0-59+), then second (0-59) and lsb is the
**	frame (0-75). The conversion is
**		hsg = min * 60 * 75 + sec * 75 + frame;
*/
unsigned long red2hsg(unsigned long l)
	{
	return((unsigned long) (HIWORD(l) & 0xff) * 60 * 75 +
		(unsigned long) (LOWORD(l) >> 8) * 75 +
		(unsigned long) (LOWORD(l) & 0xff));
	}


/* dsp_ctrl() -
**
** DESCRIPTION
**	Prints what attributes are in the 4 bits of track control information.
*/
void dsp_ctrl(unsigned char c)
{
	printf("	");
	switch (c & 0xc0) {
		case 0x00:
		case 0x80:
			if (c & 0x80)
				printf("4 chnl ");
			else
				printf("2 chnl ");
			if (c & 0x10)
				printf("w/ pre-emphasis");
			else
				printf("w/o pre-emphasis");
			break;
		case 0x40:
			if (c & 0x10)
				printf("reserved");
			else
				printf("data");
			break;
		case 0xc0:
			printf("reserved");
			break;
		}
	if (c & 0x20)
		printf(" COPY\n");
	else
		printf(" !COPY\n");
}


