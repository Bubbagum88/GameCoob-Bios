# Installs gamecoob-launch on Windows. Run in an administrator PowerShell
# from the folder holding gamecoob-launch.exe:
#
#   powershell -ExecutionPolicy Bypass -File install-windows.ps1             install or update
#   powershell -ExecutionPolicy Bypass -File install-windows.ps1 -Uninstall  remove
#
# What it sets up (docs/EMULATORS.md): the program in
# %ProgramFiles%\GameCoob, and a scheduled task that runs
# "gamecoob-launch run" elevated when you log on: it lists your games,
# syncs the boot menu's memory cards with Dolphin's, starts the game picked
# in the boot menu and syncs the cards back when Dolphin closes. Elevation
# is needed to mount the ESP (mountvol) and to read the launch request (a
# firmware variable). The task runs for the user who installs it.
param([switch]$Uninstall)
$ErrorActionPreference = 'Stop'

$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) { throw 'Run this from an administrator PowerShell.' }

$dir = Join-Path $env:ProgramFiles 'GameCoob'
$exe = Join-Path $dir 'gamecoob-launch.exe'
$task = 'GameCoob launch'

if ($Uninstall) {
    Unregister-ScheduledTask -TaskName $task -Confirm:$false -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force (Join-Path $env:ProgramData 'GameCoob') -ErrorAction SilentlyContinue
    Write-Host 'gamecoob-launch removed'
    return
}

New-Item -ItemType Directory -Force $dir | Out-Null
Copy-Item -Force (Join-Path $PSScriptRoot 'gamecoob-launch.exe') $exe

$user = [Security.Principal.WindowsIdentity]::GetCurrent().Name
$action = New-ScheduledTaskAction -Execute $exe -Argument 'run'
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $user
$principal = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Highest
# No time limit: the task waits for the game to end to sync the cards back.
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit ([TimeSpan]::Zero)
Register-ScheduledTask -TaskName $task -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Force | Out-Null

& $exe check
& $exe scan
& $exe sync
# A launch request left from before the install must not start a game at
# the next logon.
& $exe mark
Write-Host 'gamecoob-launch installed'
