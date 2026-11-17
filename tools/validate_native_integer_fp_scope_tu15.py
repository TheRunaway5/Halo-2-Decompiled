#!/usr/bin/env python3
"""Private FP-opaque integer scope with genuine FC510 child arithmetic."""
import argparse,copy,hashlib,json,os,random,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from validate_native_object_parent_mode_tu15 import corpus,REGIONS
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from validate_native_tu15_prt_helpers import physical_original
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true');parser.add_argument('--hostile-required',action='store_true');parser.add_argument('--fenced-required',action='store_true');parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=('src/physics/runtime/native_tu15_integer_fp_scope.hpp','src/physics/runtime/native_tu15_required_abi_fp.hpp','tests/native_tu15_integer_fp_scope_tests.cpp','tests/native_tu15_object_parent_mode_tests.cpp','tools/validate_native_integer_fp_scope_tu15.py','tools/validate_native_object_parent_mode_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/native_sphere_frustum_mmx_original.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
    pins={name:sha(ROOT/name)for name in files};headers={p.resolve():sha(p)for directory in('include','src')for p in(ROOT/directory).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_unbound=True,production_service_transport_qualified=False,hostile_required_provider=args.hostile_required,fenced_required_prototype=args.fenced_required,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        assert sha(args.xbe)==NEW_PIN;image=Xbe(args.xbe)
        import native_marker_owner_replacements as binding
        effect='src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp';helper='src/gameplay/weapons/native_weapon_marker_matrix_tu15_internal.hpp'
        cpp=(ROOT/effect).read_text();matrix=(ROOT/helper).read_text()
        if binding.replace_effect(matrix,cpp)!=matrix:raise ValueError('Genuine Effect marker/FC child defaults or transport drift')
        pins.update({name:sha(ROOT/name)for name in(effect,helper,'tools/native_marker_owner_replacements.py','tools/native_object_parent_mode_replacements.py','tools/native_model_marker_lookup_replacements.py')})
        effect_source=scratch/'integer_scope_effect.cpp';effect_source.write_text(cpp)
        report['actual_effect_child_api']=dict(source=effect,helper=helper,complete_required_call_helper=binding.CALL_PIN,fc510_body=binding.parent_mode.EFFECT_PIN,declared_fixture_provider=True,whole_parent_or_engine_provider_qualified=False)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original=common.metadata;common.ENTRIES=(0xfc510,)
        def metadata(source):
            result=original(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];io=[dict(entry=site['instruction'],gp=True,xmm=True)for site in sites if site['operands']]
        rows=corpus('TitleUpdate15',args.smoke);folder=scratch/'dense';folder.mkdir()
        actual,proof=physical_original(image,rows,folder,entries=(0xfc510,),regions=REGIONS,trace_calls=io,trace_extended=True,preserve_x87_prefix=True)
        stops=[]
        for index in range(min(16,len(rows))):
            for ordinal in range(len(actual['cases'][index]['reads'])):
                row=copy.deepcopy(rows[index]);row.update(name=row['name']+f'-read{ordinal}',fault=dict(phase='before_read',ordinal=ordinal));stops.append(row)
        folder=scratch/'prefixes';folder.mkdir()
        observed,prefix_proof=physical_original(image,stops,folder,entries=(0xfc510,),regions=REGIONS,trace_calls=io,trace_extended=True,preserve_x87_prefix=True)
        normal_count=len(rows);rows+=stops;actual['cases']+=observed['cases']
        from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
        opaque=[];rng=random.Random(0xbacd0fc510)
        for mask in range(16 if args.smoke else 256):
            row=copy.deepcopy(rows[mask%normal_count]);row.update(name=row['name']+f'-opaque{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800
            row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);opaque.append(row)
        folder=scratch/'opaque';folder.mkdir()
        observed,opaque_proof=physical_original_mmx(image,opaque,folder,entries=(0xfc510,),regions=REGIONS+((PROBE,4096),),trace_calls=io,trace_extended=True,preserve_x87_prefix=True)
        rows+=opaque;actual['cases']+=observed['cases'];coverage=set()
        for row,observed in zip(rows,actual['cases'],strict=True):
            assert observed['completed']==('fault'not in row);coverage.update(observed['executed_sites'])
            pc=observed['boundary']['instruction']if row.get('fault')else observed['executed_sites'][-1]
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',row['live_x87']))
            if 'active_mmx'in row:row['expected']['active_mmx']=observed['active_mmx']
            row.update(reads=observed['reads'],calls=observed['calls']);assert not observed['writes']
            for packet in row['calls']:assert packet.pop('arguments')==[]
            for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));compilers=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_integer_fp_scope_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            command+=['-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'src/gameplay/weapons'),str(effect_source)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-8000:]
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            # With two source files the compiler's final -MF names the last
            # dependency output; also capture the test's complete own closure.
            dependency=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(ROOT/'tests/native_tu15_integer_fp_scope_tests.cpp')]
            checked=subprocess.run(dependency,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-5000:]
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            run=[str(binary),str(fixture)]+(['--hostile-required']if args.hostile_required else[])+(['--fenced-required']if args.fenced_required else[])
            done=subprocess.run(run,capture_output=True,text=True);assert done.returncode==0,dict(exit_code=done.returncode,diagnostic=(done.stdout+done.stderr)[-9000:])
            backend=scratch/(Path(compiler).name+'-unsupported.cpp');backend.write_text('#define _MSC_VER 1930\n#include "physics/runtime/native_tu15_integer_fp_scope.hpp"\n#include "physics/runtime/native_tu15_required_abi_fp.hpp"\nint main(){return halo2::native_integer_fp_detail::supported(0)||halo2::native_required_abi_fp_detail::available()?1:0;}\n')
            backend_binary=backend.with_suffix('');backend_command=[compiler,'-std=c++20','-I'+str(ROOT/'src'),str(backend),'-o',str(backend_binary)]
            checked=subprocess.run(backend_command,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-5000:]
            assert subprocess.run([str(backend_binary)]).returncode==0
            compilers.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),unsupported_backend_parse_and_refusal=True))
        report.update(passed=True,physical_paths=len(rows),source_sites=sorted(hex(pc)for pc in coverage),original_dense=proof,original_read_prefixes=prefix_proof,original_opaque=opaque_proof,compilers=compilers)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
