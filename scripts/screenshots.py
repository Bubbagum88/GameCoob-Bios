#!/usr/bin/env python3
"""Boot the app headless in QEMU, press keys over QMP, and save PNG screenshots.

Usage: [IPL=path/to/ipl.bin] scripts/screenshots.py OUT_DIR
Walks the rebuilt intro and cube menu; needs your dump (IPL, or local/ipl.bin).
Needs Pillow for PNG conversion.
"""
import json, os, socket, subprocess, sys, tempfile, time
from PIL import Image

out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "screenshots")
os.makedirs(out, exist_ok=True)
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sock_path = os.path.join(tempfile.mkdtemp(), "qmp.sock")
if not (os.environ.get("IPL") or os.path.exists(os.path.join(root, "local/ipl.bin"))):
    sys.exit("needs your dump: set IPL=path/to/ipl.bin or put it at local/ipl.bin")

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

for i, sec in enumerate([10, 12, 14, 16, 18]):
    at(sec + off)
    shot(f"intro-{i:02d}")
at(23 + off)
shot("cube-top")
key("up"); shot("cube-game-face")
key("ret", 3); shot("game-boot-list")
key("esc", 2.5); key("down", 2); key("down"); shot("cube-card-face")
key("ret", 3); shot("card-system")
key("ret"); shot("card-confirm")
key("esc"); key("esc", 2.5); key("up", 2); key("left"); shot("cube-option-face")
key("ret", 3); shot("options")
key("ret"); key("right"); key("right"); shot("options-timeout-edit")
key("ret"); shot("options-timeout-saved")
key("esc", 2.5); key("right", 2); key("right"); shot("cube-clock-face")
key("ret", 3); shot("clock")
# Boot the second entry (EFI shell in stock OVMF) through BootNext.
key("esc", 2.5); key("left", 2); key("up"); key("ret", 3)
key("down"); key("ret", 15); shot("booted-entry")
# Reset: BootNext is consumed, so the app comes back. Skip the intro,
# pick Memory Card > Firmware Setup > Yes.
qmp("system_reset")
time.sleep(10 + off); key("esc", 3)
key("down"); key("ret", 3); key("ret"); key("up"); shot("firmware-confirm")
key("ret", 15); shot("firmware-setup")
qemu.kill()
