#!/bin/sh

source /etc/options_menu/scripts/om_functions

# system state, not saves: a restore must not overwrite it
mkdir -pm 777 /media/data/saves_backup
rsync -a --delete \
  --exclude=FOLDER --exclude=hakchi --exclude=home-menu \
  --exclude=dimming.txt --exclude=lang.txt --exclude=mcp-state \
  /media/hakchi/saves/ /media/data/saves_backup/ || exit 1
translate BACKUP_SAVES_USB_DONE
