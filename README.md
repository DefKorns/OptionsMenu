# Options Menu

[![Build](https://github.com/DefKorns/OptionsMenu/actions/workflows/build.yml/badge.svg)](https://github.com/DefKorns/OptionsMenu/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

A menu you can open at any moment on your NES, SNES, Famicom or Super Famicom Classic — on the Home screen or in the middle of a game — to power off, back up your saves and settings, manage Wi-Fi and RetroArch, tweak the controllers and more. Other hmods plug their own screens into it, like [Theme Selector](https://github.com/DefKorns/om_theme-selector).

This is my continuation of [CompCom's Options Menu](https://github.com/CompCom/OptionsMenu) (last release 1.3.4, 2019). Version 2 is a rewrite of the interface and a large part of the scripts:

- **New look:** smooth TTF fonts, preview icons, toggle switches, submenu markers and button hints, with colors you can change (`theme.cfg`)
- **9 languages:** English, Français, Deutsch, Español, Italiano, Nederlands, Português, Русский, 日本語
- **New menus:** Controller (autofire, Home combo, Start on the 2nd Famicom controller), Diagnostics, Saved Games, Wi-Fi backups, epilepsy protection
- **Safer behaviour:** entries that need a USB/SD drive or a missing component are hidden instead of failing, and lists such as Wi-Fi backups let you delete entries
- **NES/Famicom friendly:** the opening combo is picked automatically for consoles without shoulder buttons
- A **compat** build for setups where another hmod replaces the console's `libstdc++`

## Install

**From my Mod Hub (recommended):** in hakchi open **Manage repositories**, add `https://defkorns.github.io/hakchi-repo/` and install **Options Menu** from it. Updates show up there automatically.

**By hand:** download `options_menu.hmod` from [Releases](https://github.com/DefKorns/OptionsMenu/releases), put it in hakchi's `user_mods` folder (or drag and drop it onto the hakchi window), then install it from hakchi (**Modules → Install extra modules**).

Installing it replaces CompCom's Options Menu (`options_deluxe`) and the old Hibernate mod, and keeps your button combo, language and Home combo settings.

> **Which build?** Use `options_menu`. Pick `options_menu_compat` only if you have an hmod that replaces the console's `libstdc++` (e.g. RetroArch 1.8.4 Xtreme SC) and the menu doesn't open with the normal build. Installing one removes the other.

## Opening it

Hold **L + R** for about a second. On consoles without shoulder buttons (NES Classic, Famicom Classic, Famicom Shonen Jump), hold **B + Down** instead. You can change the combo in **Advanced Options → Controller**.

| Button | Action |
| --- | --- |
| Up / Down | Move (hold to repeat) |
| A or Start | Run the entry or open its submenu (marked **›**) |
| B | Back; closes the menu on the main screen |
| B (hold ~1 second) | Delete the selected entry, on lists that allow it — confirm with A |

The footer always shows the buttons for the current screen.

## Menus

### Main menu

| Entry | What it does |
| --- | --- |
| Hibernate/Standby | Opens Swingflip's power menu (see below) |
| Shutdown Device | Safely turns the console off\* |
| Reboot Device | Safely restarts the console\* |
| RetroArch › | RetroArch settings, backups and BIOS files. Shown when RetroArch is installed |
| Network › | IP address, Wi-Fi scan and Wi-Fi config backups |
| Saved Games › | Back up and restore your saves. Needs a USB/SD drive |
| Language › | Menu language; the menu restarts in the new language |
| Theme Options › | Added by [Theme Selector](https://github.com/DefKorns/om_theme-selector) when it's installed |
| Advanced Options › | Controller, diagnostics and system tools |

\*Don't shut down or restart from the menu while RetroArch is running: exit RetroArch first, or you may lose saves.

**Hibernate** turns the screen off and keeps everything in memory, so waking up resumes exactly where you were. **Standby** uses even less power, but waking up boots the console from scratch. To wake it:

| Combo | From Hibernate | From Standby |
| --- | --- | --- |
| L + R + Up | Resume | Reboot |
| L + R + Down | Reboot | Shut down |

### RetroArch

Works with RetroArch Neo and with the `_km` RetroArch Xtreme (Ozone).

| Entry | What it does |
| --- | --- |
| Restore Default Settings (All) | Puts back the default RetroArch config **and** deletes every game/core override and remap |
| Restore Default Settings | Puts back the default RetroArch config, keeping overrides and remaps |
| Backup Settings to NAND / USB | Copies your RetroArch config to `/etc/ra_backup/` or `USB:/data/ra_backup/` |
| Restore Settings from NAND / USB | Restores the config from those backups |
| Transfer BIOS file(s) | Copies BIOS files to `USB:/data/ra_bios` and restores them from there to NAND |
| Delete all settings backups | Removes the NAND and USB backups |
| Delete game and core overrides | Removes only the overrides |
| Delete remap files | Removes only the remaps |
| Delete BIOS file(s) from NAND | Removes the BIOS files from NAND |
| Toggle RA and Canoe load screens | Turns the RetroArch and Canoe loading screens on or off |

The USB entries only show up with a USB/SD drive connected.

### Network

| Entry | What it does |
| --- | --- |
| Display IP Address | Shows your local and public IP |
| Reconnect | Restarts the network when you're not connected |
| Search for SSIDs | Lists the Wi-Fi networks in range |
| Backup Wifi Config (NAND / USB) | Saves your `wpa_supplicant.conf` to `/etc/wifi_backup/<ssid>` or `/media/data/wifi_backup/<ssid>` |
| Restore Wifi Config (NAND / USB) › | Lists your backups; pick one to restore it and reconnect, or hold **B** to delete it |

Restore entries only show up when there's a backup to restore.

### Saved Games

Needs a USB/SD drive with your saves on it.

| Entry | What it does |
| --- | --- |
| Backup Saved Games to USB | Mirrors your saves folder to `/media/data/saves_backup`. Saves you deleted since the last backup are removed from it too; system folders (`FOLDER`, `hakchi`, `home-menu`, `mcp-state`) are skipped |
| Restore Saved Games from USB | Copies the backup back, overwriting saves with the same name and leaving the others alone |

### Advanced Options

| Entry | What it does |
| --- | --- |
| Controller › | See below |
| Diagnostics › | See below |
| Toggle Write Access on USB | Makes the USB/SD drive writable even if you don't keep your saves on it (it's read-only otherwise). Not needed if you use USB saves |
| Epilepsy protection (NES) | The console's own dimming of fast flashes in NES/Famicom games. On by default; turn it off to see games exactly as they were. NES/Famicom only |
| Clear Cache | Asks the kernel to free cached memory. Rarely needed |
| Module Uninstaller | Remove installed hmods from the console (see below) |

**Module Uninstaller:** installed mods are on the left, the ones queued for removal on the right. **A** queues the selected mod, **B** takes the last one off the queue, **Start** exits — or, with mods queued, asks you to press **Start + Select** to remove them (**B** cancels). It lists hakchi's own modules too: only remove mods you installed yourself.

#### Controller

| Entry | What it does |
| --- | --- |
| Change Options Button Combo | Press the buttons you want to open the menu with. Restart the console afterwards. Overrides the automatic L+R / B+Down choice |
| Autofire | Turbo for the controllers, with **Fast / Normal / Slow** speed |
| Use X/Y as turbo A/B | X and Y act as turbo A and B |
| Home menu button combo | Turns off the combo that sends you from a game back to the Home menu, so you can't press it by accident |
| Start on 2nd controller | Famicom only: the 2nd Famicom controller has no Start button; this gives it one |

Changes apply right away, without restarting.

#### Diagnostics

| Entry | What it does |
| --- | --- |
| System Information | Console type and region, hakchi/kernel/boot versions, firmware, free space on NAND and USB/SD, size of your saves |
| Display Temp | CPU temperature |
| Run Top | CPU and memory use and the running processes |
| Benchmark Tool | Mounts, CPU and RAM information, in 4 parts |
| RetroArch Debugger (USB logs) | Runs RetroArch in verbose mode and saves its config and log to `/media/data/log/` |
| Dump File Structure (to USB) | Writes every file and folder, with permissions and links, to `/media/data/log/Hakchi_file_structure.log` |
| Save kernel log (to USB) | Writes `dmesg` to `/media/data/log/dmesg.log` |

The USB entries only show up with a USB/SD drive connected. They're the logs to attach when reporting a problem.

## For mod developers

Your hmod can add entries and whole screens to the menu.

### How it works

`optiond` waits for the button combo and launches `options`. Everything lives in `/etc/options_menu/`: each screen is a `commands/` folder with one file per entry, and the scripts they run go in a `scripts/` folder.

`options` takes these arguments (always call it by its full path, `/etc/options_menu/options`):

| Argument | Description |
| --- | --- |
| `--commandPath <folder>` | Commands folder for this screen |
| `--scriptPath <folder>` | Scripts folder, available as `%script_dir%` |
| `--title <key>` | Screen title (a translation key or plain text) |

### Command files

One file per entry, named `cNNNN_Name`; entries are sorted by name. Each line is `FIELD=value`, no spaces around `=`, case sensitive.

| Field | Description |
| --- | --- |
| `COMMAND_NAME` | Label shown, or a translation key |
| `COMMAND_TYPE` | `INTERNAL` shows the command's output in the menu's console; `EXTERNAL` runs it with the menu paused |
| `COMMAND_STR` | Single-line command to run. Use a script for anything longer |
| `RESTART_UI` | External commands only. `FALSE` leaves the UI paused; resume it yourself with `/bin/sh /etc/options_menu/scripts/ResumeUI.sh` |
| `IGNORE_INTERRUPT` | `TRUE`: B doesn't interrupt an internal command while it runs |
| `USB_ONLY` | `TRUE`: only shown when a USB/SD drive is mounted |
| `ENABLE_IF` | Shell condition; the entry is only shown if it exits 0. `/etc/preinit` is sourced and `script_init` called first, so `$rootfs`, `$mountpoint`, `$sftype`... work, e.g. `ENABLE_IF=[ -d "$rootfs/etc/wifi_backup" ]` |
| `SUBMENU` | `TRUE`: draws **›**, for entries that open another screen. Ignored on toggles |
| `CHILD` | `TRUE`: indents the entry under the one above it |
| `STATE_STR` | Command whose first output line is the entry's state (`1`/`on`/`y`/`yes`/`true` = on). Turns the entry into a toggle switch; `COMMAND_STR` still does the toggling |
| `DELETE_STR` | Command run when the user holds B on the entry and confirms — makes list entries deletable |
| `DELETE_CONFIRM_KEY` | Translation key for that confirmation; defaults to a generic "Delete this item?" |
| `PREVIEW_IMAGE` | PNG shown in the detail panel, scaled to fit |

`COMMAND_STR`, `DELETE_STR` and `STATE_STR` can use `%options_path%` (the folder with the `options` binary) and `%script_dir%` (the current `--scriptPath`).

The shared framework also reads a few grid-only fields (`PREVIEW_NEAREST`, `PREVIEW_SQUARE`, `PREVIEW_GRID_COLS`, `PREVIEW_FIT_CONTAIN`, `PREVIEW_HIDE_LABEL`). Only Theme Selector's grid screens use them. `PREVIEW_IMAGE_X/Y/WIDTH/HEIGHT` from the original format are accepted and ignored.

### Translations

Strings live in `/etc/options_menu/language/<code>.lang` as `KEY=Text`; the chosen language is in `/etc/options_menu/language.cfg`. `en-US` is always loaded first, so missing keys fall back to English.

Your hmod can ship its own strings in `/etc/options_menu/<your_mod>/lang/<code>.lang`. Every such folder with an `en-US.lang` is merged in at startup.

### Colors

`/etc/options_menu/theme.cfg` holds the UI colors, one `Key=R,G,B` per line (0–255, `#` for comments). Missing keys use the defaults.

| Key | Used for |
| --- | --- |
| `Bg` | Background |
| `Border` | Dividers and outlines |
| `Accent` | Selected entry border, section title bar, **›** |
| `SelectedRowBg` | Selected entry fill (follows `Bg` unless set) |
| `Text` / `TextDim` | Main and secondary text |
| `ScrollArrow` | Scroll arrows (follows `Text` unless set) |
| `BadgeLetter` | Letter inside button-hint badges |
| `BadgeA`, `BadgeB`, `BadgeX`, `BadgeStart` (+ `…Dark` rims) | Button-hint badges; X is the hold-to-delete hint |
| `BadgeY` (+ `BadgeYDark`) | Reserved |

### Icons

Preview images (`/etc/options_menu/images/`, 200×140 PNG) and the UI glyphs in `images/ui/` are rendered from [Bootstrap Icons](https://icons.getbootstrap.com/). The `images/ui/` glyphs are white and tinted with the theme colors at runtime.

## Building

```sh
git clone --recursive https://github.com/DefKorns/OptionsMenu.git
cd OptionsMenu
./build.sh                    # out/options_menu.hmod   (./build.ps1 on PowerShell)
./build.sh FIX_LIBSTDCXX=1    # out/options_menu_compat.hmod
./build.sh MOD_VER=test-dev   # override the version for a test build
```

`build.sh` builds the [classicmini-cross-toolchain](https://github.com/DefKorns/classicmini-cross-toolchain) Docker image if needed and runs `make` inside it — no local ARM toolchain required. Outside Docker, the same `Makefile` works with SDL2, SDL2_ttf, libpng and `make CROSS_PREFIX=arm-linux-gnueabihf-`.

Packaging and versioning come from [hmod-build](https://github.com/DefKorns/hmod-build): the version is the latest `v*` git tag, and the `.hmod` name has no version, so hakchi sees every release as the same mod. Pushing a tag builds both variants and publishes a GitHub Release (a prerelease for tags with a hyphen, e.g. `v2.0.0-alpha5`).

## Credits

- **[CompCom](https://github.com/CompCom)** — the original Options Menu
- **[DefKorns](https://github.com/DefKorns)** — version 2: the new interface, TTF text, theme colors and icons, translations, Network (with Advokaten), Wi-Fi backups, Saved Games, Controller and Diagnostics menus, build tooling
- **Swingflip** — Hibernate Mod and Hakchi-Option-Pack scripts
- **[BsLeNuL](https://github.com/bslenul)** — RetroArch configuration scripts
- **Advokaten** — Network commands
- **ThanosRD** — layout and design of the original interface
- **[MadFranko008](https://www.reddit.com/user/MadFranko008/)** — idea for the Saved Games backup

### Third-party assets

- [Bootstrap Icons](https://icons.getbootstrap.com/) — preview icons and UI glyphs (MIT License)
- [Noto Sans JP](https://fonts.google.com/noto/specimen/Noto+Sans+JP) — UI font, subset (SIL Open Font License, see `fonts/OFL.txt`)
- [Misaki font](http://littlelimit.net/misaki.htm) by Num Kadoma — 8×8 kana and kanji glyphs

### Testing

Thanks to DNA64 (viral_dna) and Swingflip for always testing, and to Aranthys, BsLeNuL, DR1001, Patton Plays and ThanosRD.

## License

GPLv3 or later — see [LICENSE](LICENSE).
