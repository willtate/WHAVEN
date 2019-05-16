@echo off
del errors
wmake /f whaven.mak
more < errors

