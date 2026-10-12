#!/usr/bin/env python3
"""Audit literal Xbox vrtx register maps against the decoded world adapter.

Read-only caches/XBE/retained streams. The original 1C330 declaration producer
is executed unchanged over relocated literal semantic lists and the unchanged
XBE stream-declaration table. This is not original live class/admission proof.
"""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import struct
import sys

from audit_rendering import Cache
from audit_dynamic_model_shader_assets import MAPS

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe

SEMANTICS = {0: "position", 1: "blend_indices", 2: "blend_weights", 3: "uv",
             4: "normal", 5: "tangent", 6: "binormal", 8: "incident",
             9: "lightmap_uv", 14: "radiance", 19: "prt_cluster", 20: "prt_weights"}
GENERIC = {0: 0, 1: 3, 2: 4, 3: 5, 4: 6, 5: 8, 6: 9}


def reads(code):
    out = collections.Counter()
    operands = {0: (), 1: (0,), 2: (0, 1), 3: (0, 2), 4: (0, 1, 2),
                5: (0, 1), 6: (0, 1), 7: (0, 1), 8: (0, 1), 9: (0, 1),
                10: (0, 1), 11: (0, 1), 12: (0, 1), 13: (0,)}
    assert (len(code) - 4) % 16 == 0
    for off in range(4, len(code), 16):
        _, y, z, w = struct.unpack_from("<4I", code, off)
        mac, ilu = y >> 21 & 15, y >> 25 & 7
        used = set(operands[mac])
        if ilu:
            used.add(2)
        mux = (z >> 26 & 3, z >> 11 & 3, w >> 28 & 3)
        if any(mux[i] == 2 for i in used):
            out[y >> 9 & 15] += 1
    return dict(sorted(out.items()))


def current_adapter(leaf):
    mapping = GENERIC.copy()
    if leaf.startswith("lightmap_pervertex"):
        mapping[6] = 14
    if leaf in ("prt", "prt_lightmap"):
        mapping[5], mapping[6] = 19, 20
    if leaf.startswith("shadow_buffer_application"):
        # Already coordinated source phase marker in production.
        mapping[1] = 4
    return mapping


class OriginalDeclaration:
    def __init__(self, xbe):
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
        self.u = Uc(UC_ARCH_X86, UC_MODE_32)
        self.u.mem_map(0x10000, 0x600000)
        for s in xbe.sections:
            self.u.mem_write(s.va, xbe.section_bytes(s))
        self.u.mem_map(0x1000000, 0x100000)
        self.u.mem_map(0x3000000, 4096)
        self.xbe = xbe
        self.executed = 0

    def run(self, semantic_ids, stream_types):
        from unicorn.x86_const import UC_X86_REG_ESP
        u = self.u
        context, output, table, tag, classification, semantic = (
            0x1000000, 0x1001000, 0x1002000, 0x1003000, 0x1004000, 0x1005000)
        u.mem_write(context, bytes(0x300))
        u.mem_write(context + 4, struct.pack("<II", 0, 0))
        u.mem_write(0x4e3b44, struct.pack("<I", table))
        u.mem_write(table + 8, struct.pack("<I", tag))
        u.mem_write(tag + 8, struct.pack("<I", classification))
        u.mem_write(classification, struct.pack("<III", 0, len(semantic_ids), semantic))
        if semantic_ids:
            u.mem_write(semantic, struct.pack("<" + "H" * len(semantic_ids), *semantic_ids))
        expected = [[0, 0, 2, 0] for _ in range(16)]
        for stream, typ in enumerate(stream_types):
            declaration = self.xbe.read(0x43f408 + typ * 21, 21)
            u.mem_write(context + 0x8c + stream * 4,
                        struct.pack("<I", 0x43f408 + typ * 21))
            offset = 0
            for at in range(1, 21, 2):
                semantic_id, fmt = declaration[at:at + 2]
                if semantic_id == 255:
                    break
                if semantic_id == 254:
                    offset += fmt
                    continue
                if semantic_id in semantic_ids:
                    # Literal original format jump tables, not a host enum.
                    target = struct.unpack("<I", self.xbe.read(0x35804 + fmt * 4, 4))[0]
                    d3d = struct.unpack("<I", self.xbe.read(target + 1, 4))[0]
                    expected[semantic_ids.index(semantic_id)] = [stream, offset, d3d, 0]
                size_target = struct.unpack("<I", self.xbe.read(0x35888 + fmt * 4, 4))[0]
                size_code = self.xbe.read(size_target, 8)
                assert size_code[0] == 0xb8
                offset += struct.unpack_from("<I", size_code, 1)[0]
        u.mem_write(context + 0x2d0, struct.pack("<I", len(stream_types)))
        stack = 0x10fff00
        u.mem_write(stack, struct.pack("<III", 0x3000000, context, output))
        u.reg_write(UC_X86_REG_ESP, stack)
        u.emu_start(0x1c330, 0x3000000, count=100000)
        actual = list(struct.unpack("<64I", bytes(u.mem_read(output, 256))))
        assert actual == sum(expected, []), (semantic_ids, stream_types, actual, expected)
        self.executed += 1
        return expected


