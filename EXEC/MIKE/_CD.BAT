@echo off

if (%1) == () goto no_drive
        echo %1 >>\x\cd.txt
    echo %1 >\x\_cd.dat
    exec -x sh %%+d0 <\x\_cd.dat
REM rm -n \x\_cd.dat
:no_drive

:loop
if (%1) == () goto endloop
        echo %1 >>\x\cd.txt
    cd %1
    shift
    goto loop

:endloop
