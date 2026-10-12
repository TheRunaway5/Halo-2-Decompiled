#!/usr/bin/env python3
"""Recover settings labels/options from the supplied PC executable and cache.

Run with .tools/python/bin/python (Capstone/Unicorn). No Windows process is
launched. The original scalar lookup routines execute in bounded x86 memory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX
from export_vista_menu_ui import VistaCache


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=Path('Halo 2 Vista installer/halo2.exe'))
    parser.add_argument('--map', type=Path, default=Path('Halo 2 Vista installer/maps/mainmenu.map'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    exe = args.exe.read_bytes()
    digest = hashlib.sha256(exe).hexdigest()
    if digest != '33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109':
        raise ValueError('unsupported supplied PC executable')
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    assert struct.unpack_from('<I', exe, pe + 52)[0] == 0x400000
    count = struct.unpack_from('<H', exe, pe + 6)[0]
    opt = struct.unpack_from('<H', exe, pe + 20)[0]
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x400000, 0x1000000)
    for i in range(count):
        p = pe + 24 + opt + i * 40
        _, rva, size, offset = struct.unpack_from('<4I', exe, p + 8)
        u.mem_write(0x400000 + rva, exe[offset:offset + size])
    u.mem_map(0x10000000, 0x10000)
    u.mem_map(0x20000000, 4096)

    def call(fn, value):
        u.reg_write(UC_X86_REG_ESP, 0x10008000)
        u.mem_write(0x10008000, struct.pack('<II', 0x20000000, value))
        u.emu_start(fn, 0x20000000, count=1000)
        assert u.reg_read(UC_X86_REG_ESP) in (0x10008004, 0x10008008)
        return u.reg_read(UC_X86_REG_EAX)

    choices = {272: (0x650d30, 10), 273: (0x650acd, 11),
               274: (0x65088e, 3), 271: (0x650f88, 2),
               285: (0x650670, 2), 290: (0x650419, 3)}
    recovered = {screen: [call(fn, i) for i in range(n)]
                 for screen, (fn, n) in choices.items()}
    editor = [call(0x634ca2, i) for i in range(43)]
    assert call(0x634ca2, 43) == 0xffffffff
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    targets = struct.unpack_from('<8I', exe, 0x648978 - 0x400000)
    video = []
    for target in targets:
        ins = next(md.disasm(exe[target - 0x400000:target - 0x400000 + 8], target))
        assert ins.mnemonic == 'mov' and ins.op_str.startswith('edi, ')
        video.append(int(ins.op_str.split(', ')[1], 16))
    assert video == [0xc000899, 0xa000897, 0x1000089a, 0xd00089c,
                     0xd00089d, 0xf00095e, 0x9000907, 0x1600089f]
    def jump_labels(table, n, register=None):
        result=[]
        for target in struct.unpack_from('<'+str(n)+'I',exe,table-0x400000):
            instructions=md.disasm(exe[target-0x400000:target-0x400000+28],target)
            ins=next(i for i in instructions if register is None or (i.mnemonic=='mov' and i.op_str.startswith(register+', 0x')))
            assert ins.mnemonic in ('push','mov')
            result.append(int(ins.op_str.split(', ')[-1],16))
        return result
    hub=jump_labels(0x61ea48,6)
    audio=jump_labels(0x626e90,4,"edi")
    def read_float(address):
        return struct.unpack('<f',u.mem_read(address,4))[0]
    effects={}
    for name,fn,dest,n in [('brightness',0x663d0e,0xe3e4c4,10),('gamma',0x663e28,0xe3e4c0,11)]:
        effects[name]=[]
        for i in range(n):
            u.mem_write(0x10000000,struct.pack('<I',i))
            call(fn,0x10000000)
            effects[name].append(read_float(dest))
    effects['lod']=[]
    for i in range(3):
        call(0x590b26,i)
        effects['lod'].append(dict(maximum=struct.unpack('<I',u.mem_read(0x8e6944,4))[0],
            coverage=read_float(0x8e6950),secondary=read_float(0x8e6954),effect=read_float(0x81f664),
            features=list(u.mem_read(0x8e6948,6))))
    effects['safe_area_height_fraction']=read_float(0x79de04)
    modes = [struct.unpack_from('<3I', exe, 0x8680c0 - 0x400000 + i * 12)
             for i in range(11)]
    assert struct.unpack_from('<f', exe, 0x7dbd6c - 0x400000)[0] == struct.unpack('<f', struct.pack('<f', 1/1200))[0]
    assert exe[0x64ba13 - 0x400000:0x64ba19 - 0x400000] == bytes.fromhex('81c1983a0000')
    cache = VistaCache(args.map)
    try:
        assert hashlib.sha256(cache.data).hexdigest() == 'eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44'
    finally:
        cache.close()
    args.output.mkdir(parents=True, exist_ok=True)
    lines = ['H2VISTA_SETTINGS_SOURCE_V1']
    lines += [f'hub\t{i}\t{sid:08x}' for i,sid in enumerate(hub)]
    lines += [f'audio\t{i}\t{sid:08x}' for i,sid in enumerate(audio)]
    lines += [f'video\t{i}\t{sid:08x}' for i, sid in enumerate(video)]
    lines += [f'editor\t{i}\t{sid:08x}' for i, sid in enumerate(editor)]
    for screen, ids in recovered.items():
        lines += [f'choice\t{screen}\t{i}\t{sid:08x}' for i, sid in enumerate(ids)]
    (args.output / 'effects.json').write_text(json.dumps(effects,indent=2)+'\n')
    (args.output / 'source.tsv').write_text('\n'.join(lines) + '\n')
    ranges = [(0x634ca2, 0x634e14), (0x6487d3, 0x648978),
              (0x65975e, 0x6597d9), (0x65e191, 0x65e22f),
              (0x64b9f0, 0x64ba44), (0x65c53b, 0x65c5d0),
              (0x590b26,0x590d51),(0x663d0e,0x6640b0),(0x664a18,0x664aac),
              (0x61e9cc,0x61ea60),(0x626c35,0x626ef0)]
    for a, z in ranges:
        (args.output / f'{a:08x}.asm').write_text(''.join(
            f'{i.address:08x} {i.mnemonic} {i.op_str}\n'
            for i in md.disasm(exe[a-0x400000:z-0x400000], a)))
    report = dict(exe_sha256=digest, map_sha256=hashlib.sha256(args.map.read_bytes()).hexdigest(),
                  video_rows=video, editor_rows=editor, choices=recovered, hub_rows=hub, audio_rows=audio, effects=effects,
                  windowed_modes=modes, ui_scale='window_height / 1200',
                  confirmation='(start_ms - current_ms + 15000) / 1000 + 1; signed division toward zero',
                  scope='Original label/choice lookup execution and static canvas/timer evidence; not full Vista rendering or gameplay parity.')
    (args.output / 'source.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Recovered {len(video)} video rows, {len(editor)} editor rows, {sum(map(len,recovered.values()))} choices, {len(modes)} windowed modes')

if __name__ == '__main__':
    main()
