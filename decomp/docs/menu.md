# IPL main menu (PAL 1.0 BS2)

This page describes the cube menu that appears after the boot animation, in
enough detail to reimplement it. Everything here comes from the PAL 1.0 dump
(code 0x81310F58..0x813376A0), public SDK API names and public RE notes. The
decompiled code is in `src/bs2/menu/`, the structs are in
`include/bs2/menu.h` and the working names are in
`config/PAL10/names_menu.txt`.

All timings are in frames. The menu logic runs once per video frame, so a
frame is 1/50 s on a 50 Hz display and 1/60 s at 60 Hz (PAL60 / EuRGB60).
The code does not scale anything by refresh rate, so the menu runs 20% faster
at 60 Hz. Angles are 16-bit binary angles (0x4000 = 90 degrees, 0x10000 =
360). `SinS` and `CosS` take such an angle.

## 1. Overview

The menu is Scene state 5. `Scene_Update` calls `Menu_Update`
(0x81312B9C) once per frame, and the scene draw calls `Menu_Draw2D`
(0x8131567C) plus the icon-cube draws.

The big cube from the intro, the "Scene big cube", carries a 2D layout
(GLH0) on five of its faces:

| Direction from front | Face   | Submenu     | Cube rotation       |
|----------------------|--------|-------------|---------------------|
| (front)              | front  | none        | rotX = rotY = 0     |
| Up                   | top    | Game Play   | rotX -> +0x4000     |
| Down                 | bottom | Memory Card | rotX -> -0x4000     |
| Left                 | left   | Options     | rotY -> +0x4000     |
| Right                | right  | Calendar    | rotY -> -0x4000     |

The front-face layout shows the four labels: "Game Play / Memory Card /
Calendar / Options" (STH0 res 58 in English).

When a submenu is entered, the cube zooms away and three faders bring in the
submenu:

1. the icon cubes and the mosaic text,
2. the lit backdrop,
3. the 2D text.

A button guide is drawn along the bottom at all times.

## 2. State machine

`gMenuState` (0x814AD3F4) and `gMenuNextState` (0x814AD3F8) use the
`MenuState` enum:

| Id | State            | Meaning                                      |
|----|------------------|----------------------------------------------|
| 0  | MENU_TOP         | front face                                   |
| 1  | MENU_GAME_FACE   | turned to the Game Play face                 |
| 2  | MENU_GAME        | inside Game Play                             |
| 3  | MENU_CARD_FACE   | turned to the Memory Card face               |
| 4  | MENU_CARD        | inside Memory Card                           |
| 5  | MENU_OPTION_FACE | turned to the Options face                   |
| 6  | MENU_OPTION      | inside Options                               |
| 7  | MENU_CLOCK_FACE  | turned to the Calendar face                  |
| 8  | MENU_CLOCK       | inside Calendar                              |

### Each frame (`Menu_Update`)

1. **SRAM recovery flag.** If the boot code found the SRAM corrupt
   (`gSramLostPending` and `Boot_SramWasLost`), it sets SRAM flags |= 0x08
   once and syncs the SRAM.
2. **Fatal disc error.** If `Disc_GetState() == 0x17`, it sets the scene
   fatal flag (Scene+0xB0). From then on input is ignored and the error
   text is shown.
3. **Per-frame updates.** In order:
   - `Menu_UpdateRotation`: turns toward the face of `gMenuNextState`.
   - The icon updates.
   - `Menu_UpdateSlotGlow`.
   - `Menu_UpdateGuide`.
   - `Menu_UpdateSubFaders`.
4. **Edge actions.**
   - CARD_FACE -> CARD: `Card_Enter`, `CardUi_Reset`, `CardIcon_ResetFiles`.
   - GAME_FACE -> GAME: `Game_Enter`.
   - GAME -> GAME_FACE: reset the three disc faders on the Game face.
5. **Commit the new state.** `gMenuState = gMenuNextState`.
6. **Run the state.**
   - **States 0, 1, 3, 5, 7** (on the cube):
     - The zoom goes down by 0x578 per frame, to 0.
     - Unless there is a fatal error, `Menu_HandleInput` runs.
     - `wobbleFrame` goes up by 7.
     - The cube matrix is calculated and the Game-face disc faders are
       updated.
     - The mosaic pool is cleared (`Fx_Clear`).
   - **States 2, 4, 6, 8** (in a submenu):
     - The zoom goes up by 0x730 per frame, to 0x7FFF.
     - `wobbleFrame` goes up by 7 and the cube matrix is calculated.
     - The next state is whatever the submenu update returns
       (`Game_Update`, `Card_Update`, `Option_Update`, `Clock_Update`).
7. **Error text.** With a fatal error, the error-text fader fades in.

### Input on the cube (`Menu_HandleInput`, 0x81311868)

Pad state is in `gMenuPad` (0x814AF60C):

- `buttonDown` (+0xC): A 0x100, B 0x200, START 0x1000.
- `dirsNew` (+0x12) holds directions with key repeat: UP 0x1000, DOWN
  0x2000, LEFT 0x4000, RIGHT 0x8000.

Conditions:

- "Settled on front" means rotX == 0 && rotY == 0.
- "Settled on face" means the rotation equals exactly +-0x4000 and zoom == 0.
- `MenuWork.inputAlways` (+0x32, a debug flag, 0 by default) bypasses both
  checks.

| State         | Input                      | Condition          | Effect           | SE |
|---------------|----------------------------|--------------------|------------------|----|
| TOP           | Up / Down / Left / Right   | settled on front   | -> GAME_FACE / CARD_FACE / OPTION_FACE / CLOCK_FACE | 3 |
| any *_FACE    | the opposite direction     | settled on face    | -> TOP           | 4  |
| any *_FACE    | B                          | settled on face    | -> TOP           | 4  |
| any *_FACE    | A                          | settled on face    | -> the submenu   | 5  |

There is no diagonal movement. From a face you can only go back to the front.

