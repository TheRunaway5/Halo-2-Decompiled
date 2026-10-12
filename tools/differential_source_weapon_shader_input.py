#!/usr/bin/env python3
"""Complete original FF5F0/10CF50 and actual energy-blade shader animation.

The animation clock and bound weapon fields are explicit inputs. No retained
pixel tint is inverted to obtain a time or state, and no original callee is
patched. The native baseline is shared only to isolate animation arithmetic;
the source executes the literal authored remap/curve banks from the cache.
"""
import argparse, hashlib, json, math, random, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from audit_rendering import Cache
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,STACK,STOP=0x2100000,0x22fef00,0x3000000
def packed(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,required=True)
    p.add_argument('--write-fixture',type=Path)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/source-weapon-shader-input-original.json')
    a=p.parse_args();xbe=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(xbe.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in xbe.sections:u.mem_write(section.va,xbe.section_bytes(section))
    u.mem_map(STATE,0x200000);u.mem_map(STOP,0x1000)
    c=Cache(ROOT/'assets/disc/maps/zanzibar.map')
    start=c.magic&~4095;size=(c.magic+len(c.data)-start+4095)&~4095
    u.mem_map(start,size);u.mem_write(c.magic,bytes(c.data))
    def put(address,*values):u.mem_write(address,packed(values))
    put(0x4e0300,STATE+0x10000);put(STATE+0x10044,STATE+0x11000)
    put(STATE+0x11008,STATE+0x12000);put(STATE+0x12000,0xe2d70162)
    put(0x4e3b44,STATE+0x13000)
    _,weapon=c.tag(0xe2d70162,'weap');put(STATE+0x13000+0x162*16+8,c.magic+weapon)
    entries={v:0 for v in [0xff5f0,0x10cf50,0x17550,0x17420,0x176a0,0x17960,0x1b230,0x13b390,0x17c900]}
    def observe(_u,address,_size,_data):
        if address in entries:entries[address]+=1
    u.hook_add(UC_HOOK_CODE,observe)
    def run(entry,args=(),registers=()):
        put(STACK,STOP,*args);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
        for reg,value in registers:u.reg_write(reg,value)
        try:u.emu_start(entry,STOP,count=1000000)
        except Exception as e:raise RuntimeError(f'original{entry:x} failed at{u.reg_read(UC_X86_REG_EIP):x}')from e
        if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'original{entry:x} did not return')
    # The source shader callback is an external caller service. Supply the
    # output of the complete original FF5F0 below, rather than its predicate.
    callback=bytes.fromhex('817c2408680500097408')+b'\xd9\x05'+struct.pack('<I',STOP+0x208)+b'\xc2\x08\x00'
    callback+=b'\xd9\x05'+struct.pack('<I',STOP+0x200)+b'\xc2\x08\x00'
    # Conditional target begins immediately after the first FLD/RET8 (9B).
    callback=bytearray(callback);callback[9]=9
    u.mem_write(STOP+0x100,bytes(callback));put(STOP+0x300,0,STOP+0x100);put(0x485a58,STOP+0x300);put(STOP+0x208,0)
    rng=random.Random(0xff5f010cf50);rows=[];labels=[]
    values=[0,0x80000000,bits(-1),bits(.5),0x3f7fffff,0x3f800000,0x3f800001,bits(2),0x7f800000,0xff800000,0x7fc01234,0xffc01234]
    for shader in [0xe2da0165,0xe2e50170]:
        for item in [0,1,2,3,0xffffff00,0xffffffff]:
            for flags in [0,0x20,0xffffffdf,0xffffffff]:
                for value in values:
                    clock=rng.choice([0.,.00001,.01,.1,.2718,1.,10.,100.])
                    lo,hi=words(struct.pack('<d',clock));rows.append([shader,item,flags,value,lo,hi]);labels.append('explicit field/adjacent comparison')
    retained=ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008/exports/object-E2F70088.bin'
    constructor=ROOT/'analysis/multiplayer-shading/netgame-birth-probe.json'
    real=[('retained earlier baseline',retained.read_bytes()),('untouched birth descriptor execution',bytes.fromhex(json.loads(constructor.read_text())['body']))]
    for label,data in real:
        for shader in [0xe2da0165,0xe2e50170]:
            lo,hi=words(struct.pack('<d',.2718));rows.append([shader,*[struct.unpack_from('<I',data,off)[0]for off in [0x12c,0x16c,0x184]],lo,hi]);labels.append(label)
    encoded=[' '.join(f'{v:x}'for v in row)for row in rows]
    native=subprocess.run([str(a.binary.resolve()),str(ROOT/'assets/disc/maps'),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    results=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
    assert len(results)==len(rows)
    expected=[];word_count=0
    for row,result,label in zip(rows,results,labels):
        shader,item,flags,value,lo,hi=row
        assert len(result)==378
        put(STATE+0x12000+0x12c,item);put(STATE+0x12000+0x16c,flags);put(STATE+0x12000+0x184,value)
        put(STATE+0x15000,0xdeadbeef,0xdeadbeef)
        run(0xff5f0,[0,0x09000568,STATE+0x15000,STATE+0x15004])
        source_input=words(u.mem_read(STATE+0x15000,4))+[u.mem_read(STATE+0x15004,1)[0]]
        assert source_input==result[:2],(label,row,source_input,result[:2])
        put(STOP+0x200,source_input[0])
        _,raw_shader=c.tag(shader,'shad');definition=c.block(raw_shader+32,124)[0]
        _,raw_template=c.tag(c.u32(definition),'stem');template=c.block(raw_template+88,40)[0]
        pass_record=c.block(template+16,10)[0]
        _,raw_pass=c.tag(c.u32(pass_record+4),'spas');pass_pp=c.block(raw_pass+28,88)[0];pass_impl=c.block(pass_pp,306)[0]
        animation=c.block(definition+52,10)[0]
        u.mem_write(STATE,bytes(0x1800));put(STATE+12,c.magic+definition);put(STATE+32,c.magic+pass_pp,c.magic+pass_impl)
        u.mem_write(STATE+0x1424,packed(result[2:62]));u.mem_write(STATE+0x1530,packed(result[62:190]))
        u.mem_write(0x4858a0,packed([lo,hi]))
        run(0x17550,[c.magic+animation],[(UC_X86_REG_EBX,STATE)])
        run(0x17420,[STATE,0],[(UC_X86_REG_EAX,0),(UC_X86_REG_ECX,15),(UC_X86_REG_EBX,0)])
        bank=words(u.mem_read(STATE+0x1424,240))+words(u.mem_read(STATE+0x1530,512))
        if bank!=result[190:]:
            different=[i for i,(x,y)in enumerate(zip(bank,result[190:]))if x!=y]
            raise RuntimeError(f'{label} {row} animation words{different}: source{[hex(bank[i])for i in different]} native{[hex(result[190+i])for i in different]}')
        expected.append(source_input+bank);word_count+=len(source_input)+len(bank)
    report=dict(passed=True,xbe_sha256=PIN,cache_sha256=hashlib.sha256(c.path.read_bytes()).hexdigest(),native_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),cases=len(rows),compared_words=word_count,all_bit_exact=True,fpcw='027F',mxcsr='1F80',entries={f'{k:08X}':v for k,v in entries.items()},actual_input_sources={str(retained.relative_to(ROOT)):hashlib.sha256(retained.read_bytes()).hexdigest(),str(constructor.relative_to(ROOT)):hashlib.sha256(constructor.read_bytes()).hexdigest()},scope='Complete originalFF5F0/10CF50; actual top/bottom authored24-overlay banks17550->17420/176A0/17960, explicit clock and external shader callback supplying independently executed source function result. Animation baseline shares native prepared authored bank only to isolate animation, not claim an independent material assembly proof. No source pixel tint inverted, caller clock/owner inferred, guest advanced or framebuffer/hardware precision claim.')
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    if a.write_fixture:
        selected=[*range(0,len(rows)-4,8),*range(len(rows)-4,len(rows))]
        a.write_fixture.write_text('# Complete original weapon turned_on and actual authored energy blade animation; explicit clock.\n'+'\n'.join(encoded[i]+' '+' '.join(f'{v:x}'for v in expected[i])for i in selected)+'\n')
        a.write_fixture.with_suffix('.provenance.json').write_text(json.dumps({**report,'frozen_cases':len(selected)},indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
