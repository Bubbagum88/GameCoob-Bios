//! IFR, the bytecode a firmware's setup forms are written in (UEFI spec,
//! "Forms Representation"): form sets, forms, the questions on them, the
//! variables they are stored in and the conditions that hide or grey them.
//!
//! Only what a settings browser needs is kept. Anything not understood is
//! skipped by its length, so an unknown opcode never derails the walk.

use super::pkg::{guid_at, u16le, Strings};
use alloc::string::String;
use alloc::vec::Vec;

pub const OP_FORM: u8 = 0x01;
pub const OP_SUBTITLE: u8 = 0x02;
pub const OP_TEXT: u8 = 0x03;
pub const OP_ONE_OF: u8 = 0x05;
pub const OP_CHECKBOX: u8 = 0x06;
pub const OP_NUMERIC: u8 = 0x07;
pub const OP_PASSWORD: u8 = 0x08;
pub const OP_ONE_OF_OPTION: u8 = 0x09;
pub const OP_SUPPRESS_IF: u8 = 0x0A;
pub const OP_ACTION: u8 = 0x0C;
pub const OP_RESET_BUTTON: u8 = 0x0D;
pub const OP_FORM_SET: u8 = 0x0E;
pub const OP_REF: u8 = 0x0F;
pub const OP_NO_SUBMIT_IF: u8 = 0x10;
pub const OP_INCONSISTENT_IF: u8 = 0x11;
pub const OP_GRAY_OUT_IF: u8 = 0x19;
pub const OP_DATE: u8 = 0x1A;
pub const OP_TIME: u8 = 0x1B;
pub const OP_STRING: u8 = 0x1C;
pub const OP_DISABLE_IF: u8 = 0x1E;
pub const OP_ORDERED_LIST: u8 = 0x23;
pub const OP_VARSTORE: u8 = 0x24;
pub const OP_VARSTORE_NAME_VALUE: u8 = 0x25;
pub const OP_VARSTORE_EFI: u8 = 0x26;
pub const OP_END: u8 = 0x29;
pub const OP_FORM_MAP: u8 = 0x5D;
pub const OP_GUID: u8 = 0x5F;
pub const OP_WARNING_IF: u8 = 0x63;

/// Question flags (EFI_IFR_FLAG_*).
pub const QF_READ_ONLY: u8 = 0x01;
/// The driver wants a callback when it changes ("interactive").
pub const QF_CALLBACK: u8 = 0x04;
pub const QF_RESET_REQUIRED: u8 = 0x10;

/// EDK II's bit-field varstore marker (questions inside it count bits,
/// not bytes): EDKII_IFR_BIT_VARSTORE_GUID.
const BIT_VARSTORE_GUID: [u8; 16] =
    [0x8b, 0xd6, 0xdd, 0x82, 0x63, 0x91, 0x87, 0x41, 0x9b, 0x27, 0x20, 0xa8, 0xfd, 0x60, 0xa7, 0x1d];

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum CondKind {
    /// Hidden while true.
    Suppress,
    /// Shown but locked while true.
    GrayOut,
    /// Hidden while true (evaluated once by real browsers).
    Disable,
}

#[derive(Clone, Debug)]
pub struct Cond {
    pub kind: CondKind,
    /// The expression's opcodes, in order (postfix).
    pub expr: Vec<u8>,
}

/// A variable store a form set's questions live in.
#[derive(Clone, Debug)]
pub struct VarStore {
    pub id: u16,
    pub guid: [u8; 16],
    pub name: String,
    pub size: u16,
    /// EFI_IFR_VARSTORE_EFI (a UEFI variable) rather than a driver's buffer.
    pub efi: bool,
    /// Name/value stores have no buffer: their questions are shown only.
    pub name_value: bool,
}

