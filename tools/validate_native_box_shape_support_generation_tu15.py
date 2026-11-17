#!/usr/bin/env python3
"""Full current/fresh Primary generation and actual306790 raw/prepared syntax."""
import argparse,json,os,re,subprocess
from pathlib import Path
import native_box_shape_support_replacements as binding
from validate_native_box_shape_support_routes_tu15 import ROOT,prepare,sha
from generated_routine_ownership import function_bodies
from specialize_native_operands import specialize

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['src/physics/physics_live_support_tu15_generated.cpp','src/physics/runtime/physics_world_runtime_generated.cpp','tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py','tools/native_box_shape_support_replacements.py','tools/validate_native_box_shape_support_routes_tu15.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        report['preparation']=prepare(scratch,True);report['units']=[]
        for name,source in [('current-primary',(ROOT/names[0]).read_text()),('fresh-primary',(scratch/'fresh-primary.cpp').read_text())]:
            raw=scratch/(name+'.cpp');raw.write_text(source);unbound=binding.remove_box_shape_support(source);before={sig.symbol:body for sig,body in function_bodies(unbound)};after={sig.symbol:body for sig,body in function_bodies(source)}
            if set(before)!=set(after)or any(body!=after[symbol]for symbol,body in before.items()if symbol!='306790'):raise ValueError('Box-support installer changed an unrelated complete owner')
            prepared,receipt=specialize(source)
            if receipt.get('native_box_shape_support_entries')!=['306790']or binding.installed_entries(prepared)!=['306790']:raise ValueError('Missing exact actual box-support preparation receipt')
            if specialize(prepared)[0]!=prepared:raise ValueError('Repeated box-support preparation changed source')
            prepared_path=scratch/(name+'-prepared.cpp');prepared_path.write_text(prepared);commands=[]
            for path in(raw,prepared_path):
                command=['clang++','-std=c++20','-Wno-ignored-attributes','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)];checked=subprocess.run(command,capture_output=True,text=True)
                if checked.returncode:raise ValueError('Whole box-support Primary syntax failed '+name+': '+checked.stderr[-6000:])
                commands.append(command)
            report['units'].append(dict(owner=name,raw_body_sha256=binding.digest(binding.BODY.search(source)[0]),prepared_body_sha256=binding.digest(binding.BODY.search(prepared)[0]),all_other_complete_owners_preserved=True,whole_raw_prepared_syntax_commands=commands,native_box_shape_support_entries=receipt['native_box_shape_support_entries']))
        report.update(passed=True,world_local_actual_box_owner_absent=True)
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Box-support generation input drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
