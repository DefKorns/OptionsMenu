#!/bin/sh
#  Copyright (C) 2026 DefKorns (https://defkorns.github.io/LICENSE)
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
# shellcheck source=/dev/null
source /etc/preinit
script_init
source "$mountpoint/etc/options_menu/scripts/om_functions"

disk_usage() {
  df -h "$1" 2>/dev/null | tail -1 | awk -v free="$(translate SYSINFO_FREE)" '{ print $3 " / " $2 " (" $4 " " free ")" }'
}

[ -f /var/version ] && source /var/version

echo "$(translate SYSINFO_CONSOLE): $sftype-$sfregion"
echo "hakchi: $hakchiVersion"
echo "Kernel: $kernelVersion"
echo "Boot: $bootVersion"
echo "$(translate SYSINFO_FIRMWARE): $(hakchi currentFirmware 2>/dev/null | sed "s/^_nand_$/NAND/")"
echo " "
echo "NAND: $(disk_usage "$mountpoint/var/lib")"
[ -d "$usb_path" ] && echo "USB/SD: $(disk_usage "$usb_path")"
saves="$(readlink -f /var/saves)"
[ -d "$saves" ] && echo "$(translate SYSINFO_SAVES): $(du -sh "$saves" 2>/dev/null | cut -f1)"
