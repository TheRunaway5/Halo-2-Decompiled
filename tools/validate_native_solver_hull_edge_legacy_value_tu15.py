#!/usr/bin/env python3
"""Run the unchanged legacy Edge API against an owned shared-value candidate.

Only a private inspector hook retargets the generated compiler project's three
real defects. The original legacy script, source and reference image remain
untouched; no successful child output is supplied.
"""
import argparse,hashlib,inspect,json,os,platform,re,shutil,sys
from pathlib import Path
from validate_native_solver_hull_edge_value_tu15 import public_overlay,FUNCTION
import native_solver_hull_edge_legacy_mutants as mutation
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def installed(a):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);output=scratch/'legacy-output'
    names=['tools/validate_native_solver_hull_edge_tu15.py','tools/native_solver_hull_edge_legacy_mutants.py','src/physics/native_tu15_solver_hull_edge.cpp','include/halo2/native_tu15_solver_hull_edge_kernel.hpp','include/halo2/native_tu15_solver_hull_edge_value.hpp',str(Path(__file__).relative_to(ROOT))]
    pins={name:sha(ROOT/name)for name in names};report=dict(passed=False,installed_legacy_api=True,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        from validate_native_tu15_prt_helpers import PIN
        if not a.xbe or not a.xbe.is_file()or platform.system()!='Linux':return 77
        if sha(a.xbe)!=PIN:raise ValueError('Wrong explicit original image for registered legacy Edge')
        import validate_native_solver_hull_edge_tu15 as legacy
        legacy.line.base.NEW_DEFAULT=a.xbe
        report['original_input']={'path':str(a.xbe),'sha256':PIN}
        sys.argv=[str(ROOT/'tools/validate_native_solver_hull_edge_tu15.py'),'--output',str(output)]
        rc=legacy.main();result=output/'report.json'
        if result.is_file():report['legacy_report']=json.loads(result.read_text())
        if rc or not report.get('legacy_report',{}).get('passed'):raise ValueError('Registered legacy Edge API failed')
        report['passed']=True
    except Exception as error:report['error']=repr(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report['passed']=False
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--current-public',action='store_true');p.add_argument('--proposed-main',action='store_true');p.add_argument('--installed-api',action='store_true');p.add_argument('--xbe',type=Path);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if a.installed_api:return installed(a)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);overlay=scratch/'candidate-root';overlay.mkdir()
    names=['src/physics/native_tu15_solver_hull_edge.cpp','src/physics/native_tu15_solver_hull_support.cpp','src/physics/native_tu15_contact_shape.cpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp','src/physics/native_tu15_contact_unwind_internal.hpp','tools/validate_native_solver_hull_edge_tu15.py','tools/validate_native_solver_hull_line_tu15.py','tools/validate_native_solver_hull_single_vertex_tu15.py','tools/validate_native_solver_hull_edge_value_tu15.py','tools/native_solver_hull_edge_legacy_mutants.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is not None:
                name=str(target.relative_to(ROOT))
                if name not in seen:seen.add(name);pending.append(name)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,source_sha256=pins,scratch_owner=str(scratch),legacy_script_modified=False,retained_large_artifacts=[])
    try:
        for name in names:
            if not name.startswith('src/'):continue
            target=overlay/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
        for name in('include','tools','tests','analysis'):(overlay/name).symlink_to(ROOT/name,target_is_directory=True)
        source=overlay/'src/physics/native_tu15_solver_hull_edge.cpp'
        if not a.current_public:source.write_text(public_overlay(source.read_text()))
        expected=hashlib.sha256(FUNCTION.search(source.read_text())[0].encode()).hexdigest();report['compiled_public_function_sha256']=expected
        import validate_native_solver_hull_edge_tu15 as legacy
        import validate_native_solver_hull_single_vertex_tu15 as single
        target_function=single.main;original_inspect=inspect.getsource
        original=original_inspect(target_function)
        needle=' environment=dict(base.__dict__,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment[\'main\']()'
        if original.count(needle)!=1:raise ValueError('Exact innermost legacy Edge compiler-generation seam changed')
        changed=original.replace(needle,' source=retarget_edge_mutants(source)\n'+needle,1)
        single.retarget_edge_mutants=mutation.retarget_edge_mutants
        def inspect_candidate(function):return changed if function is target_function else original_inspect(function)
        if not a.proposed_main:inspect.getsource=inspect_candidate
        patched=[]
        for module in tuple(sys.modules.values()):
            if module is sys.modules[__name__]:continue
            path=getattr(module,'__file__',None)
            if path and Path(path).is_relative_to(ROOT/'tools')and getattr(module,'ROOT',None)==ROOT:
                module.ROOT=overlay;module.__file__=str(overlay/'tools'/Path(path).name);patched.append(module.__name__)
        report['temporary_source_root_modules']=sorted(patched)
        output=scratch/'legacy-output';sys.argv=[str(overlay/'tools/validate_native_solver_hull_edge_tu15.py'),'--output',str(output)]
        try:
            if a.proposed_main:
                code=original_inspect(legacy.main)
                needle=' namespace=dict(line.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(text,namespace);return namespace[\'main\']()'
                if code.count(needle)!=1:raise ValueError('Exact public legacy Edge main proposal seam changed')
                replacement=' text=inject_edge_generation(text)\n'+needle.replace('driver_source=driver_source,','driver_source=driver_source,retarget_edge_mutants=retarget_edge_mutants,')
                code=code.replace(needle,replacement,1)
                environment=dict(legacy.__dict__,inject_edge_generation=mutation.inject_edge_generation,retarget_edge_mutants=mutation.retarget_edge_mutants)
                exec(code,environment);rc=environment['main']();report['exact_proposed_main_executed']=True
            else:rc=legacy.main()
        finally:inspect.getsource=original_inspect
        result=output/'report.json'
        if result.is_file():report['legacy_report']=json.loads(result.read_text())
        compiled=next((path for path in scratch.rglob('*.cpp')if 'project'in path.parts and FUNCTION.search(path.read_text())and hashlib.sha256(FUNCTION.search(path.read_text())[0].encode()).hexdigest()==expected),None)
        if compiled is None:raise ValueError('Legacy compiler project did not receive the exact public Edge candidate')
        report['compiled_public_unit_sha256']=sha(compiled)
        if rc or not report.get('legacy_report',{}).get('passed'):raise ValueError('Legacy Edge genuine descendant gate failed')
        report.update(passed=True,legacy_import_api_preserved=True,original_code_or_child_results_patched=False)
    except Exception as error:report['error']=repr(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,source_stability_error='Private legacy Edge dependency drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
