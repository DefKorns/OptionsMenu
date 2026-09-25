#!/bin/sh

source $mountpoint/etc/options_menu/retroarch/scripts/ra_vars

if [ -f "/etc/options_menu/retroarch/default_files/retroarch.cfg" ]; then
  cp /etc/options_menu/retroarch/default_files/retroarch.cfg /etc/libretro/
  cp /etc/options_menu/retroarch/default_files/retroarch-core-options.cfg /etc/libretro/
  rm -r "${raConfig:?}"/*
  mkdir -m 777 "$raConfig/remaps"
  echo "Restored Default Settings."
else
  echo "Cannot locate default configuration files."
  echo "Please ensure RetroArch is installed."
fi
