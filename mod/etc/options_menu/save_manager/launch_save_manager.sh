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
# save_manager exit: 0 = back to the menu that opened it (top of OM_BACK_STACK), else leave the menu
options_path="${1%/}"

"$options_path/save_manager/save_manager"
if [ $? -ne 0 ]; then
  sh "$options_path/scripts/ResumeUI.sh"
  exit 0
fi

entry="${OM_BACK_STACK##*;}"
remaining="${OM_BACK_STACK%;*}"
[ "$remaining" = "$OM_BACK_STACK" ] && remaining=""
command_path="${entry%%,*}"
rest="${entry#*,}"
script_path="${rest%%,*}"
title_key="${rest#*,}"

usleep 50000
if [ -n "$script_path" ]; then
  OM_BACK_STACK="$remaining" "$options_path/options" --commandPath "$command_path" --scriptPath "$script_path" --title "$title_key"
else
  OM_BACK_STACK="$remaining" "$options_path/options" --commandPath "$command_path" --title "$title_key"
fi
