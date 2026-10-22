"""Bounded downloads, executable reads, and whole-import bitmap budgets."""
import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import io
import os
from pathlib import Path
import struct
import sys
import tempfile
import threading
import time
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
import zlib
import urllib.request

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import decompile
import disc
import mcc_content
import prepare_mcc_gameplay as preparation
from test_mcc_gameplay_preparation import cache_bytes


class Response(io.BytesIO):
    def __init__(self, data, length=None):
        super().__init__(data)
        self.headers = {} if length is None else {"Content-Length": str(length)}


class DownloadTests(unittest.TestCase):
    def transfer(self, root, response, digest=None, limit=8):
        opener = Mock()
        opener.open.return_value = response
        with patch.object(decompile.urllib.request, "build_opener", return_value=opener):
            decompile.download_pinned("https://example.invalid/pinned", root / "package",
                digest or hashlib.sha256(b"accepted").hexdigest(), limit)
        opener.open.assert_called_once_with("https://example.invalid/pinned", timeout=decompile.DOWNLOAD_TIMEOUT)

    def test_exact_pinned_content_publishes_only_after_verification(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.transfer(root, Response(b"accepted", 8))
            self.assertEqual((root / "package").read_bytes(), b"accepted")
            self.assertEqual([p.name for p in root.iterdir()], ["package"])

    def test_declared_streamed_chunked_and_truncated_excess_leave_no_partial(self):
        for data, length in ((b"accepted!", None), (b"accepted!", 9), (b"short", 8)):
            with self.subTest(length=length), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                with self.assertRaises(ValueError):
                    self.transfer(root, Response(data, length))
                self.assertEqual(list(root.iterdir()), [])

    def test_invalid_digest_and_stall_preserve_existing_destination(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "package").write_bytes(b"previous")
            for stall in (False, True):
                response = Response(b"incorrect")
                if stall:
                    response.read1 = Mock(side_effect=TimeoutError("socket stalled"))
                with self.assertRaises((ValueError, TimeoutError)):
                    self.transfer(root, response)
                self.assertEqual((root / "package").read_bytes(), b"previous")
                self.assertEqual(len(list(root.iterdir())), 1)

    def test_wall_deadline_stops_slow_trickle(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with patch.object(decompile.time, "monotonic", side_effect=[0, 0, 901]):
                response = Response(b"accepted")
                with self.assertRaises(TimeoutError):
                    self.transfer(root, response)
            self.assertEqual(list(root.iterdir()), [])

    def test_https_redirect_policy_rejects_downgrade(self):
        with self.assertRaisesRegex(ValueError, "HTTPS"):
            decompile.HttpsRedirects().redirect_request(None, None, 302, "", {}, "http://example.invalid")

    def test_real_http_body_stall_hits_socket_timeout_and_cleans_partial(self):
        class Stalled(BaseHTTPRequestHandler):
            def do_GET(self):
                self.send_response(200)
                self.send_header("Content-Length", "8")
                self.end_headers()
                time.sleep(.2)

            def log_message(self, *args):
                pass

        with HTTPServer(("127.0.0.1", 0), Stalled) as server, tempfile.TemporaryDirectory() as temp:
            server.timeout = .5
            thread = threading.Thread(target=server.handle_request)
            thread.start()
            local = f"http://127.0.0.1:{server.server_port}"
            local_opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
            opener = Mock()
            opener.open.side_effect = lambda url, timeout: local_opener.open(local, timeout=timeout)
            try:
                with patch.object(decompile.urllib.request, "build_opener", return_value=opener), \
                     patch.object(decompile, "DOWNLOAD_TIMEOUT", .05):
                    with self.assertRaises(TimeoutError):
                        decompile.download_pinned("https://example.invalid/pinned", Path(temp) / "package", "0" * 64, 8)
                self.assertEqual(list(Path(temp).iterdir()), [])
            finally:
                thread.join(timeout=2)
                self.assertFalse(thread.is_alive())


class XbeTests(unittest.TestCase):
    def test_sparse_oversize_is_rejected_before_reading(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "sparse.xbe"
            with path.open("wb") as out:
                out.write(b"XBEH")
                out.truncate(disc.MAX_XBE_BYTES + 1)
            with patch.object(disc.os, "fdopen", wraps=disc.os.fdopen) as opened:
                with self.assertRaisesRegex(disc.FormatError, "128 MiB"):
                    disc.inspect_xbe(path)
                self.assertTrue(opened.called)

    @unittest.skipUnless(hasattr(os, "mkfifo"), "POSIX pipes")
    def test_fifo_is_rejected_without_waiting_for_a_writer(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "pipe.xbe"
            os.mkfifo(path)
            with self.assertRaisesRegex(disc.FormatError, "regular file"):
                disc.inspect_xbe(path)

    def test_valid_header_sections_and_hashes_are_unchanged(self):
        data = bytearray(1024)
        data[:4] = b"XBEH"
        for at, value in ((0x104, 0x10000), (0x10C, len(data)), (0x118, 0x10200),
                          (0x11C, 1), (0x120, 0x10180), (0x128, 0x11000 ^ 0xA8FC57AB),
                          (0x164, 0x10000)):
            struct.pack_into("<I", data, at, value)
        struct.pack_into("<6I", data, 0x180, 4, 0x11000, 16, 0x300, 16, 0x101C0)
        data[0x1C0:0x1C6] = b".text\0"
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "valid.xbe"
            path.write_bytes(data)
            result = disc.inspect_xbe(path)
        self.assertEqual(result["sha256"], hashlib.sha256(data).hexdigest())
        self.assertEqual(result["sections"][0]["sha256"], hashlib.sha256(bytes(16)).hexdigest())


class BitmapBudgetTests(unittest.TestCase):
    def maps(self, root, pointers=(8, 8)):
        paths = []
        for name, pointer in zip(("mainmenu", "shared"), pointers):
            data = cache_bytes()
            struct.pack_into("<I", data, 2384 + 56, pointer)
            struct.pack_into("<I", data, 2384 + 80, 16)
            path = root / (name + ".map")
            path.write_bytes(data)
            paths.append(path)
        textures = io.BytesIO(b"padding!" + struct.pack("<ii", 1, -8) + b"pixels01"
                             + b"padding!" + struct.pack("<ii", 1, -8) + b"pixels02")
        return paths, textures

    def test_duplicate_references_are_charged_once_across_maps(self):
        with tempfile.TemporaryDirectory() as temp:
            paths, textures = self.maps(Path(temp), (8, 0x80000008))
            with patch.object(preparation, "MAX_BITMAP_BYTES", 8):
                plan, total = preparation.plan_bitmaps(paths, textures, lambda _: None)
        self.assertEqual((len(plan), total), (1, 8))

    def test_aggregate_valid_individuals_exceed_bytes_or_count(self):
        for limits in ((12, 10), (100, 1)):
            with self.subTest(limits=limits), tempfile.TemporaryDirectory() as temp:
                paths, textures = self.maps(Path(temp), (8, 32))
                with patch.object(preparation, "MAX_BITMAP_BYTES", limits[0]), \
                     patch.object(preparation, "MAX_BITMAP_PARTITIONS", limits[1]):
                    with self.assertRaises(preparation.FormatError):
                        preparation.plan_bitmaps(paths, textures, lambda _: None)
                self.assertFalse((Path(temp) / "bitmap-resources").exists())

    def test_raw_and_compressed_segments_obey_remaining_aggregate_budget(self):
        for compressed in (False, True):
            pixels = b"original pixels" * 100
            block = zlib.compress(pixels) if compressed else pixels
            encoded = struct.pack("<ii", 1, len(block) if compressed else -len(block)) + block
            with self.subTest(compressed=compressed), self.assertRaises(mcc_content.FormatError):
                mcc_content.texture_resource(io.BytesIO(encoded), 0, len(encoded), decoded_limit=len(pixels) - 1)

    def test_actual_total_is_reserved_before_any_bitmap_write(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            paths, textures = self.maps(root)
            (root / "textures.dat").write_bytes(textures.getvalue())
            output = root / "output"
            output.mkdir()
            with patch.object(preparation, "plan_catalog"), \
                 patch.object(preparation, "apply_catalog_plan", return_value={"changed_store_blocks": 0}), \
                 patch.object(preparation.shutil, "disk_usage", return_value=SimpleNamespace(free=7)), \
                 patch.object(preparation, "BITMAP_DISK_RESERVE", 0):
                with self.assertRaisesRegex(preparation.FormatError, "aggregate decoded"):
                    preparation.prepare_maps(root, output, lambda _: None)
            self.assertEqual(list(output.iterdir()), [])

    def test_measured_plan_drives_the_real_decode_write_and_report(self):
        # Catalog/gameplay conversion is independent of this pixel-copy path.
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            _, textures = self.maps(root)
            (root / "textures.dat").write_bytes(textures.getvalue())
            output = root / "output"
            output.mkdir()
            with patch.object(preparation, "plan_catalog"), \
                 patch.object(preparation, "apply_catalog_plan", return_value={"changed_store_blocks": 0, "stores": []}), \
                 patch.object(preparation, "plan_instant_charge"), \
                 patch.object(preparation, "apply_instant_charge_plan", return_value={"changed_store_blocks": 0}), \
                 patch.object(mcc_content.MccCache, "tag", return_value=({"size": 18}, 0)):
                summary = preparation.prepare_maps(root, output, lambda _: None)
            self.assertEqual((summary["bitmap_partitions"], summary["decoded_bitmap_bytes"]), (1, 8))
            self.assertEqual([p.read_bytes() for p in (output / "bitmap-resources").iterdir()], [b"pixels01"])
