#!/usr/bin/env python3
"""Unchanged TU1.1/TU1.5 contact-input segments, including original angle/CRT.

Response row, velocity/normalized direction, contact kind, header type, stored
scale, flight distance, dry damage range and primary RNG are explicit inputs.
No instruction/service is replaced. This does not generate a collision or run
attachment, penetration, post-response noise, timers or rendered effects.
"""
import argparse, hashlib, json, math, random, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
from inspect_multiplayer import Cache


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--tu11',type=Path,required=True);ap.add_argument('--tu15',type=Path,required=True)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--live-snapshot',type=Path,help='Optional authenticated TU1.5 RAM corroboration of the initialized default row')
    args=ap.parse_args()
    previous=json.loads((ROOT/'analysis/multiplayer/integration/plasma-contact-20261010/measured-v2/report.json').read_text())
    pair=next(p for p in previous['pairs'] if p['old']=='000feb40')
    mapping={int(r['old'],16):int(r['u5'],16) for r in pair['mapping']}
    defaults=[]
    for path,entry,address in ((ROOT/'assets/disc/default.xbe',0x376dd0,0x547660),
            (args.tu11,0x37d3c0,0x54f1b0),(args.tu15,0x380e70,0x5564f0)):
        image=Xbe(path)
        assert hashlib.sha256(image.data).hexdigest() in (
            '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
            *(row['sha256'] for row in previous['images']))
        vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x800000)
        for section in image.sections:vm.mem_write(section.va,image.section_bytes(section))
        vm.mem_map(0x1000000,0x10000);vm.reg_write(UC_X86_REG_ESP,0x100f000)
        vm.mem_write(0x100f000,struct.pack('<I',0x100ff00));vm.emu_start(entry,0x100ff00,count=1000)
        assert vm.reg_read(UC_X86_REG_EIP)==0x100ff00
        raw=bytes(vm.mem_read(address,88))
        defaults.append(dict(image=str(path),sha256=hashlib.sha256(image.data).hexdigest(),
            initializer=hex(entry),initializer_hex=image.read(entry,0xc9).hex(),address=hex(address),initialized_row_hex=raw.hex()))
    assert len({row['initialized_row_hex'] for row in defaults})==1
    default_row=bytes.fromhex(defaults[0]['initialized_row_hex'])
    live=None
    if args.live_snapshot:
        from oracle_effect_light_snapshot import Snapshot
        snapshot=Snapshot(args.live_snapshot);state=json.loads((args.live_snapshot/'source-state.json').read_text())
        image=Xbe(args.tu15);text=image.section('.text')
        assert state['reference']=='OG Xbox TU1.5' and not state['guest_code_patches']
        assert hashlib.sha256(snapshot.ram).hexdigest()==state['ram_sha256']
        assert snapshot.read(text.va,text.rsize)==image.read(text.va,text.rsize)
        assert snapshot.read(0x5564f0,88)==default_row
        live=dict(snapshot=str(args.live_snapshot.resolve()),ram_sha256=state['ram_sha256'],
            whole_live_text_matches_original=True,default_row_matches_initializer=True)
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    banks=[]
    for tag in cache.tags.values():
        if tag['group']=='proj' and tag['offset'] is not None:
            banks += [bytes(cache.data[p:p+0x58]) for p in cache.block(tag['offset']+0x19c,0x58)]
    banks=list(dict.fromkeys(banks));banks.append(default_row);rng=random.Random(0xfef99)
    controls=[]
    for row in banks:
        for angle in (0.,math.pi/6,math.pi/4,math.pi/2):
            controls.append((row,angle,rng.getrandbits(32),rng.choice((1.,10.,100.)),1,0))
    for _ in range(512):
        row=bytearray(banks[0]);struct.pack_into('<h',row,2,3);struct.pack_into('<h',row,0x14,4)
        struct.pack_into('<Hf',row,0x16,rng.choice((0,1,2,3)),rng.choice((0.,.5,1.)))
        struct.pack_into('<4f',row,0x1c,0.,math.pi/4,0.,rng.choice((0.,10.)))
        struct.pack_into('<h',row,0x34,rng.choice((0,1,2)))
        struct.pack_into('<2f',row,0x38,rng.choice((-.1,0.,.06981317,.5)),rng.choice((-.5,0.,.25,2.)))
        controls.append((bytes(row),rng.uniform(0,math.pi/2),rng.getrandbits(32),rng.choice((.1,1.,10.,100.)),rng.choice((1,4)),rng.randrange(4)))
    inputs=[]
    for row,angle,seed,speed,kind,otype in controls:
        v=struct.pack('<9f',-1.,0.,0.,math.sin(angle),math.cos(angle),0.,math.sin(angle)*speed,math.cos(angle)*speed,0.)
        flags=rng.choice((0,16));delta=[rng.uniform(-5,5) for _ in range(3)];traveled=rng.uniform(0,50);damage_range=rng.choice((5.,20.,100.));scale=rng.choice((0.,.25,1.))
        position=struct.pack('<3f',*(rng.uniform(-1000,1000) for _ in range(3)))
        contact=[p+d for p,d in zip(struct.unpack('<3f',position),delta)]
        extras=struct.pack('<IIIfI3ffff',seed,kind,otype,speed,flags,*contact,traveled,damage_range,scale)
        inputs.append(v+row+extras+position+struct.pack('<I',row==default_row))
    canonical=None;provenance=[]
    for n,path in enumerate((args.tu11,args.tu15)):
        x=Xbe(path);pin=hashlib.sha256(x.data).hexdigest();assert pin==previous['images'][n]['sha256']
        at=lambda old:old if n==0 else mapping[old]
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
        g.mem_map(0x1000000,0x20000)
        stack,rowaddr,packet,velocity,normal,random_,body,definition,pool,headers,stop=[0x1000000+i*0x1000 for i in range(11)]
        stack=0x101f000
        w=lambda a,v:g.mem_write(a,struct.pack('<I',v&0xffffffff))
        rng_ref=struct.unpack('<I',x.read(at(0xfefd8)+2,4))[0]
        pool_ref=struct.unpack('<I',x.read(at(0xff125)+1,4))[0]
        w(rng_ref,random_);w(pool_ref,pool);w(pool+0x44,headers)
        init=0x37d560 if n==0 else 0x381010
        def execute(start,end):
            g.reg_write(UC_X86_REG_ESP,stack);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_FPSW,0);g.reg_write(UC_X86_REG_FPTAG,0xffff);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
            g.emu_start(start,end,count=100000);assert g.reg_read(UC_X86_REG_EIP)==end
        w(stack,stop);execute(init,stop)
        outputs=[]
        for data in inputs:
            vectors=data[:36];row=data[36:124];seed,kind,otype,speed,flags,*tail=struct.unpack('<IIIfI3ffff',data[124:168])
            g.mem_write(rowaddr,row);g.mem_write(normal,vectors[:12]);g.mem_write(velocity,vectors[24:36]);g.mem_write(stack+0x38,vectors[12:24]);g.mem_write(stack+0x10,struct.pack('<f',speed));w(random_,seed);w(packet,kind);w(packet+0x40,0x12340002);g.mem_write(headers+27,bytes([otype]))
            for reg,value in ((UC_X86_REG_EAX,rowaddr),(UC_X86_REG_EDI,normal),(UC_X86_REG_ESI,velocity),(UC_X86_REG_EBP,packet),(UC_X86_REG_XMM1,0)):g.reg_write(reg,value)
            execute(at(0xfef99),at(0xff1d9))
            result=bytes(g.mem_read(stack+0x20,4))+bytes(g.mem_read(stack+0x58,4))+bytes(g.mem_read(stack+0x10,4))+bytes(g.mem_read(random_,4))
            delta=tail[:3];traveled,damage_range,scale=tail[3:]
            g.mem_write(body+0x64,data[168:180]);g.mem_write(packet+8,struct.pack('<3f',*delta));g.mem_write(body+0x168,struct.pack('<f',traveled));g.mem_write(body+0x170,struct.pack('<f',damage_range));g.mem_write(body+0x188,struct.pack('<f',scale));w(definition+0xbc,flags)
            for reg,value in ((UC_X86_REG_EBX,body),(UC_X86_REG_EDI,definition),(UC_X86_REG_EBP,packet),(UC_X86_REG_XMM1,0),(UC_X86_REG_XMM2,0x3f800000)):g.reg_write(reg,value)
            execute(at(0xfec8e),at(0xfed09));result+=bytes(g.mem_read(stack+0x28,4))
            w(stack+0x14,rowaddr);w(stack+0x1c,0x3f800000);g.reg_write(UC_X86_REG_XMM3,0);g.reg_write(UC_X86_REG_XMM4,0x3f800000)
            execute(at(0xff7ec),at(0xff836));result+=bytes(g.mem_read(stack+0x1c,4));outputs.append(result)
        if canonical is None:canonical=outputs
        else:assert outputs==canonical
        provenance.append(dict(image=str(path),sha256=pin,prefix=[hex(at(0xfef99)),hex(at(0xff1d9))],falloff=[hex(at(0xfec8e)),hex(at(0xfed09))],effect_scale=[hex(at(0xff7ec)),hex(at(0xff836))],initializer=hex(init)))
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_bytes(b'PIC3'+struct.pack('<II',len(inputs),len(inputs[0]))+bytes.fromhex(previous['images'][0]['one_hex'])+bytes.fromhex(previous['images'][0]['rng_scale_hex'])+b''.join(i+o for i,o in zip(inputs,canonical)))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(scope=__doc__,cases=len(inputs),images=provenance,default_rows=defaults,live_default_row=live,fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),paired_results_byte_exact=True),indent=2)+'\n')
    print('PASS original contact input cases',len(inputs),'paired exact')
if __name__=='__main__':main()
