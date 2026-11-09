#!/usr/bin/env python3
"""Exercise the exact frozen-cohort selector in CTest's real regex engine."""
import argparse
import ast
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('native_cohort_filter',ROOT/'tools/native_cohort_test_filter.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
FILES=('tools/native_cohort_test_filter.py','tools/validate_native_replacement_snapshot.py','tests/test_native_cohort_test_filter.py')
def hashes():return {n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in FILES}

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        assert helper.expand_test_names('^(a(b|c)?|z)$')==('ab','ac','a','z')
        assert helper.expand_test_names('^(a_?b|a_?b)$')==('a_b','ab')
        bad=('a','^(a*)$','^(a.)$','^([ab])$','^((a)$','^(a))$','^(a|)$')
        for pattern in bad:
            try:helper.expand_test_names(pattern)
            except ValueError:pass
            else:raise AssertionError('Non-finite or malformed selector accepted: '+pattern)
        tree=ast.parse((ROOT/'tools/validate_native_replacement_snapshot.py').read_text())
        values=[ast.literal_eval(node.value)for node in tree.body if isinstance(node,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='TEST_FILTER_PATTERN'for t in node.targets)]
        assert len(values)==1
        original=values[0];names=helper.expand_test_names(original);flat=helper.flatten_test_filter(original)
        assert re.compile(flat).groups==1
        negatives=tuple('unselected_'+name for name in names)+tuple(name+'_unselected'for name in names)
        for name in (*names,*negatives):assert bool(re.fullmatch(original,name))==bool(re.fullmatch(flat,name))
        # CTest consumes this fixture directly; no compiler/build tree or tests
        # are executed, and the complete data lives under the owned scratch.
        fixture=scratch/'ctest';fixture.mkdir();cmake=shutil.which('cmake');ctest=shutil.which('ctest');assert cmake and ctest
        (fixture/'CTestTestfile.cmake').write_text(''.join(f'add_test("{name}" "{cmake}" "-E" "true")\n'for name in (*names,*negatives)))
        command=[ctest,'--test-dir',str(fixture),'--show-only=json-v1','--tests-regex',flat]
        result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode==0,result.stderr+result.stdout
        selected=[row['name']for row in json.loads(result.stdout)['tests']]
        assert selected==list(names),(len(selected),len(names))
        report.update(passed=True,selected_tests=len(names),negative_names=len(negatives),malformed_rejections=len(bad),original_capture_groups=re.compile(original).groups,executed_capture_groups=1,ctest_exact_selection=True,flat_filter_sha256=hashlib.sha256(flat.encode()).hexdigest())
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
