#!/usr/bin/env python3
"""Offline physical unchanged TU1.5 cinematic reset/ack reference.

Only declared input memory and registers are installed. No original instruction
is patched, no callee return is supplied and no stopped prefix is resumed. The
original ELF, observer, raw banks and compiler output are scratch-owned.
"""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x1fcb0,0x1fce0)
POINTER,TARGET,BANK=0x515ec0,0x4c1eb0,0x1000100


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def metadata(image):
    code=_Code(image,'.text');sites=[];pins={}
    registers={name:n for n,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
    for entry in ENTRIES:
        body=complete_body(code,entry);listing=[]
        for pc in sorted(body.code):
            instruction=code.at(pc);operands=[]
            listing.append(f'{pc:x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
            for operand in instruction.operands:
                if operand.type!=X86_OP_MEM or instruction.mnemonic=='lea':
                    continue
                base=instruction.reg_name(operand.mem.base)
                index=instruction.reg_name(operand.mem.index)
                operands.append(dict(base=registers[base] if base else -1,
                    index=registers[index] if index else -1,scale=operand.mem.scale,
                    displacement=operand.mem.disp&0xffffffff,bytes=operand.size,
                    read=bool(operand.access&CS_AC_READ),write=bool(operand.access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,bytes=instruction.bytes.hex(),operands=operands))
        pins[hex(entry)]=dict(sites=len(body.code),listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
    return dict(sites=sites,stop=geometry.STOP,owners=pins)


def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir()
    section=image.section('.text');(owned/'text.bin').write_bytes(image.section_bytes(section))
    (owned/'metadata.json').write_text(json.dumps(meta))
    (owned/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{owned/'text.bin'}"
.section .bank,"aw"
.space 8192
.section .target,"aw"
.space 8192
.section .binding,"aw"
.space 8192
.section .stack,"aw"
.space 65536
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''')
    (owned/'link.ld').write_text(f'''ENTRY(_start)
SECTIONS {{
 .original 0x{section.va:x}:{{*(.original)}}
 .bank 0x1000000:{{*(.bank)}}
 .target 0x4c1000:{{*(.target)}}
 .binding 0x515000:{{*(.binding)}}
 .stack 0x{geometry.STACK:x}:{{*(.stack)}}
 .stop 0x{geometry.STOP:x}:{{*(.stop)}}
 .bridge 0x70000000:{{*(.bridge)}}
}}
''')
    commands=(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],
        ['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_geometry_tu15.cpp'),'-o',str(owned/'observer')])
    for command in commands:
        result=subprocess.run(command,text=True,capture_output=True,check=False)
        if result.returncode:
            raise RuntimeError(result.stderr)
    return owned


def cases():
    rows=[]
    def make(entry,present=True,target=0x12345678,dirty=0,bank=BANK,name=None):
        n=len(rows)
        regions={bank:bytearray((k*13+n*7+3)&255 for k in range(0x3fc))}
        def covered(address,size):
            return any(first<=address and address+size<=first+len(data) for first,data in regions.items())
        if not covered(TARGET,16):regions[TARGET]=bytearray([0x53]*16)
        if not covered(POINTER,4):regions[POINTER]=bytearray(4)
        def put(address,data):
            for first,region in regions.items():
                if first<=address and address+len(data)<=first+len(region):
                    region[address-first:address-first+len(data)]=data;return
            raise ValueError('Input outside declared bank')
        put(POINTER,struct.pack('<I',bank if present else 0))
        put(TARGET,struct.pack('<I',target));put(TARGET+13,bytes([dirty]))
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+(n%4)*4
        rows.append(dict(id=n,name=name or f'{entry:x}_{present}_{target:x}_{dirty}_{bank:x}',
            entry=entry,gp=gp,matrix=0,source=0,
            banks=[dict(address=first,before=data.hex()) for first,data in sorted(regions.items())],
            before_fp=[0x27f|((n%4)<<10),(n%8)<<11,0x1f80|((n%4)<<13)],
            live_x87=[struct.pack('<QH',0x8000000000000000,0x3fff).hex()] if n%2 else []))
    for entry in ENTRIES:
        for present in (False,True):
            for target in (0,0x12345678):
                for dirty in (0,1,2,255):make(entry,present,target,dirty)
        for bank in (BANK+1,BANK+3,0x4c1d00,0x515d00):
            make(entry,bank=bank,name=f'{entry:x}_alias_alignment_{bank:x}')
        make(entry,name=f'fault_seed_{entry:x}')
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists():raise ValueError('Preserve existing independent original fixture')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected authenticated English TU1.5')
    image=Xbe(NEW_DEFAULT);meta=metadata(image);owned=prepare(image,meta,scratch)
    rows=geometry.physical(owned,cases(),'complete');prefix=[]
    for seed in (row for row in rows if row['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            indices=range(len(events)) if phase=='before_read' else sorted({n for n in (0,1,127,254,255) if n<len(events)})
            for ordinal in indices:
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),
                    fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix')
    visited=set().union(*(set(row['executed_sites']) for row in rows))
    uncovered=[site['instruction'] for site in meta['sites'] if site['instruction'] not in visited]
    if uncovered:raise ValueError(('Original paths not executed',uncovered))
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],
        instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    raw=json.dumps(fixture,separators=(',',':')).encode()
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(raw,mtime=0))
    report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,owners=meta['owners'],cases=len(rows),
        complete_calls=sum(row['completed'] for row in rows),fault_prefixes=len(prefix),executed_sites=len(visited),
        uncovered_sites=uncovered,fixture=str(args.output),fixture_bytes=args.output.stat().st_size,
        fixture_sha256=sha(args.output),original_instructions=sum(row['original_instructions'] for row in rows),
        owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),
            'tools/observe_native_geometry_tu15.cpp':sha(ROOT/'tools/observe_native_geometry_tu15.cpp')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f"{len(rows)} complete/prefix original comparisons; retained reference {args.output.stat().st_size} bytes",flush=True)


if __name__=='__main__':
    main()
