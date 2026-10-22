# NEOS headers

The IPL's CPU-side audio engine ("NEOS", the N64 sound driver ported to
GameCube) is the same code Animal Crossing ships as `jaudio_NES`.

Headers under `jaudio_NES/`, `libultra/` and `libc64/` are copied from
[ac-decomp](https://github.com/ACreTeam/ac-decomp) (CC0), trimmed to what the
IPL units need. `PR/abi.h` is written from scratch for this project (the
original SDK header is not redistributable).
