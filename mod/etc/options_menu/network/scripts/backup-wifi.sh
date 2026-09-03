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

wpa_supplicant_conf="$wpa_supplicant/wpa_supplicant.conf"
ssid="$(grep -o 'ssid=".*"' $wpa_supplicant_conf | sed 's/^ssid="\(.*\)".*/\1/')"
ssid_lower="$(echo $ssid | awk '{print tolower($0)}')"

[ "$1" = "nand" ] && backup_path="$rootfs/etc/wifi_backup"
network="$backup_path/$ssid_lower"

Backup_Wifi() {
  if [ -d "$backup_path" ]; then
    [ -d "$network" ] && rm -rf "$network"
  fi
  mkdir -p 777 "$network"
  cp -r "$wpa_supplicant_conf" "$network"

}

Restore_Wifi() {
  if [ -n "$1" ]; then
    backup_conf="$backup_path/$1/wpa_supplicant.conf"
    rm -f "$wpa_supplicant_conf"
    cp -r "$backup_conf" "$wpa_supplicant"
    ifconfig wlan0 down
    echo "WIFI config restored."
    echo ""
    sleep 2
    ifconfig wlan0 up
  else
    echo "WIFI config does not exist."
  fi
}

case "$2" in
backup)
  decodepng "$omImages/backup_wifi.png" >/dev/fb0
  Backup_Wifi
  echo "WIFI config backed up to $network"
  sleep 1
  sh "$omScripts/om_network"
  ;;
restore)
  Restore_Wifi $3
  sh "$omNetworkScripts/reconnect.sh"
  ;;
esac
