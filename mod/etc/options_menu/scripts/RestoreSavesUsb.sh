#!/bin/sh

source /etc/options_menu/scripts/om_functions

if [ ! -d /media/data/saves_backup ]; then
  translate RESTORE_SAVES_USB_NOT_FOUND
  exit 1
fi

rsync -a /media/data/saves_backup/ /media/hakchi/saves/
translate RESTORE_SAVES_USB_DONE
