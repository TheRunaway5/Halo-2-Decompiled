#!/usr/bin/env python3
"""Complete unchanged12D6D0 and its actual359E0/1365A0/13E3A0 paths.

The first42 inputs are actual metadata/partition/address/header observations
from the frozen168-call retained allocator report. All remaining metadata,
flags, types, dimensions, level counts and addresses are explicit CPU controls.
This pure producer proves five pending header words, not resource readiness,
I/O outcomes, kernel completion, renderer use or a historical producer order.
No original instruction, service, arithmetic result or table is replaced.
"""
from pathlib import Path
import argparse, collections, hashlib, json, random, struct, subprocess, sys, tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
ALLOCATOR_PIN='7b4881fbcfc8aecd683e7e73156ea0d743fef3d043b91c11c4de15f0e1a3d146'
RECORD,OUTPUT,STACK,STOP=0x2100000,0x2102000,0x21ffe00,0x3000000
RANGES=[(0x12d6d0,0x12d8a4),(0x359e0,0x35a91),(0x1365a0,0x1365f8),(0x13e3a0,0x13e3b9)]
FLAGS=[0,2,0x10,0x12,0x20,0x22,0x30,0x32]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def packed(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def field(raw,offset):return struct.unpack_from('<H',raw,offset)[0]
def metadata(raw,partition):
    return [field(raw,4),field(raw,6),field(raw,12),field(raw,20),raw[8],
        field(raw,10),field(raw,14),partition,struct.unpack_from('<I',raw,64+partition*4)[0]]
def record_for(row):
    raw=bytearray(116)
    for index,offset in [(0,4),(1,6),(2,12),(3,20),(5,10),(6,14)]:
        struct.pack_into('<H',raw,offset,row[index])
    raw[8]=row[4]
    struct.pack_into('<3I',raw,64,0x13200000,0x24300000,0x35400000)
    struct.pack_into('<I',raw,64+row[7]*4,row[8])
    return bytes(raw)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--asan-binary',type=Path)
    parser.add_argument('--write-fixture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
    report_path=ROOT/'analysis/multiplayer-shading/source-bitmap-allocator-snapshot-original.json'
    report_raw=report_path.read_bytes();assert sha(report_raw)==ALLOCATOR_PIN
    inspection_path=ROOT/'analysis/multiplayer-shading/source-bitmap-pending-header-independent-review.json'
    inspection_raw=inspection_path.read_bytes();inspection=json.loads(inspection_raw)
    assert inspection['source_xbe_sha256']==PIN
    for region in inspection['regions']:
        assert sha(image.read(region['start'],region['end']-region['start']))==region['sha256']
    for table in inspection['tables']:
        raw=struct.pack('<'+table['element_type']*len(table['words']),*table['words'])
        assert raw==image.read(table['address'],len(raw)) and sha(raw)==table['sha256']
    retained=json.loads(report_raw);assert len(retained['records'])==168 and retained['candidates']==42
    groups=collections.defaultdict(list)
    for entry in retained['records']:groups[(entry['bitmap_record'],entry['partition'])].append(entry)
    assert len(groups)==42 and all(len(group)==4 for group in groups.values())
    cases=[];actual_inputs=[]
    for (owner,partition),group in groups.items():
        observed=[]
        for entry in group:
            raw=bytes.fromhex(entry['after_record_hex']);assert len(raw)==116
            row=metadata(raw,partition);header=words(bytes.fromhex(entry['image_row_hex'])[0x14:0x28])
            assert len(header)==5;observed.append((row,header))
        assert all(observation==observed[0]for observation in observed)
        row,header=observed[0];raw=bytes.fromhex(group[0]['after_record_hex'])
        cases.append(dict(row=row,record=raw,label='actual_retained_partition',observed=header))
        actual_inputs.append(dict(owner=owner,partition=partition,metadata=[f'{v:08X}'for v in row],
            source_record_hex=raw.hex(),observed_header=[f'{v:08X}'for v in header],
            phase_label='metadata/address/header from retained request replay; not a live completion'))
    def add(width=64,height=32,format=9,maximum=6,depth=4,type=0,flags=0,partition=0,
            address=0x80abcdef,label='explicit_control'):
        row=[width&65535,height&65535,format&65535,maximum&65535,depth&255,
            type&65535,flags&65535,partition,address&0xffffffff]
        cases.append(dict(row=row,record=record_for(row),label=label,observed=None))
    for format in range(24):
        for type in [0,1,2]:
            for flags in FLAGS:
                for partition in range(3):
                    add(format=format,type=type,flags=flags,partition=partition,label='controlled_format_type_flags_partition')
    for type in [3,0x7fff,0x8000,0xffff]:
        for format in range(24):
            for flags in [0,0x10]:
                for partition in range(3):
                    add(format=format,type=type,flags=flags,partition=partition,label='controlled_raw_type')
    wide=[-32768,-32767,-1,0,1,2,3,4,7,8,16,127,255,256,16384,32764,32765,32766,32767]
    thin=[-128,-127,-1,0,1,2,3,4,7,8,16,64,127]
    for dimension,values in [('width',wide),('height',wide),('depth',thin)]:
        for value in values:
            for format in [0,9,10,11,14,15,16,19,20,21,23]:
                for flags in FLAGS:
                    for partition in range(3):
                        dimensions=dict(width=64,height=32,depth=1);dimensions[dimension]=value
                        add(**dimensions,format=format,flags=flags,partition=partition,
                            label='controlled_signed_zero_dimensions_and_pitch_rounding')
    for maximum in [-32768,-32767,-2,-1,0,1,2,3,7,15,32767]:
        for width,height,depth in [(64,32,1),(0,0,0),(3,4,1),(1,1,1),(2,2,0),(4,4,1)]:
            for format in [9,14,15,16]:
                for flags in [0,0x10]:
                    for partition in range(3):
                        add(width,height,format,maximum,depth,flags=flags,partition=partition,
                            label='controlled_signed_wrapped_maximum_levels')
    for address in [0,0x0fffffff,0x10000000,0x80000000,0xffffffff]:
        for flags in [0,0x10]:
            for partition in range(3):
                add(flags=flags,partition=partition,address=address,label='controlled_address_mask')
    randomizer=random.Random(0x12d6d0)
    for _ in range(2048):
        add(width=randomizer.randrange(-32768,32768),height=randomizer.randrange(-32768,32768),
            format=randomizer.randrange(24),maximum=randomizer.randrange(-32768,32768),
            depth=randomizer.randrange(-128,128),type=randomizer.randrange(65536),
            flags=randomizer.randrange(65536),partition=randomizer.randrange(3),
            address=randomizer.randrange(1<<32),label='controlled_full_narrow_metadata')
    assert len(cases)==19472,dict(collections.Counter(case['label']for case in cases))
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
    u.mem_map(RECORD,0x100000);u.mem_map(STOP,4096)
    seen,calls,branches={},collections.Counter(),collections.Counter()
    def code(g,pc,size,data):
        if pc==STOP:g.emu_stop();return
        assert any(start<=pc and pc+size<=end for start,end in RANGES),f'unknown original call {pc:08X}'
        if pc not in seen:
            assert bytes(g.mem_read(pc,size))==image.read(pc,size),f'changed original instruction {pc:08X}'
            seen[pc]=size
        else:assert seen[pc]==size
        if pc in [start for start,end in RANGES]:calls[f'{pc:08X}']+=1
        if pc in [0x12d6f2,0x12d711,0x12d74e,0x12d7e0,0x1365b8,0x1365c3,
                0x35a2a,0x35a59,0x35a8b,0x13e3b0]:branches[f'{pc:08X}']+=1
    def writing(g,access,address,size,value,data):
        assert (STACK-0x100<=address and address+size<=STACK+4)or(OUTPUT<=address and address+size<=OUTPUT+20),\
            f'original wrote outside isolated stack/output {address:08X}'
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
    source=[]
    for ordinal,case in enumerate(cases):
        row,raw=case['row'],case['record'];u.mem_write(RECORD,raw);u.mem_write(OUTPUT,bytes([0xa5])*20)
        u.mem_write(STACK-0x100,bytes(0x110));u.mem_write(STACK,packed([STOP,row[7],OUTPUT]))
        for register in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
                UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(register,0)
        u.reg_write(UC_X86_REG_EAX,RECORD);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(0x12d6d0,STOP,count=2000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+12,ordinal
        assert bytes(u.mem_read(RECORD,116))==raw,f'original changed input{ordinal}'
        header=words(bytes(u.mem_read(OUTPUT,20)))
        if case['observed']is not None:assert header==case['observed'],(ordinal,'retained observation differs')
        source.append(header)
    encoded=[' '.join(f'{v:08X}'for v in case['row'])for case in cases]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',
        text=True,capture_output=True,check=True)
    result=[[int(value,16)for value in line.split()]for line in native.stdout.splitlines()]
    assert len(result)==len(source) and all(len(row)==5 for row in result)
    assert result==source,[(index,cases[index]['row'],old,new)
        for index,(old,new)in enumerate(zip(source,result))if old!=new][:8]
    args.write_fixture.write_text('# Complete unchanged12D6D0. width height format mipmapCount depth type flags partition levelAddress | Common Data Lock Format Size; hexadecimal raw narrow fields\n'+
        '\n'.join(f'{line} | '+ ' '.join(f'{v:08X}'for v in header)for line,header in zip(encoded,source))+'\n')
    normal=subprocess.run([str(args.binary.resolve()),str(args.write_fixture.resolve())],text=True,capture_output=True,check=True)
    sanitizer=None
    if args.asan_binary:
        import os
        environment=os.environ.copy();environment['ASAN_OPTIONS']='detect_leaks=1'
        outcome=subprocess.run([str(args.asan_binary.resolve()),str(args.write_fixture.resolve())],
            text=True,capture_output=True,check=True,env=environment)
        assert outcome.stdout==normal.stdout and not outcome.stderr
        sanitizer=dict(binary_sha256=sha(args.asan_binary.read_bytes()),validation=outcome.stdout.strip(),
            address_sanitizer=True,undefined_behavior_sanitizer=True,leak_check=True)
    rejections=[]
    with tempfile.TemporaryDirectory(prefix='halo2-pending-header-')as directory:
        for label,content in [('missing',None),('empty',''),
                ('truncated','\n'.join(args.write_fixture.read_text().splitlines()[:-1])+'\n')]:
            path=Path(directory)/(label+'.tsv')
            if content is not None:path.write_text(content)
            outcome=subprocess.run([str(args.binary.resolve()),str(path)],text=True,capture_output=True)
            assert outcome.returncode!=0
            rejections.append(dict(kind=label,diagnostic=outcome.stderr.strip()))
    output=dict(boundary=__doc__,source_xbe_sha256=PIN,allocator_report_sha256=ALLOCATOR_PIN,
        independent_inspection=dict(path=str(inspection_path.relative_to(ROOT)),sha256=sha(inspection_raw),
            authority='read-only inspection corroboration; this whole original execution is the result authority'),
        cases=len(cases),exact_header_words=len(cases)*5,original_record_bytes_preserved=len(cases)*116,
        actual_cases=len(actual_inputs),explicit_control_cases=len(cases)-len(actual_inputs),
        corpus_groups=dict(collections.Counter(case['label']for case in cases)),actual_inputs=actual_inputs,
        admission=dict(format='signed16 within0..23',partition='uint32 within0..2',
            width_height='all signed16',depth='all signed8',mipmap_count='all signed16',type_flags='all uint16'),
        source_instruction_count=len(seen),source_instruction_sha256=sha(b''.join(packed([pc])+image.read(pc,size)for pc,size in sorted(seen.items()))),
        original_routines=[dict(entry=f'{start:08X}',bytes=end-start,sha256=sha(image.read(start,end-start)))for start,end in RANGES],
        instruction_verification='Each reached instruction verified once; original writes restricted to isolated stack/output, input patches isolated from all code.',
        source_calls=dict(calls),source_branch_hits=dict(branches),
        original_tables=[dict(address=f'{address:08X}',bytes=length,sha256=sha(image.read(address,length)))
            for address,length in [(0x450768,96),(0x4507c8,96),(0x453550,48)]],
        tool_sha256=sha(Path(__file__).read_bytes()),
        helper_sha256=sha((ROOT/'src/source_bitmap_pending_header.cpp').read_bytes()),
        header_sha256=sha((ROOT/'include/halo2/source_bitmap_pending_header.hpp').read_bytes()),
        test_sha256=sha((ROOT/'tests/source_bitmap_pending_header_tests.cpp').read_bytes()),
        fixture_sha256=sha(args.write_fixture.read_bytes()),native_binary_sha256=sha(args.binary.read_bytes()),
        native_validation=normal.stdout.strip(),sanitizer_validation=sanitizer,incomplete_fixture_rejections=rejections,
        fpcw='023F',mxcsr='1F80',services_replaced=[],source_instruction_patches=[],
        resource_readiness_supplied=False,historical_completion_claimed=False,live_activation=False)
    args.output.write_text(json.dumps(output,indent=2)+'\n')
    args.write_fixture.with_suffix('.provenance.json').write_text(json.dumps(output,indent=2)+'\n')
    print(json.dumps({key:value for key,value in output.items()if key not in ['actual_inputs']},indent=2))

if __name__=='__main__':main()
