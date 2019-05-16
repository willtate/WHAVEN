//---------------------------------------------------------------------
// Filename:    gamesamp.c
//
// Author/Date: Scott Caddes / December 14, 1995
//
// Copyright:   (c) 1995 IVS Corp - All Rights Reserved!
//
// Description: This file contains a "sample game" that calls all of 
//              the Dwango API (Dapi) routines.
//
// Routines:    main()
//
//---------------------------------------------------------------------

//-----------
// Includes:
//-----------
#include <stdio.h>
#include <time.h>

#include "dapi.h"


//------------------------------------
// Definitions and Data Declarations:
//------------------------------------
typedef unsigned short BOOL;
typedef unsigned char  BYTE, *PBYTE;
#define TRUE   1
#define FALSE  0
#define ESCAPE      27
#define BACKSPACE    8

#define MAX_COMMAND_CHARS 60
char szCommand [MAX_COMMAND_CHARS + 1];
int  iCmdIndex   = 0;
BOOL bGotCommand = FALSE;
BOOL bEchoOn     = TRUE;
BOOL bScoreOn    = FALSE;
BOOL bAcksOn     = FALSE;
BOOL bCRCsOn     = FALSE;

char szHelp [] = 
"\n              Welcome to the Dapi Game Sample!\n\n\
Type messages to other players. The first character is the destination;\n\
eg: 1 Hello      sends a message to player-1.\n\
    B Hi all!    broadcasts a message.\n\n\
Messages that don't start with a number or B go to the previous destination.\n\n\
Your score is the number of characters you type. Type \"score on\" and gamesamp\n\
broadasts your score every ten seconds (if your score changes).\n\n\ 
Hit <escape> to exit. If one player restarts, restart every player!\n\n\
Commands: score on, score off, echo on, echo off, acks on, acks off,\n\
          crcs on, crcs off, help, ?\n\n";


#define MSG_TYPE_STRING 1

typedef struct MSG_STRING_T	 // MSG_TYPE_STRING
{
	BYTE	byMsgType;
	char	szMsg [MAX_COMMAND_CHARS + 1];	
}
MSG_STRING, *PMSG_STRING;

MSG_STRING OutStringMsg;
BYTE       RecvBuf [DAPI_MAX_PACKET_SIZE];


char szName [DAPI_NODE_NAME_CHARS + 1];  // 1 for null-terminator.
int  iPlayers, iThisPlayerID;
long lScore = 0, lLastReportedScore = 0; 


