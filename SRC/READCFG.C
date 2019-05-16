//**
//** READCFG.C - Utilities to read and write the CONTROLS.CFG file.
//**
//** 02/20/96  Les Bird
//**

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <ctype.h>
#include <sys\types.h>
#include "sos.h"
#include "profile.h"

#include "whdcntrl.h"

char controlConfigFile[_MAX_PATH];

static
char tempBuf[256];

char *controlAction[MAXACTIONS+1]={
     "MOVE FORWARD ",
     "MOVE BACKWARD",
     "TURN LEFT    ",
     "TURN RIGHT   ",
     "RUN MODE     ",
     "STRAFE MODE  ",
     "USE WEAPON   ",
     "OPEN/CLOSE   ",
     "JUMP         ",
     "CROUCH       ",
     "LOOK UP      ",
     "LOOK DOWN    ",
     "LOOK CENTER  ",
     "STRAFE LEFT  ",
     "STRAFE RIGHT ",
     "USE POTION   ",
     "CAST SPELL   ",
     "FLY UP       ",
     "FLY DOWN     ",
     "USE SHIELD   ",
     NULL
};

#ifdef CON_MOUSE
char *mouseControlLabel[MAXMOUSEBUTTONS]={
     "LEFT BUTTON  ",
     "MIDDLE BUTTON",
     "RIGHT BUTTON "
};
#endif

#ifdef CON_JOYSTICK
char *joystickControlLabel[MAXJOYSTICKBUTTONS]={
     "BUTTON 1",
     "BUTTON 2",
     "BUTTON 3",
     "BUTTON 4"
};
#endif

#ifdef CON_AVENGER
char *avengerControlLabel[MAXAVENGERBUTTONS]={
     "BUTTON A",
     "BUTTON B",
     "BUTTON C",
     "BUTTON D",
     "BUTTON E",
     "BUTTON F"
};
#endif

#ifdef CON_GAMEPAD
char *gamepadControlLabel[MAXGAMEPADBUTTONS]={
     "BUTTON A",
     "BUTTON B",
     "BUTTON C",
     "BUTTON D"
};
#endif

#ifdef CON_WINGMAN
char *wingmanControlLabel[MAXWINGMANBUTTONS]={
     "BUTTON 1",
     "BUTTON 2",
     "BUTTON 3",
     "BUTTON 4"
};
#endif

#ifdef CON_VFX1
char *VFX1ControlLabel[MAXVFX1BUTTONS]={
     "TOP BUTTON   ",
     "MIDDLE BUTTON",
     "BOTTOM BUTTON"
};
#endif

//** Supported video modes
char *videoModeList[NUMVIDEOMODES]={
     "MCGA 320 X 200",
     "SUPERVGA 640 X 480"
};

signed
char configKeyboard[MAXACTIONS];

#ifdef CON_MOUSE
signed
char configMouse[MAXMOUSEBUTTONS];
#endif
#ifdef CON_JOYSTICK
signed
char configJoystick[MAXJOYSTICKBUTTONS];
#endif
#ifdef CON_AVENGER
signed
char configAvenger[MAXAVENGERBUTTONS];
#endif
#ifdef CON_GAMEPAD
signed
char configGamepad[MAXGAMEPADBUTTONS];
#endif
#ifdef CON_WINGMAN
signed
char configWingman[MAXWINGMANBUTTONS];
#endif
#ifdef CON_VFX1
signed
char configVFX1[MAXVFX1BUTTONS];
#endif

signed
char videoModeOption;

void
readKeyboardConfig(_INI_INSTANCE *sInstance)
{
     int  i,j;

     for (i=0 ; i < MAXACTIONS ; i++) {
          configKeyboard[i]=-1;
     }
     if (!hmiINILocateSection(sInstance,"KEYBOARD")) {
          return;
     }
     for (i=0 ; i < MAXACTIONS ; i++) {
          if (hmiINIGetItemString(sInstance,controlAction[i],tempBuf,16)) {
               sscanf(tempBuf,"%x",&j);
               configKeyboard[i]=(signed char)j;
          }
     }
}

