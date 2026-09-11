#!/bin/sh

source $mountpoint/etc/options_menu/retroarch/scripts/ra_vars

if [ ! -f "/media/data/ra_backup/retroarch.cfg" ]; then
  echo "Backup file does not exist."
else
  /etc/options_menu/retroarch/scripts/RASettingsBackup "Restore" "/etc/options_menu/retroarch/default_files/retroarch.cfg" "/etc/libretro/retroarch.cfg" "/media/data/ra_backup/retroarch.cfg"
  /etc/options_menu/retroarch/scripts/RASettingsBackup "Restore" "/etc/options_menu/retroarch/default_files/retroarch-core-options.cfg" "/etc/libretro/retroarch-core-options.cfg" "/media/data/ra_backup/retroarch-core-options.cfg"
  rm -rf "${raConfig:?}"
  cp -r /media/data/ra_backup/config "$raConfig"
  echo "Retroarch configs restored."
fi
