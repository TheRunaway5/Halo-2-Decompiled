"""Synthetic metadata/snapshot fixtures; no map decoding or runtime acceptance."""
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mcc_native_map_owners as owners
from audit_rendering import FormatError


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode()


class Fixture:
    def __init__(self):
        self.provenance = dict(schema=owners.PROFILE, source_files={}, native_shader_library=dict(maps={}),
                               runtime_active=False, full_tu15_parity=False)
        self.conversion = dict(profile=owners.PROFILE, maps=[])
        self.verified, self.snapshots, self.banks = {}, {}, {}
        self.catalog = dict(schema=owners.CATALOG_SCHEMA, maps=[])
        self.authority = dict(producer="fixture-stock-producer", profile=owners.PROFILE, receipt_sha256="f" * 64)

    def add_map(self, name, data=None, role="multiplayer"):
        data = data if data is not None else name.encode()
        owner = "halo2/" + name
        identity = dict(bytes=len(data), sha256=sha(data))
        self.provenance["source_files"][name] = identity
        self.verified[owner] = identity.copy()
        self.snapshots[owner] = data
        if Path(name).parent.as_posix() == "h2_maps_win64_dx11":
            self.conversion["maps"].append(dict(map=Path(name).name, source_sha256=sha(data),
                                                 output_sha256=sha(data), role=role))
        return owner

    def add_bank(self, namespace, directory=None, *, marker="fixture", stock=True):
        directory = directory or "native-shaders/" + namespace
        # This source hash is original Xbox provenance, deliberately unrelated
        # to every fixture's copied MCC input identity.
        raw = encoded(dict(schema=owners.LIBRARY_SCHEMA, map=namespace, source_sha256="a" * 64,
                           source_xbe_sha256="b" * 64, fixture_owner=marker))
        bank = owners.bank_identity(directory, raw)
        self.banks[directory] = bank
        if stock:
            self.provenance["native_shader_library"]["maps"][namespace] = bank["bank_library_sha256"]
            self.catalog["maps"].append(dict(map=namespace, complete=True))
        return bank

    def build(self, **kwargs):
        return owners.build_index(self.provenance, self.conversion, self.verified, self.catalog, self.banks,
                                  compatibility_authority=self.authority, **kwargs)

    def bind(self, index, owner, bank, *, role="scenario", shared="halo2/Mods/Shared Assets"):
        row = next(value for value in index["maps"] if value["map"] == owner)
        receipt = dict(map=owner, input_bytes=row["input_bytes"], input_sha256=row["input_sha256"],
                       source_sha256=row["source_sha256"], cache_role=role, shared_directory=shared,
                       native_mode="owned-bank" if bank else "none", bank_owner_map=owner if bank else "",
                       producer="fixture-custom-author", profile="fixture-custom-owner-v1",
                       **{key: bank[key] if bank else "" for key in owners.BANK_FIELDS})
        return owners.add_owned_map_binding(index, owner, self.verified[owner], cache_role=role,
                                           shared_directory=shared, owned_bank=bank, producer_receipt=encoded(receipt))


