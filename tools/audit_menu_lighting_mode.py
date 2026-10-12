#!/usr/bin/env python3
"""Execute original 2D000 lighting classification for the title BSP.

No service calls are replaced. This identifies the original classification;
it deliberately does not implement unresolved lighting-pass bindings.
"""
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
image = (ROOT/'assets/disc/default.xbe').read_bytes()
assert hashlib.sha256(image).hexdigest() == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
guest = Uc(UC_ARCH_X86, UC_MODE_32)
guest.mem_map(0x10000, 0x600000)
base = struct.unpack_from('<I', image, 0x104)[0]
count, table = struct.unpack_from('<II', image, 0x11c)
for i in range(count):
    _, address, _, offset, size = struct.unpack_from('<5I', image, table-base+i*56)
    guest.mem_write(address, image[offset:offset+size])
cache = Cache(ROOT/'assets/disc/maps/mainmenu.map')
_, scenario = cache.tag(cache.scenario, 'scnr')
bsp = cache.block(scenario+0x210, 68)[0]
file, size, address = struct.unpack_from('<3I', cache.data, bsp)
guest.mem_map(0x2100000, 0x100000)
guest.mem_map(0x3000000, 4096)
mapped = address & ~4095
guest.mem_map(mapped, (address+size-mapped+4095) & ~4095)
guest.mem_write(address, bytes(cache.data[file:file+size]))
lighting = cache.u32(file+8)
guest.mem_write(0x4e0344, struct.pack('<I', lighting))
guest.mem_write(0x4e0348, struct.pack('<I', address+16))
group = cache.block(file+lighting-address+128, 104, address-file)[0]
clusters = cache.u32(file+16+156)
instances = cache.u32(file+16+320)
rows = []
for kind, indices in [('cluster', range(clusters)), ('instance', range(instances))]:
    for index in indices:
        guest.mem_write(0x21fff00, struct.pack('<I', 0x3000000))
        guest.reg_write(UC_X86_REG_ESP, 0x21fff00)
        guest.reg_write(UC_X86_REG_ECX, 0x7ff if kind == 'cluster' else index)
        guest.reg_write(UC_X86_REG_EDX, 0xffffffff)
        guest.reg_write(UC_X86_REG_EAX, 0xffffffff)
        guest.emu_start(0x2d000, 0x3000000, count=1000)
        assert guest.reg_read(UC_X86_REG_EIP) == 0x3000000
        rows.append({'kind': kind, 'index': index, 'mode': guest.reg_read(UC_X86_REG_EAX)})
report = {'entry': '0002D000', 'group_bitmap': hex(cache.u32(group+28)),
          'group_checksum': hex(cache.u32(group+4)), 'bsp_checksum': hex(cache.u32(file+24)),
          'cases': len(rows), 'classification_counts': {str(mode): sum(r['mode']==mode for r in rows) for mode in sorted(set(r['mode'] for r in rows))},
          'rows': rows, 'native_lighting_parity_verified': False}
out = ROOT/'analysis/frontend/xemu-capture/title-original-lighting-mode.json'
out.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='rows'}, indent=2))
