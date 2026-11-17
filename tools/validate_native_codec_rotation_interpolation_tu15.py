#!/usr/bin/env python3
"""Private complete normalized quaternion interpolation against unchanged TU1.5 i386."""
import argparse,copy,hashlib,json,os,random,shlex,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from native_packed_rotation_source_inputs import function,digest
import native_codec_rotation_interpolation_replacements as binding
import re
REGIONS=((0x510000,8192),(0x61000000,8192),(0x62000000,8192))
UNITS=('src/physics/physics_live_support_tu15_generated.cpp','src/rendering/models/source_model_render_tu15_generated.cpp')
CONTEXTS=(
('da886c672464b2f714114b44c34bbd69186680fe470bf2d0de846ee7d5527d37','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'),
('c05d27413620e331733629ab4f8872de3c6db16fe6f745b0e95e1686ae06df34','57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db','f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(smoke):
    rng=random.Random(0x296240);rows=[]
    from validate_native_packed_rotation_tu15 import corpus as packed
    profiles=packed(smoke)
    patterns=((0,0,0,0),(0x3f800000,0,0,0),(0x3f800000,0x40000000,0x40400000,0x40800000),(0x7fc12345,0xffc54321,0x80000000,1),(0x7f800000,0xff800000,0x7f7fffff,0x00800000))
    for case,profile in enumerate(profiles):
        row=copy.deepcopy(profile);row.update(name=f'rotation-interpolation-{case}',entry=0x296240,max_steps=100)
        banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
        sp=(0x61000030,0x510f70,row['gp'][4])[case%3];row['gp'][4]=sp;factory=0x61000000
        stride,index=((0x100,0),(0x10,2),(0x80000010,2),(0xfffffff0,0xffffffff))[case%4]
        base=(factory+stride*index+32)&0xffffffff;previous=(0,1,0x10000000,2)[case%4];following=(1,2,0x10000001,0)[case%4]
        output=(0x61000500,0x510f40,0x61000020,0x620001e0)[case%4]
        def put(address,value):
            for region,bank in banks.items():
                if region<=address<=region+len(bank)-4:bank[address-region:address-region+4]=(value&0xffffffff).to_bytes(4,'little');return
            raise ValueError('Rotation interpolation fixture escaped declared banks')
        put(0x510f60,factory);put(factory+20,stride);put(0x510f94,index);put(0x510fa0,output)
        put(0x510f4c,(0,0x3f800000,0x3f000000,0x7fc67890,0xbf800000,0x7f800000)[case%6]);put(0x510f44,previous);put(0x510f48,following)
        for ordinal,item in enumerate((previous,following)):
            address=(base+(item<<4))&0xffffffff
            values=patterns[(case+ordinal)%len(patterns)]if case<40 else tuple(rng.getrandbits(32)for _ in range(4))
            for lane,value in enumerate(values):put(address+4*lane,value)
        # The shared unchanged-i386 entry harness initializes its two declared
        # formal words even for this childless leaf. Mirror those exact entry
        # bytes when moving SP into a source/global alias bank.
        put(sp,0x63000000);put(sp+4,0);put(sp+8,0)
        for bank in row['banks']:bank['before']=banks[bank['address']].hex()
        rows.append(row)
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--smoke',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--mmx',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT);args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=UNITS+('include/halo2/native_tu15_codec_rotation_interpolation_kernel.hpp','src/physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp','tests/native_tu15_codec_rotation_interpolation_tests.cpp','include/halo2/native_tu15_packed_rotation_kernel.hpp','tools/native_packed_rotation_source_inputs.py','tools/validate_native_packed_rotation_tu15.py','tools/validate_native_codec_rotation_interpolation_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/native_mopp_ray_dense_original.py','tools/native_sphere_frustum_mmx_original.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/oracle_native_campaign_screen_lifecycle_tu15.py')
    pins={name:sha(ROOT/name)for name in files};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    pins['tools/native_codec_rotation_interpolation_replacements.py']=sha(ROOT/'tools/native_codec_rotation_interpolation_replacements.py')
    report=dict(passed=False,private_unbound=not args.require_binding,require_binding=args.require_binding,retained_vector_half_atomicity_qualified=False,opaque_input_requested=args.mmx,opaque_mmx_unchanged=True,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        assert sha(args.xbe)==NEW_PIN;image=Xbe(args.xbe)
        owners=[]
        for name,expected in zip(UNITS,CONTEXTS,strict=True):
            source=(ROOT/name).read_text();_,_,body=binding.authenticate(source)
            if args.require_binding and binding.installed_entries(source)!=['296240']:raise ValueError('Installed interpolation source missing')
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)[0]
            if tuple(map(digest,(body,resolver,dispatcher,state)))!=expected:raise ValueError('Complete rotation interpolation owner context drift')
            if 'case 0x296240u:return 0x296240u;'not in resolver:raise ValueError('Rotation interpolation actual image changed')
            for helper,pin in (('source_estimate','bb50fdcd1d22250d871032ff227a462ef42c30981de20b9f23137bb1c60efd89'),('native_estimate_vector','a5e2ad3c7a078be7decea72984e8efc82b03f966b46aea97f1cc89e4c48d6aa4')):
                if digest(function(source,helper))!=pin:raise ValueError('Complete rotation interpolation estimate helper drift')
            owners.append(dict(source=name,body_sha256=expected[0],resolver_sha256=expected[1],dispatcher_sha256=expected[2],state_sha256=expected[3]))
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original=common.metadata;common.ENTRIES=(0x296240,)
        def metadata(source):
            result=original(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];io=[site['instruction']for site in sites if site['operands']]+[0x2962c6];rows=corpus(args.smoke)
        if len(sites)!=40:raise ValueError('Complete rotation interpolation instruction set drift')
        normal_count=len(rows)
        if args.mmx:
            rng=random.Random(0x296240a11)
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal_count]);row.update(id=len(rows),name=f'rotation-interpolation-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
        opaque_count=len(rows)-normal_count
        proofs=[]
        def measure(values,label):
            directory=scratch/label;directory.mkdir();opaque='active_mmx'in values[0]
            runner=physical_original_mmx if opaque else physical_original_dense
            result,proof=runner(image,values,directory,entries=(0x296240,),regions=REGIONS+(((0x64000000,4096),)if opaque else()),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True)
            proofs.append(proof);return result['cases']
        actual=measure(rows[:normal_count],'normal')
        if opaque_count:actual+=measure(rows[normal_count:],'opaque')
        alignment=[]
        for load in(False,True):
            for offset in range(1,16):
                row=copy.deepcopy(rows[offset%normal_count]);row.update(id=len(rows)+len(alignment),name=f'rotation-interpolation-align-{load}-{offset}',phase='alignment_fault',alignment_fault=True,alignment_entry=0x296299 if load else 0x2962cf)
                banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
                def put(address,value):
                    for region,bank in banks.items():
                        if region<=address<=region+len(bank)-4:bank[address-region:address-region+4]=(value&0xffffffff).to_bytes(4,'little');return
                    raise ValueError('Rotation interpolation alignment input escaped backing')
                if load:
                    factory=0x61000000+offset;put(0x510f60,factory);put(factory+20,0);put(0x510f94,0);put(0x510f44,0);row['alignment_address']=factory+32
                else:put(0x510fa0,0x61000500+offset);row['alignment_address']=0x61000500+offset
                for bank in row['banks']:bank['before']=banks[bank['address']].hex()
                alignment.append(row)
        rows+=alignment;actual+=measure(alignment,'alignment')
        if args.prefixes:
            prefixes=[]
            indices=list(range(min(8,normal_count)))+([normal_count+n for n in(0,1,7,15)if n<opaque_count]if args.mmx else[])
            for index in indices:
                observed=actual[index]
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(observed[key]):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}{ordinal}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        if phase=='after_write'and event['instruction']==0x2962cf:row['native_observer_ordinal']=ordinal+1
                        prefixes.append(row)
            for opaque in(False,True):
                selected=[row for row in prefixes if('active_mmx'in row)==opaque]
                if selected:rows+=selected;actual+=measure(selected,'prefix-'+str(opaque))
        coverage=set()
        for row,observed in zip(rows,actual,strict=True):
            assert observed['completed']==('phase'not in row);coverage.update(observed['executed_sites'])
            pc=observed['boundary']['instruction']if'phase'in row else 0x2962d5
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            if'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            if row.get('alignment_fault'):row['execution_flags_before_fault']=observed['calls'][-1]['eflags']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for packet in row['calls']:assert packet.pop('arguments')==[]
            for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
        report.update(physical_paths=len(rows),original_normal_paths=normal_count,original_opaque_paths=opaque_count,actual_alignment_faults=len(alignment),source_sites=sorted(hex(pc)for pc in coverage),source_owners=owners,original=proofs)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_codec_rotation_interpolation_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-7000:]
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert done.returncode==0,dict(exit_code=done.returncode,diagnostic=(done.stdout+done.stderr)[-10000:])
            mutations=[]
            if args.semantic_mutants:
                kernel='include/halo2/native_tu15_codec_rotation_interpolation_kernel.hpp';frame='src/physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp'
                definitions=(('wrong-interpolation-subtract',frame,'subtract(input.second,input.first)','subtract(input.first,input.second)'),('wrong-norm-pair','include/halo2/native_tu15_packed_rotation_kernel.hpp','const auto opposite=view.opposite(squared);','const auto opposite=view.alternate(squared);'),('early-output-pointer',frame,'returned_boundary(0x2962c9u,true);s_.gp[2]=word(s_.gp[4]);','returned_boundary(0x2962c9u,true);s_.gp[0]=word(s_.gp[4]);'),('wrong-previous-row',frame,'g[2]=s_.flags.plus(g[2],g[0],4);','g[2]=s_.flags.plus(g[2],g[0]+16u,4);'))
                for name,relative,old,new in definitions:
                    source=(ROOT/relative).read_text()
                    if source.count(old)!=1:raise ValueError('Missing rotation interpolation defect witness '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));mutant=overlay/'test'
                    mutated=list(command);mutated[1:1]=['-I'+str(overlay/'include'),'-I'+str(overlay/'src')];mutated[mutated.index('-o')+1]=str(mutant)
                    checked=subprocess.run(mutated,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-5000:]
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                    if checked.returncode==0 or not any(message in diagnostic for message in('rotation interpolation full returned/stopped ABI differs','rotation interpolation completion differs','rotation interpolation original pre-I/O packets differ','Retained vector halves changed CPU ABI')):raise ValueError('Rotation interpolation defect survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,physical_paths=len(rows),original_normal_paths=normal_count,original_opaque_paths=opaque_count,actual_alignment_faults=len(alignment),source_sites=sorted(hex(pc)for pc in coverage),source_owners=owners,original=proofs,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
