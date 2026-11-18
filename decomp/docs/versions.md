# Other IPL versions

Goal: every retail IPL revision builds byte-exact from this one source tree,
not just PAL 1.0. This is the plan for adding each one.

## Revisions

| Version id | Revision | Status |
|---|---|---|
| `PAL10` | PAL 1.0 | in progress (the base) |
| `PAL12` | PAL 1.2 | need dump |
| `NTSC10J` | NTSC-J 1.0 | need dump |
| `NTSC10U` | NTSC-U 1.0 | need dump |
| `NTSC11` | NTSC 1.1 | need dump |
| `NTSC12` | NTSC 1.2 (DOL-001 and DOL-101 builds may differ) | need dump |
| `MPAL11` | MPAL 1.1 (Brazil) | need dump |

The list is what is known to exist; the id and md5 of each one get pinned
when a real dump is in hand. Dev/TDEV and Panasonic Q IPLs come last, if at all.

Dumps are never committed. They go in `orig/<VER>/ipl.bin` (gitignored), the
same as PAL 1.0.

## Layout (already in place)

- One `src/` tree for all versions. Per-version files live in
  `config/<VER>/`: `config.yml`, `symbols.txt`, `splits.txt`, `build.sha1`,
  `bs1.sha1`, `names.txt`.
- `configure.py -v <VER>` builds a version. `VERSIONS` lists them; each build
  gets `-DVERSION_<VER>` and `-DBUILD_VERSION=<n>`.
- `MatchingFor("PAL10", ...)` marks a unit as matching only for some versions.
- `tools/extract_bs2.py` `KNOWN` maps a dump md5 to its version id and the
  address of its BS2 section table.

## Steps per version

1. **Extract.** Add the md5 to `KNOWN` in `tools/extract_bs2.py` with the
   section table address (the `lis/addi r3` operand in `__init_data`; add a
   scanner so it is found, not hand-typed). Check the descramble range
   (`SCRAMBLE_END`) still fits; font layout differs on NTSC-J (Shift-JIS font).
2. **Port symbols.** Write `tools/port_symbols.py`: hash every PAL 1.0
   function with relocation operands masked, find the same bytes in the new
   `bs2.dol`, and write `config/<VER>/symbols.txt` and `splits.txt` from the
   hits. Unmatched functions are the real differences; list them.
3. **Configure.** Add the id to `VERSIONS`, write `config/<VER>/config.yml`
   (dtk hash of the new `bs2.dol`, its `block_relocations` ranges), and
   `build.sha1` / `bs1.sha1`.
4. **Diff the code.** For each changed function, put the difference behind
   `#if VERSION_...` (or `BUILD_VERSION >= n`) in the shared source. Units with
   no differences match for free. Expect: video mode and language code (PAL vs
   NTSC), Shift-JIS text on NTSC-J, SDK library revisions (newer SDK in later
   IPLs, maybe a different compiler for some libraries), and bug fixes in the
   menu between 1.0 and 1.2.
5. **Data.** Asset archives and strings get extracted at build time from that
   version's dump, like PAL 1.0 (`tools/extract_blobs.py`). Their offsets per
   version go in config, never the bytes.
6. **BS1.** Match `bs1` for the version and add its `bs1.sha1`.
7. **CI.** The progress workflow builds each version whose dump secret is set;
   progress art gets a row per version.

## Order

PAL 1.2 first if available (same region, so the smallest diff from PAL 1.0),
then NTSC 1.0/1.1/1.2 (NTSC differences learned once, reused), then MPAL and
NTSC-J. PAL 1.0 stays the reference for all of it; finishing its own leftovers
is still separate work.
