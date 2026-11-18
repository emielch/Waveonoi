#!/usr/bin/env python3
"""Write test MIDI files for the wall (no dependencies).

    python3 sim/make_test_midi.py          -> sim/midi/keys.mid, sim/midi/playing.mid
    aplaymidi -p "Teensy MIDI" sim/midi/keys.mid

keys.mid     calibration: A0, middle C and C8 held 3 s each, then every white
             key from bottom to top. Use it with the `cal` console command.
playing.mid  single notes soft/hard, chords with pedal, a cluster, fast runs
             and pedalled arpeggios: exercises every animation.
"""
import os
import struct

TPQ = 480  # ticks per quarter note; tempo 120 bpm -> 960 ticks per second


def vlq(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append(0x80 | (n & 0x7F))
        n >>= 7
    return bytes(reversed(out))


def write(path, events):
    """events: list of (seconds, bytes)"""
    events = sorted(events, key=lambda e: (e[0], e[1][0] & 0xF0 != 0x80))
    track, last = b"", 0
    for t, msg in events:
        tick = int(round(t * 960))
        track += vlq(tick - last) + msg
        last = tick
    track += b"\x00\xff\x2f\x00"
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, TPQ))
        f.write(b"MTrk" + struct.pack(">I", len(track)) + track)


def note(ev, t, n, vel, dur):
    ev.append((t, bytes([0x90, n, vel])))
    ev.append((t + dur, bytes([0x80, n, 0])))


def pedal(ev, t, down):
    ev.append((t, bytes([0xB0, 64, 127 if down else 0])))


def keys():
    ev = []
    for i, n in enumerate((21, 60, 108)):
        note(ev, 0.5 + i * 3.5, n, 100, 3.0)
    white = [n for n in range(21, 109) if n % 12 in (0, 2, 4, 5, 7, 9, 11)]
    t = 11.5
    for n in white:
        note(ev, t, n, 90, 0.4)
        t += 0.25
    for n in (21, 60, 108):
        note(ev, t + 1, n, 100, 3.0)
    return ev


def playing():
    ev = []
    t = 0.5
    for n, v in ((21, 100), (60, 100), (108, 100), (60, 30), (60, 127)):  # single notes
        note(ev, t, n, v, 0.8)
        t += 1.5
    t += 1
    pedal(ev, t, True)  # consonant chord, pedalled
    for n in (48, 52, 55, 60, 64):
        note(ev, t, n, 90, 2.5)
    pedal(ev, t + 3.5, False)
    t += 4.5
    for n in (60, 61, 62):  # dissonant cluster
        note(ev, t, n, 85, 2.0)
    t += 3.5
    scale = [0, 2, 4, 5, 7, 9, 11]  # C major runs up and down
    run = [60 + 12 * (i // 7) + scale[i % 7] for i in range(15)]
    for n in run + run[-2::-1]:
        note(ev, t, n, 80, 0.09)
        t += 0.1
    t += 1.5
    pedal(ev, t, True)  # arpeggios piling up under the pedal
    arp = [45, 52, 57, 60, 64, 69, 72, 76]
    for k in range(48):
        note(ev, t + k * 0.125, arp[k % 8] + (k // 16) * 2, 60 + (k % 8) * 6, 0.12)
    pedal(ev, t + 6.4, False)
    t += 8.5
    for k, chord in enumerate(((57, 60, 64), (53, 57, 60), (48, 52, 55), (55, 59, 62))):  # staccato chords
        for n in chord:
            note(ev, t + k * 0.6, n, 110, 0.15)
    return ev


if __name__ == "__main__":
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "midi")
    os.makedirs(out, exist_ok=True)
    write(os.path.join(out, "keys.mid"), keys())
    write(os.path.join(out, "playing.mid"), playing())
    print("wrote", os.path.join(out, "keys.mid"), "and", os.path.join(out, "playing.mid"))
