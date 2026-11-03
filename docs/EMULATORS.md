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
  firmware boot entry's name, used to pick BootNext), `esp` (where that OS
  mounts the ESP, for the helper).
- `[[emulator]]`: `id`, `name`, `os`, `exe`, `args` with `{rom}` for the
  game, and `cards` (the emulator's memory card folder, for syncing).
- `[[roms]]`: `os`, `path`, `emulator`, `ext` (defaults to iso, gcm,
  rvz, ciso, gcz, wbfs, dol) and `recursive`.

## The flow

1. The helper runs at login (and on demand). It scans each `[[roms]]`
   folder for its OS, reads the title and game code from the disc header
   where the format allows, and writes `library.toml` to the ESP, keeping
   the other OS's games in it.
2. In the boot menu, the Game side with no disc shows the game list.
3. Picking a game writes `GameCoobLaunch` (the counter one higher than
   the last one), sets BootNext to the OS's boot entry, and resets.
4. The OS boots. The helper starts at login, reads `GameCoobLaunch`, and
   if its counter is newer than the last one it handled, runs the
   emulator with the game. The counter means the request runs once
   without the helper deleting the variable (reading is allowed for
   normal users on Linux; writing or deleting needs root).

How the helper starts at login:

- **Windows**: a scheduled task at logon with highest privileges (reading
  firmware variables there needs the SE_SYSTEM_ENVIRONMENT privilege,
  which only an elevated process has). It mounts the ESP with `mountvol`
  to update the game list.
- **Linux (Arch first)**: a systemd user service. Variables are read from
  `/sys/firmware/efi/efivars`; the ESP is wherever `esp` says, or `/efi`,
  `/boot`, `/boot/efi`.

## Memory cards

Card images edited in the boot menu live in `[boot] cards` on the ESP.
Before starting the emulator the helper copies them into the emulator's
`cards` folder (converting to its format with `src/card`), and copies them
back when the emulator exits, keeping a `.bak` each way. So a save copied
in the boot menu shows up in the game, and the other way round.

## Steps

1. File formats, parser, example and `gamecoob-launch check` (this step).
2. Helper: scan folders, read disc headers, write `library.toml`, read
   `GameCoobLaunch`, start the emulator.
3. Boot menu: game list on the Game side, the launch variable, BootNext
   by boot entry name; `[boot]` paths replace the built-in ones.
4. Memory card sync.
5. Installing the helper: Windows scheduled task, Linux systemd user
   service.