A submenu returns to its face (SE 6) on B. B is accepted only when the zoom
is complete (0x7FFF) or `inputAlways` is set, and only when the submenu is at
its top level.

## 3. Cube motion and geometry

### Rotation (`Menu_UpdateRotation`, 0x813120F0)

`rotStep = 750` per frame:

- The target axis moves 750 per frame and clamps at +-0x4000.
- The other axis returns to 0. It first snaps to a multiple of 750, then
  moves one step per frame.

A quarter turn takes 22 frames: 0.44 s at 50 Hz, 0.37 s at 60 Hz. Leaving a
submenu keeps the face rotation; only the zoom runs back.

### Zoom

- `zoom` goes from 0 to 0x7FFF.
- Zooming in takes 18 frames (0x730 per frame): 0.36 s at 50 Hz, 0.30 s at
  60 Hz.
- Zooming out takes 24 frames (0x578 per frame): 0.48 s at 50 Hz, 0.40 s at
  60 Hz.

The cube is translated by `zoomDepth * (1 - CosS(zoom))` in z, with
zoomDepth = -70. It is also rotated about z by `zoom * zoomSpin`, but
zoomSpin is 0.

### Cube matrix (`Menu_CalcCubeMtx`, 0x81311D50)

The matrix is built as:

```
M = RotXY(rotX, rotY)
M = Wobble(t) * M         t = wobbleFrame (+7 per frame)
M = ZoomPush * M          only when zoom != 0
cubeMtx = M; big cube model matrix = cubeMtx, scale 1.1 * Scene.modelScale
```

The idle wobble terms are:

```
tx = 15   * SinS(-0x4000 + 25 t)
ty = 6    * SinS(50 t)
rx = 350  * CosS(50 t)
ry = 1000 * CosS(-15000 + 25 t)
rz = 1000 * CosS(25 t)
Wobble = MtxRotTrans(rx, ry, rz, tx, ty, 0)
```

The wobble runs in every menu state, including inside submenus.

### Faces (`Menu_DrawFaces`, 0x81314304)

Each face layout is 288 x 288 units with y pointing down. Its matrix is
`view * cubeMtx * SRT`, using the SRT table at 0x8137EC34 (32 bytes each:
scale, s16 rx/ry/rz, translation). The scale is (1, -1, 1) for every face.
The code fills in the +-144 component that is missing from the table:

| Face        | Rotation   | Translation        | Shown when         |
|-------------|------------|--------------------|--------------------|
| front       | 0          | (-144, 144, 144)   | see below          |
| Memory Card | rx +0x4000 | (-144, -144, 144)  | rotX <= -0x2E3A    |
| Game Play   | rx -0x4000 | (-144, 144, -144)  | rotX >= +0x2E3A    |
| Calendar    | ry +0x4000 | (144, 144, 144)    | rotY <= -0x2E3A    |
| Options     | ry -0x4000 | (-144, 144, -144)  | rotY >= +0x2E3A    |

**Side faces.** A side face is drawn once the cube has turned 0x2E3A (about
65 degrees) toward it. Its alpha is:

```
a = 255 - (0x4000 - |rot|) * 255 / 5000
```

So it goes from 0 to 255 over the last 5000 units of the turn.

**Front face.**

- With m = min(max(|rotX|, |rotY|), 5000), the front face is drawn with
  alpha 255 - m*255/5000.
- This applies only when |rotX| != |rotY|, or when both are 0.
- The front face's alpha is applied twice (alpha^2/255), and so is the
  frame of each side face.

**Panes.** Panes whose name begins with "z" are drop shadows drawn in
(40, 40, 40). They are drawn before the white pane with the same tail
(`zitl` before `titl`, and so on). Each face has these panes:

- **Game Play** (res 56/148/141/162/155/131):
  - `fram` (9-slice frame) and `titl`.
  - `zodi`/`nodi`: "no disc", shown while the disc state is 0x12.
  - `zust`/`qust`: a disc without a banner.
  - `bana`: the banner image, 96x32 RGB5A3 at BNR+0x20.
  - `zame`/`game` and `zakr`/`makr`: the BNR short name (+0x1820) and short
    maker (+0x1840). BNR2 adds language * 0x140.
- **Memory Card** (res 59/151/144/165/158/134):
  - `fram` and `titl`.
  - Per slot, `texa`/`texb` labels.
  - A `mca1`/`mcb1` frame with alpha `gSlotGlow`.
  - A `mca2`/`mcb2` fill bar. Its height is
    `fill * (glow - 60) / 195 * paneH / 255`, anchored at the bottom.
  - The glow moves +-4 per frame between 60 and 255: up while the slot is
    ready (state 4), down otherwise.
  - fill = 255 - freeBlocks * 255 / totalBlocks.
- **Calendar** (res 136/146/139/160/153/129). Shows the current
  `OSGetTime()`:
  - `colo`, `slas` and `pare` (separators) and `titl`.
  - `cal1..cal8` are the date digits: cal1 is always "2", cal2..4 are the
    year's last three digits, cal5..6 the month (1-12), cal7..8 the day.
  - `week` is the weekday texture (res 49+wday for English).
  - `tim1..tim6` are hh mm ss.
  - Every pane has a "z" shadow twin.
- **Options** (res 138/152/145/166/159/135):
  - `txt1..txt3` and `titl`.
  - `arwr` or `arwl`: an arrow, shown when the SRAM display offset is
    > 0 or < 0.
  - `num1`/`num2`: two digits of |offset|.
  - `sond`: the mono or stereo label texture (res 47/48 in English, 120x24
    I4).
  - `lang`: the language name.

**Front face.** The front-face layout is res 57/149/142/163/156/132
(English first; the per-language order is En, De, Fr, Es, It, Nl). During
the spin-out (Scene state 4), the front face is drawn on the spinning cube
by `Menu_DrawSpinOutFace`.

**Submenus.** Each submenu has three faders, `subFade[*][0..2]`, with delays
0, 20 and 70 frames and a 20-frame ramp:

- fader 0: icon cubes and the mosaic,
- fader 1: backdrop,
- fader 2: text.

