#!/usr/bin/env python3
"""Inventory original Xbox rendering requirements without claiming visual parity.

Reads the user's local version-8 caches. No shader semantics are inferred from
filenames, and no PC/MCC shader definitions are substituted for Xbox programs.
See the pinned Assembly/Reclaimer format references in docs/RESEARCH.md.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct


class FormatError(ValueError):
    pass


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


class Cache:
    def __init__(self, path):
        self.path = Path(path)
        self.file = self.path.open("rb")
        self.data = None
        try:
            self.data = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
            self.require(0, 2048)
            if (self.u32(0), self.u32(4), self.u32(2044)) != (0x68656164, 8, 0x666F6F74):
                raise FormatError("expected original Xbox version-8 cache")
            if self.u32(8) != len(self.data) or self.string(288, 32) != "02.09.27.09809":
                raise FormatError("unsupported or truncated retail cache")
            index = self.u32(16)
            self.require(index, 32)
            header_magic = self.u32(index) - index - 32
            table = self.pointer(self.u32(index + 8), header_magic, 16)
            self.magic = self.u32(table + 8) - index - self.u32(20)
            count = self.u32(index + 24)
            if not 0 < count <= 65536:
                raise FormatError("invalid tag count")
            self.require(table, count * 16)
            names, size, offsets = self.u32(708), self.u32(712), self.u32(716)
            self.require(names, size)
            self.require(offsets, count * 4)
            self.tags = {}
            for i in range(count):
                entry = table + i * 16
                name = self.u32(offsets + i * 4)
                if name >= size:
                    raise FormatError("tag filename outside string table")
                datum = self.u32(entry + 4)
                self.tags[datum] = {
                    "datum": datum, "group": self.u32(entry).to_bytes(4, "big").decode("ascii"),
                    "name": self.string(names + name, size - name),
                    "address": self.u32(entry + 8), "size": self.u32(entry + 12),
                }
            self.scenario = self.u32(index + 12)
            self.passes, self.templates, self.vertex_shaders, self.shaders, self.bitmaps = {}, {}, {}, {}, {}
        except Exception:
            self.close()
            raise

    def close(self):
        if self.data is not None:
            self.data.close()
        self.file.close()

    def require(self, offset, size):
        if offset < 0 or size < 0 or offset > len(self.data) or size > len(self.data) - offset:
            raise FormatError(f"cache range outside allocation: {offset:#x}+{size:#x}")

    def u16(self, offset):
        self.require(offset, 2)
        return struct.unpack_from("<H", self.data, offset)[0]

    def u32(self, offset):
        self.require(offset, 4)
        return struct.unpack_from("<I", self.data, offset)[0]

    def string(self, offset, size):
        self.require(offset, size)
        end = self.data.find(b"\0", offset, offset + size)
        if end < 0:
            raise FormatError("unterminated cache string")
        return self.data[offset:end].decode("utf-8")

    def pointer(self, address, magic, size):
        offset = address - magic
        self.require(offset, size)
        return offset

    def block(self, offset, stride, magic=None, bounds=None):
        count = self.u32(offset)
        if count > 65536:
            raise FormatError("excessive block count")
        if not count:
            return []
        start = self.pointer(self.u32(offset + 4), self.magic if magic is None else magic, count * stride)
        if bounds and (start < bounds[0] or start + count * stride > bounds[1]):
            raise FormatError("BSP pointer escapes its allocation")
        return list(range(start, start + count * stride, stride))

    def tag(self, datum, group):
        if datum not in self.tags or self.tags[datum]["group"] != group or not self.tags[datum]["address"]:
            raise FormatError(f"missing {group} datum {datum:#x}")
        result = self.tags[datum]
        return result, self.pointer(result["address"], self.magic, result["size"])

    def blob(self, offset, size):
        self.require(offset, size)
        return {"size": size, "sha256": sha(self.data[offset:offset + size])}

    def packed_range(self, packed, records):
        first, count = packed & 511, packed >> 9
        if count and (first > len(records) or count > len(records) - first):
            raise FormatError("packed range exceeds referenced block")
        return records[first:first + count] if count else []

    def vertex_shader(self, datum):
        key = f"{datum:08x}"
        if key in self.vertex_shaders:
            return key
        tag, offset = self.tag(datum, "vrtx")
        records = []
        for entry in self.block(offset + 4, 28):
            record = {"classification": self.u32(entry), "payloads": {}}
            for name, field in (("compiled", 12), ("code", 20)):
                size = self.u32(entry + field)
                if size:
                    start = self.pointer(self.u32(entry + field + 4), self.magic, size)
                    record["payloads"][name] = self.blob(start, size)
            records.append(record)
        self.vertex_shaders[key] = {"name": tag["name"], "platform": self.u16(offset), "classifications": records}
        return key

    def shader_pass(self, datum):
        key = f"{datum:08x}"
        if key in self.passes:
            return key
        tag, offset = self.tag(datum, "spas")
        definitions = []
        for pp in self.block(offset + 28, 88):
            result = {"blocks": {}, "implementations": []}
            for name, field, stride in (("textures", 8, 4), ("render_states", 16, 5),
                                        ("texture_states", 24, 24), ("fragments", 32, 4),
                                        ("permutations", 40, 6), ("combiners", 48, 32),
                                        ("externs", 56, 4), ("constants", 64, 7), ("constant_info", 72, 7)):
                entries = self.block(pp + field, stride)
                result["blocks"][name] = [self.data[p:p + stride].hex() for p in entries]
            for entry in self.block(pp, 306):
                impl = {"original_bytes": self.data[entry:entry + 306].hex()}
                vs = self.u32(entry + 256)
                impl["vertex_shader"] = None if vs == 0xFFFFFFFF else self.vertex_shader(vs)
                for name, field in (("textures", 0), ("render_states", 2), ("texture_states", 4),
                                    ("fragments", 246), ("permutations", 248), ("combiners", 250)):
                    impl[name] = self.packed_range(self.u16(entry + field), result["blocks"][name])
                # Preserve unresolved header fields as bytes, rather than inventing
                # a modern shader from the pass name or assuming all variants match.
                result["implementations"].append(impl)
            definitions.append(result)
        self.passes[key] = {"name": tag["name"], "postprocess": definitions}
        return key

    def template(self, datum):
        key = f"{datum:08x}"
        if key in self.templates:
            return key
        tag, offset = self.tag(datum, "stem")
        definitions = []
        for pp in self.block(offset + 88, 40):
            remaps = [self.data[p:p + 4].hex() for p in self.block(pp + 32, 4)]
            implementations = []
            for entry in self.block(pp + 24, 6):
                implementations.append({name: self.packed_range(self.u16(entry + field), remaps)
                                        for name, field in (("bitmaps", 0), ("pixel_constants", 2), ("vertex_constants", 4))})
            passes = [{"pass": self.shader_pass(self.u32(p + 4)),
                       "implementations": self.packed_range(self.u16(p + 8), implementations)}
                      for p in self.block(pp + 16, 10)]
            layers = [self.u16(p) for p in self.block(pp + 8, 2)]
            for layer in layers:
                self.packed_range(layer, passes)
            lods = []
            for p in self.block(pp, 10):
                lods.append({"available_layers": self.u32(p + 2),
                             "layer_ranges": self.packed_range(self.u16(p), layers),
                             "projected_height_bytes": self.data[p + 6:p + 10].hex()})
            definitions.append({"lods": lods, "layers": layers, "passes": passes})
        self.templates[key] = {"name": tag["name"], "postprocess": definitions}
        return key

    def shader(self, datum):
        key = f"{datum:08x}"
        if key in self.shaders:
            return key
        tag, offset = self.tag(datum, "shad")
        records = []
        for pp in self.block(offset + 32, 124):
            bitmaps = [{"datum": self.u32(p), "bitmap_index": self.u32(p + 4)} for p in self.block(pp + 4, 12)]
            for bitmap in bitmaps:
                if bitmap["datum"] != 0xFFFFFFFF:
                    key_bitmap = self.bitmap(bitmap["datum"])
                    if bitmap["bitmap_index"] >= len(self.bitmaps[key_bitmap]["records"]):
                        raise FormatError("shader bitmap index exceeds bitmap records")
            # Values stay as raw words so signed zero, exact float precision and
            # packed colors survive the audit. They are not assigned guessed uses.
            constants = {name: [self.data[p:p + stride].hex() for p in self.block(pp + field, stride)]
                         for name, field, stride in (("pixel", 12, 4), ("vertex", 20, 16), ("overlays", 60, 20))}
            records.append({"template": self.template(self.u32(pp)), "bitmaps": bitmaps,
                            "constants": constants})
        runtime = [self.data[p:p + 80].hex() for p in self.block(offset + 12, 80)]
        self.shaders[key] = {"name": tag["name"], "flags": self.u16(offset + 22),
                             "runtime_properties": runtime, "postprocess": records,
                             "lighting_selectors": {"specular": self.u16(offset + 0x3E),
                                                    "lightmap": self.u16(offset + 0x40)},
                             "lighting_brightness_bytes": self.data[offset + 0x44:offset + 0x4C].hex(),
                             "native_retail_pass_execution": False}
        return key

    def bitmap(self, datum):
        key = f"{datum:08x}"
        if key in self.bitmaps:
            return key
        tag, offset = self.tag(datum, "bitm")
        records = []
        for p in self.block(offset + 68, 116):
            records.append({"width": self.u16(p + 4), "height": self.u16(p + 6), "depth": self.u16(p + 8),
                            "type": self.u16(p + 10), "format": self.u16(p + 12), "flags": self.u16(p + 14),
                            "declared_additional_mips": self.u16(p + 20),
                            "resource_pointers": [self.u32(p + 28 + i * 4) for i in range(3)],
                            "resource_sizes": [self.u32(p + 52 + i * 4) for i in range(3)]})
        self.bitmaps[key] = {"name": tag["name"], "records": records}
        return key

    def lighting(self, start, size, memory):
        magic, bounds = memory - start, (start, start + size)
        address = self.u32(start + 8)
        if not address:
            return {"groups": []}
        base = self.pointer(address, magic, 136)
        if base < start or base + 136 > start + size:
            raise FormatError("lightmap header escapes BSP")
        groups = []
        for pp in self.block(base + 128, 104, magic, bounds):
            palettes = [self.blob(p, 1024) for p in self.block(pp + 8, 1024, magic, bounds)]
            records = {"palettes": palettes, "bitmap_datum": self.u32(pp + 28)}
            bitmap_count = 0
            if records["bitmap_datum"] != 0xFFFFFFFF:
                bitmap_count = len(self.bitmaps[self.bitmap(records["bitmap_datum"])]["records"])
            for name, field in (("clusters", 40), ("instances", 72)):
                records[name] = [{"bitmap_index": struct.unpack_from("<h", self.data, p)[0],
                                  "palette_index": self.data[p + 2], "flags": self.data[p + 3]}
                                 for p in self.block(pp + field, 4, magic, bounds)]
                for record in records[name]:
                    if record["bitmap_index"] >= 0 and record["palette_index"] >= len(palettes):
                        raise FormatError("lightmap palette reference outside block")
                    if record["bitmap_index"] >= bitmap_count:
                        raise FormatError("lightmap bitmap reference outside records")
            groups.append(records)
        return {"groups": groups, "native_lightmap_sampling": "default group P8 textures; per-vertex and alternate groups pending",
                "native_dynamic_lighting": False, "native_shadow_passes": False}

    def audit(self):
        bsps = []
        if self.scenario != 0xFFFFFFFF:
            _, scenario = self.tag(self.scenario, "scnr")
            for entry in self.block(scenario + 528, 68):
                start, size, memory = self.u32(entry), self.u32(entry + 4), self.u32(entry + 8)
                self.require(start, size)
                materials = [self.shader(self.u32(p + 12))
                             for p in self.block(start + 16 + 164, 32, memory - start, (start, start + size))]
                bsps.append({"datum": self.u32(entry + 20), "materials": materials,
                             "lighting": self.lighting(start, size, memory)})
        # Include actor/weapon/sky/effect materials, not only world BSP materials.
        for datum, tag in self.tags.items():
            if tag["group"] == "shad" and tag["address"]:
                self.shader(datum)
        return {"map": self.path.name, "sha256": file_sha(self.path), "bsps": bsps,
                "shaders": self.shaders, "templates": self.templates, "passes": self.passes,
                "vertex_shaders": self.vertex_shaders, "bitmaps": self.bitmaps, "retail_visual_parity_verified": False,
                "scope": "original asset inventory; no original frame comparison or native retail shader execution"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--map", type=Path)
    group.add_argument("--maps", type=Path)
    parser.add_argument("--output", type=Path, default=Path("analysis/rendering-audit"))
    args = parser.parse_args()
    paths = sorted(args.maps.glob("*.map")) if args.maps else [args.map or Path("assets/disc/maps/lockout.map")]
    if not paths:
        parser.error("no cache files found")
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    for path in paths:
        cache = None
        try:
            cache = Cache(path)
            report = cache.audit()
            (args.output / f"{path.stem}.json").write_text(json.dumps(report, indent=2) + "\n")
            result = {"map": path.name, "bsps": len(report["bsps"]), "shaders": len(cache.shaders),
                      "passes": len(cache.passes), "vertex_shaders": len(cache.vertex_shaders), "inventory_valid": True}
            print(f"{path.name}: {result['bsps']} BSPs, {result['shaders']} shaders, {result['passes']} passes", flush=True)
        except (FormatError, OSError, UnicodeError, ValueError) as error:
            result = {"map": path.name, "inventory_valid": False, "error": str(error)}
            print(f"{path.name}: ERROR {error}", flush=True)
        finally:
            if cache:
                cache.close()
        results.append(result)
    root = Path(__file__).resolve().parent.parent
    shaders = {name: file_sha(root / "shaders" / name) for name in ("world.vert", "world.frag", "combiner.glsl")}
    summary = {"retail_visual_parity_verified": False, "native_shader_source_hashes": shaders,
               "implemented_runtime_features": ["authored mip chains and retail mip selection", "P8 lightmap and bump palettes",
                                                "template bitmap/constant decoding", "retail pixel combiner assembly",
                                                "supported lightmap/surface/forward programs", "cluster/instance lightmap bindings",
                                                "admitted authored vertex programs and UV transform ordering",
                                                "model tangent basis decoding and skinning", "fixed object change colors",
                                                "shader lighting selectors and brightness", "six-face cubemaps and resident mips",
                                                "single-visible-sky BSP model/parallax binding"],
               "missing_runtime_features": ["complete vertex-program execution and GPU arithmetic parity", "retail LOD/layer/pass selection and ordering",
                                            "remaining externs, switches and animated overlays", "per-vertex lightmaps, alternate groups and object light probes",
                                            "dynamic lights and shadows", "remaining material lighting/environment layers",
                                            "transparent sorting and complete alpha/depth/stencil state", "per-cluster multi-sky selection, fog and postprocess", "retail sampler states and output gamma",
                                            "matched original/native frames and GPU arithmetic validation"], "maps": results}
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print("Retail shader/lighting parity: NOT VERIFIED (native pass execution is incomplete)")
    return int(any(not r["inventory_valid"] for r in results))


if __name__ == "__main__":
    raise SystemExit(main())
