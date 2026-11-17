#!/usr/bin/env python3
"""Private actual full-specializer overlay and four complete313300 source units."""
import argparse,difflib,hashlib,json,os,re,subprocess
from pathlib import Path
import native_solver_hull_sample_replacements as binding
from specialize_native_operands import specialize
ROOT=Path(__file__).resolve().parents[1]
SPEC='tools/specialize_native_operands.py'
ANCHOR="    receipt['native_solver_hull_initialize_entries']=solver_hull_initialize_entries\n"
BRANCH='''    solver_hull_sample_entries=[]
    if '#include "physics/runtime/native_tu15_solver_hull_sample_source_owner.hpp"' in output or 'halo2::native_solver_hull_sample_owner_detail::bridge' in output:
        sample_spec=importlib.util.spec_from_file_location('native_solver_hull_sample_binding',Path(__file__).with_name('native_solver_hull_sample_replacements.py'))
        sample_helper=importlib.util.module_from_spec(sample_spec);sample_spec.loader.exec_module(sample_helper)
        sample_helper.installed_entries(text)
        output=sample_helper.rebind_prepared(output)
        solver_hull_sample_entries=sample_helper.installed_entries(output)
    receipt['native_solver_hull_sample_entries']=solver_hull_sample_entries
'''
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def staged_specializer(scratch):
    original=(ROOT/SPEC).read_text();installed="receipt['native_solver_hull_sample_entries']"in original
    if installed:return specialize,dict(staged=False,actual_specializer_sha256=binding.digest(original))
    if original.count(ANCHOR)!=1:raise ValueError('Exact specializer insertion anchor drift')
    candidate=original.replace(ANCHOR,ANCHOR+BRANCH,1);(scratch/'specializer-candidate.py').write_text(candidate)
    # Execute the complete proposed module with its actual repository dependency
    # identity. Only the module source is private; siblings are the real tools.
    namespace={'__name__':'_private_hull_sample_specializer','__file__':str(ROOT/SPEC)}
    exec(compile(candidate,str(scratch/'specializer-candidate.py'),'exec'),namespace)
    return namespace['specialize'],dict(staged=True,actual_specializer_sha256=binding.digest(original),candidate_sha256=binding.digest(candidate),private_module_dependency_identity=str(ROOT/SPEC),patch=''.join(difflib.unified_diff(original.splitlines(True),candidate.splitlines(True),fromfile=SPEC,tofile=SPEC)))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['src/physics/physics_live_support_tu15_generated.cpp',SPEC,'tools/generate_physics_live_support_tu15.py','tools/native_solver_hull_sample_replacements.py','src/physics/runtime/native_tu15_solver_hull_sample_source_owner.hpp',str(Path(__file__).relative_to(ROOT))]
    pending=list(names);seen=set(names)
    while pending:
        path=ROOT/pending.pop();source=path.read_text();targets=[]
        for header in re.findall(r'^#include "([^"\n]+)"',source,re.M):
            target=next((value for value in(path.parent/header,ROOT/'include'/header,ROOT/'src'/header)if value.is_file()),None)
            if target is not None:targets.append(target)
        if path.suffix=='.py':
            modules=set(re.findall(r"['\"]([A-Za-z_]\w+)(?:\.py)?['\"]",source))|set(re.findall(r'^(?:from|import) ([A-Za-z_]\w+)',source,re.M))
            targets.extend(ROOT/'tools'/(name+'.py')for name in modules if(ROOT/'tools'/(name+'.py')).is_file())
        for target in targets:
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        current=(ROOT/names[0]).read_text();fresh=scratch/'fresh.cpp';command=[os.sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'fresh.json')]
        done=subprocess.run(command,capture_output=True,text=True,timeout=600)
        if done.returncode:raise ValueError('Genuine complete Primary generation failed: '+done.stderr[-6000:])
        report['genuine_generation_command']=command;raw=[binding.replace_solver_hull_sample(text)for text in(current,fresh.read_text())]
        if a.require_binding and raw!=[current,fresh.read_text()]:raise ValueError('Actual current/fresh313300 defaults are not installed')
        prepare,report['specializer']=staged_specializer(scratch);report['contexts']=[];report['strict_actual_API_controls']=[]
        prepared=[]
        for text in raw:
            output,receipt=prepare(text)
            if receipt.get('native_solver_hull_sample_entries')!=['313300']or prepare(output)[0]!=output:raise ValueError('Actual staged/live specializer did not authenticate/repeat exact prepared owner')
            expected=binding.rebind_prepared(specialize(text)[0])if report['specializer']['staged']else output
            if output!=expected:raise ValueError('Staged complete specializer changed unrelated prepared source bytes')
            prepared.append(output)
        for n,text in enumerate(raw+prepared):
            if binding.installed_entries(text)!=['313300']:raise ValueError('Full source context not installed')
            match,resolver,dispatcher=binding.context(text);state=binding.STATE.search(text)[0]
            report['contexts'].append(dict(variant=n,source_sha256=binding.digest(text),body_sha256=binding.digest(match[0]),resolver_sha256=binding.digest(resolver),dispatcher_sha256=binding.digest(dispatcher),class_sha256=binding.digest(state)))
            key=n>=2,n%2==1;prefix=binding.PREFIXES[key]
            controls={'wrong-tag':text.replace(prefix,binding.PREFIXES[not key[0],key[1]],1),'wrong-spill-C1-profile':text.replace(prefix,binding.PREFIXES[key[0],not key[1]],1)}
            if n<2:
                # The old output-only check would normalize this single mixed
                # input callback into a legitimate prepared source tuple.
                controls['mixed-callback-sites']=text.replace('if(s.services.instruction){s.services.instruction(0x313304u);}','s.instruction_literal(0x313304u);',1)
            else:controls['mixed-callback-sites']=text.replace('s.instruction_literal(0x313304u);','if(s.services.instruction){s.services.instruction(0x313304u);}',1)
            failures={}
            for label,bad in controls.items():
                if bad==text:raise ValueError('Specializer API mutation seam missing '+label)
                try:prepare(bad)
                except ValueError as error:failures[label]=str(error)
                else:raise ValueError('Actual complete staged/live specializer accepted malformed source '+label)
            report['strict_actual_API_controls'].append(failures)
            path=scratch/('variant-'+str(n)+'.cpp');path.write_text(text)
            syntax=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)]
            done=subprocess.run(syntax,capture_output=True,text=True,timeout=600)
            if done.returncode:raise ValueError('Actual complete313300 source unit failed syntax '+str(n)+': '+done.stderr[-10000:])
        report.update(passed=True,production_bound=a.require_binding,scope='Genuine complete Primary current/fresh raw/prepared syntax and actual complete staged/live specializer API; no runtime/default claim beyond explicit installed flag')
    except Exception as error:report['error']=repr(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,source_drift=True)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
