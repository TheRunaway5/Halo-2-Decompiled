#!/usr/bin/env python3
"""Audit extracted user-owned DLC and retail-signed XBEs without executing them.

RSA key, length-prefixed SHA-1 and padding authority:
https://github.com/XboxDev/xbedump/blob/b8cd5cd0f8b1cbc4e64f41daf925a873d34dacf1/xboxlib.c
The pinned source is read as data; its code is not imported or executed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from disc import FormatError, bounds, inspect_xbe

CRYPTO_PIN = "3eb176d28aac3688ebc1356ad2be0e1beac8de5e9f14fb1cbd887acc4ba6cac2"
CRYPTO_URL = ("https://github.com/XboxDev/xbedump/blob/"
              "b8cd5cd0f8b1cbc4e64f41daf925a873d34dacf1/xboxlib.c")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def xbox_hash(data: bytes) -> bytes:
    return hashlib.sha1(struct.pack("<I", len(data)) + data).digest()


def crypto_parameters(path: Path) -> tuple[int, int, list[bytes]]:
    data = path.read_bytes()
    if digest(data) != CRYPTO_PIN:
        raise FormatError("cryptographic reference differs from pinned source")
    source = re.sub(r"//[^\n]*|/\*.*?\*/", "", data.decode(), flags=re.S)
    match = re.search(r"unsigned char xboxPublicKeyData\[\]\s*=\s*\{(.*?)\};",
                      source, re.S)
    if match is None:
        raise FormatError("missing original public-key declaration")
    key = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]+)", match[1]))
    if len(key) != 276 or key[:4] != b"RSA1":
        raise FormatError("unexpected original public-key layout")
    modulus = int.from_bytes(key[20:], "little")
    exponent = int.from_bytes(key[16:20], "little")
    table = re.search(r"RSApkcs1paddingtable\[3\]\[16\]\s*=\s*\{(.*?)\};",
                      source, re.S)
    if table is None:
        raise FormatError("missing original digest-padding table")
    padding = []
    for row in re.findall(r"\{([^{}]*)\}", table[1]):
        values = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]+)", row))
        if len(values) != 16:
            raise FormatError("unexpected original padding row")
        padding.append(values[1:1 + values[0]])
    if len(padding) != 3:
        raise FormatError("unexpected original padding table")
    return modulus, exponent, padding


def signature_valid(data: bytes, parameters: tuple[int, int, list[bytes]]) -> bool:
    bounds(0, 0x178, len(data))
    header_size = struct.unpack_from("<I", data, 0x108)[0]
    if header_size < 0x104:
        raise FormatError("header smaller than signed region")
    bounds(0, header_size, len(data))
    modulus, exponent, padding = parameters
    value = int.from_bytes(data[4:0x104], "little")
    if value >= modulus:
        return False
    decoded = pow(value, exponent, modulus).to_bytes(256, "little")
    expected = xbox_hash(data[0x104:header_size])[::-1]
    return any(decoded == expected + tail + b"\0" +
               b"\xff" * (233 - len(tail)) + b"\x01\0" for tail in padding)


def audit_executable(path: Path, parameters: tuple[int, int, list[bytes]]) -> dict:
    data = path.read_bytes()
    result = inspect_xbe(path)
    base = result["base_address"]
    certificate = struct.unpack_from("<I", data, 0x118)[0] - base
    bounds(certificate, 0xB0, len(data))
    result["certificate_version_ac"] = f"{struct.unpack_from('<I', data, certificate + 0xAC)[0]:08x}"
    result["retail_header_signature_valid"] = signature_valid(data, parameters)
    table = struct.unpack_from("<I", data, 0x120)[0] - base
    for index, section in enumerate(result["sections"]):
        stored = data[table + index * 56 + 36:table + index * 56 + 56]
        raw = section["file_offset"]
        content = data[raw:raw + section["file_size"]]
        section["stored_xbox_sha1"] = stored.hex()
        section["calculated_xbox_sha1"] = xbox_hash(content).hex()
        section["xbox_sha1_valid"] = stored == xbox_hash(content)
    result["all_section_hashes_valid"] = all(s["xbox_sha1_valid"] for s in result["sections"])
    result["retail_authenticated"] = (result["retail_header_signature_valid"] and
                                      result["all_section_hashes_valid"])
    # Negative controls use disposable bytes and leave the reference untouched.
    corrupted_header = bytearray(data)
    corrupted_header[certificate + 0xAC] ^= 1
    result["changed_certificate_rejected"] = not signature_valid(corrupted_header, parameters)
    section = next(s for s in result["sections"] if s["file_size"] > 0)
    payload = bytearray(data[section["file_offset"]:section["file_offset"] + section["file_size"]])
    payload[0] ^= 1
    result["changed_section_rejected"] = xbox_hash(payload).hex() != section["stored_xbox_sha1"]
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True,
                        help="External extracted directory containing import-manifest.json")
    parser.add_argument("--crypto-source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    manifest = json.loads((root / "import-manifest.json").read_text())
    source = Path(manifest["source"])
    with source.open("rb") as stream:
        archive_pin = hashlib.file_digest(stream, "sha256").hexdigest()
    if archive_pin != manifest["source_sha256"]:
        raise FormatError("archive differs from import manifest")
    files = []
    seen = set()
    for entry in manifest["files"]:
        path = (root / entry["path"]).resolve()
        if not path.is_relative_to(root) or path in seen:
            raise FormatError("unsafe or repeated reference path")
        seen.add(path)
        if path.stat().st_size != entry["size"] or digest(path.read_bytes()) != entry["sha256"]:
            raise FormatError("reference differs from archive member: " + entry["path"])
        files.append(path)
    parameters = crypto_parameters(args.crypto_source)
    executables = []
    for path in files:
        if path.suffix.lower() == ".xbe":
            record = audit_executable(path, parameters)
            record["path"] = str(path.relative_to(root))
            executables.append(record)
    required = next(r for r in executables if r["path"].endswith("content/u5English/default.xbe"))
    if required["title_id"] != "4D530064" or not required["retail_authenticated"]:
        raise FormatError("English update 5 is not a retail-authenticated Halo 2 executable")
    if any(not r["changed_certificate_rejected"] or not r["changed_section_rejected"]
           for r in executables):
        raise FormatError("negative control was accepted")
    result = dict(source=str(source), source_sha256=archive_pin, storage_root=str(root),
                  verified_member_count=len(files), verified_bytes=sum(p.stat().st_size for p in files),
                  crypto_authority_url=CRYPTO_URL, crypto_authority_sha256=CRYPTO_PIN,
                  executables=executables, gameplay_version_display_verified=False,
                  limits=["Header and section authentication does not establish native gameplay parity.",
                          "The numerical title-screen update label must be verified from the original display routine.",
                          "No attached executable, installer or annotation script was executed."])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(f"Verified {len(files)} archive members; audited {len(executables)} XBEs.")
    print("English update 5: retail signature and every section digest valid.")
    print("Retail-authenticated XBEs:", sum(r["retail_authenticated"] for r in executables))
    print("Changed certificate and section negative controls rejected for every XBE.")
    print(args.output)


if __name__ == "__main__":
    main()
