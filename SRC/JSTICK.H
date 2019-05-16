//***************************************************************************
//
// Public header for JSTICK.ASM
//
//***************************************************************************

//extern
//char joyb;
//extern
//short joyx;
//extern
//short joyy;

void jstick(void);
#ifdef __FLAT__
#pragma aux jstick modify exact [eax ebx ecx edx esi edi]
#else
#pragma aux jstick modify exact [ax bx cx dx si di]
#endif
