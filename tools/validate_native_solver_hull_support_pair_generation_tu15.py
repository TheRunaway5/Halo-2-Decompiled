#!/usr/bin/env python3
"""Complete actual313390 source generation, precise owner auth and whole-unit syntax."""
import argparse
import json
import os
import shlex
import subprocess
from pathlib import Path
import native_solver_hull_support_pair_replacements as binding
from generated_routine_ownership import function_bodies
from specialize_native_operands import specialize
from validate_native_solver_hull_support_pair_routes_tu15 import ROOT,prepare,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['src/physics/physics_live_support_tu15_generated.cpp','src/physics/runtime/physics_world_runtime_generated.cpp','tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py','tools/native_solver_hull_support_pair_replacements.py','tools/validate_native_solver_hull_support_pair_routes_tu15.py',str(Path(__file__).relative_to(ROOT))]
    pins={name:sha(ROOT/name)for name in names};headers={path.resolve():sha(path)for tree in('src','include')for path in(ROOT/tree).rglob('*')if path.suffix in('.h','.hpp','.inc')};used=set();report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        report['preparation']=prepare(scratch,False,a.require_binding);report['units']=[]
        for name,source in [('current-primary',(ROOT/names[0]).read_text()),('fresh-primary',(scratch/'fresh-primary.cpp').read_text())]:
            bound=binding.replace_solver_hull_support_pair(source);unbound=binding.remove_solver_hull_support_pair(bound)
            before={sig.symbol:body for sig,body in function_bodies(unbound)};after={sig.symbol:body for sig,body in function_bodies(bound)}
            if set(before)!=set(after)or any(body!=after[symbol]for symbol,body in before.items()if symbol!='313390'):raise ValueError('Support-pair installer changed an unrelated complete owner')
            baseline,old_receipt=specialize(unbound);prepared,receipt=specialize(bound)
            prior={key:value for key,value in old_receipt.items()if(key.endswith('_entries')or key.endswith('_leaves'))and key!='native_solver_hull_support_pair_entries'}
            if prior!={key:receipt.get(key)for key in prior}:raise ValueError('Support-pair preparation changed another owner group')
            if binding.installed_entries(prepared)!=['313390']:raise ValueError('Prepared support-pair candidate ownership missing')
            if a.require_binding and receipt.get('native_solver_hull_support_pair_entries')!=['313390']:raise ValueError('Bound support-pair preparation receipt missing')
            if specialize(prepared)[0]!=prepared:raise ValueError('Support-pair repeated preparation drift')
            commands=[]
            for mode,value in [('raw',bound),('prepared',prepared)]:
                path=scratch/(name+'-'+mode+'.cpp');path.write_text(value)
                dep=path.with_suffix('.d');command=['clang++','-std=c++20','-Wno-ignored-attributes','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(path)];checked=subprocess.run(command,capture_output=True,text=True)
                if checked.returncode:raise ValueError('Whole Primary support-pair syntax failed '+name+' '+mode+': '+checked.stderr[-7000:])
                used.update(Path(value).resolve()for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1])if Path(value).resolve()in headers)
                commands.append(command)
            report['units'].append(dict(owner=name,all_other_complete_owners_preserved=True,prior_owner_groups_preserved=prior,raw_body_sha256=binding.digest(binding.BODY.search(bound)[0]),prepared_body_sha256=binding.digest(binding.BODY.search(prepared)[0]),whole_unit_syntax_commands=commands))
        world=(ROOT/names[1]).read_text()
        if binding.replace_solver_hull_support_pair(world)!=world:raise ValueError('Other selected World identity changed')
        report.update(passed=True,production_bound=a.require_binding,scope='Current/fresh complete Primary raw/prepared candidate; exact body/resolver/dispatcher and continuation placement, all unrelated owners/groups preserved. No full313BB0 closure claim.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    report['compiled_header_sha256']={str(path.relative_to(ROOT)):headers[path]for path in sorted(used)}
    report['sources_unchanged']=report['sources_unchanged']and all(sha(path)==headers[path]for path in used)
    if not report['sources_unchanged']:report.update(passed=False,error='Support-pair generation input drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
