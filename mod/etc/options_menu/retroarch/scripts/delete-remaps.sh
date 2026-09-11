#!/bin/sh

source $mountpoint/etc/options_menu/retroarch/scripts/ra_vars

rm -r "${raConfig:?}/remaps"/*
echo "Remaps deleted."
