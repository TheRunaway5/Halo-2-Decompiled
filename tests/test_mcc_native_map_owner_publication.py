"""Synthetic private-cache publication/rollback; no installed asset inputs."""
import copy
import errno
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mcc_native_map_owner_publication as publication
import mcc_native_map_owners as owners
from audit_rendering import FormatError
from test_mcc_native_map_owners import Fixture, encoded, sha
from mcc_native_scalar_fixture import shipped_native_scalar_tables


def snapshot(directory):
    return {path.relative_to(directory).as_posix(): path.read_bytes()
            for path in directory.rglob("*") if path.is_file() and not path.is_symlink()}


class NativeMapOwnerPublicationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tables = shipped_native_scalar_tables()

    def setUp(self):
        scratch = os.environ.get("HALO2_TEST_SCRATCH")
        self.assertTrue(scratch and Path(scratch).is_dir(), "Use the owned run_test lifecycle")
        temporary = tempfile.TemporaryDirectory(prefix="owner-publication-", dir=scratch)
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.cache, self.policies = self.root / "cache", self.root / "policies"
        self.cache.mkdir(); self.policies.mkdir()
        self.fixture = Fixture()
        f = self.fixture
        self.stock = f.add_map("h2_maps_win64_dx11/lockout.map", b"original source lockout")
        f.add_map("h2_maps_win64_dx11/shared.map", b"stock resource", role="resources")
        self.custom_a = f.add_map("Mods/Pack \u03a9/A/Maps/lockout.map", b"custom A")
        self.custom_b = f.add_map("Mods/Pack \u03a9/B/Maps/lockout.map", b"custom B")
        self.stock_bank = f.add_bank("lockout")
        self.cairo = f.add_bank("01b_spacestation")
        self.a_bank = f.add_bank("lockout", "native-shaders/maps/halo2/Mods/Pack \u03a9/A/lockout", marker="custom A", stock=False)
        self.b_bank = f.add_bank("lockout", "native-shaders/maps/halo2/Mods/Pack \u03a9/B/lockout", marker="custom B", stock=False)
        converted, ordinary = b"converted stock", b"post-conversion stock output"
        self.final = b"final stock output after all graphics corrections with a different extent"
        f.conversion["maps"][0]["output_sha256"] = sha(converted)
        f.provenance["output_changes"] = [dict(path=self.stock, donor_sha256=sha(converted), converted_sha256=sha(ordinary))]
        f.provenance["graphics_corrections"] = dict(changes=[dict(path=self.stock, donor_sha256=sha(ordinary), converted_sha256=sha(self.final))])
        f.snapshots[self.stock], f.verified[self.stock] = self.final, dict(bytes=len(self.final), sha256=sha(self.final))
        hud_rows = []
        for name in ("needle", "street_sweeper"):
            owner = f.add_map("h2_maps_win64_dx11/" + name + ".map", ("synthetic HUD " + name).encode())
            hud_rows.append(dict(map=name, **f.verified[owner]))
        self.hud = dict(schema=owners.HUD_SCHEMA, source_map="01b_spacestation",
                        source_canonical_sha256=self.cairo["canonical_sha256"], maps=hud_rows)
        self.closure = dict(schema="H2_MCC_RETAINED_NATIVE_LIBRARY_CLOSURE_V1", source_map="01b_spacestation",
            source_canonical_sha256=self.cairo["canonical_sha256"], libraries=[
                dict(map="lockout", canonical_sha256=self.stock_bank["canonical_sha256"], missing=[["stem", "fixture-template"]]),
                dict(map="01b_spacestation", canonical_sha256=self.cairo["canonical_sha256"], missing=[])])
        self.write(self.policies / "mcc-native-library-closure.json", encoded(self.closure))
        self.write(self.policies / "mcc-native-hud-bootstrap.json", encoded(self.hud))
        for owner, data in f.snapshots.items():
            self.write(self.cache / owner, data)
        for bank, marker in ((self.stock_bank, "fixture"), (self.cairo, "fixture"),
                             (self.a_bank, "custom A"), (self.b_bank, "custom B")):
            raw = encoded(dict(schema=owners.LIBRARY_SCHEMA, map=bank["bank_namespace"], source_sha256="a" * 64,
                               source_xbe_sha256="b" * 64, fixture_owner=marker))
            self.assertEqual(owners.bank_identity(bank["bank_directory"], raw), bank)
            self.write(self.cache / bank["bank_directory"] / "library.json", raw)
            self.write(self.cache / bank["bank_directory"] / "payloads/custom-program.bin", ("preserve " + marker).encode())
        (self.cache / "halo2/Mods/Shared Assets").mkdir(parents=True)
        self.write(self.cache / "native-shaders/function-tables.bin", self.tables)
        self.write(self.cache / "native-shaders/catalog.json", encoded(f.catalog))
        self.write(self.cache / "tag-conversion.json", encoded(f.conversion))
        self.report = copy.deepcopy(f.provenance)
        self.report["unrelated_presentation"] = dict(family="preserve", receipt="unchanged")
        self.write(self.cache / "provenance.json", encoded(self.report))

    def write(self, path, data):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def receipt(self, owner, bank):
        f = self.fixture
        return encoded(dict(map=owner, input_bytes=f.verified[owner]["bytes"], input_sha256=f.verified[owner]["sha256"],
            source_sha256=f.provenance["source_files"][owner.removeprefix("halo2/")]["sha256"],
            cache_role="scenario", shared_directory="halo2/Mods/Shared Assets", native_mode="owned-bank",
            **{key: bank[key] for key in owners.BANK_FIELDS}, bank_owner_map=owner,
            producer="fixture-custom-author", profile="fixture-custom-owner-v1"))

    def repair(self, report=None, **kwargs):
        return publication.repair_owner_index(self.cache, self.report if report is None else report,
                                             policy_root=self.policies, **kwargs)

    def no_temporary(self):
        self.assertFalse([path for path in self.cache.rglob("*") if path.name.startswith((".map-owner", ".map-owners"))])

    def test_legacy_absence_is_read_only_even_before_native_bank_install(self):
        self.assertIsNone(publication.verify_owner_index(self.cache, self.report, policy_root=self.policies))
        empty = self.root / "old-cache"
        empty.mkdir()
        self.assertIsNone(publication.verify_owner_index(empty, dict(), policy_root=self.policies))
        self.assertEqual(list(empty.iterdir()), [])

    def test_staged_publication_has_exact_pointer_and_final_output_after_graphics(self):
        before = snapshot(self.cache)
        updated = publication.publish_staged_owner_index(self.cache, self.report, policy_root=self.policies)
        self.assertEqual(set(snapshot(self.cache)) - set(before), {publication.INDEX})
        for name, data in before.items():
            self.assertEqual((self.cache / name).read_bytes(), data)
        pointer = updated["native_shader_library"]["owner_bindings"]
        raw = (self.cache / publication.INDEX).read_bytes()
        self.assertEqual(pointer, dict(schema=owners.SCHEMA, path=publication.INDEX, bytes=len(raw), sha256=sha(raw)))
        index = owners.decode_index(raw)
        stock = next(row for row in index["maps"] if row["map"] == self.stock)
        self.assertEqual((stock["input_bytes"], stock["input_sha256"]), (len(self.final), sha(self.final)))
        self.assertNotEqual(stock["source_sha256"], stock["input_sha256"])
        self.assertEqual(sum(row["native_mode"] == "authenticated-hud-bootstrap" for row in index["maps"]), 2)
        for owner in (self.custom_a, self.custom_b):
            self.assertEqual(next(row for row in index["maps"] if row["map"] == owner)["native_mode"], "none")
        self.assertEqual(publication.publish_staged_owner_index(self.cache, updated, policy_root=self.policies), updated)
        self.no_temporary()

    def test_explicit_same_leaf_custom_owners_survive_idempotent_repair(self):
        receipts = [self.receipt(self.custom_a, self.a_bank), self.receipt(self.custom_b, self.b_bank)]
        original = snapshot(self.cache)
        _, updated = self.repair(custom_receipts=receipts)
        after = snapshot(self.cache)
        self.assertEqual(set(after) - set(original), {publication.INDEX})
        for name, data in original.items():
            if name != "provenance.json":
                self.assertEqual(after[name], data)
        index = owners.decode_index(after[publication.INDEX])
        rows = {row["map"]: row for row in index["maps"]}
        self.assertEqual(len({rows[owner]["bank_directory"] for owner in (self.stock, self.custom_a, self.custom_b)}), 3)
        self.assertIsNotNone(publication.verify_owner_index(self.cache, updated, policy_root=self.policies))
        _, again = self.repair(updated)
        self.assertEqual(again, updated)
        self.assertEqual(snapshot(self.cache), after)
        self.assertFalse(updated["runtime_active"])
        self.assertFalse(updated["full_tu15_parity"])
        self.assertEqual(updated["unrelated_presentation"], self.report["unrelated_presentation"])
        self.no_temporary()

    def test_wrong_final_output_stale_donor_and_symlink_reject_without_publication(self):
        before = snapshot(self.cache)
        self.write(self.cache / self.stock, b"obsolete converted output")
        changed = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, "wrong final encoded"):
            self.repair()
        self.assertEqual(snapshot(self.cache), changed)
        self.write(self.cache / self.stock, self.final)
        bad = copy.deepcopy(self.report)
        bad["graphics_corrections"]["changes"][0]["donor_sha256"] = "0" * 64
        with self.assertRaisesRegex(FormatError, "donor chain"):
            publication.derive_owner_index(self.cache, bad, policy_root=self.policies)
        leaf = self.cache / self.custom_a
        leaf.unlink()
        outside = self.root / "outside.map"
        outside.write_bytes(self.fixture.snapshots[self.custom_a])
        leaf.symlink_to(outside)
        with self.assertRaisesRegex(FormatError, "symlink"):
            self.repair()
        self.assertNotIn(publication.INDEX, snapshot(self.cache))
        self.no_temporary()

    def test_corrupt_declared_pointer_or_index_is_never_overwritten(self):
        _, updated = self.repair()
        bad = copy.deepcopy(updated)
        bad["native_shader_library"]["owner_bindings"]["sha256"] = "0" * 64
        self.write(self.cache / "provenance.json", encoded(bad))
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, "pointer fingerprint"):
            self.repair(bad)
        self.assertEqual(snapshot(self.cache), before)
        self.write(self.cache / "provenance.json", encoded(updated))
        self.write(self.cache / publication.INDEX, b"corrupt index")
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, "pointer fingerprint"):
            self.repair(updated)
        self.assertEqual(snapshot(self.cache), before)

    def test_provenance_publish_failure_rolls_back_absent_and_existing_index(self):
        for existing in (False, True):
            with self.subTest(existing=existing):
                if existing:
                    _, self.report = self.repair()
                    receipts = [self.receipt(self.custom_a, self.a_bank)]
                else:
                    receipts = []
                before = snapshot(self.cache)
                actual_replace = publication.os.replace
                failed = False
                def replace(source, target):
                    nonlocal failed
                    if Path(target) == self.cache / "provenance.json" and not failed:
                        failed = True
                        raise OSError("injected provenance publication failure")
                    return actual_replace(source, target)
                with patch.object(publication.os, "replace", side_effect=replace):
                    with self.assertRaisesRegex(OSError, "injected"):
                        self.repair(custom_receipts=receipts)
                self.assertEqual(snapshot(self.cache), before)
                self.no_temporary()

    def test_verified_beforeimage_refreshes_only_legitimate_stock_metadata(self):
        _, updated = self.repair(custom_receipts=[self.receipt(self.custom_a, self.a_bank)])
        token = publication.verify_owner_index(self.cache, updated, policy_root=self.policies)
        # A separately authorized closure repair may change exact stock bank
        # metadata while all maps and custom programs remain unchanged.
        path = self.cache / "native-shaders/lockout/library.json"
        library = publication._json(path.read_bytes())
        library["closure_note"] = "legitimate retained definition metadata refresh"
        self.write(path, encoded(library))
        refreshed = copy.deepcopy(updated)
        refreshed["native_shader_library"]["maps"]["lockout"] = sha(path.read_bytes())
        self.write(self.cache / "provenance.json", encoded(refreshed))
        before = snapshot(self.cache)
        with self.assertRaises(FormatError):
            self.repair(refreshed)
        self.assertEqual(snapshot(self.cache), before)
        _, final = self.repair(refreshed, verified_beforeimage=token)
        self.assertIsNotNone(publication.verify_owner_index(self.cache, final, policy_root=self.policies))
        self.assertEqual((self.cache / self.a_bank["bank_directory"] / "payloads/custom-program.bin").read_bytes(), b"preserve custom A")
        self.no_temporary()

    def test_custom_bank_change_cannot_use_beforeimage_to_erase_its_owner(self):
        _, updated = self.repair(custom_receipts=[self.receipt(self.custom_a, self.a_bank)])
        token = publication.verify_owner_index(self.cache, updated, policy_root=self.policies)
        path = self.cache / self.a_bank["bank_directory"] / "library.json"
        self.write(path, encoded(dict(schema=owners.LIBRARY_SCHEMA, map="lockout", customized=True)))
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, "custom bank changed"):
            self.repair(updated, verified_beforeimage=token)
        self.assertEqual(snapshot(self.cache), before)
        self.no_temporary()

    def test_changed_common_canonical_dependency_is_not_a_whole_bank_fallback(self):
        path = self.cache / "native-shaders/01b_spacestation/library.json"
        library = publication._json(path.read_bytes())
        library["customized"] = True
        self.write(path, encoded(library))
        updated = copy.deepcopy(self.report)
        updated["native_shader_library"]["maps"]["01b_spacestation"] = sha(path.read_bytes())
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, "common dependency"):
            publication.derive_owner_index(self.cache, updated, policy_root=self.policies)
        self.assertEqual(snapshot(self.cache), before)
        self.assertNotIn(publication.INDEX, before)

    def test_beforeimage_cannot_overwrite_a_changed_declared_index(self):
        _, updated = self.repair()
        token = publication.verify_owner_index(self.cache, updated, policy_root=self.policies)
        path = self.cache / publication.INDEX
        value = owners.decode_index(path.read_bytes())
        value["maps"][0]["input_sha256"] = "0" * 64
        changed = owners.encode_index(value)
        self.write(path, changed)
        altered = copy.deepcopy(updated)
        altered["native_shader_library"]["owner_bindings"].update(bytes=len(changed), sha256=sha(changed))
        self.write(self.cache / "provenance.json", encoded(altered))
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, "beforeimage"):
            self.repair(altered, verified_beforeimage=token)
        self.assertEqual(snapshot(self.cache), before)

    def test_stage_publication_works_when_hardlinks_are_unsupported(self):
        with patch.object(publication.os, "link", side_effect=OSError(errno.EOPNOTSUPP, "hardlinks unsupported")) as link:
            updated = publication.publish_staged_owner_index(self.cache, self.report, policy_root=self.policies)
        link.assert_not_called()  # The atomic rename path needs no hard-link capability.
        pointer = updated["native_shader_library"]["owner_bindings"]
        self.assertEqual(sha((self.cache / publication.INDEX).read_bytes()), pointer["sha256"])
        self.assertEqual(sha(self.tables), publication.TABLE_SHA)
        self.no_temporary()

    def test_concurrent_existing_stage_target_is_never_overwritten(self):
        actual = publication._publish_absent
        concurrent = b"concurrent existing owner index retained"
        original_provenance = (self.cache / "provenance.json").read_bytes()
        def publish(source, target):
            target.write_bytes(concurrent)
            return actual(source, target)
        with patch.object(publication, "_publish_absent", side_effect=publish):
            with self.assertRaisesRegex(FormatError, "existing destination retained"):
                publication.publish_staged_owner_index(self.cache, self.report, policy_root=self.policies)
        self.assertEqual((self.cache / publication.INDEX).read_bytes(), concurrent)
        self.assertEqual((self.cache / "provenance.json").read_bytes(), original_provenance)
        self.no_temporary()


if __name__ == "__main__":
    unittest.main()
