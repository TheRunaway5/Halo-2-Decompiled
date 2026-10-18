"""Package the retained port material library, independently of game installations.

Only shader definitions, their relocations/identity bindings and scalar function
tables are admitted. Bitmap references are names; bitmap/gameplay payloads are
never included. Creation consumes an already preserved library, not a map/XBE.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import zlib

from audit_rendering import FormatError

SCHEMA = "H2_NATIVE_MATERIAL_BUNDLE_V1"
BUNDLE = "tools/data/native-materials.json.zlib"
TABLE_SHA = "1261274f49fbf8f8fd36e9a56f9b7a701a2bad1c5d84be168c69826a62484118"
GROUPS = {"shad", "stem", "spas", "vrtx"}
LIMIT = 192 * 1024 * 1024
CATALOG_SCHEMA = "H2_PRESERVED_NATIVE_SHADER_CATALOG_V1"
CLOSURE_POLICY = Path(__file__).resolve().parent / "data/mcc-native-library-closure.json"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode() + b"\n"


def canonical_digest(value):
    # Match the production helper's nlohmann::json::dump(), without a newline.
    return digest(json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode())


def closure_policy():
    # This is a shipped, generator-checked mirror; setup never opens reference/.
    policy = json.loads(CLOSURE_POLICY.read_bytes())
    if (policy.get("schema") != "H2_MCC_RETAINED_NATIVE_LIBRARY_CLOSURE_V1" or
            policy.get("source_map") != "01b_spacestation" or
            not fingerprint(policy.get("source_canonical_sha256"))):
        raise FormatError("Invalid retained native closure policy")
    return policy


def closure_dependency(library):
    policy = closure_policy()
    row = next((row for row in policy["libraries"] if row["map"] == library.get("map")), None)
    if row and row["missing"] and canonical_digest(library) == row["canonical_sha256"]:
        return policy["source_map"]
    return None


def require_closure_source(library):
    policy = closure_policy()
    if library.get("map") != policy["source_map"] or canonical_digest(library) != policy["source_canonical_sha256"]:
        raise FormatError("Canonical retained Cairo native library is unavailable or customized; existing programs were retained")


def selected_libraries(libraries, maps=None):
    selected = set(libraries) if maps is None else set(maps)
    for name in selected:
        identity(name)
    if selected - set(libraries):
        raise FormatError("Selected retained native library is unavailable")
    needed = {dependency for name in selected if (dependency := closure_dependency(libraries[name]))}
    for name in needed:
        if name not in libraries:
            raise FormatError("Canonical retained Cairo native library is missing; update this port installation")
        require_closure_source(libraries[name])
    return selected | needed


def identity(name):
    if not isinstance(name, str) or not name or name in (".", "..") or any(
            c not in "abcdefghijklmnopqrstuvwxyz0123456789_" for c in name):
        raise FormatError("Invalid bundled shader map identity")


def fingerprint(name):
    return isinstance(name, str) and len(name) == 64 and all(c in "0123456789abcdef" for c in name)


def validate_library(name, library, payloads):
    identity(name)
    if library.get("schema") != "H2_PRESERVED_NATIVE_SHADERS_V1" or library.get("map") != name:
        raise FormatError("Bundled shader map identity differs")
    records = library["shader_tags"]
    if not isinstance(records, list) or len(records) > 65536:
        raise FormatError("Excessive bundled shader directory")
    for record in records:
        if record["group"] not in GROUPS:
            raise FormatError("Non-shader payload in native material bundle")
    # This is the isolated rasterizer sub-block, never the gameplay globals tag.
    if library["rasterizer"]["group"] != "matg" or library["rasterizer"]["bytes"] != 464:
        raise FormatError("Invalid native rasterizer constants")
    used = set()
    for record in [*records, library["rasterizer"]]:
        filename = record["payload"]
        if not isinstance(filename, str) or not filename.endswith(".bin") or not fingerprint(filename[:-4]):
            raise FormatError("Invalid bundled shader payload identity")
        sha = filename[:-4]
        data = payloads[sha]
        if len(data) != record["bytes"] or len(data) > 16 * 1024 * 1024 or digest(data) != sha:
            raise FormatError("Bundled shader payload fingerprint differs")
        locations = set()
        for field, offset in record["pointers"]:
            if not 0 <= offset < len(data) or field in locations or not 0 <= field <= len(data) - 4:
                raise FormatError("Invalid bundled shader pointer")
            locations.add(field)
        for field, ref in record["references"]:
            if len(ref) != 2 or ref[0] not in GROUPS | {"bitm"} or not isinstance(ref[1], str):
                raise FormatError("Non-rendering reference in native material bundle")
            if field in locations or not 0 <= field <= len(data) - 4:
                raise FormatError("Invalid bundled shader reference")
            locations.add(field)
        for field, value in record["strings"]:
            if not isinstance(value, str) or len(value.encode()) > 255 or field in locations or not 0 <= field <= len(data) - 4:
                raise FormatError("Invalid bundled shader StringID")
            locations.add(field)
        used.add(sha)
    return used


def create(source, target):
    source, target = Path(source), Path(target)
    catalog = json.loads((source / "catalog.json").read_bytes())
    if catalog["schema"] != "H2_PRESERVED_NATIVE_SHADER_CATALOG_V1":
        raise FormatError("Invalid preserved shader catalog")
    maps, payloads, tables = {}, {}, None
    for row in catalog["maps"]:
        name = row["map"]
        identity(name)
        if row["complete"] is not True or name in maps:
            raise FormatError("Incomplete/duplicate preserved shader library")
        directory = source / name
        library = json.loads((directory / "library.json").read_bytes())
        for record in [*library["shader_tags"], library["rasterizer"]]:
            filename = record["payload"]
            if not filename.endswith(".bin") or not fingerprint(filename[:-4]):
                raise FormatError("Invalid preserved shader payload identity")
            payloads.setdefault(filename[:-4], (directory / "payloads" / filename).read_bytes())
        validate_library(name, library, payloads)
        current = (directory / "function-tables.bin").read_bytes()
        if len(current) != 28 * 1024 or digest(current) != TABLE_SHA or (tables is not None and current != tables):
            raise FormatError("Preserved shader function tables differ")
        tables = current
        maps[name] = library
    if not maps:
        raise FormatError("Empty preserved shader library")
    result = dict(schema=SCHEMA, maps=maps,
                  payloads={sha: base64.b64encode(data).decode() for sha, data in payloads.items()},
                  function_tables=base64.b64encode(tables).decode())
    raw = encoded(result)
    if len(raw) > LIMIT:
        raise FormatError("Native material bundle exceeds capacity")
    packed = zlib.compress(raw, 9)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(packed)
    report = dict(schema=SCHEMA, sha256=digest(packed), bytes=len(packed), decoded_bytes=len(raw),
                  maps=sorted(maps), unique_payloads=len(payloads), function_tables_sha256=TABLE_SHA,
                  policy="Retained native shader metadata/programs only; MCC supplies all referenced bitmap and gameplay assets")
    target.with_suffix("").write_bytes(encoded(report))
    return report


def load_bundled(root):
    path = Path(root) / BUNDLE
    if not path.is_file():
        return None
    report = json.loads(path.with_suffix("").read_bytes())
    if report["schema"] != SCHEMA or report["bytes"] > LIMIT or report["decoded_bytes"] > LIMIT:
        raise FormatError("Invalid native material bundle extent")
    if path.stat().st_size != report["bytes"]:
        raise FormatError("Native material bundle extent differs")
    packed = path.read_bytes()
    if digest(packed) != report["sha256"]:
        raise FormatError("Native material bundle fingerprint differs")
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, LIMIT + 1)
    if len(raw) != report["decoded_bytes"] or decoder.unconsumed_tail or decoder.unused_data or not decoder.eof:
        raise FormatError("Invalid native material bundle compression")
    bundle = json.loads(raw)
    if bundle["schema"] != SCHEMA or sorted(bundle["maps"]) != report["maps"]:
        raise FormatError("Native material bundle catalog differs")
    payloads = {}
    for sha, value in bundle["payloads"].items():
        if not fingerprint(sha):
            raise FormatError("Invalid bundled shader payload identity")
        data = base64.b64decode(value, validate=True)
        if digest(data) != sha:
            raise FormatError("Bundled shader payload fingerprint differs")
        payloads[sha] = data
    tables = base64.b64decode(bundle["function_tables"], validate=True)
    if len(tables) != 28 * 1024 or digest(tables) != TABLE_SHA:
        raise FormatError("Bundled shader function tables differ")
    # Validate the entire bundle before creating output files.
    used = {name: validate_library(name, library, payloads) for name, library in bundle["maps"].items()}
    if set().union(*used.values()) != set(payloads):
        raise FormatError("Unreferenced payload in native material bundle")
    return report, bundle["maps"], payloads, tables, used


def install_bundled_library(destination, name, package):
    report, libraries, payloads, tables, used = package
    library = libraries[name]
    directory = Path(destination) / name
    directory.mkdir()
    (directory / "payloads").mkdir()
    for sha in used[name]:
        target = directory / "payloads" / (sha + ".bin")
        target.write_bytes(payloads[sha])
        if digest(target.read_bytes()) != sha:
            raise FormatError("Native shader installation readback differs")
    (directory / "function-tables.bin").write_bytes(tables)
    manifest = encoded(library)
    (directory / "library.json").write_bytes(manifest)
    return digest(manifest), dict(map=name, complete=True, tags=len(library["shader_tags"]))


def copy_bundled(root, output, *, maps=None):
    package = load_bundled(root)
    if package is None:
        return dict(complete=False, blocker="Bundled native material library is missing; update this port installation")
    report, libraries, payloads, tables, used = package
    selected = selected_libraries(libraries, maps)
    destination = Path(output) / "native-shaders"
    destination.mkdir()
    (destination / "function-tables.bin").write_bytes(tables)
    copied, rows = {}, []
    for name in libraries:
        if name not in selected:
            continue
        copied[name], row = install_bundled_library(destination, name, package)
        rows.append(row)
    (destination / "catalog.json").write_bytes(encoded(dict(
        schema="H2_PRESERVED_NATIVE_SHADER_CATALOG_V1", maps=rows, bundle_sha256=report["sha256"])))
    return dict(complete=True, source="bundled native material library", bundle_sha256=report["sha256"], maps=copied,
                policy=report["policy"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Already preserved native shader catalog directory")
    parser.add_argument("output", type=Path)
    arguments = parser.parse_args()
    print(json.dumps(create(arguments.source, arguments.output), indent=2))
