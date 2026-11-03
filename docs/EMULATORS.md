# Launching games in an emulator from the boot menu

Goal: pick a game on the cube menu, and the PC boots into Windows or Linux
and starts it in an emulator (Dolphin first), without the user touching
the OS on the way.

GameCoob never ships games, BIOS dumps or other Nintendo files. Every path
below points at the user's own copies.

## Why it takes two programs

The boot app runs before any OS. It cannot run Dolphin, and UEFI firmware
can only read FAT partitions, so it cannot see games on an NTFS or ext4
disk either. The work is split between:

- **The boot app** (this repo's `src/`): shows the games, records the pick
  and reboots into the right OS.
- **The helper**, `gamecoob-launch` (`tools/gamecoob-launch`), a small
  program installed on each OS. It scans the game folders, writes the
  list to the EFI system partition (ESP, the FAT partition both sides can
  reach) and, after the reboot, starts the emulator.

Both read the same files, through `src/launch` (shared the way
`tools/card-tool` shares `src/card`).

## The files

| File | Written by | Holds |
|---|---|---|
| `\EFI\GameCoob\paths.toml` | the user | the boot app's files, the OSes, the emulators, the game folders |
| `\EFI\GameCoob\library.toml` | the helper | the games found, with title, game code, OS, emulator and path |
| UEFI variable `GameCoobLaunch` | the boot app | the game to start: OS, emulator, path, and a counter |

All three are a small subset of TOML: tables, arrays of tables, and
`key = value` with text, whole numbers, true/false and lists.
`'single quotes'` keep Windows backslashes as they are. Unknown tables or
keys are errors with a line number, so a typo never goes unnoticed;
`gamecoob-launch check paths.toml` lists every problem at once.

See [`paths.example.toml`](paths.example.toml) for a full paths file.
Its parts:

- `[boot]`: `ipl`, `cards`, `saves` on the ESP. Defaults are today's
  built-in paths (`\EFI\GameCoob\ipl.bin`, `...\cards`, `...\saves`).
- `[[os]]`: `id`, `kind` (`windows` or `linux`), `boot` (text in the
  firmware boot entry's name, used to pick BootNext).
- `[[emulator]]`: `id`, `name`, `os`, `exe`, `args` with `{rom}` for the
  game, and `cards` (the emulator's memory card folder, for syncing).
- `[[roms]]`: `os`, `path`, `emulator`, `ext` (defaults to iso, gcm,
  rvz, ciso, gcz, wbfs, dol) and `recursive`.

## The flow

1. The helper runs at login (and on demand). It scans each `[[roms]]`
   folder for its OS, reads the title and game code from the disc header
   where the format allows, and writes `library.toml` to the ESP, keeping
   the other OS's games in it.
2. In the boot menu, START on Game Play with no disc ready shows the game
   list.
3. Picking a game writes `GameCoobLaunch` (the counter one higher than
   the last one), sets BootNext to the OS's boot entry, and resets.
4. The OS boots. The helper starts at login, reads `GameCoobLaunch`, and
   if its counter is newer than the last one it handled, runs the
   emulator with the game. The counter means the request runs once
   without the helper deleting the variable (reading is allowed for
   normal users on Linux; writing or deleting needs root).

How the helper runs:

- **Windows**: a scheduled task at logon with highest privileges, running
  `gamecoob-launch run` (reading firmware variables needs the
  SE_SYSTEM_ENVIRONMENT privilege, which only an elevated process has).
  It mounts the ESP with `mountvol` to update the game list.
- **Linux (Arch first)**: Arch mounts the ESP so only root can read it,
  so the work is split. `gamecoob-scan.service` runs `gamecoob-launch
  scan` as root at boot: it updates the game list and keeps a readable
  copy of `paths.toml` in `/var/lib/gamecoob`. An XDG autostart entry
  runs `gamecoob-launch run` at desktop login, as you, so the emulator
  opens on your desktop; it reads the variable from
  `/sys/firmware/efi/efivars` (readable by every user) and the paths
  from the copy. The ESP is the FAT mount (from `/proc/mounts`) holding
  `EFI/GameCoob`, usually `/efi`, `/boot` or `/boot/efi`. `--esp DIR`
  overrides it on either OS.

### Installing

CI's **GameCoob-launch-helper** download has `linux/`, `windows/`, this
file and `paths.example.toml`.

1. Copy `paths.example.toml` to `\EFI\GameCoob\paths.toml` on the ESP
   and fill in your OSes, emulators and game folders.
2. Linux: `cd linux && sudo sh install-linux.sh`.
   Windows, in an administrator PowerShell: `cd windows` then
   `powershell -ExecutionPolicy Bypass -File install-windows.ps1`.
3. Each installer checks the paths file, scans once and marks any old
   launch request as handled. `--uninstall` / `-Uninstall` removes it.

## Memory cards

Card images edited in the boot menu live in `[boot] cards` on the ESP.
Before starting the emulator the helper copies them into the emulator's
`cards` folder (converting to its format with `src/card`), and copies them
back when the emulator exits, keeping a `.bak` each way. So a save copied
in the boot menu shows up in the game, and the other way round.

## Steps

1. File formats, parser, example and `gamecoob-launch check` (#144).
2. Helper: scan folders, read disc headers, write `library.toml`, read
   `GameCoobLaunch`, start the emulator (`gamecoob-launch scan`, `run`,
   `mark`) (#146).
3. Boot menu: START on Game Play with no disc opens the game list; a
   pick writes the launch variable and sets BootNext by boot entry name
   (#148).
4. Installing the helper: Windows scheduled task; Linux boot-time scan
   service and login autostart.
5. `[boot]` paths replace the built-in ones.
6. Memory card sync.
