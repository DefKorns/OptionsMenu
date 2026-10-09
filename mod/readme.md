
## What is the options menu?

The Options Menu is a custom, easily extendable menu for your console, letting other developers add their own commands to it via hmods.

### Features

- **Modern UI** with smooth TTF fonts and preview images
- **Power:** Hibernate/Standby, Shutdown, Restart
- **RetroArch:** back up/restore settings (NAND or USB), restore defaults, transfer BIOS files, clean up overrides/remaps/BIOS, toggle load screens
- **Network:** show IP, reconnect, search for SSIDs, back up/restore your wifi config (NAND or USB)
- **Save Manager:** back up/restore saves to USB, and move aside (or bring back) saves of games no longer on the console
- **Language:** English, Français, Deutsch, Español, Italiano, Nederlands, Português, Русский, 日本語
- **Advanced:**
  - Controller: change the menu button combo, autofire (speed, X/Y as turbo A/B), Home menu combo, Start on the 2nd controller
  - Diagnostics: system info, temperature, top, benchmark, RetroArch debugger, file structure and kernel log dumps to USB
  - Module Uninstaller, Clear Cache, USB write access toggle, epilepsy protection (NES)
- Extendable: other hmods can add their own entries and menus

### How to open

Hold **L + R** for about 1 second, at any point during the console's operation. On consoles with no shoulder buttons (NES Classic, Famicom Classic and its variants, e.g. the Famicom Shonen Jump 50th Anniversary edition), hold **B + Down** instead.

## Documentation

Every menu, step by step, and how to add your own entries from an hmod: [github.com/DefKorns/OptionsMenu/wiki](https://github.com/DefKorns/OptionsMenu/wiki)

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
- **ThanosRD** — UI Layout/Design assistance (original Classic UI)

### Testing

Extra thanks to **DNA64 (viral_dna)** and **Swingflip** for always testing features.

Also thanks to the following people for testing the options menu:

- **Aranthys**
- **BsLeNuL**
- **DefKorns**
- **DR1001**
- **Patton Plays**
- **ThanosRD**