/// Where a question's value is.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Question {
    pub id: u16,
    /// Varstore id in its form set, 0 for none.
    pub vsid: u16,
    pub offset: u16,
    /// Bytes; 0 when the value is not a plain number.
    pub width: u8,
    pub flags: u8,
    /// Inside an EDK II bit-field store: offset and width are in bits.
    pub bits: bool,
}

#[derive(Clone, Debug)]
pub struct OneOfOption {
    pub text: u16,
    pub value: u64,
    pub default: bool,
    /// Conditions on this option alone (indexes into [`FormSet::conds`]).
    pub conds: Vec<u16>,
}

#[derive(Clone, Debug)]
pub enum Kind {
    Subtitle,
    /// Its second string.
    Text(u16),
    /// A link to a form, in this form set or (with a GUID) another.
    Ref { form: u16, formset: Option<[u8; 16]> },
    OneOf(Vec<OneOfOption>),
    Checkbox { default: bool },
    Numeric { min: u64, max: u64, step: u64, hex: bool },
    String,
    Password,
    Date,
    Time,
    Action,
    OrderedList,
    ResetButton,
}

#[derive(Clone, Debug)]
pub struct Item {
    pub prompt: u16,
    pub help: u16,
    pub kind: Kind,
    pub q: Option<Question>,
    /// Conditions in effect (indexes into [`FormSet::conds`]).
    pub conds: Vec<u16>,
}

#[derive(Clone, Debug)]
pub struct Form {
    pub id: u16,
    pub title: u16,
    pub items: Vec<Item>,
}

#[derive(Clone, Debug)]
pub struct FormSet {
    pub guid: [u8; 16],
    pub title: u16,
    pub help: u16,
    pub class_guids: Vec<[u8; 16]>,
    pub forms: Vec<Form>,
    pub varstores: Vec<VarStore>,
    pub conds: Vec<Cond>,
}

enum Scope {
    FormSet,
    Form,
    /// The question's form index, item index and how many conditions were
    /// in effect when it opened.
    Question(usize, usize, usize),
    Cond,
    Bits,
    Other,
}

/// Is `op` part of an expression?
fn is_expr(op: u8) -> bool {
    matches!(op, 0x12..=0x17 | 0x20..=0x22 | 0x2A..=0x2C | 0x2F..=0x59 | 0x64)
}

/// The expression starting at `at`: its bytes and where it ends.
fn expression(b: &[u8], mut at: usize) -> (Vec<u8>, usize) {
    let start = at;
    let mut depth = 0usize;
    while at + 2 <= b.len() {
        let (op, len, scope) = (b[at], usize::from(b[at + 1] & 0x7F), b[at + 1] & 0x80 != 0);
        if len < 2 || at + len > b.len() {
            break;
        }
        if op == OP_END {
            if depth == 0 {
                break;
            }
            depth -= 1;
        } else if !is_expr(op) && depth == 0 {
            break;
        } else if scope {
            depth += 1;
        }
        at += len;
    }
    (b[start..at].to_vec(), at)
}

fn sized(b: &[u8], at: usize, size: usize) -> u64 {
    let mut v = [0u8; 8];
    if let Some(s) = b.get(at..at + size) {
        v[..size].copy_from_slice(s);
    }
    u64::from_le_bytes(v)
}

fn ascii(b: &[u8]) -> String {
    b.iter().take_while(|&&c| c != 0).map(|&c| c as char).collect()
}

