"""Synthetic ELF data-owner fixture; never a game or execution/readiness proof."""
import struct


def elf_with_readonly_literal(literal, *, flags=2, extra=b"", terminated=True):
    data = literal + (b"\0" if terminated else b"\x02\0\0\0") + extra
    names = b"\0.text\0.rodata\0.shstrtab\0"
    strings = (0x100 + len(data) + 15) & ~15
    table = (strings + len(names) + 15) & ~15
    payload = bytearray(table + 4 * 64)
    ident = b"\x7fELF\x02\x01\x01" + b"\0" * 9
    struct.pack_into("<16sHHIQQQIHHHHHH", payload, 0, ident, 3, 62, 1, 0, 0, table, 0, 64, 0, 0, 64, 4, 3)
    payload[0x80:0x82] = b"\x90\xc3"
    payload[0x100:0x100+len(data)] = data
    payload[strings:strings+len(names)] = names
    for index, row in enumerate(((0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
                                  (1, 1, 6, 0x1080, 0x80, 2, 0, 0, 16, 0),
                                  (7, 1, flags, 0x1100, 0x100, len(data), 0, 0, 16, 0),
                                  (15, 3, 0, 0, strings, len(names), 0, 0, 1, 0))):
        struct.pack_into("<IIQQQQIIQQ", payload, table+index*64, *row)
    return bytes(payload)
