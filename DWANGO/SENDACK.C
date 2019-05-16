#include <stdio.h>
#include "dapi.h"

#define ESCAPE      27
#define BUF_BYTES   3
unsigned char SendBuf [BUF_BYTES] = {0xFF, 0xA5, 0xAA};
unsigned char RecvBuf [DAPI_MAX_PACKET_SIZE];

void main()
{
  int iPlayers, iThisPlayerID, i, iErr, c = 0, iLength, iSource;

    iPlayers = dapiInit(NULL);

    if (iPlayers <= 0)
    {
        printf("dapiInit() error: %s\n", dapiErrorString(iPlayers)); 
        return;
    }
    printf ("\nNumber of player-nodes: %d\n", iPlayers);

    iThisPlayerID = dapiThisNode();

    //---------------------------------------------------------
    // Make sure dapiThisNode() worked (iThisPlayerID is >= 0)
    // and that there are two or more players in the game.
    //---------------------------------------------------------
    if ((iThisPlayerID < 0) || (iPlayers < 2))
    {
        printf("dapiThisNode() error: %s\n", 
                dapiErrorString(iThisPlayerID));
        return;
    }

    while (c != ESCAPE)
    {
        if (kbhit() != 0)
        {
            c = getche();
            SendBuf[1] = (unsigned char) c;

            for (i = 0; i < iPlayers; ++i)
            {
                //----------------------------------------------------
                // Send a short message to each of the other players,
                // changing the message slightly for each player.
                //----------------------------------------------------
                if (i != iThisPlayerID)
                {
                    SendBuf [0] = i;
                    iErr = dapiSendAcked(i, SendBuf, BUF_BYTES);

                    printf("\ndapiSendAcked(to %d): %s\n", 
                            i, dapiErrorString(iErr));
                }
            }
        }
        else
        {
            //-----------------------------------------------
            // dapiRecv() handles message acks and resends.
            //-----------------------------------------------
            iLength = DAPI_MAX_PACKET_SIZE;        
            iSource = dapiRecv (RecvBuf, &iLength);
        }
                

    } // end-while().

    dapiClose();
    return;

} // end-main()--------- dapiSendAcked() example -----------------
