//! The small slice of TOML the paths file, the game library and the launch
//! request use: comments, `[table]`, `[[array of tables]]`, and
//! `key = value` where a value is a string (`"basic"` with escapes, or
//! `'literal'`, handy for Windows paths), an integer, a boolean or an array
//! of those (it may run over several lines). Dotted keys, inline tables,
//! dates, floats and multi-line strings are not needed and are refused with
//! a line number, so a typo never passes as something else.

use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;
use core::fmt;

/// A value.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Value {
    Str(String),
    Int(i64),
    Bool(bool),
    Array(Vec<Value>),
}

impl Value {
    pub fn as_str(&self) -> Option<&str> {
        match self {
            Value::Str(s) => Some(s),
            _ => None,
        }
    }
}

/// One `key = value` line, with the line it came from for error messages.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Entry {
    pub key: String,
    pub value: Value,
    pub line: u32,
}

/// The keys before the first header (`name` empty), or one `[name]` or
/// `[[name]]` table.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Table {
    pub name: String,
    /// `[[name]]`: one element of an array of tables.
    pub array: bool,
    pub line: u32,
    pub entries: Vec<Entry>,
}

impl Table {
    pub fn get(&self, key: &str) -> Option<&Entry> {
        self.entries.iter().find(|e| e.key == key)
    }
}

/// A parse or schema error, with its 1-based line (0: no line).
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error {
    pub line: u32,
    pub msg: String,
}

impl Error {
    pub fn new(line: u32, msg: impl Into<String>) -> Self {
        Error { line, msg: msg.into() }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        if self.line == 0 {
            f.write_str(&self.msg)
        } else {
            write!(f, "line {}: {}", self.line, self.msg)
        }
    }
}

/// Parses a whole document into its tables, the root one first (it may be
/// empty). A plain `[name]` may appear once; `[[name]]` any number of times.
pub fn parse(text: &str) -> Result<Vec<Table>, Error> {
    let mut p = Parser { s: text.as_bytes(), text, i: 0, line: 1 };
    if p.s.starts_with(b"\xEF\xBB\xBF") {
        p.i = 3; // A UTF-8 BOM, as Notepad may write.
    }
    let mut tables = Vec::from([Table { name: String::new(), array: false, line: 0, entries: Vec::new() }]);
    loop {
        p.skip_blank_lines();
        let Some(c) = p.peek() else { break };
        let line = p.line;
        if c == b'[' {
            let array = p.s.get(p.i + 1) == Some(&b'[');
            p.i += if array { 2 } else { 1 };
            p.skip_ws();
            let name = p.key()?;
            p.skip_ws();
            let close: &[u8] = if array { b"]]" } else { b"]" };
            if !p.s[p.i..].starts_with(close) {
                return Err(p.err("expected ] after the table name (dotted names are not supported)"));
            }
            p.i += close.len();
            p.end_of_line()?;
            if let Some(prev) = tables.iter().find(|t| t.name == name) {
                if !(array && prev.array) {
                    return Err(Error::new(line, format!("table [{}] is defined twice", name)));
                }
            }
            tables.push(Table { name, array, line, entries: Vec::new() });
        } else {
            let key = p.key()?;
            p.skip_ws();
            if p.peek() == Some(b'.') {
                return Err(p.err("dotted keys are not supported"));
            }
            if p.peek() != Some(b'=') {
                return Err(p.err(format!("expected = after {}", key)));
            }
            p.i += 1;
            p.skip_ws();
            let value = p.value()?;
            p.end_of_line()?;
            let table = tables.last_mut().unwrap();
            if table.get(&key).is_some() {
                return Err(Error::new(line, format!("{} is set twice", key)));
            }
            table.entries.push(Entry { key, value, line });
        }
    }
    Ok(tables)
}

struct Parser<'a> {
    s: &'a [u8],
    text: &'a str,
    i: usize,
    line: u32,
}

