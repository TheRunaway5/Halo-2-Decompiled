#!/usr/bin/env python3
"""Private genuine codec scale source routes, live GP feedback and policy handoffs."""
import argparse,copy,hashlib,json,os,random,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import native_codec_scale_replacements as binding
from validate_native_codec_scale_tu15 import corpus
def parts(source):
    body=binding.canonical_body(binding.BODY.search(source)[0])
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
    state=re.search(r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;',source,re.S)[0]
    return body,resolver,dispatcher,state
def contexts():
    return {name:tuple(map(binding.inputs.digest,parts((ROOT/name).read_text())))for name in binding.UNITS}
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch,output,syntax,current_only,require_binding=False):
    command=[]
    if current_only:
        import specialize_native_operands as specialization
        for owner,name in zip(('model','sandbox'),binding.UNITS):
            source=(ROOT/name).read_text();(scratch/(owner+'-current-bound.cpp')).write_text(source)
            prepared,_=specialization.specialize(source);(scratch/(owner+'-current-prepared.cpp')).write_text(prepared)
    else:
        command=[sys.executable,'-B',str(ROOT/'tools/audit_native_codec_scale_owners_tu15.py'),'--output',str(output/'identity')]
        done=subprocess.run(command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Genuine scale owner preparation failed '+(done.stdout+done.stderr)[-7000:])
    blocks=[];rows=[]
    for owner in('model','sandbox'):
        for stage in(('current',)if current_only else('current','fresh')):
            for form in('bound','prepared'):
                text=(scratch/(owner+'-'+stage+'-'+form+'.cpp')).read_text();index=len(rows)
                if require_binding and binding.installed_entries(text)!=['296220']:raise ValueError('Codec scale source default missing')
                original=binding.remove_codec_scale(text);bound=binding.replace_codec_scale(text)
                if binding.replace_codec_scale(bound)!=bound or binding.remove_codec_scale(bound)!=original:raise ValueError('Codec private overlay exact reversal failed')
                if binding.installed_entries(bound)!=['296220']:raise ValueError('Codec private source entry missing')
                match=binding.BODY.search(bound);body=match[0];reference=binding.canonical_body(body);_,resolver,dispatcher,state=parts(bound)
                instrumented=body
                for spelling in('if(s.services.instruction){','s.instruction_literal('):instrumented=instrumented.replace(spelling,f'codec_statement({index});'+spelling)
                if instrumented.count(f'codec_statement({index});')!=8:raise ValueError('Codec retained site set changed')
                instrumented=instrumented.replace('run_296220(','bound(',1);reference=reference.replace('run_296220(','reference(',1)
                blocks.append('namespace codec_source'+str(index)+' {using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'+state+'\n'+resolver+'\n'+instrumented+'\n'+reference+'\n}\n')
                controls={}
                for name,mutated in{'body':bound.replace(body,body.replace('0x10u','0x8u',1),1),'resolver':bound.replace(resolver,resolver.replace('switch(p)','p^=1u;switch(p)',1),1),'dispatcher':bound.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1),'include':bound.replace(binding.INCLUDE,'',1),'missing_label':bound.replace(next(iter(binding.LABELS.values())),'',1),'foreign_label':bound.replace(next(iter(binding.LABELS.values())),'foreign_codec_label: ',1),'duplicate_body':bound+'\n'+body}.items():
                    if mutated==bound:raise ValueError('Codec context mutation seam changed '+name)
                    try:binding.installed_entries(mutated)
                    except ValueError as error:controls[name]=str(error)
                    else:raise ValueError('Codec strict context mutation accepted '+name)
                for pc,label in binding.LABELS.items():
                    changed=body.replace(label,'',1);at=changed.index('{')+1+len(binding.PREFIX);changed=changed[:at]+'\n'+label+changed[at:];mutated=bound.replace(body,changed,1)
                    try:binding.installed_entries(mutated)
                    except ValueError as error:controls['relocated_'+format(pc,'x')]=str(error)
                    else:raise ValueError('Codec relocated exact continuation label accepted '+hex(pc))
                checks=[]
                if syntax:
                    path=scratch/(owner+'-'+stage+'-'+form+'-codec.cpp');path.write_text(bound)
                    check=['g++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)];done=subprocess.run(check,capture_output=True,text=True)
                    if done.returncode:raise ValueError('Codec whole candidate CPP syntax failed '+done.stderr[-6000:])
                    checks.append(dict(command=check,passed=True))
                rows.append(dict(owner=owner,stage=stage,form=form,source_sha256=binding.inputs.digest(bound),body_sha256=binding.inputs.digest(body),strict_context_mutants=controls,whole_candidate_cpp_syntax=checks))
    source='\n'.join(blocks)+'\n#define CODEC_SOURCE_COUNT '+str(len(rows))+'\n#define CODEC_SOURCE_OWNERS(X) '+' '.join('X('+str(n)+')'for n in range(len(rows)))+'\n';(scratch/'codec_scale_sources.inc').write_text(source)
    return dict(genuine_identity_command=command,current_only=current_only,variants=rows,source_copy_sha256=binding.inputs.digest(source))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--smoke',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--whole-syntax',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--current-only',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--execution-timeout',type=int,default=3600);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if a.execution_timeout<=0:p.error('Execution timeout must be positive')
    import platform
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=('tools/native_codec_scale_replacements.py','tools/audit_native_codec_scale_owners_tu15.py','tools/validate_native_codec_scale_tu15.py','tools/validate_native_codec_scale_source_tu15.py','tests/native_tu15_codec_scale_source_tests.cpp')
    pins={name:sha(ROOT/name)for name in names};headers={p.resolve():sha(p)for tree in('src','include','tests')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc','.cpp')};report=dict(source_contexts_before=contexts(),unit_source_sha256_before={name:sha(ROOT/name)for name in binding.UNITS},passed=False,private_unbound=not a.require_binding,require_binding=a.require_binding,readonly_fp_metadata_domain=True,execution_timeout_seconds=a.execution_timeout,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        report['preparation']=prepare(scratch,a.output,a.whole_syntax,a.current_only,a.require_binding);rows=corpus(a.smoke)
        if a.mmx:
            rng=random.Random(0x296220a11)
            for mask in range(16 if a.smoke else 256):
                row=copy.deepcopy(rows[mask%len(rows)]);row.update(id=len(rows),name=f'codec-source-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800;row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
        if a.prefixes:
            pending=[]
            for index in(0,1,len(rows)-2,len(rows)-1):
                for phase,count in(('before_read',6),('before_preimage',1),('before_write',1),('after_write',1)):
                    for ordinal in range(count):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(pending),name=row['name']+'-'+phase+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));pending.append(row)
            rows+=pending
        fixture=scratch/'source-inputs.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/(Path(compiler).name+'-source');dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_codec_scale_source_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Codec scale source test compile failed '+done.stderr[-9000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            env=os.environ.copy();env.pop('HALO2_REFERENCE_CODEC_SCALE',None);modes={}
            done=subprocess.run([str(binary),'--guards'],capture_output=True,text=True,env=env,timeout=a.execution_timeout)
            if done.returncode:raise ValueError('Codec scale actual source untouched refusal failed '+(done.stdout+done.stderr)[-7000:])
            refusals=json.loads(done.stdout)
            for label in('default','own-reference','unrelated-control'):
                environment=env.copy()
                if label=='own-reference':environment['HALO2_REFERENCE_CODEC_SCALE']='1'
                if label=='unrelated-control':environment['HALO2_REFERENCE_PACKED_ROTATION']='1'
                done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=environment,timeout=a.execution_timeout)
                if done.returncode:raise ValueError('Codec scale actual source continuation failed '+label+': '+(done.stdout+done.stderr)[-7000:])
                modes[label]=json.loads(done.stdout)
            defects=[]
            if a.semantic_mutants:
                relative='src/physics/runtime/native_tu15_codec_scale_source_frame.hpp';source=(ROOT/relative).read_text()
                definitions=(
                    ('missing-retired-policy-handoff','if(!continuation(s))throw SourceContinuation{next,&token};','(void)next;'),
                    ('cached-output-data','store_source(s_.gp[0]+28u,s_.gp[2]);','store_source(s_.gp[0]+28u,value);'),
                    ('late-observer-presence','store_source(s_.gp[0]+28u,s_.gp[2]);','Base::store(0x296239u,s_.gp[0]+28u,s_.gp[2],[]{});'),
                    ('handoff-before-load-commit','g[0]=word(0x510f60u);checkpoint_(0x296225u);','const auto loaded=word(0x510f60u);checkpoint_(0x296225u);g[0]=loaded;'),
                    ('cached-row-offset','at(0x296228u);g[2]=word(0x510f9cu);checkpoint_(0x29622eu);g[1]=g[1]+g[2]*4u;','at(0x296228u);const auto cached=g[1];g[2]=word(0x510f9cu);checkpoint_(0x29622eu);g[1]=cached+g[2]*4u;'),
                    ('swallowed-foreign-continuation','if(resume.owner!=&token)throw;',''))
                for label,old,new in definitions:
                    if source.count(old)!=1:raise ValueError('Missing codec scale source defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));mutant=overlay/'test';mutated=list(command);mutated.insert(1,'-I'+str(overlay/'src'));mutated[mutated.index('-o')+1]=str(mutant);checked=subprocess.run(mutated,capture_output=True,text=True)
                    if checked.returncode:raise ValueError('Codec scale source defect compile failed '+checked.stderr[-6000:])
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,env=env,timeout=a.execution_timeout);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                    if not checked.returncode or not any(value in diagnostic for value in('Codec scale source continuation differs','Codec scale source swallowed/invented a provider exception')):raise ValueError('Codec scale source defect survived/unrecognized '+label+': '+diagnostic)
                    defects.append(dict(name=label,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,modes=modes,untouched_refusals=refusals,compiled_defects=defects))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        report['source_contexts_after']=contexts();report['unit_source_sha256_after']={name:sha(ROOT/name)for name in binding.UNITS}
        if report['source_contexts_after']!=report['source_contexts_before']:report.update(passed=False,source_context_drift=True)
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
