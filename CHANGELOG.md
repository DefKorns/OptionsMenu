# Changelog

Releases before v2.0.0 belong to CompCom's original Options Menu and are not listed here.

## v2.1.2-beta

- **Boot crash fix (testing):** the startup tasks now wait for the console's own startup to finish before syncing game names, which should stop the occasional crash right after boot
- **Menu language:** *Automatic* resolves from the console language, including the one set by Console Language

## v2.1.1

- **Open menu during games:** new toggle in Controller. Turn it off and the button combo only opens the menu from the Home screen. On by default, so nothing changes until you turn it off

## v2.1.0

- **Menu language follows the console:** Language → Options Menu language now defaults to *Automatic*, which uses the console's own language
- **Clearer confirmations:** non-destructive actions confirm with **A**; deleting or overwriting still needs **Start+Select**. Save Manager and Module Uninstaller dialogs share the same look
- **Save Manager:** saves of games with the same name are tagged with their game code, and empty saves are hidden
- **Downloads:** add-ons can show a download screen with a progress bar and the free space left
- **Waiting screens:** add-ons can show "please wait" messages with a moving bar, and reloading the Home menu shows it instead of a black screen
- **More robust:** if the menu is closed by the system (SIGTERM/SIGHUP), the console's UI comes back instead of staying paused. Scripts started from the menu no longer hold the screen
- **For add-on developers:** new `DOWNLOAD` and `CHOICE` command types, the `DELETE_BLOCKED_KEY` dialog and new `om_functions` helpers (`omSplash`, `omLoading`, `omUnpack`, `omReloadMenu`, `omReloadMenuLater`, `omLangCode`, `omLangList`). The C++ framework gained `ScreenChrome`, `PanelDialog`, `NoteBox`, list-row and text-layout helpers, and `close_on_exec`; `ModernChrome` is gone (use `ScreenChrome`) and the code builds with `-std=c++14`. Add-ons made for 2.0.0 keep working

## v2.0.0

A rewrite of the interface and a large part of the scripts, continuing CompCom's Options Menu (last release 1.3.4).

- **New look:** smooth TTF fonts, preview icons, toggle switches, submenu markers and button hints, with colors you can change (`theme.cfg`)
- **9 languages:** English, Français, Deutsch, Español, Italiano, Nederlands, Português, Русский, 日本語
- **New menus:** Controller (autofire, Home combo, Start on the 2nd Famicom controller), Diagnostics, Saved Games, Wi-Fi backups, epilepsy protection
- **Safer behaviour:** entries that need a USB/SD drive or a missing component are hidden instead of failing, and lists let you delete entries
- **NES/Famicom friendly:** the opening combo is picked automatically for consoles without shoulder buttons
- Installing it replaces CompCom's Options Menu and the old Hibernate mod, and keeps your button combo, language and Home combo settings
