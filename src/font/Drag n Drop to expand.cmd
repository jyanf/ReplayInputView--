@echo off
set /p o= output name: 

fontforge --script font-expand.py %1 %o%

pause