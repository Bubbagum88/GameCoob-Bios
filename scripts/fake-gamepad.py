#!/usr/bin/env python3
"""A made-up USB controller for QEMU, over usbredir, to test GameCoob's
gamepad support without hardware.

Usage: scripts/fake-gamepad.py KIND PORT [EVENT...]
  KIND   wiiu (Wii U GameCube adapter, 057e:0337), xbox360, or ds4 (a
         generic HID gamepad: DualShock 4 style report descriptor)
  PORT   TCP port to listen on; start QEMU after this with
           -device qemu-xhci -chardev socket,id=pad,host=127.0.0.1,port=PORT
           -device usb-redir,chardev=pad
  EVENT  START[-END]=BUTTONS: hold BUTTONS (comma separated) from START to
         END seconds after QEMU connects (END defaults to START + 0.25).
         Buttons: a b x y z l r start up down left right guide, or
         stick-left / stick-right / stick-up / stick-down (full tilt).
         For wiiu, prefix pN: to use adapter port N (1-4, default 1).
         START-END=unplug unplugs the device for that time.

Prints each USB request it answers to stderr.
"""
import socket, struct, sys, threading, time

HELLO, CONNECT, DISCONNECT, RESET, IFACE_INFO, EP_INFO = 0, 1, 2, 3, 4, 5
SET_CONFIG, GET_CONFIG, CONFIG_STATUS, SET_ALT, GET_ALT, ALT_STATUS = 6, 7, 8, 9, 10, 11
START_INT, STOP_INT, INT_STATUS, CANCEL = 15, 16, 17, 21
CONTROL, BULK, ISO, INTERRUPT = 100, 101, 102, 103
CAP_CONNECT_VERSION, CAP_EP_MPS, CAP_64BIT_IDS, CAP_32BIT_BULK = 1 << 1, 1 << 4, 1 << 5, 1 << 6
OK, STALL = 0, 4


def device(vid, pid, cls=0):
    return struct.pack("<BBHBBBBHHHBBBB", 18, 1, 0x0200, cls, 0, 0, 64, vid, pid, 0x0100, 0, 0, 0, 1)


def config(iface, extra, eps):
    body = struct.pack("<BBBBBBBBB", 9, 4, 0, 0, len(eps), *iface, 0) + extra
    for addr, mps, interval in eps:
        body += struct.pack("<BBBBHB", 7, 5, addr, 3, mps, interval)
    return struct.pack("<BBHBBBBB", 9, 2, 9 + len(body), 1, 1, 0, 0x80, 50) + body


DS4_REPORT = bytes([
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01,
    0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35,
    0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02,
    0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x0E, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0E, 0x81, 0x02,
    0x06, 0x00, 0xFF, 0x09, 0x20, 0x75, 0x06, 0x95, 0x01, 0x15, 0x00, 0x25, 0x7F, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
    0xC0,
])
WIIU_REPORT = bytes([0x05, 0x05, 0x09, 0x00, 0xA1, 0x01, 0x85, 0x11, 0x19, 0x00, 0x2A, 0xFF, 0x00,
                     0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x05, 0x91, 0x00, 0xC0])


def hid_desc(report):
    return struct.pack("<BBHBBBH", 9, 0x21, 0x0111, 0, 1, 0x22, len(report))


STICKS = {"stick-left": (-1, 0), "stick-right": (1, 0), "stick-up": (0, 1), "stick-down": (0, -1)}


def wiiu_report(held):
    r = bytearray(37)
    r[0] = 0x21
    for port in range(4):
        p = 1 + 9 * port
        # Port 1 and 2 have controllers.
        if port > 1:
            continue
        bs = {b for (n, b) in held if n == port}
        b1 = sum(1 << i for i, n in enumerate(["a", "b", "x", "y", "left", "right", "down", "up"]) if n in bs)
        b2 = sum(1 << i for i, n in enumerate(["start", "z", "r", "l"]) if n in bs)
        sx, sy = 128, 128
        for n, (dx, dy) in STICKS.items():
            if n in bs:
                sx, sy = 128 + 100 * dx, 128 + 100 * dy
        r[p:p + 9] = bytes([0x10, b1, b2, sx, sy, 128, 128, 0, 0])
    return bytes(r)


def xbox360_report(held):
    bs = {b for (_, b) in held}
    b2 = sum(1 << i for i, n in enumerate(["up", "down", "left", "right", "start", "back", "l3", "r3"]) if n in bs)
    b3 = sum(1 << i for i, n in enumerate(["l", "r", "guide", "", "a", "b", "x", "y"]) if n and n in bs)
    lx = ly = 0
    for n, (dx, dy) in STICKS.items():
        if n in bs:
            lx, ly = 32000 * dx, 32000 * dy
    return struct.pack("<BBBBBBhhhh6x", 0, 0x14, b2, b3, 0, 0, lx, ly, 0, 0)


