#include <stdio.h>
#include "dapi.h"

void main()
{
  int iPlayers, iThisPlayerID;

    iPlayers = dapiInit(NULL);
    if (iPlayers <= 0)
    {
        printf("dapiInit() error: %s\n", dapiErrorString(iPlayers)); 
        return;
    }
    printf ("\nNumber of player-nodes: %d\n", iPlayers);

    iThisPlayerID = dapiThisNode();

    if (iThisPlayerID < 0)
    {
        printf("dapiThisNode() error: %s\n", 
               dapiErrorString(iThisPlayerID));
        return;
    }

    printf("This player-node ID:    %d\n", iThisPlayerID);

    dapiClose();
    return;

} // end-main()-------------- dapiThisNode() sample -----------------
