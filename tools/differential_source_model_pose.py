#!/usr/bin/env python3
"""Untouched cold16D940/local1421F0/hierarchy142A60 arithmetic.

Literal bind quaternion bytes are copied by the original16D940 before posing.
The original RCPSS executes unchanged. Explicit-seed controls enter its untouched
post-RCPSS142273 tail with a declared register bank, proving that narrower seam
without asserting those seeds were produced by physical Pentium III hardware.
"""
from pathlib import Path
import argparse, collections, hashlib, json, math, random, struct, subprocess, sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def f(v):return struct.unpack('<f',packed([v]))[0]
def bits(v):return words(struct.pack('<f',v))[0]
def h(v):return hashlib.sha256(v).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-mp-lightmap/source_model_pose_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/model-pose-original.json')
    args=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe');assert h(x.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    BASE=0x2100000;u.mem_map(BASE,0x400000);STOP=0x3000000;u.mem_map(STOP,4096)
    INPUT,A,B,OUTPUT,MODE,RAW,COLD=[BASE+i*0x10000 for i in range(7)]
    STACK=BASE+0x3fff00;calls=collections.Counter();observed={}
    def trace(_u,pc,_size,_data):
        if pc in [0x16d940,0x1421f0,0x142273,0x142a60]:calls[f'{pc:X}']+=1
        if pc==0x142244:observed['norm']=u.reg_read(UC_X86_REG_XMM0)&0xffffffff
        if pc==0x142273:observed['rcp']=u.reg_read(UC_X86_REG_XMM1)&0xffffffff
    u.hook_add(UC_HOOK_CODE,trace)
    def run(pc,regs,stack):
        for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,0)
        for i in range(8):u.reg_write(UC_X86_REG_XMM0+i,0)
        for r,v in regs.items():u.reg_write(r,v)
        u.mem_write(STACK,packed(stack));u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPTAG,0xffff)
        u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.emu_start(pc,STOP,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
    def local(v,seed=None):
        observed.clear();u.mem_write(INPUT,packed(v));run(0x1421f0,{},[STOP,OUTPUT,INPUT])
        norm=observed['norm'];original_seed=observed.get('rcp');out=words(u.mem_read(OUTPUT,52))
        if seed is not None and f(norm)>f(0x38d1b717):
            run(0x142273,{UC_X86_REG_EAX:INPUT,UC_X86_REG_ECX:OUTPUT,
                UC_X86_REG_XMM0:norm,UC_X86_REG_XMM1:seed,
                UC_X86_REG_XMM7:int.from_bytes(packed(v[:4]),'little')},[0,STOP,OUTPUT,INPUT])
            out=words(u.mem_read(OUTPUT,52))
        return norm,original_seed,out
    def compose(a,b):
        u.mem_write(A,packed(a));u.mem_write(B,packed(b))
        run(0x142a60,{UC_X86_REG_ECX:A,UC_X86_REG_EDX:B},[STOP,OUTPUT])
        return words(u.mem_read(OUTPUT,52))
    cases=[];modes=set();maps={};assets=[];copy_words=0;asset_lines=[]
    for path in [ROOT/'tests/fixtures/retail_source_model_palette_assets.tsv',ROOT/'tests/fixtures/retail_dynamic_model_shader_assets.tsv']:
        for line in path.read_text().splitlines():
            row=line.split()
            if row and row[0]=='D':modes.add((row[1],int(row[2],16)))
            if row and row[0]=='M':modes.add((row[1],int(row[4],16)))
    for name,datum in sorted(modes):
        if name not in maps:
            path=ROOT/'assets/disc/maps'/(name+'.map')
            if not path.exists():path=ROOT/'assets/map-pack/maps'/(name+'.map')
            maps[name]=Cache(path)
        c=maps[name];_,mode=c.tag(datum,'mode');nodes=[];raw=[]
        for at in c.block(mode+0x48,96):
            parent=struct.unpack_from('<h',c.data,at+4)[0]
            assert parent==-1 or 0<=parent<len(nodes),(name,datum,len(nodes),parent)
            vals=[*words(c.data[at+24:at+40]),*words(c.data[at+12:at+24]),bits(1)]
            nodes.append((parent,vals));raw.append(bytes(c.data[at:at+96]))
        if not nodes:continue
        u.mem_write(MODE,bytes(128));u.mem_write(MODE+0x48,packed([len(nodes),RAW]));u.mem_write(RAW,b''.join(raw))
        run(0x16d940,{},[STOP,MODE,COLD]);copied=words(u.mem_read(COLD,len(nodes)*32))
        expected=[v for _p,vals in nodes for v in vals];assert copied==expected
        copy_words+=len(expected);assets.append(dict(map=name,mode=f'{datum:08X}',nodes=len(nodes),raw_sha256=h(b''.join(raw)),cold_pose_sha256=h(packed(copied))))
        asset_lines.append(f'{name} {datum:X} {len(nodes):X} '+' '.join(
            f'{v:X}'for i,(parent,vals)in enumerate(nodes)
            for v in [words(raw[i][:4])[0],parent&0xffffffff,*vals]))
        cases.append(dict(kind='literal_cold_bind',map=name,mode=f'{datum:08X}',nodes=nodes))
    rng=random.Random(0x142a60)
    rotations=[[0,0,0,1],[0,0,-1,-1e-7],[1,0,0,0],[0,1,0,0],[0,0,1,0],[math.sqrt(.5),0,0,math.sqrt(.5)],[.5,.5,.5,.5]]
    for i in range(500):
        nodes=[]
        for j in range(2+i%6):
            q=rotations[(i+j)%len(rotations)];pos=[rng.choice([0.,-0.,rng.uniform(-1000,1000)])for _ in range(3)]
            scale=rng.choice([1.,1.,.25,.5,2.,4.])if i<250 else rng.choice([.1,.75,1.25,1.7,-.75,-1.25])
            nodes.append((j-1,[*map(bits,q),*map(bits,pos),bits(scale)]))
        cases.append(dict(kind='controlled_unit_hierarchy'if i<250 else'controlled_nonunit_scale_hierarchy',nodes=nodes))
    #Explicit original tail inputs exercise the retained RCPSS-seed seam.
    for i in range(160):
        nodes=[];seeds=[]
        for j in range(2+i%5):
            q=[rng.choice([0.,-0.,rng.uniform(-2,2)])for _ in range(4)]
            vals=[*map(bits,q),*map(bits,[rng.uniform(-10,10)for _ in range(3)]),bits(rng.choice([0.,-.75,1.,1.7]))]
            norm=(f(bits(q[0]*q[0]))+f(bits(q[2]*q[2])))
            norm=f(bits(norm))+f(bits(f(bits(q[1]*q[1]))+f(bits(q[3]*q[3]))));norm=f(bits(norm))
            seed=bits(1/norm*(1+rng.choice([0.,-1/8192,1/8192,-1/4096,1/4096])))if norm>f(0x38d1b717)else bits(1)
            nodes.append((j-1,vals));seeds.append(seed)
        cases.append(dict(kind='explicit_post_rcpss_register_bank',nodes=nodes,seeds=seeds))
    for i,q in enumerate([[0.,0.,0.,0.],[0.,-0.,0.,-0.],[.009,0,0,0],[.01,0,0,0],[.0101,0,0,0],[.02,.03,.04,.05]]):
        cases.append(dict(kind='norm_guard_and_nonunit_quaternion',nodes=[(-1,[*map(bits,q),bits(-0.),bits(2),bits(-3),bits(-1.7)])]))
    for i in range(40):
        q=rotations[i%len(rotations)]
        cases.append(dict(kind='explicit_root_hierarchy',root=list(map(bits,[.75,1,0,0,0,0,-1,0,1,0,-0.,3,-4])),
            nodes=[(-1,[*map(bits,q),bits(-0.),bits(2),bits(3),bits(.75)]),(0,[*map(bits,rotations[(i+2)%len(rotations)]),bits(1),bits(-0.),bits(4),bits(-1.7)])]))
    inputs=[];goldens=[];by_kind=collections.defaultdict(collections.Counter);seeds_hist=collections.Counter();asset_world_index=0
    for case in cases:
        n=len(case['nodes']);seeded='seeds'in case;root=case.get('root');row=[int(root is not None),n,int(seeded),*(root or [])];out=[];locals=[]
        for i,(parent,v)in enumerate(case['nodes']):
            seed=case['seeds'][i]if seeded else None;row.extend([parent&0xffffffff,*v,*([seed]if seeded else[])])
            norm,rcp,transform=local(v,seed);out.extend([norm,*transform]);locals.append(transform)
            if rcp is not None:seeds_hist[f'{norm:08X}:{rcp:08X}']+=1
        world=[]
        for i,(parent,_v)in enumerate(case['nodes']):
            world.append(compose(world[parent],locals[i])if parent>=0 else compose(root,locals[i])if root else locals[i])
        out.extend(v for t in world for v in t);inputs.append(row);goldens.append(out)
        if case['kind']=='literal_cold_bind':
            asset_lines[asset_world_index]+=' | '+' '.join(f'{v:X}'for t in world for v in t)
            asset_world_index+=1
        by_kind[case['kind']]['cases']+=1;by_kind[case['kind']]['nodes']+=n;by_kind[case['kind']]['words']+=len(out)
    text=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in inputs)
    actual=[list(map(lambda x:int(x,16),line.split()))for line in subprocess.check_output([str(args.binary),'--probe'],input=text,text=True).splitlines()]
    assert len(actual)==len(goldens)
    for i,(a,b)in enumerate(zip(actual,goldens)):
        if a!=b:
            j=next(j for j,(x,y)in enumerate(zip(a,b))if x!=y)
            raise AssertionError((i,cases[i].get('map'),cases[i].get('mode'),cases[i]['kind'],j,f'{a[j]:08X}',f'{b[j]:08X}',inputs[i]))
    fixture=ROOT/'tests/fixtures/retail_source_model_pose.tsv'
    content='# Original cold16D940, local1421F0 and model-order142A60; explicit seed rows prove post-RCPSS arithmetic only.\n'
    content+=''.join(' '.join(f'{v:X}'for v in row)+' | '+' '.join(f'{v:X}'for v in out)+'\n'for row,out in zip(inputs,goldens))
    fixture.write_text(content)
    asset_fixture=ROOT/'tests/fixtures/retail_source_model_pose_assets.tsv'
    asset_fixture.write_text('# Literal names/parents, untouched16D940 bind locals | original1421F0/142A60 WORLD banks, all21 maps.\n'+'\n'.join(asset_lines)+'\n')
    report=dict(executable_sha256=PIN,tool_sha256=h(Path(__file__).read_bytes()),probe_sha256=h(args.binary.read_bytes()),fixture_sha256=h(fixture.read_bytes()),
        asset_fixture_sha256=h(asset_fixture.read_bytes()),cases=len(cases),nodes=sum(len(c['nodes'])for c in cases),words=sum(map(len,goldens)),by_kind={k:dict(v)for k,v in by_kind.items()},
        source_calls=dict(calls),source_services_replaced=[],cold_models=len(assets),cold_copy_words=copy_words,raw_cold_bind_models=assets,
        norm_rcpss_observations=dict(seeds_hist),fp_control_word='027F',mxcsr='1F80',
        scope='Original cold16D940 copies authored raw quaternion and translation, scale1. Untouched1421F0 RCPSS executes in pinned Unicorn/QEMU software convention; post-RCPSS142273 rows separately prove explicit arbitrary register seeds. Original142A60 consumes source local transforms with authored model-order parents and optional explicit root. No source function/service or arithmetic output is replaced. No physical Pentium III reciprocal-seed, live animation controller, registration chronology or GPU raster equality claim.')
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k not in ['norm_rcpss_observations','raw_cold_bind_models']},indent=2)+'\n')
    asset_fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k not in ['norm_rcpss_observations','raw_cold_bind_models']},indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ['norm_rcpss_observations','raw_cold_bind_models']},indent=2))
    for c in maps.values():c.close()
if __name__=='__main__':main()
