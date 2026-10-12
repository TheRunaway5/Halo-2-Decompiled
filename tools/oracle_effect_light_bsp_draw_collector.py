#!/usr/bin/env python3
"""Replay whole retail2CB30/133390/133520/2CBF0 with retained source RAM.

The first case uses the actual frag receiver admission and retained main
prefix. Declared variants change the pass/override, arena use, fog gates,
lighting availability, coverage, metadata residency or capacity. No original
instruction/callee is replaced. Dynamic133720 and GPU draws are not executed.
With --join-cull,133720 is executed and must preserve the BSP-owned state;
44720/45560 then execute unchanged and native consumes actual Cache geometry.
With --join-frame, whole secondary40E90 and its observed empty41C80 queue
produce commands, then whole3BAB0 performs ordinary sphere layer12 selection
and sorting. Native stages consume the preceding native unit/mask/command
outputs and independent Cache parts/shaders. The real retained main prefix
remains an explicit input. Nonempty dynamic/object queues and GPU draws are
not supplied.4BA014=1 is the declared ordinary-sphere selection input.
"""
import argparse
import hashlib
import itertools
import json
import random
import struct
import subprocess
import sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_UNMAPPED,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
NONE=0xffffffff
def pack(words):return struct.pack('<'+'I'*len(words),*(v&NONE for v in words))
def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))
def digest(data):return hashlib.sha256(data).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--native',type=Path);p.add_argument('--join-cull',action='store_true');p.add_argument('--map',type=Path)
    p.add_argument('--join-frame',action='store_true',help='Join secondary40E90 and ordinary sphere3BAB0 selection after native collector/cull')
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    if a.join_frame:a.join_cull=True
    if a.native and a.join_cull:assert a.map,'joined native cull requires actual Cache geometry'
    snapshot=Snapshot(a.snapshot);assert digest(snapshot.ram)==PIN
    xbe=Xbe(ROOT/'assets/disc/default.xbe')
    assert digest(xbe.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    executed={}
    def unchanged(vm,pc,size,_data):
        if pc not in executed:
            raw=bytes(vm.mem_read(pc,size));assert raw==xbe.read(pc,size),f'Original instruction differs at{pc:08x}'
            executed[pc]=raw
    g=snapshot.guest();g.mem_map(0x70000000,0x100000);stack,stop=0x700fe000,0x700fff00
    def read(guest,at,n):
        regions=list(guest.mem_regions())
        for page in range(at&~4095,(at+n+4095)&~4095,4096):
            if not any(begin<=page<=end for begin,end,_ in regions):
                guest.mem_map(page,4096);guest.mem_write(page,snapshot.read(page,4096))
        return bytes(guest.mem_read(at,n))
    def value(guest,at):return u(read(guest,at,4))
    def byte(guest,at):return read(guest,at,1)[0]
    def short(guest,at):return struct.unpack('<H',read(guest,at,2))[0]
    def call(entry,end):
        g.mem_write(stack,pack([stop]));g.reg_write(R.UC_X86_REG_ESP,stack)
        g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
        g.emu_start(entry,end,count=10000000);assert g.reg_read(R.UC_X86_REG_EIP)==end
    call(0xc15a0,stop);call(0x31c80,0x2cb30)
    context=value(g,0x4c0b80);bsp=value(g,0x4e0348)
    nc,cp=value(g,bsp+0x9c),value(g,bsp+0xa0)
    ni,ip=value(g,bsp+0x140),value(g,bsp+0x144)
    nd,dp=value(g,bsp+0x138),value(g,bsp+0x13c)
    assert nc<=512 and ni<=2048 and nd<=2048
    metadata=[cp+i*176 for i in range(nc)]+[dp+i*200 for i in range(nd)]
    for at in metadata:read(g,at,0x6c)
    read(g,ip,ni*88);read(g,0x4ba138,850*32);read(g,0x4c0b84,850);read(g,context,0x2ae0)
    group_pointer=value(g,0x4e0344)
    lm=value(g,group_pointer+0x84)if group_pointer and value(g,group_pointer+0x80)>0 else 0
    lm_matches=bool(lm and value(g,lm+0x1c)!=NONE and value(g,lm+4)==value(g,bsp+8))
    lightmap_resident=[]
    for i in range(ni):
        ref=value(g,lm+0x54)+i*12 if lm_matches else 0
        bucket=value(g,lm+0x44)+short(g,ref+2)*56 if ref else 0
        lightmap_resident.append(bool(bucket and byte(g,bucket+12+0x1e)))
    # Materialize the metadata used by the baseline and its saved CPU stack.
    state=g.context_save()
    base_regions=[(begin,end,permissions,read(g,begin,end-begin+1)) for begin,end,permissions in list(g.mem_regions())]
    def clone(source=None):
        guest=Uc(UC_ARCH_X86,UC_MODE_32)
        regions=base_regions if source is None else [(begin,end,permissions,read(source,begin,end-begin+1))for begin,end,permissions in list(source.mem_regions())]
        for begin,end,permissions,data in regions:guest.mem_map(begin,end-begin+1,permissions);guest.mem_write(begin,data)
        def missing(vm,_access,address,_size,_value,_data):
            page=address&~4095
            try:raw=snapshot.read(page,4096)
            except Exception:return False
            vm.mem_map(page,4096);vm.mem_write(page,raw);return True
        guest.hook_add(UC_HOOK_MEM_UNMAPPED,missing);guest.hook_add(UC_HOOK_CODE,unchanged)
        guest.context_restore(state if source is None else source.context_save());return guest
    def queue(guest,header):
        n=short(guest,header+4);assert n<=512
        flags,ids,other,association=[value(guest,header+off) for off in [8,12,16,20]]
        return [[value(guest,ids+i*4),short(guest,flags+i*2),short(guest,association+i*2)if association else 0xffff,value(guest,other+i*4)if other else 0] for i in range(n)]
    def owned_state(guest):
        out=[value(guest,0x4c0b7c),value(guest,0x4c0b78),short(guest,context+0xa5c)]
        out.extend(words(read(guest,0x4ba138,850*32)));out.extend(read(guest,0x4c0b84,850))
        out.extend(words(read(guest,context+0x25c,2048)));return out
    def inputs(guest,matches):
        out=[nc,ni,nd]
        for i,at in enumerate(metadata):
            out.extend([short(guest,at+0x14),short(guest,at+0x24),int(bool(value(guest,at+0x50))and bool(byte(guest,at+0x28+0x1e)))])
            if i<nc:out.extend(words(read(guest,at+0x54,24)))
        for i in range(ni):
            at=ip+i*88;out.extend([short(guest,at+0x34),*words(read(guest,at+0x3c,16)),short(guest,at+0x56),int(lightmap_resident[i]),at])
            if a.join_cull:out.extend(words(read(guest,at,52)))
        fog_datum=value(guest,0x4b9f8c);table=value(guest,0x4e3b44)
        fog_flags=value(guest,value(guest,table+(fog_datum&65535)*16+8))if fog_datum!=NONE else 0
        out.extend([value(guest,context+0x2a78),context+0x25c,byte(guest,context+0x2a60),value(guest,0x4b9ef0),fog_datum,value(guest,0x4b9ffc),fog_flags,int(bool(byte(guest,0x4ba004)))])
        out.extend([value(guest,at)for at in [0x4b9e20,0x4b9e2c,0x4b9e38,0x4b9e44,0x4b9ed0]])
        out.append(int(matches))
        if a.join_cull:out.extend([value(guest,context+0x2a7c),*words(read(guest,context+0x2a68,16))])
        out.extend(read(guest,context+0x1c,512))
        for header in [value(guest,context+12),value(guest,context+24)]:
            records=queue(guest,header);out.append(len(records));out.extend(v for record in records for v in record)
        out.extend(owned_state(guest));return out
    command_headers=[0x4c1a48+i*24 for i in range(3)]
    def command_state(guest):
        out=[]
        for h in command_headers:
            header=words(read(guest,h,24));out.extend(header);out.append(header[0]);out.extend(words(read(guest,header[5],header[0]*24)))
        special=0x4c62f8+value(guest,0x509440)*0x408
        out.append(short(guest,special+0x404));out.extend(words(read(guest,special,1024)))
        return out
    def command_inputs(guest):
        object_count=short(guest,value(guest,context+0x14)+4)
        assert object_count==0,'Joined frame requires native nonempty41C20 owner'
        vis=value(guest,0x4ed280)
        out=[short(guest,0x4686c4),value(guest,0x4670e4),byte(guest,0x4ba019),*words(read(guest,0x485b28,28)),object_count]
        out.extend(words(read(guest,vis,0x4204)));out.extend(command_state(guest))
        return out
    def original_frame(guest):
        trace=[]
        def observe(_vm,pc,_n,_d):trace.append(pc)
        hook=guest.hook_add(UC_HOOK_CODE,observe,begin=0x41c20,end=0x41c20)
        guest.mem_write(stack,pack([stop]));guest.reg_write(R.UC_X86_REG_ESP,stack);guest.reg_write(R.UC_X86_REG_EAX,0)
        guest.emu_start(0x40e90,stop,count=30000000);assert guest.reg_read(R.UC_X86_REG_EIP)==stop
        guest.hook_del(hook);assert not trace,'Joined frame consumed nonempty object command production'
        output=0x70000000;guest.mem_write(output,pack([0xa5a5a5a5]*(1024*6)))
        guest.mem_write(0x4ba014,pack([1])) # Explicit ordinary sphere31170 flag, as in the selection oracle.
        guest.mem_write(stack,pack([stop,0x4c1a60,0,12,output]));guest.reg_write(R.UC_X86_REG_ESP,stack)
        guest.emu_start(0x3bab0,stop,count=30000000);assert guest.reg_read(R.UC_X86_REG_EIP)==stop
        selected=guest.reg_read(R.UC_X86_REG_EAX);assert selected<=1024
        return [*command_state(guest),selected,*words(read(guest,output,1024*24))]
    variants=[('actual frag receiver with retained main prefix',{})]
    for mode,override,used in itertools.product([0,1,2],[0,128,127],[0,510,511]):
        variants.append(('pass/override/arena boundary',dict(mode=mode,override=override,used=used)))
    for prefix in [0,1,848,849,850]:variants.append(('literal850-unit capacity',dict(prefix=prefix)))
    for metric in [0.,.001,.1,1.,120.,720.,2000.]:variants.append(('actual coverage and zero-alpha gate',dict(metric=metric)))
    for policy in [0,1,65535]:variants.append(('literal instance lighting policy',dict(policy=policy)))
    for mutation in ['no_lightmap','no_geometry','no_masks','entry_sign','random_retained']:
        variants.append((mutation,{mutation:True}))
    if a.join_frame:variants=[(label,v)for label,v in variants if v.get('mode',1)!=0]
    rng=random.Random(0x133390133520)
    fixture=[0x43504442,3 if a.join_frame else 2 if a.join_cull else 1,len(variants)];cases=[];calls={}
    for label,variant in variants:
        guest=clone();matches=lm_matches
        def write(at,data):read(guest,at,len(data));guest.mem_write(at,data)
        for key,at in [('mode',context+0x2a78),('prefix',0x4c0b7c)]:
            if key in variant:write(at,pack([variant[key]]))
        if 'override'in variant:write(context+0x2a60,bytes([variant['override']]))
        if 'used'in variant:write(context+0xa5c,struct.pack('<H',variant['used']))
        if 'metric'in variant:write(0x4b9ed0,struct.pack('<f',variant['metric']))
        if 'policy'in variant:
            for i in range(ni):write(ip+i*88+0x56,struct.pack('<H',variant['policy']))
        if variant.get('no_lightmap'):write(0x4e0344,pack([0]));matches=False
        if variant.get('no_geometry'):
            for at in metadata:write(at+0x50,pack([0]))
        if variant.get('no_masks'):
            for at in metadata:write(at+0x24,b'\0\0')
        if variant.get('entry_sign'):write(context+0x1c,bytes([128 if i%2 else 255 for i in range(512)]))
        if variant.get('random_retained'):
            write(0x4ba138,pack([rng.getrandbits(32)for _ in range(850*8)]));write(0x4c0b84,bytes(rng.randrange(256)for _ in range(850)))
            write(context+0x25c,pack([rng.getrandbits(32)for _ in range(512)]))
        before=inputs(guest,matches)
        def observe(_g,pc,_size,_data):calls[f'{pc:08x}']=calls.get(f'{pc:08x}',0)+1
        for pc in [0x2cb30,0x133390,0x133520,0x2cbf0,0x2d000,0x1332b0,0x12de70]:guest.hook_add(UC_HOOK_CODE,observe,begin=pc,end=pc)
        guest.emu_start(0x2cb30,0x133720,count=10000000);assert guest.reg_read(R.UC_X86_REG_EIP)==0x133720
        after=owned_state(guest);after.extend(record[1]for record in queue(guest,value(guest,context+24)))
        if a.join_cull:
            # The actual receiver's dynamic queue contributes no draw units.
            # Verify that boundary for each variant before joining the cull.
            continuation=clone(guest);continuation.emu_start(0x133720,0x44720,count=10000000)
            assert continuation.reg_read(R.UC_X86_REG_EIP)==0x44720
            before_cull=owned_state(continuation);before_cull.extend(record[1]for record in queue(continuation,value(continuation,context+24)))
            assert before_cull==after,(label,'dynamic collector changed the BSP-only state')
            cull_guest=clone(continuation);trace=[]
            def observe_cull(vm,_pc,_size,_data):
                index=vm.reg_read(R.UC_X86_REG_ECX)&65535;pointer,radius=words(read(vm,vm.reg_read(R.UC_X86_REG_ESP)+4,8))
                trace.extend([index,*words(read(vm,pointer,12)),radius])
            cull_guest.hook_add(UC_HOOK_CODE,observe_cull,begin=0x45560,end=0x45560)
            cull_guest.emu_start(0x44720,0x31170,count=10000000);assert cull_guest.reg_read(R.UC_X86_REG_EIP)==0x31170
            after.extend(owned_state(cull_guest));after.extend(record[1]for record in queue(cull_guest,value(cull_guest,context+24)))
            after.extend([len(trace)//5,*trace])
            if a.join_frame:
                before.extend(command_inputs(cull_guest))
                after.extend(original_frame(cull_guest))
        fixture.extend([len(before),len(after),*before,*after]);cases.append(dict(label=label,mutation=variant,main_prefix=after[0],draw_count=after[1],mask_used=after[2],checked_words=len(after)))
    path=a.output/'fixture.bin';path.write_bytes(pack(fixture))
    report=dict(boundary=__doc__,snapshot_sha256=PIN,intercepted_instructions=0,joined_cull=a.join_cull,cases=cases,
        calls=calls,checked_words=sum(v['checked_words']for v in cases),fixture_sha256=digest(path.read_bytes()),tool_sha256=digest(Path(__file__).read_bytes()))
    report.update(joined_secondary_frame=a.join_frame,object_commands='observed empty41C80 only' if a.join_frame else 'unjoined',
        ordinary_sphere_flag='explicit4BA014=1' if a.join_frame else None)
    report.update(original_xbe_sha256=digest(xbe.data),executed_instructions=len(executed),
        executed_instruction_bank_sha256=digest(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))))
    if a.map:report.update(native_map=str(a.map.resolve()),native_map_sha256=digest(a.map.read_bytes()))
    if a.native:
        command=[str(a.native.resolve()),str(path.resolve())]
        if a.join_cull:command.append(str(a.map.resolve()))
        result=subprocess.run(command,text=True,capture_output=True)
        report.update(native_binary_sha256=digest(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ['cases','boundary']},indent=2))
    return report.get('native_returncode',0)
if __name__=='__main__':raise SystemExit(main())