impl Parser<'_> {
    fn peek(&self) -> Option<u8> {
        self.s.get(self.i).copied()
    }

    fn err(&self, msg: impl Into<String>) -> Error {
        Error::new(self.line, msg)
    }

    fn skip_ws(&mut self) {
        while matches!(self.peek(), Some(b' ' | b'\t')) {
            self.i += 1;
        }
    }

    fn skip_comment(&mut self) {
        if self.peek() == Some(b'#') {
            while !matches!(self.peek(), None | Some(b'\n')) {
                self.i += 1;
            }
        }
    }

    fn newline(&mut self) -> bool {
        match self.peek() {
            Some(b'\n') => {
                self.i += 1;
                self.line += 1;
                true
            }
            Some(b'\r') if self.s.get(self.i + 1) == Some(&b'\n') => {
                self.i += 2;
                self.line += 1;
                true
            }
            _ => false,
        }
    }

    /// Whitespace, comments and line breaks.
    fn skip_blank_lines(&mut self) {
        loop {
            self.skip_ws();
            self.skip_comment();
            if !self.newline() {
                break;
            }
        }
    }

    fn end_of_line(&mut self) -> Result<(), Error> {
        self.skip_ws();
        self.skip_comment();
        if self.peek().is_none() || self.newline() {
            Ok(())
        } else {
            Err(self.err("unexpected text after the value"))
        }
    }

    /// A bare key (letters, digits, `_`, `-`) or a quoted one.
    fn key(&mut self) -> Result<String, Error> {
        match self.peek() {
            Some(b'"') | Some(b'\'') => self.string(),
            _ => {
                let start = self.i;
                while matches!(self.peek(), Some(c) if c.is_ascii_alphanumeric() || c == b'_' || c == b'-') {
                    self.i += 1;
                }
                if self.i == start {
                    return Err(self.err("expected a key"));
                }
                Ok(self.text[start..self.i].to_string())
            }
        }
    }

    fn value(&mut self) -> Result<Value, Error> {
        match self.peek() {
            Some(b'"') | Some(b'\'') => Ok(Value::Str(self.string()?)),
            Some(b'[') => {
                self.i += 1;
                let mut items = Vec::new();
                loop {
                    self.skip_blank_lines();
                    if self.peek() == Some(b']') {
                        self.i += 1;
                        return Ok(Value::Array(items));
                    }
                    items.push(self.value()?);
                    self.skip_blank_lines();
                    match self.peek() {
                        Some(b',') => self.i += 1,
                        Some(b']') => {}
                        _ => return Err(self.err("expected , or ] in the array")),
                    }
                }
            }
            Some(b't') if self.s[self.i..].starts_with(b"true") => {
                self.i += 4;
                Ok(Value::Bool(true))
            }
            Some(b'f') if self.s[self.i..].starts_with(b"false") => {
                self.i += 5;
                Ok(Value::Bool(false))
            }
            Some(c) if c.is_ascii_digit() || c == b'-' || c == b'+' => {
                let start = self.i;
                self.i += 1;
                while matches!(self.peek(), Some(c) if c.is_ascii_digit() || c == b'_') {
                    self.i += 1;
                }
                let digits: String = self.text[start..self.i].chars().filter(|&c| c != '_').collect();
                digits
                    .parse::<i64>()
                    .map(Value::Int)
                    .map_err(|_| self.err("bad number (only whole numbers are supported)"))
            }
            None => Err(self.err("expected a value")),
            _ => Err(self.err("expected a value: \"text\", 'text', a number, true/false or [..] (put paths in quotes)")),
        }
    }

    fn string(&mut self) -> Result<String, Error> {
        let quote = self.peek().unwrap();
        if self.s[self.i..].starts_with(if quote == b'"' { b"\"\"\"" } else { b"'''" }) {
            return Err(self.err("multi-line strings are not supported"));
        }
        self.i += 1;
        let mut out = String::new();
        loop {
            let Some(ch) = self.text[self.i..].chars().next() else {
                return Err(self.err("string is not closed"));
            };
            self.i += ch.len_utf8();
            match ch {
                '\n' | '\r' => return Err(self.err("string is not closed on its line")),
                c if c as u32 == quote as u32 => return Ok(out),
                '\\' if quote == b'"' => {
                    let Some(e) = self.peek() else { return Err(self.err("string is not closed")) };
                    self.i += 1;
                    match e {
                        b'\\' => out.push('\\'),
                        b'"' => out.push('"'),
                        b'n' => out.push('\n'),
                        b't' => out.push('\t'),
                        b'r' => out.push('\r'),
                        b'u' | b'U' => {
                            let n = if e == b'u' { 4 } else { 8 };
                            let hex = self.text.get(self.i..self.i + n).ok_or_else(|| self.err("short \\u escape"))?;
                            let c = u32::from_str_radix(hex, 16)
                                .ok()
                                .and_then(char::from_u32)
                                .ok_or_else(|| self.err("bad \\u escape"))?;
                            self.i += n;
                            out.push(c);
                        }
                        _ => {
                            return Err(self.err(
                                "unknown \\ escape: put Windows paths in 'single quotes' (or double each \\)",
                            ))
                        }
                    }
                }
                c => out.push(c),
            }
        }
    }
}

/// Writes a string as a TOML basic string.
pub fn quote(s: &str) -> String {
    let mut out = String::with_capacity(s.len() + 2);
    out.push('"');
    for c in s.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            c if (c as u32) < 0x20 || c as u32 == 0x7F => out.push_str(&format!("\\u{:04X}", c as u32)),
            c => out.push(c),
        }
    }
    out.push('"');
    out
}
