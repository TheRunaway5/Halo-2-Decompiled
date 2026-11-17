#!/usr/bin/env python3
"""Exact actual2e8d20 current/fresh source routes; actual owner is local only in Primary."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import native_zero_shape_support_replacements as binding
from specialize_native_operands import specialize
from validate_native_zero_shape_support_tu15 import ROOT,reference,inputs as make_inputs

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch,require_binding=False):
    reference(scratch);primary=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();fresh=scratch/'fresh-primary.cpp';command=[os.sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'fresh-primary.json')];run=subprocess.run(command,capture_output=True,text=True)
    if run.returncode:raise ValueError('Genuine zero-support fresh Primary generation failed: '+run.stderr[-5000:])
    sources=[primary,fresh.read_text()];bound=[]
    for n,source in enumerate(sources):
        value=binding.replace_zero_shape_support(source)
        if require_binding and value!=source:raise ValueError('Complete zero-support default is not installed in current/fresh Primary')
        unbound=binding.remove_zero_shape_support(value)
        rebound=binding.replace_zero_shape_support(unbound)
        include_at=value.index(binding.INCLUDE);without_include=rebound.replace(binding.INCLUDE,'',1);rebound=without_include[:include_at]+binding.INCLUDE+without_include[include_at:]
        if binding.replace_zero_shape_support(value)!=value or rebound!=value:raise ValueError('Zero support overlay reversibility/idempotence drift')
        bound.append(value)
    variants=bound+[specialize(v)[0]for v in bound];blocks=[];retained=[];receipts=[]
    for n,value in enumerate(variants):
        if binding.installed_entries(value)!=['2e8d20']:raise ValueError('Wrong complete zero-support receipt')
        match,resolver,dispatcher=binding.context(value);body=match[0];at=body.index('{')+1+len(binding.PREFIX);body=body[:at]+f'\n++literals[{n}];'+body[at:];body=body.replace('run_2e8d20(',f'bound_{n}(',1)
        controls={}
        for label,mutated in {'body':value[:match.start()]+match[0].replace('0xcu','0x8u',1)+value[match.end():],'include':value.replace(binding.INCLUDE,'',1),'prefix':value.replace(binding.PREFIX,'',1),'missing_body':value[:match.start()]+value[match.end():],'resolver':value.replace(resolver,resolver.replace('switch(p)','p^=1u;switch(p)',1),1),'dispatcher':value.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)}.items():
            if mutated==value:raise ValueError('Zero support complete-context mutation seam drift '+label)
            try:binding.installed_entries(mutated)
            except ValueError as error:controls[label]=str(error)
            else:raise ValueError('Complete zero-support context mutation accepted '+label)
        if specialize(value)[0]!=(variants[n+2]if n<2 else value):raise ValueError('Zero support repeated preparation drift')
        blocks.append(body);retained.append(body.replace(binding.PREFIX,'',1).replace(f'\n++literals[{n}];','',1).replace(f'bound_{n}(',f'retained_{n}(',1));receipts.append(dict(variant=n,input_sha256=binding.digest(value),body_sha256=binding.digest(match[0]),strict_context_mutants=controls))
    text='static std::array<unsigned,4>literals{};\n'+'\n'.join(blocks+retained)+'\nstatic void run(WorldState&s,unsigned variant,bool retained=false){switch(variant){'+''.join(f'case {n}:if(retained)retained_{n}(s);else bound_{n}(s);return;'for n in range(4))+'default:throw std::runtime_error("Wrong zero-support source variant");}}\n';(scratch/'zero_shape_support_routes.inc').write_text(text)
    world=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
    if binding.BODY.search(world):raise ValueError('Different World unexpectedly acquired a local actual zero owner')
    if binding.replace_zero_shape_support(world)!=world:raise ValueError('Unrelated World source changed')
    return dict(source_variants=receipts,source_copy_sha256=binding.digest(text),genuine_fresh_primary_command=command,world_has_no_local_actual_zero_support=True,no_child_or_supplied_outcomes=True)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['src/physics/physics_live_support_tu15_generated.cpp','src/physics/runtime/physics_world_runtime_generated.cpp','src/physics/runtime/native_tu15_zero_shape_support_source_owner.hpp','tools/native_zero_shape_support_replacements.py','tools/validate_native_zero_shape_support_tu15.py','tools/specialize_native_operands.py','tools/generate_physics_live_support_tu15.py','tests/native_tu15_zero_shape_support_routes_tests.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report['source_sha256']=pins
    try:
        report['preparation']=prepare(scratch,args.require_binding);command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_zero_shape_support_routes_tests.cpp'),'-o',str(scratch/'routes')]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Zero support source route compile failed:\n'+built.stderr[-10000:])
        inputs=scratch/'inputs.json';inputs.write_text(json.dumps(make_inputs(),separators=(',',':')));report['original_qualified_input_sha256']=sha(inputs)
        env=os.environ.copy();env.pop('HALO2_REFERENCE_ZERO_SHAPE_SUPPORT',None);report['routes']={}
        for mode,index in [('default',0),('own-reference',0),('traced-entry',1),('rebound-callback-entry',2),('uninvoked-unknown-provider',3),('without-store-observer',4),('unrelated-controls',0)]:
            environment=env.copy()
            if mode=='own-reference':environment['HALO2_REFERENCE_ZERO_SHAPE_SUPPORT']='1'
            if mode=='unrelated-controls':
                for name in('SIMPLEX_BOUNDARY','SIMPLEX_PAIRED','SOLVER_HULL_CONTEXT','MATRIX_LEAVES','RAW_ALLOCATOR'):environment['HALO2_REFERENCE_'+name]='1'
            with inputs.open('rb')as inp:probe=subprocess.run([str(scratch/'routes'),str(index)],stdin=inp,capture_output=True,text=True,env=environment)
            if probe.returncode:raise ValueError('Zero support source route failed '+mode+':\n'+probe.stderr[-6000:])
            report['routes'][mode]=json.loads(probe.stdout.strip().splitlines()[-1])
        probe=subprocess.run([str(scratch/'routes'),'--refusals'],capture_output=True,text=True,env=env)
        if probe.returncode:raise ValueError('Zero support admission failed:\n'+probe.stderr[-5000:])
        report['refusals']=json.loads(probe.stdout.strip().splitlines()[-1])
        with inputs.open('rb')as inp:source_protocol=subprocess.run([str(scratch/'routes'),'--source-publication'],stdin=inp,capture_output=True,text=True,env=env)
        if source_protocol.returncode:raise ValueError('Actual retained source publication failed: '+source_protocol.stderr[-8000:])
        report['source_publication']=json.loads(source_protocol.stdout.strip().splitlines()[-1])
        relative='physics/runtime/native_tu15_zero_shape_support_source_owner.hpp';owner=(ROOT/'src'/relative).read_text()
        if owner.count('Publication::retained')!=1:raise ValueError('Source publication negative-control seam drift')
        overlay=scratch/'wrong-source-publication';target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(owner.replace('Publication::retained','Publication::physical',1))
        bad_command=command.copy();bad_command.insert(bad_command.index('-I'+str(ROOT/'include')),'-I'+str(overlay));bad_command[-1]=str(overlay/'routes')
        bad_build=subprocess.run(bad_command,capture_output=True,text=True)
        if bad_build.returncode:raise ValueError('Wrong-publication mutant compile failed: '+bad_build.stderr[-6000:])
        with inputs.open('rb')as inp:bad=subprocess.run([str(overlay/'routes'),'--source-publication'],stdin=inp,capture_output=True,text=True,env=env)
        if not bad.returncode:raise ValueError('Wrong physical publication accepted by actual source test')
        report['wrong_source_publication_mutant']=dict(rejected=True,diagnostic=bad.stderr[-6000:])
        report.update(passed=True,production_bound=args.require_binding)
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Zero support source route input drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