A submenu is drawn while fader 0 is above 0. `Menu_CardFadeDone` is true
when all three card faders are above 172.

The submenu screens use a fixed camera:

- The eye is at z = 224 / tan(fovy/2), looking at the origin, so one unit
  is one pixel on a 448-line screen.
- The SRT is S(1, -1, 1) T(-292, 224, 0). The 2D layouts are 584 x 448.

### Backdrop

`Backdrop_Draw(mv, alpha)` (0x8130B244) draws an 8x8 I4 tile (res 89) lit
by a spot light. The light is set in `MenuWork`:

- position (296, 224, 474),
- cutoff 36,
- attenuation 500 / 0.375.

The background models of the scene fade with the zoom:

```
alpha = bgMinAlpha + (255 - bgMinAlpha) * (0x7FFF - zoom) / 0x7FFF
```

bgMinAlpha is 0.

## 4. Faders

Most 2D elements are `Fader`s (0x10 bytes):

- frame, length (20), delay, ticks,
- maxAlpha (255),
- slide (in 12.4 units),
- id (a string index or tag).

`Fader_Step(f, mode)` advances the fader one frame:

- **0, in:** frame++, clamped to delay+length.
- **1, out:** frame--. Once frame drops below delay it jumps to 0.
- **2, half:** frame moves toward delay + length/2.

The alpha is `maxAlpha * (frame - delay) / length`, or 0 while
frame < delay. `Fader_DrawString` draws STH0 string `id`, shifted by
slide * 16 * (remaining part of the ramp). With the defaults, a fade-in
takes 20 frames (0.40 s at 50 Hz, 0.33 s at 60 Hz).

## 5. Button guide (`Menu_DrawGuide`, 0x8131508C)

- **Layout.** Res 115, drawn in 2D over everything.
- **Labels.** STH0 res 116..121 (one per language; En = 117), 22 strings.
  The labels used are:
  - 0 "Menu Selection"
  - 1 "Cancel"
  - 2 "Confirm"
  - 4 "Select"
  - 6 "Confirm"
  - 8 "Cancel"
  - 11 "Finish"
  - 13 "Change"

  Each label is drawn once for each of the six languages, scaled by that
  language's `langGuide` fader. This makes a language change in Options
  cross-fade.
- **Glyph groups.** `guideIcon[0..5]` are two lines of three glyphs: stick,
  B, A.
  - The panes `bac1..bac6` are the glyph backgrounds. The stick background
    is (150, 150, 150). The B background is guideColorB (210, 0, 70) and
    the A background is guideColorA (0, 200, 140).
  - The backgrounds pulse 255 -> 100 -> 255 over 256 frames.
  - The stick sprite cross-fades every 128 frames between neutral (`stk0`,
    `stk5`) and one of four tilted sprites. The tilted sprite cycles through
    the directions.
  - `buta`/`butd` (A), `butb`/`butc` (B) and `dot1..dot6` are drawn white.
    They fade in with `gGuideAlpha`, which goes up 4 per frame to 255.

Which labels are visible in each state:

| State                    | Labels shown                         | Glyph groups |
|--------------------------|--------------------------------------|--------------|
| TOP                      | Menu Selection                       | 3 (stick)    |
| *_FACE                   | Cancel, Confirm                      | 4, 5 (B, A)  |
| GAME                     | Cancel (hidden after a fatal error)  | 1            |
| OPTION, CLOCK at top     | Select, Finish, Confirm              | 0, 1, 2      |
| OPTION, CLOCK editing    | Change, Cancel, Confirm              | 0, 1, 2      |
| CARD while browsing      | Select, Finish, Confirm              | 0, 1, 2      |
| CARD otherwise           | Select, Cancel, Confirm              | 0, 1, 2      |

## 6. Submenus

### 6.1 Game Play (`game.c`)

- **Resources.**
  - Layout: res 21.
  - Strings: res 22..27 (En = 23). Index 0 "Press START to begin!",
    1 error, 2 "The disc could not be read.", 3 "Please insert a NINTENDO
    GAMECUBE Game Disc.", 4 "Reading disc...".
- **Disc states** (`Disc_GetState`):

  | Value | Meaning               |
  |-------|-----------------------|
  | 0x10  | ready                 |
  | 0x11  | reading               |
  | 0x12  | no disc               |
  | 0x13  | cover open            |
  | 0x16  | wrong disc            |
  | 0x17  | fatal error           |
  | 0x18  | retry                 |

- **Status line.** Each status line has a fader:
  - "Press START" fades in when the state is 0x10.
  - "insert disc" fades in for 0x12 or 0x13.
  - "could not be read" fades in for 0x16 or 0x18.
  - "Reading" fades in for any other state.
  - State 0x17 latches `sFatal` and sets the scene fatal flag.
- **Input.** Input is accepted only after `GameIcon_IsSettled` and
  `Fx_IsReady(0)`.
  - **B:** leaves (SE 6). Requires the zoom to be complete and no boot fade
    running.
  - **START** (with a ready disc): `Sound_StopAmbient`, SE 22, then
    `Disc_StartGame` every frame. Disc_StartGame fades the screen and boots.
- **Banner.** The banner is drawn when `Disc_HasBanner` is true:
  - `bana`: the 96x32 image.
  - `titl`: long name, BNR+0x1860.
  - `makr`: long maker, BNR+0x18A0.
  - `info`: description, BNR+0x18E0.

  BNR2 adds language * 0x140 to each offset.
- **Mosaic.** The mosaic shows the game title. Its colours are light
  (130, 50, 160) and dark (50, 20, 70).
- **Icon.** A ring of 16 cubes, radius 75, scale 0.65, tint (0, -90, 20).

### 6.2 Options (`options.c`)

- **Resources.**
  - Layout: res 122.
  - Strings: res 123..128, one per language (En = 124), all loaded at once.
    Strings: 0 "Choose NINTENDO GAMECUBE system settings", 1 "Sound",
    2 "Screen Position", 3 "Language".