void
writeKeyboardConfig(_INI_INSTANCE *sInstance)
{
     int  i;

     if (!hmiINILocateSection(sInstance,"KEYBOARD")) {
          hmiINIAddSection(sInstance,"KEYBOARD");
     }
     for (i=0 ; i < MAXACTIONS ; i++) {
          if (!hmiINILocateItem(sInstance,controlAction[i])) {
               hmiINIAddItemDecimal(sInstance,controlAction[i],
                                    (WORD)configKeyboard[i],
                                    strlen(controlAction[i])+4,16);
          }
          else {
               hmiINIWriteDecimal(sInstance,(WORD)configKeyboard[i]);
          }
     }
}

#ifdef CON_MOUSE
void
readMouseConfig(_INI_INSTANCE *sInstance)
{
     int  i,j;

     for (i=0 ; i < MAXMOUSEBUTTONS ; i++) {
          configMouse[i]=-1;
     }
     if (!hmiINILocateSection(sInstance,"MOUSE")) {
          return;
     }
     for (i=0 ; i < MAXMOUSEBUTTONS ; i++) {
          if (hmiINIGetItemString(sInstance,mouseControlLabel[i],tempBuf,16)) {
               sscanf(tempBuf,"%x",&j);
               configMouse[i]=(signed char)j;
          }
     }
}

void
writeMouseConfig(_INI_INSTANCE *sInstance)
{
     int  i;

     if (!hmiINILocateSection(sInstance,"MOUSE")) {
          hmiINIAddSection(sInstance,"MOUSE");
     }
     for (i=0 ; i < MAXMOUSEBUTTONS ; i++) {
          if (!hmiINILocateItem(sInstance,mouseControlLabel[i])) {
               hmiINIAddItemDecimal(sInstance,mouseControlLabel[i],
                                    (WORD)configMouse[i],
                                    strlen(mouseControlLabel[i])+4,16);
          }
          else {
               hmiINIWriteDecimal(sInstance,(WORD)configMouse[i]);
          }
     }
}
#endif

#ifdef CON_JOYSTICK
void
readJoystickConfig(_INI_INSTANCE *sInstance)
{
     int  i,j;

     for (i=0 ; i < MAXJOYSTICKBUTTONS ; i++) {
          configJoystick[i]=-1;
     }
     if (!hmiINILocateSection(sInstance,"JOYSTICK")) {
          return;
     }
     for (i=0 ; i < MAXJOYSTICKBUTTONS ; i++) {
          if (hmiINIGetItemString(sInstance,joystickControlLabel[i],
                                  tempBuf,16)) {
               sscanf(tempBuf,"%x",&j);
               configJoystick[i]=(signed char)j;
          }
     }
}

void
writeJoystickConfig(_INI_INSTANCE *sInstance)
{
     int  i;

     if (!hmiINILocateSection(sInstance,"JOYSTICK")) {
          hmiINIAddSection(sInstance,"JOYSTICK");
     }
     for (i=0 ; i < MAXJOYSTICKBUTTONS ; i++) {
          if (!hmiINILocateItem(sInstance,joystickControlLabel[i])) {
               hmiINIAddItemDecimal(sInstance,joystickControlLabel[i],
                                    (WORD)configJoystick[i],
                                    strlen(joystickControlLabel[i])+4,16);
          }
          else {
               hmiINIWriteDecimal(sInstance,(WORD)configJoystick[i]);
          }
     }
}
#endif

#ifdef CON_AVENGER
void
readAvengerConfig(_INI_INSTANCE *sInstance)
{
     int  i,j;

     for (i=0 ; i < MAXAVENGERBUTTONS ; i++) {
          configAvenger[i]=-1;
     }
     if (!hmiINILocateSection(sInstance,"AVENGER")) {
          return;
     }
     for (i=0 ; i < MAXAVENGERBUTTONS ; i++) {
          if (hmiINIGetItemString(sInstance,avengerControlLabel[i],
                                  tempBuf,16)) {
               sscanf(tempBuf,"%x",&j);
               configAvenger[i]=(signed char)j;
          }
     }
}

