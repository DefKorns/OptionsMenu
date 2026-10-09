#!/bin/sh

source /etc/options_menu/scripts/om_functions

# /media/hakchi/saves also holds a few non-save housekeeping entries
# (hakchi's own state, home-menu, dimming/lang/mcp-state) - excluded so a
# later restore can't clobber live system state with a stale backup of it
mkdir -pm 777 /media/data/saves_backup
rsync -a --delete \
  --exclude=FOLDER --exclude=hakchi --exclude=home-menu \
  --exclude=dimming.txt --exclude=lang.txt --exclude=mcp-state \
  /media/hakchi/saves/ /media/data/saves_backup/
translate BACKUP_SAVES_USB_DONE
