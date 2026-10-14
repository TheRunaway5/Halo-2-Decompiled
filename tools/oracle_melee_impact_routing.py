#!/usr/bin/env python3
"""Original D0340 melee presentation routing on real sword/SMG/material tags.

Object/header/tag backing, held-item parent/root and vectors are declared ABI
inputs. D0340, melee-only admission and root lookup execute unchanged. Only
material-effect188180 and object-sound creation are recorded lower boundaries;
no collision, mixer, particles, renderer or successful lower return is supplied.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--tu11',type=Path,required=True);ap.add_argument('--tu15',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);a=ap.parse_args()
    c=Cache(ROOT/'assets/disc/maps/lockout.map');glob=next(t for t in c.tags.values() if t['group']=='matg');materials=c.block(glob['offset']+0x150,0xb4)
    weapons=[t for t in c.tags.values() if t['group']=='weap' and t['name'].endswith(('\\energy_blade','\\smg'))]
    config=[(a.tu11,0xd0340,0x189bb0,0x18aa90,0x4e78d0,0x4eb114,0x4e791c),(a.tu15,0xd13d0,0x18af70,0x18be50,0x4ecbd0,0x4f0414,0x4ecc1c)]
    canonical=None;reports=[]
    for path,entry,foot_call,sound_call,pool_global,tags_global,globals_global in config:
        x=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
        g.mem_map(0x1000000,0x400000)
        pool,headers,unit,item,definition,global_,point,normal,stack,stop=[0x1000000+i*0x1000 for i in range(10)]
        stack=0x100f000;tags,rows,damage=0x1100000,0x1200000,0x1300000
        w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
        u=lambda p:struct.unpack('<I',bytes(g.mem_read(p,4)))[0]
        w(pool_global,pool);w(pool+0x44,headers);w(headers+8,unit);w(headers+20,item);w(tags_global,tags);w(globals_global,global_)
        w(unit,0);w(item,2);w(tags+8,definition+0x1000);w(tags+40,definition);w(unit+0x14,-1);w(item+0x14,0x12340000);g.mem_write(unit+0x212,b'\0');w(unit+0x218,0x12340001)
        g.mem_write(unit+0x70,struct.pack('<3f',1,0,0));g.mem_write(point,struct.pack('<3f',1.25,-2,.5));g.mem_write(normal,struct.pack('<3f',-1,0,0))
        w(global_+0x150,len(materials));w(global_+0x154,rows)
        g.mem_write(rows,b''.join(bytes(c.data[p:p+0xb4]) for p in materials));calls=[]
        def returned(pop):
            sp=g.reg_read(UC_X86_REG_ESP);g.reg_write(UC_X86_REG_EIP,u(sp));g.reg_write(UC_X86_REG_ESP,sp+4+pop)
        def hook(uc,address,size,_):
            sp=g.reg_read(UC_X86_REG_ESP)
            if address==foot_call:
                args=[u(sp+4+i*4) for i in range(8)]
                assert args[1:5]==[0x12340000,15,0,material] and args[7]==0x3f800000
                assert bytes(g.mem_read(args[5],12))==struct.pack('<3f',1,0,0)
                assert g.reg_read(UC_X86_REG_EAX)==point and g.reg_read(UC_X86_REG_ECX)==normal
                calls.append(('F',args[0]));returned(32)
            elif address==sound_call:
                assert [u(sp+4+i*4) for i in range(3)]==[0x12340000,0xffffffff,0x3f800000]
                calls.append(('S',g.reg_read(UC_X86_REG_EBX)));returned(12)
        g.hook_add(UC_HOOK_CODE,hook);outputs=[]
        for weapon in weapons:
            p=weapon['offset'];g.mem_write(definition,bytes(c.data[p:p+0x1fc]));datum=c.u(p+0x1c0)
            if datum!=0xffffffff:
                tag=c.tags[datum];g.mem_write(damage,bytes(c.data[tag['offset']:tag['offset']+0xb0]));w(tags+(datum&65535)*16+8,damage)
            for material in range(len(materials)):
                calls.clear();w(stack,stop)
                for i,v in enumerate((0x12340000,datum,material,point,normal)):w(stack+4+i*4,v)
                g.reg_write(UC_X86_REG_ESP,stack);g.emu_start(entry,stop,count=10000);assert g.reg_read(UC_X86_REG_EIP)==stop
                expected_foot=bool(c.u(p+0x12c)&(1<<24))
                assert bool(calls and calls[0][0]=='F')==expected_foot
                outputs.append((weapon['datum'],material,calls.copy()))
        if canonical is None:canonical=outputs
        else:assert outputs==canonical
        reports.append(dict(image=str(path),sha256=hashlib.sha256(x.data).hexdigest(),entry=hex(entry),cases=len(outputs),boundaries=[hex(foot_call),hex(sound_call)]))
    lines=['# Original D0340; weapon hex, material decimal, request count, ordered kind/tag hex pairs.']
    for weapon,material,calls in canonical:lines.append(f'{weapon} {material} {len(calls)} '+ ' '.join(f'{kind} {datum:08x}' for kind,datum in calls))
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('\n'.join(lines)+'\n');a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(scope=__doc__,images=reports,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),paired_results_identical=True),indent=2)+'\n');print('PASS original melee routing',len(canonical),'cases/image, paired exact')
if __name__=='__main__':main()
