//! PC stand-ins for the parts of the Dolphin SDK that BS2 calls, so the
//! decompiled units in [`crate::bs2`] can be ported call for call.
//!
//! * [`mtx`]: `PSMTX*` / `C_MTX*` matrix math.
//!
//! GX, VI, OS, PAD, DVD and CARD follow as their callers are ported (see
//! docs/PORT.md).

pub mod mtx;
