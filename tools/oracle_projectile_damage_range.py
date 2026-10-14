#!/usr/bin/env python3
"""Execute complete original FBFD0 range initializer over real projectile tags.

Explicit object/header/tag-table backing and under-water flag are ABI inputs.
No call or instruction is intercepted. Confirms body170 is a damage-range
bound, separate from the projectile's velocity at definition17C.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--xbe',type=Path,action='append',required=True);ap.add_argument('--report',type=Path,required=True);args=ap.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');rows=[];originals=[]
    for path in args.xbe:
        x=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        anchors=[]
        anchor=bytes.fromhex('8b514425ffff00008d04408b4482088b08')
        for s in x.sections:
            raw=x.section_bytes(s);g.mem_write(s.va,raw);start=0
            while (at:=raw.find(anchor,start))>=0:
                start=at+1;entry=s.va+at-6
                if x.read(entry+0x3d,8)==bytes.fromhex('f30f118070010000'):anchors.append(entry)
        assert len(anchors)==1,anchors;entry=anchors[0]
        assert x.read(entry,2)==b'\x8b\x0d' and x.read(entry+0x17,2)==b'\x8b\x15'
        pool_global=struct.unpack('<I',x.read(entry+2,4))[0];tags_global=struct.unpack('<I',x.read(entry+0x19,4))[0]
        g.mem_map(0x1000000,0x10000);pool,headers,body,tags,definition,stack,stop=[0x1000000+i*0x1000 for i in range(7)]
        w=lambda a,v:g.mem_write(a,struct.pack('<I',v&0xffffffff))
        w(pool_global,pool);w(pool+0x44,headers);w(headers+8,body);w(tags_global,tags);w(tags+7*16+8,definition);w(body,7)
        values=[]
        for tag in cache.tags.values():
            if tag['group']!='proj' or tag['offset'] is None:continue
            p=tag['offset'];g.mem_write(definition,bytes(cache.data[p:p+0x184]))
            for flags,source in ((0,0x16c),(8,0x178)):
                w(body+4,flags);w(body+0x170,0xdeadbeef);w(stack,stop);g.reg_write(UC_X86_REG_ESP,stack);g.reg_write(UC_X86_REG_EAX,0x12340000)
                g.emu_start(entry,stop,count=1000);assert g.reg_read(UC_X86_REG_EIP)==stop
                result=struct.unpack('<I',bytes(g.mem_read(body+0x170,4)))[0];assert result==cache.u(p+source)
                values.append(result)
                if len(originals)==0:rows.append(dict(projectile=tag['name'],flags=flags,source_offset=hex(source),range_bits=f'{result:08x}',velocity=cache.f(p+0x17c)))
        if originals:assert values==originals[0]['values']
        originals.append(dict(image=str(path),sha256=hashlib.sha256(x.data).hexdigest(),entry=hex(entry),cases=len(values),values=values))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(scope=__doc__,images=originals,rows=rows),indent=2)+'\n');print('PASS original damage-range initialization',len(rows),'cases/image',len(originals),'images')
if __name__=='__main__':main()
