#!/usr/bin/env python3
"""Unchanged TU1.5 refresh/attached/direct-sound request owners, declared services.

The lower services are explicitly controlled ABI inputs, not proof of allocation
or audio parity. Run through tools/run_test.py; retain only the bounded fixture.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_XMM0, UC_X86_REG_EFLAGS
from oracle_weapon_effect_dispatch_tu15 import GP, STACK, STOP, NONE, pack
from weapon_reference_versions import images

POOL, BODIES, POSITION, TAGS, DEFINITION = 0x1000000, 0x1002000, 0x1004000, 0x1006000, 0x1008000
SERVICES = {0x17a1a0:1, 0xbbe00:0, 0x1791c0:0, 0x178e30:1,
            0x18c090:2, 0x18be00:0, 0x18c130:6, 0x18cdd0:1,
            0x17b130:0, 0x17dce0:3, 0x18d540:1}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--restart',action='store_true',help='Separate bounded17A1A0 reference')
    args=p.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _, image, pin=next(x for x in images() if x[0]=='TU1.5')
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    g.mem_map(0x10000,0x800000)
    for section in image.sections: g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(0x1000000,0x100000);g.mem_map(0x2000000,0x20000);g.mem_map(STOP,4096)
    instructions={};calls=[];stores=[];active=None

    def code(g,at,size,_):
        if at in SERVICES and at!=active['entry']:
            sp=g.reg_read(GP[4]);gp=[g.reg_read(r) for r in GP]
            writes=[]
            if at in (0xbbe00,0x18c090):
                writes=[[gp[6], active['service_bytes'][:(12 if at==0xbbe00 else 20)*2]]]
            elif at==0x1791c0:
                writes=[[gp[0],active['service_bytes'][:56]]]
            elif at==0x17a1a0:
                writes=[[BODIES+3*0x70+8,active['service_bytes'][:8]]]
            calls.append(dict(target=at,gp=gp,
                xmm=[g.reg_read(UC_X86_REG_XMM0+n).to_bytes(16,'little').hex() for n in range(8)],
                stack=bytes(g.mem_read(STACK-512,544)).hex(),writes=writes))
            for address,raw in writes:g.mem_write(address,bytes.fromhex(raw))
            for r,v in zip(GP[:3],active['clobber']):g.reg_write(r,v)
            g.reg_write(UC_X86_REG_XMM0,int.from_bytes(bytes.fromhex(active['service_xmm0']),'little'))
            ret=struct.unpack('<I',g.mem_read(sp,4))[0]
            g.reg_write(GP[4],sp+4+SERVICES[at]*4);g.reg_write(UC_X86_REG_EIP,ret)
            return
        raw=bytes(g.mem_read(at,size))
        if raw!=image.read(at,size):raise ValueError('Original instructions changed')
        instructions[f'{at:08x}']=raw.hex()
    def write(g,access,at,size,value,_):
        stores.append([g.reg_read(UC_X86_REG_EIP),at,size,value & ((1<<(8*size))-1)])
    g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write)
    rng=random.Random(0x179610);rows=[]
    for entry in ((0x17a1a0,) if args.restart else (0x17dff0,0x179610,0x18be50,0x18c4b0)):
        for n in range(64 if entry==0x17dff0 else 16 if entry==0x18c4b0 else 32):
            gp=[rng.getrandbits(32) for _ in GP];gp[4]=STACK
            gp[3]=POSITION if n&1 else 0
            gp[7]=[NONE,0x12340003,0xffff0003,0x43210003][n%4]
            eflags=0x202 | (0x400 if n&16 else 0)
            xmm=[rng.randbytes(16).hex() for _ in range(8)]
            arguments=[0x34560007,rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)]
            memory=[[0x4f720c,POOL,4],[POOL+0x38,2 if n&4 else 8,4],
                    [POOL+0x24,0x70,4],[POOL+0x44,BODIES,4],
                    [BODIES+3*0x70,0xffff if n&8 else 0x1234,2],
                    [BODIES+3*0x70+2,0xff,1],[BODIES+3*0x70+8,rng.getrandbits(32),4],
                    [BODIES+3*0x70+4,0x34560008 if n&32 else arguments[0],4]]
            for j in range(3):memory.append([POSITION+j*4,rng.getrandbits(32),4])
            body=BODIES+3*(0x190 if args.restart else 0x70)
            if args.restart:
                arguments[0]=0x12340003
                memory.extend([[0x4f0414,TAGS,4],[POOL+0x44,BODIES,4],
                    [body+4,0x34560007,4],[body+2,0xff,1],[body+8,NONE if not n&4 else rng.getrandbits(32),4],
                    [TAGS+7*16+8,DEFINITION,4],[DEFINITION+0x20,NONE if n&1 else 0x13570008,4],
                    [DEFINITION+0x24,0xffff if n&2 else (0xfffe if n&16 else 2),2],
                    [body+0x7c+(-2 if n&16 else 2)*4,rng.getrandbits(32),4]])
            active=dict(entry=entry,gp=gp,xmm=xmm,eflags=eflags,memory=memory,
                arguments=arguments,clobber=[rng.getrandbits(32) for _ in range(3)],
                service_xmm0=rng.randbytes(16).hex(),service_bytes=rng.randbytes(28).hex())
            # Force both original AL branches while preserving arbitrary high bits.
            active['clobber'][0]=(active['clobber'][0]&0xffffff00)|(1 if n&2 else 0)
            if args.restart:active['clobber'][0]=rng.getrandbits(32) if n&8 else 0
            raw=rng.randbytes(544);g.mem_write(STACK-512,raw)
            g.mem_write(STACK,pack([STOP,*arguments]));active['initial_stack']=bytes(g.mem_read(STACK-512,544)).hex()
            for at,value,width in memory:g.mem_write(at,value.to_bytes(width,'little'))
            for reg,value in zip(GP,gp):g.reg_write(reg,value)
            for i,raw in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+i,int.from_bytes(bytes.fromhex(raw),'little'))
            g.reg_write(UC_X86_REG_EFLAGS,eflags)
            calls.clear();stores.clear();g.emu_start(entry,STOP,count=10000)
            if g.reg_read(UC_X86_REG_EIP)!=STOP:raise ValueError('Original did not complete')
            active.update(expected_gp=[g.reg_read(r) for r in GP],
                expected_xmm=[g.reg_read(UC_X86_REG_XMM0+i).to_bytes(16,'little').hex() for i in range(8)],
                expected_stack=bytes(g.mem_read(STACK-512,544)).hex(),calls=list(calls),stores=list(stores),
                expected_body=bytes(g.mem_read(body,12)).hex())
            if args.restart:active['body_address']=body
            rows.append(active)
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_REQUEST_REFERENCE_V1',original_sha256=pin,
        scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode()
    if len(data)>1024*1024:raise ValueError('Fixture exceeds 1MiB')
    args.fixture.write_bytes(data)
    report=dict(passed=True,cases=len(rows),original_sha256=pin,fixture_bytes=len(data),
        fixture_sha256=hashlib.sha256(data).hexdigest(),instructions=instructions,
        declared_service_boundaries=[hex(x) for x in SERVICES if any(c['target']==x for row in rows for c in row['calls'])],
        instruction_patches=[],full_output_parity=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['passed','cases','fixture_bytes','fixture_sha256']}))

if __name__=='__main__':main()
