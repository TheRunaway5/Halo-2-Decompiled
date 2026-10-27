"""Exercise the real Zanzibar source owner, old salted handles and rollback."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
from unittest import mock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--native-root", default="")
    args = parser.parse_args()
    if not args.native_root:
        print("World bitmap owner requires the optional owned native-library directory")
        return 77
    root = args.repo_root.resolve(strict=True)
    prepared = Path(args.native_root).resolve(strict=True).parent
    sys.path.insert(0, str(root / "tools"))
    import prepare_mcc_world_bitmap_owner as subject
    from audit_rendering import FormatError
    from mcc_content import MccCache
    from prepare_mcc_gameplay import save_changed_cache

    policy, _ = subject.read_policy(root)
    maps = prepared / subject.MAPS
    donor = MccCache(maps / policy["donor"]["map"])
    cases = 0
    try:
        tag, at = donor.tag(policy["donor"]["datum"], "bitm")
        metadata = bytes(donor.data[at:at + tag["size"]])
        assert hashlib.sha256(metadata).hexdigest() == policy["donor"]["metadata_sha256"]
        pixels = subject.image(donor, tag, maps / "textures.dat", policy)
        assert hashlib.sha256(pixels).hexdigest() == policy["original"]["pixels_sha256"]
        cases += 1
        with tempfile.TemporaryDirectory(prefix="world-bitmap-owner-", dir=os.environ["HALO2_TEST_SCRATCH"]) as folder:
            owned = Path(folder)
            target = owned / "target.map"
            shutil.copyfile(maps / policy["target"]["map"], target)
            cache = MccCache(target)
            try:
                # The helper is also run against qualified V8 by the main
                # renderer sweep. Its insertion unit needs the V7 preimage.
                if any(t["name"] == policy["tag"] for t in cache.tags.values()):
                    print("Insertion unit requires the retained V7 source preimage")
                    return 77
                original = bytes(cache.data)
                old_owners = copy.deepcopy(cache.tags)
                for label, body, expected_count in (
                        ("wrong directory count", metadata, policy["target"]["directory_count"] + 1),
                        ("truncated complete metadata", metadata[:-1], policy["target"]["directory_count"]),
                        ("changed full-salt self owner", metadata[:108] + bytes(4) + metadata[112:],
                         policy["target"]["directory_count"])):
                    try:
                        subject.append_bitmap(cache, body, policy["tag"], tag["datum"], expected_count)
                    except FormatError:
                        pass
                    else:
                        raise AssertionError(label + " was admitted")
                    assert bytes(cache.data) == original and cache.tags == old_owners
                    cases += 1
                result = subject.append_bitmap(cache, metadata, policy["tag"], tag["datum"],
                                               policy["target"]["directory_count"])
                save_changed_cache(cache)
                check = MccCache(target)
                try:
                    assert all(check.tags[datum] == before for datum, before in old_owners.items())
                    inserted, _ = check.tag(result["datum"], "bitm")
                    assert inserted["name"] == policy["tag"]
                    assert subject.image(check, inserted, maps / "textures.dat", policy) == pixels
                    cases += 1
                    mutated = copy.deepcopy(policy)
                    mutated["pixels"]["mips"][-1]["sha256"] = "0" * 64
                    try:
                        subject.image(check, inserted, maps / "textures.dat", mutated)
                    except FormatError:
                        pass
                    else:
                        raise AssertionError("changed final original mip was admitted")
                    cases += 1
                    before = bytes(check.data)
                    try:
                        subject.append_bitmap(check, metadata, policy["tag"], tag["datum"],
                                              policy["target"]["directory_count"] + 1)
                    except FormatError:
                        pass
                    else:
                        raise AssertionError("duplicate typed bitmap identity was admitted")
                    assert bytes(check.data) == before
                    cases += 1
                finally:
                    check.close()
            finally:
                cache.close()

            # Inject a publication failure only inside the owned temporary
            # stage. This checks rollback independently of owner admission.
            stage = owned / "rollback"
            stage.mkdir()
            files = [subject.MAPS / policy["target"]["map"], Path("provenance.json"),
                     Path("native-shaders/map-owners.json"), Path("graphics-conversion.json"),
                     Path("graphics-corrections/profile.json")]
            originals = {}
            for name in files:
                (stage / name).parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(prepared / name, stage / name)
                originals[name] = (stage / name).read_bytes()
            sidecar = stage / "graphics-corrections/native-world-bitmap-owner-v1.json"

            def rejected(*_):
                for name in files:
                    subject.atomic_bytes(stage / name, b"owned failed publication")
                subject.atomic_bytes(sidecar, b"owned failed sidecar")
                raise RuntimeError("injected owner publication failure")

            with mock.patch.object(subject, "_prepare", side_effect=rejected):
                try:
                    subject.prepare(root, prepared, stage, owned / "report.json")
                except RuntimeError as error:
                    assert str(error) == "injected owner publication failure"
                else:
                    raise AssertionError("injected publication failure was ignored")
            assert not sidecar.exists()
            assert all((stage / name).read_bytes() == raw for name, raw in originals.items())
            cases += 1
        print(f"{cases} exact source-mip, salted-owner, rejection and rollback cases passed")
        return 0
    finally:
        donor.close()


if __name__ == "__main__":
    raise SystemExit(main())
