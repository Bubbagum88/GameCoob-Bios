//! PC stand-ins for the parts of the Dolphin SDK that BS2 calls, so the
//! decompiled units in [`crate::bs2`] can be ported call for call.
//!
//! * [`mtx`]: `PSMTX*` / `C_MTX*` matrix math.
//! * [`gx`]: GX state and immediate-mode drawing on the CPU rasterizer.
//!
//! VI, OS, PAD, DVD and CARD follow as their callers are ported (see
//! docs/PORT.md).

pub mod gx;
pub mod mtx;
