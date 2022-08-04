#!/bin/bash

# GitHub actions - Create QEMU installer for Windows

# Author: Stefan Weil (2020-2022)

#~ set -e
set -x

ARCH=$1
DLLS="libgcc_s_sjlj-1.dll libgomp-1.dll libstdc++-6.dll"

if test "$ARCH" != "i686"; then
  ARCH=x86_64
  DLLS="libgcc_s_seh-1.dll libgomp-1.dll libstdc++-6.dll"
fi

ROOTDIR=$PWD
DISTDIR=$ROOTDIR/dist
HOST=$ARCH-w64-mingw32
BUILDDIR=bin/ndebug/$HOST
PKG_ARCH=mingw64-${ARCH/_/-}

pwd
find .github

echo deb http://de.archive.ubuntu.com/ubuntu kinetic main universe | \
  sudo tee /etc/apt/sources.list.d/kinetic.list

sudo apt-get update
sudo apt-get install --yes curl make pkg-config

sudo ln -sf $PWD/.github/workflows/pkg-config-crosswrapper /usr/bin/$HOST-pkg-config
# file /usr/bin/$HOST-pkg-config
# /usr/bin/$HOST-pkg-config

# Install cygwin key and add cygwin sources.
sudo curl -sS -o /etc/apt/trusted.gpg.d/weilnetz.gpg https://qemu.weilnetz.de/debian/weilnetz.gpg
echo deb https://qemu.weilnetz.de/debian/ testing contrib | \
  sudo tee /etc/apt/sources.list.d/cygwin.list

# Install packages.
sudo apt-get update
sudo apt-get install --yes --no-install-recommends \
  mingw-w64-tools ninja-build nsis \
  gcc libc6-dev \
  g++-mingw-w64-${ARCH/_/-} gcc-mingw-w64-${ARCH/_/-} \
  bison flex gettext python3-sphinx texinfo \
  $PKG_ARCH-adwaita-icon-theme $PKG_ARCH-cogl $PKG_ARCH-curl \
  $PKG_ARCH-gmp $PKG_ARCH-gnutls $PKG_ARCH-gtk3 $PKG_ARCH-icu \
  $PKG_ARCH-libxml2 $PKG_ARCH-ncurses $PKG_ARCH-sdl2 $PKG_ARCH-usbredir
if test "$ARCH" = "i686"; then
curl -sS -O http://de.archive.ubuntu.com/ubuntu/pool/universe/m/mingw-w64/mingw-w64-i686-dev_10.0.0-2_all.deb
sudo dpkg -i mingw-w64-i686-dev_10.0.0-2_all.deb
else
curl -sS -O http://de.archive.ubuntu.com/ubuntu/pool/universe/m/mingw-w64/mingw-w64-x86-64-dev_10.0.0-2_all.deb
sudo dpkg -i mingw-w64-x86-64-dev_10.0.0-2_all.deb
fi

# Workaround for buggy cross pkg-config.
sudo ln -sf $PWD/.github/workflows/pkg-config-crosswrapper /usr/bin/$HOST-pkg-config

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

if test "$ARCH" = "i686"; then
DLL_PATH=$PWD/dll/w32
else
DLL_PATH=$PWD/dll/w64
fi

mkdir -p $DISTDIR
mkdir -p $DLL_PATH

for dll in $DLLS; do
  ln -sf /usr/lib/gcc/$HOST/*-win32/$dll $DLL_PATH
done

DLLS="iconv.dll libatk-1.0-0.dll libbz2-1.dll"
DLLS="$DLLS libcairo-2.dll libcairo-gobject-2.dll libcurl-4.dll"
DLLS="$DLLS libeay32.dll libepoxy-0.dll libexpat-1.dll"
DLLS="$DLLS libffi-6.dll libfontconfig-1.dll libfreetype-6.dll"
DLLS="$DLLS libgdk-3-0.dll libgdk_pixbuf-2.0-0.dll"
DLLS="$DLLS libgio-2.0-0.dll libglib-2.0-0.dll libgmodule-2.0-0.dll"
DLLS="$DLLS libgmp-10.dll libgnutls-30.dll libgobject-2.0-0.dll libgtk-3-0.dll"
DLLS="$DLLS libharfbuzz-0.dll libhogweed-4.dll libidn2-0.dll libintl-8.dll"
DLLS="$DLLS libjpeg-8.dll liblzo2-2.dll"
DLLS="$DLLS libncursesw6.dll libnettle-6.dll libnghttp2-14.dll"
DLLS="$DLLS libp11-kit-0.dll libpango-1.0-0.dll libpangocairo-1.0-0.dll"
DLLS="$DLLS libpangoft2-1.0-0.dll libpangowin32-1.0-0.dll libpcre-1.dll"
DLLS="$DLLS libpixman-1-0.dll libpng16-16.dll libssh2-1.dll libtasn1-6.dll"
DLLS="$DLLS libunistring-2.dll libusb-1.0.dll libusbredirparser-1.dll"
DLLS="$DLLS SDL2.dll ssleay32.dll zlib1.dll"

for dll in $DLLS; do
  ln -sf /usr/$HOST/sys-root/mingw/bin/$dll $DLL_PATH
done

ln -sf /usr/$HOST/lib/libwinpthread-1.dll $DLL_PATH

#~ TAG=5.0.0-alpha.$(date +%Y%m%d)

#~ git config --global user.email "sw@weilnetz.de"
#~ git config --global user.name "Stefan Weil"
#~ git tag -a v$TAG -m "QEMU $TAG"

# Build QEMU installer.

echo Building $HOST...
mingw=/usr/$HOST/sys-root/mingw
mkdir -p $BUILDDIR && cd $BUILDDIR
#~ rm -r *

#~ file /usr/bin/*gcc*

# Run configure.
../../../configure --cross-prefix=$HOST- --disable-guest-agent-msi \
    --disable-werror \
    --enable-strip \
    --extra-cflags="-I$mingw/include" \
    --extra-ldflags="-L$mingw/lib"

cp config.log $DISTDIR/

make

echo Building installers...
date=$(date +%Y%m%d)
INSTALLER=$DISTDIR/qemu-$ARCH-setup-$date.exe
#make installer DLL_PATH=$DLL_PATH SIGNCODE=signcode INSTALLER=$INSTALLER
make installer DLL_PATH=$DLL_PATH SIGNCODE=true
mv -v qemu-setup-*.exe $INSTALLER

echo Calculate SHA-512 checksum...
(cd $DISTDIR; exe=$(basename $INSTALLER); sha512sum $exe >${exe/exe/sha512})
