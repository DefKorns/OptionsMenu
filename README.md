# Options Menu Documentation

[![Build](https://github.com/DefKorns/OptionsMenu/actions/workflows/build.yml/badge.svg)](https://github.com/DefKorns/OptionsMenu/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

## What is the options menu?

The Options Menu is a custom menu that can be launched via a controller button combo at any point during the console’s operation. It features a variety of commands to enhance the user experience of the console.

The options menu is easily extendible, allowing for other developers to add their own custom commands to the menu via hmods.

## Navigation

|Button|Action|
|------|------|
|UP / DOWN|Move the selection (hold to repeat).|
|A or START|Run the selected option or open its submenu.|
|B|Go back to the previous menu, or close the Options Menu on the main screen.|
|B (hold ~1 second)|Delete the selected entry, on lists that support it (the footer shows a **Delete (Hold)** hint). You are asked to confirm with **A**.|

Rows that open a submenu show a **›** on the right. The button hints for the current screen are always shown in the footer.

## Options Explained

Below is a detailed guide on what specific options actually do and what their purpose is.

## Default Options

#### Hibernate/Standby

This option launches Swingflip’s power menu. The power menu will provide the following options:

- **Hibernate** - Put the console in a low power state, turn the screen off and when woken up the console will resume whatever you were doing before you put the console in hibernate.
- **Standby** - Put the console in lowest power state possible, turn the screen off and when woken up the console will reboot and start the console from a fresh boot.

> **Note:** Basically... Standby saves more energy however when you remotely wake the console it will start up as a fresh boot where hibernate will resume where ever you left off when you put it in to hibernate.

To wake the console when in hibernate or standby please refer to the below table:

|Button combination|Hibernate Mode |Standby Mode     |
|------------------|---------------|-----------------|
| L + R + UP       |Resume Console |Reboot Console   |
| L + R + DOWN     |Reboot Console |Shutdown Console |

#### Shutdown Device

This option will safely shut down the console.\*

#### Restart Device

This option will safely restart the console.\*

#### RetroArch

This option will open a menu for managing retroarch configurations (see below).

#### Network

This option will open a menu for managing your network and wifi configuration (see below).

#### Saved Games

This option will open a menu for backing up and restoring your saved games (see below).

#### Language

This option opens a submenu where you can choose the display language for the Options Menu. Currently available: English, Français, Deutsch, Español, Italiano, Nederlands, Português (Europeu), Русский and 日本語. The Options Menu automatically restarts in the selected language.

#### Theme Options

Only shown when the [Theme Selector](https://github.com/DefKorns/om_theme-selector) hmod is installed. Opens its theme manager.

#### Advanced Options

This option will open a menu containing more advanced features (see below).

>\***Note:** You should **NOT** turn off your console via the options/power menu while running retroarch. Ensure you properly exit retroarch before shutting down/restarting or you may experience a loss of saves.

## Retroarch Options

Both RetroArch builds are supported: RetroArch Neo (discontinued) and the `_km` RetroArch Xtreme (Ozone).

#### Restore Default Settings (All)

Replaces your current retroarch config files with the default files included in the module. This option deletes all game/core overrides and remap files.

#### Restore Default Settings

Replaces your current retroarch config files with the default files included in the module. Does not erase your game/core overrides and remap files.

#### Backup Settings to NAND

Copies your current retroarch config files to a backup file located at /etc/ra_backup/

#### Backup Settings to USB (usb only)

Copies your current retroarch config files to a backup file located at USB:/data/ra_backup/

#### Restore Settings from NAND

Overwrites your retroarch configs with the backups located at /etc/ra_backup/

#### Restore Settings from USB (usb only)

Overwrites your retroarch configs with the backups located at USB:/data/ra_backup/

#### Delete all settings backups

Removes all backup files on both NAND and USB.

#### Transfer BIOS file(s)

Copies bios files to USB:/data/ra_bios. Restores bios files from this folder to NAND.

#### Delete game and core overrides

Removes only the game and core overrides. Does not modify the Retroarch settings and remap files.

#### Delete remap files

Removes only the remap files. Does not modify the Retroarch settings, game and core overrides.

#### Delete BIOS file(s) from NAND

Removes all bios files from NAND only.

#### Toggle RA and Canoe load screens

Toggle the RetroArch and Canoe loading screens on or off.

## Network Options

#### Display IP Address

This option will display both your public and internal ip addresses if you have a network adapter connected to your device.

#### Reconnect

This option will restart the network service on your device if you are not connected to the internet already.

#### Search for SSIDs

This option will scan and display a list of available SSIDs.

#### Backup Wifi Config (NAND)

Copies your current wifi config (wpa_supplicant.conf) to a backup folder on your NAND, named after your SSID:

    /etc/wifi_backup/your_ssid_name

#### Backup Wifi Config (USB)

Copies your current wifi config (wpa_supplicant.conf) to a backup folder on your mounted USB/SD card, named after your SSID:

    /media/data/wifi_backup/your_ssid_name

#### Restore Wifi Config (NAND)

Overwrites your wifi config with a backup located at:

    /etc/wifi_backup/

and reconnects to the network.

While browsing the list of saved backups, hold **B** (~1 second) to delete
the selected backup (you will be asked to confirm with **A**, or cancel by
pressing **B** again). The list refreshes automatically after deleting.

#### Restore Wifi Config (USB)

Overwrites your wifi config with a backup located at:

    /media/data/wifi_backup/

and reconnects to the network.

While browsing the list of saved backups, hold **B** (~1 second) to delete
the selected backup (you will be asked to confirm with **A**, or cancel by
pressing **B** again). The list refreshes automatically after deleting.

## Saved Games Options

#### Backup Saved Games to USB

Copies your saved games from the saves folder on your USB/SD card to a backup folder on the same drive:

    /media/data/saves_backup

The backup mirrors the saves folder, so saves deleted since the last backup are removed from it too. System folders (`FOLDER`, `hakchi`, `home-menu`, `mcp-state`) and system files are not included.

#### Restore Saved Games from USB

Copies the saved games from `/media/data/saves_backup` back into your saves folder, overwriting saves with the same name. Saves that are not in the backup are left untouched.

## Advanced Options

#### Change Options Button Combo

This option will allow you to change the button combo used to launch the options menu. You will need to restart the console after setting a new combo.

> **Note:** The default combo is **L + R**, or **B + Down** on consoles with no shoulder buttons (NES Classic, Famicom Classic and its variants, e.g. the Famicom Shonen Jump 50th Anniversary edition), detected automatically. Setting a custom combo here overrides that detection.

#### Toggle Write Access on USB

This option will toggle read write access on your USB device. By default, if you don't have a saves folder located on your USB/SD, write access is disabled. If you wish to write logs or use your external mounted storage and **not** use external saves... This is the toggle for you.

 > **Note:** If you already use external saves. You don't need to use this toggle.

#### Clear Cache

This option will force the kernel to clear the page cache and free up unused memory. You should not need to call this as the system should manage memory fine on it's own.

#### Module Uninstaller

This option launches the module uninstaller program. This program allows you to remove modules from your device. The installed modules are listed on the left and the ones queued for removal on the right.

- **UP / DOWN** - select a module.
- **A** - add the selected module to the uninstall list.
- **B** - remove the last module from the uninstall list.
- **START** - exit if the uninstall list is empty; otherwise you are asked to confirm. Press **START + SELECT** to remove the queued modules, or **B** to cancel.

>**Caution:** The uninstaller will list all modules on the device including the default hakchi modules. Only delete modules you have added yourself. Deleting the default hakchi modules can have unintended effects on your console.

#### Display Temp

This option will display the current temperature of the CPU.

#### Run Top

This option runs and displays the output of the top binary. This shows the current CPU, memory usage and list of running processes.

#### Benchmark Tool

This option will go through and echo out useful information regarding your console.
It will also include information on your:

- Current disk mounting information
- CPU analysis information
- RAM analysis information

> **Note:** The information is displayed in 4 parts and has a 2 second wait between each part

#### RetroArch Debugger (USB logs)

This option will run your locally installed RetroArch in full verbose mode. It will also copy the config files for your RetroArch and save them to:

    /media/data/log/RetroArchConfig

The full verbose log will be located here:

    /media/data/log/Hakchi_retroarch_debug.log

> **Note:** Depending on what you are trying to do, you might have limited functionality within RetroArch when running it via the debugger. This app is primarily designed to debug cores, custom configs and themes.

#### Dump File Structure (to USB)

This option will map and echo out your complete file and directory structure for your console and any mounted disks. The log file will be saved to your mounted USB/SD card at:

    /media/data/log/Hakchi_file_structure.log

 > **Note:** If these folders do not exist, they will be created when the option is ran.

The log file will also include information on:

- Symbolic Links (Overmounts)
- File and directory permissions

## Developer Information

### General Information

The options mod is comprised of two executables. The `optiond` executable acts as a watchdog that waits for the controller combo to be held down and executes the `options` executable. All files are stored with the `/etc/options_menu/` folder. There are two important folders for the usage of the options menu these are the commands and scripts folders. The commands folder stores the options to be displayed on the menu. The scripts folder contains scripts executed by these commands.

### Command Line Arguments
>
>Note: The options menu should always be called with the full file path.
>/etc/options_menu/options

|Argument|Description|
|----------|-------------|
|--commandPath &lt;Command Folder Path&gt;|Set a custom command folder.|
|--scriptPath &lt;Script Folder Path&gt;|Set custom script folder.|
|--title &lt;Window Title&gt;|Set custom title for options menu screen.|

### Command Files

Command Files must be named in the following format:

    cxxxx_CommandName

Command Files should contain the following fields:

|Field|Description|
|-------|-------------|
|COMMAND_NAME|Display Name of command.|
|COMMAND_TYPE|Specifies whether command is run inside the options menu pseudo terminal (internally) or run externally. Possible values `INTERNAL` or `EXTERNAL`|
|RESTART_UI|Specifies if the paused UI should be resumed after running command (external commands only). If you set this value to `FALSE` you must manually resume the UI using `/bin/sh /etc/options_menu/script/ResumeUI.sh` after executing your code.|
|COMMAND_STR|Command string to be executed. Commands must be single line only. To execute multi-line scripts use a script file.|
|USB_ONLY|If set to `TRUE`, the command is only loaded when a USB/SD card is mounted.|
|ENABLE_IF|Optional shell condition; the row is only loaded if it exits 0. Runs with `/etc/preinit` sourced and `script_init` called first, so it can reference `$rootfs`/`$mountpoint` like any other options_menu script, e.g. `ENABLE_IF=[ -d "$rootfs/etc/wifi_backup" ]`.|
|IGNORE_INTERRUPT|If set to `TRUE`, pressing B will not interrupt an internal command while its output is being displayed.|
|CHILD|If set to `TRUE`, indents the row under the one above it, for visually grouping a related entry without a separate submenu.|
|SUBMENU|If set to `TRUE`, marks the row as opening another menu and draws a **›** on its right. Ignored on toggle rows and on rows that relaunch the current menu.|
|PREVIEW_IMAGE|Specifies the path to a thumbnail/icon shown in the detail panel when this row is selected. Auto-scaled and centered to fit the panel.|
|PREVIEW_IMAGE_X|Unused - the detail panel always auto-centers the image.|
|PREVIEW_IMAGE_Y|Unused - the detail panel always auto-centers the image.|
|PREVIEW_IMAGE_WIDTH|Unused - the detail panel always auto-scales the image to fit.|
|PREVIEW_IMAGE_HEIGHT|Unused - the detail panel always auto-scales the image to fit.|
|PREVIEW_NEAREST|Grid layout only\*. If set to `TRUE`, the preview is scaled with nearest-neighbor instead of linear filtering, for small pixel-art images.|
|PREVIEW_SQUARE|Grid layout only\*. If set to `TRUE` on the first grid item, the screen uses square tiles instead of the default wide ones.|
|PREVIEW_GRID_COLS|Grid layout only\*. Set on the first grid item to force the number of tile columns.|
|PREVIEW_FIT_CONTAIN|Grid layout only\*. If set to `TRUE`, this row's tile scales the art to fit inside the tile instead of cropping it to fill it. Use for portrait art in a wide/square tile.|
|PREVIEW_HIDE_LABEL|Grid layout only\*. If set to `TRUE`, this row's tile shows no caption below it.|
|DELETE_STR|Optional command string to run when the user holds B (~1 second) on this entry and confirms. Used to make list entries deletable (e.g. the wifi backup restore list).|
|DELETE_CONFIRM_KEY|Optional translation key for the confirmation prompt shown before running DELETE_STR. Falls back to a generic "Delete this item?" if not set.|
|STATE_STR|Optional command string that prints the current on/off state of this entry (its first line of output, case-insensitively `1`/`on`/`y`/`yes`/`true` for on, anything else for off). Presence of this field renders the entry as a toggle switch instead of a plain row; it is only read to draw the switch, not to change behavior, so COMMAND_STR is still what actually flips the setting.|

>Note: Fields and values are case sensitive. Values should be separated from fields using '=' without spaces.

\*The grid layout fields are parsed by the shared framework but only used by the Theme Selector's `theme_manager` grid screens; the Options Menu itself always shows a list.

#### Command String Variables

COMMAND_STR, DELETE_STR and STATE_STR all support the use of the following variables:

|Variable|Description|
|---------|------------|
|%options_path%|Path to the folder containing the options binary.|
|%script_dir%|Path to the current script folder set in the options binary.|

### Theme Colors

The UI colors are read at startup from `/etc/options_menu/theme.cfg`, one `Key=R,G,B` per line (0-255 each, `#` for comments). A missing file, line or key falls back to the built-in default.

|Key|Used for|
|---|--------|
|Bg|App background.|
|Border|Divider lines and box outlines.|
|Accent|Selected row border, section title bar and submenu chevrons.|
|SelectedRowBg|Selected row fill. Follows `Bg` unless set.|
|Text|Titles, row labels and button hints.|
|TextDim|Secondary text, e.g. the Module Uninstaller column headers.|
|ScrollArrow|List scroll arrows. Follows `Text` unless set.|
|BadgeLetter|Letter inside a button-hint badge.|
|BadgeA / BadgeADark|A badge fill / rim.|
|BadgeB / BadgeBDark|B badge fill / rim.|
|BadgeX / BadgeXDark|Hold-to-delete badge fill / rim.|
|BadgeY / BadgeYDark|Reserved, currently unused.|
|BadgeStart / BadgeStartDark|Start pill fill / rim.|

### Translations

UI strings live in `/etc/options_menu/language/<code>.lang` as `KEY=Text` lines; the selected language code is stored in `/etc/options_menu/language.cfg`. `en-US` is always loaded first, so a partial translation falls back to English for missing keys.

A plugin hmod can ship its own strings in `/etc/options_menu/<plugin>/lang/<code>.lang`. Every such folder is merged in at startup, as long as it has an `en-US.lang`.

### Preview Icons

The per-command preview images in `/etc/options_menu/images/` (200x140 PNG) and the UI assets in `/etc/options_menu/images/ui/` are rendered from [Bootstrap Icons](https://icons.getbootstrap.com/) glyphs. The assets in `images/ui/` are white and tinted at runtime with the theme colors. A plugin can point `PREVIEW_IMAGE` at any PNG; it is scaled to fit the detail panel.

### Compiling Options Menu

#### Docker toolchain (recommended)

The repo includes a Docker-based ARM cross-compile toolchain, which is the tested and supported way to build a release-ready `.hmod`. It needs only Docker installed - no local SDL2/libpng/ARM toolchain setup required.

    ./build.sh      # Linux/macOS/Git Bash
    ./build.ps1     # Windows PowerShell

This builds the toolchain image (`Dockerfile.jessie-armhf`) if needed, then runs `Makefile.docker` inside it, producing `out/*.hmod`.

Pass `FIX_LIBSTDCXX=1` to also build a `_compat` variant that ships its own `libstdc++.so.6`, for consoles with another hmod that replaces the system one (e.g. RetroArch 1.8.4 Xtreme SC):

    ./build.sh FIX_LIBSTDCXX=1

The version shown in the app and in the `.hmod` name comes from the latest git tag. Pass `MOD_VER` to override it, e.g. for a test build:

    ./build.sh MOD_VER=test-dev

#### Releases

Pushing a `v*` tag makes the GitHub Actions workflow build the `.hmod` with that tag as its version and publish it as a GitHub Release. Tags with a hyphen (`-alpha`, `-beta`, `-rc`) are published as prereleases.

    git tag -a v2.0.0-alpha -m "Options Menu v2.0.0-alpha"
    git push origin v2.0.0-alpha

#### Native / manual build

The options menu can also be compiled using the plain `Makefile` provided. To compile the options menu you must have the SDL2 and libpng libraries as well as GNU Make installed. To cross-compile the options menu call `make` with the `CROSS_PREFIX` set to the prefix of your cross-compiler toolchain eg. `make CROSS_PREFIX=arm-linux-gnueabihf-`.

## Contributions and Thanks

### Contributions

- **Swingflip** — Hibernate Mod and Hakchi-Option-Pack scripts
- **BsLeNuL** — Retroarch Configuration scripts
- **Advokaten** — Network commands (with DefKorns)
- **DefKorns**
  - Network commands (with Advokaten)
  - Wifi Backup, preview image aspect ratio, delete feature (hold B on a saved backup)
  - Multi-language localization system (i18n) and translations
  - Docker-based ARM cross-compile toolchain and build tooling
  - Redesigned "Modern UI" (dialog frame, selection highlight, toggle switches, button-hint badges)
  - TTF text rendering, theme colors (`theme.cfg`), preview icons, Saved Games options
- **ThanosRD** — UI Layout/Design assistance (original Classic UI)
- **[MadFranko008](https://www.reddit.com/user/MadFranko008/)** — Original idea for the Saved Games backup/restore options

### Third-party assets

- [Bootstrap Icons](https://icons.getbootstrap.com/) — source glyphs for the preview icons and UI chevrons (MIT License)
- [Noto Sans JP](https://fonts.google.com/noto/specimen/Noto+Sans+JP) — UI font, subset (SIL Open Font License, see `fonts/OFL.txt`)
- [Misaki font](http://littlelimit.net/misaki.htm) by Num Kadoma — 8x8 Katakana and Kanji glyphs

### Testing

Extra thanks to DNA64 (viral_dna) and Swingflip for always testing features.

Also thanks to the following people for testing the options menu:

- Aranthys  
- BsLeNuL  
- DefKorns  
- DR1001  
- Patton Plays  
- ThanosRD
