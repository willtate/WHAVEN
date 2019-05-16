#include <stdio.h>
#include "dapi.h"

#define BUF_BYTES   3
unsigned char Buf [BUF_BYTES] = {0xFF, 0xA5, 0x5E};

void main()
{
  int iPlayers, iThisPlayerID, i, iErr;

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

    for (i = 0; i < iPlayers; ++i)
    {
        //----------------------------------------------------
        // Send a short message to each of the other players,
        // changing the message slightly for each player.
        //----------------------------------------------------
        if (i != iThisPlayerID)
        {
            Buf [0] = i;
            iErr = dapiSend (i, Buf, BUF_BYTES);

            printf("dapiSend(to %d): %s\n", i, dapiErrorString(iErr));
        }
    }
    dapiClose();
    return;

} // end-main()--------- dapiSend() example -----------------------
