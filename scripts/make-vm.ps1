# Make a VMware Workstation VM that boots GameCoob, in local\vm.
# Usage: scripts\make-vm.ps1 [-Boot <folder with EFI\BOOT\BOOTX64.EFI>]
#   Without -Boot it uses the newest boot files under local\ (an unzipped
#   GameCoob-boot-files download) or target\esp (from scripts\build.ps1).
# Your dump at local\ipl.bin is copied onto the disk so the real intro plays.
# Needs administrator rights (Windows' diskpart builds the disk); it asks for them.
# Then open local\vm\GameCoob.vmx in VMware. Close the VM before re-running.
param([string]$Boot, [switch]$Pause)
$ErrorActionPreference = "Stop"
# The elevated copy runs in its own window, so keep it open to show the result.
trap { Write-Host "Failed: $_" -ForegroundColor Red; if ($Pause) { Read-Host "Press Enter to close" }; exit 1 }
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $root

$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) {
    $argList = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Pause"
    if ($Boot) { $argList += " -Boot `"$((Resolve-Path $Boot).Path)`"" }
    $p = Start-Process powershell -Verb RunAs -ArgumentList $argList -Wait -PassThru
    exit $p.ExitCode
}

# Find the boot files.
if (-not $Boot) {
    $efi = @(Get-ChildItem -Recurse -File -Filter BOOTX64.EFI -Path "local", "target\esp" -ErrorAction SilentlyContinue |
        Where-Object { $_.Directory.Name -eq "BOOT" -and $_.Directory.Parent.Name -eq "EFI" -and $_.FullName -notlike "*\local\vm\*" } |
        Sort-Object LastWriteTime -Descending)
    if ($efi.Count -eq 0) { throw "No boot files found. Unzip the GameCoob-boot-files download into local\ or run scripts\build.ps1." }
    $Boot = $efi[0].Directory.Parent.Parent.FullName
}
$Boot = (Resolve-Path $Boot).Path
if (-not (Test-Path "$Boot\EFI\BOOT\BOOTX64.EFI")) { throw "$Boot has no EFI\BOOT\BOOTX64.EFI" }
Write-Host "Boot files: $Boot"

$vm = Join-Path $root "local\vm"
$vhd = Join-Path $vm "GameCoob.vhd"
New-Item -ItemType Directory -Force $vm | Out-Null

function Invoke-DiskPart([string[]]$lines) {
    $script = Join-Path $env:TEMP "gamecoob-diskpart.txt"
    Set-Content -Path $script -Value $lines -Encoding Ascii
    $out = diskpart /s $script
    Remove-Item $script
    if ($LASTEXITCODE -ne 0) { $out | Write-Host; throw "diskpart failed" }
}

# Start from a fresh disk each time.
if (Test-Path $vhd) {
    try { Invoke-DiskPart @("select vdisk file=`"$vhd`"", "detach vdisk noerr") } catch {}
    Remove-Item $vhd
}

$used = (Get-PSDrive -PSProvider FileSystem).Name
$letter = [char[]]"ZYXWVUTSRQPONMLKJIHG" | Where-Object { $used -notcontains "$_" } | Select-Object -First 1
if (-not $letter) { throw "No free drive letter to mount the disk" }

# A 128 MB fixed VHD is a raw disk image plus a 512-byte footer, so VMware can
# use it directly through a flat .vmdk descriptor.
$mb = 128
Invoke-DiskPart @(
    "create vdisk file=`"$vhd`" maximum=$mb type=fixed",
    "select vdisk file=`"$vhd`"",
    "attach vdisk",
    "convert mbr",
    "create partition primary",
    "format fs=fat32 quick label=GAMECOOB",
    "assign letter=$letter")
try {
    $dest = "${letter}:\"
    Copy-Item -Recurse -Force "$Boot\EFI" $dest
    New-Item -ItemType Directory -Force "${dest}EFI\GameCoob" | Out-Null
    if (Test-Path "local\ipl.bin") {
        Copy-Item -Force "local\ipl.bin" "${dest}EFI\GameCoob\ipl.bin"
        Write-Host "Copied your dump from local\ipl.bin"
    } else {
        Write-Host "No local\ipl.bin, so the original GameCoob intro will play"
    }
} finally {
    Invoke-DiskPart @("select vdisk file=`"$vhd`"", "detach vdisk")
}

$utf8 = New-Object System.Text.UTF8Encoding $false
$sectors = $mb * 1024 * 1024 / 512
[IO.File]::WriteAllText((Join-Path $vm "GameCoob.vmdk"), (@(
    '# Disk DescriptorFile'
    'version=1'
    'CID=fffffffe'
    'parentCID=ffffffff'
    'createType="monolithicFlat"'
    ''
    '# Extent description'
    "RW $sectors FLAT `"GameCoob.vhd`" 0"
    ''
    '# The Disk Data Base'
    '#DDB'
    ''
    'ddb.virtualHWVersion = "16"'
    'ddb.adapterType = "ide"'
    'ddb.geometry.cylinders = "260"'
    'ddb.geometry.heads = "16"'
    'ddb.geometry.sectors = "63"'
) -join "`n") + "`n", $utf8)

$vmx = Join-Path $vm "GameCoob.vmx"
if (-not (Test-Path $vmx)) {
    [IO.File]::WriteAllText($vmx, (@(
        '.encoding = "UTF-8"'
        'config.version = "8"'
        'virtualHW.version = "16"'
        'displayName = "GameCoob"'
        'guestOS = "other-64"'
        'firmware = "efi"'
        'uefi.secureBoot.enabled = "FALSE"'
        'memsize = "1024"'
        'numvcpus = "4"'
        'cpuid.coresPerSocket = "4"'
        'sata0.present = "TRUE"'
        'sata0:0.present = "TRUE"'
        'sata0:0.deviceType = "disk"'
        'sata0:0.fileName = "GameCoob.vmdk"'
        'usb.present = "TRUE"'
        'ehci.present = "TRUE"'
        'svga.present = "TRUE"'
        'floppy0.present = "FALSE"'
    ) -join "`r`n") + "`r`n", $utf8)
}
Write-Host "Done. Open $vmx in VMware Workstation and power it on."
if ($Pause) { Read-Host "Press Enter to close" }
