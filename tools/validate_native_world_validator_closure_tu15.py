#!/usr/bin/env python3
"""Check the exact compatibility generator against the compiled100-CPP cohort."""
import argparse,ast,json,os,re,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();s=(a.stage/'staged/tools/validate_native_world_integration_tu15.py').read_text();tree=ast.parse(s)
 nodes=[n.value for n in ast.walk(tree)if isinstance(n,ast.AugAssign)and isinstance(n.target,ast.Name)and n.target.id=='cmake'and isinstance(n.value,ast.JoinedStr)and any(isinstance(v,ast.Constant)and isinstance(v.value,str)and v.value.startswith('add_executable(native_tu15_world_residuals_tests ')for v in n.value.values)];assert len(nodes)==1
 cmake=eval(compile(ast.Expression(nodes[0]),'<originaltarget>','eval'),dict(ROOT=ROOT,source=ROOT/'src/physics/native_tu15_world_integration.cpp',sys=sys))
 begin=s.index("    production_cmake=(ROOT/'cmake/PhysicsLiveWorldTu15.cmake').read_text()");end=s.index("    (project/'CMakeLists.txt').write_text(cmake)",begin);scope=dict(ROOT=ROOT,cmake=cmake,re=re);exec(compile(__import__('textwrap').dedent(s[begin:end]),'<actualcompatibilityblock>','exec'),scope)
 target=scope['cmake'].split('add_executable(native_tu15_world_residuals_tests ',1)[1].split(')\n',1)[0];files=re.findall(r'"([^"\n]+\.cpp)"',target);assert len(files)==len(set(files))==100
 required=json.loads((ROOT/'analysis/physics/native-world-island-merge-stage-model-v2-20261101/report.json').read_text())['compiled_cpp_closure'];assert set(files)=={str(ROOT/p)for p in required}
 save(a.stage/'compatibility-report.json',dict(passed=True,scope='Exact staged CMake closure block executed and compared with GCC/Clang-qualified native100-CPP World cohort; entire legacy physical validator not rerun',unique_cpp_count=len(files),compiled_cpp_closure=required,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 pins=json.loads((a.stage/'source-pins.json').read_text());pins.update(qualified=True,qualification_scope='Python syntax/exact emitted CMake closure equivalence to qualified100-CPP coherent World model',staged_hashes={'tools/validate_native_world_integration_tu15.py':sha(a.stage/'staged/tools/validate_native_world_integration_tu15.py')},validator_source_hash=sha(Path(__file__)));save(a.stage/'source-pins.json',pins);print(dict(passed=True,unique_cpp_count=100));return 0
if __name__=='__main__':raise SystemExit(main())
