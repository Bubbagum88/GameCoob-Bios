use super::toml::{self, Value};
use super::*;

#[test]
fn example_paths_file_parses() {
    let p = Paths::parse(include_str!("../../docs/paths.example.toml")).unwrap();
    assert_eq!(p.boot, Boot::default());
    assert_eq!(p.os.len(), 2);
    assert_eq!(p.os[0].kind, OsKind::Windows);
    assert_eq!(p.os[1].kind, OsKind::Linux);
    let dolphin = p.emulator_by_id("dolphin-win").unwrap();
    assert_eq!(dolphin.exe, "C:\\Program Files\\Dolphin\\Dolphin.exe");
    let (exe, args) = dolphin.command("D:\\Games\\GameCube\\Wind Waker.rvz");
    assert_eq!(exe, dolphin.exe);
    assert_eq!(args, ["-b", "-e", "D:\\Games\\GameCube\\Wind Waker.rvz"]);
    assert_eq!(p.roms[0].ext.len(), DEFAULT_EXTS.len());
    assert!(p.roms[0].recursive);
    assert_eq!(p.roms[1].ext, ["iso", "rvz"]);
}

#[test]
fn empty_file_gives_defaults() {
    assert_eq!(Paths::parse("").unwrap(), Paths::default());
    assert_eq!(Paths::parse("\u{FEFF}# nothing\r\n\r\n").unwrap(), Paths::default());
}

#[test]
fn values() {
    let t = toml::parse("a = \"x\\\\y\\u00e9\"\nb = 'C:\\no\\escape'\nc = -1_000\nd = true\ne = [\n  1, 'two', # note\n  [false],\n]\n")
        .unwrap();
    let root = &t[0];
    assert_eq!(root.get("a").unwrap().value, Value::Str("x\\yé".into()));
    assert_eq!(root.get("b").unwrap().value, Value::Str("C:\\no\\escape".into()));
    assert_eq!(root.get("c").unwrap().value, Value::Int(-1000));
    assert_eq!(root.get("d").unwrap().value, Value::Bool(true));
    assert_eq!(
        root.get("e").unwrap().value,
        Value::Array(vec![Value::Int(1), Value::Str("two".into()), Value::Array(vec![Value::Bool(false)])])
    );
}

#[test]
fn syntax_errors_name_the_line() {
    let err = |s: &str| toml::parse(s).unwrap_err();
    assert_eq!(err("\n\na = C:\\x\n").line, 3);
    assert!(err("a = \"C:\\Games\"\n").msg.contains("single"));
    assert_eq!(err("[boot]\n[boot]\n").line, 2);
    assert_eq!(err("a = 1\na = 2\n").line, 2);
    assert!(err("a.b = 1\n").msg.contains("dotted"));
    assert!(err("a = 'open\n").msg.contains("closed"));
    assert!(err("a = 1 2\n").msg.contains("after"));
}

#[test]
fn schema_errors_are_all_reported() {
    let text = "\
stray = 1
[boot]
ipl = 3
[[os]]
id = \"win\"
kind = \"mac\"
[[emulator]]
id = \"d\"
os = \"nowhere\"
exe = \"/bin/d\"
typo = \"x\"
[[roms]]
os = \"win\"
path = \"/g\"
emulator = \"missing\"
[mystery]
";
    let errs = Paths::parse(text).unwrap_err();
    let lines: Vec<u32> = errs.iter().map(|e| e.line).collect();
    // stray key, ipl not text, bad kind, missing boot, unknown typo,
    // unknown table, emulator's os, roms' os (win was refused), roms' emulator.
    assert_eq!(lines, [1, 3, 6, 4, 11, 16, 9, 13, 15], "{:?}", errs);
}

#[test]
fn emulator_must_be_on_the_roms_os() {
    let text = "\
[[os]]
id = \"windows\"
boot = \"Windows\"
[[os]]
id = \"arch\"
boot = \"Linux\"
[[emulator]]
id = \"d\"
os = \"arch\"
exe = \"/usr/bin/dolphin-emu\"
[[roms]]
os = \"windows\"
path = 'D:\\g'
emulator = \"d\"
";
    let errs = Paths::parse(text).unwrap_err();
    assert_eq!(errs.len(), 1);
    assert_eq!(errs[0].line, 14);
    let ok = Paths::parse(&text.replace("os = \"windows\"\npath", "os = \"arch\"\npath")).unwrap();
    assert_eq!(ok.os[0].kind, OsKind::Windows);
    assert_eq!(ok.os[1].kind, OsKind::Linux);
    assert_eq!(ok.emulator[0].args, ["{rom}"]);
}

#[test]
fn library_round_trips() {
    let lib = Library {
        games: vec![
            Game {
                title: "A \"quoted\" game".into(),
                code: Some("GXXE01".into()),
                os: "windows".into(),
                emulator: "dolphin-win".into(),
                path: "D:\\Games\\a.rvz".into(),
            },
            Game { title: "Ünïcode".into(), code: None, os: "arch".into(), emulator: "d".into(), path: "/g/b.iso".into() },
        ],
    };
    assert_eq!(Library::parse(&lib.to_toml()).unwrap(), lib);
    assert_eq!(Library::parse("").unwrap(), Library::default());
}

#[test]
fn request_round_trips() {
    let req = Request {
        count: 7,
        os: "arch".into(),
        emulator: "dolphin-arch".into(),
        path: "/home/you/Games/x.iso".into(),
        title: "X".into(),
    };
    assert_eq!(Request::parse(&req.to_toml()).unwrap(), req);
    assert!(Request::parse(&req.to_toml().replace("version = 1", "version = 2")).is_err());
    assert!(Request::parse("version = 1\n").is_err());
}
