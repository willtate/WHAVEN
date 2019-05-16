#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#ifdef CALLER
#define  GLOBAL   extern
#define  INIT(x)
#else
#define  GLOBAL   
#define  INIT(x)  =x
#endif

#define LOWORD(l)    ((unsigned short)(unsigned long)(l))
#define HIWORD(l)    ((unsigned short)(((unsigned long)(l) >> 16) & 0xffff))


/* Command Codes for requests from MSCDEX */
#define  IOCTL         3            // IOCTL Input (READ)
#define	DEVPLAY		132	         // Device Play	
#define	DEVSTOP		133	         // Stop device play	

/* Command Codes for IOCTL requests */
#define  IO_head_loc     1
#define  IO_disk_info   10                                   
#define  IO_track_info  11
#define	IO_qchan_info	12
#define  IO_status_info 15

/* Addressing modes */
#define	ADDR_HSG	0
#define	ADDR_RED	1

/* Device driver request header - we'll have it's address in ES:BX when */
/*    the device's strategy routine is called.                          */
struct Request_Hdr {
	unsigned char		rqh_len;             //length of request hdr in bytes;
	unsigned char		rqh_unit;            //device sub-unit code;
	unsigned char		rqh_cmd;             //actual command code field;
	unsigned short		rqh_status;          //status - returns error codes;
	unsigned char		rqh_rsvd[8];         //reserved;
};

/* Request header for IOCTL commands (cmd_code 3) that folow */
struct Ioctl_Hdr {
   struct Request_Hdr   ioctl_rqh;        //request header;
	unsigned char		   ioctl_media;      //media descriptor - always 0;
	unsigned char far    *ioctl_xfer;      //address of control block struct 
                                          //for return info(first byte is cmd_code);

	unsigned short		   ioctl_nbytes;     //number of bytes to xfer;
	unsigned short		   ioctl_sector;     //starting sector # - always 0;
	unsigned char far	   *ioctl_volid;     //volume ID - always 0;
};

/* Request header for Read/Write command */
struct ReadWriteL_Hdr {
	struct Request_Hdr   rwl_rqh;
	unsigned char		   rwl_addrmd;
	unsigned char far	   *rwl_xfer;
	unsigned short		   rwl_nsects;
	unsigned long		   rwl_sectno;
	unsigned char		   rwl_mode;
	unsigned char		   rwl_ilsize;
	unsigned char		   rwl_ilskip;
	unsigned short		   rwl_reqno;
};

/* Record for audio_diskinfo IOCTL call cmd_code 10 */
struct DiskInfo_Rec {
	unsigned char	cmd_code;         //control block code
	unsigned char	lo_tno;           //lowest track number
	unsigned char	hi_tno;           //highest track number
	unsigned long	lead_out;         //starting point of lead-out track
};

/* Record for audio_trackinfo IOCTL call cmd_code 11 */
struct TnoInfo_Rec {
	unsigned char	cmd_code;         //control block code
	unsigned char	tno;              //track number
	unsigned long	start_addr;       //starting address of track
	unsigned char	ctrl;             //track control info
};

/* Record for audio_qchaninfo IOCTL call cmd_code 12 */
/* returns qchannel info at instant of call */
struct QchanInfo_Rec {
	unsigned char	cmd_code;
	unsigned char	ctrl;
	unsigned char	tno;
	unsigned char	x;
	unsigned char	min;
	unsigned char	sec;
	unsigned char	frame;
	unsigned char	zero;
	unsigned char	pmin;
	unsigned char	psec;
	unsigned char	pframe;
};

struct Status_Hdr {
   unsigned char  cmd_code;
   unsigned short stat_bits;
   unsigned long  last_start;
   unsigned long  last_end;
};

/* Request header for Play Audio command */
struct PlayReq_Hdr {
	struct Request_Hdr	pl_rqh;
	unsigned char  		pl_addrmd;
	unsigned long  		pl_start;
	unsigned long  		pl_num;
};

struct dev_hdr {
     void far        *nxtdrv;
     unsigned short  devattr;
     unsigned int    devstrategy;
     unsigned int    devinterrupt;
     char devname[8];
     unsigned int reserved;
     char driveletter;
     char units;
};

struct devlst {
     char drive;
     void far *dev_hdr;
};

GLOBAL int flag INIT(0);
GLOBAL void far *devptr;
GLOBAL struct dev_hdr far *hdrptr;

GLOBAL struct devlst drv_lst[26];
GLOBAL struct TnoInfo_Rec	TnoInfo[99 + 1];	/* Table of info for all tracks	*/
GLOBAL struct Ioctl_Hdr	   Ioctl_Rec; 	      /* Record for IOCTL requests	*/
GLOBAL struct PlayReq_Hdr	Play_Rec;		   /* Record for Play requests	*/
GLOBAL struct DiskInfo_Rec	DiskInfo;		   /* Disk information for CD	*/
GLOBAL struct Status_Hdr   Status_Rec;       /* CD status */   
GLOBAL struct QchanInfo_Rec  Q_Info;    /* Q channel info record */

void Init_CD(void);
void dsp_addr(unsigned long);
void read_toc(struct devlst *);
unsigned int ioctl(struct devlst far *,unsigned char *,unsigned char,unsigned char);
void send_req( struct ReadWriteL_Hdr far *, struct dev_hdr far * );
void dsp_ctrl(unsigned char);
void play_tracks(struct devlst far *,int,int);
unsigned int play(struct devlst far *, unsigned long, unsigned long,unsigned char);
unsigned long red2hsg(unsigned long);
unsigned int getCD_Stat(struct devlst *);
unsigned int getCD_Qchan( struct devlst *);


