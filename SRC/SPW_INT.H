#ifndef SPW_INT_H
#define SPW_INT_H
/* ------------------------------------------------------------------------- */

typedef struct Spw_IntPacket {
  unsigned char  command;   
  unsigned char  res1;      
  short      comspec;   
  long       period;    
  unsigned short button;    
  short          tx;        
  short      ty;        
  short      tz;        
  short      rx;        
  short      ry;        
  short      rz;        
  short      res2;      
  short      res3;      
  unsigned short checksum;
} Spw_IntPacket;

/* ------------------------------------------------------------------------- */

#define SPW_INTCMD_INITIALIZE   0x0000   /* Initialize the driver */
#define SPW_INTCMD_GCFORCE      0x0003   /* Get Cumulative Force */
#define SPW_INTCMD_SCFORCE      0x0004   /* Set Cumulative Force */
#define SPW_INTCMD_GBPRESS      0x0005   /* Get Button Press info */
#define SPW_INTCMD_GBRELEASE    0x0006   /* Get Button Release info */
#define SPW_INTCMD_SETCMIN      0x0007   /* Set Cumulative Minimums */
#define SPW_INTCMD_SETCMAX      0x0008   /* Set Cumulative Maximums */
#define SPW_INTCMD_GFORCE       0x000b   /* Get current Force */
#define SPW_INTCMD_SEVENT       0x000c   /* Set user defined Event Handler */
#define SPW_INTCMD_SRATIO       0x000f   /* Set force to cum. force ratio */
#define SPW_INTCMD_SWAPE        0x0014   /* Swap user defined event handlers */
#define SPW_INTCMD_SALTE        0x0018   /* Set alternate event handler */
#define SPW_INTCMD_GALTE        0x0019   /* Get alternate event handler */
#define SPW_INTCMD_SSENSE       0x001a   /* Set Sensativity */
#define SPW_INTCMD_GSENSE       0x001b   /* Get Sensativity */
#define SPW_INTCMD_DISABLE      0x001f   /* Disable driver */
#define SPW_INTCMD_ENABLE       0x0020   /* Enable driver */
#define SPW_INTCMD_RESET        0x0021   /* Reset driver */
#define SPW_INTCMD_INFO         0x0024   /* Get Information */

/* ------------------------------------------------------------------------- */

int SPW_IntInit(void);
void SPW_IntEnable(void);
void SPW_IntDisable(void);
int SPW_IntCommand(int command, struct Spw_IntPacket *data_pkt);
int SPW_IntGetInfo(int *major, int *minor, int *device, int *irq);

/* ------------------------------------------------------------------------- */
#endif /* SPW_INT_H */
