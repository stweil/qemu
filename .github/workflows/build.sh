#!/bin/bash

# GitHub actions - Create QEMU installer for Windows

# Author: Stefan Weil (2020-2023)

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

echo deb http://de.archive.ubuntu.com/ubuntu kinetic main universe | \
  sudo tee /etc/apt/sources.list.d/kinetic.list

sudo apt-get update
sudo apt-get install --yes curl make pkg-config

# Install packages.
sudo apt-get update
sudo apt-get install --yes --no-install-recommends \
  mingw-w64-tools ninja-build nsis \
  gcc libc6-dev \
  g++-mingw-w64-${ARCH/_/-} gcc-mingw-w64-${ARCH/_/-} \
  bison flex gettext python3-sphinx texinfo

# Get newer version of mingw-w64-*-dev.
if test "$ARCH" = "i686"; then
curl -sS -O http://de.archive.ubuntu.com/ubuntu/pool/universe/m/mingw-w64/mingw-w64-i686-dev_10.0.0-3_all.deb
sudo dpkg -i mingw-w64-i686-dev_10.0.0-3_all.deb
else
curl -sS -O http://de.archive.ubuntu.com/ubuntu/pool/universe/m/mingw-w64/mingw-w64-x86-64-dev_10.0.0-3_all.deb
sudo dpkg -i mingw-w64-x86-64-dev_10.0.0-3_all.deb
fi

# Get header files for WHPX API from Mingw-w64 git master.
if test "$ARCH" != "i686"; then
(
sudo mkdir -p /usr/$HOST/sys-include
cd /usr/$HOST/sys-include
sudo curl -sS -o winhvemulation.h https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-headers/include/winhvemulation.h?format=raw
sudo curl -sS -o winhvplatform.h https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-headers/include/winhvplatform.h?format=raw
sudo curl -sS -o winhvplatformdefs.h https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-headers/include/winhvplatformdefs.h?format=raw
sudo ln -s winhvemulation.h WinHvEmulation.h
sudo ln -s winhvplatform.h WinHvPlatform.h
sudo ln -s winhvplatformdefs.h WinHvPlatformDefs.h
)
fi

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
