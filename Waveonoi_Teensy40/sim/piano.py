#!/usr/bin/env python3
"""Virtual piano for the Waveonoi wall: play it with the mouse or the laptop
keyboard and it sends MIDI straight to the Teensy over USB (no extra packages).

    python3 sim/piano.py [/dev/snd/midiCxD0]

Mouse        click or drag over the keys; lower on a key = harder
Keyboard     Z S X D C V G B H N J M , L . ; /   lower octave (Z = C)
             Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P [ = ]   upper octave (Q = C)
Space        sustain pedal (hold)
Left/Right   octave down/up        Up/Down   velocity
Buttons      pick an animation, start/stop the demo, all notes off
"""
import glob
import os
import sys
import tkinter as tk

LOW, HIGH = 21, 108
BLACK = {1, 3, 6, 8, 10}
NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
# Same colours as the wall: pitch classes around the circle of fifths.
FIFTHS_COL = ["#ff2a1a", "#ff6e00", "#ffc200", "#b4ff1a", "#1aff75", "#00eded",
              "#2ba0ff", "#4d3dff", "#a81aff", "#ee00ff", "#ff1ab4", "#ff0a6b"]
MODES = ["Ripples", "Veins", "Rising notes", "Sparks", "Lightning", "Fireflies",
         "Coral", "Honeycomb", "Interference", "Resonance"]
MODE_KEYS = [60, 62, 64, 65, 67, 69, 71, 72, 74, 76]  # C4, D4, ... (hold A0 + B0)

LOWER = ["z", "s", "x", "d", "c", "v", "g", "b", "h", "n", "j", "m",
         "comma", "l", "period", "semicolon", "slash"]
UPPER = ["q", "2", "w", "3", "e", "r", "5", "t", "6", "y", "7", "u",
         "i", "9", "o", "0", "p", "bracketleft", "equal", "bracketright"]
LABEL = {"comma": ",", "period": ".", "semicolon": ";", "slash": "/",
         "bracketleft": "[", "equal": "=", "bracketright": "]"}


def note_color(n):
    return FIFTHS_COL[(n % 12) * 7 % 12]


def find_teensy():
    for card in sorted(glob.glob("/proc/asound/card[0-9]*")):
        try:
            name = open(os.path.join(card, "id")).read().strip()
            desc = open(os.path.join(card, "midi0")).readline()
        except OSError:
            continue
        if "Teensy" in desc or "Teensy" in name:
            dev = "/dev/snd/midiC%sD0" % card.rsplit("card", 1)[1]
            if os.path.exists(dev):
                return dev
    return None


