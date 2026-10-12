"""Conversion ownership, archive selection and failure/activation boundaries.

The numeric differential uses the real TU1.5 executable in the separate oracle;
these tests protect behavior that cannot be established by its happy path.
"""
from contextlib import ExitStack
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
import prepare_vista_gameplay as preparation
from setup_assets import extract_vista
from vista_tag_conversion import TagGraph, apply_combat


class MemoryCache:
    def __init__(self, name, tags):
        self.path, self.magic = Path(name), 0
        self.data = bytearray(4096)
        self.tags = {t["datum"]: t for t in tags}

    def pointer(self, address, magic, size):
        if address < 0 or address + size > len(self.data):
            raise FormatError("outside fixture")
        return address

    def u32(self, at):
        return struct.unpack_from("<I", self.data, at)[0]

    def u16(self, at):
        return struct.unpack_from("<H", self.data, at)[0]


def tag(datum=0xE1742710, address=1024, size=128, name="globals\\globals", group="matg"):
    return dict(datum=datum, address=address, size=size, name=name, group=group)


class ConversionTests(unittest.TestCase):
    def test_shared_write_preserves_alias_identity_and_local_stub(self):
        local = MemoryCache("lockout.map", [tag(address=0, size=0)])
        shared = MemoryCache("shared.map", [tag()])
        before_local, before_shared = bytes(local.data), bytes(shared.data)
        graph = TagGraph(local, shared)
        address = graph.resolve(0xE1742710, "matg")
        graph.store(address + 4, 0x3F000000, 0x165F81)
        # A second view resolves the same shared allocation, including its write.
        self.assertEqual(TagGraph(local, shared).u32(address + 4), 0x3F000000)
        self.assertEqual(bytes(local.data), before_local)
        self.assertEqual(shared.data[:1028], before_shared[:1028])
        self.assertEqual(shared.data[1032:], before_shared[1032:])
        self.assertEqual(graph.stores[0]["map"], "shared.map")

    def test_exact_shared_reference_and_allocation_bounds(self):
        local = MemoryCache("lockout.map", [tag(address=0, size=0)])
        with self.assertRaises(FormatError):
            TagGraph(local, MemoryCache("shared.map", [tag(name="different")]))
        graph = TagGraph(local, MemoryCache("shared.map", [tag()]))
        # Same low index with another salt cannot resolve to an authored tag.
        with self.assertRaises(FormatError):
            graph.resolve(0xE1752710, "matg")
        with self.assertRaises(FormatError):
            graph.resolve(0xE1742710, "weap")
        with self.assertRaises(FormatError):
            graph.store(1024 + 126, 0, 0)
        self.assertEqual(graph.stores, [])

    def test_campaign_gate_reads_no_multiplayer_references(self):
        cache = MemoryCache("campaign.map", [tag(group="scnr")])
        graph = TagGraph(cache)
        before = bytes(cache.data)
        apply_combat(graph, 1024, 0xDEADBEEF)
        self.assertEqual(bytes(cache.data), before)
        self.assertEqual(graph.stores, [])

    def test_gameplay_extraction_selects_maps_and_fonts_only(self):
        rows = ("maps/mainmenu.map", "maps/shared.map", "maps/lockout.map",
                "maps/single_player_shared.map", "maps/00a_introduction.map", "maps/fonts/conduit-13",
                "h2pc_setup.exe", "shaders/test.spv", "maps/nested/extra.map", "maps/readme.txt")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "installer.zip"
            with zipfile.ZipFile(source, "w") as archive:
                for name in rows:
                    archive.writestr(name, name.encode())
            original = source.read_bytes()
            extract_vista(source, root / "output", gameplay=True)
            selected = {p.relative_to(root / "output").as_posix()
                        for p in (root / "output").rglob("*") if p.is_file()}
            self.assertEqual(selected, set(rows[:6]))
            self.assertEqual(source.read_bytes(), original)

    def test_late_conversion_failure_preserves_active_selection(self):
        with tempfile.TemporaryDirectory() as directory, ExitStack() as stack:
            root = Path(directory)
            (root / "assets").mkdir()
            registry = root / "assets/asset-sources.tsv"
            registry.write_text("working prior selection\n")
            def extraction(source, target, progress, **kwargs):
                self.assertTrue(kwargs["gameplay"])
                (target / "maps/fonts").mkdir(parents=True)
                for name in preparation.XBOX_MAPS:
                    (target / "maps" / (name + ".map")).write_bytes(b"untouched fixture")
                return dict(kind="fixture")
            class FakeCache:
                def __init__(self, path): pass
                def close(self): pass
            for name, value in (("extract_vista", extraction), ("validate_xbox", lambda _: None),
                                ("Cache", FakeCache), ("VistaUiCache", FakeCache),
                                ("export_pack", lambda *args: dict(banks=1)),
                                ("texture_bindings", lambda *args: dict(matched=0, unresolved=0))):
                stack.enter_context(patch.object(preparation, name, value))
            stack.enter_context(patch.object(preparation, "convert_maps", side_effect=FormatError("bad late reference")))
            with self.assertRaisesRegex(FormatError, "bad late reference"):
                preparation.prepare(root, root / "vista", root / "xbox", lambda _: None)
            self.assertEqual(registry.read_text(), "working prior selection\n")
            self.assertEqual(list((root / "assets/imported").iterdir()), [])


if __name__ == "__main__":
    unittest.main()
