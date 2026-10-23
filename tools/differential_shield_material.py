#!/usr/bin/env python3
"""Compare actual shield authored animation against unchanged retail owners.

The complete BAB40 vitality-input chain runs on original biped functions. Its
result is supplied through an explicit shader-callback service, then unchanged
17550/17420/176A0/17960 animate the original shader records. Native prepared
baseline words are shared only to isolate animation; this is not independent
static material assembly, posed vertex, final raster or Xbox precision proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess

from audit_rendering import Cache
from oracle_damage_feedback import FeedbackOracle, bits
from oracle_multiplayer_hud_positions import ROOT, STOP
from oracle_mp_spawn import CACHE_PIN
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                              UC_X86_REG_ESP, UC_X86_REG_ESI, UC_X86_REG_FPTAG, UC_X86_REG_FPSW)
from unicorn import UC_HOOK_CODE


def packed(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/damage-shields-goal-20261011')
    parser.add_argument('--overshield',action='store_true',help='Compare the four authored overshield shaders')
    parser.add_argument('--camouflage',action='store_true',help='Execute original19CA0 with declared scalar37=1 before animation')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    original=FeedbackOracle();state=original.alloc(bytes(0x1800))
    callback_names=[]
    def observe(uc,address,size,data):
        if address==STOP+0x100:callback_names.append(original.r(uc.reg_read(UC_X86_REG_ESP)+8))
    original.g.hook_add(UC_HOOK_CODE,observe)
    def run(entry,argv=(),registers=()):
        original.g.reg_write(UC_X86_REG_FPTAG,0xffff)
        original.g.reg_write(UC_X86_REG_FPSW,0)
        return original.run(entry,argv=argv,registers=registers)
    page=cache.magic&~4095
    original.g.mem_map(page,((len(cache.data)+cache.magic-page+4095)//4096)*4096)
    original.g.mem_write(cache.magic,bytes(cache.data))
    for datum,tag in cache.tags.items():
        if tag['address']:original.w(original.table+(datum&65535)*16+8,tag['address'])
    original.w(original.unit+0x14,0xffffffff);original.w(original.unit+0xc0,0)
    original.w(STOP+0x208,0);original.w(STOP+0x300,0);original.w(STOP+0x304,STOP+0x100)
    original.w(0x485a58,STOP+0x300)
    # Keep executable callback code immutable across species; the selected
    # export ID is ordinary data. This avoids Unicorn's cached translations
    # of a previously executed callback immediate after a code overwrite.
    callback=bytes.fromhex('8b4424083b05')+packed([STOP+0x210])+b'\x75\x09'
    callback+=b'\xd9\x05'+packed([STOP+0x200])+b'\xc2\x08\x00'
    callback+=b'\xd9\x05'+packed([STOP+0x208])+b'\xc2\x08\x00'
    original.g.mem_write(STOP+0x100,callback)
    rows=[];rng=random.Random(0x4bf8317420)
    scalars=[bits(x) for x in (-1.,0.,.00001,.0001,.00010001,.125,.5,1.,1.5,2.)]+[0x80000000,0x3f7fffff]
    shaders=((0xe96307de,0xe9f2086d),(0xe96607e1,0xe9f2086d),
             (0xea8d0908,0xea1b0896),(0xea8e0909,0xea1b0896)) if args.overshield else (
             (0xe9f4086f,0xe9f2086d),(0xe9f70872,0xe9f2086d),
             (0xea860901,0xea1b0896),(0xea870902,0xea1b0896))
    for shader,biped in shaders:
        for n in range(60):
            lo,hi=words(struct.pack('<d',rng.choice([0.,.00001,.01,.1,.2718,1.,10.,100.])))
            rows.append([shader,biped,scalars[n%len(scalars)],bits(rng.choice([0.,.5,1.])),
                         bits(rng.choice([0.,.25,.5,1.,1.5,2.,3.] if args.overshield else
                                         [0.,.25,.5,1.,2.])),rng.randrange(2),lo,hi])
    encoded=[' '.join(f'{v:x}' for v in row)for row in rows]
    native=subprocess.run([str(args.binary.resolve()),str(cache.path),'--probe'],
        input='\n'.join(encoded)+'\n',text=True,capture_output=True)
    if native.returncode:raise RuntimeError(native.stderr)
    results=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
    assert len(results)==len(rows)
    cold_results=results
    if args.camouflage:
        native=subprocess.run([str(args.binary.resolve()),str(cache.path),'--probe-camouflaged'],
            input='\n'.join(encoded)+'\n',text=True,capture_output=True)
        if native.returncode:raise RuntimeError(native.stderr)
        results=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
        assert len(results)==len(rows)
    expected=[]
    for row,result,cold in zip(rows,results,cold_results):
        assert len(result)==378
        shader,biped,recent,body,shield,dead,lo,hi=row
        name=0x1800055a if args.overshield else 0x12001a0b if biped==0xea1b0896 else 0x0a0012b4
        original.w(original.unit,biped);original.w(original.unit+0xf4,recent)
        original.w(original.unit+0xec,body);original.w(original.unit+0xf0,shield)
        original.w(original.unit+0x10a,dead<<2)
        active=run(0xbab40,argv=[0,name,original.output])&255
        source=[original.r(original.output),active]
        assert source==result[:2],(row,source,result[:2])
        # Explicit stdcall shader callback returns the independently executed
        # authored biped export, or zero for another name. No original owner
        # is patched and no native result is supplied to this callback.
        original.w(STOP+0x210,name);original.w(STOP+0x200,source[0])
        _,raw_shader=cache.tag(shader,'shad');definition=cache.block(raw_shader+32,124)[0]
        _,raw_template=cache.tag(cache.u32(definition),'stem');template=cache.block(raw_template+88,40)[0]
        record=cache.block(template+16,10)[0]
        _,raw_pass=cache.tag(cache.u32(record+4),'spas');pp=cache.block(raw_pass+28,88)[0]
        impl=cache.block(pp,306)[0]
        animation=cache.block(definition+52,10)[0]
        original.g.mem_write(state,bytes(0x1800));original.w(state+12,cache.magic+definition)
        original.w(state+32,cache.magic+pp);original.w(state+36,cache.magic+impl)
        switches=[0]*8;switches[4:6]=[cache.u16(raw_shader+0x3e),cache.u16(raw_shader+0x40)]
        original.g.mem_write(state+0x300,packed(switches))
        original.g.mem_write(state+0x1424,packed(cold[2:62]))
        original.g.mem_write(state+0x1530,packed(result[62:190]))
        if args.camouflage:
            original.w(state+0x5c+37*4,bits(1.))
            run(0x19ca0,argv=[cache.magic+impl+0x122],registers=[(UC_X86_REG_ESI,state)])
            assert words(original.g.mem_read(state+0x1424,240))==result[2:62], 'Original overshield scalar37 baseline differs'
        original.g.mem_write(0x4858a0,packed([lo,hi]))
        callback_names.clear()
        run(0x17550,argv=[cache.magic+animation],registers=[(UC_X86_REG_EBX,state)])
        run(0x17420,argv=[state,0],registers=[(UC_X86_REG_EAX,0),(UC_X86_REG_ECX,15),(UC_X86_REG_EBX,0)])
        bank=words(original.g.mem_read(state+0x1424,240))+words(original.g.mem_read(state+0x1530,512))
        if bank!=result[190:]:
            difference=[i for i,(a,b)in enumerate(zip(bank,result[190:]))if a!=b]
            raise RuntimeError(f'Original shield animation differs: {row}, words {difference}, '
                f'source {[hex(bank[i])for i in difference]}, native {[hex(result[190+i])for i in difference]}, '
                f'callback names {[hex(v)for v in callback_names]}')
        expected.append(source+bank)
    stem=('overshield-material' if args.overshield else 'shield-material')+('-camouflage' if args.camouflage else '')
    fixture=args.output/(stem+'.tsv')
    fixture.write_text('# Original BAB40 chain plus actual shield animated constants; explicit shader callback and clock\n'+
        '\n'.join(encoded[i]+' '+' '.join(f'{v:x}'for v in expected[i])for i in range(len(rows)))+'\n')
    dependencies=[ROOT/'assets/disc/default.xbe',cache.path,Path(__file__).resolve(),
                  ROOT/'tools/oracle_damage_feedback.py',ROOT/'tools/oracle_multiplayer_hud_positions.py',args.binary.resolve()]
    report=dict(passed=True,method=__doc__,cases=len(rows),compared_words=len(rows)*(250 if args.camouflage else 190),all_bit_exact=True,
        camouflage_scalar=1 if args.camouflage else 0,
        post_animation_unchanged_by_shader={hex(shader):sum(a[190:]==b[190:] for row,a,b in zip(rows,results,cold_results)
            if row[0]==shader) for shader,_ in shaders} if args.camouflage else {},
        fixtures_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        dependencies={(str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p)):
                      hashlib.sha256(p.read_bytes()).hexdigest()for p in dependencies})
    (args.output/(stem+'-original.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,cases=len(rows),compared_words=report['compared_words'])))


if __name__=='__main__':main()
