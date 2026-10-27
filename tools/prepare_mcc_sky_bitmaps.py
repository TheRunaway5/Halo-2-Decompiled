#!/usr/bin/env python3
"""Create an unselected COW V7 sky candidate from authenticated prepared data.

Run through tools/run_test.py. All staged files belong to HALO2_TEST_SCRATCH;
only a complete, verified candidate moves to the explicitly requested output.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct

from audit_rendering import FormatError, file_sha
from mcc_bitmap_correction_bundle import digest
from mcc_bitmap_correction_bundle_v7 import read_v7, reconstruct_v7, validate_owner_v7, hashes
from mcc_content import MccCache
from mcc_native_map_owner_publication import verify_owner_index, repair_owner_index
from prepare_mcc_gameplay import save_changed_cache
from prepare_mcc_elongation_glass import atomic_bytes, atomic_json, unshare, prefix_digest, sizes, bitmap, rgba

MAPS = "halo2/h2_maps_win64_dx11/"
BANK = MAPS + "textures.dat"


def prepare(root, source, output, report_path, *, before_owner_refresh=None,
            beforeimage_policy_root=None):
    root, source = Path(root).resolve(strict=True), Path(source).resolve(strict=True)
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    output = Path(output).resolve()
    if output.exists() or output == source or output.is_relative_to(source) or source.is_relative_to(output):
        raise FormatError("Choose a new non-overlapping native sky candidate directory")
    if scratch.stat().st_dev != source.stat().st_dev or output.parent.stat().st_dev != source.stat().st_dev:
        raise FormatError("Owned scratch, source and COW output must share one filesystem")
    bundle_path, manifest, packed = read_v7(root)
    map_names = list(dict.fromkeys(owner["map"] for owner in manifest["owners"]))
    names = [MAPS + name for name in map_names] + [BANK, "provenance.json", "native-shaders/map-owners.json"]
    before = {name: file_sha(source / name) for name in names}
    registry = root / "assets/mcc-source.tsv"
    registry_before = registry.read_bytes() if registry.exists() else None
    provenance = json.loads((source / "provenance.json").read_bytes())
    beforeimage_options = {} if beforeimage_policy_root is None else {
        "policy_root": Path(beforeimage_policy_root).resolve(strict=True)}
    beforeimage_policies = {} if beforeimage_policy_root is None else {
        name: file_sha(beforeimage_options["policy_root"] / name)
        for name in ("mcc-native-library-closure.json", "mcc-native-hud-bootstrap.json")}
    verified = verify_owner_index(source, provenance, **beforeimage_options)
    if verified is None:
        raise FormatError("Native sky candidate requires authenticated prepared map owners")
    corrections = []
    for owner in manifest["owners"]:
        name = owner["map"]
        if before[MAPS + name] != owner["prepared_map_sha256"]:
            raise FormatError("Native sky prepared donor map fingerprint differs: " + name)
        entry = next(e for e in manifest["profiles"] if e["profile_id"] == owner["profile_id"])
        profile = entry["profile"]
        cache = MccCache(source / MAPS / name)
        try:
            at = bitmap(cache, profile)
            tag = cache.tags[owner["datum"]]
            validate_owner_v7(root, name, cache, tag, at)
            donor = rgba(cache, source / BANK, profile)
            images, _, _, _ = reconstruct_v7(root, {profile["tag"]: donor}, map_name=name)
            if len(images) != 1 or images[0][0] != profile:
                raise FormatError("Native sky donor has an unexpected correction cohort")
            corrections.append(dict(map=name, profile=profile, target=images[0][1], donor_sha256=digest(donor),
                descriptor_before=bytes(cache.data[at:at + 168])))
        finally:
            cache.close()
    stage = scratch / "native-sky-candidate"
    try:
        shutil.copytree(source, stage, copy_function=os.link, symlinks=True)
        stage_provenance = json.loads((stage / "provenance.json").read_bytes())
        stage_verified = verify_owner_index(stage, stage_provenance, **beforeimage_options)
        for name in [MAPS + name for name in map_names] + [BANK]:
            unshare(stage / name)
            if (stage / name).stat().st_ino == (source / name).stat().st_ino:
                raise FormatError("Native sky COW mutation retained a shared source inode")
        bank_extent = (stage / BANK).stat().st_size
        resources = {}
        with (stage / BANK).open("ab") as stream:
            for correction in corrections:
                profile, target = correction["profile"], correction["target"]
                native = bytearray(target)
                if profile["encoding"] == "rgba8":
                    native[0::4], native[2::4] = target[2::4], target[0::4]
                identity = (len(native), digest(native))
                if identity not in resources:
                    pointer = stream.tell()
                    partition = struct.pack("<Ii", 1, -len(native)) + native
                    if pointer + len(partition) >= 0x40000000:
                        raise FormatError("Native sky texture bank exceeds resource address capacity")
                    stream.write(partition)
                    resources[identity] = pointer, len(partition)
                pointer, extent = resources[identity]
                correction["fields"] = [(4, struct.pack("<H", profile["width"])),
                    (6, struct.pack("<H", profile["height"])), (12, struct.pack("<H", profile["format"])),
                    (16, struct.pack("<hh", *profile["registration"])),
                    (20, struct.pack("<H", profile["mip_count"] - 1)),
                    (56, struct.pack("<I", pointer)), (80, struct.pack("<I", extent))]
        for correction in corrections:
            name, profile = correction["map"], correction["profile"]
            cache = MccCache(stage / MAPS / name)
            try:
                at = bitmap(cache, profile)
                for offset, raw in correction["fields"]:
                    cache.data[at + offset:at + offset + len(raw)] = raw
                save_changed_cache(cache)
                check = MccCache(cache.path)
                try:
                    if check.data != cache.data:
                        raise FormatError("Native sky map publication changed unowned bytes")
                    target = rgba(check, stage / BANK, profile, donor=False)
                    at = bitmap(check, profile, donor=False)
                    unchanged = bytearray(check.data[at:at + 168])
                    for offset, raw in correction["fields"]:
                        unchanged[offset:offset + len(raw)] = correction["descriptor_before"][offset:offset + len(raw)]
                    if bytes(unchanged) != correction["descriptor_before"] or target != correction["target"]:
                        raise FormatError("Native sky target pixels or unowned descriptor bytes differ")
                    correction["target_mip_sha256"] = hashes(target, profile)
                finally:
                    check.close()
            finally:
                cache.close()
        if prefix_digest(stage / BANK, bank_extent) != before[BANK]:
            raise FormatError("Native sky correction changed the existing resource bank prefix")
        directory = stage / "graphics-corrections"
        current = json.loads((directory / "profile.json").read_bytes())
        current["schema"] = "H2_MCC_GRAPHICS_CORRECTIONS_V7"
        current.setdefault("bundles", []).append(manifest)
        atomic_json(directory / "profile.json", current)
        atomic_bytes(directory / bundle_path.name, packed)
        # Preserve the exact trusted manifest bytes rather than reserializing.
        atomic_bytes(directory / bundle_path.with_suffix("").name, bundle_path.with_suffix("").read_bytes())
        graphics = stage_provenance["graphics_corrections"]
        graphics.update(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V7",
            profile_sha256=file_sha(directory / "profile.json"),
            restored_mips=graphics["restored_mips"] + sum(c["profile"]["mip_count"] for c in corrections),
            full_graphics_parity=False,
            scope=graphics.get("scope", "") + "; eleven authenticated complete native sky texture chains")
        for correction in corrections:
            profile = correction["profile"]
            graphics["profiles"].append(dict(map=correction["map"], tag=profile["tag"],
                restored_mips=profile["mip_count"], format=profile["format"], target_sha256=profile["target_sha256"]))
        changes = [dict(path=name, donor_sha256=before[name], converted_sha256=file_sha(stage / name),
                        producer=Path(__file__).name, profile="H2_MCC_SKY_BITMAP_COW_V7")
                   for name in [MAPS + name for name in map_names] + [BANK]]
        graphics["changes"].extend(changes)
        stage_provenance["graphics_v7_candidate"] = dict(source=str(source), candidate=str(output),
            selected=False, original_reference_runtime_input=False)
        atomic_json(stage / "graphics-conversion.json", graphics)
        atomic_json(stage / "provenance.json", stage_provenance)
        # Offline policy maintenance may need the exact completed PC map hash
        # before adding its finite input row. The private candidate remains
        # owned scratch until the callback and owner verification succeed.
        if before_owner_refresh is not None:
            before_owner_refresh(stage, changes)
        repair_owner_index(stage, stage_provenance, verified_beforeimage=stage_verified)
        verify_owner_index(stage, json.loads((stage / "provenance.json").read_bytes()))
        from mcc_onboarding import _needs_graphics_reimport, private_cache
        if _needs_graphics_reimport(root, stage_provenance):
            raise FormatError("Native sky candidate lacks the complete current correction cohort")
        if any(file_sha(source / name) != pin for name, pin in before.items()) or (
                registry.read_bytes() if registry.exists() else None) != registry_before:
            raise FormatError("Native sky candidate changed its source cache or selection registry")
        retained = sizes(stage)
        stage.rename(output)
        private_cache(root, output, verify=True)
        report = dict(schema="H2_MCC_SKY_BITMAP_COW_V7", passed=True, candidate=str(output),
            source=str(source), candidate_selected=False, original_assets_unchanged=True,
            source_unchanged=True, registry_unchanged=True, restored_mips=graphics["restored_mips"],
            beforeimage_policy_sha256=beforeimage_policies,
            retained=retained, changes=changes,
            corrections=[dict(map=c["map"], tag=c["profile"]["tag"], donor_sha256=c["donor_sha256"],
                target_sha256=c["profile"]["target_sha256"], target_mip_sha256=c["target_mip_sha256"],
                descriptor_changed_fields=[offset for offset, _ in c["fields"]]) for c in corrections])
        atomic_json(Path(report_path), report)
        return report
    finally:
        if stage.exists():
            shutil.rmtree(stage)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--beforeimage-policy-root", type=Path,
        help="Preserved policy directory that authenticates the source cache's existing index")
    args = parser.parse_args()
    print(json.dumps(prepare(args.root, args.source, args.output, args.report,
        beforeimage_policy_root=args.beforeimage_policy_root), indent=2))
