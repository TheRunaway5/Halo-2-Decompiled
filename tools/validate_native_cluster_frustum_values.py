#!/usr/bin/env python3
"""Compare the shared group/range DATA loop to its pinned public predecessor."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES=('tools/validate_native_cluster_frustum_values.py','tests/native_tu15_cluster_frustum_value_tests.cpp','tests/native_tu15_cluster_frustum_value_reference.inc','include/halo2/native_tu15_cluster_frustum_kernel.hpp','include/halo2/native_tu15_cluster_frustum_value.hpp','src/rendering/world/source_visibility.cpp','include/halo2/source_visibility.hpp','include/halo2/native_tu15_sphere_frustum_value.hpp','include/halo2/native_tu15_sphere_frustum_kernel.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','tools/native_cluster_frustum_replacements.py')
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}
    from native_cluster_frustum_replacements import PUBLIC_PIN,replace_world_visibility
    reference=(ROOT/FILES[2]).read_text().rstrip().replace('source_cluster_frustum_visibility_reference(','source_cluster_frustum_visibility(',1)
    if hashlib.sha256(reference.encode()).hexdigest()!=PUBLIC_PIN:raise ValueError('Frozen public cluster reference drift')
    source=(ROOT/'src/rendering/world/source_visibility.cpp').read_text();bound=replace_world_visibility(source)
    if args.require_binding and bound!=source:raise ValueError('Public shared cluster binding missing')
    if replace_world_visibility(bound)!=bound:raise ValueError('Public cluster binding nonidempotent')
    binary=scratch/'cluster-values';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_cluster_frustum_value_tests.cpp'),str(ROOT/'src/rendering/world/source_visibility.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Cluster values compile failed: '+compiled.stderr[-5000:])
    tested=subprocess.run([str(binary)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    if pins!={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}:raise ValueError('Public cluster proof source drift')
    report=dict(passed=tested.returncode==0,owned_scratch=str(scratch),source_sha256=pins,compiler=args.compiler,sanitizers=args.sanitizers,scope='Source-backed value/control/FP regression; raw ABI separately compared with physical original',retained_large_artifacts=[],diagnostic=(tested.stdout+tested.stderr)[-6000:])
    if tested.returncode==0:report['values']=json.loads(tested.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return tested.returncode
if __name__=='__main__':raise SystemExit(main())
