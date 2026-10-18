#!/usr/bin/env python3
"""Regenerate the ignored Reach FLOAT3 fixture from owned OG Xbox TU1.5 inputs.

Only metadata/stream decoding is performed; no game or guest code is executed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from inspect_multiplayer import Cache

XBE_SHA = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
MAP_SHA = "5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28"


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def word(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def generate(map_path, xbe_path):
    if sha(map_path) != MAP_SHA or sha(xbe_path) != XBE_SHA:
        raise ValueError("Expected authenticated OG Xbox TU1.5 XBE and original opening map")
    xbe = xbe_path.read_bytes()
    base, = struct.unpack_from("<I", xbe, 0x104)
    count, table = struct.unpack_from("<II", xbe, 0x11C)
    sections = [struct.unpack_from("<5I", xbe, table - base + i * 56) for i in range(count)]

    def read(address, size):
        for _, virtual, _, offset, raw_size in sections:
            if virtual <= address and address + size <= virtual + raw_size:
                return xbe[offset + address - virtual:offset + address - virtual + size]
        raise ValueError("Declaration escapes authenticated image")

    # Literal4494C8 type1: POSITION/internal2 (FLOAT3); type25: UV/internal13.
    for kind, semantic, fmt, d3d, size in ((1, 0, 2, 0x32, 12), (25, 3, 13, 0x21, 4)):
        declaration = read(0x4494C8 + kind * 21, 21)
        if declaration[1:3] != bytes((semantic, fmt)) or declaration[3] != 255:
            raise ValueError("Original stream declaration differs")
        for dispatch, expected in ((0x357F4, d3d), (0x35878, size)):
            target, = struct.unpack("<I", read(dispatch + fmt * 4, 4))
            code = read(target, 5)
            if code[0] != 0xB8 or struct.unpack_from("<I", code, 1)[0] != expected:
                raise ValueError("Original format/size dispatch differs")

    cache = Cache(map_path)
    try:
        model = cache.tags[0xE26800F3]
        if model["group"] != "mode" or model["name"] != "objects\\cinematics\\matte_paintings\\reach_space\\reach_space":
            raise ValueError("Reach render-model identity differs")
        at = model["offset"]
        section, = cache.block(at + 0x24, 0x5C)
        vertices = cache.h(section + 4)
        if vertices != 8 or cache.h(section) != 1 or cache.h(section + 0x1A) != 2:
            raise ValueError("Reach class/count/UV-only compression flags differ")
        pointer, size = cache.u(section + 0x38), cache.u(section + 0x3C)
        if pointer >> 30:
            raise ValueError("Reach resource is not map-local")
        raw = cache.data[pointer:pointer + size]
        resource_base = size - cache.u(section + 0x44) - 4
        resources = {}
        parts = None
        for ordinal, p in enumerate(cache.block(section + 0x48, 16)):
            kind, channel, length, offset = struct.unpack_from("<HHII", cache.data, p + 4)
            start = resource_base + offset
            if not 0 <= start <= start + length <= len(raw):
                raise ValueError("Reach resource extent differs")
            resources[kind, channel] = raw[start:start + length]
            if ordinal == 0:
                parts = raw[start:start + length]
        descriptors = resources[56, 32]
        if descriptors[:2] != bytes((1, 12)) or descriptors[32:34] != bytes((25, 4)):
            raise ValueError("Authored Reach stream descriptor differs")
        positions = resources[56, 0]
        uv = resources[56, 1]
        if len(positions) != vertices * 12 or len(uv) != vertices * 4 or len(parts) != 72:
            raise ValueError("Authored Reach stream extents differ")
        bounds_at, = cache.block(at + 0x14, 0x38)
        bounds = struct.unpack_from("<10f", cache.data, bounds_at)
        if bounds[:6] != (0.0,) * 6:
            raise ValueError("Reach XYZ bounds differ")
        texcoords = []
        for i in range(vertices):
            row = []
            for axis in range(2):
                short, = struct.unpack_from("<h", uv, i * 4 + axis * 2)
                normalized = max(-1.0, f32(short / 32767.0))
                low, high = bounds[6 + axis * 2:8 + axis * 2]
                scale, center = f32(f32(high - low) * 0.5), f32(f32(low + high) * 0.5)
                row.append(word(f32(f32(normalized * scale) + center)))
            texcoords.append(row)
        indices = resources[32, 2]
        first, length = struct.unpack_from("<HH", parts, 6)
        strip = struct.unpack_from("<" + "H" * length, indices, first * 2)
        expanded = []
        for i in range(len(strip) - 2):
            triangle = list(strip[i:i + 3])
            if len(set(triangle)) < 3:
                continue
            if i & 1:
                triangle[0], triangle[1] = triangle[1], triangle[0]
            if any(index >= vertices for index in triangle):
                raise ValueError("Reach topology escapes source vertices")
            expanded.extend(triangle)
        return {"positions": [list(struct.unpack_from("<3I", positions, i * 12)) for i in range(vertices)],
                "uv": texcoords, "expanded_indices": expanded}
    finally:
        cache.data.close()
        cache.stream.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--xbe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    data = (json.dumps(generate(args.map, args.xbe), indent=2) + "\n").encode()
    if args.output.exists():
        if args.output.read_bytes() != data:
            raise ValueError("Existing fixture differs; preserve it and use a new output path")
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(data)
    print("PASS authored Reach FLOAT3/packed-UV/topology fixture", hashlib.sha256(data).hexdigest())


if __name__ == "__main__":
    main()