class NativeMapOwnerTests(unittest.TestCase):
    def basic(self):
        fixture = Fixture()
        fixture.add_map("h2_maps_win64_dx11/lockout.map")
        fixture.add_bank("lockout")
        return fixture

    def test_three_same_leaf_owners_bind_distinct_custom_banks(self):
        f = self.basic()
        a = f.add_map("Mods/A/Maps/lockout.map", b"custom A")
        b = f.add_map("Mods/B/Maps/lockout.map", b"custom B")
        ba = f.add_bank("lockout", "native-shaders/maps/halo2/Mods/A/Maps/lockout", marker="A", stock=False)
        bb = f.add_bank("lockout", "native-shaders/maps/halo2/Mods/B/Maps/lockout", marker="B", stock=False)
        base = f.build()
        self.assertEqual(owners.resolve_binding(base, a, b"custom A", f.banks)["native_mode"], "none")
        self.assertEqual(owners.resolve_binding(base, b, b"custom B", f.banks)["native_mode"], "none")
        one = f.bind(base, a, ba)
        self.assertEqual(owners.resolve_binding(one, b, b"custom B", f.banks)["native_mode"], "none")
        both = f.bind(one, b, bb)
        results = [owners.resolve_binding(both, owner, f.snapshots[owner], f.banks)
                   for owner in ("halo2/h2_maps_win64_dx11/lockout.map", a, b)]
        self.assertEqual(len({row["bank_directory"] for row in results}), 3)
        self.assertEqual(len({row["bank_library_sha256"] for row in results}), 3)
        self.assertEqual({row["bank_namespace"] for row in results}, {"lockout"})
        self.assertEqual(results[1]["bank_owner_map"], a)
        self.assertEqual(base, f.build())  # Functional add never mutates source metadata.

    def test_case_spaces_and_loose_tag_data_paths_preserve_exact_spelling(self):
        f = self.basic()
        upper = f.add_map("Mods/My Pack/Maps/Lockout.map")
        lower = f.add_map("Mods/My Pack/Maps/lockout.map")
        f.provenance["source_files"]["Mods/My Pack/tags/Example.tag"] = dict(bytes=3, sha256=sha(b"tag"))
        f.provenance["source_files"]["Mods/My Pack/data/Example.bin"] = dict(bytes=4, sha256=sha(b"data"))
        before = copy.deepcopy(f.provenance)
        index = f.build()
        self.assertIsNotNone(owners.resolve_binding(index, upper, f.snapshots[upper], f.banks))
        self.assertIsNotNone(owners.resolve_binding(index, lower, f.snapshots[lower], f.banks))
        self.assertIsNone(owners.resolve_binding(index, "halo2/mods/My Pack/Maps/Lockout.map", b"", f.banks))
        self.assertEqual(owners.decode_index(owners.encode_index(index)), index)
        self.assertEqual(f.provenance, before)

    def test_path_alias_traversal_absolute_and_duplicate_json_keys_rejected(self):
        f, bad = self.basic(), ("halo2//x.map", "halo2/./x.map", "halo2/../x.map", "/halo2/x.map",
                               "C:/halo2/x.map", "halo2\\x.map", "halo2/x\0.map")
        index = f.build()
        for name in bad:
            with self.subTest(name=name), self.assertRaises(FormatError):
                owners.resolve_binding(index, name, b"", f.banks)
        with self.assertRaises(FormatError):
            owners.decode_index(b'{"schema":"first","schema":"second"}')
        repeated = copy.deepcopy(index)
        repeated["maps"].append(copy.deepcopy(repeated["maps"][0]))
        with self.assertRaisesRegex(FormatError, "duplicate exact map"):
            owners.encode_index(repeated)

    def test_ordered_conversion_output_and_graphics_chain_uses_final_identity(self):
        f = self.basic()
        owner = "halo2/h2_maps_win64_dx11/lockout.map"
        source = f.verified[owner]["sha256"]
        converted, output, graphics = b"converted output", b"second output", b"final graphics encoded map"
        f.conversion["maps"][0]["output_sha256"] = sha(converted)
        f.provenance["output_changes"] = [dict(path=owner, donor_sha256=sha(converted), converted_sha256=sha(output))]
        f.provenance["graphics_corrections"] = dict(changes=[dict(path=owner, donor_sha256=sha(output), converted_sha256=sha(graphics))])
        f.verified[owner] = dict(bytes=len(graphics), sha256=sha(graphics))
        index = f.build()
        row = owners.resolve_binding(index, owner, graphics, f.banks)
        self.assertEqual(row["source_sha256"], source)
        self.assertEqual(row["input_sha256"], sha(graphics))
        self.assertEqual(row["input_bytes"], len(graphics))
        with self.assertRaisesRegex(FormatError, "admitted encoded map differs"):
            owners.resolve_binding(index, owner, converted, f.banks)
        f.provenance["graphics_corrections"]["changes"][0]["donor_sha256"] = source
        with self.assertRaisesRegex(FormatError, "donor chain"):
            f.build()

    def test_stale_source_final_output_bank_and_owner_alias_rejected(self):
        f = self.basic()
        original = f.build()
        f.conversion["maps"][0]["source_sha256"] = "0" * 64
        with self.assertRaises(FormatError):
            f.build()
        f.conversion["maps"][0]["source_sha256"] = f.conversion["maps"][0]["output_sha256"]
        f.verified["halo2/h2_maps_win64_dx11/lockout.map"]["sha256"] = "1" * 64
        with self.assertRaisesRegex(FormatError, "wrong final encoded"):
            f.build()
        bad = copy.deepcopy(original)
        bad["maps"][0]["bank_owner_map"] = "halo2/Mods/A/lockout.map"
        with self.assertRaisesRegex(FormatError, "exact map"):
            owners.resolve_binding(bad, bad["maps"][0]["map"], b"", f.banks)
        changed = dict(f.banks)
        changed["native-shaders/lockout"] = owners.bank_identity("native-shaders/lockout", encoded(
            dict(schema=owners.LIBRARY_SCHEMA, map="lockout", fixture_owner="changed")))
        with self.assertRaisesRegex(FormatError, "bank identity differs"):
            owners.resolve_binding(original, original["maps"][0]["map"], f.snapshots[original["maps"][0]["map"]], changed)
        stale = copy.deepcopy(original)
        stale["input_authority"]["conversion_sha256"] = "2" * 64
        with self.assertRaisesRegex(FormatError, "current exact"):
            owners.validate_index(stale, original)

    def test_custom_receipt_cannot_admit_other_map_or_stock_bank(self):
        f = self.basic()
        a = f.add_map("Mods/A/lockout.map", b"same")
        b = f.add_map("Mods/B/lockout.map", b"same")
        bank = f.add_bank("custom", "native-shaders/maps/A", stock=False)
        base = f.build()
        one = f.bind(base, a, bank)
        with self.assertRaisesRegex(FormatError, "aliases distinct"):
            f.bind(one, b, bank)
        with self.assertRaisesRegex(FormatError, "retained stock"):
            f.bind(base, a, f.banks["native-shaders/lockout"])
        with self.assertRaisesRegex(FormatError, "receipt lost"):
            owners.add_owned_map_binding(base, a, f.verified[a], cache_role="scenario", shared_directory="halo2/Mods/Shared",
                owned_bank=bank, producer_receipt=encoded(dict(producer="author", profile="v1", map=b)))

    def test_resource_role_comes_from_source_bound_row_or_explicit_custom_receipt(self):
        f = self.basic()
        stock = f.add_map("h2_maps_win64_dx11/shared.map", role="resources")
        scenario = f.add_map("Mods/Scenario/shared.map")
        resource = f.add_map("Mods/Resources/single_player_shared.map")
        menu = f.add_map("Mods/Scenario/mainmenu.map")
        index = f.build()
        rows = {row["map"]: row for row in index["maps"]}
        self.assertEqual(rows[stock]["cache_role"], "resources")
        self.assertEqual({rows[name]["cache_role"] for name in (scenario, resource, menu)}, {"scenario"})
        explicit = f.bind(index, resource, None, role="resources", shared="halo2/Mods/Resources")
        self.assertEqual(owners.resolve_binding(explicit, resource, f.snapshots[resource], f.banks)["cache_role"], "resources")

    def test_existing_style_43_stock_rows_keep_39_banks_two_hud_and_two_resources(self):
        f = Fixture()
        stock_names = ("00a_introduction 01a_tutorial 01b_spacestation 03a_oldmombasa 03b_newmombasa "
            "04a_gasgiant 04b_floodlab 05a_deltaapproach 05b_deltatowers 06a_sentinelwalls 06b_floodzone "
            "07a_highcharity 07b_forerunnership 08a_deltacliffs 08b_deltacontrol ascension backwash beavercreek "
            "burial_mounds coagulation colossus containment cyclotron deltatap derelict dune elongation foundation "
            "gemini headlong highplains lockout mainmenu midship triplicate turf warlock waterworks zanzibar").split()
        for name in stock_names:
            f.add_map("h2_maps_win64_dx11/" + name + ".map")
            f.add_bank(name)
        for name in ("shared", "single_player_shared"):
            f.add_map("h2_maps_win64_dx11/" + name + ".map", role="resources")
        hud = []
        for name in ("needle", "street_sweeper"):
            owner = f.add_map("h2_maps_win64_dx11/" + name + ".map")
            hud.append(dict(map=name, **f.verified[owner]))
        cairo = f.banks["native-shaders/01b_spacestation"]
        policy = encoded(dict(schema=owners.HUD_SCHEMA, source_map="01b_spacestation",
                              source_canonical_sha256=cairo["canonical_sha256"], maps=hud))
        before = copy.deepcopy(f.provenance)
        index = f.build(hud_bootstrap=policy)
        modes = [row["native_mode"] for row in index["maps"]]
        self.assertEqual((len(modes), modes.count("owned-bank"), modes.count("authenticated-hud-bootstrap"), modes.count("none")), (43, 39, 2, 2))
        self.assertEqual(f.provenance, before)
        self.assertNotIn("owner_bindings", f.provenance["native_shader_library"])
        for row in index["maps"]:
            if row["native_mode"] == "authenticated-hud-bootstrap":
                self.assertEqual(row["bank_directory"], "")
                self.assertEqual(owners.resolve_binding(index, row["map"], f.snapshots[row["map"]], f.banks)["native_mode"], "authenticated-hud-bootstrap")

    def test_common_cairo_dependency_does_not_require_cairo_gameplay_owner(self):
        f = self.basic()
        cairo = f.add_bank("01b_spacestation")
        dependency = dict(kind="authenticated-common-templates", **{key: cairo[key] for key in owners.BANK_FIELDS},
                          canonical_sha256=cairo["canonical_sha256"], authority_sha256="c" * 64)
        index = f.build(dependencies=[dependency])
        self.assertEqual(len(index["maps"]), 1)
        self.assertIsNone(owners.resolve_binding(index, "halo2/h2_maps_win64_dx11/01b_spacestation.map", b"", f.banks))
        self.assertEqual(index["dependencies"], [dependency])
        bad = copy.deepcopy(index)
        bad["dependencies"][0]["canonical_sha256"] = "d" * 64
        with self.assertRaisesRegex(FormatError, "common dependency identity"):
            owners.resolve_binding(bad, index["maps"][0]["map"], f.snapshots[index["maps"][0]["map"]], f.banks)

    def test_deterministic_projection_and_missing_final_extent_authority(self):
        f = self.basic()
        a = f.build()
        f.provenance["source_files"] = dict(reversed(list(f.provenance["source_files"].items())))
        b = f.build()
        self.assertEqual(owners.encode_index(a), owners.encode_index(b))
        f.verified.clear()
        with self.assertRaisesRegex(FormatError, "final verified map owner set"):
            f.build()


