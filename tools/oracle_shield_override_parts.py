#!/usr/bin/env python3
"""Run unchanged retail41040 through its override gate, stopping before scheduling.

Resident model/section/tag pointer banks are explicit supplied services. Actual
Lockout part types, original shader masks and shield masks are extracted from
read-only original assets. Synthetic mask/LOD cases exercise compatibility.
Complete1BF50 executes; no original instruction or result is replaced. This is
part admission/selection evidence, not collector lifetime, queue or pixel proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import ROOT, PIN
from oracle_mp_spawn import CACHE_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def packed(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]

def shader_masks(cache,datum):
    if datum==0xffffffff:return []
    _,raw=cache.tag(datum,'shad');definitions=cache.block(raw+32,124)
    assert len(definitions)==1
    _,template=cache.tag(cache.u32(definitions[0]),'stem');banks=cache.block(template+88,40)
    assert len(banks)==1
    return [cache.u32(lod+2)for lod in cache.block(banks[0],10)]

def actual_parts(cache,datum):
    _,model=cache.tag(datum,'mode')
    materials=[cache.u32(at+12)for at in cache.block(model+0x60,32)]
    for section in cache.block(model+0x24,0x5c):
        resources=cache.block(section+0x48,16)
        if not resources:continue
        pointer,size,header=[cache.u32(section+at)for at in (0x38,0x3c,0x44)]
        location,offset=pointer>>30,pointer&0x3fffffff
        source=cache.path if location==0 else cache.path.parent/('', 'mainmenu.map','shared.map','single_player_shared.map')[location]
        with source.open('rb')as stream:stream.seek(offset);raw=stream.read(size)
        assert len(raw)==size
        resource=resources[0]
        assert cache.u16(resource+4)==0 and cache.u16(resource+6)==72
        first,length=len(raw)-header-4+cache.u32(resource+12),cache.u32(resource+8)
        assert 0<=first<=len(raw)-length and length%72==0
        for at in range(first,first+length,72):
            kind,_,material=struct.unpack_from('<HHh',raw,at)
            shader=materials[material]if material>=0 else 0xffffffff
            yield kind,shader,shader_masks(cache,shader)

class Original:
    def __init__(self):
        image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
        self.u=u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
        for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
        self.base=0x2100000;u.mem_map(self.base,0x200000);self.stack=self.base+0x1fff00
        (self.table,self.model,self.section,self.header,self.part,self.material,
         self.original,self.override,self.opost,self.vpost,self.otemplate,self.vtemplate,
         self.obank,self.vbank,self.olods,self.vlods)=[self.base+i*0x1000 for i in range(16)]
        self.put(0x4e3b44,self.table);self.put(self.table+16,0x6d6f6465,1,self.model,0)
        for index,shader,template,post,bank,lods in (
            (2,self.original,self.otemplate,self.opost,self.obank,self.olods),
            (3,self.override,self.vtemplate,self.vpost,self.vbank,self.vlods)):
            self.put(self.table+index*16,0x73686164,index,shader,0)
            self.put(self.table+(index+2)*16,0x7374656d,index+2,template,0)
            self.put(shader+0x24,post);self.put(post,index+2);self.put(template+0x5c,bank)
        self.put(self.model+0x28,self.section);self.put(self.section+0x34,self.header);self.put(self.header+4,self.part)
        self.put(0x4ba13c,1);self.put(0x4ba144,0);self.put(self.material+0x28,bits(1))
        self.put(0x4670e4,bits(1));self.put(0x485b28,*([bits(1)]*7))
        self.put(0x4ba019,0);self.result=None;self.selected=None;self.choices=0
        def trace(_u,address,_size,_data):
            if address==0x1bf50:self.choices+=1
            if address==0x41167:
                self.selected=struct.unpack('<i',u.mem_read(u.reg_read(UC_X86_REG_ESP)+0x14,4))[0]
            if address in (0x412a3,0x41477):self.result=int(address==0x412a3);u.emu_stop()
        u.hook_add(UC_HOOK_CODE,trace)
    def put(self,address,*values):self.u.mem_write(address,packed(values))
    def run(self,kind,shader,selected,original,override):
        assert 0<=selected<len(override) and (shader==0xffffffff or selected<len(original))
        self.put(self.part,kind);self.put(self.material,0xffffffff if shader==0xffffffff else 2)
        for bank,lods,masks in ((self.obank,self.olods,original),(self.vbank,self.vlods,override)):
            self.put(bank,len(masks),lods)
            for i,mask in enumerate(masks):
                self.u.mem_write(lods+i*10,struct.pack('<HII',0,mask,bits(1 if i<selected else 0)))
        self.put(self.stack,0,0,0,self.material,3,0,0,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.u.reg_write(reg,0)
        self.u.reg_write(UC_X86_REG_ESP,self.stack);self.u.reg_write(UC_X86_REG_EFLAGS,2)
        self.u.reg_write(UC_X86_REG_FPCW,0x027f);self.u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        self.result=None;self.selected=None
        self.u.emu_start(0x41040,0,count=100000)
        assert self.result is not None
        if self.selected is not None:assert self.selected==selected,(self.selected,selected)
        return self.result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/damage-shields-goal-20261011')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    rows=[];actual=0
    for model,shield in ((0xe9070782,0xe9f70872),(0xe9e90864,0xe9f4086f),(0xea1d0898,0xea870902),(0xe9f80873,0xea860901)):
        override=shader_masks(cache,shield);assert override==[0x8000]
        for kind,shader,original in actual_parts(cache,model):
            assert shader==0xffffffff or original
            rows.append((kind,shader,0,original,override));actual+=1
    for kind in range(8):
        for shader in (2,0xffffffff):
            for selected in range(3):
                for mask in (0,2,8,0x80,0x8000,0x8080,0x8002,0x8008,0x200000,0xffffffff):
                    for first in (0,0x8000,0x208000):
                        original=[0x1234]*3;original[selected]=mask
                        override=[first,0x8000,0x208000]
                        rows.append((kind,shader,selected,original,override))
    oracle=Original();encoded=[];accepted=0
    for kind,shader,selected,original,override in rows:
        result=oracle.run(kind,shader,selected,original,override);accepted+=result
        encoded.append(' '.join(f'{v:x}'for v in (kind,shader,selected,len(original),len(override),*original,*override,result)))
    fixture=args.output/'shield-override-parts.tsv'
    fixture.write_text('# Unchanged complete retail41040 prefix and1BF50; explicit resident banks, stop before scheduling\n'+'\n'.join(encoded)+'\n')
    dependencies=[ROOT/'assets/disc/default.xbe',cache.path,Path(__file__).resolve()]
    report=dict(method=__doc__,cases=len(rows),actual_part_contexts=actual,accepted=accepted,
        original_lod_calls=oracle.choices,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        dependencies={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()for path in dependencies})
    (args.output/'shield-override-parts-original.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in ('cases','actual_part_contexts','accepted','original_lod_calls')}))
if __name__=='__main__':main()
