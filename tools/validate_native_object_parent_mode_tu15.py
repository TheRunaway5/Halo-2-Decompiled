#!/usr/bin/env python3
"""Private FC510/FB5F0 parent-mode kernel against unchanged original i386."""
import argparse,copy,hashlib,json,os,random,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from validate_native_tu15_prt_helpers import physical_original
REGIONS=((0x4e7000,4096),(0x4ec000,4096),(0x61000000,4096),(0x62000000,4096))
OLD_PIN='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
INPUTS=('include/halo2/native_tu15_object_parent_mode_kernel.hpp','include/halo2/native_tu15_object_parent_mode_value.hpp','src/gameplay/weapons/native_tu15_object_parent_mode.hpp','src/gameplay/weapons/native_tu15_object_parent_mode_admission.hpp','tests/native_tu15_object_parent_mode_tests.cpp','tools/validate_native_object_parent_mode_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
INPUTS+=('tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/generate_physics_live_support_tu15.py','tools/native_model_marker_lookup_replacements.py','tools/native_halfword_bit_query_replacements.py','tools/native_selected_operand_binding.py')
INPUTS+=('tools/native_object_parent_mode_replacements.py','tools/native_object_parent_mode_inputs.py','tools/native_object_parent_mode_mutations.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(image,smoke):
    rng=random.Random(0xfc510);rows=[]
    live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
    normal_count=16 if smoke else 256
    for case in range(normal_count+(4 if smoke else 16)):
        flags=case if case<normal_count else(case-normal_count)*17
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        variant=case%16;sp=0x62000100+variant;objects=0x61000020+variant;slot=0x61000100+variant;record=0x61000200+variant
        datum=(rng.getrandbits(16)<<16)|rng.choice((0,1,7,65535))
        rows_base=(slot-8-(datum&65535)*12)&0xffffffff
        if case>=normal_count:record=sp-0x12c
        def put(address,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Parent-mode input escaped bank')
        def get(address):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-4:return int.from_bytes(bank[address-base:address-base+4],'little')
            raise ValueError('Parent-mode stack escaped bank')
        put(0x4ecbd0 if image=='TitleUpdate15'else 0x4e78d0,objects);put(objects+0x44,rows_base);put(slot,record);put(record+0x12c,flags,1);put(sp,0x63000000)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=datum;gp[4]=sp
        depth=(0,1,4,8)[flags%4];cw=(0x7f,0x27f,0x37f)[flags%3]|((flags//3%4)<<10)
        rows.append(dict(id=len(rows),name=f'{image}-flags{flags:02x}'+('-return-alias'if case>=normal_count else''),image=image,entry=0xfc510 if image=='TitleUpdate15'else 0xfb5f0,gp=gp,eflags=0x202|rng.choice((0,1,0x10,0x40,0x80,0x800,0x8d5))|(0x400 if flags&1 else 0),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,((flags%8)<<11)|0x4500|(flags*7&0x3f),0x1f80|((flags%4)<<13)],live_x87=list(live[:depth]),matrix=get(sp+4),source=get(sp+8),max_steps=100,banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--mmx',action='store_true');parser.add_argument('--source-copies',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT);args=parser.parse_args()
    if args.require_binding and not args.source_copies:parser.error('--require-binding requires --source-copies')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    pins={name:sha(ROOT/name)for name in INPUTS};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_unbound=not args.require_binding,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        if args.source_copies:
            from native_object_parent_mode_inputs import prepare
            report['actual_source_ownership']=prepare(ROOT,scratch,args.require_binding)
            pins.update({name:sha(ROOT/name)for name in(*report['actual_source_ownership']['source_files'],'tools/native_object_parent_mode_replacements.py','tools/native_object_parent_mode_inputs.py','tools/specialize_native_operands.py')})
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        metadata_original=common.metadata
        def metadata(image):
            result=metadata_original(image);code=common._Code(image,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;all_rows=[];authority=[];sites_all=set()
        for name,path,pin,entry in(('TitleUpdate15',args.xbe,NEW_PIN,0xfc510),('MapPack11',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',OLD_PIN,0xfb5f0)):
            assert sha(path)==pin;image=Xbe(path);common.ENTRIES=(entry,);sites=metadata(image)['sites'];io=[site['instruction']for site in sites if site['operands']]
            folder=scratch/name;folder.mkdir();normal=folder/'normal';normal.mkdir();rows=corpus(name,args.smoke);normal_count=len(rows)
            actual,proof=physical_original(image,rows,normal,entries=(entry,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
            realized={row['boundary_gp'][2]&255 for row in actual['cases']}
            if not args.smoke and realized!=set(range(256)):raise ValueError('Original source did not load all256 flag bytes')
            proof['source_realized_flag_bytes']=sorted(realized)
            if args.prefixes:
                prefix=[]
                for index in range(min(32,len(rows))):
                    for ordinal,_ in enumerate(actual['cases'][index]['reads']):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefix),name=row['name']+f'-before-read{ordinal}',fault=dict(phase='before_read',ordinal=ordinal));prefix.append(row)
                target=folder/'prefixes';target.mkdir();stops,stopped_proof=physical_original(image,prefix,target,entries=(entry,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
                rows+=prefix;actual['cases']+=stops['cases'];proof['before_read_prefixes']=stopped_proof
            if args.mmx:
                from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
                opaque=[];rng=random.Random(0x2015)
                for mask in range(256):
                    row=copy.deepcopy(rows[mask%normal_count]);row.update(id=len(rows)+len(opaque),name=row['name']+f'-opaque{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800
                    row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);opaque.append(row)
                target=folder/'opaque';target.mkdir();observed,opaque_proof=physical_original_mmx(image,opaque,target,entries=(entry,),regions=REGIONS+((PROBE,4096),),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
                rows+=opaque;actual['cases']+=observed['cases'];proof['opaque_mmx']=opaque_proof
            for row,observed in zip(rows,actual['cases'],strict=True):
                assert observed['completed']==('fault'not in row);sites_all.update(observed['executed_sites'])
                pc=observed['boundary']['instruction']if row.get('fault')else observed['executed_sites'][-1]
                row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',row['live_x87']))
                if 'active_mmx'in row:row['expected']['active_mmx']=observed['active_mmx']
                row.update(reads=observed['reads'],calls=observed['calls']);assert not observed['writes']
                for packet in row['calls']:assert packet.pop('arguments')==[]
                for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
            all_rows+=rows;authority.append(dict(image=name,image_sha256=pin,proof=proof))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=all_rows),separators=(',',':')));outcomes=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_object_parent_mode_tests.cpp'),'-o',str(binary)]
            if args.source_copies:
                command+=['-DHALO2_OBJECT_PARENT_SOURCE_COPIES=1','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(scratch),'-I'+str(ROOT/'src/gameplay/weapons'),str(scratch/'parent_mode_effect.cpp'),str(scratch/'parent_mode_public.cpp'),str(scratch/'parent_mode_public_reference.cpp')]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,result.stderr[-7000:]
            sources=[ROOT/'tests/native_tu15_object_parent_mode_tests.cpp']
            if args.source_copies:sources +=[scratch/'parent_mode_effect.cpp',scratch/'parent_mode_public.cpp',scratch/'parent_mode_public_reference.cpp']
            for source in sources:
                dep=scratch/(Path(compiler).name+'-'+source.name+'.d')
                syntax=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),'-I'+str(ROOT/'src/gameplay/weapons'),'-MD','-MF',str(dep),str(source)]
                if args.source_copies:syntax.append('-DHALO2_OBJECT_PARENT_SOURCE_COPIES=1')
                checked=subprocess.run(syntax,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-7000:]
                for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(value).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-10000:]
            checks=[]
            if args.source_copies:
                reference=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,HALO2_REFERENCE_OBJECT_PARENT_MODE='1',HALO2_OBJECT_PARENT_REFERENCE_TEST='1'))
                assert reference.returncode==0,(reference.stdout+reference.stderr)[-10000:]
                checks.append(dict(positive_actual_source_reference=json.loads(reference.stdout)))
                guarded=subprocess.run([str(binary),'--source-guards'],capture_output=True,text=True);assert guarded.returncode==0,(guarded.stdout+guarded.stderr)[-6000:]
                checks.append(dict(untouched_actual_source_fallbacks=json.loads(guarded.stdout)))
                empty=subprocess.run([str(binary),'--empty-services',str(fixture)],capture_output=True,text=True)
                assert empty.returncode==0,(empty.stdout+empty.stderr)[-10000:]
                checks.append(dict(actual_empty_service_defaults=json.loads(empty.stdout),external_pc_publication=False,physical_gp_flags_fp_and_access_packets_unchanged=True))
            for option,env in(('--guards',{}),('--reference-guard',dict(HALO2_REFERENCE_OBJECT_PARENT_MODE='1'))):
                guarded=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,**env));assert guarded.returncode==0,(guarded.stdout+guarded.stderr)[-6000:]
                checks.append(dict(option=option,result=json.loads(guarded.stdout)))
            mutations=[]
            if args.semantic_mutants:
                from native_object_parent_mode_mutations import qualify
                pins['tools/native_object_parent_mode_mutations.py']=sha(ROOT/'tools/native_object_parent_mode_mutations.py')
                mutations=qualify(scratch,fixture,compiler,command)
            outcomes.append(dict(compiler=compiler,command=command,result=json.loads(result.stdout),checks=checks,compiled_mutations=mutations))
        report.update(passed=True,physical_paths=len(all_rows),source_sites=sorted(hex(pc)for pc in sites_all),originals=authority,compilers=outcomes)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
