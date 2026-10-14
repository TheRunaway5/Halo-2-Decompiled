#!/usr/bin/env python3
"""Whole unchanged TU1.5 134D30 on actual grenade receiver object/spatial banks.

C35D0 and31C70 execute to134D30 with no replaced instructions or callees.
The first case preserves the original caller. Additional cases declare query
bounds/flags and auxiliary queue presence explicitly; all object generations,
body bounds and cluster links remain actual snapshot inputs. This verifies
receiver admission, not live native registration or framebuffer parity.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot, u

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


def words(raw):
    return list(struct.unpack('<'+'I'*(len(raw)//4), raw))


def pack(values):
    return struct.pack('<'+'I'*len(values), *values)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot', type=Path, required=True)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    s, x = Snapshot(a.snapshot), Xbe(a.xbe)
    assert hashlib.sha256(x.data).hexdigest() == PIN
    state = json.loads((a.snapshot/'source-state.json').read_text())
    assert not state['guest_code_patches']
    assert hashlib.sha256(s.ram).hexdigest() == state['ram_sha256']
    t = x.section('.text')
    assert s.read(t.va, t.rsize) == x.read(t.va, t.rsize)
    g = s.guest()
    g.mem_map(0x70000000, 0x100000)
    sp, stop = 0x700fe000, 0x700fff00
    executed = set()

    def hook(vm, pc, n, _):
        if pc not in executed:
            assert bytes(vm.mem_read(pc, n)) == x.read(pc, n), hex(pc)
            executed.add(pc)

    g.hook_add(UC_HOOK_CODE, hook)

    def read(vm, at, n):
        for page in range(at & ~4095, (at+n+4095) & ~4095, 4096):
            if not any(b <= page <= e for b, e, _ in vm.mem_regions()):
                vm.mem_map(page, 4096)
                vm.mem_write(page, s.read(page, 4096))
        return bytes(vm.mem_read(at, n))

    def val(vm, at):
        return u(read(vm, at, 4))

    def write(vm, at, raw):
        read(vm, at, len(raw))
        vm.mem_write(at, raw)

    def call(vm, entry, end=stop, args=()):
        vm.mem_write(sp, pack([stop, *args]))
        vm.reg_write(R.UC_X86_REG_ESP, sp)
        vm.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        vm.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        vm.emu_start(entry, end, count=30000000)
        assert vm.reg_read(R.UC_X86_REG_EIP) == end

    def queue(vm, context):
        header = val(vm, context+0x14)
        capacity, count, flags, ids, other, assoc = words(read(vm, header, 24))
        return capacity, [[val(vm, ids+i*4), struct.unpack('<H', read(vm, flags+i*2, 2))[0],
                           val(vm, other+i*4), struct.unpack('<H', read(vm, assoc+i*2, 2))[0]]
                          for i in range(count & 65535)]

    def associations(vm, context):
        count = struct.unpack('<H', read(vm, context+0xa5e, 2))[0]
        return [words(read(vm, context+0xa60+i*16, 16)) for i in range(count)]

    def clone():
        vm = s.guest()
        for b, e, permissions in g.mem_regions():
            vm.mem_map(b, e-b+1, permissions)
            vm.mem_write(b, bytes(g.mem_read(b, e-b+1)))
        vm.context_restore(g.context_save())
        vm.hook_add(UC_HOOK_CODE, hook)
        return vm

    call(g, 0xc35d0)
    call(g, 0x31c70, 0x134d30)
    context = val(g, g.reg_read(R.UC_X86_REG_ESP)+4)
    assert context == val(g, 0x4cd3a0)
    assert val(g, context+0x2a78) == val(g, context+0x2a7c) == 1
    auxiliary = val(g, context+0x2ad0)
    assert auxiliary and not queue(g, context)[1]
    visibility = val(g, context)
    groups = struct.unpack('<H', read(g, visibility, 2))[0]
    count = struct.unpack('<H', read(g, visibility+0xa6c, 2))[0]
    entries = [list(struct.unpack('<13h', read(g, visibility+0xa6e+i*26, 26))) for i in range(count)]
    bsp = val(g, 0x4ecc18)
    clusters = val(g, bsp+0x9c)
    fog_vm = clone()
    fog = [0]*512
    for index in range(clusters):
        fog_vm.reg_write(R.UC_X86_REG_ECX, index)
        fog_vm.reg_write(R.UC_X86_REG_EDI, val(g, 0x4c67ac))
        call(fog_vm, 0x171070)
        fog[index] = fog_vm.reg_read(R.UC_X86_REG_EAX) & 255
    pools = []
    for head_global, pool_global in ((0x4eabb0, 0x4eabb4), (0x4eaba4, 0x4eaba8)):
        header = val(g, pool_global)
        assert val(g, header+0x20) == 2048 and val(g, header+0x24) == 32
        pools.append(dict(heads=words(read(g, val(g, head_global), 512*4)),
                          records=[words(read(g, val(g, header+0x44)+i*32, 32)) for i in range(2048)]))
    pool = val(g, 0x4ecbd0)
    assert val(g, pool+0x20) == 2048 and val(g, pool+0x24) == 12
    occupied = words(read(g, val(g, pool+0x48), 256))
    objects, memory_banks = [], []
    def retain(address, size):
        memory_banks.append(dict(address=address, hex=read(g,address,size).hex()))
    memory_roots = [pool,val(g,0x4eabb0),val(g,0x4eabb4),val(g,0x4eaba4),val(g,0x4eaba8)]
    for address,stride in ((pool,12),(memory_roots[2],32),(memory_roots[4],32)):
        retain(address,76)
        retain(val(g,address+0x44),2048*stride)
        retain(val(g,address+0x48),256)
    for address in (memory_roots[1],memory_roots[3]):retain(address,512*4)
    for slot in range(2048):
        if not occupied[slot//32] & (1 << (slot % 32)):
            continue
        header = read(g, val(g, pool+0x44)+slot*12, 12)
        owner = (struct.unpack_from('<H', header)[0] << 16) | slot
        body = u(header, 8)
        retain(body,0xac)
        objects.append(dict(id=owner, flags=val(g, body+4), type=read(g, body+0xaa, 1)[0],
                            rendering=words(read(g, body+0x30, 16)), lighting=words(read(g, body+0x50, 16))))
    original_sphere = words(read(g, context+0x2a68, 16))
    flags = val(g, context+0x2a60)
    probes = [(original_sphere, flags, True, 'actual31C70')]
    for sphere in [original_sphere, *[o['lighting'] for o in objects[::max(1, len(objects)//12)]]]:
        for radius in (0., .371, 2., 2.999999761581421):
            query = sphere[:3]+[u(struct.pack('<f', radius))]
            for f in (0, 1, 0x81, 0x201, 0x401, 0x641):
                probes.append((query, f, bool(len(probes) % 2), 'declared134D30-input'))
    cases = []
    for sphere, f, has_aux, label in probes:
        vm = clone()
        write(vm, context+0x2a60, pack([f]))
        write(vm, context+0x2a68, pack(sphere))
        write(vm, context+0x2ad0, pack([auxiliary if has_aux else 0]))
        call(vm, 0x134d30, args=(context,))
        cases.append(dict(label=label, sphere=sphere, flags=f, auxiliary=has_aux,
                          queue=queue(vm, context)[1], associations=associations(vm, context)))
    fixture = dict(scope=__doc__, original_xbe_sha256=PIN, snapshot_sha256=state['ram_sha256'],
        groups=groups, entries=entries, fog=fog, pools=pools, objects=objects,
        memory_roots=memory_roots,memory_banks=memory_banks,
        capacity=queue(g, context)[0], auxiliary=queue(g, auxiliary)[1],
        prior=associations(g, context), plane=words(read(g, context+0x2a88, 16)),
        mode=1, context_2a78=1, cases=cases, unique_instructions=len(executed),
        replaced_instructions=0, replaced_callees=0,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(fixture, separators=(',', ':'))+'\n')
    print('PASS original TU1.5 object light admission:', len(cases), 'queries,', len(objects),
          'actual objects, actual queue', cases[0]['queue'], flush=True)


if __name__ == '__main__':
    main()
