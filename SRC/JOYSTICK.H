#ifndef _joystick_public
#define _joystick_public

//***************************************************************************
//
// Public header for JOYSTICK.ASM.
//
//***************************************************************************

extern byte Joy_xb;
extern byte Joy_yb;
extern byte Joy_xs;
extern byte Joy_ys;
extern word Joy_x;
extern word Joy_y;

void JoyStick_Vals( void );
#ifdef __FLAT__
#pragma aux JoyStick_Vals modify exact [eax ebx ecx edx esi edi]
#else
#pragma aux JoyStick_Vals modify exact [ax bx cx dx si di]
#endif

#define JOYSTICK_SetShift( x, y ) \
   { \
   Joy_x  = 0; \
   Joy_y  = 0; \
   Joy_xs = ( x ); \
   Joy_ys = ( y ); \
   Joy_xb = 1 << Joy_xs; \
   Joy_yb = 1 << Joy_ys; \
   }

#define JOYSTICK_GetPos( x, y ) \
   { \
   JoyStick_Vals(); \
   *( x ) = Joy_x; \
   *( y ) = Joy_y; \
   }


#endif
