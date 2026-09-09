# Options Menu Documentation

## What is the options menu?

The Options Menu is a custom menu that can be launched via a controller button combo at any point during the console’s operation. It features a variety of commands to enhance the user experience of the console.

The options menu is easily extendible, allowing for other developers to add their own custom commands to the menu via hmods.

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

#### Retroarch Options

This option will open a menu for managing retroarch configurations (see below).

#### Advanced Options

This option will open a menu containing more advanced features (see below).

>\***Note:** You should **NOT** turn off your console via the options/power menu while running retroarch. Ensure you properly exit retroarch before shutting down/restarting or you may experience a loss of saves.

## Retroarch Options

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

## Advanced Options

#### Module Uninstaller

This option launches the module uninstaller program. This program allows you to remove modules from your device. Press the up and down buttons to select a module. Press A to add a module to the uninstall list. Press B to remove a module from the uninstall list. Pressing start will exit if no modules are added to the uninstall list. If there are modules on the uninstall this you will be prompted to ensure you want to remove them press START+SELECT to remove selected modules or B to cancel.

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

#### Dump File Structure (to USB)

This option will map and echo out your complete file and directory structure for your console and any mounted disks. The log file will be saved to your mounted USB/SD card at:

    /media/data/log/Hakchi_file_structure.log

 > **Note:** If these folders do not exist, they will be created when the option is ran.

The log file will also include information on:

- Symbolic Links (Overmounts)
- File and directory permissions

#### Toggle Write Access on USB

This option will toggle read write access on your USB device. By default, if you don't have a saves folder located on your USB/SD, write access is disabled. If you wish to write logs or use your external mounted storage and **not** use external saves... This is the toggle for you.

 > **Note:** If you already use external saves. You don't need to use this toggle.

#### Modern UI

Toggles between the redesigned "settings dialog" look (dialog frame, selection highlight bar, button-hint badges, real toggle switches) and the original plain menu style. Switching restarts the Options Menu so the change applies immediately.

#### RetroArch Debugger (USB logs)

This option will run your locally installed RetroArch in full verbose mode. It will also copy the config files for your RetroArch and save them to:

    /media/data/log/RetroArchConfig

The full verbose log will be located here:

    /media/data/log/Hakchi_retroarch_debug.log

> **Note:** Depending on what you are trying to do, you might have limited functionality within RetroArch when running it via the debugger. This app is primarily designed to debug cores, custom configs and themes.

#### Change Options Button Combo

This option will allow you to change the button combo used to launch the options menu. You will need to restart the console after setting a new combo.

#### Clear Cache

This option will force the kernel to clear the page cache and free up unused memory. You should not need to call this as the system should manage memory fine on it's own.

#### Language Options

This option opens a submenu where you can choose the display language for the Options Menu. Currently available: English, Portuguese, French, German, Spanish, Italian and Dutch. The Options Menu automatically restarts in the selected language.

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

While browsing the list of saved backups, press **X** to delete the
selected backup (you will be asked to confirm with **A**, or cancel with
**B**). The list refreshes automatically after deleting.

#### Restore Wifi Config (USB)

Overwrites your wifi config with a backup located at:

    /media/data/wifi_backup/

and reconnects to the network.

While browsing the list of saved backups, press **X** to delete the
selected backup (you will be asked to confirm with **A**, or cancel with
**B**). The list refreshes automatically after deleting.

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
|IGNORE_INTERRUPT|If set to `TRUE`, pressing B will not interrupt an internal command while its output is being displayed.|
|PREVIEW_IMAGE|Specifies the path to a thumbnail/icon to the oprion menu.|
|PREVIEW_IMAGE_X|Position the thumbnail/icon on the X axys|
|PREVIEW_IMAGE_Y|Position the thumbnail/icon on the Y axys.|
|PREVIEW_IMAGE_WIDTH|Sets the width for the thumbnail/icon.|
|PREVIEW_IMAGE_HEIGHT|Sets the height for the thumbnail/icon.|
|DELETE_STR|Optional command string to run when the user presses X on this entry and confirms. Used to make list entries deletable (e.g. the wifi backup restore list).|
|DELETE_CONFIRM_KEY|Optional translation key for the confirmation prompt shown before running DELETE_STR. Falls back to a generic "Delete this item?" if not set.|
|STATE_STR|Optional command string that prints the current on/off state of this entry (its first line of output, case-insensitively `1`/`on`/`y`/`yes`/`true` for on, anything else for off). Presence of this field renders the entry as a toggle switch instead of a plain row; it is only read to draw the switch, not to change behavior, so COMMAND_STR is still what actually flips the setting.|

>Note: Fields and values are case sensitive. Values should be separated from fields using '=' without spaces.

#### Command String Variables

The command string supports the use of the following variables:

|Variable|Description|
|---------|------------|
|%options_path%|Path to the folder containing the options binary.|
|%script_dir%|Path to the current script folder set in the options binary.|

### Compiling Options Menu

#### Docker toolchain (recommended)

The repo includes a Docker-based ARM cross-compile toolchain, which is the tested and supported way to build a release-ready `.hmod`. It needs only Docker installed - no local SDL2/libpng/ARM toolchain setup required.

    ./build.sh      # Linux/macOS/Git Bash
    ./build.ps1     # Windows PowerShell

This builds the toolchain image (`Dockerfile.jessie-armhf`) if needed, then runs `Makefile.docker` inside it, producing `out/*.hmod`.

#### Native / manual build

The options menu can also be compiled using the plain `Makefile` provided. To compile the options menu you must have the SDL2 and libpng libraries as well as GNU Make installed. To cross-compile the options menu call `make` with the `CROSS_PREFIX` set to the prefix of your cross-compiler toolchain eg. `make CROSS_PREFIX=arm-linux-gnueabihf-`.

## Contributions and Thanks

### Contributions

- Hibernate Mod and Hakchi-Option-Pack scripts courtesy of Swingflip  
- Retroarch Configuration scripts courtesy of BsLeNuL
- Network commands courtesy by Advokaten and DefKorns
- Wifi Backup courtesy of DefKorns
- Preview image aspect ratio courtesy of DefKorns
- Wifi backup delete feature (press X on a saved backup) courtesy of DefKorns
- Multi-language localization system (i18n) and translations courtesy of DefKorns
- Docker-based ARM cross-compile toolchain and build tooling courtesy of DefKorns
- Redesigned "Modern UI" (dialog frame, selection highlight, toggle switches, button-hint badges) courtesy of DefKorns
- Thanks to ThanosRD for assistance with UI Layout/Design

### Testing

Extra thanks to DNA64 (viral_dna) and Swingflip for always testing features.

Also thanks to the following people for testing the options menu:

- BsLeNuL  
- DefKorns  
- DR1001  
- Patton Plays  
- ThanosRD
