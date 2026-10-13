#!/bin/sh

source /etc/options_menu/scripts/om_functions

if [ ! -d /media/data/saves_backup ]; then
  translate RESTORE_SAVES_USB_NOT_FOUND
  exit 1
fi

rsync -a --exclude=game_names.txt /media/data/saves_backup/ /media/hakchi/saves/ || exit 1
sh /etc/options_menu/scripts/OrphanSaves.sh merge-backup-names
translate RESTORE_SAVES_USB_DONE
