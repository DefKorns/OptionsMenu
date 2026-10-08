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
# Saves of games that are no longer on the console. An entry is either a game
# code (CLV-..., the folder of its SRAM and suspend points) or retroarch/<content>
# (every RetroArch save/state file of that content).
#   OrphanSaves.sh stats              saves, USB backup (-1 = none), orphans, moved aside
#   OrphanSaves.sh orphans            entries with saves but no game
#   OrphanSaves.sh deleted            entries already moved aside
#   OrphanSaves.sh move FILE          move the entries listed in FILE aside
#   OrphanSaves.sh restore FILE       move them back (never over an existing save)

source /etc/preinit
script_init

saves_root=/media/hakchi/saves
backup_root=/media/data/saves_backup
deleted_root=/media/data/deleted_games_saves
ra_save_suffix='\.(srm|rtc|state([0-9]+)?(\.auto)?)(\.png)?$'

raContent() {
  sed -E "s/$ra_save_suffix//"
}

saveEntries() {
  ls "$1" 2>/dev/null | grep "^CLV-" | grep -v "^CLV-S-"
  ls "$1/retroarch" 2>/dev/null | raContent | sed 's#^#retroarch/#'
}

entriesOnConsole() {
  local games_root
  games_root="$(dirname "$(findGameStorage)")"
  {
    find "$games_root" -maxdepth 4 -name "*.desktop" -exec sed -n "s/^Code=//p" {} +
    ls "$games_root"/*/.storage "$squashfs/usr/share/games"
    {
      find "$games_root"/*/.storage "$squashfs/usr/share/games" "$rootfs/usr/share/games" -mindepth 2 -maxdepth 2 -type f
      find "$games_root" -maxdepth 4 -name "*.desktop" -exec sed -n "s/^Exec=//p" {} + | tr ' ' '\n' | grep /
    } | sed 's#.*/##' | while IFS= read -r rom; do
      echo "retroarch/$rom"
      echo "retroarch/${rom%.*}"
    done
  } 2>/dev/null | grep -E "^(CLV-|retroarch/)" | sort -u
}

listOrphans() {
  local on_console
  on_console="$(mktemp)"
  entriesOnConsole > "$on_console"
  saveEntries "$saves_root" | sort -u | comm -23 - "$on_console"
  rm -f "$on_console"
}

isEntry() {
  case "$1" in
    CLV-S-*|*/*/*|retroarch/|retroarch/.|retroarch/..) return 1 ;;
    CLV-*) case "$1" in */*) return 1 ;; esac ;;
    retroarch/*) ;;
    *) return 1 ;;
  esac
}

moveEntry() {
  local from="$1" to="$2" entry="$3" content file
  case "$entry" in
    retroarch/*)
      content="${entry#retroarch/}"
      mkdir -p "$to/retroarch"
      ls "$from/retroarch" 2>/dev/null | while IFS= read -r file; do
        [ "$(echo "$file" | raContent)" = "$content" ] || continue
        [ -e "$to/retroarch/$file" ] && [ "$to" = "$saves_root" ] && continue
        mv -f "$from/retroarch/$file" "$to/retroarch/$file"
      done
      rmdir "$from/retroarch" 2>/dev/null
      ;;
    *)
      [ -d "$from/$entry" ] || return
      [ -e "$to/$entry" ] && [ "$to" = "$saves_root" ] && return
      rm -rf "$to/$entry"
      mv "$from/$entry" "$to/$entry"
      ;;
  esac
}

moveListed() {
  local from="$1" to="$2" list="$3" entry
  mkdir -pm 777 "$to"
  while IFS= read -r entry; do
    isEntry "$entry" && moveEntry "$from" "$to" "$entry"
  done < "$list"
  rmdir "$deleted_root" 2>/dev/null
  sync
}

case "$1" in
  stats)
    saveEntries "$saves_root" | sort -u | wc -l
    if [ -d "$backup_root" ]; then saveEntries "$backup_root" | sort -u | wc -l; else echo -1; fi
    listOrphans | wc -l
    saveEntries "$deleted_root" | sort -u | wc -l
    ;;
  orphans)
    listOrphans
    ;;
  deleted)
    saveEntries "$deleted_root" | sort -u
    ;;
  move)
    [ -f "$2" ] && moveListed "$saves_root" "$deleted_root" "$2"
    ;;
  restore)
    [ -f "$2" ] && moveListed "$deleted_root" "$saves_root" "$2"
    ;;
  *)
    echo "usage: $0 stats|orphans|deleted|move FILE|restore FILE" >&2
    exit 2
    ;;
esac
