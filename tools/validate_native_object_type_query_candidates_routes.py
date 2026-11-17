#!/usr/bin/env python3
"""Execute exact TypeQuery candidate prefixes and all authenticated suffix labels."""
import argparse,hashlib,json,os,platform,shlex,subprocess
from pathlib import Path
import native_object_type_query_replacements as binding
import validate_native_object_type_query_owner_tu15 as owner
from oracle_smg_firing_1_1 import Xbe
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch,require_binding=False):
    harness,proof=owner.prepare(scratch)
    current=(ROOT/UNIT).read_text()
    if require_binding:binding.installed_entries(current)
    raw=binding.remove_object_type_query(current)
    sources=(raw,owner.source.specialize(raw)[0])
    candidates=[];fixtures=[]
    for source,literal in zip(sources,(False,True),strict=True):
        candidate=binding.replace_object_type_query(source);body=binding.parts(candidate)['body']
        prefix=binding.entry_prefix(body);instrumented=prefix
        for pc in binding.FRONTIERS:
            old=f'case 0x{pc:x}u:goto NativeObjectTypeQuery{pc:x};'
            if instrumented.count(old)!=1:raise ValueError('Candidate exact suffix switch changed')
            instrumented=instrumented.replace(old,f'case 0x{pc:x}u:resume=0x{pc:x}u;goto NativeObjectTypeQuery{pc:x};',1)
        body=body.replace(prefix,instrumented,1)
        reference='source_reference'if literal else'source_reference_raw'
        callback='s.instruction_literal('if literal else'if(s.services.instruction){'
        body=body.replace(callback,'++'+reference+'::parent_statements;'+callback)
        namespace='source_candidate'if literal else'source_candidate_raw'
        fixtures.append('namespace '+namespace+' {\nusing '+reference+'::sign_extend;\nstatic std::uint32_t resume{};\nstatic void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){'+reference+'::dispatch(s,target,site,tail);}\n'+body+'\n}\n')
        candidates.append(dict(format='prepared'if literal else'raw',complete_candidate_body_sha256=binding.digest(binding.parts(candidate)['body']),retained_semantics_changed=False,private_instrumentation='Record chosen suffix and retained parent statement counts only'))
    text=harness.read_text();at=text.index('namespace {\nstruct Memory')
    text=text[:at]+''.join(fixtures)+text[at:]
    begin=text.index('        if(native){');end=text.index('    }catch(const Boundary&)',begin)
    text=text[:begin]+'''        if(native){
            if(format){source_candidate_raw::resume=0u;source_candidate_raw::run_10de40(state);out.resume=source_candidate_raw::resume;}
            else{source_candidate::resume=0u;source_candidate::run_10de40(state);out.resume=source_candidate::resume;}
        }else retained(0u);
'''+text[end:]
    harness.write_text(text)
    return harness,dict(parent_and_child_provenance=proof,candidates=candidates)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,owner.OWNER,owner.source.FRAME,owner.source.TEST,
        'tools/validate_native_object_type_query_owner_tu15.py','tools/validate_native_object_type_query_candidates_routes.py',
        'tools/native_object_type_query_replacements.py','tools/specialize_native_operands.py')
    pins={name:sha(ROOT/name)for name in names};report=dict(passed=False,production_bound=a.require_binding,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    headers={path.resolve():sha(path)for tree in('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc','.cpp')}
    try:
        if sha(a.xbe)!=owner.source.physical.PIN:raise ValueError('Wrong original input')
        harness,proof=prepare(scratch,a.require_binding);report['qualification']=proof
        owner.source.physical.REGIONS=tuple(sorted((*owner.source.physical.REGIONS,(0x46a000,4096),(0x4f0000,4096))))
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(owner.source.physical.corpus(Xbe(a.xbe),True),separators=(',',':')))
        results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(harness),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True,timeout=600)
            if built.returncode:raise ValueError('TypeQuery candidate route compile failed '+built.stderr[-7000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            modes={}
            for mode in('default','own-reference','unrelated-control'):
                env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_REFERENCE_')}
                if mode=='own-reference':env['HALO2_REFERENCE_OBJECT_TYPE_QUERY']='1'
                if mode=='unrelated-control':env['HALO2_REFERENCE_OBJECT_BODY']='1'
                done=subprocess.run([str(binary),str(fixture)],env=env,capture_output=True,text=True,timeout=1200)
                if done.returncode:raise ValueError('TypeQuery candidate route failed '+mode+': '+(done.stdout+done.stderr)[-7000:])
                modes[mode]=json.loads(done.stdout)
            mutations=[]
            if a.semantic_mutants:
                original=harness.read_text()
                definitions=(
                    ('wrong-suffix-label','resume=0x10de46u;goto NativeObjectTypeQuery10de46;','resume=0x10de46u;goto NativeObjectTypeQuery10de49;'),
                    ('replay-completed-entry','case 0xffffffffu:return;','case 0xffffffffu:break;'),
                    ('wrong-raw-entry-tag','::bridge<false>(s,native_child)','::bridge<true>(s,native_child)'))
                for label,old,new in definitions:
                    if old not in original:raise ValueError('Candidate route defect seam changed '+label)
                    bad_source=scratch/(Path(compiler).name+'-'+label+'.cpp');bad_source.write_text(original.replace(old,new))
                    changed=list(command);changed[changed.index(str(harness))]=str(bad_source);changed[changed.index('-o')+1]=str(bad_source.with_suffix(''))
                    compiled=subprocess.run(changed,capture_output=True,text=True,timeout=600)
                    if compiled.returncode:raise ValueError('Candidate route defect compile failed '+label+': '+compiled.stderr[-5000:])
                    env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_REFERENCE_')}
                    bad=subprocess.run([str(bad_source.with_suffix('')),str(fixture)],env=env,capture_output=True,text=True,timeout=1200)
                    diagnostic=(bad.stdout+bad.stderr)[-5000:]
                    if not bad.returncode or not any(value in diagnostic for value in('Type-query source continuation differs','TypeQuery owner entry admission differs','Native TypeQuery entered retained parent')):
                        raise ValueError('Candidate route defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,modes=modes,compiled_mutations=mutations))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
