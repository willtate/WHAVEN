#include <stdio.h>
#include "dapi.h"

unsigned char Buf [DAPI_MAX_PACKET_SIZE];

void main(int argc, char *argv[])
{
  int iPlayers, iThisPlayerID, i, iErr, iLength;
  unsigned short *pus;

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

    //------------------------------------------------------------
    // Accept a message size as a command line argument. Fill the 
    // message with random numbers to better test the CRC.
    //------------------------------------------------------------
    if (argc >= 2) iLength = atoi (argv[1]);
    else           iLength = 5;

    srand (time(NULL));
    pus = (unsigned short *) &Buf[0];
    for (i = 0; i < min(DAPI_MAX_PACKET_SIZE + 1, iLength); i += 2)
    {
        *pus = rand();
        ++pus;
    }

    for (i = 0; i < iPlayers; ++i)
    {
        //------------------------------------------------
        // Send a message to each of the other players,
        // changing the message slightly for each player.
        //------------------------------------------------
        if (i != iThisPlayerID)
        {
            Buf [0] = i;
            iErr = dapiSendWithCRC (i, Buf, iLength);

            printf("dapiSendWithCRC(to %d, %d bytes): %s\n", 
                   i, iLength, dapiErrorString(iErr));
        }
    }
    
    iErr = dapiBroadcastWithCRC(Buf, iLength);

    printf("dapiBroadcastWithCRC(%d bytes): %s\n", 
            iLength, dapiErrorString(iErr));

    dapiClose();
    return;

} // end-main()--------- dapiSendWithCRC() example -----------------------
