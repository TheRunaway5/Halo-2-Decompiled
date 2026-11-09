#!/usr/bin/env python3
"""Generate the finite original global camouflage closure for Uplift and District."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from generate_mcc_native_pc_world_programs import SOURCES
from mcc_content import MccCache
from native_program_storage_graph import storage_graph

ROOT = Path(__file__).resolve().parents[1]
SHADER = ("shad", r"rasterizer\render_layer_disabled")
BITMAPS = [
    ("bitm", r"shaders\default_bitmaps\bitmaps\default_vector"),
    ("bitm", r"shaders\default_bitmaps\bitmaps\color_white"),
]


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError("PC global camouflage: " + message)


def key(record):
    return record["group"], record["name"]


def generate(cache):
    native = cache / "native-shaders"
    maps = cache / "halo2/h2_maps_win64_dx11"
    original = {}
    for name in ("coagulation", "01b_spacestation"):
        source = json.loads((native / name / "library.json").read_bytes())
        require(digest(canonical(source)) == SOURCES[name], "changed complete original library")
        original[name] = {key(row): row for row in source["shader_tags"]}
        require(len(original[name]) == len(source["shader_tags"]), "duplicate original owner")

    def payload(name, record):
        filename = record["payload"]
        data = (native / name / "payloads" / filename).read_bytes()
        require(len(data) == record["bytes"] and digest(data) + ".bin" == filename,
                "changed original payload")
        return data

    donor = original["coagulation"]
    closure, pending = {}, [SHADER]
    while pending:
        identity = pending.pop()
        if identity in closure or identity in BITMAPS:
            continue
        require(identity in donor, "unowned original dependency")
        record = donor[identity]
        require(not record["strings"], "unexpected original StringID dependency")
        payload("coagulation", record)
        closure[identity] = record
        pending.extend(tuple(target) for _, target in record["references"])
    require(len(closure) == 9, "changed original nine-record closure")
    stencil = ("vrtx", r"rasterizer\vertex_shaders_xbox\stencil_texture")
    retain = original["01b_spacestation"][stencil]
    before = storage_graph(retain, payload("01b_spacestation", retain))
    after = storage_graph(closure[stencil], payload("coagulation", closure[stencil]))
    require(before["graph"] == after["graph"], "existing stencil program differs from original camo closure")

    world = json.loads((ROOT / "tools/data/mcc-native-pc-world-programs.json").read_bytes())
    shared = MccCache(maps / "shared.map")
    source_raw = payload("coagulation", closure[SHADER])
    u32 = lambda data, off: struct.unpack_from("<I", data, off)[0]
    orig_post = dict(closure[SHADER]["pointers"])[0x24]
    orig_ptrs = dict(closure[SHADER]["pointers"])
    orig_refs = {at: tuple(target) for at, target in closure[SHADER]["references"]}
    rows = []
    for name in ("needle", "street_sweeper"):
        path = maps / (name + ".map")
        encoded = path.read_bytes()
        profile = next(row for row in world["maps"] if row["map"] == name)
        require(digest(encoded) == profile["sha256"] and len(encoded) == profile["bytes"],
                "unknown exact PC map input")
        local = MccCache(path)
        try:
            def owner(datum, group):
                t = local.tags.get(datum)
                src = local
                if not t or t["offset"] is None:
                    src = shared
                    inherited = shared.tags[datum]
                    require(not t or key(t) == key(inherited), "changed shared typed identity")
                    t = inherited
                require(t["group"] == group and t["offset"] is not None, "unresident typed metadata")
                return src, t, t["offset"]

            def region(src, at, size):
                src.require(at, size)
                return bytes(src.data[at:at+size])

            def identity(datum, group):
                return key(owner(datum, group)[1])

            globals_tag, = [t for t in local.tags.values() if key(t) == ("matg", r"globals\globals")]
            g, _, at = owner(globals_tag["datum"], "matg")
            raster, = g.block(at + 0x108, 0xd4)
            require(g.u32(raster + 0xcc) == 0x73686164, "invalid rasterizer shader group")
            datum = g.u32(raster + 0xd0)
            src, tag, base = owner(datum, "shad")
            require(key(tag) == SHADER, "unexpected global material")
            raw = region(src, base, tag["size"])
            require(identity(u32(raw, 4), "stem") == orig_refs[4] and raw[0x3c:0x54] == source_raw[0x3c:0x54],
                    "original typed root or lighting fields differ")
            post = src.block(base + 0x20, 124)
            require(len(post) == 3 and identity(src.u32(post[0]), "stem") == orig_refs[orig_post],
                    "changed primary cooked material row")
            require(all(base <= address and address+124 <= base+tag["size"] for address in post),
                    "cooked row escapes its authenticated complete material")
            shared_regions = []
            for field, stride in ((4, 12), (12, 4), (20, 16), (28, 6), (36, 2),
                                  (44, 2), (52, 10), (60, 20), (68, 4), (76, 2),
                                  (84, 4), (92, 4), (100, 2), (108, 2)):
                live = src.block(post[0] + field, stride)
                count = u32(source_raw, orig_post + field)
                require(len(live) == count, "original and MCC cooked row count differs")
                if not count:
                    continue
                old = orig_ptrs[orig_post + field + 4]
                for i, address in enumerate(live):
                    a = old + i * stride
                    b = region(src, address, stride)
                    if not base <= address <= base+tag["size"]-stride:
                        shared_regions.append(dict(address=address+src.magic, bytes=stride,
                                                   sha256=digest(b)))
                    if field == 4:
                        require(identity(u32(b, 0), "bitm") == orig_refs[a], "changed bitmap argument")
                        b = source_raw[a:a+4] + b[4:]
                    require(b == source_raw[a:a+stride], "changed authored cooked material operand")

            bitmap_rows = []
            for group, path_name in BITMAPS:
                matches = {t["datum"] for c in (local, shared) for t in c.tags.values() if key(t) == (group, path_name)}
                require(len(matches) == 1, "ambiguous destination bitmap")
                bitm = matches.pop()
                b, bt, bp = owner(bitm, group)
                bitmap_rows.append(dict(name=path_name, datum=bitm, bytes=bt["size"],
                                        sha256=digest(region(b,bp,bt["size"]))))
            rows.append(dict(map=name, bytes=len(encoded), sha256=digest(encoded),
                             globals=globals_tag["datum"], shader=datum, shader_bytes=tag["size"],
                             shader_sha256=digest(raw), shared_regions=shared_regions, bitmaps=bitmap_rows))
        finally:
            local.close()
    shared.close()
    return dict(schema="H2_MCC_NATIVE_PC_CAMOUFLAGE_V1", maps=rows,
                source_map="coagulation", source_canonical_sha256=SOURCES["coagulation"],
                records=[closure[k] for k in sorted(closure) if k != stencil],
                stencil=dict(name=stencil[1], payload=retain["payload"],
                             source_payload=closure[stencil]["payload"],
                             typed_graph_sha256=before["graph_sha256"]))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--cache", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    body = canonical(generate(args.cache)).decode()
    generated = (
        '// Generated by tools/generate_mcc_native_pc_camouflage.py; do not edit.\n'
        'constexpr std::string_view native_pc_camouflage_policy=R"H2PCCAMO(' + body + ')H2PCCAMO";\n')
    if args.check:
        require(args.output.read_text() == generated, "stale generated policy")
    else:
        args.output.write_text(generated)


if __name__ == "__main__":
    main()
