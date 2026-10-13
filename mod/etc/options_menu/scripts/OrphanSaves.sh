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
#   OrphanSaves.sh backup             entries in the USB backup
#   OrphanSaves.sh restore-backup FILE  copy the entries listed in FILE back from the USB backup (replaces same names)
#   OrphanSaves.sh hakchi             portable Hakchi CE folders found on the USB drive
#   OrphanSaves.sh names FILE         add the games of the Hakchi folders listed in FILE to the entry<TAB>name cache
#   OrphanSaves.sh merge-names FILE   add the entry<TAB>name lines in FILE to the cache
#   OrphanSaves.sh sync-names         add the games installed on the console to the cache
#   OrphanSaves.sh cached-names       print the name cache
# Name lookups only read Hakchi's .desktop files, saves are never touched.

source /etc/preinit
script_init

saves_root=/media/hakchi/saves
backup_root=/media/data/saves_backup
deleted_root=/media/data/deleted_games_saves
names_cache=/media/data/hakchi_game_names
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

# like the full restore: files with the same name are replaced, the others are left alone
copyFromBackup() {
  local entry="$1" content file
  case "$entry" in
    retroarch/*)
      content="${entry#retroarch/}"
      mkdir -p "$saves_root/retroarch" || return 1
      ls "$backup_root/retroarch" 2>/dev/null | while IFS= read -r file; do
        [ "$(echo "$file" | raContent)" = "$content" ] || continue
        cp -a "$backup_root/retroarch/$file" "$saves_root/retroarch/$file" || exit 1
      done
      ;;
    *)
      [ -d "$backup_root/$entry" ] || return 0
      cp -a "$backup_root/$entry" "$saves_root/"
      ;;
  esac
}

restoreFromBackup() {
  local entry
  while IFS= read -r entry; do
    isEntry "$entry" || continue
    copyFromBackup "$entry" || return 1
  done < "$1"
  sync
}

# game folders are pruned so the walk stays shallow on big libraries
hakchiRoots() {
  find /media -maxdepth 4 -type d \( -name 'games*' -o -name saves -o -name '.*' \) -prune     -o -type f -name '[Hh][Aa][Kk][Cc][Hh][Ii].[Ee][Xx][Ee]' -print 2>/dev/null | sed 's#/[^/]*$##' | sort -u
}

# entry<TAB>name for every game folder inside the folders read from stdin.
# RetroArch content names come from the files in each game folder (exact, spaces included)
# and from Exec= paths (ROMs kept elsewhere).
gameNames() {
  local dir
  set --
  while IFS= read -r dir; do
    [ -d "$dir" ] && set -- "$@" "$dir"
  done
  [ $# -gt 0 ] || return 0
  {
    find "$@" -mindepth 2 -maxdepth 2 -type f -name '*.desktop' -exec awk '
      function flush(  n, i, token) {
        if(code ~ /^CLV-/ && name != "") {
          print "N\t" code "\t" name
          n = split(exec, token, " ")
          for(i = 1; i <= n; i++)
            if(token[i] ~ /\//) { sub(/.*\//, "", token[i]); print "R\t" code "\t" token[i] }
        }
        code = name = exec = ""
      }
      FNR == 1 { flush() }
      { sub(/\r$/, ""); gsub(/\t/, " ") }
      /^Code=/ { code = substr($0, 6) }
      /^Name=/ && name == "" { name = substr($0, 6) }
      /^Exec=/ { exec = substr($0, 6) }
      END { flush() }' {} + 2>/dev/null
    find "$@" -mindepth 2 -maxdepth 2 -type f ! -name '*.desktop' ! -name '*.png' 2>/dev/null \
      | awk -F / '{ print "R\t" $(NF-1) "\t" $NF }'
  } | awk -F '\t' '
    NF != 3 || $3 == "" { next }
    $1 == "N" { if(!($2 in name)) { name[$2] = $3; print $2 "\t" $3 } next }
    { rom[++n] = $2 "\t" $3 }
    END {
      for(i = 1; i <= n; i++) {
        split(rom[i], r, "\t")
        if(!(r[1] in name)) continue
        # game.sfc.gz is saved as game.srm
        print "retroarch/" r[2] "\t" name[r[1]]
        if(sub(/\.[^.]*$/, "", r[2])) print "retroarch/" r[2] "\t" name[r[1]]
        if(sub(/\.[^.]*$/, "", r[2])) print "retroarch/" r[2] "\t" name[r[1]]
      }
    }' | awk -F '\t' '!seen[$1]++'
}

# games, games_snes, games_md, games_originals, games_cache of each Hakchi folder listed in $1
hakchiNames() {
  local root
  while IFS= read -r root; do
    printf '%s\n' "$root"/games*
  done < "$1" | gameNames
}

# menu folders, linked storage and the stock games
installedNames() {
  local games_root
  games_root="$(dirname "$(findGameStorage)")"
  printf '%s\n' "$games_root"/*/* "$games_root"/*/.storage "$squashfs/usr/share/games" | gameNames
}

# New names win and names of games that are gone are kept, so a save stays named after its
# game is removed. The cache is only rewritten when something changed.
mergeNames() {
  local tmp="$names_cache.tmp"
  mkdir -pm 777 "$(dirname "$names_cache")" || return 1
  { cat "$1"; cat "$names_cache" 2>/dev/null; } | awk -F '\t' 'NF == 2 && $2 != "" && !seen[$1]++' | sort > "$tmp" || { rm -f "$tmp"; return 1; }
  if cmp -s "$tmp" "$names_cache"; then
    rm -f "$tmp"
    return 0
  fi
  mv -f "$tmp" "$names_cache" || return 1
  sync
}

# runs a lookup ($@) and merges what it found; for Hakchi folders also prints how many games had a name
addNames() {
  local found status
  found="$(mktemp)"
  "$@" > "$found"
  mergeNames "$found"
  status=$?
  [ $status -eq 0 ] && [ "$1" = hakchiNames ] && { grep -c "^CLV-" "$found" || true; }
  rm -f "$found"
  return $status
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
  backup)
    saveEntries "$backup_root" | sort -u
    ;;
  restore-backup)
    [ -f "$2" ] && restoreFromBackup "$2"
    ;;
  hakchi)
    hakchiRoots
    ;;
  names)
    [ -s "$2" ] && addNames hakchiNames "$2"
    ;;
  merge-names)
    [ -f "$2" ] && mergeNames "$2"
    ;;
  sync-names)
    addNames installedNames
    ;;
  cached-names)
    cat "$names_cache" 2>/dev/null
    ;;
  *)
    echo "usage: $0 stats|orphans|deleted|move FILE|restore FILE|backup|restore-backup FILE|hakchi|names FILE|merge-names FILE|sync-names|cached-names" >&2
    exit 2
    ;;
esac
