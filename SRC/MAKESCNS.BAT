@echo off
copy scnbgn.asm screens.asm
:loop
if "%1" == "" goto assem
scn2asm %1.bin %1.asm
copy screens.asm+%1.asm
shift
goto loop
:assem
copy screens.asm+scnend.asm
tasm screens
