#!/usr/bin/env python3
"""Preserve existing Xbox shader definitions without retaining disc assets.

The result is an owned, relocatable shader library. Bitmap references retain
group/path identities for binding to converted assets. No texture, geometry or
gameplay tag payload is copied. This does not import MCC shader programs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import shutil
import xml.etree.ElementTree as ET

from audit_rendering import Cache, FormatError, file_sha

ROOT = Path(__file__).resolve().parents[1]
SCHEMAS = ROOT / "tools/reference/assembly/src/Assembly/Plugins"
GROUPS = {"shad", "stem", "spas", "vrtx"}


def copy_selected(root, output):
    """Carry an already owned shader library into an offline MCC import."""
    registry = Path(root) / "assets/native-shaders-source.tsv"
    if not registry.is_file():
        from native_shader_bundle import copy_bundled
        return copy_bundled(root, output)
    rows = registry.read_text().splitlines()
    if len(rows) != 2 or rows[0] != "H2_NATIVE_SHADER_SOURCE_V1" or not rows[1].startswith("native\t"):
        raise FormatError("Invalid selected native shader library")
    source = Path(rows[1].split("\t", 1)[1])
    source = (Path(root) / source if not source.is_absolute() else source).resolve()
    catalog = json.loads((source / "catalog.json").read_text())
    if catalog.get("schema") != "H2_PRESERVED_NATIVE_SHADER_CATALOG_V1":
        raise FormatError("Invalid selected native shader catalog")
    destination = Path(output) / "native-shaders"
    if destination.exists():
        raise FormatError("Native shader preservation destination already exists")
    destination.mkdir()
    copied = {}
    for row in catalog["maps"]:
        if not row["complete"]:
            continue
        name = row["map"]
        if Path(name).name != name or name in (".", ".."):
            raise FormatError("Invalid native shader map identity")
        library = source / name
        for path in library.rglob("*"):
            if path.is_symlink():
                raise FormatError("Symlink in selected native shader library")
        manifest = json.loads((library / "library.json").read_text())
        if manifest.get("map") != name or manifest.get("schema") != "H2_PRESERVED_NATIVE_SHADERS_V1":
            raise FormatError("Selected native shader map manifest differs")
        records = [*manifest["shader_tags"], manifest["rasterizer"]]
        for record in records:
            payload = record["payload"]
            if len(payload)!=68 or not payload.endswith(".bin") or any(c not in "0123456789abcdef" for c in payload[:64]):
                raise FormatError("Invalid selected shader payload identity")
            path = library / "payloads" / payload
            if file_sha(path) != payload[:64] or path.stat().st_size != record["bytes"]:
                raise FormatError("Selected native shader payload fingerprint differs")
        tables = (library / "function-tables.bin").read_bytes()
        if len(tables)!=28*1024 or fnv(tables)!="22708b4cc478ddb7":
            raise FormatError("Selected native shader function tables differ")
        (destination / "function-tables.bin").write_bytes(tables)
        shutil.copytree(library, destination / name)
        for path in library.rglob("*"):
            if path.is_file() and file_sha(path) != file_sha(destination / name / path.relative_to(library)):
                raise FormatError("Preserved native shader readback differs")
        copied[name] = file_sha(destination / name / "library.json")
    shutil.copyfile(source / "catalog.json", destination / "catalog.json")
    return dict(complete=all(row["complete"] for row in catalog["maps"]), source=str(source), maps=copied,
                policy="Existing native shader metadata; never MCC shader programs")


def fnv(data):
    result = 14695981039346656037
    for b in data:
        result = ((result ^ b) * 1099511628211) & 0xffffffffffffffff
    return f"{result:016x}"


def schema(group):
    path = SCHEMAS / "Halo2Xbox" / (group + ".xml")
    if not path.is_file():
        path = SCHEMAS / "Halo2" / (group + ".xml")
    return ET.parse(path).getroot(), file_sha(path)


class Library:
    def __init__(self, cache, output):
        self.cache, self.output = cache, Path(output)
        count, size, indices, data = struct.unpack_from("<4I", cache.data, 356)
        cache.require(indices, count * 4); cache.require(data, size)
        self.strings = {}
        for i in range(count):
            off = cache.u32(indices + i * 4)
            if off != 0xffffffff and off < size:
                self.strings[i] = cache.string(data + off, size - off)
        self.resident = [(t["address"], t["address"] + t["size"]) for t in cache.tags.values() if t["address"] and t["size"]]

    def record(self, tag, node=None, root_address=None, root_size=None):
        cache = self.cache
        if node is None:
            node, _ = schema(tag["group"])
        address = tag["address"] if root_address is None else root_address
        size = tag["size"] if root_size is None else root_size
        if size > 16 * 1024 * 1024:
            raise FormatError("Excessive preserved shader payload")

        def read(pointer, extent):
            if not any(low <= pointer <= pointer + extent <= high for low, high in self.resident):
                raise FormatError("Shader pointer escapes exact resident allocations")
            at = cache.pointer(pointer, cache.magic, extent)
            return cache.data[at:at + extent]

        payload = bytearray(read(address, size))
        pointers, references, strings, visited, copied = {}, {}, {}, set(), {}

        def word(at, value):
            struct.pack_into("<I", payload, at, value)

        def walk(layout, source, target, depth=0):
            if depth > 32 or len(visited) > 1000000:
                raise FormatError("Excessive preserved shader graph")
            key = (id(layout), source, target)
            if key in visited:
                return
            visited.add(key)
            for field in layout:
                if "offset" not in field.attrib:
                    continue
                off = int(field.attrib["offset"], 0)
                source_at, at = source + off, target + off
                kind = field.tag.lower()
                if kind in ("tagblock", "dataref"):
                    count, pointer = struct.unpack("<II", read(source_at, 8))
                    stride = int(field.attrib.get("elementSize", "1"), 0)
                    # Original 1BA00 addresses four-byte fragments; the old
                    # Assembly Xbox schema incorrectly describes three bytes.
                    if tag["group"] == "spas" and field.attrib.get("name") == "Ps Fragments":
                        stride = 4
                    extent = count * stride
                    if extent > 16 * 1024 * 1024 or (kind == "tagblock" and count > 65536):
                        raise FormatError("Excessive preserved shader bank")
                    if not count:
                        word(at + 4, 0)
                        continue
                    if address <= pointer <= pointer + extent <= address + size:
                        destination = pointer - address
                    elif (pointer, extent) in copied:
                        destination = copied[pointer, extent]
                    else:
                        destination = (len(payload) + 15) & ~15
                        payload.extend(bytes(destination - len(payload)))
                        payload.extend(read(pointer, extent))
                        copied[pointer, extent] = destination
                    pointers[at + 4] = destination
                    word(at + 4, destination)
                    if kind == "tagblock":
                        for i in range(count):
                            walk(field, pointer + i * stride, destination + i * stride, depth + 1)
                elif kind == "tagref":
                    handle_at = at if field.attrib.get("withGroup", "true").lower() == "false" else at + 4
                    handle = struct.unpack_from("<I", payload, handle_at)[0]
                    if handle == 0xffffffff:
                        continue
                    ref = cache.tags.get(handle)
                    if ref is None:
                        raise FormatError(f"Unresolved full shader reference {handle:08x}")
                    references[handle_at] = [ref["group"], ref["name"]]
                elif kind == "stringid":
                    value = struct.unpack_from("<I", payload, at)[0]
                    if value in (0, 0xffffffff):
                        continue
                    if value & 65535 not in self.strings:
                        raise FormatError("Unresolved preserved shader string ID")
                    strings[at] = self.strings[value & 65535]

        walk(node, address, 0)
        digest = hashlib.sha256(payload).hexdigest()
        path = self.output / "payloads" / (digest + ".bin")
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists():
            path.write_bytes(payload)
        if file_sha(path) != digest:
            raise FormatError("Preserved shader readback differs")
        return dict(group=tag["group"], name=tag["name"], bytes=len(payload), payload=digest + ".bin",
                    fnv64=fnv(payload), pointers=sorted(pointers.items()), references=sorted(references.items()),
                    strings=sorted(strings.items()))


def preserve(source, output, executable):
    output = Path(output)
    if output.exists():
        raise FormatError("Choose a new native shader library directory")
    output.mkdir(parents=True)
    cache = Cache(source)
    try:
        library = Library(cache, output)
        records = [library.record(t) for t in cache.tags.values() if t["group"] in GROUPS and t["address"] and t["size"]]
        globals_, base = cache.tag(cache.u32(cache.u32(16) + 16), "matg")
        node, _ = schema("matg")
        raster = next(f for f in node if f.attrib.get("name") == "Rasterizer Data")
        rows = cache.block(base + 0x108, int(raster.attrib["elementSize"], 0))
        if len(rows) != 1:
            raise FormatError("Unsupported original rasterizer globals count")
        raster_record = library.record(globals_, raster, cache.u32(base + 0x10c), int(raster.attrib["elementSize"], 0))
        exe = Path(executable)
        if file_sha(exe) != "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d":
            raise FormatError("Unsupported existing shader table executable")
        with exe.open("rb") as stream:
            stream.seek(0x464bc8); tables = stream.read(28 * 1024)
        if len(tables) != 28 * 1024 or fnv(tables) != "22708b4cc478ddb7":
            raise FormatError("Unsupported existing shader function tables")
        (output / "function-tables.bin").write_bytes(tables)
        result = dict(schema="H2_PRESERVED_NATIVE_SHADERS_V1", map=Path(source).stem,
                      source_sha256=file_sha(source), source_xbe_sha256=file_sha(exe),
                      schemas={g: schema(g)[1] for g in sorted(GROUPS | {"matg"})},
                      shader_tags=records, rasterizer=raster_record,
                      nonresident_shader_tags=[dict(group=t["group"],name=t["name"]) for t in cache.tags.values()
                                               if t["group"] in GROUPS and (not t["address"] or not t["size"])],
                      policy="Existing native shader definitions only; bind bitmap identities to converted game assets")
        (output / "library.json").write_text(json.dumps(result, indent=2) + "\n")
        return result
    finally:
        cache.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("map", type=Path); parser.add_argument("output", type=Path)
    parser.add_argument("--executable", type=Path, default=ROOT / "assets/disc/default.xbe")
    args = parser.parse_args()
    result = preserve(args.map, args.output, args.executable)
    print(f"Preserved {len(result['shader_tags'])} native shader definitions")
