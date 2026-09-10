#!/bin/sh

#Create Data directories if it's not already created and mounted...
mkdir -m 777 /media/data/
mkdir -m 777 /media/data/log
mkdir -m 777 /media/data/log/RetroArchConfig

#Clear down the log files
rm /media/data/log/Hakchi_retroarch_debug.log
rm -rf /media/data/log/RetroArchConfig/*

{
	chmod -R 777 /etc/libretro #Set full permissions for retroarch
	echo "Dumping retroarch config to log folder..."
	RA_CONFIG=/etc/libretro/.config
	[ -d "$RA_CONFIG" ] || RA_CONFIG=/etc/libretro/config
	cp -r "$RA_CONFIG" /media/data/log/RetroArchConfig
	cp -r /etc/libretro/retroarch-core-options.cfg /media/data/log/RetroArchConfig
	cp -r /etc/libretro/retroarch.cfg /media/data/log/RetroArchConfig
} >> /media/data/log/Hakchi_retroarch_debug.log 2>&1
if /bin/retroarch-mini null >> /media/data/log/Hakchi_retroarch_debug.log 2>&1 ; then
	echo "RetroArch successfully ran using the test rig at $(date)" >> /media/data/log/Hakchi_retroarch_debug.log
else
	echo "RetroArch FAILED to run using the test rig at $(date)" >> /media/data/log/Hakchi_retroarch_debug.log
fi