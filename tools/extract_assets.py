#!/usr/bin/env python3
"""Extract assets from a Captain Comic executable (R5sw1991 only).

This tool consolidates the various Go and Python helpers previously used in
`deriv/Makefile` into a single portable script.  It reads the unpacked
R5sw1991 executable plus the companion files in `orig/R5sw1991` and writes
PNG/GIF/WAV outputs into a user-specified directory tree.

Requirements:
    pip install -r tools/requirements.txt

Usage:
    ./tools/extract_assets.py --exe original/COMIC.EXE \
        --orig original --out assets

    ./tools/extract_assets.py --orig original

The output layout mirrors the existing `deriv/R5sw1991` tree:

    assets/
        graphics/       (sys000..sys005.png)
        sprites/        (sprite-*.png)
        shp/            ({base}.shp-{label}-{idx}.png)
        maps/           (base0.pt.png, ...)
        sounds/         (sound-*.wav)
        tiles/          (tt2 tiles as PNGs)

Currently the script only knows about R5sw1991; the metadata is hard-coded
but structured so additional versions can be added later.
"""

import argparse
import io
import os
import struct
import sys
import wave
from pathlib import Path

from PIL import Image

# global behaviour flags set by main
FORCE = False

# -----------------------------------------------------------------------------
# constants for R5sw1991
# -----------------------------------------------------------------------------

# simple 16‑colour EGA palette (same order as programs/ega/graphic.go)
PALETTE = [
    (0x00, 0x00, 0x00), (0x00, 0x00, 0xaa), (0x00, 0xaa, 0x00),
    (0x00, 0xaa, 0xaa), (0xaa, 0x00, 0x00), (0xaa, 0x00, 0xaa),
    (0xaa, 0x55, 0x00), (0xaa, 0xaa, 0xaa), (0x55, 0x55, 0x55),
    (0x55, 0x55, 0xff), (0x55, 0xff, 0x55), (0x55, 0xff, 0xff),
    (0xff, 0x55, 0x55), (0xff, 0x55, 0xff), (0xff, 0xff, 0x55),
    (0xff, 0xff, 0xff),
]

# sprite area slider
SPRITES_START = 0x9d00

# offsets for every known sprite within the sprite block (relative).
# values taken directly from the deriv Makefile, reproduced as Python.
SPRITE_ENTRIES = []

# helper to register a sprite

def _add_sprite(name, offset, size, mask):
    SPRITE_ENTRIES.append({
        "name": name,
        "offset": offset,
        "width": size[0],
        "height": size[1],
        "mask": mask,
    })

# even/odd item graphics
_item_order = [
    "corkscrew", "doorkey", "boots", "lantern", "teleportwand",
    "gems", "crown", "gold", "cola",
]
_BASE = 0x2dc4
for idx, nm in enumerate(_item_order):
    _add_sprite(f"{nm}_even", _BASE + 160 * idx, (16, 16), True)

# Blastola Cola inventory icons (firepower count 1-5).
# In the original data these are stored in the five 16x16 masked slots
# immediately after the base cola icon.
for firepower in range(1, 6):
    _add_sprite(
        f"cola_inventory_{firepower}_even",
        _BASE + 160 * (8 + firepower),
        (16, 16),
        True,
    )

# shield_even is at position 14
_add_sprite("shield_even", _BASE + 160 * 14, (16, 16), True)
# life icon
_add_sprite("life_icon_bright", _BASE + 160 * 15, (16, 16), True)
# odd set begins 0xa00 bytes later (16*160)
for idx, nm in enumerate(_item_order):
    _add_sprite(f"{nm}_odd", _BASE + 0xa00 + 160 * idx, (16, 16), True)

for firepower in range(1, 6):
    _add_sprite(
        f"cola_inventory_{firepower}_odd",
        _BASE + 0xa00 + 160 * (8 + firepower),
        (16, 16),
        True,
    )

_add_sprite("shield_odd", _BASE + 0xa00 + 160 * 14, (16, 16), True)
_add_sprite("life_icon_dark", _BASE + 0xa00 + 160 * 15, (16, 16), True)

