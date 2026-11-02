"""Build literal shared MCC env_illum operands in owned test scratch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


FIELDS = ((4, 12), (12, 4), (20, 16), (28, 6), (36, 2), (44, 2),
          (52, 10), (60, 20), (68, 4), (76, 2), (84, 4), (92, 4), (100, 12), (108, 4))
SHADER = r"objects\weapons\rifle\plasma_rifle\shaders\plasma_rifle_luminous"
TEMPLATE = r"shaders\shader_templates\opaque\tex_bump_env_illum"


def fixture(repo, prepared):
    sys.path.insert(0, str(repo / "tools"))
    from mcc_content import MccCache
    u32 = lambda b, n: struct.unpack_from("<I", b, n)[0]
    directory = prepared / "native-shaders/coagulation"
    library = json.loads((directory / "library.json").read_bytes())
    native = next(r for r in library["shader_tags"] if r["group"] == "stem" and r["name"] == TEMPLATE)
    assert native["payload"] == "9977d3432b7b9a28a5de532a6e33d343a86e77d07a7011c89f05c42e5e54f76b.bin"
    payload = (directory / "payloads" / native["payload"]).read_bytes()
    assert len(payload) == 1878 and hashlib.sha256(payload).hexdigest() + ".bin" == native["payload"]
    original = next(r for r in library["shader_tags"] if r["group"] == "shad" and r["name"] == SHADER)
    original_bytes = (directory / "payloads" / original["payload"]).read_bytes()
    assert hashlib.sha256(original_bytes).hexdigest() + ".bin" == original["payload"]
    maps = prepared / "halo2/h2_maps_win64_dx11"
    shared, cache = MccCache(maps / "shared.map"), MccCache(maps / "street_sweeper.map")
    try:
        def resolve(handle):
            tag = cache.tags[handle]
            if tag["offset"] is not None:
                return cache, tag
            other = shared.tags[handle]
            assert (other["group"], other["name"]) == (tag["group"], tag["name"]) and other["offset"] is not None
            return shared, other

        owner, shader = resolve(next(h for h, t in cache.tags.items() if t["group"] == "shad" and t["name"] == SHADER))
        root = shader["offset"]
        stem_owner, stem = resolve(owner.u32(root + 4))
        assert stem["name"] == TEMPLATE
        regions, names, frames = {}, {}, {}

        def namespace(handle):
            source, tag = resolve(handle)
            names[handle] = {k: tag[k] for k in ("group", "name", "datum", "address", "size")}
            if tag["group"] == "bitm":
                frames[handle] = len(source.block(tag["offset"] + 68, 168))

        def retain(source, pointer, extent):
            if extent:
                at = source.pointer(pointer, source.magic, extent)
                data = bytes(source.data[at:at + extent])
                old = regions.get(pointer, b"")
                assert not old or old == data[:len(old)] or data == old[:len(data)]
                if len(data) > len(old):
                    regions[pointer] = data

        namespace(shader["datum"])
        retain(owner, shader["address"], shader["size"])
        retain(owner, owner.u32(root + 16), 80)
        runtime = owner.pointer(owner.u32(root + 16), owner.magic, 80)
        for field in (4, 12, 48, 56):
            h = owner.u32(runtime + field)
            if h != 0xffffffff:
                namespace(h)
        retain(owner, owner.u32(root + 36), 3 * 124)
        pp = owner.pointer(owner.u32(root + 36), owner.magic, 3 * 124)
        for field, stride in FIELDS:
            count, pointer = owner.u32(pp + field), owner.u32(pp + field + 4)
            retain(owner, pointer, count * stride)
            if field == 4:
                for at in owner.block(pp + field, stride):
                    h = owner.u32(at)
                    if h != 0xffffffff:
                        namespace(h)
        retain(owner, owner.u32(root + 48), owner.u32(root + 44) * 8)
        prediction = owner.pointer(owner.u32(root + 48), owner.magic, owner.u32(root + 44) * 8)
        for n in range(owner.u32(root + 44)):
            namespace(owner.u32(prediction + n * 8 + 4))
        translated = dict(group="stem", name=TEMPLATE, datum=stem["datum"], address=0x70000000, size=len(payload))
        names[translated["datum"]] = translated
        # Retain the independently authenticated original schedule and main
        # slot identities. Authored MCC words remain the source of projection.
        native_pp = u32(original_bytes, 36)
        expected = {}
        for field, stride in FIELDS:
            count, at = u32(original_bytes, native_pp + field), u32(original_bytes, native_pp + field + 4)
            expected[str(field)] = [original_bytes[at + n * stride:at + (n + 1) * stride].hex() for n in range(count)]
        reference = {at: ref for at, ref in original["references"]}
        main = dict(bitmap=[], pixel=[], vertex=[])
        for n in range(u32(original_bytes, native_pp + 4)):
            at = u32(original_bytes, native_pp + 8) + n * 12
            main["bitmap"].append([reference.get(at, ["NONE", ""]), u32(original_bytes, at + 4)])
        for n in range(u32(original_bytes, native_pp + 12)):
            main["pixel"].append(u32(original_bytes, u32(original_bytes, native_pp + 16) + n * 4))
        for n in range(u32(original_bytes, native_pp + 20)):
            main["vertex"].append(list(struct.unpack_from("<4I", original_bytes, u32(original_bytes, native_pp + 24) + n * 16)))
        return dict(schema="H2_MCC_SHARED_ENV_ILLUM_FIXTURE_V1", native_payload_name=native["payload"],
                    native_payload_hex=payload.hex(), cases=[dict(name=SHADER, shader=names[shader["datum"]], native_stem=translated,
                    namespace=list(names.values()), bitmap_frame_counts={str(k): n for k, n in frames.items()},
                    regions=[[at, b.hex()] for at, b in sorted(regions.items())], expected_original_banks=expected,
                    expected_original_main=main, expected_original_predictions=None)])
    finally:
        cache.close()
        shared.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--test-executable", type=Path, required=True)
    parser.add_argument("--asset-source-root", type=Path)
    parser.add_argument("--neighbor-audit", action="store_true")
    args = parser.parse_args()
    repo = args.repo_root.resolve(strict=True)
    asset_root = (args.asset_source_root or repo).resolve(strict=True)
    prepared = asset_root / "assets/imported/mcc-sky-water-tu15-20261025-v7"
    if not (prepared / "halo2/h2_maps_win64_dx11/street_sweeper.map").is_file():
        return 77
    with tempfile.TemporaryDirectory(prefix="shared-env-illum-", dir=os.environ["HALO2_TEST_SCRATCH"]) as folder:
        path = Path(folder) / "fixture.json"
        path.write_text(json.dumps(fixture(repo, prepared), separators=(",", ":")))
        command = [str(args.test_executable.resolve(strict=True)), str(path), str(prepared)]
        if args.neighbor_audit:
            command.append("--neighbor-audit")
        return subprocess.run(command, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
