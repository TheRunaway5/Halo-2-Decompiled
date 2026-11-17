#!/usr/bin/env python3
"""Compare actual compiled-source provider transport, not CPU fault envelopes."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from validate_native_arena_initialize_frame_tu15 import corpus,rep_corpus,body,UNITS
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);files=('tools/validate_native_arena_initialize_source_transport_tu15.py','tools/validate_native_arena_initialize_frame_tu15.py','tests/native_tu15_arena_initialize_source_transport_tests.cpp','tests/native_tu15_arena_initialize_frame_tests.cpp','include/halo2/native_tu15_arena_initialize_kernel.hpp','src/physics/runtime/native_tu15_arena_initialize_frame.hpp','src/physics/physics_game_arena_runtime_generated.cpp');pins={p:sha(ROOT/p)for p in files};headers={p.resolve():sha(p)for directory in('include','src')for p in(ROOT/directory).rglob('*')if p.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],scope='Authentic compiled-source preimage/read/write/observer exception transport and real child-entry stops; no hardware exception-envelope, production/default or full cold-return claim.')
    try:
        from native_arena_initialize_replacements import remove_arena_initialize
        pins['tools/native_arena_initialize_replacements.py']=sha(ROOT/'tools/native_arena_initialize_replacements.py')
        source=remove_arena_initialize((ROOT/files[-1]).read_text());parent=body(source)
        if hashlib.sha256(parent.encode()).hexdigest()!=UNITS[files[-1]]:raise ValueError('Original complete parent pin differs')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        if not resolver:raise ValueError('Original complete resolver missing')
        rep=next(line for line in parent.splitlines()if'instruction(0x1252b8u)'in line)
        (scratch/'arena_source_transport.inc').write_text(resolver[0]+'\n'+parent+'\nvoid rep_statement(WorldState&s){\n'+rep+'\n}\n')
        normal=corpus();repeat=rep_corpus(normal);rows=[normal[n]for n in(0,1,15,16,31,80,81,96,97,112,113,127,128,129,143,144,159)]+[repeat[n]for n in(0,1,15,16,31,48,49,63,64,65,79)]
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=[0x125280,0x12528e,0x12528f,0x12529a,0x1252a4,0x1252b8]),separators=(',',':')))
        compilers=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_arena_initialize_source_transport_tests.cpp'),'-Wl,--gc-sections','-lcrypto','-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Source transport compile failed: '+done.stderr[-10000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path.is_relative_to(ROOT)and path.is_file():pins[str(path.relative_to(ROOT))]=headers.get(path,pins.get(str(path.relative_to(ROOT)),sha(path)))
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
            if done.returncode:raise ValueError('Source transport mismatch: '+(done.stdout+done.stderr)[-14000:])
            mutations=[]
            if args.semantic_mutants:
                relative='src/physics/runtime/native_tu15_arena_initialize_frame.hpp';frame=(ROOT/relative).read_text()
                for name,old,new in(('cpu-push-publication','if(!operands_.source_publication){Base::push','if(true){Base::push'),('cpu-rep-publication','if(operands_.source_publication){store(0x1252b8u','if(false){store(0x1252b8u')):
                    if frame.count(old)!=1:raise ValueError('Missing source-publication mutation '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(frame.replace(old,new,1));mutant=overlay/'test'
                    changed=list(command);changed[1:1]=['-I'+str(overlay/'src')];changed[changed.index('-o')+1]=str(mutant)
                    checked=subprocess.run(changed,capture_output=True,text=True)
                    if checked.returncode:raise ValueError('Source-publication mutant did not compile '+name+': '+checked.stderr[-5000:])
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=120);full=checked.stdout+checked.stderr;diagnostic=full[:1500]+('\n... bounded diagnostic middle omitted ...\n'+full[-3500:]if len(full)>5000 else full[1500:])
                    if checked.returncode==0 or'source transport key='not in diagnostic:raise ValueError('Source-publication mutant survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
            compilers.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,declared_input_cases=len(rows),compilers=compilers,parent_pin=UNITS[files[-1]],rep_statement_sha256=hashlib.sha256(rep.encode()).hexdigest())
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={p:sha(ROOT/p)for p in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
