#include <stdio.h>
#include "dapi.h"

void main()
{
  int iPlayers, iErr;

    iPlayers = dapiInit(NULL);

    if (iPlayers <= 0)
    {
        printf("dapiInit() error: %s\n", 
               dapiErrorString(iPlayers)); 
        return;
    }

    printf ("\nNumber of player-nodes: %d\n", iPlayers);

    iErr = dapiClose();

    printf("dapiClose(): %s\n", dapiErrorString(iErr)); 
    return;

} // end-main()-------------- dapiClose() example -------------------
