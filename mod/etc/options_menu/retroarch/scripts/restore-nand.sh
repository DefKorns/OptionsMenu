#!/bin/sh

source $mountpoint/etc/options_menu/retroarch/scripts/ra_vars

if [ ! -f "/etc/ra_backup/retroarch.cfg" ]; then
  echo "Backup file does not exist."
else
  /etc/options_menu/retroarch/scripts/RASettingsBackup "Restore" "/etc/options_menu/retroarch/default_files/retroarch.cfg" "/etc/libretro/retroarch.cfg" "$rootfs/etc/ra_backup/retroarch.cfg"
  /etc/options_menu/retroarch/scripts/RASettingsBackup "Restore" "/etc/options_menu/retroarch/default_files/retroarch-core-options.cfg" "/etc/libretro/retroarch-core-options.cfg" "$rootfs/etc/ra_backup/retroarch-core-options.cfg"
  rm -rf "${raConfig:?}"
  cp -r /etc/ra_backup/config "$raConfig"
  echo "Retroarch configs restored."
fi
