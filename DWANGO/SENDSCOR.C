#include <stdio.h>
#include <time.h>
#include "dapi.h"

#define ESCAPE 27

void main( int argc, char *argv[] )
{
  int    iErr, iPlayers, c = 0;
  long   lScore = -3;
  time_t tLastSend = time(NULL), tCurrent, tSendInterval = 3; 

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
            c = getche();
            iErr = dapiSendScore (++lScore);

            printf("  dapiSendScore(%ld): %s\n", 
                   lScore, dapiErrorString(iErr));
            tLastSend = time (NULL);
        }
                
        tCurrent = time(NULL);

        if ((tCurrent - tLastSend) >= tSendInterval)
        {
            iErr = dapiSendScore (++lScore);

            printf("  dapiSendScore(%ld): %s\n", 
                   lScore, dapiErrorString(iErr));
            tLastSend = time (NULL);
        }

    } // end-while().
    return;

} // end-main()--------- dapiSendScore() example ---------------------
