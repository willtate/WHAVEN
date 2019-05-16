#include <stdio.h>
#include "dapi.h"

void main()
{
  int iPlayers, iErr, iThisPlayerID, i;
  char szName [DAPI_NODE_NAME_CHARS + 1];  // 1 for null-terminator.

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

    for (i = 0; i < iPlayers; ++i)
    {
        iErr = dapiNodeName(i, szName);

        if (iErr < 0)
        {
            printf("dapiNodeName() error: %s\n", dapiErrorString(iErr));
        }
        else
        {           
            printf("Player-node %d name: %s\n", i, szName);
        }
    }

    dapiClose();
    return;

} // end-main()--------- dapiNodeName() example -----------------------
