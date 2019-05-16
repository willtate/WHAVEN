#include <stdio.h>
#include "dapi.h"

#define ESCAPE      27
unsigned char RecvBuf [DAPI_MAX_PACKET_SIZE];

void main()
{
  int iPlayers, iSource, c = 0, iLength, iMsgsRcvd = 0, i;

    iPlayers = dapiInit(NULL);

    if (iPlayers <= 0)
    {
        printf("dapiInit() error: %s\n", dapiErrorString(iPlayers)); 
        return;
    }
    printf ("\nNumber of player-nodes: %d\n", iPlayers);

    while (c != ESCAPE)
    {
	    if (kbhit() != 0)
        {
            c = getch();
            putch (c);
            printf("\n");
        }
        
        iLength = DAPI_MAX_PACKET_SIZE;        
        iSource = dapiRecv (RecvBuf, &iLength);

        if (iSource >= 0)
        {
            printf("%d dapiRecv() from %d, %d bytes (%#x %#x %#x ....)\n", 
                   ++iMsgsRcvd, iSource, iLength, 
                   RecvBuf[0], RecvBuf[1], RecvBuf[2]);
        }
        else if (iSource != DAPI_NoPacket)
        {
            printf("dapiRecv() error: %s\n", dapiErrorString(iSource)); 
        }

    } // end-while().
    dapiClose();
    return;

} // end-main()--------- dapiRecv() example ---------------------