/// The form sets in one form package's IFR.
pub fn parse(b: &[u8]) -> Vec<FormSet> {
    let mut out = Vec::new();
    let mut fs: Option<FormSet> = None;
    let mut form: Option<usize> = None;
    let mut conds: Vec<u16> = Vec::new();
    let mut stack: Vec<Scope> = Vec::new();
    let mut bits = 0usize;
    let mut at = 0usize;
    while at + 2 <= b.len() {
        let (op, len, scope) = (b[at], usize::from(b[at + 1] & 0x7F), b[at + 1] & 0x80 != 0);
        if len < 2 || at + len > b.len() {
            break;
        }
        let o = &b[at..at + len];
        let mut next = at + len;
        let mut pushed = false;
        match op {
            OP_FORM_SET => {
                if let Some(done) = fs.take() {
                    out.push(done);
                }
                let n = usize::from(o.get(22).copied().unwrap_or(0) & 3);
                let class_guids = (0..n).map(|i| guid_at(o, 23 + 16 * i)).collect();
                fs = Some(FormSet {
                    guid: guid_at(o, 2),
                    title: u16le(o, 18),
                    help: u16le(o, 20),
                    class_guids,
                    forms: Vec::new(),
                    varstores: Vec::new(),
                    conds: Vec::new(),
                });
                conds.clear();
                if scope {
                    stack.push(Scope::FormSet);
                    pushed = true;
                }
            }
            OP_FORM | OP_FORM_MAP => {
                if let Some(fs) = fs.as_mut() {
                    fs.forms.push(Form { id: u16le(o, 2), title: u16le(o, 4), items: Vec::new() });
                    form = Some(fs.forms.len() - 1);
                }
                if scope {
                    stack.push(Scope::Form);
                    pushed = true;
                }
            }
            OP_VARSTORE => {
                if let Some(fs) = fs.as_mut() {
                    fs.varstores.push(VarStore {
                        id: u16le(o, 18),
                        guid: guid_at(o, 2),
                        name: ascii(o.get(22..).unwrap_or(&[])),
                        size: u16le(o, 20),
                        efi: false,
                        name_value: false,
                    });
                }
            }
            OP_VARSTORE_EFI => {
                if let Some(fs) = fs.as_mut() {
                    fs.varstores.push(VarStore {
                        id: u16le(o, 2),
                        guid: guid_at(o, 4),
                        name: ascii(o.get(26..).unwrap_or(&[])),
                        size: if o.len() >= 26 { u16le(o, 24) } else { 0 },
                        efi: true,
                        name_value: false,
                    });
                }
            }
            OP_VARSTORE_NAME_VALUE => {
                if let Some(fs) = fs.as_mut() {
                    fs.varstores.push(VarStore {
                        id: u16le(o, 2),
                        guid: guid_at(o, 4),
                        name: String::new(),
                        size: 0,
                        efi: false,
                        name_value: true,
                    });
                }
            }
            OP_SUPPRESS_IF | OP_GRAY_OUT_IF | OP_DISABLE_IF => {
                let (expr, end) = expression(b, next);
                next = end;
                let kind = match op {
                    OP_SUPPRESS_IF => CondKind::Suppress,
                    OP_GRAY_OUT_IF => CondKind::GrayOut,
                    _ => CondKind::Disable,
                };
                if scope {
                    if let Some(fs) = fs.as_mut() {
                        fs.conds.push(Cond { kind, expr });
                        conds.push((fs.conds.len() - 1) as u16);
                        stack.push(Scope::Cond);
                    } else {
                        stack.push(Scope::Other);
                    }
                    pushed = true;
                }
            }
            OP_NO_SUBMIT_IF | OP_INCONSISTENT_IF | OP_WARNING_IF => {
                next = expression(b, next).1;
                if scope {
                    stack.push(Scope::Other);
                    pushed = true;
                }
            }
            OP_GUID => {
                if scope {
                    if guid_at(o, 2) == BIT_VARSTORE_GUID {
                        bits += 1;
                        stack.push(Scope::Bits);
                    } else {
                        stack.push(Scope::Other);
                    }
                    pushed = true;
                }
            }
            OP_ONE_OF_OPTION => {
                // Belongs to the innermost open question.
                let q = stack.iter().rev().find_map(|s| match s {
                    Scope::Question(f, i, n) => Some((*f, *i, *n)),
                    _ => None,
                });
                if let (Some((f, i, n)), Some(fs)) = (q, fs.as_mut()) {
                    let ty = o.get(5).copied().unwrap_or(0);
                    let size = match ty {
                        0 | 4 => 1,
                        1 => 2,
                        2 => 4,
                        3 => 8,
                        _ => 0,
                    };
                    let opt = OneOfOption {
                        text: u16le(o, 2),
                        value: sized(o, 6, size),
                        default: o.get(4).copied().unwrap_or(0) & 0x10 != 0,
                        conds: conds.get(n..).unwrap_or(&[]).to_vec(),
                    };
                    if let Some(Kind::OneOf(v)) = fs.forms.get_mut(f).and_then(|fm| fm.items.get_mut(i)).map(|it| &mut it.kind) {
                        v.push(opt);
                    }
                }
            }
            OP_END => {
                match stack.pop() {
                    Some(Scope::FormSet) => {
                        if let Some(done) = fs.take() {
                            out.push(done);
                        }
                        form = None;
                    }
                    Some(Scope::Form) => form = None,
                    Some(Scope::Cond) => {
                        conds.pop();
                    }
                    Some(Scope::Bits) => bits = bits.saturating_sub(1),
                    _ => {}
                }
                pushed = true;
            }
            _ => {
                if let Some(kind) = item_kind(op, o) {
                    let q = question(op, o, bits > 0);
                    if let (Some(fs), Some(f)) = (fs.as_mut(), form) {
                        let fm = &mut fs.forms[f];
                        fm.items.push(Item { prompt: u16le(o, 2), help: u16le(o, 4), kind, q, conds: conds.clone() });
                        if scope {
                            stack.push(Scope::Question(f, fm.items.len() - 1, conds.len()));
                            pushed = true;
                        }
                    }
                }
            }
        }
        if scope && !pushed {
            stack.push(Scope::Other);
        }
        at = next;
    }
    if let Some(done) = fs.take() {
        out.push(done);
    }
    out
}

