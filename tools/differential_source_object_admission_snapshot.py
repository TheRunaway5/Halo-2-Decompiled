#!/usr/bin/env python3
"""Replay unchanged object admission over an immutable restored Xbox snapshot.

The complete object records and independent spatial data arrays are read through
the captured CPU page tables. No target lookup, registration, allocation, fog,
frustum, queue or association routine is replaced. Visibility inputs are either
the retained current bank or explicitly synthetic banks. This is a fresh pass,
not a claim that the saved collector was produced at this paused instruction.
"""
import argparse, collections, hashlib, json, random, re, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
SYNTH, SET, CONTEXT, QUEUES, ARRAYS = 0x20000000, 0x20000000, 0x20040000, 0x20050000, 0x20060000
STACK, STOP = 0x200fe000, 0x200ff000

def uint(data, at=0): return struct.unpack_from('<I', data, at)[0]
def words(data): return list(struct.unpack('<'+'I'*(len(data)//4), data))
def packed(values): return struct.pack('<'+'I'*len(values), *(v & 0xffffffff for v in values))
def fw(value): return struct.unpack('<I', struct.pack('<f', value))[0]

class Snapshot:
    def __init__(self, directory):
        self.directory = directory
        self.report = json.loads((directory/'export.json').read_text())
        self.ram = (directory/'exports/physical-ram.bin').read_bytes()
        self.cr3 = int(re.search(r'CR3=([0-9a-fA-F]+)', self.report['registers_before'])[1], 16)
        assert not int(re.search(r'CR4=([0-9a-fA-F]+)', self.report['registers_before'])[1], 16) & 32
        assert self.report['qmp_before']['running'] is False and self.report['qmp_after']['running'] is False
    def physical(self, address):
        pde = uint(self.ram, (self.cr3 & 0xfffff000)+((address >> 22) & 1023)*4)
        if not pde & 1: raise ValueError(f'absent captured PDE for {address:08X}')
        if pde & 128: return (pde & 0xffc00000)+(address & 0x3fffff)
        pte = uint(self.ram, (pde & 0xfffff000)+((address >> 12) & 1023)*4)
        if not pte & 1: raise ValueError(f'absent captured PTE for {address:08X}')
        return (pte & 0xfffff000)+(address & 0xfff)
    def read(self, address, size):
        output = bytearray()
        while size:
            count = min(size, 4096-(address & 4095)); physical = self.physical(address)
            if physical+count > len(self.ram): raise ValueError('captured virtual address reaches uncaptured MMIO')
            output.extend(self.ram[physical:physical+count]); address += count; size -= count
        return bytes(output)
    def file(self, name): return (self.directory/'exports'/name).read_bytes()

def cube(center=(0., 0., 0.), half=128.):
    x, y, z = center
    return [fw(v) for v in [x-half, x+half, y-half, y+half, z-half, z+half,
        1,0,0,x+half, -1,0,0,half-x, 0,1,0,y+half, 0,-1,0,half-y,
        0,0,1,z+half, 0,0,-1,half-z]]

def serialize(case):
    result = [case['capacity'], case['flags'], *case['plane'], case['groups'], len(case['entries'])]
    for entry in case['entries']: result.extend(v & 65535 for v in entry)
    result.append(len(case['frustums']))
    for frustum in case['frustums']: result.extend(frustum)
    result.append(len(case['fog'])); result.extend(case['fog'])
    result.append(len(case['prior']))
    for mask in case['prior']: result.extend(mask)
    return result

def freeze_snapshot(snapshot, path, cluster_count):
    # Full original banks, including unused slots, are retained. No links are
    # rebuilt from owner membership or source metadata order.
    names = ['object-pool-header.bin', 'object-pool-records.bin', 'object-pool-occupied.bin']
    for side in ['collidable', 'noncollidable']:
        names.append(side+'-heads.bin')
        for kind in ['entries', 'refs']:
            names.extend(f'{side}-{kind}-{suffix}.bin' for suffix in ['header', 'records', 'occupied'])
    result = bytearray(b'H2OASNP1'+packed([cluster_count, len(names)]))
    hashes = {}
    for name in names:
        raw = snapshot.file(name); result.extend(packed([len(raw)])); result.extend(raw)
        hashes[name] = hashlib.sha256(raw).hexdigest()
    owners = snapshot.report['objects']; result.extend(packed([len(owners)]))
    for owner in owners:
        raw = snapshot.file(f"object-{owner['handle']}.bin")
        assert raw == snapshot.read(int(owner['address'], 16), owner['size'])
        result.extend(packed([int(owner['handle'],16), int(owner['address'],16), len(raw)])); result.extend(raw)
    path.write_bytes(result)
    return hashes

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, default=ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--fixture', type=Path, default=ROOT/'tests/fixtures/retail_source_object_admission_snapshot.tsv')
    parser.add_argument('--snapshot-fixture', type=Path, default=ROOT/'tests/fixtures/retail_source_object_admission_snapshot.bin')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer-shading/object-admission-snapshot-original.json')
    args = parser.parse_args()
    snapshot = Snapshot(args.snapshot); image = Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest() == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32); u.mem_map(SYNTH, 0x100000)
    original_pages, calls = {}, collections.Counter()
    source_entries = [0x132220,0x133050,0x132e60,0x165010,0x181800,0x132fd0,
        0x132b10,0x134300,0x163c80,0xbec70,0x16e210,0x1348e0,0x1cac60,0x16b840]
    verified_instructions = set()
    def page(address):
        base = address & 0xfffff000
        if base not in original_pages:
            raw = snapshot.read(base,4096); u.mem_map(base,4096); u.mem_write(base,raw); original_pages[base] = raw
    def unmapped(_u,_access,address,size,_value,_data):
        for base in range(address & ~4095, ((address+size-1)&~4095)+4096,4096): page(base)
        return True
    def code(_u,address,size,_data):
        if address in source_entries: calls[address] += 1
        if address in verified_instructions or SYNTH <= address < SYNTH+0x100000: return
        for section in image.sections:
            if section.va <= address and address+size <= section.va+section.rsize:
                expected=image.section_bytes(section)[address-section.va:address-section.va+size]
                if bytes(u.mem_read(address,size)) != expected: raise RuntimeError(f'original instruction changed at {address:08X}')
                verified_instructions.add(address); return
        raise RuntimeError(f'uncaptured external instruction service at {address:08X}')
    u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped); u.hook_add(UC_HOOK_CODE,code)
    def read(address,size):
        for base in range(address & ~4095, ((address+size-1)&~4095)+4096,4096):
            if not SYNTH <= base < SYNTH+0x100000: page(base)
        return bytes(u.mem_read(address,size))
    def write(address,data): read(address,len(data)); u.mem_write(address,data)
    def put(address,*values): write(address,packed(values))
    def run(address,argv=(),registers={}):
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]: u.reg_write(reg,registers.get(reg,0))
        put(STACK,STOP,*argv); u.reg_write(UC_X86_REG_ESP,STACK); u.reg_write(UC_X86_REG_EFLAGS,2)
        u.reg_write(UC_X86_REG_FPCW,0x023f); u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(address,STOP,count=20000000)
        if u.reg_read(UC_X86_REG_EIP)!=STOP: raise RuntimeError(f'original {address:X} did not return')
    actual_context = snapshot.read(0x547f88,0x2ae0); actual_set_address=uint(actual_context)
    actual_set=snapshot.read(actual_set_address,0x22974)
    retained_inputs=args.output.parent/'object-admission-snapshot-inputs'
    retained_inputs.mkdir(parents=True,exist_ok=True)
    (retained_inputs/'original-context-547F88.bin').write_bytes(actual_context)
    (retained_inputs/'original-visibility-set.bin').write_bytes(actual_set)
    groups=struct.unpack_from('<H',actual_set)[0]; count=struct.unpack_from('<H',actual_set,0xa6c)[0]
    frustums=struct.unpack_from('<H',actual_set,0x1970)[0]
    assert groups<=6 and count<=128 and frustums<=512
    assert uint(actual_context,0x2a78)==0 and uint(actual_context,0x2ad0)==0
    bsp=uint(snapshot.read(0x4e0348,4)); cluster_count=uint(snapshot.read(bsp+0x9c,4)); assert cluster_count==55
    fog=[]
    for cluster in range(cluster_count):
        run(0x16e210,registers={UC_X86_REG_ECX:cluster,UC_X86_REG_EDI:uint(snapshot.read(0x4b9f8c,4))})
        fog.append(u.reg_read(UC_X86_REG_EAX)&255)
    fog.extend([0]*(512-cluster_count)) # Explicit unqueried unused capacity, not invented authored clusters.
    hashes=freeze_snapshot(snapshot,args.snapshot_fixture,cluster_count)
    base=dict(capacity=256,flags=0x10,plane=[0]*4,groups=groups,
        entries=[list(struct.unpack_from('<13H',actual_set,0xa6e+i*0x1a))for i in range(count)],
        frustums=[words(actual_set[0x1974+i*0x108+0x60:0x1974+i*0x108+0x60+120])for i in range(frustums)],
        fog=fog,prior=[])
    cases=[]
    def add(label, **overrides):
        case=dict(base);case.update(overrides);cases.append((label,case))
    add('retained-current-frusta-fresh-ordinary-pass')
    prior_count=struct.unpack_from('<H',actual_context,0xa5e)[0]
    add('retained-frusta-explicit-prior-associations',prior=[words(actual_context[0xa60+i*16:0xa70+i*16])for i in range(prior_count)])
    for flags in [0x50,0x90,0x410,0x210,0x610]: add(f'retained-frusta-flags-{flags:x}',flags=flags,plane=[fw(.4),fw(-.2),fw(.1),fw(1.)])
    for capacity in [1,2,4,16,32,64,128,256]: add(f'retained-frusta-capacity-{capacity}',capacity=capacity)
    rng=random.Random(0x132220)
    for index in range(24):
        order=list(range(cluster_count))
        if index&1: order.reverse()
        if index&2: rng.shuffle(order)
        group_count=1+index%6; entries=[]
        for ordinal,cluster in enumerate(order):
            counts=[0]*6;firsts=[65535]*6;group=ordinal%group_count
            counts[group]=1;firsts[group]=ordinal;entries.append([cluster,*counts,*firsts])
        center=(rng.uniform(-35,35),rng.uniform(-35,35),rng.uniform(-10,20)); half=[2.,5.,15.,64.,128.][index%5]
        add(f'explicit-synthetic-frusta-{index}',groups=group_count,entries=entries,
            frustums=[cube(center,half)for _ in order],capacity=[4,32,128,256][index%4],
            flags=[0x10,0x90,0x410,0x210][index%4],plane=[fw(.5),fw(-.25),fw(.75),fw(2.)],
            prior=[[rng.getrandbits(32)for _ in range(4)]for _ in range(512 if index==23 else index%5)])
    protected=[]
    for entry in snapshot.report['files']:
        if entry['address_space']=='CPU0 virtual' and ('object-' in entry['file'] or any(side in entry['file']for side in ['collidable','noncollidable'])):
            protected.append((int(entry['address'],16),snapshot.read(int(entry['address'],16),entry['size'])))
    expected=[];rows=[];case_reports=[]
    for label,case in cases:
        for address,raw in original_pages.items(): u.mem_write(address,raw)
        u.mem_write(SYNTH,bytes(0x100000));put(SET,case['groups'])
        write(SET+0xa6c,struct.pack('<H',len(case['entries'])))
        for i,entry in enumerate(case['entries']):write(SET+0xa6e+i*0x1a,struct.pack('<13H',*entry))
        for i,frustum in enumerate(case['frustums']):put(SET+0x1974+i*0x108+0x60,*frustum)
        put(CONTEXT,SET);write(CONTEXT+8,struct.pack('<h',-1));put(CONTEXT+0x2a60,case['flags']);put(CONTEXT+0x2a88,*case['plane'])
        for i,offset in enumerate([0xc,0x10,0x14,0x18]):
            address=QUEUES+i*0x100;arrays=ARRAYS+i*0x10000
            put(address,case['capacity']if i==2 else 1,0,arrays,arrays+0x2000,arrays+0x4000,arrays+0x6000);put(CONTEXT+offset,address)
        run(0x1348e0,registers={UC_X86_REG_EDX:CONTEXT})
        write(CONTEXT+0xa5e,struct.pack('<H',len(case['prior'])))
        for i,mask in enumerate(case['prior']):put(CONTEXT+0xa60+i*16,*mask)
        put(0x4de2fc,uint(snapshot.read(0x4de2fc,4))+1)
        write(0x4b9ee9,b'\0') # Explicit no sky bypass; no negative association write.
        run(0x132220,[CONTEXT]);queue=QUEUES+0x200;n=struct.unpack('<H',read(queue+4,2))[0]
        flags,ids,other,associations=struct.unpack('<4I',read(queue+8,16));result=[n]
        for i in range(n):result.extend([uint(read(ids+i*4,4)),struct.unpack('<H',read(flags+i*2,2))[0],uint(read(other+i*4,4)),struct.unpack('<H',read(associations+i*2,2))[0]])
        na=struct.unpack('<H',read(CONTEXT+0xa5e,2))[0];result.extend([na,*words(read(CONTEXT+0xa60,na*16))])
        for address,raw in protected: assert read(address,len(raw))==raw,hex(address)
        expected.append(result);rows.append(' '.join(f'{v:x}'for v in serialize(case)))
        case_reports.append(dict(label=label,owners=n,associations=na))
    assert calls[0x1cac60]==0 and calls[0x16b840]==0
    failures=[]
    if args.binary:
        result=subprocess.check_output([str(args.binary.resolve()),'--probe',str(args.snapshot_fixture.resolve())],input='\n'.join(rows)+'\n',text=True)
        native=[list(int(v,16)for v in row.split())for row in result.splitlines()];assert len(native)==len(expected)
        failures=[dict(label=cases[i][0],original=a,native=b)for i,(a,b)in enumerate(zip(expected,native))if a!=b]
    args.fixture.write_text('# Fresh untouched132220 over restored source202owner/fullspatialpools; no re-registration\n'+''.join(row+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(rows,expected)))
    report=dict(source_executable_sha256=PIN,snapshot=snapshot.report['snapshot'],clock_tick=snapshot.report['clock_tick'],
        physical_ram_sha256=hashlib.sha256(snapshot.ram).hexdigest(),snapshot_fixture_sha256=hashlib.sha256(args.snapshot_fixture.read_bytes()).hexdigest(),
        object_count=len(snapshot.report['objects']),object_record_bytes=sum(o['size']for o in snapshot.report['objects']),
        complete_owner_records=[dict(handle=o['handle'],address=o['address'],size=o['size'],sha256=hashlib.sha256(snapshot.file(f"object-{o['handle']}.bin")).hexdigest())for o in snapshot.report['objects']],
        authored_clusters=cluster_count,head_capacity=512,raw_bank_hashes=hashes,
        actual_visibility=dict(context_address='00547F88',set_address=f'{actual_set_address:08X}',groups=groups,clusters=count,frustums=frustums,retained_associations=prior_count),
        cases=len(cases),words=sum(map(len,expected)),case_results=case_reports,original_calls={f'{a:08X}':calls[a]for a in source_entries},
        verified_original_instruction_addresses=len(verified_instructions),mapped_captured_pages=len(original_pages),
        failures=failures,native_binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest()if args.binary else None,
        boundary='Fresh ordinary mode0/auxNULL pass over actual complete snapshot records and list order. Retained current frusta or explicit synthetic frusta. Flags10 explicitly skip light-volume services; sky bypass explicitly disabled. No registration/allocator/target lookup hooks. Saved41collector is not a same-phase expected result. Unqueried55..511 fog capacity is explicitly zero. Guest never advanced or modified.')
    provenance=dict(source_executable_sha256=PIN,snapshot=snapshot.report['snapshot'],snapshot_metadata=snapshot.report['snapshot_metadata'],
        clock_tick=snapshot.report['clock_tick'],cr3=f'{snapshot.cr3:08X}',fpcw='023F',mxcsr='1F80',
        physical_ram_sha256=report['physical_ram_sha256'],raw_snapshot_fixture_sha256=report['snapshot_fixture_sha256'],
        cases_fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        context_sha256=hashlib.sha256(actual_context).hexdigest(),visibility_set_sha256=hashlib.sha256(actual_set).hexdigest(),
        snapshot_fixture_size=args.snapshot_fixture.stat().st_size,
        complete_owner_records=report['complete_owner_records'],raw_bank_hashes=hashes,
        case_labels=[label for label,_ in cases],original_calls=report['original_calls'],
        words=report['words'],boundary=report['boundary'],
        recreation='tools/differential_source_object_admission_snapshot.py --binary build-mp-lightmap/source_object_admission_snapshot_tests; requires the immutable restored64MiB RAM/export.json plus originalXBE. Binary fixture is local original evidence and intentionally ignored.')
    args.fixture.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ['raw_bank_hashes','complete_owner_records','case_results','boundary','failures']},indent=2));print(json.dumps(failures[:1],indent=2));assert not failures
if __name__=='__main__': main()