# comic animations
for i, nm in enumerate([
    "comic_standing_right", "comic_running_1_right",
    "comic_running_2_right", "comic_running_3_right",
    "comic_jumping_right", "comic_standing_left",
    "comic_running_1_left", "comic_running_2_left",
    "comic_running_3_left", "comic_jumping_left",
]):
    # these character animations all include a 32‑byte mask after the four
    # EGA planes; without it the extracted PNGs are filled with the background
    # colour and look like a solid rectangle.
    _add_sprite(nm, 0x0000 + 320 * i, (16, 32), True)

# pause / game over
# In the R5sw1991 executable these two graphics are stored back-to-back with
# no intervening mask data.  the nominal mask area would start exactly where
# the next sprite's plane data begins, so enabling ``mask=True`` causes the
# extractor to treat the following graphic as a mask.  the result is spurious
# transparency.  (earlier we observed nonzero bytes in that region because they
# belong to the *next* image.)
# Other versions of the game may include explicit masks, which is why the Go
# helper had ``mask=True``; here we override that assumption.
_add_sprite("pause", 0x4504, (128, 48), False)
_add_sprite("game_over", 0x5104, (128, 48), False)

# animation sequences with simple loops

def _range(name, start, count, step=320, size=(16, 32), mask=True):
    # previously this helper defaulted to False; almost every range used in
    # the R5 data actually contains a mask.  change the default so callers do
    # not have to think about it and accidental omissions are harder to make.
    for i in range(count):
        _add_sprite(f"{name}_{i}", start + i * step, size, mask)

_range("comic_death", 0x10e0, 8)
_range("teleport", 0x1ae0, 3)
_range("materialize", 0x1ea0, 12)
_range("fireball", 0x0c80, 2, step=80, size=(16, 8))
_range("white_spark", 0x0d20, 3)
_range("red_spark", 0x0f00, 3)

# score digits and meter bars
for i in range(10):
    _add_sprite(f"score_digit_{i}", 0x41c4 + i * 64, (8, 16), False)
_add_sprite("meter_empty", 0x4444, (8, 16), False)
_add_sprite("meter_half", 0x4484, (8, 16), False)
_add_sprite("meter_full", 0x44c4, (8, 16), False)

# -----------------------------------------------------------------------------
# sound offsets
# -----------------------------------------------------------------------------
SOUND_OFFSETS = {
    "title": 0x29c4,
    "door": 0xfc29,
    "death": 0xfc51,
    "teleport": 0xfc79,
    "unknown": 0xfc99,
    "materialize": 0xfca5,
    "gameover": 0xfd05,
    "transition": 0xfd3f,
    "toobad": 0xfd5f,
    "shoot": 0xfdae,
    "pop": 0xfdba,
    "damage": 0xfdd1,
    "itemget": 0xfde1,
    "wereinthemoney": 0xfdf5,
}

# -----------------------------------------------------------------------------
# helper functions
# -----------------------------------------------------------------------------

def _make_palette():
    """Return a flat palette suitable for `Image.putpalette`."""
    pal = []
    for r, g, b in PALETTE:
        pal.extend((r, g, b))
    # pad to 768 entries
    pal += [0] * (768 - len(pal))
    return pal


def decode_ega_planes(data: bytes, rle: bool = True) -> bytes:
    """Return the 4-plane decoded buffer (320×200 EGA) from raw bytes."""
    br = io.BytesIO(data)
    if rle:
        plane_size = struct.unpack("<H", br.read(2))[0]
        if plane_size != 8000:
            raise ValueError(f"bad plane size {plane_size}")
    else:
        plane_size = 8000
    planes = bytearray(4 * plane_size)
    for i in range(4):
        if rle:
            out = bytearray()
            while len(out) < plane_size:
                c = br.read(1)
                if not c:
                    raise EOFError("unexpected EOF in RLE data")
                c = c[0]
                if c < 128:
                    chunk = br.read(c)
                    if len(chunk) != c:
                        raise EOFError("unexpected EOF in RLE data")
                    out += chunk
                else:
                    rep_byte = br.read(1)
                    if not rep_byte:
                        raise EOFError("unexpected EOF in RLE data")
                    rep = rep_byte[0]
                    out += bytes([rep]) * (c - 128)
            planes[i * plane_size : (i + 1) * plane_size] = out[:plane_size]
        else:
            planes[i * plane_size : (i + 1) * plane_size] = br.read(plane_size)
    return bytes(planes)