def ds4_report(held):
    bs = {b for (_, b) in held}
    hats = {"up": 0, "right": 2, "down": 4, "left": 6}
    hat = next((v for n, v in hats.items() if n in bs), 8)
    # Square, cross, circle, triangle, L1, R1, L2, R2, share, options, L3, R3, PS.
    names = ["x", "a", "b", "y", "z", "r1", "l", "r", "back", "start", "l3", "r3", "guide"]
    buttons = sum(1 << i for i, n in enumerate(names) if n in bs)
    lx = ly = 128
    for n, (dx, dy) in STICKS.items():
        if n in bs:
            lx, ly = 128 + 127 * dx, 128 - 127 * dy
    b = hat | buttons << 4
    return bytes([0x01, min(lx, 255), min(ly, 255), 128, 128, b & 255, b >> 8 & 255, b >> 16 & 255, 0, 0])


KINDS = {
    "wiiu": dict(
        dev=device(0x057E, 0x0337), iface=(3, 0, 0), extra=hid_desc(WIIU_REPORT), report_desc=WIIU_REPORT,
        eps=[(0x81, 37, 8), (0x02, 5, 8)], report=wiiu_report),
    "xbox360": dict(
        dev=device(0x045E, 0x028E, 0xFF), iface=(0xFF, 0x5D, 0x01), extra=b"", report_desc=b"",
        eps=[(0x81, 32, 4), (0x01, 32, 8)], report=xbox360_report),
    "ds4": dict(
        dev=device(0x054C, 0x05C4), iface=(3, 0, 0), extra=hid_desc(DS4_REPORT), report_desc=DS4_REPORT,
        eps=[(0x84, 64, 5), (0x03, 64, 5)], report=ds4_report),
}


def parse_events(args):
    events = []
    for a in args:
        when, what = a.split("=", 1)
        t0, _, t1 = when.partition("-")
        t0 = float(t0)
        t1 = float(t1) if t1 else t0 + 0.25
        port = 0
        if what.startswith("p") and ":" in what:
            port, what = int(what[1]) - 1, what.split(":", 1)[1]
        events.append((t0, t1, port, what.split(",")))
    return events


