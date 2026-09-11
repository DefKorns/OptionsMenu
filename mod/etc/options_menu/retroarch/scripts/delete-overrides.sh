#!/bin/sh

source $mountpoint/etc/options_menu/retroarch/scripts/ra_vars

mv "$raConfig/remaps" /tmp/ra_remaps
rm -r "${raConfig:?}"/*
mv /tmp/ra_remaps "$raConfig/remaps"
echo "Overrides deleted."
