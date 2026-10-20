#!/usr/bin/env python3
"""Boot the app headless in QEMU, press keys over QMP, and save PNG screenshots.

Usage: scripts/screenshots.py OUT_DIR
Needs Pillow for PNG conversion.
"""
import json, os, socket, subprocess, sys, tempfile, time
from PIL import Image

out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "screenshots")
os.makedirs(out, exist_ok=True)
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sock_path = os.path.join(tempfile.mkdtemp(), "qmp.sock")

qemu = subprocess.Popen(
    [os.path.join(root, "scripts/run-qemu.sh"), "-display", "none",
     "-qmp", f"unix:{sock_path},server,nowait"],
    cwd=root)

for _ in range(600):
    if os.path.exists(sock_path):
        break
    time.sleep(0.2)
s = socket.socket(socket.AF_UNIX)
s.connect(sock_path)
f = s.makefile("rw")
f.readline()

def qmp(cmd, **args):
    f.write(json.dumps({"execute": cmd, "arguments": args}) + "\n")
    f.flush()
    while True:
        r = json.loads(f.readline())
        if "return" in r or "error" in r:
            return r

qmp("qmp_capabilities")
t0 = time.time()

def shot(name):
    ppm = os.path.join(out, name + ".ppm")
    qmp("screendump", filename=ppm)
    time.sleep(0.3)
    Image.open(ppm).save(os.path.join(out, name + ".png"))
    os.remove(ppm)
    print(f"{time.time() - t0:6.1f}s  {name}.png", flush=True)

def key(k, wait=1.5):
    qmp("send-key", keys=[{"type": "qcode", "data": k}], **{"hold-time": 200})
    time.sleep(wait)

def at(sec):
    time.sleep(max(0, sec - (time.time() - t0)))

# Timings are wall-clock from QEMU start; adjust with SHOT_OFFSET if firmware boots slower.
off = float(os.environ.get("SHOT_OFFSET", "0"))
for i, sec in enumerate([4, 5, 6, 7, 8, 9, 10, 11, 12, 13]):
    at(sec + off)
    shot(f"intro-{i:02d}")
at(18 + off)
shot("menu-boot")
key("down"); shot("menu-settings")
key("ret"); shot("settings")
key("right"); key("right"); shot("settings-timeout-changed")
key("esc", 2.5); key("down"); shot("menu-firmware")
key("ret"); shot("confirm-firmware")
key("esc", 2.5); key("up"); key("up"); key("ret"); shot("boot-list")
# Boot the second entry (EFI shell in stock OVMF) through BootNext.
key("down"); key("ret", 15); shot("booted-entry")
# Reset: BootNext is consumed, so the app comes back. Skip the intro and
# confirm "Firmware Setup"; the firmware should open its own setup screen.
qmp("system_reset")
time.sleep(10 + off); key("spc", 4)
key("down"); key("down"); key("ret"); key("ret", 15); shot("firmware-setup")
qemu.kill()