def shader_links(c):
    links = []
    for shader in c.tags.values():
        if shader["group"] != "shad":
            continue
        for pp in c.block(shader["address"] - c.magic + 0x20, 124):
            stem_datum = c.u32(pp)
            if stem_datum == 0xffffffff:
                continue
            _, stem = c.tag(stem_datum, "stem")
            for t in c.block(stem + 0x58, 40):
                layers, passes = c.block(t + 8, 2), c.block(t + 16, 10)
                for lod_index, lod in enumerate(c.block(t, 10)):
                    packed, mask = c.u16(lod), c.u32(lod + 2)
                    for layer, layer_at in enumerate(layers[packed & 511:(packed & 511) + (packed >> 9)]):
                        if not mask & (1 << layer):
                            continue
                        layer_range = c.u16(layer_at)
                        for pass_at in passes[layer_range & 511:(layer_range & 511) + (layer_range >> 9)]:
                            spas_datum = c.u32(pass_at + 4)
                            _, spas = c.tag(spas_datum, "spas")
                            for definition in c.block(spas + 0x1c, 88):
                                impls = c.block(definition, 306)
                                selected = c.u16(pass_at + 8)
                                # Cache::load_material records the implementation ordinal
                                # within this packed template pass range.
                                for index in range(selected >> 9):
                                    impl = impls[index]
                                    vrtx = c.u32(impl + 0x100)
                                    if vrtx == 0xffffffff:
                                        continue
                                    links.append(dict(shader=shader["datum"], shader_name=shader["name"],
                                        lod=lod_index, layer=layer, implementation=index,
                                        pass_name=c.tags[spas_datum]["name"], vrtx=vrtx))
    return links


