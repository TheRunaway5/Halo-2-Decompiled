"""The Windows runtime resolves shaders through SDL_GetBasePath(), beside its PE.

This checks layout, not behavior or qualification. Only the exact executable hash
and a passed native menu receipt can authorize a frontend capability.
"""
import struct

BINDING = 'windows-executable-adjacent'


def readonly_shader_literal(data):
    if len(data) < 64 or data[:2] != b'MZ':
        raise ValueError('Windows shader binding requires a PE32+ executable')
    pe = struct.unpack_from('<I', data, 60)[0]
    if pe < 64 or pe + 26 > len(data) or data[pe:pe+4] != b'PE\0\0':
        raise ValueError('Invalid Windows PE header')
    machine, count = struct.unpack_from('<HH', data, pe+4)
    optional = struct.unpack_from('<H', data, pe+20)[0]
    sections = pe + 24 + optional
    if (machine != 0x8664 or not 1 <= count <= 96 or optional < 112 or
            sections + count*40 > len(data) or
            struct.unpack_from('<H', data, pe+24)[0] != 0x20b):
        raise ValueError('Windows shader binding requires bounded x64 PE32+ sections')
    for index in range(count):
        at = sections + index*40
        size, start = struct.unpack_from('<II', data, at+16)
        flags = struct.unpack_from('<I', data, at+36)[0]
        if start + size > len(data):
            raise ValueError('Windows PE section escaped its file owner')
        # Initialized, readable data; neither writable nor executable.
        if flags & 0xe0000040 != 0x40000040:
            continue
        found = data.find(b'shaders\0', start, start+size)
        while found >= 0:
            if found == start or data[found-1] == 0:
                return found
            found = data.find(b'shaders\0', found+1, start+size)
    raise ValueError('Windows executable lacks its read-only adjacent shader literal')
