//! The mosaic glyph tables (`menu_fx_anim.c`, PAL 1.0 BS2
//! 0x81330F30..0x81332FB8). Each language's table (res 179..184, read raw
//! as [`Glyphs`](crate::ipl::fx::Glyphs)) lists the cubes of every word and
//! digit slot; BS2 has one pair of accessors per entry (the cells and the
//! cell count, for the menu language), here one constant per entry, named
//! after what it is, with the accessor in its doc. Entries 0..26 index cubes
//! of the Calendar shape, 27..29 Game Play's, 40..55 Options'; the digit
//! entries 30..39 index the 28 cells of a digit slot.

/// fn_813311F4: the Calendar time row.
pub const TIME_ROW: usize = 0x00;
/// fn_81331294: both slashes and both colons.
pub const SEPARATORS: usize = 0x01;
/// fn_81331338, fn_813313DC, fn_81331480, fn_81331524: the separators one
/// by one (sparks).
pub const SEPARATOR: [usize; 4] = [0x02, 0x03, 0x04, 0x05];
/// fn_813315C8: the Calendar date row.
pub const DATE_ROW: usize = 0x06;
/// fn_8133166C
pub const DAY: usize = 0x07;
/// fn_81331710, fn_813317B4
pub const DAY_DIGITS: [usize; 2] = [0x08, 0x09];
/// fn_81331858
pub const HOUR: usize = 0x0A;
/// fn_813318FC, fn_813319A0
pub const HOUR_DIGITS: [usize; 2] = [0x0B, 0x0C];
/// fn_81331A44
pub const MINUTE: usize = 0x0D;
/// fn_81331AE8, fn_81331B8C
pub const MINUTE_DIGITS: [usize; 2] = [0x0E, 0x0F];
/// fn_81331C30
pub const MONTH: usize = 0x10;
/// fn_81331CD4, fn_81331D78
pub const MONTH_DIGITS: [usize; 2] = [0x11, 0x12];
/// fn_81331E1C
pub const SECOND: usize = 0x13;
/// fn_81331EC0, fn_81331F64
pub const SECOND_DIGITS: [usize; 2] = [0x14, 0x15];
/// fn_81332008
pub const YEAR: usize = 0x16;
/// fn_813320AC, fn_81332150, fn_813321F4, fn_81332298
pub const YEAR_DIGITS: [usize; 4] = [0x17, 0x18, 0x19, 0x1A];
/// fn_8133233C: Game Play's "no disc".
pub const NO_DISC: usize = 0x1B;
/// fn_813323E0: Game Play's "press start".
pub const PRESS_START: usize = 0x1C;
/// fn_81332484: Game Play's unknown disc.
pub const UNKNOWN_DISC: usize = 0x1D;
/// fn_81332528: the Options screen-position diamond (sparks).
pub const OFFSET_DIAMOND: usize = 0x28;
/// fn_813325CC, fn_81332670: the arrow shown for a negative or positive
/// offset.
pub const OFFSET_MINUS: usize = 0x29;
pub const OFFSET_PLUS: usize = 0x2A;
/// fn_81331008: the language list.
pub const LANG_LIST: usize = 0x2B;
/// fn_81332714 (fn_81330F88 for another language): "mono".
pub const MONO: usize = 0x2C;
/// fn_813327B8: the screen-position line.
pub const OFFSET_ROW: usize = 0x2D;
/// fn_8133285C, fn_81332900: its two digit slots.
pub const OFFSET_DIGITS: [usize; 2] = [0x2E, 0x2F];
/// fn_813329A4, fn_81332A48, fn_81332AEC, fn_81332B90, fn_813310AC,
/// fn_81331150: the left and right arrow bars of the sound, screen-position
/// and language lines (fn_8132EEB0's six lists).
pub const BARS: [usize; 6] = [0x30, 0x31, 0x32, 0x33, 0x34, 0x35];
/// fn_81332C34: the sound line.
pub const SOUND_ROW: usize = 0x36;
/// fn_81332CD8 (fn_81330FC8 for another language): "stereo".
pub const STEREO: usize = 0x37;

/// fn_81332D7C: the cells of digit `d` in a digit slot.
pub fn digit(d: i16) -> usize {
    (0x1E + d as isize) as usize
}
