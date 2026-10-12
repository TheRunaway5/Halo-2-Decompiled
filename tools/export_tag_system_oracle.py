#!/usr/bin/env python3
"""Run unchanged retail 122C10/122C70/122D00 on actual cache directories.

Only the tag-header/table/group-table pointers are relocated into the emulator.
Payload addresses, salts, group relationships and iterator states remain literal.
No function result is supplied by a hook. This proves the shared tag operations,
not a playthrough or platform-specific file I/O.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from oracle_multiplayer_hud_positions import Retail, PIN, STOP, ROOT
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'tests/fixtures/tag_system_retail.tsv')
    parser.add_argument('--report', type=Path, default=ROOT/'analysis/tags/retail-oracle.json')
    parser.add_argument('--maps', nargs='+', type=Path, default=[
        ROOT/'assets/disc/maps/mainmenu.map', ROOT/'assets/disc/maps/zanzibar.map',
        ROOT/'assets/disc/maps/01b_spacestation.map'])
    args = parser.parse_args()
    retail = Retail()
    def run(entry, registers):
        retail.prepare(registers=registers)
        retail.g.emu_start(entry, STOP, count=5000000)
        assert retail.g.reg_read(UC_X86_REG_EIP) == STOP
        return retail.g.reg_read(UC_X86_REG_EAX)

    output = []
    report = dict(executable_sha256=PIN, functions=['00122C10', '00122C70', '00122D00'], maps=[])
    for path in args.maps:
        data = path.read_bytes()
        word = lambda at: struct.unpack_from('<I', data, at)[0]
        index = word(16)
        magic = word(index)-index-32
        count = word(index+24)
        group_count = word(index+4)
        group_offset = word(index)-magic
        groups = [struct.unpack_from('<III', data, group_offset+i*12) for i in range(group_count)]
        table_offset = word(index+8)-magic
        table = data[table_offset:table_offset+count*16]
        # Reuse the original emulator allocation; no metadata dereference occurs.
        retail.g.mem_write(retail.table, table)
        group_pointer = retail.alloc(data[group_offset:group_offset+group_count*12])
        header = bytearray(data[index:index+32])
        struct.pack_into('<I', header, 0, group_pointer)
        struct.pack_into('<I', header, 8, retail.table)
        retail.w(0x547f00, retail.alloc(header))
        iterator = retail.alloc(bytes(20))
        output.append('M '+json.dumps(str(path.relative_to(ROOT))))
        group_map = {g[0]: g for g in groups}
        lookups = iterations = 0
        for ordinal, (group, _, _) in enumerate(groups):
            ptr = run(0x122d00, registers=[(UC_X86_REG_ECX, group)])
            assert ptr == group_pointer+ordinal*12
            actual = retail.read(ptr, '<III')
            output.append('G '+' '.join(f'{v:08x}' for v in actual))
        for group in [g[0] for g in groups]+[0xffffffff, 0x7a7a7a7a]:
            retail.g.mem_write(iterator, struct.pack('<IIIII', 0, 0, 0x13572468, 0, group))
            handles = []
            while True:
                handle = run(0x122c70, registers=[(UC_X86_REG_EDI, iterator)])
                if handle == 0xffffffff:
                    break
                handles.append(handle)
            state = retail.read(iterator+8, '<II')
            output.append('I '+f'{group:08x} {state[0]:08x} {state[1]} '+
                          ' '.join(f'{v:08x}' for v in handles))
            iterations += len(handles)+1
        for i in range(count):
            group, handle, _, _ = struct.unpack_from('<IIII', table, i*16)
            if group == 0xffffffff or handle == 0xffffffff:
                continue
            queries = [(handle, g) for g in group_map[group]]
            queries += [(handle, 0x7a7a7a7a), (handle ^ 0x10000, group)]
            for query, expected in queries:
                address = run(0x122c10, registers=[(UC_X86_REG_ECX, query), (UC_X86_REG_EBX, expected)])
                output.append(f'L {query:08x} {expected:08x} {address:08x}')
                lookups += 1
        for handle in [0xffffffff, 0xe0008000, 0xe0007fff]:
            address = run(0x122c10, registers=[(UC_X86_REG_ECX, handle), (UC_X86_REG_EBX, 0x6f626a65)])
            output.append(f'L {handle:08x} 6f626a65 {address:08x}')
            lookups += 1
        report['maps'].append(dict(path=str(path.relative_to(ROOT)), sha256=hashlib.sha256(data).hexdigest(),
                                   tags=count, groups=group_count, lookup_calls=lookups, iterator_calls=iterations))
        print(path.name, 'lookups=', lookups, 'iterator_calls=', iterations, flush=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(output)+'\n')
    report['fixture_sha256'] = hashlib.sha256(args.output.read_bytes()).hexdigest()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(args.report)


if __name__ == '__main__':
    main()