class Piano:
    def __init__(self, root, device):
        self.root = root
        self.device_arg = device
        self.fd = None
        self.base = 48  # lower keyboard row starts at C3
        self.vel = 90
        self.sustain = False
        self.sounding = {}     # note -> number of sources holding it
        self.keys_down = {}    # keysym -> note
        self.release_jobs = {}
        self.mouse_note = None
        self.rects = {}

        root.title("Waveonoi piano")
        root.configure(bg="#111")
        bar = tk.Frame(root, bg="#111")
        bar.pack(fill="x", padx=8, pady=6)
        for i, name in enumerate(MODES):
            tk.Button(bar, text="%d %s" % (i + 1, name), command=lambda i=i: self.pick_mode(i),
                      bg="#222", fg="#ddd", activebackground="#444", relief="flat",
                      padx=6).pack(side="left", padx=2)
        tk.Button(bar, text="Demo", command=lambda: self.gesture(108), bg="#223", fg="#ddd",
                  relief="flat", padx=6).pack(side="left", padx=(12, 2))
        tk.Button(bar, text="All off", command=self.panic, bg="#322", fg="#ddd",
                  relief="flat", padx=6).pack(side="left", padx=2)

        self.canvas = tk.Canvas(root, height=220, bg="#111", highlightthickness=0)
        self.canvas.pack(fill="both", expand=True, padx=8)
        self.status = tk.Label(root, anchor="w", bg="#111", fg="#aaa", font=("TkFixedFont", 10))
        self.status.pack(fill="x", padx=8, pady=6)

        self.canvas.bind("<Configure>", lambda e: self.draw())
        self.canvas.bind("<ButtonPress-1>", self.mouse_down)
        self.canvas.bind("<B1-Motion>", self.mouse_drag)
        self.canvas.bind("<ButtonRelease-1>", self.mouse_up)
        root.bind("<KeyPress>", self.key_down)
        root.bind("<KeyRelease>", self.key_up)
        root.geometry("1500x330")
        self.connect()
        self.update_status()

    # ------------------------------------------------------------- MIDI out
    def connect(self):
        if self.fd is None:
            dev = self.device_arg or find_teensy()
            if dev:
                try:
                    self.fd = os.open(dev, os.O_WRONLY | os.O_NONBLOCK)
                    self.dev = dev
                except OSError:
                    self.fd = None
        self.update_status()
        self.root.after(2000, self.connect)

    def send(self, *msg):
        if self.fd is None:
            return
        try:
            os.write(self.fd, bytes(msg))
        except OSError:
            os.close(self.fd)
            self.fd = None
            self.update_status()

    def on(self, n, vel=None):
        if n < LOW or n > HIGH:
            return
        self.sounding[n] = self.sounding.get(n, 0) + 1
        if self.sounding[n] == 1:
            self.send(0x90, n, vel or self.vel)
        self.paint(n)

    def off(self, n):
        if n not in self.sounding:
            return
        self.sounding[n] -= 1
        if self.sounding[n] <= 0:
            del self.sounding[n]
            self.send(0x80, n, 0)
        self.paint(n)

    def set_sustain(self, down):
        if down != self.sustain:
            self.sustain = down
            self.send(0xB0, 64, 127 if down else 0)
            self.update_status()

    def panic(self):
        for n in list(self.sounding):
            self.send(0x80, n, 0)
        self.sounding.clear()
        self.keys_down.clear()
        self.set_sustain(False)
        self.send(0xB0, 123, 0)
        self.draw()

    def gesture(self, key):
        """Hold A0 + B0 (very softly) and tap `key`: the wall's own shortcut."""
        self.send(0x90, 21, 1)
        self.send(0x90, 23, 1)
        self.send(0x90, key, 64)

        def release():
            for n in (key, 23, 21):
                self.send(0x80, n, 0)
        self.root.after(80, release)

    def pick_mode(self, i):
        self.gesture(MODE_KEYS[i])
        self.mode_name = MODES[i]
        self.update_status()

    # ------------------------------------------------------------- drawing
    def layout(self):
        w = max(self.canvas.winfo_width(), 400)
        h = max(self.canvas.winfo_height(), 100)
        whites = [n for n in range(LOW, HIGH + 1) if n % 12 not in BLACK]
        kw = w / len(whites)
        rects, x = {}, 0.0
        for n in range(LOW, HIGH + 1):
            if n % 12 in BLACK:
                rects[n] = (x - kw * 0.32, 0, x + kw * 0.32, h * 0.62, True)
            else:
                rects[n] = (x, 0, x + kw, h, False)
                x += kw
        return rects, h

    def draw(self):
        c = self.canvas
        c.delete("all")
        self.rects, self.h = self.layout()
        for black in (False, True):
            for n, (x0, y0, x1, y1, b) in self.rects.items():
                if b != black:
                    continue
                fill = note_color(n) if n in self.sounding else ("#151515" if b else "#e8e8e8")
                c.create_rectangle(x0, y0, x1, y1, fill=fill, outline="#333", tags="k%d" % n)
                if n % 12 == 0 and not b:
                    c.create_text((x0 + x1) / 2, y1 - 12, text="C%d" % (n // 12 - 1), fill="#777",
                                  font=("TkDefaultFont", 8), tags="l%d" % n)
        for row, start in ((LOWER, self.base), (UPPER, self.base + 12)):
            for i, ks in enumerate(row):
                n = start + i
                if n not in self.rects:
                    continue
                x0, y0, x1, y1, b = self.rects[n]
                c.create_text((x0 + x1) / 2, y1 - (8 if b else 28), text=LABEL.get(ks, ks).upper(),
                              fill="#aaa" if b else "#555", font=("TkDefaultFont", 8, "bold"))
        for n in MODE_KEYS:
            x0, y0, x1, y1, b = self.rects[n]
            c.create_line(x0 + 3, y1 - 3, x1 - 3, y1 - 3, fill="#58a")

    def paint(self, n):
        b = n % 12 in BLACK
        fill = note_color(n) if n in self.sounding else ("#151515" if b else "#e8e8e8")
        self.canvas.itemconfigure("k%d" % n, fill=fill)

    def update_status(self):
        dev = getattr(self, "dev", None)
        conn = ("Teensy on %s" % dev) if self.fd is not None else "Teensy not found (retrying)"
        lo = self.base
        txt = "%s  |  keys: %s%d - %s%d  |  velocity %d (Up/Down)  |  sustain %s (Space)" % (
            conn, NAMES[lo % 12], lo // 12 - 1, NAMES[(lo + 31) % 12], (lo + 31) // 12 - 1,
            self.vel, "DOWN" if self.sustain else "up")
        if getattr(self, "mode_name", None):
            txt += "  |  animation: %s" % self.mode_name
        self.status.configure(text=txt)

    # ------------------------------------------------------------- mouse
    def note_at(self, x, y):
        for black in (True, False):
            for n, (x0, y0, x1, y1, b) in self.rects.items():
                if b == black and x0 <= x < x1 and y0 <= y < y1:
                    return n, (y - y0) / (y1 - y0)
        return None, 0

    def mouse_down(self, e):
        n, depth = self.note_at(e.x, e.y)
        if n is not None:
            self.mouse_note = n
            self.on(n, max(1, min(127, int(25 + 102 * depth))))

    def mouse_drag(self, e):
        n, depth = self.note_at(e.x, e.y)
        if n != self.mouse_note:
            if self.mouse_note is not None:
                self.off(self.mouse_note)
            self.mouse_note = n
            if n is not None:
                self.on(n, max(1, min(127, int(25 + 102 * depth))))

    def mouse_up(self, e):
        if self.mouse_note is not None:
            self.off(self.mouse_note)
            self.mouse_note = None

    # ------------------------------------------------------------- keyboard
    def key_note(self, ks):
        ks = ks.lower()
        if ks in LOWER:
            return self.base + LOWER.index(ks)
        if ks in UPPER:
            return self.base + 12 + UPPER.index(ks)
        return None

    def key_down(self, e):
        ks = e.keysym.lower()
        job = self.release_jobs.pop(ks, None)
        if job:  # auto-repeat: the key never really went up
            self.root.after_cancel(job)
            return
        if ks in self.keys_down:
            return
        if ks == "space":
            self.keys_down[ks] = None
            self.set_sustain(True)
            return
        if ks in ("left", "right"):
            self.base = max(LOW - 9, min(HIGH - 31, self.base + (-12 if ks == "left" else 12)))
            self.draw()
        elif ks in ("up", "down"):
            self.vel = max(10, min(127, self.vel + (10 if ks == "up" else -10)))
        n = self.key_note(ks)
        if n is not None:
            self.keys_down[ks] = n
            self.on(n)
        self.update_status()

    def key_up(self, e):
        ks = e.keysym.lower()
        if ks in self.keys_down and ks not in self.release_jobs:
            self.release_jobs[ks] = self.root.after(30, lambda: self.really_up(ks))

    def really_up(self, ks):
        self.release_jobs.pop(ks, None)
        n = self.keys_down.pop(ks, None)
        if ks == "space":
            self.set_sustain(False)
        elif n is not None:
            self.off(n)


if __name__ == "__main__":
    root = tk.Tk()
    Piano(root, sys.argv[1] if len(sys.argv) > 1 else None)
    root.mainloop()
