#!/usr/bin/env python3
"""Execute whole unchanged TU1.5 Random vehicle selectors as an offline reference."""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from audit_rendering import Cache, file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

GP = (R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--include-flying',action='store_true');args=parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest()!=NEW_PIN:raise ValueError('Original TU1.5 changed')
    code=_Code(image,'.text')
    families=(('light',0x15cb60,76,6),('heavy',0x15cc70,78,3),('turret',0x15cdf0,81,5))
    if args.include_flying:families+=(('flying',0x15cd30,79,3),)
    bodies={entry:complete_body(code,entry) for _,entry,_,_ in families}
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    context=0x61000000;stack=0x600f0000;stop=0x60300000;variant=context+0x5000;seed_at=context+0x5500
    cpu.mem_map(context,0x6000);cpu.mem_map(stack-0x1000,0x2000);cpu.mem_map(stop,0x1000)
    rng=random.Random(0x15cc10);seeds=[0,1,0xffffffff,0x80000000,0x12345678]+[rng.getrandbits(32)for _ in range(27)]
    trace=[];reads=[];stores=[];current={};covered=set()
    def instruction(_cpu,pc,size,_):
        if pc not in current['sites'] or bytes(cpu.mem_read(pc,size))!=image.read(pc,size):
            raise ValueError('Original Random selector escaped its authenticated complete owner')
        current['pc']=pc;trace.append(pc);covered.add(pc)
    def read(_cpu,_kind,at,size,_value,_):
        reads.append([current['pc'],at,bytes(cpu.mem_read(at,size)).hex()])
    def write(_cpu,_kind,at,size,value,_):
        if not(context<=at<at+size<=context+0x6000 or stack-0x1000<=at<at+size<=stack+0x1000):
            raise ValueError('Original selector wrote outside its declared component banks')
        stores.append([current['pc'],at,(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
    cpu.hook_add(UC_HOOK_CODE,instruction);cpu.hook_add(UC_HOOK_MEM_READ,read);cpu.hook_add(UC_HOOK_MEM_WRITE,write)
    maps=[];cases=[]
    for map_name in ('coagulation','headlong','zanzibar'):
        cache=Cache(Path('assets/disc/maps')/(map_name+'.map'))
        _,globals_at=cache.tag(next(tag['datum']for tag in cache.tags.values()if tag['group']=='matg'),'matg')
        _,multiplayer_at=cache.tag(cache.u32(globals_at+0x16c),'mulg')
        runtime=cache.block(multiplayer_at+8,0x568)[0];rows=cache.block(runtime+0x70,8)
        raw_choices=bytes(cache.data[rows[0]:rows[0]+len(rows)*8])
        if len(rows)<11:raise ValueError('Original vehicle-reference table is incomplete')
        maps.append(dict(name=map_name,path=str(cache.path),sha256=file_sha(cache.path),vehicle_reference_bytes=raw_choices.hex()))
        for family,entry,parameter,mode in families:
            current['sites']=set(bodies[entry].code)
            # Every signed byte reaches its own original fixed/default/None
            # branch. Random additionally spans32 declared independent seeds.
            inputs=[(mode,seed)for seed in seeds]+[(value,seeds[(value+128)%len(seeds)])
                for value in range(-128,128)if value!=mode]
            for value,seed in inputs:
                trace.clear();reads.clear();stores.clear()
                cpu.mem_write(context,b'\xa5'*0x6000);cpu.mem_write(stack-0x1000,b'\xa5'*0x2000)
                patches=[]
                def raw(at,data):cpu.mem_write(at,data);patches.append([at,data.hex()])
                def word(at,value):raw(at,struct.pack('<I',value&0xffffffff))
                word(0x4ecc1c,context);word(context+0x16c,0xeeff0001)
                word(0x4f0414,context+0x1000);word(context+0x1018,context+0x2000)
                word(context+0x200c,context+0x3000);word(context+0x3074,context+0x4000)
                raw(context+0x4000,raw_choices);word(variant,0xdead1234)
                word(0x4f3cd8,seed_at);word(seed_at,seed)
                raw(stack,struct.pack('<4I',stop,0xeeff4321,value&0xffffffff,variant))
                gp=[0x71392481,0x11223344,0xaabbccdd,0xbadc0ffe,stack,0xd00dcafe,0x13572468,0x24681357]
                for reg,register_value in zip(GP,gp):cpu.reg_write(reg,register_value)
                cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f)
                cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                cpu.emu_start(entry,stop,count=10000)
                if cpu.reg_read(R.UC_X86_REG_EIP)!=stop:raise ValueError('Original Random selector did not return within its declared instruction budget')
                draws=[row for row in stores if row[1]==seed_at]
                if value==mode and(not draws or cpu.reg_read(R.UC_X86_REG_EAX)==0xffffffff):
                    raise ValueError('Original Random selector did not produce its own admitted result')
                cases.append(dict(map=map_name,family=family,entry=entry,parameter=parameter,value=value,random=value==mode,
                    initial_seed=seed,patches=patches,initial_gp=gp,initial_flags=0x202,
                    final_gp=[cpu.reg_read(reg)for reg in GP],final_flags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
                    final_variant=struct.unpack('<I',cpu.mem_read(variant,4))[0],final_seed=struct.unpack('<I',cpu.mem_read(seed_at,4))[0],
                    rng_draws=len(draws),trace=list(trace),reads=list(reads),stores=list(stores)))
    all_sites=set().union(*(set(body.code)for body in bodies.values()))
    if covered!=all_sites:raise ValueError('Not every complete original selector instruction was covered')
    fixture=dict(schema='H2_RANDOM_VEHICLE_ORIGINAL_V1',original_sha256=NEW_PIN,maps=maps,cases=cases,
        component_banks=[[context,0x6000,0xa5],[stack-0x1000,0x2000,0xa5]],
        scope='Declared selector globals/reference tables and seeds; unchanged complete original CPU owners. No constructor, native kernel, renderer or playable Random admission is inferred.',
        source_instruction_patches=[],supplied_dependency_returns=[])
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode();packed=lzma.compress(raw)
    if len(packed)>1024*1024:raise ValueError('Required original reference exceeds the bounded compressed budget')
    target=args.output/'original-reference.json.xz';target.write_bytes(packed)
    if lzma.decompress(target.read_bytes())!=raw:raise ValueError('Original reference compression changed its bytes')
    summary=dict(passed=True,original_sha256=NEW_PIN,cases=len(cases),retry_cases=sum(row['rng_draws']>1 for row in cases),
        maximum_rng_draws=max(row['rng_draws']for row in cases),families=sorted({row['family']for row in cases}),
        random_cases=sum(row['random']for row in cases),signed_byte_choices_complete=True,
        instruction_coverage_complete=covered==all_sites,complete_original_instructions=len(all_sites),
        required_original_reference=dict(path=str(target.resolve()),bytes=len(packed),uncompressed_bytes=len(raw),
            sha256=file_sha(target),raw_sha256=hashlib.sha256(raw).hexdigest(),reason='Unchanged original Random selector ABI, ordered accesses and source RNG for native integration'),
        source_instruction_patches=[],supplied_dependency_returns=[],native_random_admission_verified=False,
        tool_sha256=file_sha(Path(__file__)))
    (args.output/'report.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(summary),flush=True)


if __name__=='__main__':main()