void
writeAvengerConfig(_INI_INSTANCE *sInstance)
{
     int  i;

     if (!hmiINILocateSection(sInstance,"AVENGER")) {
          hmiINIAddSection(sInstance,"AVENGER");
     }
     for (i=0 ; i < MAXAVENGERBUTTONS ; i++) {
          if (!hmiINILocateItem(sInstance,avengerControlLabel[i])) {
               hmiINIAddItemDecimal(sInstance,avengerControlLabel[i],
                                    (WORD)configAvenger[i],
                                    strlen(avengerControlLabel[i])+4,16);
          }
          else {
               hmiINIWriteDecimal(sInstance,(WORD)configAvenger[i]);
          }
     }
}
#endif

#ifdef CON_GAMEPAD
void
readGamepadConfig(_INI_INSTANCE *sInstance)
{
     int  i,j;

     for (i=0 ; i < MAXGAMEPADBUTTONS ; i++) {
          configGamepad[i]=-1;
     }
     if (!hmiINILocateSection(sInstance,"GAMEPAD")) {
          return;
     }
     for (i=0 ; i < MAXGAMEPADBUTTONS ; i++) {
          if (hmiINIGetItemString(sInstance,gamepadControlLabel[i],
                                  tempBuf,16)) {
               sscanf(tempBuf,"%x",&j);
               configGamepad[i]=(signed char)j;
          }
     }
}

void
writeGamepadConfig(_INI_INSTANCE *sInstance)
{
     int  i;

     if (!hmiINILocateSection(sInstance,"GAMEPAD")) {
          hmiINIAddSection(sInstance,"GAMEPAD");
     }
     for (i=0 ; i < MAXGAMEPADBUTTONS ; i++) {
          if (!hmiINILocateItem(sInstance,gamepadControlLabel[i])) {
               hmiINIAddItemDecimal(sInstance,gamepadControlLabel[i],
                                    (WORD)configGamepad[i],
                                    strlen(gamepadControlLabel[i])+4,16);
          }
          else {
               hmiINIWriteDecimal(sInstance,(WORD)configGamepad[i]);
          }
     }
}
#endif

#ifdef CON_WINGMAN
void
readWingmanConfig(_INI_INSTANCE *sInstance)
{
     int  i,j;

     for (i=0 ; i < MAXWINGMANBUTTONS ; i++) {
          configWingman[i]=-1;
     }
     if (!hmiINILocateSection(sInstance,"WINGMAN")) {
          return;
     }
     for (i=0 ; i < MAXWINGMANBUTTONS ; i++) {
          if (hmiINIGetItemString(sInstance,wingmanControlLabel[i],
                                  tempBuf,16)) {
               sscanf(tempBuf,"%x",&j);
               configWingman[i]=(signed char)j;
          }
     }
}

void
writeWingmanConfig(_INI_INSTANCE *sInstance)
{
     int  i;

     if (!hmiINILocateSection(sInstance,"WINGMAN")) {
          hmiINIAddSection(sInstance,"WINGMAN");
     }
     for (i=0 ; i < MAXWINGMANBUTTONS ; i++) {
          if (!hmiINILocateItem(sInstance,wingmanControlLabel[i])) {
               hmiINIAddItemDecimal(sInstance,wingmanControlLabel[i],
                                    (WORD)configWingman[i],
                                    strlen(wingmanControlLabel[i])+4,16);
          }
          else {
               hmiINIWriteDecimal(sInstance,(WORD)configWingman[i]);
          }
     }
}
#endif

#ifdef CON_VFX1
void
readVFX1Config(_INI_INSTANCE *sInstance)
{
     int  i,j;

     for (i=0 ; i < MAXVFX1BUTTONS ; i++) {
          configVFX1[i]=-1;
     }
     if (!hmiINILocateSection(sInstance,"VFX1")) {
          return;
     }
     for (i=0 ; i < MAXVFX1BUTTONS ; i++) {
          if (hmiINIGetItemString(sInstance,VFX1ControlLabel[i],
                                  tempBuf,16)) {
               sscanf(tempBuf,"%x",&j);
               configVFX1[i]=(signed char)j;
          }
     }
}