- **Sub-states** (`sOptSub`). Input waits for `Fx_IsReady(0)`.

| Sub | Name    | Input                                                                 |
|-----|---------|-----------------------------------------------------------------------|
| 0   | menu    | Up/Down: row 0..2 (no wrap), SE 11. A: enter row, SE 8. B: back to the face, SE 6 |
| 1   | sound   | Left: mono, Right: stereo. SE 9 on change, SE 13 at the end (not on key repeat). A: if changed, `OSSetSoundMode` + `Sound_SetOutputMode`; SE 12. B: restore, SE 7 |
| 2   | offset  | Left/Right: -1/+1 in -32..32, applied live by `__VISetAdjustingValues(x, 0)`. SE 9, or SE 13 at the limit. A: write SRAM displayOffsetH, sync; SE 12. B: restore from SRAM and reapply; SE 7 |
| 3   | language| Left/Right: 0..5, `Menu_SetGuideLang` (the guide cross-fades). SE 9/13. A: `Lang_Set` (SRAM language; reloads every per-language resource); SE 12. B: restore; SE 7 |

- **Language list.** The language list on the right is panes `lan1..lan6`.
  The language being browsed has its fader at full, the others at half.
  Separators `sla1..sla5` are drawn at alpha 128.
- **Item texts.** The item texts are drawn in every language and
  cross-faded:
  - Colour (0, 255, 120).
  - Vertical offsets -15, -4 and 6 for rows 1-3.
  - The language list is offset by 8.
- **Highlight pulse.** A highlight value pulses between 50 and 120 at
  +-2 per frame.
- **Mosaic colours.** Three light/dark pairs: (180, 255, 0)/(20, 100, 50),
  (120, 220, 35)/(0, 80, 25), (40, 120, 0)/(0, 40, 10).
- **Icon tint.** (-30, 20, -50).

### 6.3 Calendar (`calendar.c`)

- **Resources.**
  - Layout: res 14.
  - Strings: res 15..20 (En = 16): "Set the date and time." / "Date" /
    "Time".
- **Sub-states:**

| Sub | Name  | Input |
|-----|-------|-------|
| 0   | menu  | Up: date, Down: time (SE 11 on change). A (SE 8): load the edit buffer from `OSGetTime`. The date buffer is year, mon, mday; the time buffer is hour, min, 0. B: back, SE 6 |
| 1   | date  | Left/Right (only without Up/Down): move the field cursor, wrapping, SE 10. Down/Up: value -1/+1, SE 9. A: commit, SE 12. B: SE 7 |
| 2   | time  | the same, with fields hour, minute, second |

- **Date limits.**
  - The year wraps within 2000..2099 and the month within 0..11.
  - The day is limited to the month length (normal 0x813814C0, leap
    0x813814F0). Leap year is (y%4==0 && y%100!=0) || y%400==0.
  - When the day goes past the end it wraps to 1 if the day is the field
    being edited; otherwise it is clamped. A day below 1 becomes the last
    day.
- **Time limits.** The hour wraps 0..23; minutes and seconds wrap 0..59.
- **Field cursor.** On PAL (language codes 4..9), Left increases the date
  field index, so the fields appear as day / month / year from left to
  right. The time fields always go hour, minute, second.
- **Commit** (`Clock_Commit`):
  1. Take the current calendar time and replace the date (or the time)
     fields. When the time is set, a year above 2099 becomes 2000.
  2. Convert it to ticks and call `__OSSetTime(t)`.
  3. Set SRAM counterBias = t / (busClock / 4) - RTC.
  4. Set SRAM flags |= 0x20, then sync.
- **Colours.** Text (255, 255, 0). Mosaic pairs (240, 255, 0)/(100, 50, 20),
  (235, 185, 0)/(70, 40, 0), (160, 90, 0)/(50, 40, 0).
- **Icon.** A clock made of cubes. The hands follow the time, with lengths
  0xFB0/0x1240/0x1740 and 0xFF0/0x1240/0x15D0. Tints (40, 20, -50) and
  (80, 0, -60).

### 6.4 Memory Card (`memcard*.c`, summary)

**Resources.**

- Layout: res 28.
- Strings: res 29..34 (En = 30), 47 strings. The string index is also the
  index of the message fader at `gCardUi + 0xA6C + 16*i`:

  | Index | Strings                                                        |
  |-------|----------------------------------------------------------------|
  | 0-1   | "Open" (free) for each slot                                    |
  | 2-4   | Move, Copy, Erase                                              |
  | 5-6   | Yes, No                                                        |
  | 7-18  | move results                                                   |
  | 19-30 | copy results                                                   |
  | 31-32 | erase results                                                  |
  | 33-38 | slot problems                                                  |
  | 39-46 | format prompts and results                                     |

- Fill-bar texture: res 90 (110x14).
- 3D models: res 87 and 88.

**UI state** (`sCardUiState`, 0x814AD470; the next state is at 0x814AD472):

| State | Name    | Behaviour |
|-------|---------|-----------|
| 1     | browse  | Move over a 4-wide grid of file icons in the current slot, with Left/Right switching slots at the edges (SE 11, SE 13 at the limits). A opens the operation menu (SE 8). B leaves (SE 6) |
| 2     | op menu | Left/Right selects Move / Copy / Erase (SE 10). A: SE 8, or SE 13 when the operation is not allowed (`Card_CheckCopy` / `Card_CheckMove`: other card missing or not ready, file has no copy/move permission). B: SE 7 |
| 3     | confirm | Yes/No with Up/Down (SE 9). Yes posts the command to the card thread: Move = cmd 3, Copy = cmd 2, Erase = cmd 4 (SE 8). Move and Copy need the other slot ready; otherwise error -0x17 "no card in the other slot" |
| 4     | format  | Shown for a slot in state 6 (broken) or 8 (wrong encoding). Yes/No; A on Yes asks a second time; the second Yes posts cmd 1. No or B marks the slot 7/9 ("asked") so the prompt is not repeated |
| 5     | message | Waits for the result and plays the result SE. A or B returns to browse (SE 7) |

