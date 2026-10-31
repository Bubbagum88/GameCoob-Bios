//! Mosaic side columns (`menu_fx_morph.c`, PAL 1.0 BS2
//! 0x81333344..0x8133370C): while the Options screen position is being
//! edited, two columns of cubes (shapes 3 and 4) fly in beside it, and out
//! again after.

use super::{shape, FxCube, State, P};
use crate::ipl::fx::Shapes;
use alloc::vec;
use alloc::vec::Vec;

pub struct Morph {
    /// 0 none, 1 flying in, 2 flying out.
    state: i16,
    frame: u16,
}

impl Morph {
    pub fn new() -> Morph {
        Morph { state: 0, frame: 0 }
    }

    /// fn_81333344
    pub fn reset(&mut self) {
        self.state = 0;
        self.frame = 0;
    }

    /// fn_81333384: count on, and start flying in or out when the Options
    /// sub-state changes.
    pub fn update(&mut self, cur: &State, prev: &State) {
        self.frame = self.frame.wrapping_add(1);
        match self.state {
            1 => {
                if self.frame as i32 >= P.x124 as i32 {
                    self.frame = P.x124 as u16;
                }
            }
            2 => {
                if self.frame as i32 >= P.x126 as i32 {
                    self.frame = 0;
                    self.state = 0;
                }
            }
            _ => {}
        }
        if cur.v[2] != prev.v[2] {
            match cur.v[2] {
                2 => {
                    self.state = 1;
                    self.frame = 0;
                }
                0 => {
                    if prev.v[2] == 2 {
                        self.state = 2;
                        self.frame = 0;
                    } else {
                        self.state = 0;
                    }
                }
                _ => self.state = 0,
            }
        }
    }

    /// fn_813334AC, fn_813335AC and fn_81333614: the columns' cubes, how
    /// many there are and their alpha.
    pub fn cubes(&self, s: &Shapes) -> (Vec<FxCube>, usize, u8) {
        let mut out = vec![FxCube::default(); 100];
        let f = self.frame as f32;
        match self.state {
            1 => {
                shape::eval(s, 3, &mut out, f / P.x124 as f32);
                let n = s.count(3) as usize;
                (out, n, (255.0 * f / P.x124 as f32) as i32 as u8)
            }
            2 => {
                shape::eval(s, 4, &mut out, 1.0 - f / P.x126 as f32);
                let n = s.count(4) as usize;
                (out, n, (255.0 - 255.0 * f / P.x126 as f32) as i32 as u8)
            }
            _ => (out, 0, 0),
        }
    }
}
