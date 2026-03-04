rem
rem build.bat clean - deletes build artefacts
rem build.bat - builds the "optimized" version of main_test.exe
rem build.bat debug - builds the "debug" version of main_test.exe
rem

@echo off
cls
SETLOCAL

rem
rem Set up compiler flags
rem
IF "%1"=="debug" (GOTO Debug) ELSE (GOTO Release)

:Debug
@echo "Debug Build"
SET flags=/Od /Zi /favor:INTEL64 /arch:AVX2 /WX /W3 
GOTO Operation

:Release
@echo "Release Build"
SET flags=/O2 /favor:INTEL64 /arch:AVX2 /DNDEBUG /WX /W3 
GOTO Operation

rem
rem The main operation....build or clean?
rem
:Operation
IF "%1"=="clean" GOTO Clean 
GOTO BuildAll

rem
rem Clean up operations
rem
:Clean
@echo "Clean"
del *.exe *.obj *.pdb *.ilk 
GOTO EndSuccess

rem
rem Build All
rem
:BuildAll

@echo "Build"
cl /std:c11 /TC /utf-8 /nologo %flags% tests\main.c
IF "%1"=="test" (GOTO Test) ELSE (GOTO EndSuccess)

rem
rem Test
rem
:Test
main.exe


rem
rem Exit Quietly with no errors
rem
:EndSuccess

