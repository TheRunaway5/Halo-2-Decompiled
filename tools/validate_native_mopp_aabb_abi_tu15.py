#!/usr/bin/env python3
"""Qualify the native309940 caller-frame adapter on unchanged TU1.5 hardware."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys
import re
ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if(seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
from validate_native_tu15_prt_helpers import physical_original,PIN
from validate_native_mopp_aabb_traversal_tu15 import cases as data_cases
from native_mopp_aabb_replacements import BODY,PREFIX,OWNER_PINS,canonical_body,ordered_body,replace_mopp_aabb
import native_mopp_aabb_canonical_replacements as canonical_owner

REGIONS=((0x1000000,0x3000),(0x62000000,0x8000),(0x48c000,0x1000))


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def corpus(image):
    rng=random.Random(0x309940);rows=[]
    live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f',
                 '00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
    for seed in data_cases(image):
        ordinal=len(rows);row=copy.deepcopy(seed);stack=bytearray(rng.randbytes(REGIONS[1][1]));globals_=bytearray(rng.randbytes(REGIONS[2][1]));sp=0x62006000+ordinal%16
        def put(p,value,width=4):
            for bank in row['banks']:
                at=bank['address'];data=bytearray.fromhex(bank['before'])
                if at<=p and p+width<=at+len(data):data[p-at:p-at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');bank['before']=data.hex();return
            if REGIONS[1][0]<=p<REGIONS[1][0]+len(stack):stack[p-REGIONS[1][0]:p-REGIONS[1][0]+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
            raise ValueError('MOPP ABI input escaped bank')
        put(sp,0x63000000);put(sp+4,row['variables']);put(sp+8,row['code'])
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=row['receiver'];gp[4]=sp
        row.update(id=ordinal,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),matrix=row['variables'],source=row['code'],max_steps=10000,stop_pc=0x63000000,
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=list(live_values[:(0,1,4,8)[ordinal%4]]),
            stop_before_entries=[0x2e3250,0x2e3080,0x2e30e0])
        row['banks']+=[dict(address=REGIONS[1][0],before=stack.hex()),dict(address=REGIONS[2][0],before=globals_.hex())]
        rows.append(row)
    for n in range(4):
        row=copy.deepcopy(rows[-1]);row.update(id=len(rows),name='genuine_growth_entry_'+str(n));data=bytearray.fromhex(row['banks'][0]['before']);data[0x304:0x308]=(n).to_bytes(4,'little');data[0x308:0x30c]=(0x80000000|n).to_bytes(4,'little');row['banks'][0]['before']=data.hex();rows.append(row)
    def put(row,p,value,width=4):
        for bank in row['banks']:
            base=bank['address'];data=bytearray.fromhex(bank['before'])
            if base<=p and p+width<=base+len(data):data[p-base:p-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');bank['before']=data.hex();return
        raise ValueError('Directed MOPP input escaped bank')
    def program(row,values,address=0x1001000):
        for n,value in enumerate(values):put(row,address+n,value,1)
    def directed(name,values):
        row=copy.deepcopy(rows[0]);row.update(id=len(rows),name=name);row['gp'][4]=0x62006000+(len(rows)%16)
        put(row,row['gp'][4],0x63000000);put(row,row['gp'][4]+4,row['variables']);put(row,row['gp'][4]+8,row['code']);program(row,values);return row
    for op in (8,12,0x1d,0x2c,0x54,0x5f,0x6c,0xff):
        row=directed(f'diagnostic_entry_{op:x}',[op]);put(row,0x48c99c,0x1002800);put(row,0x1002800,0x1002900);put(row,0x1002904,0x2e3080);rows.append(row)
    for depth in (2,8,32,64):
        values=[0x30]
        for _ in range(depth):values=[0x20,128,len(values),*values,0x30]
        row=directed(f'recursive_depth_{depth}',values);put(row,0x1000308,0x80000080);rows.append(row)
    for op in (0x10,0x13):
      for alias in ('program','variables','cache','jump'):
        row=directed(f'recursive_saved_{alias}_{op:x}',[op,64,192,1,0x30,0x31,0x32]);sp=row['gp'][4]
        target=0x1001800 if alias=='program'else 0x1002080 if alias=='variables'else 0x1000180 if alias=='cache'else 2
        put(row,0x1000300,sp-{'program':0x90,'variables':0x98,'cache':0x94,'jump':0x9c}[alias]);put(row,0x100202c,target)
        if alias=='program':program(row,[0x32,0x33,0],target)
        if alias=='variables':
            data=bytes.fromhex(row['banks'][0]['before']);start=row['variables']-0x1000000
            for n,value in enumerate(data[start:start+56]):put(row,target+n,value,1)
            put(row,target+0x2c,0xbeef00)
        if alias=='cache':
            data=bytes.fromhex(row['banks'][0]['before']);start=0x1000100-0x1000000
            for n,value in enumerate(data[start:start+64]):put(row,target+n,value,1)
        rows.append(row)
    for op in (0x62,0x63,0x66,0x67,0x6a,0x6b,0x69):
        count=1 if op<0x64 else 2 if op<0x68 else 4;value=0x63000000 if op==0x69 else 0xa1b2c3d4&((1<<(count*8))-1)
        row=directed(f'caller_property_slot_{op:x}',[op,*value.to_bytes(count,'big'),0]);rows.append(row)
    for op in (0x61,0x65):
        count=1 if op==0x61 else 2;row=directed(f'changed_return_owner_{op:x}',[op,*([0xa5]*count),0]);row['stop_before_entries'].append(0x309e9e);rows.append(row)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--compiler',default='c++');p.add_argument('--require-binding',action='store_true');p.add_argument('--require-canonical-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,parent_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    proof_files=('include/halo2/native_tu15_mopp_aabb_traversal_kernel.hpp','include/halo2/native_tu15_mopp_aabb_projection_kernel.hpp','src/physics/runtime/native_tu15_mopp_aabb_traversal.hpp','tests/native_tu15_mopp_aabb_abi_tests.cpp','tools/validate_native_mopp_aabb_abi_tu15.py','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','tools/native_mopp_aabb_replacements.py','src/physics/runtime/physics_world_runtime_generated.cpp','tools/native_mopp_aabb_canonical_replacements.py','src/physics/runtime/physics_contact_runtime_generated.cpp','tools/specialize_native_operands.py')
    source_before={name:sha(ROOT/name)for name in proof_files};report['source_before']=source_before
    try:
        source=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text();bound=replace_mopp_aabb(source)
        if args.require_binding and bound!=source:raise ValueError('Production MOPP prefix is not installed')
        if replace_mopp_aabb(bound)!=bound:raise ValueError('MOPP prefix is not idempotent')
        rejected=0
        for variant in (source,bound):
            body=BODY.search(variant)[0]
            for before,after in (('0x38u,4','0x34u,4'),('0x6bu,4','0x6au,4'),('0x309f24u','0x309f25u')):
                changed=body.replace(before,after,1)
                if changed==body:raise ValueError('Missing whole-body mutation witness')
                try:replace_mopp_aabb(variant.replace(body,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Changed whole MOPP body was admitted')
            for pattern in OWNER_PINS:
                owner=re.search(pattern,variant,re.S)[0];changed=owner.replace('return','return ',1)
                if changed==owner:changed=owner+' '
                try:replace_mopp_aabb(variant.replace(owner,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Changed whole MOPP supporting owner was admitted')
        from specialize_native_operands import specialize
        specialized,receipt=specialize(bound);again,_=specialize(specialized)
        if specialized!=again:raise ValueError('MOPP specialization is not idempotent')
        owners='\n'.join(re.search(pattern,source,re.S)[0]for pattern in list(OWNER_PINS)[:2])
        owners+='\nstd::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,const ObservedWorldState&s){return s.selected_observation_pcs&&s.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;}\n'
        generated='namespace copies {\ninline unsigned fallback_calls;\ninline std::function<void(ObservedWorldState&,std::uint32_t,std::uint32_t,bool)> owner;\n'+owners+'\ninline void run_2df780(ObservedWorldState&s){owner(s,0x2e3250u,0x30640du,false);}\n'
        for name,body in (('bound',BODY.search(bound)[0]),('specialized',BODY.search(specialized)[0]),('reference',ordered_body(canonical_body(BODY.search(source)[0])))):
            if name!='reference':body=body.replace(PREFIX,PREFIX+'\n++copies::fallback_calls;',1)
            generated+='namespace '+name+' {\nstatic void dispatch(ObservedWorldState&,std::uint32_t,std::uint32_t,bool);\n'+body+'\nstatic void dispatch(ObservedWorldState&s,std::uint32_t target,std::uint32_t site,bool tail){if(target==selected(0x305e40u,s.image)){run_305e40(s);return;}copies::owner(s,target,site,tail);}\n}\n'
        generated+='}\n'
        canonical_source=(ROOT/'src/physics/runtime/physics_contact_runtime_generated.cpp').read_text();canonical_bound=canonical_owner.replace_mopp_aabb_canonical(canonical_source)
        if args.require_canonical_binding and canonical_bound!=canonical_source:raise ValueError('Canonical-State MOPP prefix is not installed')
        if canonical_owner.replace_mopp_aabb_canonical(canonical_bound)!=canonical_bound:raise ValueError('Canonical-State MOPP binding is not idempotent')
        # This is a distinct complete owning context; equal numerical layout is
        # checked against the original complete body, never inferred by name.
        original_world=canonical_body(BODY.search(source)[0]).replace('ObservedWorldState','State')
        original_world=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)',r'\1',original_world)
        if original_world!=canonical_owner.canonical_body(canonical_owner.BODY.search(canonical_source)[0]):raise ValueError('Canonical-State MOPP semantics differ from the qualified complete World body')
        for variant in(canonical_source,canonical_bound):
            body=canonical_owner.BODY.search(variant)[0]
            for old,new in(('0x38u,4','0x34u,4'),('0x6bu,4','0x6au,4'),('0x306424u','0x306425u')):
                changed=body.replace(old,new,1)
                if changed==body:raise ValueError('Missing canonical-State body mutation')
                try:canonical_owner.replace_mopp_aabb_canonical(variant.replace(body,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Changed complete canonical-State MOPP body admitted')
            for pattern in canonical_owner.OWNER_PINS:
                owner=re.search(pattern,variant,re.S)[0];changed=owner.replace('return','return ',1)
                try:canonical_owner.replace_mopp_aabb_canonical(variant.replace(owner,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Changed complete canonical-State MOPP owner admitted')
        canonical_specialized,canonical_receipt=specialize(canonical_bound);again,_=specialize(canonical_specialized)
        if again!=canonical_specialized or canonical_receipt['native_mopp_aabb_canonical_entries']!=['309940']:raise ValueError('Canonical-State MOPP specialized routing changed')
        owners='\n'.join(re.search(pattern,canonical_source,re.S)[0]for pattern in list(canonical_owner.OWNER_PINS)[:2])
        generated+='namespace canonical_copies {\nusing State=halo2::physics_contact_runtime_detail::State;\ninline unsigned fallback_calls;\ninline std::function<void(State&,std::uint32_t,std::uint32_t,bool)> owner;\n'+owners+'\ninline void run_2df780(State&s){owner(s,0x2e3250u,0x30640du,false);}\n'
        for name,body in(('bound',canonical_owner.BODY.search(canonical_bound)[0]),('specialized',canonical_owner.BODY.search(canonical_specialized)[0]),('reference',ordered_body(canonical_owner.canonical_body(canonical_owner.BODY.search(canonical_source)[0])))):
            if name!='reference':body=body.replace(canonical_owner.PREFIX,canonical_owner.PREFIX+'\n++canonical_copies::fallback_calls;',1)
            generated+='namespace '+name+' {\nstatic void dispatch(State&,std::uint32_t,std::uint32_t,bool);\n'+body+'\nstatic void dispatch(State&s,std::uint32_t target,std::uint32_t site,bool tail){if(target==selected(0x305e40u,s.image)){run_305e40(s);return;}canonical_copies::owner(s,target,site,tail);}\n}\n'
        (scratch/'mopp_aabb_bodies.inc').write_text(generated+'}\n')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from oracle_smg_firing_1_1 import Xbe
        image=Xbe(args.xbe);common.ENTRIES=(0x309940,);metadata=common.metadata(image);code=common._Code(image,'.text');io=[];old_metadata=common.metadata
        def amended(image):
            result=old_metadata(image)
            for site in result['sites']:
                instruction=code.at(site['instruction'])
                if instruction.mnemonic in ('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=amended
        for site in metadata['sites']:
            pc=site['instruction'];ins=code.at(pc)
            if pc in (0x309960,0x309967):continue
            if site['operands']or ins.mnemonic in ('push','pop','ret','call')or pc==0x309940:io.append(pc)
        rows=corpus(image);physical,proof=physical_original(image,rows,scratch,entries=(0x309940,),regions=REGIONS,
            trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
        # Each distinct physical read site, both DF domains, and both ends of
        # repeated reads get an independently stopped reference execution.
        # Stack-alias cases additionally stop every actual read, so no native
        # result or continuation is supplied by the reference fixture.
        prefixes=[];witnesses=set()
        for row,case in zip(rows,physical['cases'],strict=True):
            last={read['instruction']:n for n,read in enumerate(case['reads'])}
            first={}
            for n,read in enumerate(case['reads']):first.setdefault(read['instruction'],n)
            for n,read in enumerate(case['reads']):
                key=(read['instruction'],read['bytes'],bool(row['eflags']&1024),n==last[read['instruction']])
                alias=row['name'].startswith(('recursive_saved_','caller_property_','changed_return_owner_'))
                if not alias and(n not in (first[read['instruction']],last[read['instruction']])or key in witnesses):continue
                witnesses.add(key);prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_before_read_{n}',fault=dict(phase='before_read',ordinal=n));prefixes.append(prefix)
        read_prefix_count=len(prefixes)
        operand_sites={site['instruction']:site for site in amended(image)['sites']}
        write_witnesses=set()
        for row,case in zip(rows,physical['cases'],strict=True):
            first={};last={write['instruction']:n for n,write in enumerate(case['writes'])}
            for n,write in enumerate(case['writes']):first.setdefault(write['instruction'],n)
            for n,write in enumerate(case['writes']):
              pc=write['instruction']
              for phase in ('before_write','after_write'):
                # An interrupted RMW/REP has a finer memory-provider contract
                # than a stopped pre-instruction observer. Its completed write
                # is still qualified by the after-write/observer gate below.
                if phase=='before_write'and any(op['read']for op in operand_sites[pc]['operands']):continue
                key=(phase,pc,bool(row['eflags']&1024),n==last[pc])
                alias=row['name'].startswith(('recursive_saved_','caller_property_','changed_return_owner_'))
                if not alias and(n not in(first[pc],last[pc])or key in write_witnesses):continue
                write_witnesses.add(key);prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{n}',fault=dict(phase=phase,ordinal=n));prefixes.append(prefix)
        prefix_scratch=scratch/'read-prefixes';prefix_scratch.mkdir()
        prefix_physical,prefix_proof=physical_original(image,prefixes,prefix_scratch,entries=(0x309940,),regions=REGIONS,
            trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
        rows+=prefixes;physical['cases']+=prefix_physical['cases']
        executed=set();packet_count=0
        for row,case in zip(rows,physical['cases'],strict=True):
            executed.update(case['executed_sites']);row.update({key:case[key]for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes')})
            # An empty REP has no memory operand and hence no pre-I/O packet.
            row['calls']=[event for event in case['calls']if not(code.at(event['entry']).mnemonic.startswith('rep')and not event['gp'][1])]
            for event in row['calls']:event.pop('arguments');packet_count+=1
            stopped=case['stopped_at'];kind=row['fault']['phase']if row.get('fault')else'return'if stopped==0x63000000 else'before_return'if stopped==0x309e9e else'child'
            row.update(stop_kind=kind,expected_pc=case['boundary']['instruction']if row.get('fault')else stopped if kind!='return'else(0x309f2c if 0x309f2c in case['executed_sites']else 0x309e9e))
            after={bank['address']:bank['after']for bank in case['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'mopp-aabb-abi.json';fixture.write_text(json.dumps(dict(cases=rows,io_sites=io,reference_tables=dict(address=0x309f30,bytes=image.read(0x309f30,0xec).hex())),separators=(',',':')))
        binary=scratch/'mopp-aabb-abi';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_mopp_aabb_abi_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compiled=subprocess.run(command,capture_output=True,text=True,timeout=300)
        if compiled.returncode:raise ValueError('MOPP ABI compilation failed: '+compiled.stderr[:7000])
        env=dict(os.environ);env.pop('HALO2_REFERENCE_MOPP_AABB',None)
        modes={}
        for mode,forced in (('native',False),('reference',True)):
            current_env=dict(env)
            if forced:current_env['HALO2_REFERENCE_MOPP_AABB']='1'
            run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=current_env,timeout=150)
            if run.returncode:raise ValueError(mode+' MOPP ABI comparison failed: '+run.stderr[:16000])
            modes[mode]=json.loads(run.stdout)
            report['partial_modes']=modes
        source_after={name:sha(ROOT/name)for name in proof_files}
        if source_after!=source_before:raise ValueError('MOPP proof source drifted during compilation/comparison')
        report.update(passed=True,result=modes,modes=modes,authentication_rejections=rejected,specialization_receipt=receipt,canonical_specialization_receipt=canonical_receipt,parent_bound=args.require_binding,canonical_parent_bound=args.require_canonical_binding,physical_original=proof,physical_cases=len(rows),pre_io_packets=packet_count,source_after=source_after,
            physical_fault_prefixes=prefix_proof,read_prefix_cases=read_prefix_count,
            write_prefix_cases=sum(row['fault']['phase']=='before_write'for row in prefixes),
            observer_prefix_cases=sum(row['fault']['phase']=='after_write'for row in prefixes),
            executed_sites=sorted(executed),body_sites=464,compiler_command=command,
            sources=source_before)
    except Exception as error:report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,result=report['result'])));return 0


if __name__=='__main__':raise SystemExit(main())
