#!/usr/bin/env python3
"""Run established hull-value descendants against an owned public313390 candidate."""
import argparse
import hashlib
import json
import os
import re
import shutil
import sys
from pathlib import Path
from validate_native_solver_hull_support_pair_value_tu15 import public_overlay,NEW_PIN,FUNCTION

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--mode',choices=('declared','genuine','pair-declared'),required=True);p.add_argument('--archive',type=Path);p.add_argument('--current-public',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if a.mode=='genuine'and not a.archive:p.error('Genuine mode requires its existing authenticated archive')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);overlay=scratch/'candidate-root';overlay.mkdir()
    names=['src/physics/native_tu15_solver_hull_query.cpp','src/physics/native_tu15_solver_hull_support.cpp','src/physics/native_tu15_solver_hull_single_vertex.cpp','src/physics/native_tu15_contact_shape.cpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp','src/physics/native_tu15_contact_unwind_internal.hpp','tools/validate_native_solver_hull_support_pair_tu15.py','tools/validate_native_solver_hull_initialize_tu15.py','tools/validate_native_solver_hull_single_vertex_tu15.py','tools/validate_native_solver_hull_query_tu15.py','tools/validate_native_solver_hull_query_actual_tu15.py','tools/validate_native_solver_hull_query_defined_data_tu15.py','tools/validate_native_solver_hull_support_pair_value_tu15.py',str(Path(__file__).relative_to(ROOT))]
    names+=['include/halo2/native_tu15_solver_hull_support_pair_kernel.hpp','include/halo2/native_tu15_solver_hull_support_pair_value.hpp','tools/native_solver_hull_support_pair_legacy_mutants.py']
    pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is not None:
                relative=str(target.relative_to(ROOT))
                if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,mode=a.mode,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        # Only temporary source ownership changes: existing algorithms, fixtures,
        # imports and original execution remain those of the registered suite.
        for name in names:
            if not name.startswith('src/'):continue
            target=overlay/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
        for name in('include','tools','tests','analysis'):(overlay/name).symlink_to(ROOT/name,target_is_directory=True)
        source=overlay/'src/physics/native_tu15_solver_hull_support.cpp'
        if not a.current_public:source.write_text(public_overlay(source.read_text()))
        expected_function_pin=hashlib.sha256(FUNCTION.search(source.read_text())[0].encode()).hexdigest()
        if not a.current_public and expected_function_pin!=NEW_PIN:raise ValueError('Wrong complete public candidate')
        report['compiled_public_function_sha256']=expected_function_pin
        if a.mode=='pair-declared':import validate_native_solver_hull_support_pair_tu15 as legacy
        elif a.mode=='declared':import validate_native_solver_hull_query_tu15 as legacy
        else:
            import validate_native_solver_hull_query_defined_data_tu15 as legacy
            from validate_native_solver_hull_context_value_tu15 import ARCHIVE_PIN,MANIFEST_PIN
            if sha(a.archive)!=ARCHIVE_PIN or sha(a.archive.parent/'capture-native-before.json')!=MANIFEST_PIN:raise ValueError('Genuine existing BEFORE archive identity changed')
            report['required_reference_input']=dict(path=str(a.archive.resolve()),sha256=ARCHIVE_PIN,bytes=a.archive.stat().st_size,manifest_sha256=MANIFEST_PIN,unchanged_existing_reference=True)
        patched=[]
        for module in tuple(sys.modules.values()):
            if module is sys.modules[__name__]:continue
            path=getattr(module,'__file__',None)
            if not path or not Path(path).is_relative_to(ROOT/'tools'):continue
            if getattr(module,'ROOT',None)==ROOT:
                module.ROOT=overlay;module.__file__=str(overlay/'tools'/Path(path).name);patched.append(module.__name__)
        report['temporary_source_root_modules']=sorted(patched)
        output=scratch/'legacy-output';sys.argv=[str(overlay/'tools'/Path(legacy.__file__).name),'--output',str(output)]
        if a.mode=='genuine':sys.argv+=['--archive',str(a.archive),'--archive-sha256',report['required_reference_input']['sha256']]
        rc=legacy.main();result=output/'report.json'
        if result.is_file():report['legacy_report']=json.loads(result.read_text())
        if a.mode=='genuine':
            copied=source
            if report.get('legacy_report',{}).get('source_hashes',{}).get('src/physics/native_tu15_solver_hull_support.cpp')!=sha(source):raise ValueError('Genuine descendant did not authenticate its public candidate compile unit')
        else:copied=next((path for path in scratch.rglob('*.cpp')if 'project'in path.parts and FUNCTION.search(path.read_text())and hashlib.sha256(FUNCTION.search(path.read_text())[0].encode()).hexdigest()==expected_function_pin),None)
        if copied is None or not FUNCTION.search(copied.read_text())or hashlib.sha256(FUNCTION.search(copied.read_text())[0].encode()).hexdigest()!=expected_function_pin:raise ValueError('Legacy compiler project did not receive the complete public compile unit')
        report['compiled_public_candidate_sha256']=sha(copied)
        if rc or not report.get('legacy_report',{}).get('passed'):raise ValueError('Established candidate hull-value descendant failed')
        report.update(passed=True,original_code_or_child_results_patched=False,legacy_import_api_unchanged=True)
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Legacy public-candidate source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','mode','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