fn item_kind(op: u8, o: &[u8]) -> Option<Kind> {
    let flags = o.get(13).copied().unwrap_or(0);
    Some(match op {
        OP_SUBTITLE => Kind::Subtitle,
        OP_TEXT => Kind::Text(u16le(o, 6)),
        OP_ONE_OF => Kind::OneOf(Vec::new()),
        OP_CHECKBOX => Kind::Checkbox { default: flags & 1 != 0 },
        OP_NUMERIC => {
            let size = 1usize << (flags & 3);
            Kind::Numeric {
                min: sized(o, 14, size),
                max: sized(o, 14 + size, size),
                step: sized(o, 14 + 2 * size, size),
                hex: flags & 0x30 == 0x20,
            }
        }
        OP_REF => Kind::Ref {
            form: if o.len() >= 15 { u16le(o, 13) } else { 0 },
            formset: (o.len() >= 33).then(|| guid_at(o, 17)),
        },
        OP_STRING => Kind::String,
        OP_PASSWORD => Kind::Password,
        OP_DATE => Kind::Date,
        OP_TIME => Kind::Time,
        OP_ACTION => Kind::Action,
        OP_ORDERED_LIST => Kind::OrderedList,
        OP_RESET_BUTTON => Kind::ResetButton,
        _ => return None,
    })
}

fn question(op: u8, o: &[u8], bits: bool) -> Option<Question> {
    if matches!(op, OP_SUBTITLE | OP_TEXT | OP_RESET_BUTTON) || o.len() < 13 {
        return None;
    }
    let flags = o.get(13).copied().unwrap_or(0);
    let width = match op {
        OP_CHECKBOX => 1,
        OP_ONE_OF | OP_NUMERIC => 1u8 << (flags & 3),
        _ => 0,
    };
    Some(Question { id: u16le(o, 6), vsid: u16le(o, 8), offset: u16le(o, 10), width, flags: o[12], bits })
}

/// The name of a form set's title for lists.
pub fn title<'a>(fs: &FormSet, s: &'a Strings) -> &'a str {
    s.get(fs.title)
}
