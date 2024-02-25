#!/bin/bash

# GitHub actions - Create QEMU installer for Windows

# Author: Stefan Weil (2020-2024)

#~ set -e
set -x

ARCH=$1

if test "$ARCH" = "i686"; then
  MINGW=mingw32
else
  ARCH=x86_64
  MINGW=mingw64
fi

ROOTDIR=$PWD
DISTDIR=$ROOTDIR/dist
HOST=$ARCH-w64-mingw32
BUILDDIR=bin/ndebug/$HOST

mkdir -p $DISTDIR

#~ TAG=5.0.0-alpha.$(date +%Y%m%d)

#~ git config --global user.email "sw@weilnetz.de"
#~ git config --global user.name "Stefan Weil"
#~ git tag -a v$TAG -m "QEMU $TAG"

# Build QEMU installer.

echo Building $HOST...
mkdir -p $BUILDDIR && cd $BUILDDIR

# Run configure.
../../../configure --cross-prefix=$HOST- --disable-guest-agent-msi \
    --disable-werror \
    --enable-strip \
    --extra-cflags="-isystem /$MINGW/include" \
    --extra-ldflags="-L/$MINGW/lib"

cp config.log $DISTDIR/

make

echo Building installers...
date=$(date +%Y%m%d)
INSTALLER=$DISTDIR/qemu-$ARCH-setup-$date.exe
#make installer SIGNCODE=signcode INSTALLER=$INSTALLER
make installer SIGNCODE=true
mv -v qemu-setup-*.exe $INSTALLER

echo Calculate SHA-512 checksum...
(cd $DISTDIR; exe=$(basename $INSTALLER); sha512sum $exe >${exe/exe/sha512})