void
writeVFX1Config(_INI_INSTANCE *sInstance)
{
     int  i;

     if (!hmiINILocateSection(sInstance,"VFX1")) {
          hmiINIAddSection(sInstance,"VFX1");
     }
     for (i=0 ; i < MAXVFX1BUTTONS ; i++) {
          if (!hmiINILocateItem(sInstance,VFX1ControlLabel[i])) {
               hmiINIAddItemDecimal(sInstance,VFX1ControlLabel[i],
                                    (WORD)configVFX1[i],
                                    strlen(VFX1ControlLabel[i])+4,16);
          }
          else {
               hmiINIWriteDecimal(sInstance,(WORD)configVFX1[i]);
          }
     }
}
#endif

void
readVideoMode(_INI_INSTANCE *sInstance)
{
     int  i;

     videoModeOption=0;
     if (!hmiINILocateSection(sInstance,"VIDEO")) {
          return;
     }
     if (hmiINILocateItem(sInstance,"Resolution")) {
          if (hmiINIGetString(sInstance,tempBuf,40)) {
               for (i=0 ; i < NUMVIDEOMODES ; i++) {
                    if (stricmp(videoModeList[i],tempBuf) == 0) {
                         videoModeOption=i;
                         return;
                    }
               }
          }
     }
}

void
writeVideoMode(_INI_INSTANCE *sInstance)
{
     if (!hmiINILocateSection(sInstance,"VIDEO")) {
          hmiINIAddSection(sInstance,"VIDEO");
     }
     if (!hmiINILocateItem(sInstance,"Resolution")) {
          hmiINIAddItemString(sInstance,"Resolution",
                              videoModeList[videoModeOption],_SETUP_JUSTIFY);
     }
     else {
          hmiINIWriteString(sInstance,videoModeList[videoModeOption]);
     }
}

void
readControlConfigs(void)
{
     _INI_INSTANCE sInstance;

     strcpy(controlConfigFile,"CONTROLS.CFG");
     if (!hmiINIOpen(&sInstance,controlConfigFile)) {
          return;
     }
     readKeyboardConfig(&sInstance);
#ifdef CON_MOUSE
     readMouseConfig(&sInstance);
#endif
#ifdef CON_JOYSTICK
     readJoystickConfig(&sInstance);
#endif
#ifdef CON_AVENGER
     readAvengerConfig(&sInstance);
#endif
#ifdef CON_GAMEPAD
     readGamepadConfig(&sInstance);
#endif
#ifdef CON_WINGMAN
     readWingmanConfig(&sInstance);
#endif
#ifdef CON_VFX1
     readVFX1Config(&sInstance);
#endif
     readVideoMode(&sInstance);
     hmiINIClose(&sInstance);
}

void
writeControlConfigs(void)
{
     WORD hFile;
     _INI_INSTANCE sInstance;

     if (!hmiINIOpen(&sInstance,controlConfigFile)) {
          if ((hFile=creat(controlConfigFile,S_IREAD|S_IWRITE)) == -1) {
               return;
          }
          close(hFile);
          if (!hmiINIOpen(&sInstance,controlConfigFile)) {
               return;
          }
     }
     writeKeyboardConfig(&sInstance);
#ifdef CON_MOUSE
     writeMouseConfig(&sInstance);
#endif
#ifdef CON_JOYSTICK
     writeJoystickConfig(&sInstance);
#endif
#ifdef CON_AVENGER
     writeAvengerConfig(&sInstance);
#endif
#ifdef CON_GAMEPAD
     writeGamepadConfig(&sInstance);
#endif
#ifdef CON_WINGMAN
     writeWingmanConfig(&sInstance);
#endif
#ifdef CON_VFX1
     writeVFX1Config(&sInstance);
#endif
     writeVideoMode(&sInstance);
     hmiINIClose(&sInstance);
}
