#!/usr/bin/env python3
"""Private authentic source-retirement/REP continuation proof; no default."""
import argparse,copy,hashlib,json,os,platform,re,shlex,subprocess
from pathlib import Path
import native_arena_initialize_continuation_replacements as binding
from validate_native_arena_initialize_frame_tu15 import corpus,rep_corpus,middle_corpus,tail_corpus,final_corpus
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_game_arena_runtime_generated.cpp'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch):
    source=(ROOT/UNIT).read_text();raw,bound=binding.body(source)
    canonical=binding.original.remove_arena_initialize(source)
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',canonical,re.S)[0]
    reference=raw
    entries={1:0x1252a9,2:0x1252b8,3:0x1252bf,4:0x1252f2,5:0x125326}
    for pc in entries.values():
        callback='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        if reference.count(callback)!=1:raise ValueError('Declared original source entry missing')
        reference=reference.replace(callback,'arena_declared_'+format(pc,'x')+': '+callback,1)
    opening=reference.index('{')+1
    reference=reference[:opening]+'\n switch(static_cast<unsigned>(declared_phase)){'+''.join('case '+str(n)+'u:goto arena_declared_'+format(pc,'x')+';'for n,pc in entries.items())+'default:break;}'+reference[opening:]
    for spelling in('if(s.services.instruction){',binding.REPEAT):
        reference=reference.replace(spelling,'statement();'+spelling)
        bound=bound.replace(spelling,'statement();'+spelling)
    (scratch/'arena_continuation_reference.inc').write_text(resolver+'\n'+reference+'\n')
    (scratch/'arena_continuation_candidate.inc').write_text(resolver+'\n'+bound+'\n')
    controls=[]
    _,authentic=binding.body(source)
    for pc,label in binding.LABELS.items():
        changed=authentic.replace(label,'',1);at=changed.index('{')+1+len(binding.PREFIX);changed=changed[:at]+'\n'+label+changed[at:]
        try:binding.canonical_body(changed)
        except ValueError:controls.append(hex(pc))
        else:raise ValueError('Relocated private Arena continuation admitted '+hex(pc))
    changed=authentic.replace(binding.REP_LABEL,'',1);at=changed.index('{')+1+len(binding.PREFIX);changed=changed[:at]+'\n'+binding.REP_LABEL+changed[at:]
    try:binding.canonical_body(changed)
    except ValueError:controls.append('repeat_callback')
    else:raise ValueError('Relocated Arena repeat callback admitted')
    return dict(original_body_sha256=binding.digest(raw),private_body_sha256=binding.digest(authentic),exact_original_reconstruction=True,relocated_label_refusals=controls)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--all-boundaries',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=(UNIT,'tools/native_arena_initialize_continuation_replacements.py','tools/native_arena_initialize_replacements.py','tools/validate_native_arena_initialize_continuation_tu15.py','tools/validate_native_arena_initialize_frame_tu15.py','src/physics/runtime/native_tu15_arena_initialize_source_frame.hpp','tests/native_tu15_arena_initialize_continuation_tests.cpp')
    pins={n:sha(ROOT/n)for n in names};headers={x.resolve():sha(x)for tree in('include','src','tests')for x in(ROOT/tree).rglob('*')if x.suffix in('.hpp','.h','.inc','.cpp')}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],scope='Actual pinned source body and exact retired-I/O suffixes. Warm returns, genuine child-entry stops and separately declared metadata/root/REP/final entries; no whole cold child outcome supplied, no physical-fault envelope or World production admission claim.')
    try:
        report['preparation']=prepare(scratch);normal=corpus();rows=[]
        groups=((0,normal),(2,rep_corpus(normal)),(3,[r for r in middle_corpus(normal)if r['phase']=='metadata']),(4,tail_corpus(normal)),(5,final_corpus(normal)))
        for phase,group in groups:
            # Per-I/O placement coverage needs bounded repeat counts; full
            # authored spans are independently qualified by the REP oracle.
            if a.all_boundaries and phase==2:group=[r for r in group if r['gp'][1]<=17]
            chosen=group[:2]if a.smoke else[group[n]for n in sorted({0,1,15,16,len(group)//2,len(group)-2,len(group)-1})if n<len(group)]
            for source in chosen:
                row=copy.deepcopy(source);row.update(phase_id=phase,observed=False);rows.append(row)
                observed=copy.deepcopy(row);observed['name']+='-observer';observed['observed']=True;rows.append(observed)
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(dict(cases=rows,all_boundaries=a.all_boundaries),separators=(',',':')));results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_arena_initialize_continuation_tests.cpp'),'-Wl,--gc-sections','-lcrypto','-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Arena source continuation compile failed '+done.stderr[-9000:])
            for word in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(word).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
            if done.returncode:raise ValueError('Arena source continuation mismatch '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutants=[]);report['compilers']=results+[result]
            if a.semantic_mutants:
                relative='src/physics/runtime/native_tu15_arena_initialize_source_frame.hpp';source=(ROOT/relative).read_text()
                defects=(('missing-policy-handoff','if(!continuation(s))throw Continuation{next,&token};','(void)next;'),('early-load-handoff','s_.gp[0]=(s_.gp[0]&0xffffff00u)|flag;\n        checkpoint_(0x125286u);','checkpoint_(0x125286u);s_.gp[0]=(s_.gp[0]&0xffffff00u)|flag;'),('early-repeat-handoff','g[7]+=(s_.other_flags&0x400u)?0xfffffffcu:4u;--g[1];','checkpoint_(0x1252b8u);g[7]+=(s_.other_flags&0x400u)?0xfffffffcu:4u;--g[1];'),('missing-final-repeat-callback','checkpoint_(0x1252b8u);','if(g[1])checkpoint_(0x1252b8u);'),('swallowed-foreign-exception','if(resume.owner!=&token)throw;',''))
                for label,old,new in defects:
                    if source.count(old)!=1:raise ValueError('Missing source continuation defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));mutant=overlay/'test';changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(mutant)
                    compiled=subprocess.run(changed,capture_output=True,text=True)
                    if compiled.returncode:raise ValueError('Arena source defect compile failed '+label+': '+compiled.stderr[-5000:])
                    rejected=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=120);full=rejected.stdout+rejected.stderr;diagnostic=full if len(full)<=4000 else full[:1800]+'\n... bounded diagnostic ...\n'+full[-1800:]
                    if rejected.returncode==0 or not any(text in full for text in('Arena source continuation differs','Normal Arena candidate retained','Foreign Arena exception')):raise ValueError('Arena continuation defect survived/unrecognized '+label+': '+diagnostic)
                    result['compiled_mutants'].append(dict(name=label,rejected=True,exit_code=rejected.returncode,diagnostic=diagnostic))
            results.append(result)
        report.update(passed=True,declared_cases=len(rows),compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={n:sha(ROOT/n)for n in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
