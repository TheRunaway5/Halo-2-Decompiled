#!/usr/bin/env python3
"""Public instanced-child facade sharing the source DATA policy, all other owners pinned."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import native_instanced_ray_child_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
FILES=('src/physics/native_tu15_instanced_ray_callbacks.cpp','include/halo2/native_tu15_instanced_ray_child_kernel.hpp',
    'include/halo2/native_tu15_instanced_ray_child_value.hpp','tests/native_tu15_instanced_ray_child_value_tests.cpp',
    'tests/reference/native_tu15_instanced_ray_child_value_reference.hpp','tools/native_instanced_ray_child_replacements.py',
    'tools/validate_native_instanced_ray_child_values.py','src/physics/native_tu15_joint_constraint_numeric_internal.hpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++')
    p.add_argument('--require-binding',action='store_true');p.add_argument('--sanitizers',action='store_true');p.add_argument('--leak-sanitizer',action='store_true');args=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);pins={name:sha(ROOT/name)for name in FILES};report=dict(passed=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        source=ROOT/FILES[0];text=source.read_text();bound=binding.replace_public(text)
        if args.require_binding:
            if bound!=text:raise ValueError('Shared DATA provider facade not installed')
        else:source=scratch/'public.cpp';source.write_text(bound)
        if sha(ROOT/FILES[4])!=binding.PUBLIC_REFERENCE_PIN:raise ValueError('Immutable public child source control changed')
        binary=scratch/'values';command=[args.compiler,'-std=c++20','-O1'if args.sanitizers else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',*["-I"+str(ROOT/name)for name in('include','src','src/physics','tests')],str(ROOT/FILES[3]),str(source),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True);env=dict(os.environ)
        if args.sanitizers:env['ASAN_OPTIONS']=env.get('ASAN_OPTIONS','')+(':detect_leaks=1'if args.leak_sanitizer else':detect_leaks=0')
        result=subprocess.run([str(binary)],capture_output=True,text=True,env=env,timeout=90)
        if result.returncode:raise ValueError('Shared public provider differs from original '+(result.stdout+result.stderr)[-3000:])
        if any(sha(ROOT/name)!=digest for name,digest in pins.items()):raise ValueError('Public child qualification source changed')
        interrupted=re.search(r'interrupted=(\d+)',result.stdout)
        if not interrupted:raise ValueError('Missing actual interrupted public-provider count')
        report.update(passed=True,production_binding=args.require_binding,compile_command=command,stdout=result.stdout.strip(),normal_cases=512,prefix_attempts=8192,interrupted_cases=int(interrupted[1]),
            independent_callback_collector_helper_pins=binding.PUBLIC_PRESERVATION_PINS,public_input_key_capture_preserved=True,
            explicit_observer_null_backend_and_stats_policy_preserved=True,sanitizers=args.sanitizers,leak_detection_qualified=args.sanitizers and args.leak_sanitizer,
            raw_abi_or_live_world_qualified_by_this_test=False)
    except BaseException as error:report.update(passed=False,error=repr(error));raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(result.stdout.strip());return 0
if __name__=='__main__':raise SystemExit(main())