//-----------------------
//  main()
//-----------------------
void main()
{
  int iErr, iMsgDest = 0; 
  int iSource, c = 0, iLength, iMsgsRcvd = 0, i, iSendLength;
  PMSG_STRING pStringMsg;
  BOOL        bBroadcast = FALSE;
  time_t      tLastSend = time(NULL), tCurrent, tSendInterval = 10; 

    //------------------------------------------------------
    // Initialize Dapi and display this player-node's info.
    //------------------------------------------------------
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

    iErr = dapiNodeName(iThisPlayerID, szName);

    if (iErr < 0)
    {
        printf("dapiNodeName() error: %s\n", dapiErrorString(iErr));
        return;
    }
    printf("This player-node name:  %s\n", szName);
    printf(szHelp);

    //------------------------------------------------------ 
    // Process keyboard input until the user hits <escape>.
    //------------------------------------------------------ 
    while (c != ESCAPE)
    {
	    if (kbhit() != 0)
        {
            c = getch();

            if ((c == '\r') || (c == '\n'))
            {
                //---------------------------------------------
                // User hit <enter>, so we now have a command
                // in our command line buffer.
                //---------------------------------------------
                if (iCmdIndex < MAX_COMMAND_CHARS)
                {
	                szCommand [iCmdIndex++] = 0;
                }
                else
                {
	                szCommand [MAX_COMMAND_CHARS] = 0;
                }

                bGotCommand = TRUE;
                iCmdIndex   = 0;

                printf("\n");
            }
            else if ((c == BACKSPACE) && (iCmdIndex > 0))
            {
                putch (c);
                putch (' ');
                putch (c);
                --iCmdIndex;
            }
            else
            {
                //----------------------------------------------
                // Echo the character to the screen, put it
                // in the command line buffer if there is room.
                //----------------------------------------------
                putch(c);

                if (iCmdIndex < MAX_COMMAND_CHARS)
                {
	                szCommand [iCmdIndex++] = c;
                }
                else
                {
	                szCommand [MAX_COMMAND_CHARS] = 0;
                    iCmdIndex   = 0;
                    bGotCommand = TRUE;
                    printf("\n");
                }
            }

        } // endif-kbhit() check.

        //-----------------------------------------------------                                                
        // Check for received messages only when this player's
        // command line buffer is empty.
        //-----------------------------------------------------
        if (iCmdIndex == 0)
        {
            iLength = DAPI_MAX_PACKET_SIZE;        
            iSource = dapiRecv (RecvBuf, &iLength);

            if (iSource >= 0)
            {
                ++iMsgsRcvd;
                switch (RecvBuf[0])
                {
                  case MSG_TYPE_STRING:
                    //----------------------------------------------
                    // We received a string message: null-terminate
                    // it to be safe, then display it.
                    //----------------------------------------------
                    pStringMsg = (PMSG_STRING) RecvBuf;
                    pStringMsg->szMsg[MAX_COMMAND_CHARS] = 0;

                    printf("Rcvd fr %2d: %s\n", iSource, pStringMsg->szMsg);

                    break;

                  default:
                    printf("%d dapiRecv() unrecognized msg from %d, %d bytes (%#x %#x %#x....)\n", 
                           iMsgsRcvd, iSource, iLength, 
                           RecvBuf[0], RecvBuf[1], RecvBuf[2]);

                    break;

                } // end-switch().
            } 

        } // endif-(iCmdIndex == 0)

        //--------------------------------------------
        // If the user entered a command, process it.
        //--------------------------------------------
        if (bGotCommand && (   (szCommand[0] == '?')
                            || (strnicmp ("help", szCommand, 4) == 0)))
        {
            printf(szHelp);
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("echo on", szCommand, 7) == 0))
        {
            bEchoOn     = TRUE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("echo off", szCommand, 8) == 0))
        {
            bEchoOn     = FALSE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("score on", szCommand, 8) == 0))
        {
            bScoreOn    = TRUE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("score off", szCommand, 9) == 0))
        {
            bScoreOn    = FALSE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("acks on", szCommand, 7) == 0))
        {
            bAcksOn     = TRUE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("acks off", szCommand, 8) == 0))
        {
            bAcksOn     = FALSE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("crcs on", szCommand, 7) == 0))
        {
            bCRCsOn     = TRUE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand && (strnicmp ("crcs off", szCommand, 8) == 0))
        {
            bCRCsOn     = FALSE;
            bGotCommand = FALSE;
        }
        else if (bGotCommand)
        {
            //------------------------------------------------
            // Set the message destination based on the first
            // few characters of the command line.
            //------------------------------------------------
            if (isdigit(szCommand[0]))
            {
                iMsgDest   = atoi (szCommand);
                bBroadcast = FALSE;
            }
            else if (toupper(szCommand[0]) == 'B')
            {
                bBroadcast = TRUE;
            }
                  
            OutStringMsg.byMsgType = MSG_TYPE_STRING;

            strncpy (OutStringMsg.szMsg, szCommand, MAX_COMMAND_CHARS);
            OutStringMsg.szMsg[MAX_COMMAND_CHARS] = 0;

            iSendLength = (strlen (OutStringMsg.szMsg) + 2);

            if (bBroadcast)
            {
                if (bCRCsOn)
                {
                    iErr = dapiBroadcastWithCRC (
                                        (unsigned char *) &OutStringMsg, 
                                        iSendLength);

                    if (bEchoOn) printf ("dapiBroadcastWithCRC");
                }
                else
                {
                    iErr = dapiBroadcast ((unsigned char *) &OutStringMsg,
                                          iSendLength);

                    if (bEchoOn) printf ("dapiBroadcast");
                }

                if ((iErr == DAPI_NoError) && bEchoOn)
                {
                    printf("(%d bytes): %s\n", 
                           iSendLength, OutStringMsg.szMsg);

                    lScore += strlen(OutStringMsg.szMsg);
                }
                else if (iErr != DAPI_NoError)
                {
                    printf("(%d bytes): %s\n", 
                           iSendLength, dapiErrorString(iErr));
                }
            }
            else
            {
                if (bAcksOn)
                {
                    iErr = dapiSendAcked (iMsgDest, 
                                          (unsigned char *) &OutStringMsg, 
                                          iSendLength);

                    if (bEchoOn) printf ("dapiSendAcked");
                }
                else if (bCRCsOn)
                {
                    iErr = dapiSendWithCRC (iMsgDest, 
                                            (unsigned char *) &OutStringMsg, 
                                             iSendLength);

                    if (bEchoOn) printf ("dapiSendWithCRC");
                }
                else
                {
                    iErr = dapiSend (iMsgDest, 
                                     (unsigned char *) &OutStringMsg, 
                                     iSendLength);

                    if (bEchoOn) printf ("dapiSend");
                }

                if ((iErr == DAPI_NoError) && bEchoOn)
                {
                    printf("(to %d, %d bytes): %s\n", 
                            iMsgDest, iSendLength, OutStringMsg.szMsg);

                    lScore += strlen(OutStringMsg.szMsg);
                }
                else if (iErr != DAPI_NoError)
                {
                    printf("(to %d, %d bytes): %s\n", 
                            iMsgDest, iSendLength, dapiErrorString(iErr));
                }
            }

            bGotCommand = FALSE;

        } // endif-bGotCommand check.

        //---------------------------------------------------------
        // Send our score periodically, reporting it to the Dwango 
        // server with dapiSendScore() and broadcasting it to the
        // other players.
        //---------------------------------------------------------
        tCurrent = time(NULL);

        if (   ((tCurrent - tLastSend) >= tSendInterval)
            && (lScore != lLastReportedScore)
            && bScoreOn
           )
        {
            lLastReportedScore = lScore;
            iErr = dapiSendScore (lScore);

            // Send score to Dwango server:
            if (iErr != DAPI_NoError)
            {
                printf("dapiSendScore(%ld): %s\n", 
                       lScore, dapiErrorString(iErr));
            }

            // Broadcast score to all other players:
            iSendLength            = sizeof (OutStringMsg);
            OutStringMsg.byMsgType = MSG_TYPE_STRING;

            sprintf(OutStringMsg.szMsg, "Score (%02d):  %ld", 
                    iThisPlayerID, lScore);

            iErr = dapiBroadcast ((unsigned char *) &OutStringMsg, 
                                  iSendLength);

            if (iErr != DAPI_NoError)
            {
                printf("dapiBroadcast(%d bytes): %s\n", 
                       iSendLength, dapiErrorString(iErr));
            }
            tLastSend = time (NULL);
        }

    } // end-while().

    dapiClose();
    return;

} // end-main().

//-------------------------
// End-of-file: gamesamp.c
//-------------------------
