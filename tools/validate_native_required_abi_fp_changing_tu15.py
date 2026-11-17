#!/usr/bin/env python3
"""Genuine x87/MXCSR-changing children through the private ABI FP fence."""
import argparse,hashlib,json,os,platform,random,shlex,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from validate_native_tu15_prt_helpers import physical_original
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def ticks(smoke):
    from validate_native_tick_conversion_tu15 import LIVE
    rng=random.Random(0x22a5c0f0);rows=[]
    controls=[(pc,rc)for pc in(0x7f,0x27f,0x37f)for rc in range(4)]if not smoke else[(0x37f,0)]
    numbers=(0,1,-1,733333334,2**53+1,2**63-1,-2**63,733333333)if not smoke else(1,2**53+1)
    for pc,rc in controls:
        for depth in((0,1,4,6)if not smoke else(0,4)):
            for number in numbers:
                index=len(rows);bank=bytearray(rng.randbytes(8192));sp=0x62001000+index%16;value=number&0xffffffffffffffff
                for n,word in enumerate((0x63000000,value&0xffffffff,value>>32)):struct.pack_into('<I',bank,sp-0x62000000+n*4,word)
                gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
                rows.append(dict(id=index,name=f'tick-{pc:x}-{rc}-{depth}-{number}',entry=0x22a5c0,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),matrix=value&0xffffffff,source=value>>32,before_fp=[pc|(rc<<10),((index%8)<<11)|((index%4)<<8),0x1f80|(rc<<13)],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=list(LIVE[:depth]),max_steps=1000,banks=[dict(address=0x62000000,before=bank.hex())]))
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT);args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if sys.platform!='linux' or platform.machine().lower()not in('x86_64','amd64'):print('SKIP required ABI FP proof needs Linux x86-64');return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=('cmake/NativeTu15RequiredAbiFp.cmake','tools/validate_native_required_abi_fp_changing_tu15.py','tests/native_tu15_required_abi_fp_changing_tests.cpp','src/physics/runtime/native_tu15_required_abi_fp.hpp','src/physics/runtime/native_tu15_integer_fp_scope.hpp','tests/native_tu15_marker_owner_test_support.hpp','tests/native_tu15_object_parent_mode_tests.cpp','tools/validate_native_tick_conversion_tu15.py','tools/validate_native_sphere_frustum_tu15.py','tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
    pins={name:sha(ROOT/name)for name in files};headers={p.resolve():sha(p)for directory in('include','src')for p in(ROOT/directory).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_unbound=True,declared_entry_only=True,physical_fault_or_parent_default_qualified=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        assert sha(args.xbe)==NEW_PIN;image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from validate_native_sphere_frustum_tu15 import corpus,REGIONS
        sphere=corpus()
        if args.smoke:sphere=[row for row in sphere if row['name'].startswith(tuple('family-'+str(n)+'-'for n in range(18,25)))and row['name'].endswith(tuple('variant-'+str(n)for n in range(4)))]
        all_rows=[];authority=[]
        for name,rows,entries,regions,returned in(('tick',ticks(args.smoke),(0x22a5c0,0x32ae70,0x32acd0),((0x62000000,8192),),True),('sphere',sphere,(0x184690,),REGIONS,False)):
            common.ENTRIES=entries;sites=common.metadata(image)['sites'];code=common._Code(image,'.text')
            writes=[site['instruction']for site in sites if any(op['write']for op in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
            folder=scratch/name;folder.mkdir();actual,proof=physical_original(image,rows,folder,entries=entries,regions=regions,return_x87=returned,trace_extended=not returned,preserve_x87_prefix=not returned)
            for row,observed in zip(rows,actual['cases'],strict=True):
                assert observed['completed']
                live=[observed['returned_extended'],*row['live_x87']]if returned else observed.get('boundary_live_x87',row['live_x87'])
                final_pc=0x22a621 if returned else observed['executed_sites'][-1]
                if final_pc not in observed['executed_sites']or code.at(final_pc).mnemonic!='ret':raise ValueError('Missing genuine child return site')
                row['expected']=dict(entry=final_pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=live)
                row.update(writes=observed['writes'],write_sites=writes)
                for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
            all_rows+=rows;authority.append(dict(kind=name,paths=len(rows),proof=proof))
        if not any((row['expected']['fp'][1]&~row['before_fp'][1])&0x3f for row in all_rows):raise ValueError('Original corpus produced no new x87 exceptions')
        if not any((row['expected']['fp'][2]&~row['before_fp'][2])&0x3f for row in all_rows):raise ValueError('Original corpus produced no new MXCSR exceptions')
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=all_rows),separators=(',',':')));results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_required_abi_fp_changing_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-8000:]
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert done.returncode==0,dict(exit_code=done.returncode,diagnostic=(done.stdout+done.stderr)[-10000:])
            mutations=[]
            if args.semantic_mutants:
                relative='src/physics/runtime/native_tu15_required_abi_fp.hpp';source=(ROOT/relative).read_text()
                definitions=(('missing-fence','native_host_fp_io_detail::preserve(std::forward<Provider>(provider));','std::forward<Provider>(provider)();'),('restored-after-load','\n    state.load(abi);\n}','\n    const auto old=native_raw_x87_detail::capture();state.load(abi);native_raw_x87_detail::restore(old);\n}'),('missing-returned-abi-load','\n    state.load(abi);\n}','\n    (void)abi;\n}'))
                for name,old,new in definitions:
                    if source.count(old)!=1:raise ValueError('Missing FP fence mutation witness '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));mutant=overlay/'test'
                    mutated=list(command);mutated[1:1]=['-I'+str(overlay/'src')];mutated[mutated.index('-o')+1]=str(mutant)
                    checked=subprocess.run(mutated,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-5000:]
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=(checked.stdout+checked.stderr)[-6000:]
                    known='FP-changing full returned ABI differs'in diagnostic or(name=='missing-fence'and('FPE'in diagnostic or checked.returncode==-8))
                    if checked.returncode==0 or not known:raise ValueError('FP fence defect survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,physical_paths=len(all_rows),authority=authority,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
