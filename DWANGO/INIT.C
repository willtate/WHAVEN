#include <stdio.h>
#include "dapi.h"

void main()
{
  int iPlayers;

    iPlayers = dapiInit(NULL);

    if (iPlayers <= 0)
    {
        printf("dapiInit() error: %s\n", dapiErrorString(iPlayers));
        return;
    }

    printf ("\nNumber of player-nodes: %d\n", iPlayers);

    dapiClose();
    return;

} // end-main()---------------- dapiInit() sample -------------------
