#!/usr/bin/env python3
"""Shared native1E6CB0 query with genuine2D/polygon child ABI and FP phases."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import struct
import subprocess
import sys
from generated_routine_ownership import function_bodies
from validate_native_tu15_prt_helpers import PIN,physical_original
from native_bsp_surface_polygon_replacements import canonical_body as polygon_body
from native_bsp_surface_2d_replacements import canonical_body as tree_body
from native_bsp_leaf_surface_replacements import canonical_body,replace_bsp_leaf_surface

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x1000000,0x5000),(0x44a000,8192),(0x46a000,4096),(0x62000000,16384))
COPIES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/gameplay/units/death_cycle_runtime_tu15_generated.cpp')
INPUTS=('include/halo2/native_tu15_bsp_leaf_surface_kernel.hpp','src/physics/runtime/native_tu15_bsp_leaf_surface.hpp',
    'tests/native_tu15_bsp_leaf_surface_abi_tests.cpp','tools/validate_native_bsp_leaf_surface_abi_tu15.py',
    'tools/native_bsp_leaf_surface_replacements.py','src/physics/native_tu15_bsp_surface_query.cpp',
    'src/physics/runtime/native_tu15_bsp_surface_polygon.hpp','src/physics/runtime/native_tu15_bsp_surface_2d.hpp')


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def fresh_primary(body):
    """The exact current lowerer's C1/spill refinement of the old whole body."""
    original=body
    for pc in (0x1e6d07,0x1e6d12,0x1e6d21,0x1e6d2a,0x1e6d2e,0x1e6d0c,0x1e6d1b,0x1e6d24):
        line=next(line for line in body.splitlines()if f'instruction(0x{pc:x}u)'in line)
        body=body.replace(line,line+'s.fp.status&=~0x200u;',1)
    if body.count('u(spill(s.fp.st(0)))')!=3:raise ValueError('Fresh spill refinement witness missing')
    body=body.replace('u(spill(s.fp.st(0)))','native_single_spill(s.fp.st(0),s.fp.status)')
    body=body.replace('s.fp.st(0)=s.fp.st(0);s.fp.pop();','s.fp.st(0)=s.fp.st(0);s.fp.status&=~0x200u;s.fp.pop();',1)
    if body==original or hashlib.sha256(body.encode()).hexdigest()!='c321544ee067f27ade04a7f89d79d8e0414aac6a73702584bed21a01ec45a0a0':raise ValueError('Exact fresh primary body refinement changed')
    return body