**Sounds.**

| SE    | When                                                   |
|-------|--------------------------------------------------------|
| 15    | erase done                                             |
| 16    | copy done, or format done                              |
| 17/18 | transfer started, from slot A / slot B (10 frames after the request) |
| 19    | copy finished, when the list refreshes                 |
| 20/21 | move finished, when the list refreshes (slot A / slot B) |
| 13    | failure                                                |

**Card thread** (0x8131C9F8, priority 17, 32 KB stack).

- Started by `CardThread_Init`.
- It receives commands `cmd | slot<<16 | file<<24` from a message queue
  and replies with `cmd | result<<8 | slot<<16 | file<<24`.

| Cmd | Action                                   |
|-----|------------------------------------------|
| 0   | mount and read the directory             |
| 1   | format                                   |
| 2   | copy                                     |
| 3   | move (copy, then delete the source)      |
| 4   | erase                                    |
| 9   | unmount                                  |
| 10  | enable                                   |
| 11  | disable                                  |
| 12  | sync                                     |

- `CardThread_Poll` runs every frame from the menu. It probes both slots
  with `CARDProbe` and posts mount (0) or unmount (9) as cards come and go.
- **Slot status.** `gCardSlots` (0x814B7280) has 2 x 0x14 bytes per slot.
  The state field is +2:

  | State | Meaning                     |
  |-------|-----------------------------|
  | 0     | empty                       |
  | 3     | mounting                    |
  | 4     | ready                       |
  | 5     | wrong device                |
  | 6     | broken                      |
  | 8     | encoding                    |
  | 10    | other                       |

- **Files.** The directory starts at 0x814B72A8: 127 entries of 0xC bytes
  per slot (0x5F4 per slot).
- **Error mapping.** CARD results map to UI errors (`Card_MapError`):

  | CARD result                         | UI error |
  |-------------------------------------|----------|
  | 0                                   | 0        |
  | EXIST                               | -0x18    |
  | NOENT / INSSPACE                    | -0x19    |
  | -27 (different card type)           | -0x1B    |
  | anything else                       | -0x1C    |

  -0x15 means "busy / no result yet".

**Icon.** Behind the browser is a 6x6 grid of cubes spaced 20 apart; the
inner 12 are hidden. The cubes flip in as files appear. Tint is
(-45, -40, 50).

## 7. Mosaic text ("fx", `menu_fx_*.c`)

- **What it does.** The titles and rows inside Game Play, Options and
  Calendar are built from small cubes ("mosaic"). The cubes fly to glyph
  shapes and fly away again.
- **Data.**
  - Shapes are stored per language in res 179..184 (`Res_FxGlyphs`).
  - Keyframes are in res 37.
  - The cube pool is at 0x814DE638 (0xA798 bytes); the cube count is at
    pool+0xA794.
- **Per-frame flow.**
  1. Each submenu update calls its builder: `FxOption_Update(sub, row,
     offset, sound)`, `FxClock_Update(sub, row, dateField, timeField,
     edit)` or `FxGame_Update(starting, discMsg)`.
  2. The builder lays out text lines from the `fxTitle` / `fxItem` /
     `fxSel` parameters in MenuWork.
  3. `Fx_Update` then animates every cube toward its target.
  4. `Fx_Draw(alpha)` draws them with the small cube model.
- **Readiness.** `Fx_IsReady(margin)` is true once at least 58 - margin
  cubes have arrived. Submenu input waits for it.
- **Fully reimplementing it** requires the shape tables. A reimplementation
  can treat the mosaic as "text drawn with cubes" that fades in over about
  60 frames.

## 8. Sound effects

`Sound_Play(id)` is at 0x8135AA80.

- Ids 0..2 are boot jingles.
- Id 23 is the spin-out cycle.
- Ids 3..22 go to the SE port.

| SE | Use |
|----|-----|
| 3  | cube turns to a face |
| 4  | cube turns back to the front |
| 5  | enter a submenu (A) |
| 6  | leave a submenu (B) |
| 7  | cancel |
| 8  | decide |
| 9  | value change |
| 10 | field cursor (calendar), op-menu cursor (card) |
| 11 | list cursor |
| 12 | value stored |
| 13 | buzzer (not allowed / limit; suppressed on key repeat) |
| 15-21 | memory card results (see 6.4) |
| 22 | START in Game Play |

Ambient music is stopped with `Sound_StopAmbient` (0x8135ABE0) when the game
boots, and restarted with `Sound_StartAmbient(0)` if the boot is aborted.

## 9. Resources by id

Per-language resources are listed in the order En, De, Fr, Es, It, Nl. The
language index is SRAM language 0..5; `Lang_GetCode()` returns index + 4.

| What                         | Ids (En, De, Fr, Es, It, Nl) | Type |
|------------------------------|------------------------------|------|
| Front face layout            | 57, 149, 142, 163, 156, 132  | GLH0 |
| Front face labels            | 58 (En); 133/143/150/157/164 | STH0 |
| Game face layout             | 56, 148, 141, 162, 155, 131  | GLH0 |
| Card face layout             | 59, 151, 144, 165, 158, 134  | GLH0 |
| Calendar face layout         | 136, 146, 139, 160, 153, 129 | GLH0 |
| Options face layout          | 138, 152, 145, 166, 159, 135 | GLH0 |
| Card strings (47)            | 30, 32, 31, 34, 33, 29       | STH0 |
| Game strings (5)             | 23, 25, 24, 27, 26, 22       | STH0 |
| Calendar strings (3)         | 16, 18, 17, 20, 19, 15       | STH0 |
| Dialog strings (18)          | 110, 112, 111, 114, 113, 109 | STH0 |
| Guide strings (22)           | 117, 119, 118, 121, 120, 116 | STH0 |
| Option strings (4)           | 124, 126, 125, 128, 127, 123 | STH0 |
| Weekday textures (+0..6)     | 49, 71, 62, 171, 80, 40      | I4 48x24 |
| "Mono" texture               | 47, 69, 60, 169, 78, 38      | I4 120x24 |
| "Stereo" texture             | 48, 70, 61, 170, 79, 39      | I4 120x24 |
| Mosaic shapes                | 180, 182, 181, 184, 183, 179 | Yay0 |

