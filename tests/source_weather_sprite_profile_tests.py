"""Own the literal original/prepared64-square weather sprite resource fixture."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zlib


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def export(root, prepared):
    sys.path.insert(0, str(root / "tools"))
    from audit_rendering import Cache
    from mcc_content import MccCache, texture_resource
    name = "objects\\cinematics\\flood\\flood_spore\\bitmaps\\flood_spore"
    paths = (root / "assets/disc/maps/08b_deltacontrol.map",
             prepared / "halo2/h2_maps_win64_dx11/08b_deltacontrol.map")
    pins = ("2b71293c775ad33a73025b7a640ba45e88fc4c01c7783d79cf0cce8862cc2828",
            "4844a2ea86949fe33b051d86f7410e83f4f3c63d58b5e4aac140b8f7b55c680f")
    cases = []
    for mcc, path, pin in zip((False, True), paths, pins):
        if sha(path.read_bytes()) != pin:
            raise ValueError("Complete source weather cache changed")
        cache = MccCache(path) if mcc else Cache(path)
        try:
            tag = next(t for t in cache.tags.values() if t["group"] == "bitm" and t["name"] == name)
            at = tag["offset"] if mcc else cache.pointer(tag["address"], cache.magic, tag["size"])
            images = cache.block(at + 68, 168 if mcc else 116)
            if len(images) != 1:
                raise ValueError("Source weather bitmap frame count changed")
            at = images[0]
            descriptor = {key: cache.u16(at + field) for key, field in
                          (("width", 4), ("height", 6), ("depth", 8), ("type", 10),
                           ("format", 12), ("flags", 14), ("declared_mips", 20))}
            if mcc:
                with (path.parent / "textures.dat").open("rb") as stream:
                    raw = texture_resource(stream, cache.u32(at + 56), cache.u32(at + 80), decoded_limit=16384)
            else:
                pointer, size = cache.u32(at + 28), cache.u32(at + 52)
                if pointer >> 30:
                    raise ValueError("Unexpected original weather texture resource owner")
                cache.require(pointer, size)
                raw = bytes(cache.data[pointer:pointer + size])
            if sha(raw[:5456]) != "c5d137fbdb1b369f583f807f89ca580deac087933502583a639a5ac92232ca52":
                raise ValueError("Complete original/prepared weather mip payload changed")
            offsets, hashes = 0, []
            for side in (64, 32, 16, 8, 4):
                size = (side // 4) ** 2 * 16
                hashes.append(sha(raw[offsets:offsets + size]))
                offsets += size
            cases.append(dict(mcc=mcc, source_map_sha256=pin, name=name, datum=tag["datum"],
                              descriptor=descriptor, raw_resource_hex=raw.hex(), mip_sha256=hashes))
        finally:
            cache.close()
    return dict(schema="H2_WEATHER_SPRITE_RESOURCE_FIXTURE_V1",
                scope="Actual original/prepared resource data component; no live particle or GPU draw claim", cases=cases)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--test-executable", type=Path)
    parser.add_argument("--gpu-executable", type=Path)
    parser.add_argument("--prepared-root", type=Path)
    parser.add_argument("--export-output", type=Path)
    args = parser.parse_args()
    if not (args.test_executable or args.gpu_executable or args.export_output):
        parser.error("Select a component test, GPU test or owned export output")
    root = args.repo_root.resolve(strict=True)
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="weather-sprite-", dir=owner) as directory:
        if args.prepared_root:
            raw = (json.dumps(export(root, args.prepared_root.resolve(strict=True)), indent=2) + "\n").encode()
        else:
            folder = root / "tests/fixtures"
            metadata = json.loads((folder / "source-weather-sprite-v1.json").read_bytes())
            name = metadata["compressed_file"]
            if (metadata["schema"] != "H2_WEATHER_SPRITE_RESOURCE_REGRESSION_V1" or Path(name).name != name or
                    not 0 < metadata["compressed_bytes"] <= 16384 or not 0 < metadata["decoded_bytes"] <= 65536):
                raise ValueError("Invalid source weather resource fixture owner")
            packed = (folder / name).read_bytes()
            if len(packed) != metadata["compressed_bytes"] or sha(packed) != metadata["compressed_sha256"]:
                raise ValueError("Source weather resource fixture differs")
            decoder = zlib.decompressobj()
            raw = decoder.decompress(packed, metadata["decoded_bytes"] + 1)
            if (len(raw) != metadata["decoded_bytes"] or sha(raw) != metadata["decoded_sha256"] or
                    not decoder.eof or decoder.unused_data or decoder.unconsumed_tail):
                raise ValueError("Source weather resource fixture has invalid extent/hash/trailing data")
        fixture = Path(directory) / "source.json"
        fixture.write_bytes(raw)
        if args.export_output:
            target = args.export_output.resolve()
            if not target.is_relative_to(owner):
                raise ValueError("Exported raw fixture must stay inside owned scratch")
            target.write_bytes(raw)
        if args.test_executable:
            result = subprocess.run([str(args.test_executable.resolve(strict=True)), str(fixture)], check=False)
            if result.returncode:
                return result.returncode
        if args.gpu_executable:
            return subprocess.run([str(args.gpu_executable.resolve(strict=True)), "--gpu", "--sprite-fixture", str(fixture)], check=False).returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
