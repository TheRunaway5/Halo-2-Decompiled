#!/usr/bin/env python3
"""Export the unchanged original Vista PE7F4BB8 loading ambience bank.

This is a separate export step. It does not replace the Xbox scene/backdrop or
alter shared frontend exports. Different original executables need their own
verified byte location and hash; an unknown executable is never guessed.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
BANK_SHA = "4f61e827be9b54b71e3d37a145c9b88fb32eaf517e23746af85236ca41ba8fc9"
BANK_VA, BANK_BYTES = 0x7F4BB8, 18396


def digest(data):
    return hashlib.sha256(data).hexdigest()


def extract(data):
    if digest(data) != EXE_SHA:
        raise ValueError("Unverified original Vista executable SHA-256")
    if data[:2] != b"MZ":
        raise ValueError("Original DOS signature missing")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Original PE signature missing")
    sections = struct.unpack_from("<H", data, pe + 6)[0]
    optional_bytes = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError("Original PE32 optional header missing")
    base = struct.unpack_from("<I", data, optional + 28)[0]
    rva = BANK_VA - base
    for index in range(sections):
        section = optional + optional_bytes + index * 40
        _, va, raw_bytes, raw = struct.unpack_from("<4I", data, section + 8)
        offset = rva - va
        if 0 <= offset and offset + BANK_BYTES <= raw_bytes:
            bank = data[raw + offset:raw + offset + BANK_BYTES]
            if len(bank) != BANK_BYTES or digest(bank) != BANK_SHA:
                raise ValueError("Original loading ambience bank differs")
            return bank, raw + offset
    raise ValueError("Original loading ambience VA has no complete file-backed bank")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / "Halo 2 Vista installer/halo2.exe")
    parser.add_argument("--output", type=Path,
                        default=ROOT / "assets/menu-ui/vista/loading/loading-tone.xbox-adpcm")
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    bank, offset = extract(args.source.read_bytes())
    if args.output.exists() and args.output.read_bytes() != bank:
        raise ValueError("Refusing to overwrite a changed loading ambience bank")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(bank)
    if args.output.read_bytes() != bank:
        raise ValueError("Export readback differs")
    manifest = dict(source=str(args.source.resolve()), source_sha256=EXE_SHA,
                    source_virtual_address=f"{BANK_VA:08X}", source_file_offset=offset,
                    output=str(args.output.resolve()), bytes=len(bank), sha256=BANK_SHA,
                    tool_sha256=digest(Path(__file__).read_bytes()),
                    format="Original mono 36-byte Xbox ADPCM blocks; original49A688 emits 65408 S16LE bytes")
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
