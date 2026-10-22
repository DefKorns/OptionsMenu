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
# ingame_menu.sh state|toggle|blocked
# blocked exits 0 when the combo must be ignored: in-game menu off and a game holds /dev/fb0.
blocked_file="/etc/options_menu/ingame_blocked"

inGame() {
  local pid
  for pid in $(lsof -n 2>/dev/null | grep -F /dev/fb0 | awk '{print $1}' | sort -u); do
    case "$(readlink "/proc/$pid/exe")" in */ReedPlayer-Clover) continue ;; esac
    tr '\0' ' ' < "/proc/$pid/cmdline" | grep -qF /autoplay/ && continue
    return 0
  done
  return 1
}

case "$1" in
state)
  if [ -f "$blocked_file" ]; then echo "off"; else echo "on"; fi
  ;;
toggle)
  if [ -f "$blocked_file" ]; then rm -f "$blocked_file"; else touch "$blocked_file"; fi
  ;;
blocked)
  [ -f "$blocked_file" ] && inGame
  ;;
*)
  exit 1
  ;;
esac
