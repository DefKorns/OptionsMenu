<#
  Copyright (C) 2026 DefKorns

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 3
  of the License, or (at your option) any later version.

  Sends the game names of a Hakchi CE library to the console, so the Save Manager
  shows names instead of codes for saves of removed games. Read-only on both sides
  except for game_names.txt in the saves folder; saves are never touched.

  push_game_names.ps1 [--ip CONSOLE_IP] [--hakchi-path DIR]
  --ip defaults to 169.254.13.37, the console over USB.
#>
[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Ip = '169.254.13.37',
    [string]$HakchiPath,
    # in-session calls pass --ip as a plain value, not as a parameter name
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Rest
)

$ErrorActionPreference = 'Stop'
$PushPath = '/tmp/hakchi_game_names.push'
$MergeScript = '/etc/options_menu/scripts/OrphanSaves.sh'

function Stop-WithError([string]$message) {
    Write-Host $message -ForegroundColor Red
    exit 1
}

for ($i = 0; $i -lt $Rest.Count; $i++) {
    $option = $Rest[$i]
    if ($i + 1 -ge $Rest.Count) { Stop-WithError "Missing value for $option. Usage: push_game_names.ps1 [--ip CONSOLE_IP] [--hakchi-path DIR]" }
    switch ($option) {
        '--ip' { $Ip = $Rest[++$i] }
        '--hakchi-path' { $HakchiPath = $Rest[++$i] }
        default { Stop-WithError "Unknown option $option. Usage: push_game_names.ps1 [--ip CONSOLE_IP] [--hakchi-path DIR]" }
    }
}

function Test-Library([string]$dir) {
    $dir -and (Test-Path -LiteralPath $dir) -and @(Get-ChildItem -LiteralPath $dir -Directory -Filter 'games*').Count -gt 0
}

# portable mode keeps games next to hakchi.exe, non-portable in Documents\hakchi2
function Find-Library {
    $candidates = @($PSScriptRoot)
    foreach ($process in @(Get-Process -Name hakchi -ErrorAction SilentlyContinue)) {
        if ($process.Path) { $candidates += Split-Path -Parent $process.Path }
    }
    $candidates += Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'hakchi2'
    foreach ($dir in $candidates) {
        if (Test-Library $dir) { return $dir }
    }
    return $null
}

if (-not $HakchiPath) { $HakchiPath = Find-Library }
if (-not (Test-Library $HakchiPath)) {
    Stop-WithError 'Hakchi CE games not found. Put this script next to hakchi.exe, open Hakchi CE first, or pass --hakchi-path with the folder that has the games folders.'
}

# same rules as OrphanSaves.sh gameNames: code -> Name=, RetroArch content -> name of its game
$utf8 = New-Object System.Text.UTF8Encoding($false)
$gameNames = New-Object 'System.Collections.Generic.Dictionary[string,string]'
$seen = New-Object 'System.Collections.Generic.HashSet[string]'
$lines = New-Object 'System.Collections.Generic.List[string]'
$roms = New-Object 'System.Collections.Generic.List[string[]]'
$skipped = 0

function Add-Line([string]$key, [string]$name) {
    if ($script:seen.Add($key)) { $script:lines.Add("$key`t$name") }
}

foreach ($gamesDir in Get-ChildItem -LiteralPath $HakchiPath -Directory -Filter 'games*' | Sort-Object Name) {
    foreach ($gameDir in Get-ChildItem -LiteralPath $gamesDir.FullName -Directory) {
        foreach ($file in Get-ChildItem -LiteralPath $gameDir.FullName -File) {
            if ($file.Extension -cne '.desktop') {
                if ($file.Extension -cne '.png') { $roms.Add(@($gameDir.Name, $file.Name)) }
                continue
            }
            try { $text = [IO.File]::ReadAllLines($file.FullName, $utf8) }
            catch { $skipped++; continue }
            $code = ''; $name = ''; $exec = ''
            foreach ($raw in $text) {
                $line = $raw.Replace("`t", ' ')
                if ($line.StartsWith('Code=')) { $code = $line.Substring(5) }
                elseif ($line.StartsWith('Name=') -and $name -eq '') { $name = $line.Substring(5) }
                elseif ($line.StartsWith('Exec=')) { $exec = $line.Substring(5) }
            }
            if (-not $code.StartsWith('CLV-') -or $name -eq '') { $skipped++; continue }
            if (-not $gameNames.ContainsKey($code)) {
                $gameNames[$code] = $name
                Add-Line $code $name
            }
            foreach ($token in $exec.Split(' ')) {
                if ($token.Contains('/')) { $roms.Add(@($code, $token.Substring($token.LastIndexOf('/') + 1))) }
            }
        }
    }
}

foreach ($rom in $roms) {
    $name = $null
    if ($rom[1] -eq '' -or -not $gameNames.TryGetValue($rom[0], [ref]$name)) { continue }
    # game.sfc.gz is saved as game.srm
    $content = $rom[1]
    Add-Line "retroarch/$content" $name
    for ($i = 0; $i -lt 2 -and $content -match '\.[^.]*$'; $i++) {
        $content = $content -replace '\.[^.]*$', ''
        Add-Line "retroarch/$content" $name
    }
}

if ($gameNames.Count -eq 0) {
    Stop-WithError "No games with a name found in $HakchiPath."
}

$tmp = [IO.Path]::GetTempFileName()
try {
    [IO.File]::WriteAllText($tmp, ($lines -join "`n") + "`n", $utf8)
    $remote = "`"cat > $PushPath && sh $MergeScript merge-names $PushPath; status=`$?; rm -f $PushPath; exit `$status`""
    Write-Host "Sending $($gameNames.Count) game names from $HakchiPath to $Ip..."
    $ssh = Start-Process -FilePath ssh -NoNewWindow -Wait -PassThru -RedirectStandardInput $tmp `
        -ArgumentList @('-o', 'ConnectTimeout=10', '-o', 'StrictHostKeyChecking=accept-new', "root@$Ip", $remote)
    if ($ssh.ExitCode -ne 0) {
        Stop-WithError "Could not reach the console at $Ip (ssh exit code $($ssh.ExitCode)). Check that it is on and pass its IP with --ip."
    }
}
finally {
    Remove-Item -LiteralPath $tmp -ErrorAction SilentlyContinue
}

Write-Host "Done. Open Save Manager > Clean Saves of Removed Games to see the names."
if ($skipped -gt 0) { Write-Host "$skipped game files without a code or name were skipped." }