def corpus(image):
    from validate_native_bsp_surface_query_tu15 import cases
    old=[c for c in cases(image)if c['entry']==0x1e6cb0];rng=random.Random(0x1e6cb0);rows=[]
    live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','0100000000000080ff7f','0000000000000080ff7f')
    for n,case in enumerate(old):
        sp=0x62002000+n%4;stack=bytearray(rng.randbytes(16384));struct.pack_into('<6I',stack,sp-0x62000000,0x63000000,*case['arguments'])
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=case['receiver'];gp[4]=sp
        for name,index in (('ebx',3),('esi',6),('edi',7)):gp[index]=case['entry_registers'][name]
        rows.append(dict(id=n,name=case['name'],entry=0x1e6cb0,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),matrix=case['arguments'][0],source=case['arguments'][1],
            before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64)|((n*0x100)&0x4700),0x1f80|((n%4)<<13)|(n%64)],
            live_x87=list(live[:(0,1,4,6)[n%4]]),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=10000,
            banks=[dict(address=0x1000000,before=case['banks'][0]['before']),dict(address=0x44a000,before=image.read(0x44a000,8192).hex()),dict(address=0x46a000,before=image.read(0x46a000,4096).hex()),dict(address=0x62000000,before=stack.hex())]))
    seed=rows[48] # one matching plane, no polygon verification
    def extra(name,edit):
        row=copy.deepcopy(seed);row['id']=len(rows);row['name']=name
        def put(address,value,width=4):
            for bank in row['banks']:
                data=bytearray.fromhex(bank['before']);offset=address-bank['address']
                if 0<=offset<=len(data)-width:
                    data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');bank['before']=data.hex();return
            raise ValueError('Directed query fixture address outside banks')
        # Direct leaf nodes keep descriptor/scratch aliases independent from
        # unrelated tree storage; separate cases exercise the genuine tree.
        put(0x1000a02,0x8000,2);edit(row,put)
        # The physical launch always installs these declared entry words.
        # Preserve that same initial backing when a pointed-to input overlaps
        # the caller's return/argument area.
        for offset,value in ((0,0x63000000),(4,row['matrix']),(8,row['source'])):put(row['gp'][4]+offset,value)
        rows.append(row)
    extra('tree-invalid-sentinel',lambda row,put:(put(0x1000a02,0,2),put(0x100100c,0xffff,2),put(0x100100e,0xffff,2)))
    for register in (6,7):
        for offset in (0x1c,0x20,0x24,0x28):
            def alias(row,put,register=register,offset=offset):
                pointer=row['gp'][4]-0x30+offset;row['gp'][register]=pointer
                for lane,value in enumerate((0x3e800000,0x3f000000,0x3e000000)):put(pointer+lane*4,value)
            extra(f'point-input-stack-r{register}-offset{offset:x}',alias)
    for distance in (0x38,0x44,0x48,0x50):
        def descriptor(row,put,distance=distance):
            descriptor=row['gp'][4]-distance;row['gp'][3]=descriptor
            for offset,value in ((12,0x1000800),(0x14,0x1000900),(0x1c,0x1000a00),(0x24,0x1001000),(0x2c,0x1001800),(0x34,0x1002000),(0x3c,0x1002800)):put(descriptor+offset,value)
        extra(f'descriptor-stack-minus{distance:x}',descriptor)
    for offset in (8,0x10):
        def normal(row,put,offset=offset):
            pointer=row['gp'][4]-0x30+offset;put(0x100010c,pointer)
            for lane,value in enumerate((0x3f000000,0x3e800000,0x3f800000)):put(pointer+lane*4,value)
        extra(f'plane-coefficients-stack-offset{offset:x}',normal)
    for normal in ((0x7fc23456,0x7fc12345,0x7f812345),(0x7f800000,0xff800000,0),(1,0x80000001,1),(0x7f7fffff,0x00800000,0x80000000)):
        for rounding in range(4):
            def floating(row,put,normal=normal,rounding=rounding):
                row['before_fp'][0]=(row['before_fp'][0]&~0xc00)|(rounding<<10);row['before_fp'][2]=(row['before_fp'][2]&~0x6000)|(rounding<<13)
                for lane,value in enumerate(normal):put(0x1000800+lane*4,value)
            extra(f'normal-special-{normal[0]:x}-rc{rounding}',floating)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--compiler',default='c++');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in ('x86_64','amd64','i386','i686'):print('SKIP physical query requires Linux x86');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,candidate_unbound=not args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        input_hashes={path:sha(ROOT/path)for path in INPUTS};binding_sources={}
        owners=((0x1e6cb0,'src/physics/physics_live_support_tu15_generated.cpp',canonical_body,'94ffc606749a9b3c7d7167eff2bb4e8d6ba12f9ba6ef1620894f6aafca433d7d'),
            (0x2a0230,'src/campaign/ai/source_ai_frame_tu15_generated.cpp',tree_body,None),(0x1e6e80,'src/runtime/source_map_world_support_tu15_generated.cpp',polygon_body,None))
        bodies=[]
        for entry,path,strip,pin in owners:
            source=(ROOT/path).read_text();body=strip(next(b for m,b in function_bodies(source)if int(m[1],16)==entry))
            if pin and hashlib.sha256(body.encode()).hexdigest()!=pin:raise ValueError('Whole original query body drift')
            if entry==0x2a0230:
                from native_bsp_surface_2d_replacements import replace_bsp_surface_2d
                if replace_bsp_surface_2d(source)!=source:raise ValueError('Genuine2D child binding changed')
            if entry==0x1e6e80:
                from native_bsp_surface_polygon_replacements import replace_bsp_surface_polygon
                if replace_bsp_surface_polygon(source)!=source:raise ValueError('Genuine polygon child binding changed')
            bodies.append(re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',body))
        declarations='\n'.join(f'static void run_{entry:x}(WorldState&);'for entry,_,_,_ in owners)
        dispatch='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x2a0230u)run_2a0230(s);else if(target==0x1e6e80u)run_1e6e80(s);else throw std::runtime_error("Unexpected original query child");}'
        reference='namespace reference {\n'+declarations+'\n'+dispatch+'\n'+'\n'.join(bodies)+'\n}\n';rejections=0
        for index,path in enumerate(COPIES):
            source=(ROOT/path).read_text();binding_sources[path]=hashlib.sha256(source.encode()).hexdigest();parent=next(b for m,b in function_bodies(source)if m[1]=='1e6cb0');original=canonical_body(parent)
            raw=source.replace(parent,original,1);bound=replace_bsp_leaf_surface(raw)
            if args.require_binding and bound!=source:raise ValueError('Parent source prefix missing: '+path)
            if replace_bsp_leaf_surface(bound)!=bound:raise ValueError('Parent binding is not idempotent')
            selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
            shapes=(original,re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original))
            if not index:shapes+=fresh_primary(bodies[0]),
            for shape in shapes:
                candidate=raw.replace(original,shape,1);replace_bsp_leaf_surface(candidate)
                for version in (candidate,replace_bsp_leaf_surface(candidate)):
                    target=next(b for m,b in function_bodies(version)if m[1]=='1e6cb0');changed=target.replace('s.gp[1]*4u','s.gp[1]*8u',1)
                    if changed==target:raise ValueError('Parent body mutation witness missing')
                    try:replace_bsp_leaf_surface(version.replace(target,changed,1))
                    except ValueError:rejections+=1
                    else:raise ValueError('Changed complete parent body accepted')
                if 'selected('in shape:
                    try:replace_bsp_leaf_surface(candidate.replace(selected,selected.replace('{','{if(p==0)return 1u;',1),1))
                    except ValueError:rejections+=1
                    else:raise ValueError('Changed whole parent resolver accepted')
            original_dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            dispatches=(original_dispatch,)
            if index:
                pairs=re.findall(r'if\(target==selected\(0x([0-9a-f]+)u,s.image\)\)\{run_([0-9a-f]+)\(s\);return;\}',original_dispatch)
                if not pairs or any(a!=b for a,b in pairs)or len({a for a,b in pairs})!=len(pairs):raise ValueError('Canonical dispatcher identity/uniqueness drift')
                fresh_dispatch='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){switch(target){'+''.join(f'case 0x{int(a,16):x}u:run_{b}(s);return;'for a,b in sorted(pairs,key=lambda pair:int(pair[0],16)))+'default:s.required(site,target,tail);return;}}'
                replace_bsp_leaf_surface(source.replace(original_dispatch,fresh_dispatch,1));dispatches+=fresh_dispatch,
            for dispatcher in dispatches:
                for changed in (dispatcher.replace('{','{if(target==0)return;',1),dispatcher.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1),dispatcher.replace('run_1e6cb0(s)','run_2a0230(s)',1)):
                    if changed==dispatcher:raise ValueError('Parent dispatcher mutation witness missing')
                    try:replace_bsp_leaf_surface(source.replace(original_dispatch,changed,1))
                    except ValueError:rejections+=1
                    else:raise ValueError('Changed complete parent dispatcher accepted')
            mini=selected+'\n'+next(b for m,b in function_bodies(bound)if m[1]=='1e6cb0')
            helper=''
            if 'native_single_spill('in original:
                helper=re.search(r'std::uint32_t native_single_spill\(.*?\n\}',source,re.S)[0]
                if hashlib.sha256(helper.encode()).hexdigest()!='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1':raise ValueError('Original single-spill helper drift')
            reference+=f'namespace bound{index} {{\n'+declarations+'\n'+dispatch+'\n'+'\n'.join(bodies[1:])+'\n'+helper+'\n'+mini+'\n}\n'
            if index:reference+=f'namespace retained{index} {{\n'+declarations+'\n'+dispatch+'\n'+'\n'.join(bodies[1:])+'\n'+helper+'\n'+selected+'\n'+original+'\n}\n'
        reference+='namespace fresh_reference {\n'+declarations+'\n'+dispatch+'\n'+'\n'.join(bodies[1:])+'\n'+helper+'\n'+fresh_primary(bodies[0])+'\n}\n'
        (scratch/'bsp_leaf_surface_bodies.inc').write_text(reference)
        sites=set()
        for name in ('native_tu15_bsp_leaf_surface.hpp','native_tu15_bsp_surface_2d.hpp','native_tu15_bsp_surface_polygon.hpp'):
            for value in re.findall(r'0x([0-9a-f]+)u',(ROOT/'src/physics/runtime'/name).read_text()):
                pc=int(value,16)
                if 0x1e6cb0<=pc<=0x1e6fac or 0x2a0230<=pc<=0x2a028d:sites.add(pc)
        from oracle_smg_firing_1_1 import Xbe
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from validate_native_bsp_surface_polygon_abi_tu15 import metadata
        original=common.metadata;common.metadata=lambda image:metadata(image,original)
        image=Xbe(args.xbe);rows=corpus(image);delta={0x1e6d0e,0x1e6d15,0x1e6d1d,0x1e6d26,0x1e6d2e}
        observed,proof=physical_original(image,rows,scratch,entries=(0x1e6cb0,0x2a0230,0x1e6e80),regions=REGIONS,
            trace_calls=[dict(entry=pc,arguments=5 if pc==0x1e6e80 else 0,gp=True,xmm=True,x87_delta=int(pc in delta))for pc in sorted(sites)],trace_extended=True)
        cases=[]
        for row,out in zip(rows,observed['cases'],strict=True):
            if not out['completed']or not out['live_x87_unchanged']:raise ValueError('Original query did not finish/preserve caller80')
            case=copy.deepcopy(row)
            returns=set(out['executed_sites'])&{0x1e6e67,0x1e6e70}
            if len(returns)!=1:raise ValueError('Original parent return cursor ambiguous')
            case['final_pc']=returns.pop()
            for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes'):case[key]=out[key]
            case['packets']=[{key:value[key]for key in ('entry','gp','eflags','xmm','fp','live_x87','arguments')}for value in out['calls']]
            for before,after in zip(case['banks'],out['banks'],strict=True):before['after']=after['after']
            cases.append(case)
        read_sites=sorted({e['instruction']for out in observed['cases']for e in out['reads']});fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases,explicit_read_sites=read_sites),separators=(',',':')))
        binary=scratch/'query-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_bsp_leaf_surface_abi_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compile=subprocess.run(command,capture_output=True,text=True)
        if compile.returncode:raise ValueError('Query compile failed: '+compile.stderr[-5000:])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=90)
        if run.returncode:raise ValueError('Query native comparison failed: '+run.stderr[-6000:])
        control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=90,env=dict(os.environ,HALO2_REFERENCE_BSP_LEAF_SURFACE='1',HALO2_BSP_LEAF_SURFACE_REFERENCE_TEST='1'))
        if control.returncode:raise ValueError('Forced parent reference control failed: '+control.stderr[-4000:])
        reference_control=json.loads(control.stdout)
        if not reference_control.get('native_refused')or reference_control.get('callbacks',0)<=0:raise ValueError('Reference control did not measure real literal fallback')
        if any(sha(ROOT/path)!=pin for path,pin in input_hashes.items()):raise ValueError('Proof source changed during qualification')
        result=json.loads(run.stdout)
        report.update(result,physical_original=proof,source_sha256=input_hashes,binding_source_sha256_at_extraction=binding_sources,binding_rejections=rejections,reference_control=reference_control,original_sites_executed=len({pc for row in observed['cases']for pc in row['executed_sites']}))
        if report['original_sites_executed']!=252:raise ValueError('Parent/child original instruction coverage incomplete')
        report['genuine_child_entry_packets']={f'{entry:x}':sum(call['entry']==entry for row in observed['cases']for call in row['calls'])for entry in (0x2a0230,0x1e6e80)}
        report['fresh_primary_retained_reference_sha256']=hashlib.sha256(fresh_primary(bodies[0]).encode()).hexdigest()
        report['open_domains']=['memory/observer exception and partial-write prefixes','unsupported MMX/unmasked FP legacy fallback','full-runtime delivery beyond focused source cohort']
        print(run.stdout.strip())
    except BaseException as error:report.update(passed=False,error=str(error));raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':raise SystemExit(main())
