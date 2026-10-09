#!/bin/sh

source $mountpoint/etc/options_menu/retroarch/scripts/ra_vars

mv "$ra_config/remaps" /tmp/ra_remaps
rm -r "${ra_config:?}"/*
mv /tmp/ra_remaps "$ra_config/remaps"
echo "Overrides deleted."
