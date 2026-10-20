# Boot GameCoob in QEMU on Windows, straight from a downloaded build.
# Usage: scripts\run-qemu.ps1 [-Boot <folder with EFI\BOOT\BOOTX64.EFI>] [-Fast] [-Slow]
#   Without -Boot it uses the newest boot files under local\ (an unzipped
#   GameCoob-boot-files download, or the .zip itself) or target\esp.
#   By default it uses Windows Hypervisor Platform when it is turned on
#   ("Turn Windows features on or off", then reboot) and falls back to
#   plain emulation, which is many times slower: GameCoob draws every frame
#   with floating-point math, which emulation does in software.
#   -Fast insists on Windows Hypervisor Platform; -Slow skips it.
# Your dump at local\ipl.bin is added so the real intro plays.
param([string]$Boot, [switch]$Fast, [switch]$Slow)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $root

function Find-Boot {
    @(Get-ChildItem -Recurse -File -Filter BOOTX64.EFI -Path "local", "target\esp" -ErrorAction SilentlyContinue |
        Where-Object { $_.Directory.Name -eq "BOOT" -and $_.Directory.Parent.Name -eq "EFI" -and
                       $_.FullName -notlike "*\local\vm\*" -and $_.FullName -notlike "*\local\qemu\*" } |
        Sort-Object LastWriteTime -Descending)
}

if (-not $Boot) {
    $efi = Find-Boot
    $zip = Get-ChildItem -File -Filter "GameCoob-boot-files*.zip" -Path "local" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    # A zip newer than any unzipped copy wins, so a fresh download just works.
    if ($zip -and ($efi.Count -eq 0 -or $zip.LastWriteTime -gt $efi[0].LastWriteTime)) {
        $out = Join-Path "local" $zip.BaseName
        Expand-Archive -Force $zip.FullName $out
        (Get-Item "$out\EFI\BOOT\BOOTX64.EFI").LastWriteTime = Get-Date
        Write-Host "Unzipped $($zip.Name)"
        $efi = Find-Boot
    }
    if ($efi.Count -eq 0) { throw "No boot files found. Put the GameCoob-boot-files download (zip or unzipped) in local\ or run scripts\build.ps1." }
    $Boot = $efi[0].Directory.Parent.Parent.FullName
}
$Boot = (Resolve-Path $Boot).Path
Write-Host "Boot files: $Boot"

# Lay out a fresh virtual boot partition so the download itself stays untouched.
$esp = "local\qemu\esp"
if (Test-Path $esp) { Remove-Item -Recurse -Force $esp }
New-Item -ItemType Directory -Force "$esp\EFI\GameCoob" | Out-Null
Copy-Item -Recurse -Force "$Boot\EFI" "$esp"
if (Test-Path "local\ipl.bin") {
    Copy-Item -Force "local\ipl.bin" "$esp\EFI\GameCoob\ipl.bin"
    Write-Host "Using your dump from local\ipl.bin"
} else {
    Write-Host "No local\ipl.bin; GameCoob needs your dump and will skip to the next boot entry without it"
}

$qemu = (Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue).Source
if (-not $qemu) { $qemu = "C:\Program Files\qemu\qemu-system-x86_64.exe" }
if (-not (Test-Path $qemu)) { throw "QEMU not found. Install it with: winget install SoftwareFreedomConservancy.QEMU" }
$share = Join-Path (Split-Path $qemu) "share"
$code = Join-Path $share "edk2-x86_64-code.fd"
# Writable variable store so Timeout/BootNext changes persist between runs.
$vars = "local\qemu\vars.fd"
if (-not (Test-Path $vars)) { Copy-Item (Join-Path $share "edk2-i386-vars.fd") $vars }

# QEMU tries each -accel in turn. Emulation runs one host thread per CPU.
$whpx = @("-accel", "whpx,kernel-irqchip=off")
$tcg = @("-accel", "tcg,thread=multi,tb-size=512")
$accel = if ($Fast) { $whpx } elseif ($Slow) { $tcg } else { $whpx + $tcg }
# The renderer splits each frame across every CPU it is given.
$cpus = [Math]::Max(2, [Math]::Min(8, [Environment]::ProcessorCount))
# Show the screen 1:1 instead of stretched to the window, which blurs it.
$display = @()
$displays = try { & $qemu -display help 2>$null } catch { @() }
if ($displays -match "^gtk") { $display = @("-display", "gtk,zoom-to-fit=off") }
Write-Host "CPUs: $cpus; accelerator: $(($accel | Where-Object { $_ -ne '-accel' }) -join ', then ')"
& $qemu -machine q35 -m 1024 -smp $cpus @accel @display `
    -drive "if=pflash,format=raw,readonly=on,file=$code" `
    -drive "if=pflash,format=raw,file=$vars" `
    -drive "format=raw,file=fat:rw:$esp" `
    -net none