def main():
    kind = KINDS[sys.argv[1]]
    port = int(sys.argv[2])
    events = parse_events(sys.argv[3:])
    cfg = config(kind["iface"], kind["extra"], kind["eps"])
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", port))
    srv.listen(1)
    conn, _ = srv.accept()
    t_start = time.time()
    lock = threading.Lock()
    receiving = {}  # IN endpoint -> True
    state = {"connected": False}

    # Message ids are 64-bit (XHCI needs it) once both hellos are through.
    def send(ty, id_, payload=b""):
        fmt = "<IIQ" if state.get("ids64") else "<III"
        with lock:
            conn.sendall(struct.pack(fmt, ty, len(payload), id_) + payload)

    def log(*a):
        print(f"[fake-gamepad {time.time() - t_start:6.2f}s]", *a, file=sys.stderr, flush=True)

    def connect():
        ifc = struct.pack("<I", 1) + bytes([0] + [0] * 31) + bytes([kind["iface"][0]] + [0] * 31) \
            + bytes([kind["iface"][1]] + [0] * 31) + bytes([kind["iface"][2]] + [0] * 31)
        types, intervals, ifaces, mps = [255] * 32, [0] * 32, [0] * 32, [0] * 32
        types[0] = types[16] = 0
        mps[0] = mps[16] = 64
        for addr, m, interval in kind["eps"]:
            i = ((addr & 0x80) >> 3) | (addr & 0x0F)
            types[i], intervals[i], mps[i] = 3, interval, m
        send(IFACE_INFO, 0, ifc)
        send(EP_INFO, 0, bytes(types) + bytes(intervals) + bytes(ifaces) + struct.pack("<32H", *mps))
        d = kind["dev"]
        send(CONNECT, 0, struct.pack("<BBBBHHH", 1, d[4], 0, 0, *struct.unpack("<HH", d[8:12]), 0x0100))
        state["connected"] = True
        log("connected")

    send(HELLO, 0, b"gamecoob-fake-gamepad".ljust(64, b"\0")
         + struct.pack("<I", CAP_CONNECT_VERSION | CAP_EP_MPS | CAP_64BIT_IDS | CAP_32BIT_BULK))

    def reports():
        nid = 1 << 20
        while True:
            time.sleep(0.008)
            t = time.time() - t_start
            unplug = any(t0 <= t < t1 and b == ["unplug"] for t0, t1, _, b in events)
            if unplug and state["connected"]:
                receiving.clear()
                state["connected"] = False
                send(DISCONNECT, 0)
                log("unplugged")
            elif not unplug and not state["connected"] and state.get("hello"):
                connect()
            if not state["connected"]:
                continue
            held = {(p, b) for t0, t1, p, bs in events if t0 <= t < t1 for b in bs if b != "unplug"}
            data = kind["report"](held)
            # On change, else a few times a second: QEMU queues what the
            # guest has not polled yet, so a steady stream would lag.
            if data == state.get("last") and t - state.get("sent", 0) < 0.25:
                continue
            state["last"], state["sent"] = data, t
            for ep in list(receiving):
                send(INTERRUPT, nid, struct.pack("<BBH", ep, OK, len(data)) + data)
                nid += 1

    buf = b""
    def read(n):
        nonlocal buf
        while len(buf) < n:
            chunk = conn.recv(65536)
            if not chunk:
                sys.exit(0)
            buf += chunk
        out, buf = buf[:n], buf[n:]
        return out

    threading.Thread(target=reports, daemon=True).start()
    cur_config = 0
    while True:
        if state.get("ids64"):
            ty, length, id_ = struct.unpack("<IIQ", read(16))
        else:
            ty, length, id_ = struct.unpack("<III", read(12))
        payload = read(length)
        if ty == HELLO:
            caps = struct.unpack("<I", payload[64:68])[0] if len(payload) >= 68 else 0
            state["ids64"] = bool(caps & CAP_64BIT_IDS)
            state["hello"] = True
            log(f"QEMU hello, caps {caps:#x}")
        elif ty == CONTROL:
            ep, req, rtype, _, value, index, ln = struct.unpack("<BBBBHHH", payload[:10])
            data = None
            if rtype == 0x80 and req == 6:
                dt, di = value >> 8, value & 0xFF
                data = {1: kind["dev"], 2: cfg}.get(dt)
            elif rtype == 0x81 and req == 6 and value >> 8 == 0x22:
                data = kind["report_desc"] or None
            elif rtype == 0x80 and req == 0:  # GET_STATUS
                data = b"\0\0"
            elif rtype & 0x80 == 0:  # SET_IDLE, SET_FEATURE...: accept
                data = b""
            log(f"control type {rtype:02x} req {req:02x} value {value:04x} index {index} len {ln}"
                + ("" if data is not None else " -> stall"))
            if data is None:
                send(CONTROL, id_, struct.pack("<BBBBHHH", ep, req, rtype, STALL, value, index, 0))
            elif rtype & 0x80:
                data = data[:ln]
                send(CONTROL, id_, struct.pack("<BBBBHHH", ep, req, rtype, OK, value, index, len(data)) + data)
            else:
                send(CONTROL, id_, struct.pack("<BBBBHHH", ep, req, rtype, OK, value, index, ln))
        elif ty == SET_CONFIG:
            cur_config = payload[0]
            log(f"set configuration {cur_config}")
            send(CONFIG_STATUS, id_, bytes([OK, cur_config]))
        elif ty == GET_CONFIG:
            send(CONFIG_STATUS, id_, bytes([OK, cur_config]))
        elif ty == SET_ALT:
            send(ALT_STATUS, id_, bytes([OK, payload[0], payload[1]]))
        elif ty == GET_ALT:
            send(ALT_STATUS, id_, bytes([OK, payload[0], 0]))
        elif ty == START_INT:
            receiving[payload[0]] = True
            log(f"start interrupt IN {payload[0]:02x}")
            send(INT_STATUS, id_, bytes([OK, payload[0]]))
        elif ty == STOP_INT:
            receiving.pop(payload[0], None)
            log(f"stop interrupt IN {payload[0]:02x}")
            send(INT_STATUS, id_, bytes([OK, payload[0]]))
        elif ty == INTERRUPT:
            ep, _, ln = struct.unpack("<BBH", payload[:4])
            log(f"interrupt OUT {ep:02x}: {payload[4:].hex()}")
            send(INTERRUPT, id_, struct.pack("<BBH", ep, OK, ln))
        elif ty == RESET:
            log("reset")
        elif ty == CANCEL:
            pass
        else:
            log(f"ignored message {ty}")


if __name__ == "__main__":
    main()
