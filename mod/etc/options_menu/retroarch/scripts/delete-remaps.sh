#!/bin/sh

source $mountpoint/etc/options_menu/retroarch/scripts/ra_vars

rm -r "${ra_config:?}/remaps"/*
echo "Remaps deleted."