def cross_language_fixture(consumer):
    """Run the actual Python producer against the standalone C++ consumer.

    This is an explicit fixture mode, never part of ordinary unittest import.
    Its expected index hash is sealed by the synthetic producer before passing
    independent snapshot/negative cases to C++; it is not consumer self-trust.
    """
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    if not scratch.is_absolute() or not scratch.is_dir():
        raise RuntimeError("Run through tools/run_test.py with owned on-disk scratch")
    with tempfile.TemporaryDirectory(prefix="map-owner-producer-", dir=scratch) as temporary:
        directory = Path(temporary)
        os.environ["XDG_CACHE_HOME"] = str(directory / "xdg-cache")
        (directory / "xdg-cache").mkdir()
        cache = directory / "cache"
        cache.mkdir()
        f = Fixture()
        stock = f.add_map("h2_maps_win64_dx11/lockout.map", b"original copied MCC lockout")
        shared = f.add_map("h2_maps_win64_dx11/shared.map", b"source-bound stock resource", role="resources")
        a = f.add_map("Mods/Pack \u03a9/A/Maps/lockout.map", b"original custom A")
        b = f.add_map("Mods/Pack \u03a9/B/Maps/lockout.map", b"original custom B")
        scenario = f.add_map("Mods/Pack \u03a9/Scenario/shared.map", b"scenario with resource-looking leaf")
        resources = f.add_map("Mods/Pack \u03a9/Resources/custom.map", b"explicit custom resources")
        realistic = dict(note="cr\u00e8me \u03a9 \u65e5\u672c", datum=3820831920, address=4294967295,
                         raw_u32=[0, 2147483648, 4068692854, 4294967295])
        f.add_bank("lockout", marker=realistic | dict(owner="stock"))
        ba = f.add_bank("lockout", "native-shaders/maps/halo2/Mods/Pack \u03a9/A/Maps/lockout",
                        marker=realistic | dict(owner="custom A"), stock=False)
        bb = f.add_bank("lockout", "native-shaders/maps/halo2/Mods/Pack \u03a9/B/Maps/lockout",
                        marker=realistic | dict(owner="custom B"), stock=False)
        cairo = f.add_bank("01b_spacestation", marker=realistic | dict(owner="dependency, no Cairo map"))
        # Conversion output, ordinary postconversion output and final graphics
        # output have different contents/sizes. No source size is reused.
        conversion_output = b"decoded-converted encoded lockout"
        after_output = b"postconversion output with another encoded extent"
        final = b"final graphics encoded lockout; deliberately longer than every source fixture"
        next(row for row in f.conversion["maps"] if row["map"] == "lockout.map")["output_sha256"] = sha(conversion_output)
        f.provenance["output_changes"] = [dict(path=stock, donor_sha256=sha(conversion_output), converted_sha256=sha(after_output))]
        f.provenance["graphics_corrections"] = dict(changes=[dict(path=stock, donor_sha256=sha(after_output), converted_sha256=sha(final))])
        f.verified[stock], f.snapshots[stock] = dict(bytes=len(final), sha256=sha(final)), final
        custom_final = b"final encoded A from its explicit custom producer, larger than its source"
        f.provenance["output_changes"].append(dict(path=a, donor_sha256=f.verified[a]["sha256"], converted_sha256=sha(custom_final)))
        f.verified[a], f.snapshots[a] = dict(bytes=len(custom_final), sha256=sha(custom_final)), custom_final
        hud_maps = []
        for name in ("needle", "street_sweeper"):
            owner = f.add_map("h2_maps_win64_dx11/" + name + ".map", ("exact synthetic HUD " + name).encode())
            hud_maps.append(dict(map=name, **f.verified[owner]))
        hud = encoded(dict(schema=owners.HUD_SCHEMA, source_map="01b_spacestation",
                           source_canonical_sha256=cairo["canonical_sha256"], maps=hud_maps))
        dependency = dict(kind="authenticated-common-templates", **{key: cairo[key] for key in owners.BANK_FIELDS},
                          canonical_sha256=cairo["canonical_sha256"], authority_sha256="c" * 64)
        index = f.build(hud_bootstrap=hud, dependencies=[dependency])
        index = f.bind(index, a, ba, shared="halo2/Mods/Pack \u03a9/Shared Assets")
        index = f.bind(index, b, bb, shared="halo2/Mods/Pack \u03a9/Shared Assets")
        index = f.bind(index, resources, None, role="resources", shared="halo2/Mods/Pack \u03a9/Resources")
        raw_index = owners.encode_index(index)
        owners.validate_index(owners.decode_index(raw_index), index)
        # Test producer's own stale-donor/final-output rejection before export.
        bad = copy.deepcopy(f.provenance)
        bad["graphics_corrections"]["changes"][0]["donor_sha256"] = "0" * 64
        try:
            owners.derive_final_inputs(bad, f.conversion, f.verified)
        except FormatError:
            stale_donor_rejected = True
        else:
            raise AssertionError("Actual producer admitted a stale graphics donor")
        bad_verified = copy.deepcopy(f.verified)
        bad_verified[stock]["sha256"] = sha(conversion_output)
        try:
            owners.derive_final_inputs(f.provenance, f.conversion, bad_verified)
        except FormatError:
            wrong_final_rejected = True
        else:
            raise AssertionError("Actual producer admitted an obsolete final output")
        # Recreate exact metadata bytes used by bank_identity, including JSON
        # escaped Unicode whose canonical digest is UTF-8 rather than ASCII.
        library_metadata = {}
        for bank, marker in ((f.banks["native-shaders/lockout"], realistic | dict(owner="stock")),
                             (ba, realistic | dict(owner="custom A")), (bb, realistic | dict(owner="custom B")),
                             (cairo, realistic | dict(owner="dependency, no Cairo map"))):
            raw = encoded(dict(schema=owners.LIBRARY_SCHEMA, map=bank["bank_namespace"], source_sha256="a" * 64,
                               source_xbe_sha256="b" * 64, fixture_owner=marker))
            if owners.bank_identity(bank["bank_directory"], raw) != bank:
                raise AssertionError("Fixture changed actual producer bank bytes")
            relative = bank["bank_directory"] + "/library.json"
            library_metadata[relative] = raw
        for owner, snapshot in f.snapshots.items():
            path = cache / owner
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(snapshot)
        for relative, data in library_metadata.items():
            path = cache / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        for row in index["maps"]:
            (cache / row["shared_directory"]).mkdir(parents=True, exist_ok=True)
        cases = []
        for row in index["maps"]:
            expected = owners.resolve_binding(index, row["map"], f.snapshots[row["map"]], f.banks)
            cases.append(dict(name="positive:" + row["map"], map=row["map"], shared_directory=row["shared_directory"],
                              snapshot_hex=f.snapshots[row["map"]].hex(), expected=expected))
        unknown = "halo2/Mods/Unbound/Maps/lockout.map"
        (cache / unknown).parent.mkdir(parents=True)
        (cache / unknown).write_bytes(b"unbound leaf")
        cases.append(dict(name="unbound_same_leaf", map=unknown, shared_directory="halo2/h2_maps_win64_dx11",
                          snapshot_hex=b"unbound leaf".hex(), expected=None))
        cases.extend([
            dict(name="stale_admitted_output", map=stock, shared_directory="halo2/h2_maps_win64_dx11",
                 snapshot_hex=conversion_output.hex(), expect_error=True),
            dict(name="changed_bank_snapshot", map=a, shared_directory="halo2/Mods/Pack \u03a9/Shared Assets",
                 snapshot_hex=f.snapshots[a].hex(), metadata_override=dict(path=ba["bank_directory"] + "/library.json",
                    bytes_hex=b"changed bank metadata".hex()), expect_error=True),
            dict(name="traversing_map_path", map=a, path_override="halo2/Mods/../Mods/Pack \u03a9/A/Maps/lockout.map",
                 shared_directory="halo2/Mods/Pack \u03a9/Shared Assets", snapshot_hex=f.snapshots[a].hex(), expect_error=True),
            dict(name="wrong_independent_index_fingerprint", map=stock, shared_directory="halo2/h2_maps_win64_dx11",
                 snapshot_hex=final.hex(), index_sha_override="0" * 64, expect_error=True)])
        envelope = dict(schema="H2_MCC_NATIVE_MAP_OWNER_CROSS_LANGUAGE_FIXTURE_V1", cache_root=str(cache),
                        encoded_index=raw_index.decode("utf-8"), expected_index_sha256=sha(raw_index),
                        metadata=[dict(path=name, bytes_hex=data.hex()) for name, data in sorted(library_metadata.items())],
                        cases=cases, producer_controls=dict(stale_graphics_donor_rejected=stale_donor_rejected,
                            wrong_final_output_rejected=wrong_final_rejected),
                        expected_distinct_bank_owners=[stock, a, b],
                        replacement_after_snapshot=dict(map=a, library=ba["bank_directory"] + "/library.json"))
        fixture_path = directory / "producer-fixture.json"
        fixture_data = encoded(envelope)
        if len(fixture_data) > 1024 * 1024:
            raise AssertionError("Cross-language fixture exceeds finite one-MiB bound")
        fixture_path.write_bytes(fixture_data)
        result = subprocess.run([str(Path(consumer).resolve(strict=True)), "--producer-fixture", str(fixture_path)],
                                check=False, timeout=30)
        if result.returncode:
            raise RuntimeError("Actual C++ producer fixture consumer failed: " + str(result.returncode))
        print("Actual Python producer to C++ resolver compatibility passed; no product/cache adoption")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--cpp-consumer":
        cross_language_fixture(sys.argv[2])
    else:
        unittest.main()
