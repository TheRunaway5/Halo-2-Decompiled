#!/usr/bin/env python3
"""Compare native clear plans with original132D0/14B60/14AC0 instructions.

Projection/resource binding and SDK GPU submission are recorded service
boundaries. All clear selection, mask conversion, viewport save/restore and
RGB packing instructions execute unchanged. No attachment alias is assumed.
"""
import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe,FUNCTIONS_CSV
SYNTH,HEADER,DEVICE,STACK,STOP=0x2100000,0x2100000,0x2101000,0x21fff00,0x3000000
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def pack(v):return struct.pack('<'+'I'*len(v),*v)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--native-binary',type=Path)
    parser.add_argument('--fixture',type=Path)
    parser.add_argument('--original-session',type=Path,
        default=ROOT/'analysis/multiplayer-shading/original-clear-scheduling/original-session')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
    assert x.read(0x45dbc0,4)==pack([0x3f800000])
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
    u.mem_map(SYNTH,0x100000);u.mem_map(STOP,4096);u.mem_write(0x407488,pack([DEVICE]))
    events=[];calls={};verified=set();viewports=[]
    boundaries={0x15370:0,0x12fa0:8,0x14bc0:8,0x3f8100:4,0x3f9fc0:24}
    def words(address,n):return list(struct.unpack('<'+'I'*n,u.mem_read(address,n*4)))
    def hook(_u,address,size,_data):
        if address in boundaries:
            calls[f'{address:08X}']=calls.get(f'{address:08X}',0)+1
            esp=u.reg_read(UC_X86_REG_ESP);argv=words(esp+4,boundaries[address]//4)
            if address==0x14bc0:
                events.append([0,argv[0],u.reg_read(UC_X86_REG_EAX)&255,argv[1],0,0,0])
            elif address==0x3f8100:
                viewport=words(argv[0],6);events.append([1,*viewport]);u.mem_write(DEVICE+0xee0,pack(viewport))
            elif address==0x3f9fc0:
                assert argv[:2]==[0,0]
                events.append([2,*argv[2:5],argv[5]&255,0,0]);viewports.append(words(DEVICE+0xee0,6))
            u.reg_write(UC_X86_REG_EIP,words(esp,1)[0]);u.reg_write(UC_X86_REG_ESP,esp+4+boundaries[address]);return
        if address not in verified:
            assert bytes(u.mem_read(address,size))==x.read(address,size),f'Original instruction changed at{address:08X}'
            verified.add(address)
    u.hook_add(UC_HOOK_CODE,hook)
    def run(address,argv,control,viewport,eax=0):
        events.clear();viewports.clear();u.mem_write(DEVICE+0xee0,pack(viewport))
        u.mem_write(STACK,pack([STOP,*argv]));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EAX,eax)
        u.reg_write(UC_X86_REG_FPCW,control);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        end=0x1339f if address==0x132d0 else STOP
        u.emu_start(address,end,count=10000);assert u.reg_read(UC_X86_REG_EIP)==end
        assert words(DEVICE+0xee0,6)==viewport,'Original temporary viewport not restored'
        return [e[:] for e in events]
    rows=[];cases=[];rng=random.Random(0x132d0)
    vp=[37,19,319,241,0x3e800000,0x3f400000]
    trace_path=args.original_session.resolve()/'verified-original-trace.json'
    header_path=args.original_session.resolve()/'clear-trace.json'
    original_trace=json.loads(trace_path.read_text())
    original_header=bytes.fromhex(json.loads(header_path.read_text())[0]['header'])
    actual_rgb=list(struct.unpack_from('<3I',original_header,12))
    def add_view(label,mode,temporary,clear,transition,control,rgb,viewport):
        header=bytearray(0x298);struct.pack_into('<H',header,2,mode);header[7]=temporary;struct.pack_into('<H',header,8,clear);struct.pack_into('<3I',header,12,*rgb)
        u.mem_write(HEADER,bytes(header));u.mem_write(0x4670cd,bytes([transition]))
        result=run(0x132d0,[HEADER],control,viewport)
        values=[mode,temporary,clear,transition,control,*rgb,*viewport,len(result),*[v for e in result for v in e]]
        rows.append('V '+label+' '+' '.join(f'{v:08X}'for v in values));cases.append({'id':label,'events':result})
        return result
    actual_clear=original_trace['observed_sdk_clears'][0]
    actual=add_view('actual-original-main-entry',0,0,2,0,0x023f,actual_rgb,actual_clear['viewport_words'])
    assert actual==[[0,0,1,0,0,0,0],[2,*actual_clear['arguments'][2:5],actual_clear['arguments'][5],0,0]]
    for index,(mode,temporary,clear,transition) in enumerate(itertools.product((0,1,2,0xffff,0x8000),(0,1,255),(0,2,0xffff),(0,1,255))):
        rgb=[rng.getrandbits(32)for _ in range(3)]
        add_view('branch-'+str(index),mode,temporary,clear,transition,(0x023f,0x027f,0x0f3f)[index%3],rgb,vp)
    for index,(flags,temporary,target) in enumerate(itertools.product([*range(128),0xffffffff,0x80000000],(0,1,255),(0,18,-1))):
        color=rng.getrandbits(32);depth=rng.getrandbits(32);stencil=rng.getrandbits(32)
        u.mem_write(0x485607,bytes([temporary]));u.mem_write(0x4858b8,pack([target&0xffffffff]))
        result=run(0x14b60,[color,depth,stencil],0x023f,vp,flags)
        values=[flags,color,depth,stencil,temporary,target&0xffffffff,*vp,len(result),*[v for e in result for v in e]]
        rows.append('C mask-'+str(index)+' '+' '.join(f'{v:08X}'for v in values));cases.append({'id':'mask-'+str(index),'events':result})
    fixture=out/'source_view_clear.tsv';fixture.write_text('\n'.join(rows)+'\n')
    if args.fixture:args.fixture.write_bytes(fixture.read_bytes())
    functions={int(r['va'],16):r for r in csv.DictReader(open(FUNCTIONS_CSV))}
    blocks={f'{a:08X}':digest_bytes(x.read(a,int(functions[a]['size'])))for a in (0x132d0,0x131ed0,0x14b60,0x14ac0)}
    report={'cases':len(cases),'passed':True,'original_xbe_sha256':PIN,'source_blocks':blocks,'verified_original_instruction_addresses':len(verified),
        'instruction_byte_patches':[],'recorded_service_boundaries':calls,'fixture_sha256':digest(fixture),'generator_sha256':digest(Path(__file__)),
        'oracle_inputs':{str(p):digest(p)for p in (trace_path,header_path,Path(FUNCTIONS_CSV),ROOT/'tools/reference/halo2-decompiled/tools/xbe.py')},
        'actual_original_main_entry_matches':True,'scope':'132D0 prefix before1339F, full14B60/14AC0 and131ED0; projection/bind/SDK services recorded, not implemented; no live lifecycle, alias or framebuffer parity claim','framebuffer_parity':False}
    if args.native_binary:
        r=subprocess.run([str(args.native_binary.resolve()),str(fixture)],cwd=ROOT,capture_output=True,text=True)
        (out/'native.log').write_text(r.stdout+r.stderr);report.update(passed=r.returncode==0,native_exit=r.returncode,native_binary=str(args.native_binary.resolve()),native_sha256=digest(args.native_binary))
    (out/'cases.json').write_text(json.dumps(cases,indent=2)+'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    return 0 if report['passed']else 1

def digest_bytes(raw):return {'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
if __name__=='__main__':raise SystemExit(main())
