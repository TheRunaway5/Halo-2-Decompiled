#!/usr/bin/env python3
"""Strict complete current/fresh raw/prepared effect-reference generation."""
import argparse,hashlib,json,os,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from native_effect_reference_element_inputs import prepare
from native_effect_reference_element_replacements import replace_effect_reference_element,digest
from audit_native_effect_reference_generation_tu15 import UNITS
from specialize_native_operands import specialize

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',default='g++');p.add_argument('--report',type=Path,required=True);p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.report.parent.mkdir(parents=True,exist_ok=True)
    names=('tools/validate_native_effect_reference_element_generation_tu15.py','tools/native_effect_reference_element_inputs.py','tools/native_effect_reference_element_replacements.py',
        'tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py','tools/native_source_prefixes.py','tools/native_model_marker_lookup_replacements.py','tools/native_selected_operand_binding.py')
    names+=tuple(unit for _,unit,*_ in UNITS)+tuple(generator for _,_,generator,*_ in UNITS)
    before={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names}
    headers={p.resolve():hashlib.sha256(p.read_bytes()).hexdigest()for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,source_before=before,owned_scratch=str(scratch),binding_required=a.require_binding,retained_large_artifacts=[])
    try:
        report['ownership']=prepare(ROOT,scratch,a.require_binding);checks=[]
        for name,unit,*_ in UNITS:
            for origin,source in(('current',(ROOT/unit).read_text()),('fresh',(scratch/name/'fresh.cpp').read_text())):
                bound=replace_effect_reference_element(source);prepared,receipt=specialize(bound)
                for variant,text in(('raw',bound),('prepared',prepared)):
                    stem=name+'-'+origin+'-'+variant;cpp=scratch/(stem+'.cpp');dep=scratch/(stem+'.d');cpp.write_text(text)
                    command=[a.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/gameplay/weapons'),'-MD','-MF',str(dep),str(cpp)]
                    result=subprocess.run(command,capture_output=True,text=True,timeout=180)
                    if result.returncode:raise ValueError(stem+' whole-CPP syntax failed '+result.stderr[-6000:])
                    for path in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                        path=Path(path).resolve()
                        if path in headers:before[str(path.relative_to(ROOT))]=headers[path]
                    checks.append(dict(name=stem,whole_cpp_sha256=digest(text),passed=True))
        report.update(passed=True,complete_cpp_checks=checks,scope='Exact complete source/body/helper/public ownership,48authentication controls and12wholeCPPsyntax checks; no runtime delivery or whole-source byte-equality claim')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in before}
        if before!=report['source_after']:report.update(passed=False,source_drift=True)
        a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
