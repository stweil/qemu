#!/bin/bash

# GitHub actions - Install packages for Windows cross builds

# Author: Stefan Weil (2023-2024)

ARCH=$1

if test "$ARCH" = "i686"; then
  MINGW=mingw32
else
  ARCH=x86_64
  MINGW=mingw64
fi

# Install required packages.

sudo apt-get update
sudo apt-get install --no-install-recommends --yes \
  git libarchive-dev libcurl3-dev libgpgme-dev libssl-dev make meson sudo \
  curl make pkg-config \
  mingw-w64-tools ninja-build nsis \
  gcc libc6-dev \
  g++-mingw-w64-${ARCH/_/-} gcc-mingw-w64-${ARCH/_/-} \
  bison bzip2 flex gettext python3-sphinx python3-venv texinfo

# Get newer version of mingw-w64-*-dev (with pathcch).

if test "$ARCH" = "i686"; then
curl -sS -O http://de.archive.ubuntu.com/ubuntu/pool/universe/m/mingw-w64/mingw-w64-i686-dev_10.0.0-3_all.deb
sudo dpkg -i mingw-w64-i686-dev_10.0.0-3_all.deb
else
curl -sS -O http://de.archive.ubuntu.com/ubuntu/pool/universe/m/mingw-w64/mingw-w64-x86-64-dev_10.0.0-3_all.deb
sudo dpkg -i mingw-w64-x86-64-dev_10.0.0-3_all.deb
fi

# Install pacman.

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

# Install packages for cross build with pacman.

sudo pacman -S --noconfirm mingw-w64-$ARCH-SDL2 mingw-w64-$ARCH-SDL2_image mingw-w64-$ARCH-asciidoc mingw-w64-$ARCH-cairo mingw-w64-$ARCH-capstone mingw-w64-$ARCH-curl-winssl mingw-w64-$ARCH-cyrus-sasl mingw-w64-$ARCH-gnutls mingw-w64-$ARCH-gtk3 mingw-w64-$ARCH-headers-git mingw-w64-$ARCH-icu mingw-w64-$ARCH-jack2 mingw-w64-$ARCH-leptonica mingw-w64-$ARCH-libarchive mingw-w64-$ARCH-libb2 mingw-w64-$ARCH-libnfs mingw-w64-$ARCH-libslirp mingw-w64-$ARCH-libssh mingw-w64-$ARCH-lz4 mingw-w64-$ARCH-pango mingw-w64-$ARCH-pdcurses mingw-w64-$ARCH-snappy mingw-w64-$ARCH-spice mingw-w64-$ARCH-usbredir mingw-w64-$ARCH-virglrenderer

sudo mkdir -p /usr/local/$ARCH-w64-mingw32/lib
sudo ln -s /$MINGW/lib/pkgconfig /usr/local/$ARCH-w64-mingw32/lib

# Use the native gdbus-codegen instead of failing with the wrong one.
sudo rm -fv /$MINGW/bin/gdbus-codegen.exe
