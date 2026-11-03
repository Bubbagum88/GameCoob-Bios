//! What differs between Windows and Linux: finding the ESP, reading the
//! launch variable, and where the helper keeps its state.

use crate::launch::{REQUEST_VAR, VENDOR_GUID};
use std::path::{Path, PathBuf};

/// True if `dir` looks like the ESP GameCoob boots from.
pub fn is_esp(dir: &Path) -> bool {
    dir.join("EFI").join("GameCoob").is_dir()
}

/// The ESP as a folder, mounted for as long as this lives (Windows mounts
/// it on a free drive letter and unmounts it on drop).
pub struct Esp {
    pub dir: PathBuf,
    #[cfg_attr(not(windows), allow(dead_code))]
    letter: Option<char>,
}

impl Drop for Esp {
    fn drop(&mut self) {
        #[cfg(windows)]
        if let Some(l) = self.letter {
            let _ = std::process::Command::new("mountvol").arg(format!("{}:", l)).arg("/D").status();
        }
    }
}

/// Finds (and on Windows mounts) the ESP, unless `given` names it.
pub fn esp(given: Option<&str>) -> Result<Esp, String> {
    if let Some(dir) = given {
        let dir = PathBuf::from(dir);
        return if is_esp(&dir) {
            Ok(Esp { dir, letter: None })
        } else {
            Err(format!("{} has no EFI/GameCoob folder", dir.display()))
        };
    }
    find_esp()
}

#[cfg(not(windows))]
fn find_esp() -> Result<Esp, String> {
    // FAT mounts first, in /proc/mounts order, then the usual places.
    let mounts = std::fs::read_to_string("/proc/mounts").unwrap_or_default();
    let fat = mounts.lines().filter_map(|l| {
        let mut f = l.split(' ');
        let (_, dir, kind) = (f.next()?, f.next()?, f.next()?);
        (kind == "vfat").then(|| PathBuf::from(dir.replace("\\040", " ")))
    });
    let usual = ["/efi", "/boot", "/boot/efi"].into_iter().map(PathBuf::from);
    fat.chain(usual)
        .find(|d| is_esp(d))
        .map(|dir| Esp { dir, letter: None })
        .ok_or_else(|| "no mounted ESP has an EFI/GameCoob folder (mount it, or pass --esp DIR)".to_string())
}

#[cfg(windows)]
fn find_esp() -> Result<Esp, String> {
    // Already mounted somewhere (by an earlier run, or by the user)?
    for l in ('A'..='Z').rev() {
        let dir = PathBuf::from(format!("{}:\\", l));
        if is_esp(&dir) {
            return Ok(Esp { dir, letter: None });
        }
    }
    let free = ('D'..='Z').rev().find(|l| !Path::new(&format!("{}:\\", l)).exists()).ok_or("no free drive letter")?;
    let ok = std::process::Command::new("mountvol")
        .arg(format!("{}:", free))
        .arg("/S")
        .status()
        .map_err(|e| format!("mountvol: {}", e))?
        .success();
    let esp = Esp { dir: PathBuf::from(format!("{}:\\", free)), letter: Some(free) };
    if !ok {
        return Err("mountvol /S failed (the helper must run as administrator)".into());
    }
    if !is_esp(&esp.dir) {
        return Err("the ESP has no EFI\\GameCoob folder".into());
    }
    Ok(esp)
}

/// The launch variable's bytes, or None if it is not set.
#[cfg(not(windows))]
pub fn read_request() -> Result<Option<Vec<u8>>, String> {
    let p = format!("/sys/firmware/efi/efivars/{}-{}", REQUEST_VAR, VENDOR_GUID);
    match std::fs::read(&p) {
        // efivarfs puts the 4-byte attributes first.
        Ok(b) if b.len() >= 4 => Ok(Some(b[4..].to_vec())),
        Ok(_) => Ok(None),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(None),
        Err(e) => Err(format!("{}: {}", p, e)),
    }
}

#[cfg(windows)]
pub fn read_request() -> Result<Option<Vec<u8>>, String> {
    win::read_var(REQUEST_VAR, VENDOR_GUID)
}

