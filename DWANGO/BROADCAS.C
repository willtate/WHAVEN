#include <stdio.h>
#include "dapi.h"

#define BUF_BYTES   3
unsigned char Buf [BUF_BYTES] = {0xFF, 0xA5, 0xBB};

void main()
{
  int iPlayers, iErr;

    iPlayers = dapiInit(NULL);

    if (iPlayers <= 0)
    {
        printf("dapiInit() error: %s\n", dapiErrorString(iPlayers)); 
        return;
    }
    printf ("\nNumber of player-nodes: %d\n", iPlayers);

    //-------------------------------------------------
    // Broadcast a short message to all other players.
    //-------------------------------------------------
    iErr = dapiBroadcast (Buf, BUF_BYTES);

    printf("dapiBroadcast(): %s\n", dapiErrorString(iErr)); 

    dapiClose();
    return;

} // end-main()--------- dapiBroadcast() example ---------------------
