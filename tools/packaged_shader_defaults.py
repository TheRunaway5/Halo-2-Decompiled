"""Bounded Linux relocation of the existing renderer's default shader path.

Only one authenticated byte extent inside allocated read-only ELF data may
change. Instructions, file extent, shader bytes and runtime options remain
unchanged. The canonical native wrapper supplies the relative path's project
working directory; it does not use the runtime's capture-only shader override.
"""
import hashlib
import os
from pathlib import Path, PurePosixPath
import re
import struct

PACKAGE_RUNTIME = "bin/halo2_native"
PACKAGE_PAYLOAD = "bin/halo2_native.real"
PACKAGE_SHADERS = "bin/shaders"
PACKAGE_LOADER = "lib/ld-linux-x86-64.so.2"
PACKAGED_BINDING = "packaged-compiled-default"


def native_runtime_wrapper_body():
    return ('root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)\n'
            'cd "$root"\n'
            'exec "$root/lib/ld-linux-x86-64.so.2" --library-path "$root/lib" "$root/bin/halo2_native.real" "$@"\n')


def native_runtime_wrapper():
    return ("#!/bin/sh\nset -eu\n" + native_runtime_wrapper_body()).encode()


def relative_compiled_default(value):
    """Admit only the two padding forms the fixed-width packager produces."""
    if (not isinstance(value, str) or not re.fullmatch(r"(?:\./)*bin/{1,2}shaders", value) or
            PurePosixPath(value) != PurePosixPath(PACKAGE_SHADERS)):
        raise ValueError("Unsupported packaged compiled shader default")
    return value


def readonly_literal_location(payload, literal):
    """Find one exact byte extent in ELF64/x86-64 read-only section ownership.

    The compiler can construct filesystem::path using a known-length string;
    the actual retained renderer constant has no following NUL. The validated
    executable/source consumer establishes this field's meaning and length.
    Bytes after the field are neither read as a terminator nor rewritten.
    """
    needle = literal
    if not literal or b"\0" in literal or len(payload) < 64:
        raise ValueError("Invalid compiled shader literal")
    header = struct.unpack_from("<16sHHIQQQIHHHHHH", payload)
    if (header[0][:7] != b"\x7fELF\x02\x01\x01" or header[1] not in (2, 3) or
            header[2] != 62 or header[3] != 1 or header[8] != 64 or
            header[11] != 64 or not header[12] or header[13] >= header[12]):
        raise ValueError("Packaged shader relocation requires ordinary ELF64 x86-64 section ownership")
    table, count = header[6], header[12]
    if table < 64 or table + count * 64 > len(payload):
        raise ValueError("Truncated ELF section directory")
    sections = [struct.unpack_from("<IIQQQQIIQQ", payload, table + i * 64) for i in range(count)]
    strings = sections[header[13]]
    if strings[1] != 3 or strings[4] + strings[5] > len(payload):
        raise ValueError("Invalid ELF section-name bank")
    names = payload[strings[4]:strings[4] + strings[5]]
    at = payload.find(needle)
    if at < 0 or payload.find(needle, at + 1) >= 0:
        raise ValueError("Compiled shader path is not one exact unambiguous extent")
    # Reject a proper substring of a larger printable path. The retained
    # known-length constant has a zero before it and binary data after it;
    # ordinary terminated compiler strings also satisfy these boundaries.
    if ((at and payload[at-1] != 0) or
            (at + len(needle) < len(payload) and 0x20 <= payload[at + len(needle)] < 0x7f)):
        raise ValueError("Compiled shader extent overlaps a larger printable value")
    owners = []
    for index, row in enumerate(sections):
        if row[1] == 8:  # SHT_NOBITS owns no file bytes.
            continue
        if row[4] + row[5] > len(payload):
            raise ValueError("Truncated ELF section payload")
        if row[4] <= at and at + len(needle) <= row[4] + row[5]:
            if row[1] != 1 or not row[2] & 2 or row[2] & 5:
                raise ValueError("Compiled shader path is not allocated read-only data")
            if row[0] >= len(names):
                raise ValueError("Invalid ELF section name")
            end = names.find(b"\0", row[0])
            if end < 0:
                raise ValueError("Unterminated ELF section name")
            owners.append(dict(section_index=index, section=names[row[0]:end].decode("ascii"),
                               section_flags=row[2], offset=at, bytes=len(literal),
                               section_offset=row[4], section_bytes=row[5]))
    if len(owners) != 1:
        raise ValueError("Compiled shader path has no unique read-only section owner")
    return owners[0]


def relocate_compiled_shader_default(payload, original_directory, *, target_directory=None):
    """Return a staged postimage; preserve instructions and the path field width.

    Packages use the fixed relative layout. A local launcher may instead bind
    a canonical owned directory without replacing another build's shader bank.
    """
    directory = Path(original_directory)
    if not directory.is_absolute() or directory.resolve(strict=True) != directory:
        raise ValueError("Select the actual canonical compiled default shader directory")
    original = os.fsencode(directory)
    location = readonly_literal_location(payload, original)
    if target_directory is not None:
        target = Path(target_directory)
        if (not target.is_absolute() or target.is_symlink() or not target.is_dir() or
                target.resolve(strict=True) != target):
            raise ValueError("Local shader relocation requires its canonical directory owner")
        relative = os.fsencode(target)
        padding = len(original) - len(relative)
        if padding < 0:
            raise ValueError("Compiled shader path is too short to relocate")
        # filesystem::path consumes this known-length field. Trailing directory
        # separators preserve its width and resolve to the same canonical owner.
        relative += b"/" * padding
        if Path(os.fsdecode(relative)).resolve(strict=True) != target:
            raise ValueError("Local shader relocation changed its directory owner")
        output_directory = str(target)
    else:
        relative = PACKAGE_SHADERS.encode()
        padding = len(original) - len(relative)
        if padding % 2:
            relative = b"bin//shaders"
            padding = len(original) - len(relative)
        if padding < 0:
            raise ValueError("Compiled shader path is too short to relocate")
        relative = b"./" * (padding // 2) + relative
        relative_compiled_default(os.fsdecode(relative))
        output_directory = PACKAGE_SHADERS
    if len(relative) != len(original):
        raise ValueError("Packaged shader default extent changed")
    start = location["offset"]
    postimage = payload[:start] + relative + payload[start + len(original):]
    assert len(postimage) == len(payload)
    readonly_literal_location(postimage, relative)
    record = dict(location, original_directory=str(directory), compiled_default=os.fsdecode(relative),
                  directory=output_directory, input_sha256=hashlib.sha256(payload).hexdigest(),
                  output_sha256=hashlib.sha256(postimage).hexdigest(),
                  changed_bytes=sum(a != b for a, b in zip(original, relative)),
                  only_declared_readonly_literal_changed=True)
    return postimage, record


def shader_inventory(directory):
    """The complete existing bank includes nested and non-SPIR-V companion files."""
    directory = Path(directory)
    if directory.is_symlink() or directory.resolve(strict=True) != directory:
        raise ValueError("Shader bank requires its canonical directory owner")
    result = {}
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise ValueError("Shader inventory cannot contain symlink aliases")
        if path.is_file():
            result[path.relative_to(directory).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    if len(result) != 97 or not {"world.vert.spv", "world.frag.spv", "hud.frag.spv"} <= result.keys():
        raise ValueError("Expected the existing complete 97-file shader bank")
    return result