/// A copy of the paths file outside the ESP, readable by every user, for
/// runs that can't read the ESP. `GAMECOOB_PATHS_CACHE` overrides it.
pub fn paths_cache() -> PathBuf {
    if let Some(p) = std::env::var_os("GAMECOOB_PATHS_CACHE") {
        return PathBuf::from(p);
    }
    #[cfg(windows)]
    let base = std::env::var_os("ProgramData").map(PathBuf::from).unwrap_or_else(|| PathBuf::from("C:\\ProgramData"));
    #[cfg(not(windows))]
    let base = PathBuf::from("/var/lib");
    base.join("gamecoob").join("paths.toml")
}

/// Where the helper remembers the last request it ran.
pub fn state_file() -> PathBuf {
    #[cfg(windows)]
    let base = std::env::var_os("LOCALAPPDATA").map(PathBuf::from).unwrap_or_else(|| PathBuf::from("C:\\"));
    #[cfg(not(windows))]
    let base = std::env::var_os("XDG_STATE_HOME")
        .map(PathBuf::from)
        .or_else(|| std::env::var_os("HOME").map(|h| PathBuf::from(h).join(".local").join("state")))
        .unwrap_or_else(|| PathBuf::from("/tmp"));
    base.join("gamecoob").join("last-launch")
}

#[cfg(windows)]
mod win {
    use core::ffi::c_void;

    #[repr(C)]
    struct Luid {
        low: u32,
        high: i32,
    }

    #[repr(C)]
    struct TokenPrivileges {
        count: u32,
        luid: Luid,
        attributes: u32,
    }

    #[link(name = "kernel32")]
    unsafe extern "system" {
        fn GetCurrentProcess() -> isize;
        fn CloseHandle(h: isize) -> i32;
        fn GetLastError() -> u32;
        fn GetFirmwareEnvironmentVariableW(name: *const u16, guid: *const u16, buf: *mut c_void, size: u32) -> u32;
    }

    #[link(name = "advapi32")]
    unsafe extern "system" {
        fn OpenProcessToken(process: isize, access: u32, token: *mut isize) -> i32;
        fn LookupPrivilegeValueW(system: *const u16, name: *const u16, luid: *mut Luid) -> i32;
        fn AdjustTokenPrivileges(
            token: isize,
            disable_all: i32,
            new: *const TokenPrivileges,
            len: u32,
            prev: *mut c_void,
            ret_len: *mut u32,
        ) -> i32;
    }

    const TOKEN_ADJUST_PRIVILEGES: u32 = 0x20;
    const TOKEN_QUERY: u32 = 0x8;
    const SE_PRIVILEGE_ENABLED: u32 = 2;
    const ERROR_ENVVAR_NOT_FOUND: u32 = 203;

    fn wide(s: &str) -> Vec<u16> {
        s.encode_utf16().chain(Some(0)).collect()
    }

    /// Reading firmware variables needs SeSystemEnvironmentPrivilege,
    /// which an elevated process has but must switch on.
    fn enable_privilege() -> Result<(), String> {
        unsafe {
            let mut token = 0isize;
            if OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &mut token) == 0 {
                return Err(format!("OpenProcessToken failed ({})", GetLastError()));
            }
            let mut tp = TokenPrivileges { count: 1, luid: Luid { low: 0, high: 0 }, attributes: SE_PRIVILEGE_ENABLED };
            let name = wide("SeSystemEnvironmentPrivilege");
            let ok = LookupPrivilegeValueW(core::ptr::null(), name.as_ptr(), &mut tp.luid) != 0
                && AdjustTokenPrivileges(token, 0, &tp, 0, core::ptr::null_mut(), core::ptr::null_mut()) != 0
                && GetLastError() == 0;
            CloseHandle(token);
            if ok { Ok(()) } else { Err("could not enable SeSystemEnvironmentPrivilege (run as administrator)".into()) }
        }
    }

    pub fn read_var(name: &str, guid: &str) -> Result<Option<Vec<u8>>, String> {
        enable_privilege()?;
        let name = wide(name);
        let guid = wide(&format!("{{{}}}", guid));
        let mut buf = vec![0u8; 4096];
        let n = unsafe {
            GetFirmwareEnvironmentVariableW(name.as_ptr(), guid.as_ptr(), buf.as_mut_ptr().cast(), buf.len() as u32)
        };
        if n == 0 {
            let err = unsafe { GetLastError() };
            return if err == ERROR_ENVVAR_NOT_FOUND { Ok(None) } else { Err(format!("reading the launch variable failed ({})", err)) };
        }
        buf.truncate(n as usize);
        Ok(Some(buf))
    }
}
