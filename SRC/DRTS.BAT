@echo off
if exist h:\nul goto actdrts
echo *
echo * Load IPX & NETX then run this batch file again
echo *
goto done
:actdrts
if "%OLDPATH%" == "" goto actenv
path=%OLDPATH%
:actenv
set DRTSUID=Rafael
set DRTSUSER=Rafael
set DRTSENVDIR=H:\WHSRC
set DRTSENVMAP=H:\WHSRC\LESENV.MAP
set OLDPATH=%PATH%
path=%OLDPATH%;H:\DRTS
:done
