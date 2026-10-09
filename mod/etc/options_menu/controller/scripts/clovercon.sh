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
# clovercon.sh state|toggle <param>
# clovercon.sh is|set <param> <value>
# clovercon.sh home state|toggle
# Saves cfg_clovercon_<param> and applies it live through sysfs.
source /etc/preinit
script_init

home_disabled="0x7FFF"
home_default="0x0024"
home_prev_file="$mountpoint/etc/options_menu/controller/home_combination.prev"

getCfg() {
  eval "echo \"\$cfg_clovercon_$1\""
}

apply() {
  eval "cfg_clovercon_$1='$2'"
  save_config
  param_file="/sys/module/clovercon/parameters/$1"
  [ -w "$param_file" ] && echo "$2" > "$param_file"
}

onOff() {
  if [ "$1" = "y" ]; then echo "on"; else echo "off"; fi
}

homeEnabled() {
  current="$(getCfg home_combination)"
  [ -n "$current" ] && [ "$(printf '%d' "$current")" != "$(printf '%d' "$home_disabled")" ]
}

case "$1" in
state)
  [ "$(getCfg "$2")" = "1" ] && onOff y || onOff n
  ;;
toggle)
  if [ "$(getCfg "$2")" = "1" ]; then apply "$2" 0; else apply "$2" 1; fi
  ;;
is)
  [ "$(getCfg "$2")" = "$3" ] && onOff y || onOff n
  ;;
set)
  apply "$2" "$3"
  ;;
home)
  case "$2" in
  state)
    homeEnabled && onOff y || onOff n
    ;;
  toggle)
    if homeEnabled; then
      mkdir -p "$(dirname "$home_prev_file")"
      getCfg home_combination > "$home_prev_file"
      apply home_combination "$home_disabled"
    else
      prev="$(cat "$home_prev_file" 2>/dev/null)"
      apply home_combination "${prev:-$home_default}"
    fi
    ;;
  esac
  ;;
esac
