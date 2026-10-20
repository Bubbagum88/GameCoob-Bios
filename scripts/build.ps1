# Build GameCoob on Windows and lay out a boot partition in target\esp.
# Usage: scripts\build.ps1 [-Run] [-Usb E:]
#   -Run       boot it in QEMU (install QEMU for Windows; it ships the OVMF firmware)
#   -Usb E:    also copy the boot files to a FAT32 USB stick at that drive
# Your dump at local\ipl.bin is copied along so the real intro plays.
param([switch]$Run, [string]$Usb)
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

rustup target add x86_64-unknown-uefi | Out-Null
cargo build --release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$esp = "target\esp"
New-Item -ItemType Directory -Force "$esp\EFI\BOOT", "$esp\EFI\GameCoob" | Out-Null
Copy-Item "target\x86_64-unknown-uefi\release\gamecoob.efi" "$esp\EFI\BOOT\BOOTX64.EFI"
Remove-Item -ErrorAction SilentlyContinue "$esp\EFI\GameCoob\ipl.bin"
if (Test-Path "local\ipl.bin") {
    Copy-Item "local\ipl.bin" "$esp\EFI\GameCoob\ipl.bin"
    Write-Host "Using your dump from local\ipl.bin"
} else {
    Write-Host "No local\ipl.bin, so the original GameCoob intro will play"
}
Write-Host "Boot files are in $esp"

if ($Usb) {
    $dest = $Usb.TrimEnd('\') + '\'
    Copy-Item -Recurse -Force "$esp\EFI" $dest
    Write-Host "Copied to $dest"
}

if ($Run) {
    $qemu = (Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue).Source
    if (-not $qemu) { $qemu = "C:\Program Files\qemu\qemu-system-x86_64.exe" }
    if (-not (Test-Path $qemu)) { throw "QEMU not found. Install it from https://qemu.weilnetz.de/w64/" }
    $share = Join-Path (Split-Path $qemu) "share"
    $code = if ($env:OVMF_CODE) { $env:OVMF_CODE } else { Join-Path $share "edk2-x86_64-code.fd" }
    New-Item -ItemType Directory -Force "target\qemu" | Out-Null
    $vars = "target\qemu\OVMF_VARS.fd"
    # Writable variable store so Timeout/BootNext changes persist between runs.
    if (-not (Test-Path $vars)) { Copy-Item (Join-Path $share "edk2-i386-vars.fd") $vars }
    & $qemu -machine q35 -m 512M -smp 4 `
        -drive "if=pflash,format=raw,readonly=on,file=$code" `
        -drive "if=pflash,format=raw,file=$vars" `
        -drive "format=raw,file=fat:rw:$esp" `
        -net none
}
