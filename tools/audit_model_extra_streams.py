#!/usr/bin/env python3
"""Freeze all literal extra MODEL streams across the 21 Xbox caches.

Expected format conversion follows the pinned GPU declaration reference:
NORMPACKED3 signed11/11/10 divided by1023/1023/511 without clamp, PBYTE3
bytes divided by255. No live constructor, model class selection or physical
Xbox vertex-output claim is made.
"""
from pathlib import Path
import collections
import hashlib
import json
import struct

from audit_dynamic_model_shader_assets import MAPS
from audit_rendering import Cache
from audit_world_vertex_declarations import ROOT, OriginalDeclaration, raw_model_streams
from xbe import Xbe


def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def packed(word):
    out = []
    for shift, width in ((0, 11), (11, 11), (22, 10)):
        value = (word >> shift) & ((1 << width) - 1)
        if value & (1 << (width - 1)):
            value -= 1 << width
        out.append(bits(value / ((1 << (width - 1)) - 1)))
    return out


def original_format_upload(original, stream_records):
    """Run unchanged 1C330 and 3F7440 with literal declarations/strides.

    The device and buffer records are explicit valid relocated inputs. No
    allocator, draw or GPU-fetch service is replaced or executed here.
    """
    from unicorn.x86_const import UC_X86_REG_ESP
    semantic_ids = [0, 3, 4, 5, 6, 14, 19, 20]
    declaration = original.run(semantic_ids, [v["type"] for v in stream_records])
    u = original.u
    device, push, streams, buffers = 0x1007000, 0x1009000, 0x100a000, 0x100b000
    u.mem_write(device, bytes(0x1000))
    u.mem_write(push, bytes(0x1000))
    u.mem_write(device, struct.pack("<II", push, push + 0x1000))
    u.mem_write(0x407488, struct.pack("<I", device))
    u.mem_write(0x407490, bytes(4))
    u.mem_write(0x40852c, bytes(u.mem_read(0x1001000, 256)))
    for index, v in enumerate(stream_records):
        buffer = buffers + index * 16
        u.mem_write(buffer, struct.pack("<III", 1, 0x200000 + index * 0x10000, 0))
        u.mem_write(streams + index * 12, struct.pack("<III", buffer, v["stride"], 0))
    stack = 0x10fff00
    u.mem_write(stack, struct.pack("<IIII", 0x3000000, 0x408519, len(stream_records), streams))
    u.reg_write(UC_X86_REG_ESP, stack)
    u.emu_start(0x3f7440, 0x3000000, count=100000)
    command = list(struct.unpack("<34I", bytes(u.mem_read(push, 136))))
    assert command[0] == 0x401720 and command[17] == 0x401760
    expected = [stream_records[v[0]]["stride"] * 256 + v[2] for v in declaration]
    assert command[18:] == expected
    assert command[18 + semantic_ids.index(14)] == 0x334
    return dict(semantics=semantic_ids, stream_types=[v["type"] for v in stream_records],
                strides=[v["stride"] for v in stream_records], command=command,
                radiance_format_word=command[18 + semantic_ids.index(14)])


def main():
    xbe = Xbe(ROOT / "assets/disc/default.xbe")
    original = OriginalDeclaration(xbe)
    fixture = ROOT / "tests/fixtures/retail_model_extra_streams.tsv"
    rows = ["# Literal raw MODEL channel3/type48 and channel4/type47; pinned declaration conversion, not original GPU vertex exports."]
    counts, affected, controls = collections.Counter(), [], []
    uploads, radiance_components, examples = {}, collections.Counter(), []
    for index, name in enumerate(MAPS):
        c = Cache(ROOT / ("assets/disc/maps" if index < 12 else "assets/map-pack/maps") / (name + ".map"))
        for t in c.tags.values():
            if t["group"] != "mode":
                continue
            sections = raw_model_streams(c, t["datum"], xbe)
            extras = [s for s in sections if any(v["channel"] >= 3 for v in s["streams"])]
            if not extras:
                if c.u32(t["address"] - c.magic + 0x74):
                    controls.append(dict(map=name, mode=t["datum"], name=t["name"]))
                    rows.append(f"C {name} {t['datum']:08X}")
                continue
            models = [v for v in c.tags.values() if v["group"] == "hlmt" and
                      c.u32(v["address"] - c.magic + 4) == t["datum"]]
            assert len(models) == 1
            rows.append(f"H {name} {t['datum']:08X} {models[0]['datum']:08X}")
            rows.append(f"M {name} {t['datum']:08X} {len(extras)}")
            for sec in extras:
                extra = [v for v in sec["streams"] if v["channel"] >= 3]
                assert [(v["channel"], v["type"], v["stride"]) for v in extra] == [(3, 48, 4), (4, 47, 3)]
                key = tuple((v["type"], v["stride"]) for v in sec["streams"])
                if key not in uploads:
                    uploads[key] = original_format_upload(original, sec["streams"])
                payload = []
                for v in extra:
                    with (ROOT / v["file"]).open("rb") as f:
                        f.seek(v["file_offset"])
                        raw = f.read(v["bytes"])
                    assert hashlib.sha256(raw).hexdigest() == v["sha256"]
                    payload.append(raw)
                    counts[v["type"]] += 1
                rows.append(f"S {name} {t['datum']:08X} {sec['section']} {sec['vertices']}")
                for vertex in range(sec["vertices"]):
                    incident = struct.unpack_from("<I", payload[0], vertex * 4)[0]
                    radiance = list(payload[1][vertex * 3:vertex * 3 + 3])
                    radiance_components.update(radiance)
                    if any(0 < value < 255 for value in radiance) and len(examples) < 12:
                        examples.append(dict(map=name, mode=t["datum"], section=sec["section"],
                            vertex=vertex, raw_bytes=radiance, normalized_bits=[bits(v / 255) for v in radiance]))
                    expected = packed(incident) + [bits(v / 255) for v in radiance]
                    rows.append(f"V {name} {t['datum']:08X} {sec['section']} {vertex} " + " ".join(f"{v:08X}" for v in expected))
                per_section = collections.Counter(payload[1])
                affected.append(dict(map=name, mode=t["datum"], name=t["name"], section=sec["section"], vertices=sec["vertices"], streams=extra,
                    radiance_nonzero_components=sum(n for value, n in per_section.items() if value),
                    radiance_interior_components=sum(n for value, n in per_section.items() if 0 < value < 255)))
        c.close()
    fixture.write_text("\n".join(rows) + "\n")
    report = dict(maps=len(MAPS), sections=len(affected), vertices=sum(s["vertices"] for s in affected),
        model_instances=len({(s["map"], s["mode"]) for s in affected}), plain_prt_controls=controls,
        stream_counts=dict(counts), records=affected, fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        original_format_uploads=list(uploads.values()), original_format_upload_words=len(uploads) * 34,
        radiance_component_histogram=dict(sorted(radiance_components.items())), nonzero_radiance_examples=examples,
        fetch_reference=dict(format="NV097 component count 3/type 4 UB_OGL, literal upload low byte 0x34; stride3 command 0x334",
            gl="analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/pgraph/gl/vertex.c:102",
            vk="analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/pgraph/vk/vertex.c:87"),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), scope=__doc__)
    fixture.with_suffix(".provenance.json").write_text(json.dumps(report, indent=2) + "\n")
    (ROOT / "analysis/multiplayer-shading/model-extra-stream-census.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k not in ("records", "scope", "plain_prt_controls", "radiance_component_histogram", "nonzero_radiance_examples", "original_format_uploads")}, indent=2))


if __name__ == "__main__":
    main()
