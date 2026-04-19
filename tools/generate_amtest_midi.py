#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import struct
import sys


PPQ = 480
EIGHTH_TICKS = PPQ // 2
BARS = 100
CHANNELS = 16
NOTES_PER_BAR = [60, 62, 64, 65, 67, 69, 71, 72]  # C4..C5 major scale


def varlen(value: int) -> bytes:
    if value < 0:
        raise ValueError("Variable-length value must be non-negative")
    out = [value & 0x7F]
    value >>= 7
    while value:
        out.insert(0, (value & 0x7F) | 0x80)
        value >>= 7
    return bytes(out)


def chunk(chunk_type: bytes, payload: bytes) -> bytes:
    return chunk_type + struct.pack(">I", len(payload)) + payload


def build_meta_track() -> bytes:
    events = bytearray()
    # 4/4 time signature: nn=4, dd=2 (2^2 = quarter), cc=24, bb=8.
    events += varlen(0) + bytes([0xFF, 0x58, 0x04, 0x04, 0x02, 0x18, 0x08])
    # 120 BPM => 500000 microseconds per quarter note.
    events += varlen(0) + bytes([0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20])
    # Key signature C major: sf=0, mi=0.
    events += varlen(0) + bytes([0xFF, 0x59, 0x02, 0x00, 0x00])
    events += varlen(0) + bytes([0xFF, 0x2F, 0x00])
    return chunk(b"MTrk", bytes(events))


def build_channel_track(channel_one_based: int) -> bytes:
    channel = channel_one_based - 1
    status_pc = 0xC0 | channel
    status_on = 0x90 | channel
    status_off = 0x80 | channel
    velocity = 96

    events = bytearray()
    # Program change first, user-visible PC 1..16 => MIDI program 0..15.
    events += varlen(0) + bytes([status_pc, channel])

    for _ in range(BARS):
        for note in NOTES_PER_BAR:
            events += varlen(0) + bytes([status_on, note, velocity])
            events += varlen(EIGHTH_TICKS) + bytes([status_off, note, 0])

    events += varlen(0) + bytes([0xFF, 0x2F, 0x00])
    return chunk(b"MTrk", bytes(events))


def build_file() -> bytes:
    header = chunk(
        b"MThd",
        struct.pack(">HHH", 1, CHANNELS + 1, PPQ),
    )
    tracks = [build_meta_track()] + [build_channel_track(ch) for ch in range(1, CHANNELS + 1)]
    return header + b"".join(tracks)


def validate_written_file(path: Path) -> None:
    data = path.read_bytes()
    if len(data) < 14:
        raise RuntimeError("MIDI file too small to contain a valid header")
    if data[0:4] != b"MThd":
        raise RuntimeError("Missing MThd header")
    header_len = struct.unpack(">I", data[4:8])[0]
    if header_len != 6:
        raise RuntimeError(f"Unexpected MIDI header length: {header_len}")
    fmt, track_count, division = struct.unpack(">HHH", data[8:14])
    if fmt != 1:
        raise RuntimeError(f"Expected MIDI format 1, got {fmt}")
    if track_count != CHANNELS + 1:
        raise RuntimeError(f"Expected {CHANNELS + 1} tracks, got {track_count}")
    if division != PPQ:
        raise RuntimeError(f"Expected PPQ {PPQ}, got {division}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    output = repo_root / "docs" / "midi" / "amtest.mid"
    if len(sys.argv) > 1:
        output = Path(sys.argv[1]).resolve()

    output.parent.mkdir(parents=True, exist_ok=True)
    midi_bytes = build_file()
    output.write_bytes(midi_bytes)
    validate_written_file(output)

    notes_per_channel = BARS * len(NOTES_PER_BAR)
    print(f"Wrote: {output}")
    print(f"Bytes: {len(midi_bytes)}")
    print(f"Format: 1, Tracks: {CHANNELS + 1}, PPQ: {PPQ}")
    print("Meta: TimeSig=4/4 Tempo=120BPM Key=C")
    print(f"Per channel: PC={1}..{CHANNELS}, notes={notes_per_channel} note-on + {notes_per_channel} note-off")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