def planes_to_image(planes: bytes, width: int, height: int) -> Image.Image:
    """Convert a 4‑plane buffer to a paletted PIL image."""
    im = Image.new("P", (width, height))
    im.putpalette(_make_palette())
    for y in range(height):
        for x in range(width):
            q = (y * width + x) // 8
            r = (y * width + x) % 8
            idx = 0
            for p in range(4):
                byte = planes[p * (width * height // 8) + q]
                bit = (byte >> (7 - r)) & 1
                idx |= bit << p
            im.putpixel((x, y), idx)
    return im


def find_case_insensitive(dirname: str, name: str) -> str:
    for entry in os.listdir(dirname):
        if entry.lower() == name.lower():
            return os.path.join(dirname, entry)
    raise FileNotFoundError(name)

# -----------------------------------------------------------------------------
# map / tileset helpers
# -----------------------------------------------------------------------------

def read_tt2_file(path: str):
    with open(path, "rb") as f:
        header = f.read(4)
        last_pass = header[0]
        tiles = []
        while True:
            buf = f.read(128)
            if not buf or len(buf) < 128:
                break
            tiles.append((len(tiles) <= last_pass, buf))
    return tiles


def read_pt_file(path: str):
    with open(path, "rb") as f:
        w, h = struct.unpack("<HH", f.read(4))
        tiles = list(f.read(w * h))
    return w, h, tiles

# -----------------------------------------------------------------------------
# CTL: exe parsing for level metadata & shp load
# -----------------------------------------------------------------------------

class ExeFile:
    def __init__(self, header, relocs, sections, data):
        self.header = header
        self.relocs = relocs
        self.sections = sections
        self.data = data


# -----------------------------------------------------------------------------
# EXEPACK decompressor (ported from rando/src/exepack.rs)
# -----------------------------------------------------------------------------


class ExepackError(Exception):
    """A problem occurred while trying to unpack an EXEPACK-compressed file."""


def _peek_u16le(buf: bytes, offset: int) -> int:
    return buf[offset] | (buf[offset + 1] << 8)


def _parse_exepack_header(buf: bytes) -> dict:
    # header must be 16 or 18 bytes long
    if len(buf) == 16:
        has_skip = False
    elif len(buf) == 18:
        has_skip = True
    else:
        raise ExepackError(f"HeaderLength {len(buf)}")
    real_ip = _peek_u16le(buf, 0)
    real_cs = _peek_u16le(buf, 2)
    # mem_start (offset 4) ignored
    exepack_len = _peek_u16le(buf, 6)
    real_sp = _peek_u16le(buf, 8)
    real_ss = _peek_u16le(buf, 10)
    uncompressed_len = _peek_u16le(buf, 12) * 16
    if has_skip:
        skip_len = _peek_u16le(buf, 14)
        signature = buf[16:18]
    else:
        skip_len = 1
        signature = buf[14:16]
    if signature != b"RB":
        raise ExepackError(f"Bad signature {signature!r}")
    return {
        "real_ip": real_ip,
        "real_cs": real_cs,
        "real_sp": real_sp,
        "real_ss": real_ss,
        "exepack_len": exepack_len,
        "uncompressed_len": uncompressed_len,
        "skip_len": skip_len,
    }


def _find_packed_relocation_table(buf: bytes):
    # returns index or None
    PATTERN = b"\xcd\x21\xb8\xff\x4c\xcd\x21"
    ERR_LEN = 22
    limit = len(buf) - ERR_LEN
    for i in range(limit, -1, -1):
        if buf[:i].endswith(PATTERN):
            return i + ERR_LEN
    return None


def _parse_packed_relocation_table(buf: bytes) -> list:
    relocs = []
    i = 0
    for segment in range(0, 0x1000 * 16, 0x1000):
        if i + 2 > len(buf):
            raise ExepackError("TooShort")
        n = _peek_u16le(buf, i)
        i += 2
        for _ in range(n):
            if i + 2 > len(buf):
                raise ExepackError("TooShort")
            offset = _peek_u16le(buf, i)
            i += 2
            relocs.append((segment, offset))
    if i != len(buf):
        raise ExepackError("Stub")
    return relocs


def _unpad(buf: bytearray, i: int) -> int:
    # remove up to 15 trailing 0xff bytes
    count = 0
    while count < 15 and i - count - 1 >= 0 and buf[i - count - 1] == 0xFF:
        count += 1
    return i - count


def _decompress_exepack(buf: bytearray, uncompressed_len: int):
    # returns modified buffer (same object) or raises on error
    compressed_len = len(buf)
    src = compressed_len
    dst = uncompressed_len
    if len(buf) < dst:
        buf.extend(b"\x00" * (dst - len(buf)))
    src = _unpad(buf, src)
    while True:
        if src == 0:
            raise ExepackError("SrcUnderflow")
        src -= 1
        command = buf[src]
        if src < 2:
            raise ExepackError("SrcUnderflow")
        src -= 2
        length = buf[src] | (buf[src + 1] << 8)
        if command & 0xFE == 0xB0:
            if src == 0:
                raise ExepackError("SrcUnderflow")
            src -= 1
            fill = buf[src]
            dst -= length
            if dst < 0:
                raise ExepackError("DstUnderflow")
            for i in range(length):
                buf[dst + i] = fill
        elif command & 0xFE == 0xB2:
            if src < length:
                raise ExepackError("SrcUnderflow")
            src -= length
            dst -= length
            if dst < 0:
                raise ExepackError("DstUnderflow")
            for i in range(length - 1, -1, -1):
                buf[dst + i] = buf[src + i]
        else:
            raise ExepackError(f"Unknown command {command:#02x}")
        if command & 1:
            break
    if compressed_len < dst:
        raise ExepackError(f"Gap of {dst - compressed_len} bytes")
    del buf[uncompressed_len:]
    return buf


def maybe_unpack_exe_bytes(raw: bytes) -> bytes:
    """If *raw* appears to be EXEPACK compressed return a new buffer with
    the decompressed data; otherwise return *raw* unchanged.
    """
    try:
        exe = read_exe_file(raw)
    except Exception:
        return raw
    if exe.relocs:
        # decompress routine does not handle relocations
        return raw
    cs = exe.header[11]
    ip = exe.header[10]
    data = exe.data
    # split the data region into the portion *before* the EXEPACK header and
    # the portion starting with the header itself.  the Rust implementation
    # used `Vec::split_off` for this; mirror that behaviour here.
    if cs * 16 > len(data) or ip > len(data):
        return raw
    prefix = data[: cs * 16]
    suffix = data[cs * 16 :]
    exepack_header = suffix[:ip]
    decompression_stub = suffix[ip:]
    # the EXEPACK header parser throws ExepackError when the signature is
    # missing or the header length is wrong.  that simply means the executable
    # isn't packed; treat it as a no-op rather than bubbling the exception up
    # and causing a misleading warning later.
    try:
        header = _parse_exepack_header(exepack_header)
    except ExepackError:
        return raw
    if header["exepack_len"] < len(exepack_header):
        raise ExepackError("TooShort")
    decompression_stub = decompression_stub[: header["exepack_len"] - len(exepack_header)]
    offset = _find_packed_relocation_table(decompression_stub)
    if offset is None:
        return raw
    packed_relocs = decompression_stub[offset:]
    try:
        _parse_packed_relocation_table(packed_relocs)
    except ExepackError:
        return raw
    skip = 16 * (header["skip_len"] - 1)
    if skip > len(prefix):
        raise ExepackError("SkipLen")
    compressed_len = len(prefix) - skip
    uncompressed_len = header["uncompressed_len"] - skip
    if uncompressed_len < 0:
        raise ExepackError("SkipLen")
    buf = bytearray(prefix[:compressed_len])
    buf = _decompress_exepack(buf, uncompressed_len)
    data_offset = exe.header[4] * 16

    # Rebuild the EXE buffer and update the MZ header size fields
    # (num_last, num_blocks) to match the new file size. This ensures that
    # any later call to read_exe_file() on the returned buffer will see the
    # correct data region size.
    header_bytes = bytearray(raw[:data_offset])
    new_size = len(header_bytes) + len(buf)
    full_pages, remainder = divmod(new_size, 512)
    if remainder == 0:
        num_blocks = full_pages
        num_last = 0
    else:
        num_blocks = full_pages + 1
        num_last = remainder
    # MZ header layout: "<2sHHHHHHHHHHHHH"
    # Offsets: magic (0-1), num_last (2-3), num_blocks (4-5), ...
    struct.pack_into("<H", header_bytes, 2, num_last)
    struct.pack_into("<H", header_bytes, 4, num_blocks)

    return bytes(header_bytes) + bytes(buf)
def read_exe_file(raw: bytes) -> ExeFile:
    # port of programs/exe/exe.go:Read
    header_fmt = "<2sHHHHHHHHHHHHH"
    header_size = struct.calcsize(header_fmt)
    if len(raw) < header_size:
        raise ValueError(
            f"EXE header too short: expected at least {header_size} bytes, "
            f"got {len(raw)}"
        )
    hdr = struct.unpack_from(header_fmt, raw, 0)
    (magic, num_last, num_blocks, num_relocs, num_paras, minalloc,
     maxalloc, initSS, initSP, checksum, initIP, initCS,
     relocOffset, overlay) = hdr

    data_offset = num_paras * 16
    data_length = num_blocks * 512
    if num_last != 0:
        data_length -= 512 - num_last
    data = raw[data_offset : data_offset + data_length]

    # read relocations
    relocs = []
    # Ensure relocation table lies within the raw buffer; truncate if malformed
    if relocOffset > len(raw):
        # relocation table starts beyond end of file; treat as no relocations
        effective_num_relocs = 0
        print(
            f"Warning: relocation offset {relocOffset} beyond file size {len(raw)}; "
            "skipping relocations",
            file=sys.stderr,
        )
    else:
        max_relocs = (len(raw) - relocOffset) // 4
        if num_relocs > max_relocs:
            effective_num_relocs = max_relocs
            print(
                f"Warning: relocation count {num_relocs} truncated to {effective_num_relocs} "
                f"due to file size {len(raw)}",
                file=sys.stderr,
            )
        else:
            effective_num_relocs = num_relocs

    for i in range(effective_num_relocs):
        offset, segment = struct.unpack_from("<HH", raw, relocOffset + 4 * i)
        relocs.append((offset, segment))

    # infer sections similar to Go
    sections = [0]
    for offset, segment in relocs:
        linear = segment * 16 + offset
        if linear + 1 >= len(raw):
            continue
        val = raw[linear] | (raw[linear + 1] << 8)
        if val not in sections:
            sections.append(val)
    sections.sort()
    sections = [s * 16 for s in sections]

    # sections are offsets into the data region; adjust by subtracting data_offset
    sections = [s - data_offset for s in sections]

    return ExeFile(header=hdr, relocs=relocs, sections=sections, data=data)


def read_level_metadata(exe_data: bytes, orig_dir: str):
    """Parse the eight-level metadata table directly from the EXE bytes.

    The table isn't anchored at a fixed offset in R5, so we search for the
    first 14-byte field that matches one of the TT2 filenames actually present
    in `orig_dir`.
    """
    tt2names = [f.lower() for f in os.listdir(orig_dir) if f.lower().endswith(".tt2")]
    if not tt2names:
        raise ValueError("no .tt2 files found in orig directory")
    start = None
    for i in range(len(exe_data) - 14):
        field = exe_data[i : i + 14]
        try:
            name = field.rstrip(b" \x00").decode("ascii").lower()
        except UnicodeDecodeError:
            continue
        if name in tt2names:
            start = i
            break
    if start is None:
        raise ValueError("could not locate level metadata in executable")
    buf = exe_data[start:]
    levels = []
    off = 0
    def require(n):
        if off + n > len(buf):
            raise ValueError("level metadata truncated or malformed")
    for _ in range(8):
        require(14)
        tt2 = buf[off : off + 14].rstrip(b" \x00").decode("ascii")
        off += 14
        stages = []
        for _ in range(3):
            require(14)
            stages.append(buf[off : off + 14].rstrip(b" \x00").decode("ascii"))
            off += 14
        require(4)
        off += 4  # door tiles
        shps = []
        for _ in range(4):
            require(17)
            numdist = buf[off]
            horiz = buf[off + 1]
            anim = buf[off + 2]
            fname = buf[off + 3 : off + 17].rstrip(b" \x00").decode("ascii")
            shps.append({
                "numdistinct": numdist,
                "horizontal": horiz,
                "animation": anim,
                "filename": fname,
            })
            off += 17
        require(25 * 3)
        off += 25 * 3
        require(5)
        off += 5
        levels.append({"tt2": tt2, "stage": stages, "shps": shps})
    return levels

# -----------------------------------------------------------------------------
# shp decoding
# -----------------------------------------------------------------------------

def _load_shp_frames(shp_path: str, levels_meta):
    basename = os.path.basename(shp_path).lower()
    info = None
    for lvlnum, lvl in enumerate(levels_meta):
        for shpnum, shp in enumerate(lvl.get("shps", [])):
            if shp["filename"].lower() != basename:
                continue
            if info is None:
                info = (lvlnum, shpnum, shp)
            else:
                if info[2] != shp:
                    print(f"warning: {shp_path}: SHP data mismatch; using first instance")
    if info is None:
        raise ValueError(f"no SHP metadata for {basename}")

    numframes = info[2]["numdistinct"]
    horiz = info[2]["horizontal"]
    anim = info[2]["animation"]

    with open(shp_path, "rb") as f:
        data = f.read()

    # If frames are stored separately for left/right, the two sets
    # appear back-to-back.  In that case double the amount we read into
    # "graphics" so that right-facing frames are available there.
    readcount = 160 * numframes
    if horiz == 2:  # shpHorizontalSeparate
        readcount *= 2
    graphics = data[:readcount]
    extra = []
    tail = data[readcount:]
    for i in range(0, len(tail), 160):
        extra.append(tail[i : i + 160])
    # generate left/right/extra according to rules in Go version
    left_frames = []
    p = 0
    for i in range(numframes):
        left_frames.append(graphics[p * 160 : p * 160 + 160])
        p += 1
    if anim != 0:  # not loop
        left_frames.append(left_frames[-2])
    right_frames = []
    if horiz != 2:
        p = 0
    for i in range(numframes):
        right_frames.append(graphics[p * 160 : p * 160 + 160])
        p += 1
    if anim != 0:
        right_frames.append(right_frames[-2])
    return left_frames, right_frames, extra


def _mask_to_alpha(maskdata: bytes, w: int, h: int) -> Image.Image:
    """Convert a mask buffer to a grayscale alpha image.

    The mask format is the same one used by the Go program
    `unpack-game-graphic` – there is one bit per pixel, packed left‑to‑right,
    top‑to‑bottom.  A bit value of **1** means the corresponding pixel should be
    **transparent**.  The original implementation in this script inverted the
    value in the same way, but it only ever worked for 16×16 frames and made no
    attempt to round the mask size.  The code below mirrors the Go logic exactly
    and calculates the proper byte count so that non‑standard widths/heights
    (e.g. 128×48) are handled correctly.
    """
    mask_size = (w * h + 7) // 8
    if len(maskdata) < mask_size:
        # protect against truncated data; fill remainder opaque
        maskdata = maskdata.ljust(mask_size, b"\x00")
    alpha = Image.new("L", (w, h))
    for y in range(h):
        for x in range(w):
            q = (y * w + x) // 8
            r = (y * w + x) % 8
            bit = (maskdata[q] >> (7 - r)) & 1
            # bit==1 => transparent (alpha=0); bit==0 => opaque (alpha=255)
            alpha.putpixel((x, y), 0 if bit else 255)
    return alpha


def _frames_to_png(frames, out_dir, base, label):
    """Save each frame as a separate PNG file.

    The filenames will be ``{base}.shp-{label}-{i}.png`` where ``i`` is the
    frame index.  ``label`` is typically "left", "right" or "extra".
    """
    if not frames:
        return
    os.makedirs(out_dir, exist_ok=True)
    for idx, frame in enumerate(frames):
        # SHP frames are 16×16; the first 128 bytes are the four EGA planes and
        # the following bytes (usually 32) are the mask.  If additional trailing
        # data is ever present we ignore it.
        planes = frame[:128]
        maskdata = frame[128:]
        im = planes_to_image(planes, 16, 16).convert("RGBA")
        if maskdata:
            alpha = _mask_to_alpha(maskdata, 16, 16)
            im.putalpha(alpha)
        filename = os.path.join(out_dir, f"{base}.shp-{label}-{idx}.png")
        if os.path.exists(filename) and not FORCE:
            continue
        im.save(filename)


def extract_shp(orig_dir, exe_data, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    levels_meta = read_level_metadata(exe_data, orig_dir)
    for fname in os.listdir(orig_dir):
        if fname.lower().endswith(".shp"):
            shp_path = os.path.join(orig_dir, fname)
            left, right, extra = _load_shp_frames(shp_path, levels_meta)
            base = os.path.splitext(fname.lower())[0]
            if left:
                _frames_to_png(left, out_dir, base, "left")
            if right:
                _frames_to_png(right, out_dir, base, "right")
            if extra:
                _frames_to_png(extra, out_dir, base, "extra")

# -----------------------------------------------------------------------------
# sound helpers
# -----------------------------------------------------------------------------

def _read_uint16(buf, pos):
    return buf[pos] | (buf[pos + 1] << 8)


def _read_freq_table(exe_data, offset):
    t = []
    pos = offset
    while True:
        if pos + 2 > len(exe_data):
            raise ValueError("EOF reading freq")
        freq = _read_uint16(exe_data, pos)
        pos += 2
        if freq == 0:
            break
        duration = _read_uint16(exe_data, pos)
        pos += 2
        t.extend([freq] * duration)
    return t


def _save_sound(freq_table, filename):
    BASE_FREQ = 1193182.0
    DIV_FREQ = BASE_FREQ / 65536.0
    FRAME_RATE = 48000
    c = 0
    p = 0.0
    # use context manager to ensure file is closed on exception
    with wave.open(filename, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(48000)
        while True:
            s = c / FRAME_RATE
            i = int(s * DIV_FREQ)
            if i >= len(freq_table):
                break
            if freq_table[i] <= 0x28:
                x = 0
            else:
                # simple square wave with symmetric amplitude
                x = 32767 if p % 1.0 >= 0.5 else -32767
            w.writeframesraw(bytes([x & 0xff, (x >> 8) & 0xff]))
            if freq_table[i] <= 0x28:
                cur_freq = 0
            else:
                cur_freq = BASE_FREQ / freq_table[i]
            p = (p + cur_freq / FRAME_RATE) % 1.0
            c += 1

# -----------------------------------------------------------------------------
# high-level operations
# -----------------------------------------------------------------------------

def _maybe_save_image(img: Image.Image, path: str):
    """Save the PIL image unless it already exists and force flag is disabled."""
    if os.path.exists(path) and not FORCE:
        return
    img.save(path)


def extract_graphics(orig_dir, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for name in [f"SYS00{i}.EGA" for i in range(6)]:
        src = os.path.join(orig_dir, name)
        if not os.path.exists(src):
            continue
        with open(src, "rb") as f:
            data = f.read()
        planes = decode_ega_planes(data, rle=True)
        img = planes_to_image(planes, 320, 200)
        base = os.path.splitext(name)[0].lower()  # e.g. sys000
        dest = os.path.join(out_dir, f"{base}.ega.png")
        _maybe_save_image(img, dest)


def extract_sprites(exe_data, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for entry in SPRITE_ENTRIES:
        offset = SPRITES_START + entry["offset"] + 512
        w = entry["width"]
        h = entry["height"]
        use_mask = entry["mask"]
        # mask size must round *up* to the next whole byte; Go code does
        # integer division which truncates, but the stored data is always
        # padded, so compute the same value for consistency.
        planes_len = w * h // 2
        mask_size = (w * h + 7) // 8 if use_mask else 0
        length = planes_len + mask_size
        chunk = exe_data[offset : offset + length]
        if len(chunk) < planes_len:
            continue
        planes = chunk[:planes_len]
        maskdata = chunk[planes_len : planes_len + mask_size] if use_mask else None
        im = planes_to_image(planes, w, h).convert("RGBA")
        if use_mask and maskdata:
            alpha = _mask_to_alpha(maskdata, w, h)
            im.putalpha(alpha)
        dest = os.path.join(out_dir, f"sprite-{entry['name']}.png")
        if os.path.exists(dest) and not FORCE:
            continue
        im.save(dest)


def extract_sounds(exe_data, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for name, offs in SOUND_OFFSETS.items():
        freq_table = _read_freq_table(exe_data, offs)
        _save_sound(freq_table, os.path.join(out_dir, f"sound-{name}.wav"))


# map / tile / shp extraction

def extract_tiles(orig_dir, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for fname in os.listdir(orig_dir):
        if fname.lower().endswith(".tt2"):
            tiles = read_tt2_file(os.path.join(orig_dir, fname))
            basename = os.path.splitext(fname.lower())[0]
            for idx, (passable, planes) in enumerate(tiles):
                im = planes_to_image(planes, 16, 16)
                dest = os.path.join(out_dir, f"{basename}.tt2-{idx:02x}.png")
                if os.path.exists(dest) and not FORCE:
                    continue
                im.save(dest)


def extract_maps(exe_data, orig_dir, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    levels_meta = read_level_metadata(exe_data, orig_dir)
    for lvl_idx, lvl in enumerate(levels_meta):
        name = os.path.splitext(lvl["tt2"])[0].lower()
        try:
            tt2path = find_case_insensitive(orig_dir, lvl["tt2"])
        except FileNotFoundError:
            continue
        tileset = read_tt2_file(tt2path)
        for stage_idx, fname in enumerate(lvl["stage"]):
            try:
                ptpath = find_case_insensitive(orig_dir, fname)
            except FileNotFoundError:
                continue
            w, h, tiles = read_pt_file(ptpath)
            im = Image.new("RGBA", (w * 16, h * 16))
            for i, tidx in enumerate(tiles):
                x = i % w
                y = i // w
                # Skip tiles whose indices are out of range for this tileset.
                # Malformed or corrupted PT files may contain invalid indices.
                if not isinstance(tidx, int) or tidx < 0 or tidx >= len(tileset):
                    continue
                _, planes = tileset[tidx]
                tileim = planes_to_image(planes, 16, 16).convert("RGBA")
                im.paste(tileim, (x * 16, y * 16))
            out_name = f"{name}{stage_idx}.pt.png"
            dest = os.path.join(out_dir, out_name)
            if os.path.exists(dest) and not FORCE:
                continue
            im.save(dest)

# -----------------------------------------------------------------------------
# update main() to call the new routines
# -----------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", required=False,
        help=(
            "(optional) path to the executable; if omitted the script will"
            " search the orig directory for an .exe (preferably COMIC.EXE)"
        ),
    )
    parser.add_argument("--orig", required=True, help="path to orig/R5sw1991 or similar")
    parser.add_argument("--out", required=False, default="assets", help="output directory")
    parser.add_argument("--force", action="store_true",
                        help="force regeneration of output files")
    parser.add_argument("--clean", action="store_true",
                        help="remove existing output directory before extracting")
    args = parser.parse_args()

    global FORCE
    exe_path = args.exe
    orig_dir = args.orig
    out_root = args.out
    FORCE = args.force

    if args.clean:
        # delete output directory entirely before extraction
        import shutil
        shutil.rmtree(out_root, ignore_errors=True)

    # if the caller didn't supply an explicit executable, look for one in the
    # orig directory.  by convention the game exe is called COMIC.EXE, so
    # prefer that name (case-insensitive) and fall back to any other `.exe`.
    if not exe_path:
        candidates = [f for f in os.listdir(orig_dir)
                      if f.lower().endswith(".exe")]
        if not candidates:
            raise FileNotFoundError("no executable found in orig directory")
        # prefer COMIC.EXE
        for c in candidates:
            if c.lower() == "comic.exe":
                exe_path = os.path.join(orig_dir, c)
                break
        else:
            candidates.sort(key=lambda s: s.lower())
            exe_path = os.path.join(orig_dir, candidates[0])
        print(f"auto-detected executable '{exe_path}'")

    with open(exe_path, "rb") as f:
        exe_data = f.read()

    # automatically decompress if the executable is EXEPACK-compressed.  this
    # relieves the user from installing a separate `exepack` binary or the
    # Rust toolchain; the implementation is baked into this script.
    try:
        new_data = maybe_unpack_exe_bytes(exe_data)
    except ExepackError as e:
        print(f"warning: failed to unpack EXEPACK header: {e}")
        new_data = exe_data
    if new_data is not exe_data:
        print("detected and decompressed EXEPACK executable")
        exe_data = new_data

    print("Extracting graphics...")
    extract_graphics(orig_dir, os.path.join(out_root, "graphics"))

    print("Extracting sprites...")
    extract_sprites(exe_data, os.path.join(out_root, "sprites"))

    print("Extracting sounds...")
    extract_sounds(exe_data, os.path.join(out_root, "sounds"))

    print("Extracting tiles.tt2...")
    extract_tiles(orig_dir, os.path.join(out_root, "tiles"))

    print("Extracting maps...")
    extract_maps(exe_data, orig_dir, os.path.join(out_root, "maps"))

    print("Extracting shp files...")
    extract_shp(orig_dir, exe_data, os.path.join(out_root, "shp"))


if __name__ == "__main__":
    main()
