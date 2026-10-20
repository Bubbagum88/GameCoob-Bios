# BS2 boot intro (PAL 1.0)

Everything below was read from the decompiled code (`src/bs2/cube.c`,
`src/bs2/scene.c`). All timings are in logic frames (one per VI retrace).

## Scene setup
* Camera: `C_MTXLookAt` eye `(0, -55, 224 / tan(10°))`, target `(0, -55, 0)`, up `+Y`.
* Projection: orthographic, top 224, bottom -224, left -296, right 292,
  near 50, far 10000 (perspective fovy 20, aspect 1.3214 only for the spin-out).
* World matrix: `RotY(spin) · Shake · Wobble · Iso · T(15,15,15)`, then
  squash/stretch `(1-(1-0.5)k, 1+1.0k, 1-(1-0.5)k)` with `k = spinSpeed/5000`.
  * `Iso = RotTrans(0x2000, -0x1914, -0x1555)` (u16 angles, 0x10000 = 360°, applied Z·Y·X).
  * `Wobble`: each axis `1000f + (1-f)·amp·sin(phase·{0x5A,0x3C,0x46})`, `f = 0.7·spinSpeed/5000`.
    `amp += 36` per frame while A accelerates the spin, else `×0.945`; phase `+= 7`.
  * `Shake = RotTrans(-drop·s, 0, land·s; ty = 0.015·land·s + 0.005·drop·s)`, `s = sin(shakePhase)`.
    Landing impulse 4000 (×0.93/frame, cut below 10), drop impulse 12000
    (×0.925/frame, cut below 80); phase starts at 0x7FFF and advances 1000/frame.

## Models (resource ids in the IPL archive)
| id | use |
|----|-----|
| 0x23 | inner background (alpha follows the impact shake) |
| 0x24 | outer background, two draw nodes (+0x30 before the cube, +0x78 after the logo), scale 1.01 |
| 0x65 | I8 64×64 soft tile texture for the trail |
| 0x66 | the cube logo with the "G" cut out |
| 0x6B + 0x69 | "GAMECUBE" wordmark + its 60-frame joint animation |
| 0xA8 | the small rolling cube (edge 54) |

## Cube timeline
1. **Fall** (4 frames + 13 hold): from height 100 with constant acceleration
   `2h/n²`. Impact starts the landing shake.
2. **Path**: steps from the table at `0x81381160` (dir, group, life):
   `0,0,3,3,5,0,0,3,3,6,0,0,3,3,2,2,7`. A flat roll is 90° in 8 frames
   (+1 frame to register the step), an edge roll onto the next face is 180°
   in 15 frames. Each step leaves a tile on the 32×32 grid of the current
   face; tiles fade in over a side roll, fade out over a side roll when their
   life (in steps) runs out. The first tile lives 5 steps, the rest forever.
   Start cell (16, 18), face 0.
3. **Rise** (15 + 6 frames): scale 1→1.5, 6 quarter turns about X, height
   `130·(1 - r⁴/n⁴)`.
4. **Drop** (8 + 17 frames): from 130 with initial speed 30 and constant
   acceleration `2h/n² - v0/n`. Impact starts the drop shake; the wordmark
   animation clock starts.
5. **Logo**: logo alpha 0→255 over 33 frames, trail and small cube fade by
   `255 - logoAlpha`. Wordmark BCK frame `60·(t-4)/50`. Done once the text
   finishes and the drop shake has died out.

## Boot jingle
The jingle starts when the cube is placed: sound 0 normally after frame 0,
sound 1 after 6 frames when Z is held on all four controllers, sound 2 after
5 frames when Z is held on controller 1.

## Spin to menu
Holding A while the cube is still on its path adds 1.5 to the spin
acceleration each frame. Once spin speed exceeds 5000 the scene switches to
the spin-out (state 4), which goes to perspective and opens the main menu.
Released, acceleration decays ×0.76 and speed ×0.95 and the spin eases back
over 50 frames (Hermite). A whoosh plays on each wrap when speed/5000 > 0.4.

## Draw order (state 1, no disc)
wordmark → inner bg → outer bg (front) → trail tiles + small cube → logo →
outer bg (back) → disc banners. Translucent models disable z-compare, so the
order above is what produces the layering.
