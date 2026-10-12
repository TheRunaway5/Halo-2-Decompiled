#!/usr/bin/env python3
"""Execute the complete unchanged D1E10 object branch over literal contacts.

The eight inputs are independently recorded 14A8E0 diagnostic outputs. Controls
change data only: current PRT count, model PRT count, section NONE, the material's shader
datum NONE, explicit radiance/incident streams, and float PRT point layouts, and distinct geometry-section/PRT-cache indices.
No guest actions or substituted source services.
Native replay supplies the independently recorded D30D0 bitmap service result;
this does not prove a native bitmap residency or current PRT cache producer.
"""
from pathlib import Path
import collections, hashlib, json, struct, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed, fw
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH, CONTACT, OUT, STACK, STOP = 0x20000000, 0x20000100, 0x20000200, 0x200fe000, 0x200ff000

def main():
    snapshot = Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest() == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32); u.mem_map(SYNTH, 0x100000)
    pages = {}; verified = set(); calls = collections.Counter(); observed = {}
    def page(address):
        base = address & ~4095
        if base not in pages:
            raw = snapshot.read(base, 4096); u.mem_map(base, 4096); u.mem_write(base, raw); pages[base] = raw
    def read(address, size):
        if size == 0: return b''
        for base in range(address & ~4095, ((address+size-1) & ~4095)+4096, 4096):
            if not SYNTH <= base < SYNTH+0x100000: page(base)
        return bytes(u.mem_read(address, size))
    def write(address, raw):
        read(address, len(raw)); u.mem_write(address, raw)
    def unmapped(g, access, address, size, value, data):
        for base in range(address & ~4095, ((address+size-1) & ~4095)+4096, 4096): page(base)
        return True
    def code(g, address, size, data):
        if address == STOP: g.emu_stop(); return
        if address in [0xd1e10, 0x24490, 0x241c0, 0xd33a0, 0xd31c0, 0xd2dc0, 0xd30d0]: calls[f'{address:08X}'] += 1
        if address == 0xd220c:
            descriptor = read(g.reg_read(UC_X86_REG_EAX), 16)
            indices = struct.unpack('<3H', read(g.reg_read(UC_X86_REG_ESI), 6))
            start = uint(descriptor, 4)+uint(descriptor, 12)
            size = (max(indices)+1)*(10 if descriptor[0] == 57 else 20)
            observed['resource'] = [descriptor[0], read(start, size)]
            observed['resource_descriptor'] = g.reg_read(UC_X86_REG_EAX)
            observed['resource_offset'] = uint(descriptor,4)
        if address == 0xd2dc0:
            observed['uv_out'] = [g.reg_read(UC_X86_REG_EBX), g.reg_read(UC_X86_REG_EDI)]
        if address == 0xd294c:
            observed['uv'] = [uint(read(p, 4)) for p in observed['uv_out']]
        if address == 0xd30d0:
            observed['diffuse_out'] = g.reg_read(UC_X86_REG_EDI)
            observed['bitmap_datum'] = g.reg_read(UC_X86_REG_EAX)
            observed['bitmap_args'] = words(read(g.reg_read(UC_X86_REG_ESP)+4, 12))
        if address == 0xd2984:
            observed['diffuse'] = [g.reg_read(UC_X86_REG_EAX)&255, *words(read(observed['diffuse_out'], 12))]
        if address in verified or SYNTH <= address < SYNTH+0x100000: return
        for section in image.sections:
            if section.va <= address and address+size <= section.va+section.rsize:
                assert read(address, size) == image.section_bytes(section)[address-section.va:address-section.va+size], f'changed source instruction {address:08X}'
                verified.add(address); return
        raise RuntimeError(f'uncaptured source instruction service {address:08X}')
    u.hook_add(UC_HOOK_MEM_UNMAPPED, unmapped); u.hook_add(UC_HOOK_CODE, code)
    table = uint(snapshot.read(0x4e3b44, 4))
    tag = lambda datum: uint(snapshot.read(table+(datum&65535)*16+8, 4))
    inputs = json.loads((ROOT/'analysis/multiplayer-shading/dynamic-object-attribution-snapshot-original.json').read_text())
    rows = ['# Complete unchanged D1E10 object branch. A=current PRT; G=raw geometry; R=raw PRT resource; C=explicit contact/services/output.']
    def emit(kind, values): rows.append(kind+' '+' '.join(f'{v&0xffffffff:08X}' for v in values))
    def raw_words(raw): return [len(raw), *words(raw+bytes((-len(raw))%4))]
    count = struct.unpack('<i', snapshot.read(0x5093e8, 4))[0]
    cache_banks = {(None,None):(0,snapshot.read(0x4b89b0,count*104),snapshot.read(0x487b1c,0x27000))}
    emit('A', [0, count, *words(cache_banks[(None,None)][1]), *raw_words(cache_banks[(None,None)][2])])
    default = words(snapshot.read(uint(snapshot.read(0x4687b0, 4)), 12))
    geometries = {}; resource_ids = {}; records = []; categories = collections.Counter()
    for entry in inputs['records']:
        if not entry.get('attributed'): continue
        contact = entry['output_words']; mode = tag(contact[3])
        geometry = uint(snapshot.read(uint(snapshot.read(mode+0x28, 4))+contact[5]*0x5c+0x34, 4))
        if geometry not in geometries:
            gid = len(geometries); geometries[geometry] = gid
            ni, ip = words(snapshot.read(geometry+0x20, 8)); ns, dp = words(snapshot.read(geometry+0x38, 8))
            indices = list(struct.unpack('<'+'H'*ni, snapshot.read(ip, ni*2)))
            values = [gid, ni, *indices, ns]
            for i in range(ns):
                descriptor = snapshot.read(dp+i*32, 32)
                nv = struct.unpack_from('<H', descriptor, 2)[0]
                values += [descriptor[0], uint(descriptor, 8), *raw_words(snapshot.read(uint(descriptor, 12), nv*descriptor[1]))]
            emit('G', values)
        gid = geometries[geometry]
        ni, ip = words(snapshot.read(geometry+0x20, 8)); ns, dp = words(snapshot.read(geometry+0x38, 8))
        indices = list(struct.unpack('<'+'H'*ni, snapshot.read(ip, ni*2)))
        assert ns == 3, 'these retained object contacts have three source descriptors'
        nv = max(indices)+1
        extra_banks = {}
        for baked in [1, 2]:
            key = (geometry, baked)
            if key in geometries:
                extra_banks[baked] = geometries[key]; continue
            extra_banks[baked] = len(geometries); geometries[key] = extra_banks[baked]
            values = [extra_banks[baked], ni, *indices, ns+2]
            for i in range(ns):
                d = snapshot.read(dp+i*32, 32); size = struct.unpack_from('<H', d, 2)[0]*d[1]
                values += [d[0], uint(d, 8), *raw_words(snapshot.read(uint(d, 12), size))]
            radiance = packed([0xff80a0c0 if baked == 1 else 0]*nv)
            incident = packed([0x80200400]*nv)
            values += [46, 0, *raw_words(radiance), 48, 0, *raw_words(incident)]
            emit('G', values)
        model_count = struct.unpack('<i', snapshot.read(mode+0x74, 4))[0]
        part_pointer = uint(snapshot.read(geometry+4, 4))+contact[8]*72
        material_index = struct.unpack('<h', snapshot.read(part_pointer+4, 2))[0]
        material = uint(snapshot.read(mode+0x64, 4))+material_index*32
        actual_resource = None
        for cache_index in [contact[7], 1, 15]:
            cache_id, cache_rows, cache_arena = cache_banks[(None,None)]
            if cache_index != contact[7]:
                key = (contact[2],cache_index)
                if key not in cache_banks:
                    controlled_rows = bytearray(cache_rows); controlled_arena = bytearray(cache_arena)
                    row = next(i for i in range(count) if uint(controlled_rows,i*104)==contact[2])
                    base = uint(controlled_rows,row*104+8)&0x0fffffff
                    assert base%4==0
                    struct.pack_into('<I',controlled_rows,row*104+(3+cache_index)*4,(0x26000-base)//4)
                    color = [.15,.7,.1] if cache_index==1 else [.9,.05,.25]
                    controlled_arena[0x26000:0x26400] = packed([fw(v) for v in color+[0.]*13]*16)
                    bank_id = len(cache_banks); cache_banks[key]=(bank_id,bytes(controlled_rows),bytes(controlled_arena))
                    emit('A',[bank_id,count,*words(controlled_rows),*raw_words(controlled_arena)])
                cache_id,cache_rows,cache_arena = cache_banks[key]
            for point_layout in [0, 1, 255]:
                for baked in [0, 1, 2]:
                    for current_count in [count, 0]:
                        for authored_count in [model_count, 0]:
                            for bitmap_missing in [False, True]:
                                for section_missing in [False, True]:
                                    # Restore every lazy-mapped captured page between cases.
                                    for address, raw in pages.items(): u.mem_write(address, raw)
                                    write(0x4b89b0,cache_rows); write(0x487b1c,cache_arena)
                                    p = list(contact); p[7] = cache_index
                                    if section_missing: p[5] = 0xffffffff
                                    write(0x5093e8, packed([current_count])); write(mode+0x74, packed([authored_count]))
                                    if bitmap_missing: write(material+12, packed([0xffffffff]))
                                    actual_gid = gid
                                    if baked:
                                        actual_gid = extra_banks[baked]
                                        sg, sd, sr = SYNTH+0x1000, SYNTH+0x2000, SYNTH+0x4000
                                        si = sr+nv*4+16
                                        assert si+nv*4 < STACK
                                        u.mem_write(sg, snapshot.read(geometry, 0x70))
                                        u.mem_write(sd, snapshot.read(dp, ns*32))
                                        u.mem_write(sd+ns*32, bytes([46,4])+struct.pack('<H',nv)+packed([0,0,sr,0,0,0,0]))
                                        u.mem_write(sd+(ns+1)*32, bytes([48,4])+struct.pack('<H',nv)+packed([0,0,si,0,0,0,0]))
                                        u.mem_write(sr, packed([0xff80a0c0 if baked == 1 else 0]*nv)); u.mem_write(si, packed([0x80200400]*nv))
                                        u.mem_write(sg+0x38, packed([ns+2,sd]))
                                        section_ptr = uint(snapshot.read(mode+0x28,4))+contact[5]*0x5c+0x34
                                        write(section_ptr, packed([sg]))
                                    if point_layout:
                                        assert actual_resource is not None
                                        address, original_payload, original_size = actual_resource
                                        converted = []
                                        reciprocal = struct.unpack('<f', packed([0x37800080]))[0]
                                        for point in struct.iter_unpack('<5h', original_payload):
                                            converted.extend([fw(float(point[0])), *[fw((v*2+1)*reciprocal) for v in point[1:]]])
                                        resource_data = SYNTH+0x60000
                                        assert resource_data+len(converted)*4 < STACK
                                        write(address, bytes([point_layout])); write(address+12, packed([resource_data-original_size*2]))
                                        u.mem_write(resource_data, packed(converted))
                                    observed.clear()
                                    u.mem_write(CONTACT, packed(p)); u.mem_write(OUT, packed([0xcdcdcdcd]*5))
                                    u.mem_write(STACK, packed([STOP, CONTACT, OUT, fw(1.), 0]))
                                    for reg in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]: u.reg_write(reg, 0)
                                    u.reg_write(UC_X86_REG_ESP, STACK); u.reg_write(UC_X86_REG_EFLAGS, 2)
                                    u.reg_write(UC_X86_REG_FPCW, 0x023f); u.reg_write(UC_X86_REG_FPSW, 0); u.reg_write(UC_X86_REG_FPTAG, 0xffff); u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
                                    u.emu_start(0xd1e10, STOP+1, count=3000000)
                                    assert u.reg_read(UC_X86_REG_EIP) == STOP and u.reg_read(UC_X86_REG_ESP) == STACK+20
                                    status = u.reg_read(UC_X86_REG_EAX); output = words(read(OUT, 20))
                                    resource = observed.get('resource'); rid = 0xffffffff
                                    if resource:
                                        if point_layout == 0:
                                            assert resource[0] == 57
                                            actual_resource = (observed['resource_descriptor'], resource[1], observed['resource_offset'])
                                        key = (resource[0], resource[1])
                                        if key not in resource_ids:
                                            resource_ids[key] = len(resource_ids); emit('R', [resource_ids[key], resource[0], *raw_words(resource[1])])
                                        rid = resource_ids[key]
                                    uv = observed.get('uv', [0, 0]); diffuse = observed.get('diffuse', [0, 0, 0, 0])
                                    if bitmap_missing and not section_missing: assert observed['bitmap_datum'] == 0xffffffff and diffuse[0] == 0
                                    if section_missing: assert output == [0xcdcdcdcd]*5 and status == 2
                                    assert read(0x4b89b0, count*104) == cache_rows
                                    assert read(0x487b1c, 0x27000) == cache_arena
                                    emit('C', [cache_id, actual_gid, rid, authored_count, current_count, *p, *default, *uv, *diffuse, status, *output])
                                    category = 'retained_inputs' if cache_index == contact[7] and point_layout == 0 and baked == 0 and current_count == count and authored_count == model_count and not bitmap_missing and not section_missing else 'explicit_data_controls'
                                    categories[category] += 1
                                    records.append({'contact_index': entry['index'], 'geometry': actual_gid, 'baked_control': baked, 'point_layout_control': point_layout, 'cache_index_control': cache_index, 'cache_bank': cache_id, 'prt_resource': rid, 'current_count': current_count, 'model_count': authored_count,
                                        'bitmap_missing': bitmap_missing, 'section_missing': section_missing, 'status': status, 'output_words': output,
                                        'uv_words': uv, 'diffuse': diffuse, 'bitmap_datum': observed.get('bitmap_datum'), 'bitmap_args': observed.get('bitmap_args')})
    fixture = ROOT/'tests/fixtures/retail_source_object_light_sample.tsv'; fixture.write_text('\n'.join(rows)+'\n')
    report = {'source_xbe_sha256': PIN, 'snapshot_ram_sha256': hashlib.sha256(snapshot.ram).hexdigest(), 'snapshot_tick': snapshot.report['clock_tick'],
        'source_unchanged': True, 'instructions_verified': len(verified), 'calls': dict(calls), 'categories': dict(categories), 'cases': len(records),
        'geometries': len(geometries), 'resources': len(resource_ids), 'cache_banks': len(cache_banks), 'prt_cache_rows_unchanged': True, 'prt_cache_arena_unchanged': True,
        'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(), 'tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'records': records, 'boundary': __doc__}
    fixture.with_suffix('.provenance.json').write_text(json.dumps(report, indent=2)+'\n')
    (ROOT/'analysis/multiplayer-shading/object-light-sample-original.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: report[k] for k in ['cases', 'categories', 'geometries', 'resources', 'cache_banks', 'instructions_verified', 'calls']}), flush=True)
if __name__ == '__main__': main()