| Fixed resource            | Id  | Type |
|---------------------------|-----|------|
| Calendar layout           | 14  | GLH0 |
| Game Play layout          | 21  | GLH0 |
| Memory Card layout        | 28  | GLH0 |
| Mosaic keyframes          | 37  | Yay0 |
| Card browser models       | 87, 88 | Yay0 BMD |
| Backdrop tile             | 89  | I4 8x8 |
| Card fill bar             | 90  | I4 110x14 |
| Digits 0-9                | 91..100 | I4 16x22 |
| Dialog layout             | 108 | GLH0 |
| Button guide layout       | 115 | GLH0 |
| Options layout            | 122 | GLH0 |
| Menu texture bank         | 167 | Yay0 TXH0 |
| Small cube model (icons)  | 178 | Yay0 BMD |

## 10. Uncertain points

- **SRAM flag 0x08** is set when the SRAM was lost at boot. Its exact
  meaning is not known; it is probably "settings were reset".
- **Mosaic layout parameters** (`MenuWork` 0x34..0x17E): the fields are
  named by use only.
- **Memory card grid navigation** is summarised from the code shape; the
  exact cursor wrapping rules were not traced.
- **Calendar field order** depends on `Lang_GetCode`; only PAL codes were
  checked.
- **`__VISetAdjustingValues`:** names.txt calls 0x8136A504 `VIConfigure`.
  The menu calls it with (offsetX, 0), which matches
  `__VISetAdjustingValues`.

## 11. Function index

Functions in 0x81310F58..0x813376A0, grouped by source file. Unnamed small
mosaic helpers are left out (432 functions in total, 207 named).

**menu.c**

| Address | Name | Insns |
|---|---|---|
| 81310F58 | Menu_LoadLangRes | 48 |
| 81311014 | Menu_InitRes | 198 |
| 81311314 | Menu_Init | 56 |
| 813113EC | Menu_SetPalTextLayout | 30 |
| 81311464 | Menu_SetDefaults | 252 |
| 81311850 | Menu_ResetCube | 6 |
| 81311868 | Menu_HandleInput | 360 |
| 81311D50 | Menu_CalcCubeMtx | 190 |
| 81312044 | Menu_UpdateDiscFaders | 46 |
| 813120F0 | Menu_UpdateRotation | 193 |
| 813123B0 | Menu_UpdateIcons | 63 |
| 8131247C | Menu_UpdateSlotGlow | 73 |
| 81312580 | Menu_UpdateGuide | 341 |
| 81312A94 | Menu_UpdateSubFaders | 75 |
| 81312B9C | Menu_Update | 263 |
| 81312F68 | Menu_GetCubeMtx | 3 |
| 81312F74 | Menu_Get | 2 |
| 81312F7C | Menu_SetGuideLang | 3 |

**menu_draw.c**

| Address | Name | Insns |
|---|---|---|
| 81312F88 | Menu_DrawSpinOutFace | 81 |
| 813130CC | Menu_DrawBannerInfo | 156 |
| 81313334 | Menu_DrawGameFace | 152 |
| 8131358C | Menu_DrawSlot | 99 |
| 81313708 | Menu_DrawClockFace | 518 |
| 81313F20 | Menu_DrawOptionFace | 262 |
| 81314304 | Menu_DrawFaces | 652 |
| 81314CC8 | Menu_CardFadeDone | 45 |
| 81314D70 | Menu_DrawSubmenus | 125 |
| 81314F44 | Menu_DrawGuideText | 83 |
| 8131508C | Menu_DrawGuide | 387 |
| 8131567C | Menu_Draw2D | 43 |

**game.c**

| Address | Name | Insns |
|---|---|---|
| 81315724 | Game_InitParams | 19 |
| 8131576C | GameParams_SetDefaults | 17 |
| 813157B0 | Game_GetParams | 2 |

**memcard.c**

| Address | Name | Insns |
|---|---|---|
| 813157B8 | Card_InitParams | 19 |
| 81315800 | CardParams_SetDefaults | 77 |
| 81315934 | Card_GetParams | 2 |
| 8131593C | CardParams_SetColors | 170 |
| 81315BA4 | Card_LoadLangRes | 10 |
| 81315BCC | Card_Init | 274 |
| 81315FDC | Card_CheckCopy | 151 |
| 813161F8 | Card_CheckMove | 151 |
| 81316414 | CardUi_Browse_Enter | 64 |
| 813164FC | CardUi_Browse | 462 |
| 81316BAC | CardUi_OpMenu | 150 |
| 81316DDC | CardUi_Confirm | 244 |
| 81317150 | CardUi_Format | 186 |
| 813173EC | CardUi_Message | 82 |
| 81317514 | CardFile_Compare | 24 |
| 81317564 | Card_SortFiles | 89 |
| 813176AC | CardUi_OnSlotRefresh | 114 |
| 8131783C | CardUi_Reset | 11 |
| 81317868 | CardUi_PollOpResult | 110 |
| 813179E0 | CardUi_PollSlot | 113 |
| 81317B80 | CardUi_Update | 66 |
| 81317C6C | CardUi_UpdateOpFaders | 75 |
| 81317D88 | CardUi_UpdateFormatMsg | 119 |
| 81317F40 | CardUi_UpdateFaders | 534 |
| 813186F4 | CardUi_UpdateIconAnim | 97 |
| 81318858 | CardUi_CopyResultMsg | 157 |
| 81318A88 | CardUi_MoveResultMsg | 157 |
| 81318CB8 | CardUi_UpdateResultMsg | 125 |
| 81318E80 | CardUi_UpdateCursor | 71 |
| 81318F80 | Card_Update | 87 |
| 813190C8 | Card_GetSelectedFile | 21 |
| 8131911C | Card_GetSlot | 2 |
| 81319124 | Card_Enter | 71 |
| 81319240 | CardDraw_SetupTev | 128 |
| 81319440 | CardDraw_LoadIconTex | 87 |
| 8131959C | CardDraw_LoadTexture | 182 |
| 81319864 | CardDraw_SetIconFrame | 86 |
| 813199AC | CardDraw_IconFrameCount | 62 |
| 81319A8C | CardDraw_Icon | 79 |
| 81319BC4 | CardDraw_SlotHeader | 109 |
| 81319D70 | CardDraw_FileBanner | 152 |
| 81319FC8 | CardDraw_FileGrid | 98 |
| 8131A13C | CardDraw_OpMenu | 338 |
| 8131A644 | (unnamed) | 17 |
| 8131A688 | CardUi_StepSlotFader | 83 |
| 8131A7B4 | CardDraw_SetScissor | 87 |
| 8131A90C | CardDraw_MeasureLines | 96 |
| 8131AA74 | CardDraw_TextBox | 272 |
| 8131AE80 | CardDraw_TextLines | 106 |
| 8131B018 | CardDraw_Dialog | 247 |
| 8131B3CC | CardDraw_LoadFileIcon | 109 |
| 8131B56C | CardDraw_FileInfo | 94 |
| 8131B6E0 | CardDraw_Messages | 155 |
| 8131B94C | Card_Draw | 389 |

