#!/usr/bin/env python3
"""Qualify typed vehicle choice DATA/ABI against unchanged original CPU owners."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
from validate_native_tu15_prt_helpers import PIN,physical_original
from native_vehicle_choice_data_replacements import replace_vehicle_choice_data,remove_vehicle_choice_data
from native_vehicle_choice_kernel_tu15 import strip_native_vehicle_choice,PINS
from generated_routine_ownership import function_bodies

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=(0x15cb60,0x15cc70,0x15cdf0,0x15cd30)
REGIONS=((0x4ec000,4096),(0x4f0000,16384),(0x61000000,16384),(0x62000000,16384))
FILES=('include/halo2/native_tu15_vehicle_choice_kernel.hpp','src/gameplay/objects/native_tu15_vehicle_choice.hpp',
       'tests/native_tu15_vehicle_choice_tests.cpp','tools/validate_native_tu15_vehicle_choice.py',
       'src/gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp','src/gameplay/vehicles/source_vehicle_compiled_symbols_tu15.cpp',
       'tools/native_vehicle_choice_data_replacements.py','tools/native_vehicle_choice_kernel_tu15.py',
       'src/gameplay/vehicles/native_vehicle_variant_symbols_tu15.hpp','tools/validate_native_tu15_prt_helpers.py')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def corpus(image):
    rng=random.Random(0x15cb60);rows=[]
    tables=[dict(address=at,before=image.read(at,count*4).hex())for at,count in((0x15cc50,7),(0x15cd1c,4),(0x15ced0,6),(0x15cddc,4))]
    for entry,fixed in zip(ENTRIES,(5,2,4,2)):
        for choice in(-128,-1,0,*range(1,fixed+4),127):
            for variant in range(4):
                n=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
                def put(at,value,width=4):
                    for (base,size),bank in zip(REGIONS,banks):
                        if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                    raise ValueError('Vehicle fixture escaped declared memory '+hex(at))
                stack=0x62002000+n%16;context=0x61000100;objects=0x61001000;definition=0x61000200;runtime=0x61000300;references=0x61000400;seed=0x61000500
                sink=0x61000600 if variant==0 else stack-4 if variant==1 else stack-8 if variant==2 else seed
                put(0x4ecc1c,context);put(context+0x16c,0xdead0002);put(0x4f0414,objects);put(objects+2*16+8,definition);put(definition+12,runtime);put(runtime+0x74,references)
                put(0x4f3cd8,seed);put(seed,rng.getrandbits(32))
                for index in range(11):put(references+index*8+4,0x12340000+index)
                if choice==fixed+1 and variant==3 and entry!=0x15cd30:
                    # Genuine LCG retries: only one authored candidate is
                    # resident; no draw/result is supplied by the fixture.
                    refs={0x15cb60:(0,1,6,5),0x15cc70:(2,3),0x15cdf0:(7,9,8,10)}[entry]
                    for index in refs[1:]:put(references+index*8+4,0xffffffff)
                    put(seed,0x89abcdef if entry==0x15cc70 else 0x12345678)
                    sink=0x61000600
                gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=0x61000700;gp[4]=stack
                put(stack,0x63000000);put(stack+4,0x61000800);put(stack+8,choice&0xff);put(stack+12,sink)
                live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
                rows.append(dict(id=n,name=f'{entry:x}-choice{choice}-alias{variant}',entry=entry,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64),0x1f80|((n%4)<<13)|(0x8040 if n%8>=4 else 0)],
                    live_x87=list(live[:(0,1,4,8)[n%4]]),matrix=0x61000800,source=choice&0xff,max_steps=10000,tables=tables,
                    banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--compiler',default='c++');p.add_argument('--prefixes',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if sha(args.xbe)!=PIN:raise ValueError('Vehicle original image authority changed')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);rows=corpus(image);pins={name:sha(ROOT/name)for name in FILES}
    kernel=(ROOT/FILES[0]).read_text()
    if any(token in kernel for token in('s.gp[','s.flags.','services.instruction','goto ')):raise ValueError('Vehicle DATA contains instruction-state execution')
    source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();bound=replace_vehicle_choice_data(source)
    production_source_sha256=hashlib.sha256(source.encode()).hexdigest()
    if args.require_binding and bound!=source:raise ValueError('Vehicle typed DATA default admission is absent')
    if replace_vehicle_choice_data(bound)!=bound or remove_vehicle_choice_data(bound)!=remove_vehicle_choice_data(source):raise ValueError('Vehicle DATA binding is not reversible/idempotent')
    control=strip_native_vehicle_choice(remove_vehicle_choice_data(bound));declared={f'{entry:x}'for entry in ENTRIES}
    parts=[]
    for name,text in(('installed',bound),('reference',control)):
        bodies=[body for signature,body in function_bodies(text)if signature[1]in declared]
        if len(bodies)!=4:raise ValueError('Vehicle proof lost a complete family')
        parts.append('namespace '+name+' {\nusing namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
            +'inline std::uint32_t sign_extend(std::uint32_t v,unsigned){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))));}\n'
            +'inline std::uint32_t canonical_branch(std::uint32_t pc,SourcePhysicsContactImage){return pc;}\n'+'\n'.join(bodies)
            +'\nvoid run(std::uint32_t entry,WorldState&s){switch(entry){'+''.join(f'case 0x{entry:x}u:run_{entry:x}(s);return;'for entry in ENTRIES)+'default:throw std::runtime_error("Unknown selector");}}\n}\n')
    (scratch/'vehicle_choice_bodies.inc').write_text('\n'.join(parts))
    mutations=0
    for entry,(family,pin)in PINS.items():
        body=next(body for signature,body in function_bodies(bound)if signature[1]==f'{entry:x}')
        for old,new in(('0x19660du','0x19660cu'),('0x4ecc1cu','0x4ecc18u')):
            changed=body.replace(old,new,1)
            try:replace_vehicle_choice_data(bound.replace(body,changed,1))
            except ValueError:mutations+=1
            else:raise ValueError('Vehicle DATA accepted changed complete owner')
    dispatcher_mutation=bound.replace('target==0x15cb60u','target==0x15cb61u',1)
    if dispatcher_mutation==bound:dispatcher_mutation=bound.replace('case 0x15cb60u:run_15cb60','case 0x15cb61u:run_15cb60',1)
    try:replace_vehicle_choice_data(dispatcher_mutation)
    except ValueError:mutations+=1
    else:raise ValueError('Vehicle DATA accepted wrong actual dispatcher target')
    binary=scratch/'vehicle-choice';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_vehicle_choice_tests.cpp'),str(ROOT/'src/gameplay/vehicles/source_vehicle_compiled_symbols_tu15.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    # Read-only full ABI packets at every genuine memory/PUSH/POP/RET site.
    # This does not change source instructions, registers or child outcomes.
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=ENTRIES;code=common._Code(image,'.text');original_metadata=common.metadata
    def complete_metadata(image):
        metadata=original_metadata(image)
        for site in metadata['sites']:
            # Capstone has no explicit memory operand for POP/RET. These real
            # implicit ESP reads belong in access-ordinal and ABI observations.
            if code.at(site['instruction']).mnemonic in('pop','ret'):
                site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return metadata
    common.metadata=complete_metadata;metadata=complete_metadata(image)
    observed=[dict(entry=s['instruction'],arguments=0,gp=True,xmm=True)for s in metadata['sites']if s['operands']or code.at(s['instruction']).mnemonic in('push','pop','ret','call')]
    original,proof=physical_original(image,rows,scratch,entries=ENTRIES,regions=REGIONS,trace_calls=observed,trace_extended=True,preserve_x87_prefix=True)
    executed=set().union(*(set(case['executed_sites'])for case in original['cases']))
    missing={site['instruction']for site in metadata['sites']}-executed
    if missing:raise ValueError('Vehicle corpus missed original sites '+','.join(hex(at)for at in sorted(missing)))
    seed_store={0x15cc23,0x15ccf3,0x15cea3,0x15cdb3}
    draws=[sum(event['instruction']in seed_store for event in case['writes'])for case in original['cases']]
    retry_families={row['entry']for row,count in zip(rows,draws,strict=True)if count>1}
    if retry_families!={0x15cb60,0x15cc70,0x15cdf0}:raise ValueError('Vehicle retry families lack genuine positive coverage')
    def compare(rows,cases,label,*,full_io=True,reference_mode=False):
        inp=scratch/(label+'.in.json');out=scratch/(label+'.out.json');inp.write_text(json.dumps(rows,separators=(',',':')))
        environment=dict(os.environ)
        if reference_mode:environment['HALO2_REFERENCE_VEHICLE_CHOICE']='1'
        subprocess.run([str(binary),str(inp),str(out)],check=True,env=environment);native=json.loads(out.read_text())
        for row,a,b in zip(rows,native,cases,strict=True):
            for key in('completed','boundary_gp','boundary_eflags','boundary_xmm','after_fp','banks','writes'):
                if a[key]!=b[key]:raise ValueError(f'Vehicle {row["name"]} {key} mismatch: native={str(a[key])[:400]} physical={str(b[key])[:400]}')
            # The code-section jump table is immutable; physical retained-bank
            # access metadata intentionally excludes that original text page.
            actual_reads=[r for r in a['reads']if not any(t['address']<=r['address']<t['address']+len(t['before'])//2 for t in row['tables'])]
            if not row.get('observer')and actual_reads!=b['reads']:
                first=next((n for n,(x,y)in enumerate(zip(actual_reads,b['reads']))if x!=y),min(len(actual_reads),len(b['reads'])))
                raise ValueError(f'Vehicle {row["name"]} ordered reads mismatch at{first}: native={actual_reads[first:first+2]} physical={b["reads"][first:first+2]} lengths={len(actual_reads)}/{len(b["reads"])}')
            expected=[{k:v for k,v in event.items()if k!='arguments'}for event in b['calls']]
            if full_io and a['calls']!=expected:
                first=next((n for n,(x,y)in enumerate(zip(a['calls'],expected))if x!=y),min(len(a['calls']),len(expected)))
                raise ValueError(f'Vehicle {row["name"]} I/O ABI packet{first} mismatch: native={a["calls"][first:first+1]} physical={expected[first:first+1]}')
        return sha(out),sum(len(c['calls'])for c in cases)
    comparison,packets=compare(rows,original['cases'],'normal')
    installed=[dict(row,route=1)for row in rows];compare(installed,original['cases'],'installed')
    compare(installed,original['cases'],'forced-reference',full_io=False,reference_mode=True)
    compare([dict(row,route=2)for row in rows],original['cases'],'literal-reference',full_io=False)
    compare([dict(row,route=1,generic=True)for row in rows],original['cases'],'generic-observations',full_io=False)
    literal=json.loads((scratch/'literal-reference.out.json').read_text());generic=json.loads((scratch/'generic-observations.out.json').read_text())
    if any(a['trace']!=b['trace']for a,b in zip(literal,generic,strict=True)):raise ValueError('Vehicle generic observer lost forwarded source observations')
    prefixes=0
    if args.prefixes:
        expanded=[]
        for row,case in zip(rows,original['cases'],strict=True):
            if not row['name'].endswith('alias3'):continue
            for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                for ordinal in range(len(case[key])):
                    request=copy.deepcopy(row);request['id']=len(expanded);request['name']+=f'-{phase}{ordinal}';request['fault']=dict(phase=phase,ordinal=ordinal);request['observer']=phase=='after_write';expanded.append(request)
        private=scratch/'prefixes';private.mkdir()
        original_prefix,prefix_proof=physical_original(image,expanded,private,entries=ENTRIES,regions=REGIONS,trace_calls=observed,trace_extended=True,preserve_x87_prefix=True)
        atomic=[n for n,row in enumerate(expanded)if not row['observer']];observed_prefix=[n for n,row in enumerate(expanded)if row['observer']]
        compare([expanded[n]for n in atomic],[original_prefix['cases'][n]for n in atomic],'atomic-prefixes')
        compare([expanded[n]for n in observed_prefix],[original_prefix['cases'][n]for n in observed_prefix],'observer-prefixes',full_io=False)
        prefixes=len(expanded);proof['prefix_original_instructions']=prefix_proof['original_instructions']
    # An independently rebuilt wrong LCG must disagree with original-backed
    # expected banks/results. No original instruction or outcome is replaced.
    mutant=scratch/'mutant/halo2';mutant.mkdir(parents=True);target=mutant/'native_tu15_vehicle_choice_kernel.hpp'
    needle='data.seed*0x19660du+0x3c6ef35fu'
    if kernel.count(needle)!=1:raise ValueError('Vehicle LCG negative-control seam changed')
    target.write_text(kernel.replace(needle,'data.seed*0x19660cu+0x3c6ef35fu',1))
    mutant_binary=scratch/'vehicle-choice-mutant';mutant_command=[args.compiler,'-std=c++20','-O0','-I'+str(mutant.parent),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_vehicle_choice_tests.cpp'),str(ROOT/'src/gameplay/vehicles/source_vehicle_compiled_symbols_tu15.cpp'),'-o',str(mutant_binary)]
    subprocess.run(mutant_command,check=True);mutant_output=scratch/'mutant.out.json'
    subprocess.run([str(mutant_binary),str(scratch/'normal.in.json'),str(mutant_output)],check=True)
    wrong=json.loads(mutant_output.read_text());semantic_rejected=sum(a['banks']!=b['banks']or a['boundary_gp']!=b['boundary_gp']for a,b in zip(wrong,original['cases'],strict=True))
    if not semantic_rejected:raise ValueError('Vehicle original oracle accepted an incorrect LCG')
    if any(sha(ROOT/name)!=pin for name,pin in pins.items()):raise ValueError('Vehicle proof sources changed while running')
    if (ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()!=source:raise ValueError('Vehicle production source changed after extraction')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=True,cases=len(rows),physical_prefixes=prefixes,full_io_abi_packets=packets,original_xbe_sha256=PIN,physical_original=proof,
        normal_comparison_sha256=comparison,source_sha256=pins,source_unchanged=True,native_instruction_callbacks=0,ordered_reads_stores_verified=True,real_compiled_symbol_owner=True,
        instruction_shaped_reference_adapter_still_available=True,production_binding_installed=bound==source,reference_mode_and_literal_comparisons=len(rows)*2,default_body_comparisons=len(rows),rejected_mutations=mutations,
        native_normal_comparisons=len(rows)*2,generic_observation_cases=len(rows),admission_refusals=10,semantic_lcg_mutation_rejected=True,semantic_lcg_wrong_cases=semantic_rejected,
        executed_original_sites=len(executed),original_sites=len(metadata['sites']),genuine_lcg_draws=sum(draws),genuine_retry_cases=sum(n>1 for n in draws),
        original_body_pins={hex(entry):pin for entry,(family,pin)in PINS.items()},scratch_owner=str(scratch),
        production_source_sha256=production_source_sha256,
        actual_page_fault_atomicity_verified=False,observer_preimage_read_faults_verified=False,mcc_symbol_lease_verified=False,retained_large_artifacts=[]),indent=2,sort_keys=True)+'\n')
    print(f'PASS typed vehicle choice {len(rows)} physical cases/{prefixes} prefixes/{packets} full I/O ABI packets');return 0
if __name__=='__main__':raise SystemExit(main())
