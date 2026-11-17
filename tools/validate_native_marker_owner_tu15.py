#!/usr/bin/env python3
"""Actual BACD0 semantic core and genuine native FC510 composition."""
import argparse,copy,hashlib,itertools,json,os,random,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from validate_native_tu15_prt_helpers import physical_original
REGIONS=((0x4ec000,4096),(0x4f0000,4096),(0x61000000,0x6000),(0x62000000,4096))
INPUTS=('include/halo2/native_tu15_marker_owner_kernel.hpp','include/halo2/native_tu15_marker_owner_value.hpp','src/gameplay/weapons/native_tu15_marker_owner.hpp','tests/native_tu15_marker_owner_tests.cpp','tests/native_tu15_object_parent_mode_tests.cpp','tools/validate_native_marker_owner_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
INPUTS+=('tests/native_tu15_marker_owner_test_support.hpp',)
INPUTS+=('tools/native_model_marker_lookup_replacements.py','tools/native_object_parent_mode_replacements.py','tools/native_halfword_bit_query_replacements.py','tools/native_selected_operand_binding.py')
INPUTS+=('tools/native_marker_owner_mutations.py',)
INPUTS+=('tests/native_tu15_marker_owner_value_tests.cpp',)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(smoke):
    rng=random.Random(0xbacd0);cases=list(itertools.product(range(3),range(2),range(8),range(4)));rng.shuffle(cases)
    if smoke:cases=cases[:32]
    rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
    expanded=[(*case,None)for case in cases]
    if not smoke:expanded +=[(depth,1,2,1,offset)for depth in range(3)for offset in(-4,-8,-12,-16)]
    for index,(depth,candidate,kind,pickup,alias)in enumerate(expanded):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(address,value,width=4):
            for(base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Marker-owner input escaped bank')
        objects=0x61000020;slots=0x61000100;tags=0x61000200;sp=0x62000200+(index%16)
        put(0x4ecbd0,objects);put(objects+0x44,slots);put(0x4f0414,tags)
        for n in range(4):
            record=sp+alias-0x154 if alias is not None and n==depth else 0x61001000+n*0x200
            definition=0x61000400+n*0x40
            put(slots+n*12+8,record);put(record,n);put(tags+n*16+8,definition)
            put(record+0x14,(0xa6000000|n+1)if n<depth else(0xffffffff if index%2 else 0xa6000003))
            put(definition+2,0x10 if n<depth else 0,1)
            put(record+4,candidate if n==depth else 0,1);put(record+0xaa,kind,1);put(record+0x12c,pickup,1)
            put(record+0x154,0x53000000|(n<<16)|index)
        datum=0xa6000000;put(sp,0x63000000);put(sp+4,datum);put(sp+8,0x65000000|index)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
        count=(0,1,4,8)[index%4];cw=(0x7f,0x27f,0x37f)[index%3]|((index%4)<<10)
        rows.append(dict(id=index,name=f'inherit{depth}-unit{candidate}-kind{kind}-pickup{pickup}'+(f'-output-alias{alias}'if alias is not None else''),entry=0xbacd0,gp=gp,eflags=0x202|rng.choice((0,1,0x10,0x40,0x80,0x800,0x8d5))|(0x400 if index%2 else 0),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,((index%8)<<11)|0x4500|(index*7&63),0x1f80|((index%4)<<13)],live_x87=list(live[:count]),matrix=datum,source=0x65000000|index,max_steps=1000,value_domain=alias is None,banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)]))
    if not smoke:
        seed=next(row for row in rows if row['name']=='inherit0-unit1-kind2-pickup1')
        variants=[('kind-byte'+str(kind),('kind',kind))for kind in range(8,256)]
        variants +=[('tag-index'+hex(low),('tag',low))for low in(0x7fff,0x8000,0xffff)]
        variants +=[('object-index'+hex(low),('object',low))for low in(0x7fff,0x8000,0xffff)]
        for name,(which,value)in variants:
            row=copy.deepcopy(seed);row.update(id=len(rows),name=name)
            banks=[bytearray.fromhex(bank['before'])for bank in row['banks']]
            def put(address,value,width=4):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Marker-owner variant escaped bank')
            if which=='kind':put(0x610010aa,value,1)
            elif which=='tag':
                put(0x61001000,0xa5a50000|value);put(0x61001014,0xa6000003)
                put(0x4f0414,(0x61000208-8-value*16)&0xffffffff)
            else:
                row['matrix']=0xa6000000|value;put(row['gp'][4]+4,row['matrix'])
                put(0x61000064,(0x61000108-8-value*12)&0xffffffff)
            for bank,data in zip(row['banks'],banks):bank['before']=data.hex()
            rows.append(row)
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--smoke',action='store_true');parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--source-copies',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--public-value',action='store_true')
    parser.add_argument('--require-binding',action='store_true');parser.add_argument('--xbe',type=Path)
    args=parser.parse_args()
    if args.semantic_mutants and not args.source_copies:parser.error('--semantic-mutants requires --source-copies')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    pins={name:sha(ROOT/name)for name in INPUTS};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_unbound=not args.require_binding,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        if args.source_copies:
            from native_marker_owner_inputs import prepare
            report['actual_source_ownership']=prepare(ROOT,scratch,args.require_binding)
            pins.update({name:sha(ROOT/name)for name in(*report['actual_source_ownership']['source_files'],'tools/native_marker_owner_inputs.py','tools/native_marker_owner_replacements.py','src/gameplay/weapons/native_tu15_marker_owner_admission.hpp','tests/native_tu15_marker_owner_source_tests.cpp','tools/specialize_native_operands.py')})
        if args.public_value:
            from native_marker_owner_inputs import prepare_value,PUBLIC
            report['public_value_ownership']=prepare_value(ROOT,scratch,args.require_binding)
            pins.update({name:sha(ROOT/name)for name in(PUBLIC,'tools/native_marker_owner_inputs.py','tools/native_marker_owner_replacements.py')})
        original_xbe=args.xbe or NEW_DEFAULT
        assert sha(original_xbe)==NEW_PIN;image=Xbe(original_xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original=common.metadata;common.ENTRIES=(0xbacd0,0xfc510)
        def metadata(source):
            result=original(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in('ret','pop'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];code=common._Code(image,'.text')
        io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')]
        rows=corpus(args.smoke);folder=scratch/'physical';folder.mkdir()
        actual,proof=physical_original(image,rows,folder,entries=(0xbacd0,0xfc510),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
        kinds=set()
        for row,observed in zip(rows,actual['cases'],strict=True):
            for read in observed['reads']:
                if read['instruction']!=0xbad26:continue
                address=read['address']
                if any(write['address']<=address<write['address']+len(bytes.fromhex(write['written']))for write in observed['writes']):
                    raise ValueError('Kind coverage witness overlaps a source write')
                bank=next(bank for bank in row['banks']if bank['address']<=address<bank['address']+len(bytes.fromhex(bank['before'])))
                kinds.add(bytes.fromhex(bank['before'])[address-bank['address']])
        if not args.smoke and kinds!=set(range(256)):raise ValueError('Original did not load every kind byte')
        proof['loaded_kind_bytes']=sorted(kinds)
        if args.prefixes:
            aliases=[i for i,row in enumerate(rows)if 'output-alias'in row['name']]
            selected=list(dict.fromkeys([0,1,*[next(i for i,row in enumerate(rows)if row['name'].startswith('inherit'+str(depth)))for depth in(1,2)],*[i for i,observed in enumerate(actual['cases'])if 0xfc510 in observed['executed_sites']][:2],*aliases[-2:],*range(max(0,len(rows)-4),len(rows))]))
            prefixes=[]
            for index in selected:
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal in range(len(actual['cases'][index][key])):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefixes.append(row)
            folder=scratch/'prefixes';folder.mkdir()
            stopped,stopped_proof=physical_original(image,prefixes,folder,entries=(0xbacd0,0xfc510),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
            rows+=prefixes;actual['cases']+=stopped['cases'];proof['prefixes']=stopped_proof
        write_sites=[site['instruction']for site in sites if any(op['write']for op in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
        covered=set()
        for row,observed in zip(rows,actual['cases'],strict=True):
            assert observed['completed']==('fault'not in row);covered.update(observed['executed_sites'])
            # The child's addresses are higher than its parent's epilogues.
            # Derive final publication from the actual original RET read,
            # never from the sorted union of executed parent/child sites.
            row['expected']=dict(entry=observed['boundary']['instruction']if row.get('fault')else observed['reads'][-1]['instruction'],gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',row['live_x87']))
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'],write_sites=write_sites)
            for packet in row['calls']:assert packet.pop('arguments')==[]
            for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=scratch/(Path(compiler).name+'.d')
            test='native_tu15_marker_owner_source_tests.cpp'if args.source_copies else'native_tu15_marker_owner_tests.cpp'
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests'/test),'-o',str(binary)]
            if args.source_copies:command+=['-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(scratch),'-I'+str(ROOT/'src/gameplay/weapons'),str(scratch/'marker_owner_effect.cpp')]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,result.stderr[-7000:]
            for source in(ROOT/'tests'/test, *((scratch/'marker_owner_effect.cpp',)if args.source_copies else())):
                syntax=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),'-I'+str(ROOT/'src/gameplay/weapons'),'-MD','-MF',str(dep),str(source)]
                checked=subprocess.run(syntax,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-7000:]
                for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(value).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-12000:]
            hostile_required=None
            if args.source_copies:
                checked=subprocess.run([str(binary),'--hostile-required',str(fixture)],capture_output=True,text=True)
                assert checked.returncode==0,(checked.stdout+checked.stderr)[-12000:]
                hostile_required=json.loads(checked.stdout)
            mapped=None
            if not args.source_copies:
                checked=subprocess.run([str(binary),'--canonical',str(fixture)],capture_output=True,text=True)
                assert checked.returncode==0,(checked.stdout+checked.stderr)[-12000:]
                mapped=dict(result=json.loads(checked.stdout),parent_pc_delta=-0x1140,actual_fc510_child_and_return_word_preserved=True,actual_motion_source_routing_qualified=False)
            reference=None
            checks=[]
            if args.source_copies:
                retained=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,HALO2_REFERENCE_MARKER_OWNER='1',HALO2_MARKER_OWNER_SOURCE_REFERENCE_TEST='1'))
                assert retained.returncode==0,(retained.stdout+retained.stderr)[-12000:]
                reference=json.loads(retained.stdout)
                for options,extra in(
                    (['--guards'],{}),
                    (['--source-guards'],{}),
                    (['--reference-guards'],{'HALO2_REFERENCE_MARKER_OWNER':'1'}),
                    (['--reference-guards'],{'HALO2_REFERENCE_OBJECT_PARENT_MODE':'1'}),
                    (['--required-entry-stop',str(fixture)],{}),
                    (['--empty-services',str(fixture)],{})):
                    checked=subprocess.run([str(binary),*options],capture_output=True,text=True,env=dict(os.environ,**extra))
                    assert checked.returncode==0,(checked.stdout+checked.stderr)[-12000:]
                    checks.append(dict(arguments=options,environment=extra,result=json.loads(checked.stdout)))
                changed=[]
                for extra in({},dict(HALO2_REFERENCE_MARKER_OWNER='1',HALO2_MARKER_OWNER_SOURCE_REFERENCE_TEST='1')):
                    checked=subprocess.run([str(binary),'--provider-policy-mutations',str(fixture)],capture_output=True,text=True,env=dict(os.environ,**extra))
                    assert checked.returncode==0,(checked.stdout+checked.stderr)[-12000:]
                    changed.append(json.loads(checked.stdout))
                if changed[0]!=changed[1]:raise ValueError('Uncertified returned-provider policy fallback differs from exact source reference')
                policy=changed[0]
                checks.append(dict(arguments=['--provider-policy-mutations'],actual_reference_equal=True,provider_policy_rows=policy['provider_policy_rows'],complete_results_sha256=hashlib.sha256(json.dumps(policy,sort_keys=True,separators=(',',':')).encode()).hexdigest(),retained_callbacks_and_packets_compared=True))
            mutations=[]
            if args.semantic_mutants:
                from native_marker_owner_mutations import qualify
                mutations=qualify(scratch,fixture,compiler,command)
            public=None
            if args.public_value:
                value_binary=scratch/(Path(compiler).name+'-value')
                value_command=[compiler,'-std=c++20','-O2','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_marker_owner_value_tests.cpp'),str(scratch/'marker_owner_public.cpp'),str(scratch/'marker_owner_public_reference.cpp'),'-o',str(value_binary)]
                if args.sanitizers:value_command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
                checked=subprocess.run(value_command,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-7000:]
                checked=subprocess.run([str(value_binary),str(fixture)],capture_output=True,text=True);assert checked.returncode==0,(checked.stdout+checked.stderr)[-12000:]
                public=dict(command=value_command,result=json.loads(checked.stdout))
            results.append(dict(compiler=compiler,command=command,result=json.loads(result.stdout),hostile_required_provider=hostile_required,canonical_parent_projection=mapped,positive_source_reference=reference,source_controls=checks,compiled_mutations=mutations,public_value=public))
        report.update(passed=True,physical_paths=len(rows),source_sites=sorted(hex(pc)for pc in covered),original=proof,compilers=results,observer_preimage_read_failures_qualified=False,atomic_provider_and_completed_write_observer_domain=True)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
