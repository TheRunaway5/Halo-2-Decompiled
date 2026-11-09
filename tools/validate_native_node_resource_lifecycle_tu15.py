#!/usr/bin/env python3
"""Unchanged-i386 proof for the unbound typed1E5C80 lifecycle controller."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
from native_effect_slot_scan_replacements import body
import native_node_resource_lifecycle_replacements as binding
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/campaign/ai/source_ai_actor_tu15_generated.cpp')
REGIONS=((0x61000000,65536),(0x62000000,8192),(0x51d000,8192),(0x4f0000,8192))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def corpus():
    rng=random.Random(0x1e5c80);rows=[]
    for n in range(160):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];mode=n%16;stack=0x62001000+n%4
        descriptor=0x61000100;nodes=0x61001000;resources=0x61008000;slots=0x61000600;objects=0x61000900
        if mode==11:descriptor=stack-0x38
        elif mode==14:slots=stack-4
        elif mode==15:stack=0x51df24+n%4
        root=0;node_base=nodes;low=0xffff if n%2 else 0
        if mode==12:root=0x8000;node_base=(nodes-(-32768)*108)&0xffffffff
        elif mode==13:low=0xfffe
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
            raise ValueError('Lifecycle fixture escaped bank')
        def get(at):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
            raise ValueError('Lifecycle fixture lookup escaped bank')
        put(0x51df20,0 if mode==0 else 1,1);put(0x51df21,0 if mode==1 else 1,1)
        put(descriptor+0x30,node_base);put(descriptor+0xb0,resources);put(descriptor+0x4c,4);put(descriptor+0x50,slots)
        put(0x4f0414,objects)
        for index in (-2,0,1,2,3):put(slots+index*32+4,0x12340002)
        put(objects+2*16+8,descriptor);put(objects+8,descriptor)
        for index in range(8):
            put(nodes+index*108+0x30,0xffff,2);put(nodes+index*108+0x32,0xffff,2);put(nodes+index*108+0x16,0,1)
            put(nodes+index*108+0x24,index);put(resources+index*20+0x10,0,1)
        if mode in (4,6,10):put(nodes+0x30,1,2)
        if mode in (5,6,8,9,10,11,14,15):
            if mode not in (6,10):put(nodes+0x32,1,2)
            put(nodes+108+0x32,2,2);put(nodes+216+0x32,3,2)
        if mode==7:put(nodes+0x16,0x10,1);put(resources+0x10,1,1)
        if mode in (8,9,10,11,14,15):
            for index in (1,2,3):put(nodes+index*108+0x16,0x10,1);put(resources+index*20+0x10,1 if mode in (8,10,14,15)else 0,1)
        handle=((0xffff if mode==2 else root)<<16)|low
        put(stack,0x63000000);put(stack+4,handle)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[2]=descriptor;gp[4]=stack
        if mode==11:gp[7]=node_base
        values=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000000080',
                '123456789abcde80fe3f','01000000000000000000','00000000000000800040','00000000000000800100')
        rows.append(dict(id=n,name=f'lifecycle-{n}-family{mode}',entry=0x1e5c80,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
            before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64),0x1f80|((n%4)<<13)|(0x8040 if n%8>=4 else 0)],
            live_x87=list(values[:(0,1,4,8)[n%4]]),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],matrix=handle,source=get(stack+8),
            max_steps=5000,stop_before_entries=[0x126200],banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    return rows
def sources(scratch,required):
    rendered=[];rejected=0;snapshots={path:(ROOT/path).read_text()for path in SOURCES};pins={path:hashlib.sha256(value.encode()).hexdigest()for path,value in snapshots.items()}
    for name,path in zip(('primary','frame','actor','literal'),(*SOURCES,SOURCES[1])):
        source=snapshots[path]
        if name=='literal':
            original=body(source,'run_1e5c80');unbound=binding.canonical_body(original)
            literal=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',unbound)
            source=source.replace(original,literal,1)
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            targets=re.findall(r'if\(target==selected\(0x([0-9a-f]+)u,s.image\)\)\{run_([0-9a-f]+)\(s\);return;\}',dispatcher)
            if len(targets)!=691 or any(a!=b for a,b in targets):raise ValueError('Fresh literal dispatcher construction drift')
            fresh='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){switch(target){'+''.join(f'case 0x{a}u:run_{b}(s);return;'for a,b in sorted(targets,key=lambda p:int(p[0],16)))+'default:s.required(site,target,tail);return;}}'
            source=source.replace(dispatcher,fresh,1)
        native=binding.replace_node_resource_lifecycle(source)
        if required and name!='literal'and native!=source:raise ValueError('Lifecycle production binding missing '+path)
        if binding.replace_node_resource_lifecycle(native)!=native:raise ValueError('Lifecycle binding not idempotent')
        current=body(source,'run_1e5c80')
        unbound_source=source.replace(current,binding.canonical_body(current),1)
        for raw in (unbound_source,native):
            target=body(raw,'run_1e5c80')
            for old,new in (('0x32u','0x30u'),('run_126200(s)','run_1e5af0(s)')):
                changed=raw.replace(target,target.replace(old,new,1),1)
                if changed==raw:raise ValueError('Missing lifecycle mutation witness')
                try:binding.replace_node_resource_lifecycle(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Lifecycle accepted whole-body mutation')
        resolver=next((line for line in source.splitlines()if line.startswith('std::uint32_t selected(')),'')
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        for old,new in (('run_1e5af0(s);','run_126200(s);'),
                        ('s.required(site,target,tail);','s.required(site+1u,target,tail);')):
            changed=source.replace(dispatcher,dispatcher.replace(old,new,1),1)
            if changed==source:raise ValueError('Missing lifecycle dispatcher drift witness')
            try:binding.replace_node_resource_lifecycle(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Lifecycle accepted changed child dispatcher')
        if name in ('frame','actor'):
            changed=source.replace(resolver,resolver.replace('case 0x1e3270u:return 0x1e3270u;','case 0x1e3270u:return 0x1e3271u;',1),1)
            if changed==source:raise ValueError('Missing lifecycle resolver drift witness')
            try:binding.replace_node_resource_lifecycle(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Lifecycle accepted changed resolver')
        # AI's real1E3270 handoff leaves its unit. Exercise the authenticated
        # primary child body, not a supplied lookup result; production cross-unit
        # routing remains a separately recorded dependency.
        lookup=body(snapshots[SOURCES[0]],'run_1e3270')
        parts=['namespace '+name+' {',resolver,
            'static void run_1e3270(WorldState&);static void run_1e5af0(WorldState&);',
            'void run_126200(WorldState&s){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x126200u;throw ChildBoundary{};}',
            'static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool){if(target==0x1e3270u)run_1e3270(s);else if(target==0x1e5af0u)run_1e5af0(s);else if(target==0x126200u)run_126200(s);else throw std::runtime_error("Unknown lifecycle owner");}',
            lookup,body(source,'run_1e5af0'),body(native,'run_1e5c80'),'}']
        rendered.append('\n'.join(parts))
    rendered='\n'.join(rendered);(scratch/'node_resource_lifecycle_bodies.inc').write_text(rendered)
    return rejected,pins,hashlib.sha256(rendered.encode()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--compiler',default='c++');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        rejected,source_pins,compiled_bodies_pin=sources(scratch,args.require_binding);binary=scratch/'lifecycle-tests'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_node_resource_lifecycle_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        result=subprocess.run(command,capture_output=True,text=True,timeout=120)
        if result.returncode:raise ValueError('Lifecycle compilation failed: '+result.stderr[:4000])
        from oracle_smg_firing_1_1 import Xbe
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old_metadata=common.metadata
        def metadata(image):
            result=old_metadata(image);code=common._Code(image,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;rows=corpus();physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x1e5c80,0x1e3270,0x1e5af0),regions=REGIONS,preserve_x87_prefix=True)
        prefixes=[]
        for row,case in zip(rows[:16],physical['cases'][:16],strict=True):
            for ordinal,event in enumerate(case['reads']):
                if 0x1e5c80<=event['instruction']<=0x1e5d5d:
                    value=copy.deepcopy(row);value.update(id=len(rows)+len(prefixes),name=row['name']+'-before-read-'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(value)
        prefix_dir=scratch/'prefix';prefix_dir.mkdir();prefix_observed,prefix_proof=physical_original(Xbe(args.xbe),prefixes,prefix_dir,entries=(0x1e5c80,0x1e3270,0x1e5af0),regions=REGIONS,preserve_x87_prefix=True)
        executed={pc for c in physical['cases']for pc in c['executed_sites']if 0x1e5c80<=pc<=0x1e5d5d}
        if len(executed)!=proof['owners']['0x1e5c80']['sites']:raise ValueError('Lifecycle parent original sites not all executed')
        cases=[]
        for row,original in zip(rows+prefixes,physical['cases']+prefix_observed['cases'],strict=True):
            value=copy.deepcopy(row);value.update({key:original[key]for key in ('boundary_gp','boundary_eflags','reads','writes')})
            value['completed']=original['stopped_at']==0x63000000
            value['stop_kind']='before_read'if 'fault'in row else'return'if value['completed']else'child'
            value['expected_pc']=original['boundary']['instruction']if 'fault'in row else 0x1e5d5d if value['completed']else 0x126200
            if original['boundary_xmm']!=row['xmm']or original['after_fp']!=row['before_fp']or not original['live_x87_unchanged']:raise ValueError('Original lifecycle FP changed')
            for bank,after in zip(value['banks'],original['banks'],strict=True):bank['after']=after['after']
            for event in value['reads']+value['writes']:event.pop('instruction')
            cases.append(value)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')))
        results=[]
        for forced in (False,True):
            env=dict(os.environ)
            env.pop('HALO2_REFERENCE_NODE_RESOURCE_LIFECYCLE',None)
            if forced:env['HALO2_REFERENCE_NODE_RESOURCE_LIFECYCLE']='1'
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env,timeout=60)
            if result.returncode:raise ValueError('Lifecycle comparison failed: '+result.stderr[:5000])
            results.append(json.loads(result.stdout))
        report.update(passed=True,physical_original=proof,physical_read_prefix=prefix_proof,normal_return_cases=sum(c['stop_kind']=='return'for c in cases),genuine_126200_entry_cases=sum(c['stop_kind']=='child'for c in cases),before_parent_read_cases=len(prefixes),native=results[0],forced_reference=results[1],production_copies=3,compiled_variants=4,
            authentication_rejections=rejected,compiler_command=command,original_parent_sites_executed=len(executed),compiled_bodies_sha256=compiled_bodies_pin,
            source_sha256={**source_pins,**{path:sha(ROOT/path)for path in ('include/halo2/native_tu15_node_resource_lifecycle_kernel.hpp','src/physics/runtime/native_tu15_node_resource_lifecycle.hpp','tools/native_node_resource_lifecycle_replacements.py','tools/validate_native_node_resource_lifecycle_tu15.py','tests/native_tu15_node_resource_lifecycle_tests.cpp')}},
            ordered_read_write_addresses_widths_verified=True,child_read_cursor_pcs_qualified=False,real_126200_return_closed=False,partial_memory_exception_abi_qualified=False)
        print(json.dumps(results[0]))
    except BaseException as error:report['error']=str(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
