#!/bin/bash

# GitHub actions - Install packages for Windows cross builds

# Author: Stefan Weil (2023)

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

sudo apt-get update
sudo apt-get install --no-install-recommends -y git libarchive-dev libcurl3-dev libgpgme-dev libssl-dev make meson sudo

git clone https://gitlab.archlinux.org/sw/pacman.git
(
cd pacman/
meson setup build
cd build
sudo meson install
)

sudo mkdir /etc/pacman.d
echo "[$MINGW]" | sudo tee -a /etc/pacman.conf
echo "Include = /etc/pacman.d/mirrorlist.mingw" | sudo tee -a /etc/pacman.conf
curl https://raw.githubusercontent.com/msys2/MSYS2-packages/master/pacman-mirrors/mirrorlist.mingw | sudo tee /etc/pacman.d/mirrorlist.mingw

git clone https://github.com/msys2/MSYS2-keyring.git
(
cd MSYS2-keyring
sudo make
)

sudo ln -s /usr/local/share/pacman/keyrings /usr/share/pacman/
sudo pacman-key --init
sudo pacman-key --populate msys2
sudo pacman -Syu

test -e /etc/mtab || sudo ln -s ../proc/self/mounts /etc/mtab

sudo pacman -S --noconfirm mingw-w64-$ARCH-SDL2 mingw-w64-$ARCH-SDL2_image mingw-w64-$ARCH-asciidoc mingw-w64-$ARCH-cairo mingw-w64-$ARCH-capstone mingw-w64-$ARCH-curl-winssl mingw-w64-$ARCH-cyrus-sasl mingw-w64-$ARCH-gnutls mingw-w64-$ARCH-gtk3 mingw-w64-$ARCH-headers-git mingw-w64-$ARCH-icu mingw-w64-$ARCH-jack2 mingw-w64-$ARCH-leptonica mingw-w64-$ARCH-libarchive mingw-w64-$ARCH-libb2 mingw-w64-$ARCH-libnfs mingw-w64-$ARCH-libslirp mingw-w64-$ARCH-libssh mingw-w64-$ARCH-lz4 mingw-w64-$ARCH-pango mingw-w64-$ARCH-pdcurses mingw-w64-$ARCH-snappy mingw-w64-$ARCH-spice mingw-w64-$ARCH-usbredir mingw-w64-$ARCH-virglrenderer

sudo mkdir -p /usr/local/$ARCH-w64-mingw32/lib
sudo ln -s /$MINGW/lib/pkgconfig /usr/local/$ARCH-w64-mingw32/lib

# Use the native gdbus-codegen instead of failing with the wrong one.
sudo rm -fv /$MINGW/bin/gdbus-codegen.exe
