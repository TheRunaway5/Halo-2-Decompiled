#!/usr/bin/env python3
"""Complete native16E6A0 ABI and staged memory/observer prefixes on real i386."""
import argparse
import copy
import gzip
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
from validate_native_tu15_prt_helpers import physical_original,PIN
from generated_routine_ownership import function_bodies
import native_effect_datum_allocate_replacements as binding

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x1000000,16384),(0x62000000,8192))
COPIES=('src/campaign/ai/source_ai_frame_tu15_generated.cpp',
        'src/campaign/source_level_handles_new_map_tu15_generated.cpp',
        'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',
        'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
        'src/physics/physics_live_support_tu15_generated.cpp')


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def metadata_with_stack_reads(image,original):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    result=original(image);code=common._Code(image,'.text')
    for site in result['sites']:
        instruction=code.at(site['instruction'])
        if site['instruction']==0x16e6c5:
            if bytes(instruction.bytes).hex()!='852c8b':raise ValueError('Original allocation bitmap TEST bytes changed')
            site['operands'][0]['read']=True
        if instruction.mnemonic in ('pop','ret'):
            if site['operands']:raise ValueError('Unexpected allocation POP/RET explicit memory')
            site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
    return result


def corpus():
    old=json.loads(gzip.decompress((ROOT/'tests/fixtures/native_tu15_data_array_allocate.json.gz').read_bytes()))
    rng=random.Random(0x16e6a0);rows=[]
    for row in old['cases']:
        if not row['completed']:continue
        n=len(rows);main=bytearray(rng.randbytes(16384));bank=bytes.fromhex(row['banks'][0]['before']);main[:len(bank)]=bank
        stack=bytearray(rng.randbytes(8192));gp=list(row['gp']);gp[4]=0x62001000+n%4;offset=gp[4]-0x62000000
        struct.pack_into('<I',stack,offset,0x63000000)
        rows.append(dict(id=n,name=row['name'],entry=0x16e6a0,gp=gp,eflags=0x202|(rng.getrandbits(12)&0x8d5),
            matrix=struct.unpack_from('<I',stack,offset+4)[0],source=struct.unpack_from('<I',stack,offset+8)[0],
            before_fp=row['before_fp'],live_x87=row['live_x87'],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=10000,
            banks=[dict(address=0x1000000,before=main.hex()),dict(address=0x62000000,before=stack.hex())]))
    for row in list(rows[:108:6])+[r for r in rows if r['name'].startswith('fault_seed')]:
        copied=copy.deepcopy(row);copied.update(id=len(rows),name=row['name']+'-DF1',eflags=row['eflags']|0x400);rows.append(copied)
    def directed(name,cursor=0,extent=0,capacity=8,stride=4,index=0,backwards=False,pool_offset=None,record_offset=None):
        ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];stack=0x62001000+ordinal%4
        pool=0x1000100 if pool_offset is None else stack-pool_offset
        record=0x1001000 if record_offset is None else stack-record_offset
        signed=lambda value:value if value<0x80000000 else value-0x100000000
        bitmap=(0x1000500-(signed(index)>>5)*4)&0xffffffff;records=(record-index*stride)&0xffffffff
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at and at+width<=base+size:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Directed allocation input escaped bank')
        def get(at):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
            raise ValueError('Directed allocation lookup escaped bank')
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack;gp[2]=pool
        for off,value in ((0x20,capacity),(0x24,stride),(0x34,cursor),(0x38,extent),(0x3c,0xffffffff),(0x44,records),(0x48,bitmap)):put(pool+off,value)
        put(pool+0x40,0xfffe,2)
        for off in (-4,0,4):put(0x1000500+off,0)
        if pool_offset==0x4c:gp[5]=records;gp[3]=bitmap;gp[6]=0x1234fffe;gp[7]=0xffffffff
        if pool_offset==0x50:gp[6]=records;gp[5]=bitmap;gp[7]=0x1234fffe
        if pool_offset==0x54:gp[7]=records;gp[6]=bitmap
        put(stack,0x63000000)
        flags=0x202|(rng.getrandbits(12)&0x8d5)|(0x400 if backwards else 0)
        rows.append(dict(id=ordinal,name=name,entry=0x16e6a0,gp=gp,eflags=flags,matrix=get(stack+4),source=get(stack+8),
            before_fp=[0x37f|((ordinal%4)<<10),((ordinal%8)<<11)|(ordinal%64),0x1f80|((ordinal%4)<<13)],
            live_x87=['0000000000000080ff3f'],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=10000,
            banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    for index,stride in ((0x40000000,4),(0x7ffffffe,3),(0x80000000,3)):
        for backwards in (False,True):directed(f'fault_seed_overflow_{index:x}_{stride}_{backwards}',cursor=index,extent=index,capacity=(index+1)&0xffffffff,stride=stride,index=index,backwards=backwards)
    for backwards in (False,True):
        directed(f'negative_hole_{backwards}',cursor=0x80000000,extent=0,capacity=8,stride=3,index=0x80000000,backwards=backwards)
        directed(f'negative_sentinel_append_{backwards}',cursor=0xffffffff,extent=0,capacity=8,stride=5,index=0,backwards=backwards)
        for offset in (0x4c,0x50,0x54):directed(f'fault_seed_saved_pool_{offset:x}_{backwards}',stride=5,pool_offset=offset,backwards=backwards)
        for offset in (16,12,8,4):directed(f'fault_seed_saved_record_{offset}_{backwards}',stride=4,record_offset=offset,backwards=backwards)
    return rows


def sources(scratch,require_binding):
    bound=[];rejected=0
    for name,path in zip(('frame','level','shadow','scene','primary'),COPIES):
        source=(ROOT/path).read_text();current=next(body for match,body in function_bodies(source)if match[1]=='16e6a0')
        original=binding.canonical_body(current);raw=source.replace(current,original,1)
        native=binding.install_native_effect_datum_allocate(raw)
        if require_binding and native!=source:raise ValueError('Missing native datum production route '+path)
        if binding.install_native_effect_datum_allocate(native)!=native:raise ValueError('Nonidempotent native datum route')
        selected=next((line for line in source.splitlines()if line.startswith('std::uint32_t selected(')),'')
        mini=binding.install_native_effect_datum_allocate('#include "physics/runtime/physics_world_runtime_internal.hpp"\n'+selected+'\n'+original)
        bound.append('namespace '+name+' {\n'+mini+'\n}\n')
        literal=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)
        for shape in (original,literal):
            candidate=raw.replace(original,shape,1);binding.install_native_effect_datum_allocate(candidate)
            for victim in (candidate,binding.install_native_effect_datum_allocate(candidate)):
                target=next(body for match,body in function_bodies(victim)if match[1]=='16e6a0')
                changed=target.replace('0x34u','0x38u',1)
                if changed==target:raise ValueError('Missing datum body drift witness')
                try:binding.install_native_effect_datum_allocate(victim.replace(target,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Accepted changed complete datum body')
        if 'selected('in original:
            changed=raw.replace('{if(image==SourcePhysicsContactImage::MapPack11)',
                '{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)',1)
            try:binding.install_native_effect_datum_allocate(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Accepted changed datum selected resolver')
    (scratch/'effect_datum_allocate_bodies.inc').write_text('\n'.join(bound))
    path=ROOT/'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp';source=path.read_text();native=binding.install_owner(source)
    if require_binding and source!=native:raise ValueError('Native datum Effect owner prefix missing')
    if binding.install_owner(native)!=native:raise ValueError('Nonidempotent native datum Effect owner')
    for shape in (binding.canonical_owner(binding.body(source,'allocate_effect_datum')),binding.body(native,'allocate_effect_datum')):
        changed=shape.replace('a.gp[2]+0x34','a.gp[2]+0x38',1)
        if changed==shape:raise ValueError('Missing datum Effect owner drift witness')
        try:binding.install_owner(source.replace(binding.body(source,'allocate_effect_datum'),changed,1))
        except ValueError:rejected+=1
        else:raise ValueError('Accepted changed complete datum Effect owner')
    for name in binding.ROUTE_PINS:
        route=binding.body(source,name);changed=route.replace('0x16e6a0u','0x16e6a1u',1)
        if changed==route:changed=route.replace('allocate_effect_datum(m,a,o,s)','allocate_effect_datum(m,a,s,o)',1)
        if changed==route:raise ValueError('Missing datum Effect route drift witness '+name)
        try:binding.install_owner(source.replace(route,changed,1))
        except ValueError:rejected+=1
        else:raise ValueError('Accepted changed datum Effect entry route '+name)
    (scratch/'effect.cpp').write_text(native)
    return rejected


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--normal-only',action='store_true');parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--compiler',default='c++');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in ('x86_64','amd64','i386','i686'):
        print('SKIP physical allocation ABI requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original image');return 77
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),candidate_unbound=not args.require_binding,retained_large_artifacts=[])
    try:
        rejected=sources(scratch,args.require_binding)
        sys.path.insert(0,str(ROOT/'tools'));from oracle_smg_firing_1_1 import Xbe
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original_metadata=common.metadata;common.metadata=lambda image:metadata_with_stack_reads(image,original_metadata)
        rows=corpus();physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x16e6a0,),regions=REGIONS,preserve_x87_prefix=True)
        executed=set().union(*(case['executed_sites']for case in physical['cases']))
        if len(executed)!=78:raise ValueError('Original allocation78-site coverage incomplete')
        prefix=[];by_id={row['id']:row for row in rows}
        if not args.normal_only:
            for seed in physical['cases']:
                original=by_id[seed['id']]
                if not original['name'].startswith('fault_seed')or '-DF1'in original['name']:continue
                for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                    for ordinal in range(len(events)):
                        row=copy.deepcopy(original);row.update(id=len(rows)+len(prefix),seed_id=original['id'],name=original['name']+'-'+phase+'-'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
        prefix_proof=None;all_observed=list(physical['cases'])
        if prefix:
            second=scratch/'prefix';second.mkdir();observed,prefix_proof=physical_original(Xbe(args.xbe),prefix,second,entries=(0x16e6a0,),regions=REGIONS,preserve_x87_prefix=True)
            all_observed+=observed['cases']
        inputs={row['id']:row for row in rows+prefix};cases=[];io_states={}
        for observed in all_observed:
            source=inputs[observed['id']]
            if 'seed_id'in source:
                fault=source['fault'];io_states.setdefault(source['seed_id'],{})[fault['phase']+'-'+str(fault['ordinal'])]=dict(gp=observed['boundary_gp'],eflags=observed['boundary_eflags'])
        for observed in all_observed:
            row=copy.deepcopy(inputs[observed['id']]);row.update({key:observed[key]for key in ('completed','boundary_gp','boundary_eflags','after_fp','reads','writes')})
            if observed['boundary_xmm']!=row['xmm']or not observed['live_x87_unchanged']or observed['after_fp']!=row['before_fp']:
                raise ValueError('Original integer allocation FP input/observation mismatch: '+json.dumps(dict(name=row['name'],before=row['before_fp'],after=observed['after_fp'],live_unchanged=observed['live_x87_unchanged'],xmm_unchanged=observed['boundary_xmm']==row['xmm'])))
            for before,after in zip(row['banks'],observed['banks'],strict=True):before['after']=after['after']
            cases.append(row)
        for row in cases:
            if row['id']in io_states:row['io_states']=io_states[row['id']]
        default_dir=scratch/'default-prefix';default_dir.mkdir();control=copy.deepcopy(rows[0]);control.update(id=1,fault=dict(phase='before_read',ordinal=0))
        defaults,_=physical_original(Xbe(args.xbe),[rows[0],control],default_dir,entries=(0x16e6a0,),regions=REGIONS)
        if [row['live_x87_unchanged']for row in defaults['cases']]!=[True,False]or any('prefix_x87_preservation_checked'in row for row in defaults['cases']):
            raise ValueError('Physical helper default prefix qualification changed')
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')))
        binary=scratch/'datum-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
            '-I'+str(ROOT/'src/gameplay/weapons'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_effect_datum_allocate_tests.cpp'),
            str(scratch/'effect.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compiled=subprocess.run(command,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Datum test compilation failed: '+compiled.stderr[-4000:])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
        if run.returncode:raise ValueError('Native datum ABI failed: '+run.stdout[-2000:]+run.stderr[-6000:])
        io_check=re.fullmatch(r'live_io (\d+)\n',run.stderr)
        if not io_check or int(io_check[1])<sum(len(row)for row in io_states.values())*4:
            raise ValueError('Native datum did not report its actual live I/O ABI checks')
        refusal=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,
            env=dict(os.environ,HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE='1',HALO2_EFFECT_DATUM_REFUSAL_TEST='1'))
        if refusal.returncode:raise ValueError('Forced datum reference failed: '+refusal.stderr[-3000:])
        reference=json.loads(refusal.stdout)
        if not reference['reference_admission_refused']or reference['literal_callbacks']<=0 or reference['last_literal_pc']!=0x16e760 or reference['returned_eax']!=cases[0]['boundary_gp'][0]:
            raise ValueError('Forced reference datum did not deliver measured refusal/callback/return evidence')
        paths=(*COPIES,'include/halo2/native_tu15_datum_allocate_kernel.hpp','include/halo2/native_tu15_datum_initialize_kernel.hpp','src/gameplay/weapons/native_tu15_effect_datum_allocate.hpp',
            'src/gameplay/weapons/native_tu15_effect_slot_scan.hpp','include/halo2/native_weapon_effect_birth_color_observer.hpp',
            'tests/native_tu15_effect_datum_allocate_tests.cpp','tools/validate_native_effect_datum_allocate_tu15.py',
            'tools/validate_native_tu15_prt_helpers.py','tests/fixtures/native_tu15_data_array_allocate.json.gz',
            'src/core/native_tu15_data_array_allocate.cpp','tools/validate_native_tu15_data_array_allocate.py',
            'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp','tools/native_effect_datum_allocate_replacements.py',
            'tools/native_weapon_effect_datum_bridge_tu15.py','tools/native_selected_operand_binding.py')
        report.update(passed=True,cases=len(cases),normal=len(rows),prefixes=len(prefix),physical_original=proof,physical_prefix=prefix_proof,
            source_sha256={path:digest(ROOT/path)for path in paths},before_write_rmw_read_prefix_claimed=False,
            full_prefix_routes=9,normal_observer_routes=2,comparisons=len(cases)*9+len(rows)*2,original_sites_executed=len(executed),
            native_instruction_callbacks=0,forced_reference=reference,rejected_mutations=rejected,live_x87_xmm_mxcsr_preserved=True,
            physical_prefix_logical80_topology_verified=bool(prefix),
            raw_live_io_gp_flags_packets=int(io_check[1]),compiler_command=command,
            generated_flags_view_during_live_memory_reads_qualified=False,physical_helper_default_regression_cases=2,
            opaque_mmx_transport_verified=True,physical_mmx_execution_verified=False,unknown_callback_fallback_cases=7,
            raw_world_fallback_exception_order_claimed=False,partial_instruction_write_faults_claimed=False)
        print(run.stdout.strip())
    except BaseException as error:report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':raise SystemExit(main())
