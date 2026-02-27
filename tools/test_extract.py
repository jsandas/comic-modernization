import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

from PIL import Image

class ExtractAssetsTest(unittest.TestCase):
    def setUp(self):
        # paths relative to repo root
        self.repo = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
        # extraction script lives in tools/
        self.script = os.path.join(self.repo, "tools", "extract_assets.py")
        # original game files must be placed in the `original/` directory
        # (see README.md for instructions); tests will be skipped if absent.
        self.orig = os.path.join(self.repo, "original")
        self.outdir = tempfile.mkdtemp()
        if not os.path.isdir(self.orig):
            self.skipTest("original game files not available; download as documented in README.md")

        # import helper routines from the script for internal testing
        import importlib.util
        spec = importlib.util.spec_from_file_location("extract", self.script)
        self.extract_mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.extract_mod)

    def tearDown(self):
        shutil.rmtree(self.outdir)

    def test_extract_r5(self):
        # run the extraction script (will auto-detect COMIC.EXE in orig)
        subprocess.check_call([sys.executable, self.script,
                               "--orig", self.orig,
                               "--out", self.outdir])
        # check that a handful of expected outputs exist
        graphic = os.path.join(self.outdir, "graphics", "sys000.ega.png")
        self.assertTrue(os.path.exists(graphic))
        self.assertTrue(os.path.exists(os.path.join(self.outdir, "sprites", "sprite-comic_standing_right.png")))
        self.assertTrue(os.path.exists(os.path.join(self.outdir, "sounds", "sound-title.wav")))
        self.assertTrue(os.path.exists(os.path.join(self.outdir, "maps", "base0.pt.png")))
        # expect at least one frame extracted for the ball sprite
        self.assertTrue(os.path.exists(os.path.join(self.outdir, "shp", "ball.shp-left-0.png")))
        self.assertTrue(os.path.exists(os.path.join(self.outdir, "tiles", "base.tt2-00.png")))

        # also exercise the decompressor directly on a tiny synthetic payload
        # single fill command for 16 bytes
        buf = bytearray([ord("A"), 0x10, 0x00, 0xB1])
        decompressed = self.extract_mod._decompress_exepack(buf, 16)
        self.assertEqual(decompressed, b"A" * 16)

    def test_exepack_helpers(self):
        # uncompressed data should return itself
        raw = b"garbage"
        self.assertEqual(self.extract_mod.maybe_unpack_exe_bytes(raw), raw)

        # build a minimal fake EXE that is EXEPACK-compressed and see that the
        # helper actually expands it.  layout:
        #   DOS header (28 bytes, mostly zeros)
        #   exepack header (18 bytes) + minimal stub + relocation table
        #   compressed payload (simple fill command from above)
        # we want the EXEPACK header to be located at cs*16 relative to the
        # start of the *data* region, and `read_exe_file` defines the data
        # region to begin at `num_paras * 16` bytes into the file.  the DOS
        # header is 28 bytes long, so bump `num_paras` to 2 (32-byte header) and
        # insert four padding bytes between the DOS header and the exepack
        # structure.
        num_paras = 2
        # build the DOS header, leaving space for two paragraphs
        header = struct.pack(
            "<2sHHHHHHHHHHHHH",
            b"MZ",          # magic
            0,               # num_last (will fill later)
            0,               # num_blocks (filled after we know size)
            0,               # num_relocs
            num_paras,       # num_paras (data offset=32)
            0, 0,            # min/max alloc
            0, 0,            # ss, sp
            0,               # checksum
            18,              # initIP -> exepack header length
            0,               # initCS -> offset 0 (relative to data)
            28,              # relocOffset (unused)
            0,               # overlay
        )
        # build the exepack header and stub, then fill in the size fields
        PAT = b"\xcd\x21\xb8\xff\x4c\xcd\x21"
        stub = PAT + b"x" * 22 + (b"\x00" * (2 * 16))
        exehdr = bytearray(18)
        # header size + stub length
        total_block = 18 + len(stub)
        exehdr[6:8] = total_block.to_bytes(2, "little")
        # uncompressed length must be a multiple of 16; we'll request
        # sixteen bytes here (1 paragraph).  The header stores the value in
        # paragraphs, so store 1.
        exehdr[12:14] = (1).to_bytes(2, "little")
        exehdr[14:16] = (1).to_bytes(2, "little")
        exehdr[16:18] = b"RB"
        # length 16 (0x0010) with termination flag set in command
        payload = bytearray([ord("A"), 0x10, 0x00, 0xB1])
        # put the compressed payload *before* the header; the CS field
        # determines how many 16‑byte paragraphs of prefix exist.  we choose
        # one paragraph (16 bytes) and put our 4‑byte payload at the end of it.
        prefix = b"\x00" * 12 + bytes(payload)  # total length 16
        fake_data = b"\x00" * (num_paras * 16 - len(header)) + prefix + bytes(exehdr) + stub
        # fix up num_blocks/num_last to describe fake_data length
        data_len = len(fake_data)
        num_blocks = (data_len + 511) // 512
        num_last = data_len % 512
        header = struct.pack(
            "<2sHHHHHHHHHHHHH",
            b"MZ", num_last, num_blocks, 0, num_paras, 0, 0, 0, 0, 0, 18, 1, 28, 0
        )
        fake_raw = header + fake_data
        new_raw = self.extract_mod.maybe_unpack_exe_bytes(fake_raw)
        # after unpacking the data portion should be sixteen 'A's (one
        # paragraph)
        exe_file = self.extract_mod.read_exe_file(new_raw)
        self.assertEqual(exe_file.data, b"A" * 16)

if __name__ == "__main__":
    unittest.main()
