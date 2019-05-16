#ifndef _DAPI_H
#define _DAPI_H

// if your game is going to have more than 16 nodes, then this
// number needs to be changed
#define MAX_NODES 16

// max size of node name length
#define DAPI_NODE_NAME_CHARS 20

// biggest packet that can be sent
#define DAPI_MAX_PACKET_SIZE  400

// error codes
#define DAPI_NoError             1    // no error
#define DAPI_AlreadyInited      -1    // dapi system alread inited
#define DAPI_NoConfigFile       -2    // unable to open config file
#define DAPI_BadLogicalAddress  -3    // bad logical address in dapi config
#define DAPI_ConfigError        -4    // config file is bad
#define DAPI_PortOpenFailed     -5    // unable to open com port
#define DAPI_InvalidAddress     -6    // Invalid Destination Address
#define DAPI_NoLoopBack         -7    // Cant send packets to yourself yet
#define DAPI_PacketTooLarge     -8    // Compiled packet > 400 bytes
#define DAPI_NoPacket           -9    // no packet readt, not an error
#define DAPI_BadAddress         -10   // send or recv, bad address
#define DAPI_NotInited          -11   // dapi not inited
#define DAPI_BadNodeNum         -12   // error in dapi config
#define DAPI_BadCRC16           -13   // data error: crc16 did not match sent crc16.
#define DAPI_SendBufFull        -14   // send buffer full (ack'd sends).

#define LAST_DAPI_ERR   ((-(DAPI_SendBufFull)) + 2) 
#define ERR_NONE        0
#define ERR_UNKNOWN     (LAST_DAPI_ERR-1)


#if defined(_cplusplus) || defined(__cplusplus)
extern "C" {
#endif

int dapiInit(char *file_nm);
int dapiSend(int logDest, unsigned char *str, int len);
int dapiThisNode(void);
int dapiBroadcast(unsigned char *str, int len);
int dapiSendScore(long score);
int dapiRecv(unsigned char *str, int *len);
int dapiNodeName(int node, char *name);
int dapiClose(void);
char *dapiErrorString(int iErrorNum);
int dapiSendWithCRC(int logDest, unsigned char *str, int len);
int dapiBroadcastWithCRC(unsigned char *str, int len);
int dapiSendAcked (int logDest, unsigned char *str, int len);
int dapiService (void);
long dapiResponseTime     (int iNode);
void dapiResetResponseTime(int iNode);
int  dapiNumToSend        (int iNode);

#if defined(_cplusplus) || defined(__cplusplus)
}
#endif


#endif // for _DAPI_H
