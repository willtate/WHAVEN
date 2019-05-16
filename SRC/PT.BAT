@echo off
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whaven.c
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whtag.c
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whplr.c
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whobj.c
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whinp.c
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whfx.c
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whsndmod.c
wcc386 -d_PROTOTYPING_ -zs -zg -mf -oneatx -zp4 -5r whmenu.c
wtouch icorp.h

