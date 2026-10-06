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
# armet.sh state|toggle
# protection is on unless cfg_disable_armet=y (clover-kachikachi-wr)
source /etc/preinit
script_init

case "$1" in
state)
  if [ "$cfg_disable_armet" = "y" ]; then echo "off"; else echo "on"; fi
  ;;
toggle)
  if [ "$cfg_disable_armet" = "y" ]; then cfg_disable_armet='n'; else cfg_disable_armet='y'; fi
  save_config
  ;;
esac
