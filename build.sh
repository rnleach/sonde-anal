#!/bin/sh -l

PROJDIR="$(pwd)"
SOURCEDIR="$PROJDIR/src"
GUIDIR="$PROJDIR/gui"
TESTDIR="$PROJDIR/tests"

CFLAGS="-Wall -Werror -Wno-unknown-pragmas -std=c11 -march=native"
CFLAGS="$CFLAGS -D_DEFAULT_SOURCE -D_GNU_SOURCE -I$SOURCEDIR -I$TESTDIR"
LDLIBS=" -lraylib -lGL -lm -lpthread -ldl -lrt -lX11"

CC=cc

if [ "$#" -gt 0 -a "$1" = "debug" ]
then
    echo "debug build"
    CFLAGS="$CFLAGS -O0 -g"
elif [ "$#" -gt 0 -a "$1" != "clean" -o \( "$#" = 0 \) ]
then
    echo "release build"
    CFLAGS="$CFLAGS -Wno-array-bounds -Wno-unused-but-set-variable -O3 -DNDEBUG"
fi

if [ "$#" -gt 0 -a "$1" = "clean" ] 
then
    echo "clean compiled programs"
    echo
    rm -f main_test sonde2
    rm -r -f *.dSYM *.o
else
    $CC $CFLAGS $TESTDIR/main.c -o main_test $LDLIBS
    $CC $CFLAGS $GUIDIR/main.c -o sonde2 $LDLIBS
fi

if [ "$#" -gt 0 -a \( "$1" = "test" -o "$2" = "test" \) ]
then
    ./main_test
fi

if [ "$#" -gt 0 -a \( "$1" = "run" -o "$2" = "run" -o "$3" = "run" \) ]
then
    ./sonde2
fi

