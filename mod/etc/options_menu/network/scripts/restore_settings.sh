#!/bin/sh
# shellcheck disable=SC2154,SC1091,SC1090
#  Copyright 2022 DefKorns (https://gitlab.com/DefKorns/om-wifi-backup/LICENSE)
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# shellcheck disable=SC2086,SC3046
source $mountpoint/etc/options_menu/network/scripts/om_vars
script_init

if [ "$1" = "nand" ]; then
  backup_path="$rootfs/etc/wifi_backup"
  dst="$1"
fi

find "$omWifiRestoreCmds" -type f -name 'c0001_*' -print0 -exec rm {} \;

find "$backup_path/." -mindepth 1 -maxdepth 1 -type d -print0 | xargs -0 -n 1 basename | while IFS= read -r wifi_network; do
  wpa_supplicant_conf="$backup_path/$wifi_network/wpa_supplicant.conf"
  ssid="$(grep -o 'ssid=".*"' $wpa_supplicant_conf | sed 's/^ssid="\(.*\)".*/\1/')"
  ssid_lower="$(echo $ssid | awk '{print tolower($0)}')"
  echo "COMMAND_NAME=$ssid
COMMAND_TYPE=INTERNAL
RESTART_UI=FALSE
COMMAND_STR=sh $omNetworkScripts/backup-wifi.sh $dst restore $ssid_lower" >"$omWifiRestoreCmds/c0001_$ssid_lower"
done

$optionsMenu/options --commandPath $omWifiRestoreCmds/ --scriptPath $omWifiRestoreScripts --title "Restore WIFI Config" &
