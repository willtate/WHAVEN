/****************************************************************************
*
*  File              : netnow.h
*  Date Created      : 1/18/95
*  Description       : NETNOW! support library header file
*
*  Programmer(s)     : Nick Skrepetos
*  Last Modification : 2/16/95 - 8:32:9 AM
*  Additional Notes  :
*
*****************************************************************************
*            Copyright (c) 1994-95,  HMI, Inc.  All Rights Reserved         *
****************************************************************************/

#ifndef  _HMI_NET_DEFINED
#define  _HMI_NET_DEFINED

// equates
#define  _DPMI_INT               0x31

// maximum number of allowable nodes
#define  _NETNOW_MAX_NODES       16

// broadcast command
#define  _NETNOW_BROADCAST       _NETNOW_MAX_NODES + 1

// maximum number of packet in send/listen queue
#define  _NETNOW_MAX_SEND_PACKETS      24
#define  _NETNOW_MAX_LISTEN_PACKETS    24

// size of data packet for transmission
#define  _NETNOW_DATA_PACKET           512

// filler for alignment
#define  __HMI_FILL(a)  unsigned short a;

// real mode interrupt structure
typedef  struct   _tagRMI_REGS
         {
            // system registers
            long  EDI;
            long  ESI;
            long  EBP;

            // reserved
            long  reserved_by_system;

            long  EBX;
            long  EDX;
            long  ECX;
            long  EAX;

            // system flags
            short flags;

            // segment registers
            short ES,DS,FS,GS,IP,CS,SP,SS;

         }  _RMI_REGS;

// set structure packing to byte alignment
#pragma  pack(1)

// byte registers
typedef  struct   _tagBREGS
         {

            char  al, ah;  unsigned short _1;
            char  bl, bh;  unsigned short _2;
            char  cl, ch;  unsigned short _3;
            char  dl, dh;  unsigned short _4;

         } _HMI_BREGS;                

// word registers
typedef  struct   _tagWREGS
         {

            unsigned short    ax;   unsigned short _1;
            unsigned short    bx;   unsigned short _2;
            unsigned short    cx;   unsigned short _3;
            unsigned short    dx;   unsigned short _4;
            unsigned short    si;   unsigned short _5;
            unsigned short    di;   unsigned short _6;

         } _HMI_WREGS;

// dword registers
typedef  struct   _tagDREGS
         {

            unsigned int   eax;
            unsigned int   ebx;
            unsigned int   ecx;
            unsigned int   edx;
            unsigned int   esi;
            unsigned int   edi;
            unsigned int   cflags;

         } _HMI_DREGS;


// segment registers
typedef  struct   _tagSREGS
         {

            unsigned short    es;
            unsigned short    cs;
            unsigned short    ss;
            unsigned short    ds;
            unsigned short    fs;
            unsigned short    gs;

         } _HMI_SREGS;


// general register packet
typedef  union
         {

            _HMI_DREGS  x;
            _HMI_WREGS  w;
            _HMI_BREGS  h;

         } _HMI_REGS;

// reset structure packing
#pragma  pack()

// equates for node addition
enum
   {
      _NETNOW_ADD_COMPLETE,
      _NETNOW_ADD_DUPLICATE,
      _NETNOW_ADD_LIST_FULL
   };

// error code enumerations
enum
   {
      _NETNOW_NO_ERROR,
      _NETNOW_NO_NETWORK,
      _NETNOW_INIT_ERROR,
      _NETNOW_NO_REALMEM,
      _NETNOW_IPX,
      _NETNOW_NETBIOS,
   };

#include "hmiipx.h"
#include "hminetb.h"
#include "hmixfer.h"

// set structure packing to byte alignment
#pragma  pack(1)

// generic network address
typedef  union 
         {

            _IPX_LOCAL_TARGET       sIPX;
            _NETBIOS_LOCAL_TARGET   sNETBIOS;

         } _NETNOW_NODE_ADDR;

// reset structure packing
#pragma  pack()

#endif

// function prototypes from IPX/NETBIOS
BOOL	cdecl	hmiIPXGetNetworkAddr       ( _NETNOW_NODE_ADDR * sNode );
WORD	cdecl hmiIPXAddNode              ( _NETNOW_NODE_ADDR * sNewNetwork );
BOOL	cdecl	hmiNETBIOSGetNetworkAddr   ( _NETNOW_NODE_ADDR * sNode );
WORD	cdecl hmiNETBIOSAddNode          ( _NETNOW_NODE_ADDR * sNewNetwork );

// function protoypes from netasm.asm
VOID  cdecl hmiINT386x                 ( WORD wInterrupt, _HMI_REGS * sIn, _HMI_REGS * sOut,
                                         _HMI_SREGS * sRegs );
USHORT   cdecl hmiGetDS                ( VOID );
USHORT   cdecl hmiGetCS                ( VOID );
WORD     cdecl hmiAllocateLinearMem    ( WORD wSize, WORD * wAddress, WORD * wHandle );
WORD     cdecl hmiMapPhysicalToLinear  ( WORD wHandle, WORD wPhysical, WORD wSize );

// function prototypes
BOOL	cdecl	hmiNETNOWInitSystem              ( WORD wNodes );
BOOL	cdecl	hmiNETNOWUnInitSystem            ( VOID );
BOOL	cdecl hmiNETNOWAllocateRealMem         ( WORD wSize, PSTR * pPtr, WORD * wSegment );
BOOL	cdecl	hmiNETNOWFindNodes               ( WORD wNodes );
WORD	cdecl	hmiNETNOWGetConsoleNode          ( VOID );
WORD	cdecl	hmiNETNOWGetActiveNodes          ( VOID );
BOOL	cdecl	hmiNETNOWSendData                ( PSTR pHeader, WORD wHSize,
                                               PSTR pData,   WORD wDSize,
                                               WORD wNode );
BOOL	cdecl	hmiNETNOWGetData                 ( PSTR pHeader, WORD wHSize,
                                               PSTR pData,   WORD wDSize );
WORD	cdecl	hmiNETNOWAddNode                 ( _NETNOW_NODE_ADDR * sNode );
VOID	cdecl	hmiNETNOWSortNodes               ( VOID );
BOOL	cdecl	hmiNETNOWPostListen              ( VOID );
BOOL	cdecl	hmiNETNOWGetNetworkAddr          ( _NETNOW_NODE_ADDR * sNode  );
WORD	cdecl	hmiNETNOWGetNetworkType          ( VOID );
BOOL	cdecl	hmiNETNOWGetHeader               ( PSTR pHeader,   WORD wHSize, 
                                               PSTR * pPacket );
VOID	cdecl	hmiNETNOWGetBlock                ( PSTR pPacket, PSTR pData, WORD wDSize );

