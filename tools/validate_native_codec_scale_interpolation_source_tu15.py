#!/usr/bin/env python3
"""Private genuine scale interpolation source routes, live GP feedback and policy handoffs."""
import argparse,copy,hashlib,json,os,random,re,shlex,subprocess,sys
import time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import native_codec_scale_interpolation_replacements as binding
from validate_native_codec_scale_interpolation_tu15 import corpus
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
        for owner,name in zip(('model',),binding.UNITS,strict=True):
            source=(ROOT/name).read_text();(scratch/(owner+'-current-bound.cpp')).write_text(source)
            prepared,_=specialization.specialize(source);(scratch/(owner+'-current-prepared.cpp')).write_text(prepared)
    else:
        command=[sys.executable,'-B',str(ROOT/'tools/audit_native_codec_scale_interpolation_owners_tu15.py'),'--output',str(output/'identity')]
        done=subprocess.run(command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Genuine interpolation owner preparation failed '+(done.stdout+done.stderr)[-7000:])
    blocks=[];rows=[]
    for owner in('model',):
        for stage in(('current',)if current_only else('current','fresh')):
            for form in('bound','prepared'):
                text=(scratch/(owner+'-'+stage+'-'+form+'.cpp')).read_text();index=len(rows)
                if require_binding and binding.installed_entries(text)!=['296620']:raise ValueError('Scale interpolation source default missing')
                original=binding.remove_codec_scale_interpolation(text);bound=binding.replace_codec_scale_interpolation(text)
                if binding.replace_codec_scale_interpolation(bound)!=bound or binding.remove_codec_scale_interpolation(bound)!=original:raise ValueError('Codec private overlay exact reversal failed')
                if binding.installed_entries(bound)!=['296620']:raise ValueError('Codec private source entry missing')
                match=binding.BODY.search(bound);body=match[0];legacy=binding.canonical_body(body);reference=binding.ordered_body(legacy);_,resolver,dispatcher,state=parts(bound)
                instrumented=body
                for spelling in('if(s.services.instruction){','s.instruction_literal('):instrumented=instrumented.replace(spelling,f'codec_statement({index});'+spelling)
                if instrumented.count(f'codec_statement({index});')!=21:raise ValueError('Codec retained site set changed')
                instrumented=instrumented.replace('run_296620(','bound(',1);reference=reference.replace('run_296620(','reference(',1);legacy=legacy.replace('run_296620(','legacy_reference(',1)
                blocks.append('namespace codec_source'+str(index)+' {using namespace halo2;using namespace halo2::physics_world_runtime_detail;\nconstexpr bool literal_source='+str(form=='prepared').lower()+';\n'+state+'\n'+resolver+'\n'+instrumented+'\n'+reference+'\n'+legacy+'\n}\n')
                controls={}
                for name,mutated in{'body':bound.replace(body,body.replace('0x10u','0x8u',1),1),'resolver':bound.replace(resolver,resolver.replace('switch(p)','p^=1u;switch(p)',1),1),'dispatcher':bound.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1),'include':bound.replace(binding.INCLUDE,'',1),'missing_label':bound.replace(next(iter(binding.LABELS.values())),'',1),'foreign_label':bound.replace(next(iter(binding.LABELS.values())),'foreign_codec_label: ',1),'duplicate_body':bound+'\n'+body}.items():
                    if mutated==bound:raise ValueError('Codec context mutation seam changed '+name)
                    try:binding.installed_entries(mutated)
                    except ValueError as error:controls[name]=str(error)
                    else:raise ValueError('Codec strict context mutation accepted '+name)
                for pc,label in binding.LABELS.items():
                    prefix=binding.PREPARED_PREFIX if form=='prepared'else binding.RAW_PREFIX
                    changed=body.replace(label,'',1);at=changed.index('{')+1+len(prefix);changed=changed[:at]+'\n'+label+changed[at:];mutated=bound.replace(body,changed,1)
                    try:binding.installed_entries(mutated)
                    except ValueError as error:controls['relocated_'+format(pc,'x')]=str(error)
                    else:raise ValueError('Codec relocated exact continuation label accepted '+hex(pc))
                for original,ordered in binding.ORDERED.items():
                    mutated=bound.replace(body,body.replace(ordered,original,1),1)
                    try:binding.installed_entries(mutated)
                    except ValueError as error:controls['unordered_'+original]=str(error)
                    else:raise ValueError('Codec incomplete ordered fallback accepted')
                prefix=binding.PREPARED_PREFIX if form=='prepared'else binding.RAW_PREFIX
                wrong=binding.RAW_PREFIX if form=='prepared'else binding.PREPARED_PREFIX
                for label,mutated in (('wrong_format_tag',bound.replace(prefix,wrong,1)),('old_untagged_prefix',bound.replace(prefix,binding.OLD_PREFIX,1)),('relocated_entry',bound.replace(body,body.replace(prefix,'',1)[:-1]+prefix+'}',1))):
                    try:binding.installed_entries(mutated)
                    except ValueError as error:controls[label]=str(error)
                    else:raise ValueError('Codec wrong source-format or entry location accepted '+label)
                if form=='prepared':
                    transitional=bound.replace(binding.PREPARED_PREFIX,binding.RAW_PREFIX,1)
                    if binding.rebind_prepared(transitional)!=bound or binding.rebind_prepared(bound)!=bound:raise ValueError('Exact late prepared retag failed')
                else:
                    import specialize_native_operands as specialization
                    transformed,_=specialization.specialize(bound)
                    retagged=binding.rebind_prepared(transformed)
                    canonical=binding.remove_codec_scale_interpolation(bound);independent,_=specialization.specialize(canonical)
                    if retagged!=binding.replace_codec_scale_interpolation(independent):raise ValueError('Whole actual raw-to-prepared interpolation retag differs')
                checks=[]
                if syntax:
                    path=scratch/(owner+'-'+stage+'-'+form+'-codec.cpp');path.write_text(bound)
                    check=['g++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)];done=subprocess.run(check,capture_output=True,text=True)
                    if done.returncode:
                        (output/(owner+'-'+stage+'-'+form+'-syntax-error.log')).write_text(done.stderr[:524288]+done.stderr[-524288:])
                        errors='\n'.join(line for line in done.stderr.splitlines()if 'error:'in line)
                        raise ValueError('Codec whole candidate CPP syntax failed '+errors[:6000])
                    checks.append(dict(command=check,passed=True))
                rows.append(dict(owner=owner,stage=stage,form=form,source_sha256=binding.inputs.digest(bound),body_sha256=binding.inputs.digest(body),strict_context_mutants=controls,whole_candidate_cpp_syntax=checks))
    source='\n'.join(blocks)+'\n#define CODEC_SOURCE_COUNT '+str(len(rows))+'\n#define CODEC_SOURCE_OWNERS(X) '+' '.join('X('+str(n)+')'for n in range(len(rows)))+'\n';(scratch/'codec_scale_interpolation_sources.inc').write_text(source)
    return dict(genuine_identity_command=command,current_only=current_only,variants=rows,source_copy_sha256=binding.inputs.digest(source))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--smoke',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--whole-syntax',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--current-only',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--execution-timeout',type=int,default=3600);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if a.execution_timeout<=0:p.error('Execution timeout must be positive')
    import platform
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False);started=time.monotonic()
    names=('tools/native_codec_scale_interpolation_replacements.py','tools/audit_native_codec_scale_interpolation_owners_tu15.py','tools/validate_native_codec_scale_interpolation_tu15.py','tools/validate_native_codec_scale_interpolation_source_tu15.py','tests/native_tu15_codec_scale_interpolation_source_tests.cpp')
    tools_before={str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'tools').rglob('*.py')}
    pins={name:sha(ROOT/name)for name in names};headers={p.resolve():sha(p)for tree in('src','include','tests')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc','.cpp')};report=dict(source_contexts_before=contexts(),unit_source_sha256_before={name:sha(ROOT/name)for name in binding.UNITS},passed=False,private_unbound=not a.require_binding,require_binding=a.require_binding,readonly_fp_metadata_domain=True,execution_timeout_seconds=a.execution_timeout,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        report['reference_arithmetic']='No reference arithmetic rewrite; full original scalar SSE body and literal control retained'
        report['preparation']=prepare(scratch,a.output,a.whole_syntax,a.current_only,a.require_binding);rows=corpus(a.smoke)
        for row in rows:
            banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
            def get(address):
                for region,bank in banks.items():
                    if region<=address<=region+len(bank)-4:return int.from_bytes(bank[address-region:address-region+4],'little')
                raise ValueError('Source shadow factory read escaped backing')
            factory=get(0x510f60);offset=get(factory+16)
            for region,bank in banks.items():
                if region<=factory+20<=region+len(bank)-4:bank[factory+20-region:factory+24-region]=((offset-4)&0xffffffff).to_bytes(4,'little');break
            else:raise ValueError('Source shadow factory write escaped backing')
            for bank in row['banks']:bank['before']=banks[bank['address']].hex()
        if a.mmx:
            rng=random.Random(0x296620a11)
            for mask in range(16 if a.smoke else 256):
                row=copy.deepcopy(rows[mask%len(rows)]);row.update(id=len(rows),name=f'codec-source-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800;row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
        if a.prefixes:
            pending=[]
            for index in(0,1,len(rows)-2,len(rows)-1):
                for phase,count in(('before_read',13),('before_preimage',3),('before_write',3),('after_write',3)):
                    for ordinal in range(count):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(pending),name=row['name']+'-'+phase+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));pending.append(row)
            rows+=pending
        fixture=scratch/'source-inputs.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/(Path(compiler).name+'-source');dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_codec_scale_interpolation_source_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Scale interpolation source test compile failed '+done.stderr[-9000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            env=os.environ.copy();env.pop('HALO2_REFERENCE_CODEC_SCALE_INTERPOLATION',None);modes={}
            done=subprocess.run([str(binary),'--guards'],capture_output=True,text=True,env=env,timeout=a.execution_timeout)
            if done.returncode:raise ValueError('Scale interpolation actual source untouched refusal failed '+(done.stdout+done.stderr)[-7000:])
            refusals=json.loads(done.stdout)
            done=subprocess.run([str(binary),'--legacy',str(fixture)],capture_output=True,text=True,env=env,timeout=a.execution_timeout)
            if done.returncode:raise ValueError('Interpolation unmodified retained control failed '+(done.stdout+done.stderr)[-7000:])
            legacy=json.loads(done.stdout)
            for label in('default','own-reference','unrelated-control'):
                environment=env.copy()
                if label=='own-reference':environment['HALO2_REFERENCE_CODEC_SCALE_INTERPOLATION']='1'
                if label=='unrelated-control':environment['HALO2_REFERENCE_PACKED_ROTATION']='1'
                done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=environment,timeout=a.execution_timeout)
                if done.returncode:raise ValueError('Scale interpolation actual source continuation failed '+label+': '+(done.stdout+done.stderr)[-7000:])
                modes[label]=json.loads(done.stdout)
                if label=='own-reference':
                    if modes[label]['native_calls'] or modes[label]['authenticated_continuations'] or not modes[label]['retained_suffix_statements']:
                        raise ValueError('Actual scale interpolation reference control did not execute its retained route')
                elif not modes[label]['native_calls']:
                    raise ValueError('Actual scale interpolation native route was not exercised '+label)
            defects=[];results.append(dict(compiler=compiler,command=command,modes=modes,untouched_refusals=refusals,unmodified_retained_control=legacy,compiled_defects=defects));report['compilers']=results
            if a.semantic_mutants:
                relative='src/physics/runtime/native_tu15_codec_scale_interpolation_source_frame.hpp';source=(ROOT/relative).read_text()
                definitions=(
                    ('missing-retired-policy-handoff','if(!continuation(s,next))throw SourceContinuation{next,&token};','(void)next;'),
                    ('swallowed-foreign-continuation','if(resume.owner!=&token)throw;','(void)token;'),
                    ('before-first-load-commit','at(0x296623u);g[1]=word(0x510f60u);checkpoint_(0x296629u);','at(0x296623u);const auto first=word(0x510f60u);checkpoint_(0x296629u);g[1]=first;'),
                    ('late-installed-observer','if(observing&&s_.observe)','if(s_.observe)'),
                    ('factor-handoff-before-mul-commit','s_.xmm[1]=_mm_mul_ss(s_.xmm[1],scalar(word(0x510f4cu)));checkpoint_(0x29666du);','s_.xmm[1]=_mm_mul_ss(s_.xmm[1],scalar([&]{const auto factor=word(0x510f4cu);checkpoint_(0x29666du);return factor;}()));'),
                    ('cached-first-lane-before-factor','__m128 add_first(__m128,__m128)const{return _mm_add_ss(s_.xmm[1],s_.xmm[0]);}','__m128 add_first(__m128,__m128 old_first)const{return _mm_add_ss(s_.xmm[1],old_first);}'),
                    ('cached-first-stack-value','Checkpoint&checkpoint_;','Checkpoint&checkpoint_;std::uint32_t cached_first_{};'))
                for label,old,new in definitions:
                    if source.count(old)!=1:raise ValueError('Missing scale interpolation source defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/relative;target.parent.mkdir(parents=True);changed=source.replace(old,new,1)
                    if label=='cached-first-stack-value':
                        changed=changed.replace('g[2]=word(g[0]+g[1]*4u);checkpoint_(0x296641u);','g[2]=word(g[0]+g[1]*4u);cached_first_=g[2];checkpoint_(0x296641u);',1)
                        changed=changed.replace('store(0x296647u,g[4]+4u,g[2]);','store(0x296647u,g[4]+4u,cached_first_);',1)
                    target.write_text(changed);mutant=overlay/'test';mutated=list(command);mutated.insert(1,'-I'+str(overlay/'src'));mutated[mutated.index('-o')+1]=str(mutant);checked=subprocess.run(mutated,capture_output=True,text=True)
                    if checked.returncode:raise ValueError('Scale interpolation source defect compile failed '+checked.stderr[-6000:])
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,env=env,timeout=a.execution_timeout);complete_diagnostic=checked.stdout+checked.stderr
                    diagnostic=complete_diagnostic if len(complete_diagnostic)<=6000 else complete_diagnostic[:2000]+'\n[bounded diagnostic tail]\n'+complete_diagnostic[-4000:]
                    if not checked.returncode or not any(value in complete_diagnostic for value in('Scale interpolation source continuation differs','Scale interpolation source swallowed/invented a provider exception')):raise ValueError('Scale interpolation source defect survived/unrecognized '+label+': '+diagnostic)
                    if label=='factor-handoff-before-mul-commit'and ' / 28 / 'not in complete_diagnostic:raise ValueError('Early factor handoff did not fail the late policy-revocation case: '+diagnostic)
                    defects.append(dict(name=label,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['elapsed_seconds']=round(time.monotonic()-started,3)
        for module in tuple(sys.modules.values()):
            file=getattr(module,'__file__',None)
            if file and Path(file).resolve().is_relative_to(ROOT/'tools'):
                name=str(Path(file).resolve().relative_to(ROOT));pins[name]=tools_before[name]
        report['source_after']={name:sha(ROOT/name)for name in pins}
        report['source_contexts_after']=contexts();report['unit_source_sha256_after']={name:sha(ROOT/name)for name in binding.UNITS}
        if report['source_contexts_after']!=report['source_contexts_before']:report.update(passed=False,source_context_drift=True)
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
