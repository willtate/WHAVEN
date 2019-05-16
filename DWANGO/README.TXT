Dapi Beta Version 0.2, January 4, 1996
---------------------------------------

Contents:
---------
Dapi.doc        Dapi specification in Word 7.0 format
Dapi.txt        Text version of Dapi spec.

dapi.h          Dapi include file.
dapi.lib        Dapi runtime library.
dapi.cfg        Dapi config file.

*.exe           Executable Dapi samples.
*.c             Source for Dapi samples.
*.cfg           Sample dapi config files: rename to dapi.cfg to use.
makefile        Watcom 10.0 makefile for Dapi samples.


Known Dapi Problems:
--------------------
1) Sending large messages (200-400 bytes) does not always work.

2) All nodes must restart their games if one node restarts
   its game; in order for dapiSendAcked() to work correctly.

-end.

