"""Authentic shipped scalar constants for asset-free synthetic ownership tests.

Only function_tables is base64-decoded. No program payload is decoded, no
private MCC cache is consulted, and this module performs no reads on import.
"""
import base64
import hashlib
import json
from pathlib import Path
import zlib

TABLE_SHA = "1261274f49fbf8f8fd36e9a56f9b7a701a2bad1c5d84be168c69826a62484118"
LIMIT = 192 * 1024 * 1024  # Existing native_shader_bundle decoded bound.


def shipped_native_scalar_tables(root=None):
    root = Path(root) if root is not None else Path(__file__).resolve().parents[1]
    packed_path = root / "tools/data/native-materials.json.zlib"
    report_path = packed_path.with_suffix("")
    if packed_path.is_symlink() or report_path.is_symlink():
        raise ValueError("Shipped scalar fixture requires ordinary source-distributed files")
    report = json.loads(report_path.read_bytes())
    if (report.get("schema") != "H2_NATIVE_MATERIAL_BUNDLE_V1" or
            type(report.get("bytes")) is not int or not 0 <= report["bytes"] <= LIMIT or
            type(report.get("decoded_bytes")) is not int or not 0 <= report["decoded_bytes"] <= LIMIT or
            report.get("function_tables_sha256") != TABLE_SHA):
        raise ValueError("Invalid source-distributed scalar bundle manifest")
    if packed_path.stat().st_size != report["bytes"]:
        raise ValueError("Source-distributed scalar bundle extent differs")
    packed = packed_path.read_bytes()
    if hashlib.sha256(packed).hexdigest() != report["sha256"]:
        raise ValueError("Source-distributed scalar bundle fingerprint differs")
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, LIMIT + 1)
    if (len(raw) != report["decoded_bytes"] or decoder.unconsumed_tail or
            decoder.unused_data or not decoder.eof):
        raise ValueError("Invalid source-distributed scalar bundle compression")
    bundle = json.loads(raw)
    if bundle.get("schema") != "H2_NATIVE_MATERIAL_BUNDLE_V1" or sorted(bundle["maps"]) != report["maps"]:
        raise ValueError("Source-distributed scalar bundle catalog differs")
    tables = base64.b64decode(bundle["function_tables"], validate=True)
    if len(tables) != 28 * 1024 or hashlib.sha256(tables).hexdigest() != TABLE_SHA:
        raise ValueError("Source-distributed scalar constants differ")
    return tables
