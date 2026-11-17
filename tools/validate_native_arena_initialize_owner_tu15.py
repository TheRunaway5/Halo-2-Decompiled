#!/usr/bin/env python3
"""Full private arena parent through real existing page/file/worker owners."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from validate_native_arena_initialize_frame_tu15 import body,UNITS
SOURCES=('tests/native_tu15_arena_initialize_owner_tests.cpp','src/physics/native_tu15_arena_data.cpp','src/physics/physics_game_arena_native.cpp','src/physics/physics_physical_storage_host.cpp','src/physics/physics_physical_storage_runtime_generated.cpp','src/physics/physics_game_arena_runtime_generated.cpp','src/runtime/source_runtime_memory.cpp','src/runtime/source_runtime_host_storage.cpp','src/runtime/source_runtime_host_physical_memory.cpp','src/platform/native_synchronization.cpp','src/platform/native_threads.cpp','src/runtime/source_runtime_snapshot_bytes.cpp','src/assets/cache_file_snapshot.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);inputs=(*SOURCES,'tools/validate_native_arena_initialize_owner_tu15.py','tools/validate_native_arena_initialize_frame_tu15.py','include/halo2/native_tu15_arena_initialize_kernel.hpp','src/physics/runtime/native_tu15_arena_initialize_frame.hpp');pins={p:sha(ROOT/p)for p in inputs};headers={p.resolve():sha(p)for directory in('include','src')for p in(ROOT/directory).rglob('*')if p.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],scope='Private full native parent versus independently retained complete parent, real backed resources/child ABIs, unchanged original store stream. No production/default, Xbox kernel clobber or all-native dependency closure claim.')
    try:
        from native_arena_initialize_replacements import remove_arena_initialize
        pins['tools/native_arena_initialize_replacements.py']=sha(ROOT/'tools/native_arena_initialize_replacements.py')
        unit='src/physics/physics_game_arena_runtime_generated.cpp';source=remove_arena_initialize((ROOT/unit).read_text());parent=body(source)
        if hashlib.sha256(parent.encode()).hexdigest()!=UNITS[unit]:raise ValueError('Complete original arena parent pin differs')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        if not resolver:raise ValueError('Full arena resolver missing')
        (scratch/'arena_initialize_parent.inc').write_text(resolver[0]+'\n'+parent+'\n')
        fixture=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/original-v2/report.json';original=json.loads(fixture.read_text());stores=fixture.parent/original['store_file']
        if original['source_sha256']!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'or original['instruction_patches']or original['supplied_gameplay_results']:raise ValueError('Original arena fixture authority differs')
        if sha(stores)!='73ae9913a4e1be6066fff2fb3fad28cc156b2ca4f4e6eb0371a8c3cdb06a79ba':raise ValueError('Original ordered store stream differs')
        binary=scratch/'arena-owner';flags=['-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch)]
        if args.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        objects=[];commands=[]
        for index,name in enumerate(SOURCES):
            target=scratch/(str(index)+'.o');dep=target.with_suffix('.d');command=[args.compiler,*flags,'-c',str(ROOT/name),'-o',str(target),'-MD','-MF',str(dep)];commands.append(command)
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Real arena owner compile failed: '+done.stderr[-12000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path.is_relative_to(ROOT)and path.is_file():pins[str(path.relative_to(ROOT))]=headers.get(path,pins.get(str(path.relative_to(ROOT)),sha(path)))
            objects.append(target)
        link=[args.compiler,*flags,*map(str,objects),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(binary)];commands.append(link)
        done=subprocess.run(link,capture_output=True,text=True)
        if done.returncode:raise ValueError('Real arena owner link failed: '+done.stderr[-12000:])
        started=time.time()
        done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
        if done.returncode:raise ValueError('Real arena owner mismatch: '+(done.stdout+done.stderr)[-12000:])
        elapsed=time.time()-started;mutations=[]
        if args.semantic_mutants:
            relative='include/halo2/native_tu15_arena_initialize_kernel.hpp';kernel=(ROOT/relative).read_text()
            definitions=(('metadata-size','metadata_bytes=0x1288u;','metadata_bytes=0x1284u;'),('missing-last-clear','clear_words(view,backing_words);','clear_words(view,backing_words-1u);'),('missing-root-vtable','if(result.root)view.initialize_root(result.root);','if(!result.root)view.initialize_root(result.root);'),('wrong-accounting-size','view.account_allocation(Allocation::metadata,metadata_bytes);','view.account_allocation(Allocation::metadata,root_bytes);'))
            for name,old,new in definitions:
                if kernel.count(old)!=1:raise ValueError('Missing unique arena semantic mutation '+name)
                overlay=scratch/('mutant-'+name);header=overlay/relative;header.parent.mkdir(parents=True);header.write_text(kernel.replace(old,new,1));target=overlay/'test.o'
                command=[args.compiler,'-I'+str(overlay/'include'),*flags,'-c',str(ROOT/SOURCES[0]),'-o',str(target)]
                checked=subprocess.run(command,capture_output=True,text=True)
                if checked.returncode:raise ValueError('Arena semantic mutant did not compile '+name+': '+checked.stderr[-5000:])
                mutant=overlay/'test';command=[args.compiler,*flags,str(target),*map(str,objects[1:]),'-Wl,--gc-sections','-pthread','-lcrypto','-o',str(mutant)]
                checked=subprocess.run(command,capture_output=True,text=True)
                if checked.returncode:raise ValueError('Arena semantic mutant did not link '+name+': '+checked.stderr[-5000:])
                checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=120);diagnostic=(checked.stdout+checked.stderr)[-5000:]
                if checked.returncode==0 or'genuine-reference-complete stores=1046910'not in diagnostic or not any(message in diagnostic for message in('Whole parent actual store differs','Whole original parent store suffix not retired')):raise ValueError('Arena semantic mutant survived/unrecognized '+name+': '+diagnostic)
                mutations.append(dict(name=name,rejected=True,positive_reference_completed=True,fresh_owned_files=True,exit_code=checked.returncode,diagnostic=diagnostic))
        report.update(passed=True,compiler=args.compiler,commands=commands,result=json.loads(done.stdout),test_elapsed_seconds=elapsed,compiled_mutations=mutations,original_fixture_sha256=sha(fixture),original_store_stream_sha256=sha(stores),full_resolver_sha256=hashlib.sha256(resolver[0].encode()).hexdigest())
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={p:sha(ROOT/p)for p in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