**memcard_card.c**

| Address | Name | Insns |
|---|---|---|
| 8131BF50 | Card_IsBrowsing | 8 |
| 8131BF6C | CardThread_GetFiles | 3 |
| 8131BF78 | CardThread_GetSlots | 3 |
| 8131BF84 | CardThread_IsEnabled | 2 |
| 8131BF8C | CardThread_GetCmd | 2 |
| 8131BF94 | CardThread_GetResult | 2 |
| 8131BF9C | CardThread_GetDestResult | 2 |
| 8131BFA4 | CardThread_GetDestFile | 2 |
| 8131BFAC | CardThread_Format | 21 |
| 8131C000 | CardThread_Copy | 23 |
| 8131C05C | CardThread_Move | 23 |
| 8131C0B8 | CardThread_Erase | 23 |
| 8131C114 | CardThread_Enable | 11 |
| 8131C140 | CardThread_UnmountAll | 35 |
| 8131C1CC | Card_MapError | 40 |
| 8131C268 | CardThread_Poll | 227 |
| 8131C5C4 | CardThread_Init | 222 |
| 8131C910 | CardThread_CmdQueue | 3 |
| 8131C91C | CardThread_ReplyQueue | 3 |
| 8131C928 | CardThread_GetIcons | 3 |
| 8131C934 | CardThread_GetBanner | 10 |
| 8131C95C | CardFile_Clear | 39 |
| 8131C9F8 | CardThread_Main | 98 |
| 8131CB64 | (unnamed) | 46 |
| 8131CC10 | CardThread_SetEnabled | 14 |
| 8131CC48 | (unnamed) | 79 |
| 8131CD74 | (unnamed) | 39 |
| 8131CE08 | (unnamed) | 87 |
| 8131CF54 | (unnamed) | 225 |
| 8131D268 | (unnamed) | 110 |
| 8131D408 | (unnamed) | 86 |
| 8131D548 | CardThread_ReadEntry | 83 |
| 8131D670 | CardThread_DoUnmount | 37 |
| 8131D6FC | CardThread_DoMount | 281 |
| 8131DAE8 | (unnamed) | 3 |
| 8131DAF4 | CardThread_CheckDest | 109 |
| 8131DC8C | CardThread_CopyFile | 74 |
| 8131DD9C | CardThread_MoveFile | 177 |
| 8131E044 | (unnamed) | 64 |
| 8131E130 | CardThread_DoCopyMove | 585 |
| 8131E994 | CardThread_DoFormat | 98 |
| 8131EB04 | CardThread_DoErase | 118 |

**memcard_draw.c**

| Address | Name | Insns |
|---|---|---|
| 8131ECC4 | (unnamed) | 68 |
| 8131EDBC | (unnamed) | 66 |
| 8131EEC4 | (unnamed) | 103 |
| 8131F048 | (unnamed) | 466 |
| 8131F770 | (unnamed) | 101 |
| 8131F8F8 | (unnamed) | 510 |
| 8132005C | (unnamed) | 7 |
| 81320078 | (unnamed) | 7 |
| 81320094 | (unnamed) | 163 |
| 8132030C | (unnamed) | 110 |
| 813204B0 | (unnamed) | 49 |
| 8132056C | (unnamed) | 157 |
| 813207C8 | CardIcon_ResetFiles | 88 |
| 81320920 | (unnamed) | 107 |
| 81320ABC | (unnamed) | 213 |
| 81320DD0 | (unnamed) | 168 |
| 81320FD8 | (unnamed) | 103 |
| 81321168 | (unnamed) | 76 |
| 8132128C | (unnamed) | 60 |
| 81321378 | (unnamed) | 238 |
| 813216F4 | (unnamed) | 8 |
| 81321714 | (unnamed) | 8 |

**options.c**

| Address | Name | Insns |
|---|---|---|
| 81321730 | Option_InitParams | 19 |
| 81321778 | OptionParams_SetDefaults | 28 |
| 813217E8 | Option_GetParams | 2 |

**calendar.c**

| Address | Name | Insns |
|---|---|---|
| 813217F0 | Clock_InitParams | 19 |
| 81321838 | ClockParams_SetDefaults | 34 |
| 813218C0 | Clock_GetParams | 2 |

**menu_icon_card.c**

