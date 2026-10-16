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
# mod_uninstall exit: 0 = back, 2 = uninstalling (reboots), else leave menu
options_path="${1%/}"

"$options_path/mod_uninstall/mod_uninstall"
case $? in
0)
  usleep 50000
  "$options_path/options" --commandPath "$options_path/advanced_commands/" --title "ADVANCED_OPTIONS"
  ;;
2) ;;
*)
  sh "$options_path/scripts/ResumeUI.sh"
  ;;
esac