def raw_model_streams(c, mode_datum, xbe):
    _, mode = c.tag(mode_datum, "mode")
    sections = []
    for ordinal, at in enumerate(c.block(mode + 0x24, 0x5c)):
        resources = c.block(at + 0x48, 16)
        if not resources:
            continue
        pointer, size, header = c.u32(at + 0x38), c.u32(at + 0x3c), c.u32(at + 0x44)
        resource = c.path if pointer >> 30 == 0 else c.path.parent / (
            "", "mainmenu.map", "shared.map", "single_player_shared.map")[pointer >> 30]
        if not resource.exists():
            resource = ROOT / "assets/disc/maps" / resource.name
        with resource.open("rb") as f:
            f.seek(pointer & 0x3fffffff)
            raw = f.read(size)
        assert len(raw) == size
        base = size - header - 4
        descriptors = next(r for r in resources if c.data[r] == 0 and c.u16(r + 4) == 56)
        streams = []
        for channel in range(c.u32(descriptors + 8) // 32):
            descriptor = raw[base + c.u32(descriptors + 12) + channel * 32:base + c.u32(descriptors + 12) + (channel + 1) * 32]
            payload = next(r for r in resources if c.data[r] == 2 and c.u16(r + 4) == 56 and c.u16(r + 6) == channel)
            offset, length = base + c.u32(payload + 12), c.u32(payload + 8)
            streams.append(dict(channel=channel, type=descriptor[0], stride=descriptor[1],
                declaration=xbe.read(0x43f408 + descriptor[0] * 21, 21).hex(),
                file=str(resource.relative_to(ROOT)), file_offset=(pointer & 0x3fffffff) + offset,
                bytes=length, sha256=hashlib.sha256(raw[offset:offset + length]).hexdigest(),
                sample_hex=raw[offset:offset + min(length, 24)].hex()))
        sections.append(dict(section=ordinal, classification=c.u16(at), vertices=c.u16(at + 4), streams=streams))
    return sections


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output", type=Path, default=ROOT / "analysis/multiplayer-shading/world-vertex-declarations.json")
    a = p.parse_args()
    xbe = Xbe(ROOT / "assets/disc/default.xbe")
    original = OriginalDeclaration(xbe)
    layouts, programs, all_links, streams, cache_hashes = {}, [], [], [], []
    dynamic = collections.defaultdict(set)
    for line in (ROOT / "tests/fixtures/retail_dynamic_model_shader_assets.tsv").read_text().splitlines():
        fields = line.split()
        if fields and fields[0] == "M":
            dynamic[fields[1]].update(int(x, 16) for x in fields[6:])
    scene = json.loads((ROOT / "analysis/multiplayer-shading/transparent-authored-inventory.json").read_text())
    selected = collections.defaultdict(set)
    for part in scene["parts"]:
        selected[part["map"]].add(part["shader"])
    for index, name in enumerate(MAPS):
        path = ROOT / ("assets/disc/maps" if index < 12 else "assets/map-pack/maps") / (name + ".map")
        c = Cache(path)
        cache_hashes.append(dict(map=name, sha256=hashlib.sha256(c.data).hexdigest()))
        for t in c.tags.values():
            if t["group"] != "vrtx":
                continue
            for ordinal, at in enumerate(c.block(t["address"] - c.magic + 4, 28)):
                n, addr = c.u32(at + 4), c.u32(at + 8)
                semantic_ids = tuple(struct.unpack_from("<" + "H" * n, c.data, addr - c.magic)) if n else ()
                if semantic_ids and semantic_ids not in layouts:
                    # Complete literal ordinary semantic coverage. Each stream
                    # comes directly from 1C8C0's immutable XBE table.
                    layouts[semantic_ids] = original.run(semantic_ids, (9, 27, 24, 48, 31, 47, 57))
                size = c.u32(at + 12)
                if not size:
                    continue
                code = c.data[c.u32(at + 16) - c.magic:c.u32(at + 16) - c.magic + size]
                read = reads(code)
                leaf = t["name"].split("\\")[-1]
                native = current_adapter(leaf)
                mismatches = [dict(register=reg, expected_semantic=semantic_ids[reg], native_semantic=native.get(reg))
                    for reg in read if reg < len(semantic_ids) and native.get(reg) != semantic_ids[reg]] if ordinal == 0 else []
                programs.append(dict(map=name, datum=t["datum"], name=t["name"], classification=ordinal,
                    semantics=semantic_ids, bytes=size, sha256=hashlib.sha256(code).hexdigest(),
                    read_counts=read, class0_adapter_differences=mismatches))
        for link in shader_links(c):
            link.update(map=name, dynamic_role_material=link["shader"] in dynamic[name],
                        bsp_or_selected_model_material=link["shader"] in selected[name])
            all_links.append(link)
        for t in c.tags.values():
            if t["group"] == "mode" and ("ze_train" in t["name"] or "scarab" in t["name"]):
                streams.append(dict(map=name, mode=t["datum"], name=t["name"], sections=raw_model_streams(c, t["datum"], xbe)))
        c.close()
    by_key = {(p["map"], p["datum"]): p for p in programs if p["classification"] == 0}
    differences = [dict(**link, program=by_key[(link["map"], link["vrtx"])]) for link in all_links
        if (link["map"], link["vrtx"]) in by_key and by_key[(link["map"], link["vrtx"])]["class0_adapter_differences"]]
    relevant = [d for d in differences if d["dynamic_role_material"] or d["bsp_or_selected_model_material"]]
    summary = dict(maps=len(MAPS), compiled_program_records=len(programs), class0_records=len(by_key),
        unique_class0_payloads=len({p["sha256"] for p in by_key.values()}),
        literal_shader_pass_references=len(all_links), original_declaration_cases=original.executed,
        original_declaration_words=original.executed * 64,
        differing_referenced_leaves=sorted({d["program"]["name"].split("\\")[-1] for d in differences}),
        dynamic_role_difference_references=sum(d["dynamic_role_material"] for d in differences),
        selected_asset_difference_references=len(relevant))
    report = dict(summary=summary, semantic_names=SEMANTICS, caches=cache_hashes,
        xbe_sha256=hashlib.sha256(xbe.data).hexdigest(),
        original_authority="1C330 semantic-to-register mapping; 1C8C0 table43F408 stride21; unchanged35790/35850 format and size routines",
        original_declaration_layouts=[dict(semantics=k, declaration_words=sum(v, [])) for k, v in layouts.items()],
        programs=programs, links=all_links, differences=differences, selected_differences=relevant,
        affected_model_streams=streams,
        scope="Literal all21 cache programs/classes and shad/stem/spas references. Class0 differences compare the CPU-decoded native generic adapter, not live source class selection. Dynamic roles are the ten original body/hands/held/FP fixtures per map; selected asset context is the existing BSP/selected model part census. Missing effect, particle and screen adapters are outside ordinary world admission. No source live runtime/registration/physical-hardware parity is inferred.",
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    a.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
