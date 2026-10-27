#!/usr/bin/env python3
"""Extract the finite original water-beach owner into disposable component input.

No prepared cache, texture bank, runtime owner or selected asset is modified.
This recipe proves the lossless storage transformation required by a future
coordinated private-stage writer. It does not publish that writer's output.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from audit_rendering import Cache, FormatError

ROOT = Path(__file__).resolve().parents[1]
POLICY = "tools/data/mcc-native-water-beach-owner-v1.json"
MAP = "assets/disc/maps/03a_oldmombasa.map"
MAP_SHA = "7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4"
NAME = "scenarios\\bitmaps\\water\\water_earthcity_beach"
SHADER = "scenarios\\shaders\\solo\\earthcity\\water_beach_overlay"
SHADER_PAYLOAD = "ddd34378a0b4329369a1d93ed9edb2b13065cc72ff9e379eb7020fa16126b6d8.bin"


def require(ok, message):
    if not ok:
        raise FormatError("Original water-beach owner: " + message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def linear_rg8(raw, side):
    require(len(raw) == side * side * 2, "complete Morton mip extent differs")
    result = bytearray(len(raw))
    # Same positive power-of-two 2D Morton storage order as prepare_texture.
    for y in range(side):
        for x in range(side):
            address = sum(((x >> bit) & 1) << (bit * 2) |
                          ((y >> bit) & 1) << (bit * 2 + 1) for bit in range(side.bit_length() - 1))
            at = (y * side + x) * 2
            result[at:at + 2] = raw[address * 2:address * 2 + 2]
    return bytes(result)


def rgba8(raw):
    require(len(raw) % 2 == 0, "incomplete G8B8 word")
    result = bytearray(len(raw) * 2)
    result[0::4] = result[2::4] = raw[0::2]
    result[1::4] = result[3::4] = raw[1::2]
    return bytes(result)


def extract(root):
    cache = Cache(root / MAP)
    try:
        require(sha(cache.data) == MAP_SHA, "complete original map differs")
        tag = next(t for t in cache.tags.values() if t["group"] == "bitm" and t["name"] == NAME)
        _, at = cache.tag(tag["datum"], "bitm")
        require(tag["datum"] == 2160402042 and tag["size"] == 2048 and
                cache.u32(at + 60) == cache.u32(at + 64) == 0, "original complete typed metadata differs")
        records = cache.block(at + 68, 116)
        require(len(records) == 17 and records[0] == at + 76, "complete original frame allocation differs")
        shader = next(t for t in cache.tags.values() if t["group"] == "shad" and t["name"] == SHADER)
        _, shader_at = cache.tag(shader["datum"], "shad")
        definitions = cache.block(shader_at + 32, 124)
        require(len(definitions) == 1, "source water shader platform bank differs")
        images = cache.block(definitions[0] + 4, 12)
        require(images and cache.u32(images[0]) == tag["datum"] and cache.u32(images[0] + 4) == 0,
                "actual consuming source bitmap role differs")
        predicted = [cache.u16(row + 2) for row in cache.block(shader_at + 44, 8)
                     if cache.u32(row + 4) == tag["datum"]]
        require(predicted == list(range(17)), "actual complete source predicted frame ownership differs")
        original, projected, frames = bytearray(), bytearray(), []
        for index, frame in enumerate(records):
            require(tuple(cache.u16(frame + offset) for offset in (4, 6, 8, 10, 12, 14, 20)) ==
                    (128, 128, 1, 0, 23, 4617, 2), "source frame shape/format/flags changed")
            pointer, size = cache.u32(frame + 28), cache.u32(frame + 52)
            require(pointer >> 30 == 0 and size == 43008,
                    "source hardware partition contract differs")
            cache.require(pointer, size)
            raw = bytes(cache.data[pointer:pointer + size])
            partitions = []
            for slot in range(3):
                address, count = cache.u32(frame + 28 + slot * 4), cache.u32(frame + 52 + slot * 4)
                part = dict(slot=slot, source_pointer=address, source_bytes=count)
                if count:
                    require(address >> 30 == 0, "source hardware partition escaped its actual cache owner")
                    cache.require(address, count)
                    data = bytes(cache.data[address:address + count])
                    part["sha256"] = sha(data)
                    if slot == 1:
                        part["exact_highest_chain_64_32_suffix"] = data == raw[32768:]
                partitions.append(part)
            blob, blob_size = cache.u32(0x158), cache.u32(0x15c)
            low_offset, low_size = cache.u32(frame + 0x60), cache.u32(frame + 0x64)
            require(low_offset <= blob_size and low_size <= blob_size - low_offset,
                    "source low-detail resource escaped its exact shared blob")
            cache.require(blob + low_offset, low_size)
            low_detail = dict(source_shared_offset=low_offset, source_bytes=low_size,
                format=cache.u16(frame + 0x68), width=cache.u16(frame + 0x6a),
                height=cache.u16(frame + 0x6c), depth=cache.u16(frame + 0x6e),
                sha256=sha(cache.data[blob + low_offset:blob + low_offset + low_size]))
            offset, levels, linear = 0, [], bytearray()
            for side in (128, 64, 32):
                n = side * side * 2
                source = raw[offset:offset + n]
                converted = linear_rg8(source, side)
                levels.append(dict(width=side, height=side, rg8_offset=offset, rg8_bytes=n,
                                   original_morton_sha256=sha(source), linear_rg8_sha256=sha(converted),
                                   decoded_rgba8_sha256=sha(rgba8(converted))))
                offset += n
                linear.extend(converted)
            frames.append(dict(frame=index, metadata_hex=bytes(cache.data[frame:frame + 116]).hex(),
                               source_resource_pointer=pointer, source_resource_bytes=size,
                               complete_hardware_partitions=partitions, software_low_detail=low_detail,
                               original_morton_sha256=sha(raw), linear_rg8_sha256=sha(linear),
                               decoded_rgba8_sha256=sha(rgba8(linear)), mips=levels))
            original.extend(raw)
            projected.extend(linear)
        metadata = bytes(cache.data[at:at + tag["size"]])
        policy = dict(schema="H2_MCC_NATIVE_WATER_BEACH_OWNER_V1",
            status="source-qualified-extraction-only-not-published", tag=NAME,
            original=dict(map=MAP, map_sha256=MAP_SHA, tag=tag, complete_metadata_sha256=sha(metadata),
                          root_metadata_hex=metadata[:76].hex(), frame_count=17, frames=frames),
            consuming_native_shader=dict(map="03a_oldmombasa", name=SHADER, payload=SHADER_PAYLOAD,
                original_tag=shader, original_body_sha256=sha(cache.data[shader_at:shader_at + shader["size"]]),
                required_frames=predicted, scope="Actual source compiled bitmap slot0 and complete predicted frame ownership"),
            storage_projection=dict(format=23, width=128, height=128, depth=1, type=0,
                frame_count=17, mip_count=3, original_flags=4617, linear_flags=4617 & ~8,
                raw_rg8_bytes_per_frame=43008, total_rg8_bytes=len(original),
                total_decoded_rgba8_bytes=len(original) * 2, original_morton_pool_sha256=sha(original),
                projected_linear_pool_sha256=sha(projected), synthesized_mips=False,
                resampling=False, numeric_tolerance=False),
            future_private_writer=dict(status="not-created-root-coordination-required",
                highest_detail_resource_bank_delta_bytes=17 * (43008 + 8),
                resource_encoding="Seventeen existing-format uncompressed MCC partitions: count1, signed-size-43008, complete linear G8B8 mip bytes",
                typed_owner="Append one new bitm owner with17 actual frames; preserve every existing salted directory row and byte",
                metadata_requirements=["Source-authenticated76-byte original authoring root to80-byte MCC root projection",
                    "Complete17x116-byte original frame fields to17x168-byte MCC resource descriptors",
                    "Explicit root frame pointer and every per-frame full-salt datum relocation",
                    "Original frame order, registration, type23 and three literal mip extents retained",
                    "Only swizzle flag removed after lossless storage permutation; no authored field/value substitution",
                    "Separate original second hardware partitions and software low-detail descriptors must retain explicit source ownership; this highest-chain component test does not implement their MCC lifecycle",
                    "Independent Cache reopen: all17 decoded images and51 mip payloads exact",
                    "Full source/candidate map, textures.dat prefix, provenance and owner-index before/after pins"]))
        return policy, bytes(original), bytes(projected)
    finally:
        cache.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report-output", type=Path)
    parser.add_argument("--record-profile", action="store_true")
    parser.add_argument("--test-executable", type=Path)
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    output = args.output.resolve()
    require(output.is_relative_to(owner) and output != owner and not output.exists(),
            "extraction must use a new subdirectory of the active owned scratch")
    policy, original, projected = extract(root)
    policy_path = root / POLICY
    encoded = (json.dumps(policy, indent=2) + "\n").encode()
    if args.record_profile:
        require(not policy_path.exists(), "source profile already exists")
        policy_path.write_bytes(encoded)
    else:
        require(policy_path.read_bytes() == encoded, "complete source profile differs")
    output.mkdir()
    (output / "original-morton.rg8").write_bytes(original)
    (output / "projected-linear.rg8").write_bytes(projected)
    (output / "fixture.json").write_bytes(encoded)
    report = dict(schema=policy["schema"], scope=__doc__, profile_sha256=sha(encoded),
        original_frames=17, original_mips=51, source_or_prepared_assets_modified=False,
        prepared_derivative_created=False, owned_test_input=str(output),
        owned_test_input_bytes=len(original) + len(projected) + len(encoded),
        projection=policy["storage_projection"], future_private_writer=policy["future_private_writer"])
    if args.report_output:
        args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(dict(frames=17, mips=51, profile_sha256=sha(encoded),
                         owned_test_input_bytes=report["owned_test_input_bytes"], prepared_derivative_created=False)))
    if args.test_executable:
        return subprocess.run([str(args.test_executable.resolve(strict=True)),
                               str(output / "fixture.json")], check=False).returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