| Address | Name | Insns |
|---|---|---|
| 813218C8 | CardIcon_Init | 85 |
| 813219F4 | CardIcon_InitGrid | 116 |
| 81321B98 | (unnamed) | 23 |
| 81321BE4 | (unnamed) | 34 |
| 81321C68 | (unnamed) | 205 |
| 81321F60 | (unnamed) | 362 |
| 81322434 | (unnamed) | 141 |
| 81322650 | CardIcon_UpdateFace | 269 |
| 81322A18 | CardIcon_CalcMtx | 166 |
| 81322C8C | CardIcon_Draw | 54 |
| 81322D58 | CardIcon_UpdateHide | 110 |
| 81322EE8 | CardIcon_UpdateEnter | 147 |
| 813230E8 | (unnamed) | 2 |

**menu_icon_game.c**

| Address | Name | Insns |
|---|---|---|
| 813230F0 | GameIcon_Init | 71 |
| 813231F8 | GameIcon_InitRing | 118 |
| 813233C8 | GameIcon_UpdateFace | 141 |
| 813235D4 | GameIcon_CalcMtx | 92 |
| 8132373C | GameIcon_Draw | 84 |
| 81323868 | GameIcon_UpdateHide | 104 |
| 813239E4 | GameIcon_UpdateEnter | 132 |
| 81323BC0 | GameIcon_IsSettled | 33 |

**menu_icon_option.c**

| Address | Name | Insns |
|---|---|---|
| 81323C30 | OptionIcon_Init | 142 |
| 81323E20 | OptionIcon_ResetShape | 157 |
| 8132406C | (unnamed) | 13 |
| 81324098 | (unnamed) | 125 |
| 81324264 | (unnamed) | 1 |
| 81324268 | (unnamed) | 473 |
| 81324938 | (unnamed) | 202 |
| 81324C20 | (unnamed) | 1 |
| 81324C24 | (unnamed) | 195 |
| 81324EEC | (unnamed) | 156 |
| 81325140 | (unnamed) | 84 |
| 8132527C | (unnamed) | 197 |
| 8132554C | OptionIcon_UpdateFace | 282 |
| 81325960 | OptionIcon_CalcMtx | 126 |
| 81325B48 | OptionIcon_Draw | 62 |
| 81325C30 | OptionIcon_UpdateHide | 112 |
| 81325DB0 | OptionIcon_UpdateEnter | 243 |
| 8132610C | OptionIcon_IsSettled | 2 |

**menu_icon_clock.c**

| Address | Name | Insns |
|---|---|---|
| 81326114 | ClockIcon_Init | 84 |
| 81326248 | ClockIcon_InitHands | 39 |
| 813262E4 | ClockIcon_UpdateFace | 352 |
| 81326808 | ClockIcon_CalcMtx | 312 |
| 81326CA4 | ClockIcon_Draw | 85 |
| 81326DE0 | ClockIcon_UpdateHide | 140 |
| 81326FDC | ClockIcon_UpdateEnter | 419 |
| 813275C4 | ClockIcon_IsSettled | 2 |

**game.c**

| Address | Name | Insns |
|---|---|---|
| 813275CC | Game_LoadLangRes | 10 |
| 813275F4 | Game_Init | 47 |
| 813276AC | Game_HandleInput | 70 |
| 813277A8 | Game_UpdateMessages | 107 |
| 81327920 | Game_Update | 71 |
| 81327A10 | Game_Enter | 46 |
| 81327AB4 | Game_IsFatal | 2 |
| 81327ABC | Game_DrawBanner | 134 |
| 81327CCC | Game_Draw | 135 |

**options.c**

| Address | Name | Insns |
|---|---|---|
| 81327EE4 | Option_LoadLangRes | 9 |
| 81327F08 | Option_Init | 140 |
| 81328104 | Option_UpdateMenu | 105 |
| 8132827C | Option_UpdateSound | 82 |
| 813283A0 | Option_UpdateOffset | 101 |
| 81328510 | Option_UpdateLang | 86 |
| 81328648 | Option_Update | 113 |
| 813287D4 | Option_IsTop | 8 |
| 813287F0 | Option_GetLangRow | 7 |
| 8132880C | Option_GetLang | 2 |
| 81328814 | Option_DrawText | 197 |
| 81328B20 | Option_Draw | 98 |

**calendar.c**

| Address | Name | Insns |
|---|---|---|
| 81328CA4 | Clock_Commit | 81 |
| 81328DD4 | Clock_LoadLangRes | 10 |
| 81328DFC | Clock_Init | 26 |
| 81328E60 | Clock_UpdateMenu | 108 |
| 81328FE4 | Clock_FixDate | 92 |
| 8132912C | Clock_UpdateDate | 149 |
| 81329338 | Clock_FixTime | 58 |
| 81329404 | Clock_UpdateTime | 107 |
| 81329590 | Clock_Update | 68 |
| 81329680 | Clock_IsTop | 8 |
| 8132969C | Clock_Draw | 126 |

**menu_fx_option.c**

| Address | Name | Insns |
|---|---|---|
| 81329890 | Fx_SetShapes | 8 |
| 813298B0 | Fx_EvalShape | 164 |
| 81329B34 | Fx_GetCubes | 3 |
| 81329B40 | Fx_GetDrawBuf | 3 |
| 81329B4C | Fx_GetCount | 2 |
| 81329B6C | Fx_IsReady | 24 |
| 81329BC4 | Fx_Init | 53 |
| 81329C98 | Fx_Clear | 19 |
| 81329CE4 | FxOption_Update | 292 |
| 8132A14C | FxClock_Update | 165 |
| 8132A3D4 | FxGame_Update | 182 |

**menu_fx_build.c**

| Address | Name | Insns |
|---|---|---|
| 8132D10C | FxOption_SetState | 34 |
| 8132DD84 | FxOption_ChangeLang | 76 |
| 8132DE94 | FxOption_ChangeUiLang | 1 |

**menu_fx_anim.c**

| Address | Name | Insns |
|---|---|---|
| 81332FB8 | Fx_InitCubeModel | 31 |

**menu_fx_draw.c**

| Address | Name | Insns |
|---|---|---|
| 81333030 | Fx_Draw | 142 |
| 81333250 | Fx_Update | 63 |
| 81333898 | Fx_EmitSparks | 259 |
