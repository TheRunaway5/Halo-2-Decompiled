#!/usr/bin/env python3
"""Compare native camera clear RGB with unchanged original sky/fog producers.

This runs14B360, complete12E5E0 and the2B790 authored-override copy. No
aggregation instructions are replaced. Forced cluster samples are metadata
probes, not recorded gameplay or proof of full framebuffer parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_MXCSR, UC_X86_REG_FPCW)

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache

STATE,EYE,DIRECTION,CONTROL,STACK,STOP=0x2100000,0x2101000,0x2101100,0x2101200,0x21fff00,0x3000000

def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def invoke(g,address,args=(),registers=()):
    g.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
    g.reg_write(UC_X86_REG_ESP,STACK)
    g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    g.reg_write(UC_X86_REG_FPCW,0x027f)
    for reg,value in registers:g.reg_write(reg,value)
    g.emu_start(address,STOP,count=1000000)
    assert g.reg_read(UC_X86_REG_EIP)==STOP,hex(g.reg_read(UC_X86_REG_EIP))

def setup(c):
    image=(ROOT/'assets/disc/default.xbe').read_bytes()
    assert hashlib.sha256(image).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    for i in range(count):
        _,address,_,offset,size=struct.unpack_from('<5I',image,table-base+i*56)
        g.mem_write(address,image[offset:offset+size])
    g.mem_map(STATE,0x100000);g.mem_map(0x2300000,0x100000);g.mem_map(STOP,4096)
    invoke(g,0x377110)
    mapped=c.magic&~4095;size=(c.magic+len(c.data)-mapped+4095)&~4095
    g.mem_map(mapped,size);g.mem_write(c.magic,bytes(c.data))
    _,scnr=c.tag(c.scenario,'scnr');bsp=c.block(scnr+0x210,68)[0]
    file,size,address=struct.unpack_from('<3I',c.data,bsp)
    g.mem_write(address,bytes(c.data[file:file+size]))
    g.mem_write(0x4e0350,struct.pack('<I',c.magic+scnr))
    g.mem_write(0x4e0348,struct.pack('<I',address+16))
    g.mem_write(0x4e3b44,struct.pack('<I',0x2300000))
    for tag in c.tags.values():
        g.mem_write(0x2300000+(tag['datum']&65535)*16+8,struct.pack('<I',tag['address']))
    return g,file,address

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--map',action='append',help='Optional map stems for a minimal loop')
    parser.add_argument('--compare-sky-density',action='store_true',help='Also compare original post-tail bank+58')
    args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    roots=[ROOT/'assets/disc/maps',ROOT/'assets/map-pack/maps']
    paths=[p for root in roots for p in sorted(root.glob('*.map'))
        if args.map is None or p.stem in args.map]
    pins=[ROOT/'assets/disc/default.xbe',Path(__file__).resolve(),ROOT/'tools/audit_rendering.py',
        args.native_binary.resolve()]
    before={str(p):{'bytes':p.stat().st_size,'sha256':digest(p)} for p in pins}
    cases=[];maps=[]
    for path in paths:
        c=Cache(path)
        if c.scenario==0xffffffff:c.close();continue
        _,scnr=c.tag(c.scenario,'scnr')
        if c.u16(scnr+0x10)!=1:c.close();continue
        before[str(path)]={'bytes':path.stat().st_size,'sha256':digest(path)}
        g,file,address=setup(c);magic=address-file
        clusters=c.block(file+16+156,176,magic)
        map_cases=[]
        for cluster,p in enumerate(clusters):
            bounds=struct.unpack_from('<6f',c.data,p+0x54)
            center=[(bounds[i]+bounds[i+1])*.5 for i in (0,2,4)]
            for offset in (0,-40,40):
                eye=list(struct.unpack('<3f',struct.pack('<3f',center[0],center[1],center[2]+offset)))
                g.mem_write(EYE,struct.pack('<3f',*eye));g.mem_write(DIRECTION,struct.pack('<3f',1,0,0))
                g.mem_write(CONTROL,bytes(32))
                invoke(g,0x14b360,registers=[(UC_X86_REG_ECX,cluster),
                    (UC_X86_REG_EDX,CONTROL),(UC_X86_REG_EBX,CONTROL+4),(UC_X86_REG_EDI,CONTROL+8)])
                model=g.reg_read(UC_X86_REG_EAX)&255
                control=bytes(g.mem_read(CONTROL,20))
                invoke(g,0x12e5e0,(EYE,DIRECTION,model,1),
                    [(UC_X86_REG_EAX,STATE),(UC_X86_REG_ECX,cluster)])
                state=bytes(g.mem_read(STATE,0x120))
                # Exact copy at2B888..2B8A4, after original fog selection.
                rgb=control[8:20] if control[4] else state[4:16]
                row={'map':path.name,'cluster':cluster,'eye':eye,'sky_model_control':model,
                    'authored_override':control[4],'sky_ordinal':struct.unpack_from('<i',control)[0],
                    'original_rgb_bits':list(struct.unpack('<3I',rgb)),
                    'original_sky_density_bits':struct.unpack_from('<I',state,0x58)[0],
                    'original_state_hex':state.hex()}
                cases.append(row);map_cases.append(row)
        maps.append(path.name)
        print(path.name,len(map_cases),'samples',flush=True);c.close()
    assert cases,'No multiplayer inputs'
    inputs=out/'inputs.txt'
    inputs.write_text(''.join(' '.join([s['map'],*map(str,s['eye']),str(s['cluster'])])+'\n' for s in cases))
    command=[str(args.native_binary.resolve()),str(roots[0]),str(roots[1]),str(inputs)]
    if args.compare_sky_density:command.append('--sky-density')
    result=subprocess.run(command,cwd=ROOT,text=True,capture_output=True,check=True)
    (out/'native.txt').write_text(result.stdout)
    rows=result.stdout.splitlines();assert len(rows)==len(cases)
    mismatches=[];density_mismatches=[]
    for s,line in zip(cases,rows):
        row=line.split();assert row[:2]==[s['map'],str(s['cluster'])] and len(row)==(6 if args.compare_sky_density else 5)
        native=list(map(lambda v:int(v,16),row[2:5]))
        s['native_rgb_bits']=native
        if native!=s['original_rgb_bits']:
            mismatches.append({k:s[k] for k in ('map','cluster','eye','sky_model_control',
                'authored_override','original_rgb_bits','native_rgb_bits')})
        if args.compare_sky_density:
            s['native_sky_density_bits']=int(row[5],16)
            if s['native_sky_density_bits']!=s['original_sky_density_bits']:
                density_mismatches.append({k:s[k] for k in ('map','cluster','eye',
                    'original_sky_density_bits','native_sky_density_bits')})
    after={p:{'bytes':Path(p).stat().st_size,'sha256':digest(Path(p))} for p in before}
    assert before==after,'Source or executable changed during comparison'
    (out/'fixtures.json').write_text(json.dumps(cases,indent=2)+'\n')
    report={'passed':not mismatches and not density_mismatches,'maps':maps,'samples':len(cases),
        'rgb_mismatches':mismatches,'sky_density_compared':args.compare_sky_density,'sky_density_mismatches':density_mismatches,
        'inputs_before':before,'inputs_after':after,'inputs_unchanged':before==after,
        'original_instruction_replacements':[],
        'floating_point_context':{'mxcsr':'00001F80','x87_control_word':'027F'},
        'framebuffer_parity':False,
        'scope':'Complete original14B360/12E5E0 with forced metadata cluster and authored override. RGB bits only; no clear scheduler or GPU equality claim.'}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'passed':report['passed'],'maps':len(maps),'samples':len(cases),
        'rgb_mismatches':len(mismatches),'sky_density_mismatches':len(density_mismatches)}))
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
