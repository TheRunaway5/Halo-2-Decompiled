#!/usr/bin/env python3
"""Capture an actual native D8110 call and compare unchanged original i386.

The separate probe owns only a diagnostic fixture/build and raw captures. It
borrows authenticated libraries from the still-active native build read-only.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def native_link_inputs(build):
    """Borrow the active fixture's exact link inputs without copying archives."""
    link=shlex.split((build/'CMakeFiles/destruction_runtime_tu15_tests.dir/link.txt').read_text())
    libraries=[];pins={}
    for token in link[link.index('-o')+2:]:
        if any(c in token for c in '\n\r";$'):raise ValueError('Unexpected native link syntax')
        if token=='-ldl':libraries.append(token);continue
        if token.startswith('-Wl,@'):
            path=Path(token[len('-Wl,@'):]).resolve(strict=True)
            if not path.is_relative_to(build):raise ValueError('Response file escaped native build')
            pins[str(path)]=sha(path);libraries.append(token);continue
        if token.startswith('-'):raise ValueError('Unqualified native link flag')
        path=Path(token)if Path(token).is_absolute()else build/token
        pins[str(path)]=sha(path);libraries.append(str(path))
    return libraries,pins


def run_probe(args,owner):
    args.output.mkdir(parents=True,exist_ok=True)
    native_report=owner/'actual-material-entry.json'
    command=[str(args.binary),str(args.map),str(args.map.parent),'ghost',str(native_report)]
    env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CAPTURE_MATERIAL='1',
        HALO2_DESTRUCTION_QUERY_CAPTURE_ONLY='1',H2_SECURITY_LIBRARY=str(args.security_library))
    log=owner/'capture.log'
    with log.open('w')as output:result=subprocess.run(command,env=env,stdout=output,stderr=subprocess.STDOUT,timeout=240).returncode
    capture=owner/'material-lookup-capture'
    native=json.loads(native_report.read_text())if native_report.exists()else{}
    if result or not native.get('capture_only')or not(capture/'capture.json').exists():
        (args.output/'capture-failure.log').write_text(log.read_text()[-16000:])
        raise ValueError('Ordinary native gameplay produced no complete material entry/return')
    metadata=json.loads((capture/'capture.json').read_text())
    activity=native.get('captured_material_activity',{})
    if metadata['entry']!=0xd8110 or activity.get('calls')!=1 or not activity.get('modifiers_found'):
        raise ValueError('Capture did not execute the native material kernel')
    (args.output/'native-entry.json').write_text(json.dumps(native,indent=2)+'\n')
    command=[sys.executable,str(ROOT/'tools/qualify_destruction_original_entry_tu15.py'),
        '--capture-name',capture.name,'--output',str(args.output/'original')]
    result=subprocess.run(command).returncode
    print(json.dumps(dict(original_returncode=result,capture_scope='Actual native D8110 call only; no new destruction acceptance')),flush=True)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run-probe',action='store_true')
    parser.add_argument('--binary',type=Path)
    parser.add_argument('--map',type=Path)
    parser.add_argument('--security-library',type=Path)
    parser.add_argument('--admission',type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);args.output=args.output.resolve()
    if args.run_probe:
        if not(args.binary and args.map and args.security_library):parser.error('Probe requires binary, map and security library')
        return run_probe(args,owner)
    if not args.admission:parser.error('Build requires the active native admission')
    admission=json.loads(args.admission.read_text());frozen=Path(admission['owner']).resolve(strict=True)
    inputs=frozen/'source';build=frozen/'build';source=owner/'source';source.mkdir()
    if any(sha(inputs/name)!=row['after']for name,row in admission['changes'].items()):raise ValueError('Native material source drifted')
    fixture='tests/destruction_runtime_tu15_tests.cpp';before=(inputs/fixture).read_text();after=before
    edits=[
        ('  const bool capture_model=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL");\n',
         '  const bool capture_model=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL");\n  const bool capture_material=std::getenv("HALO2_DESTRUCTION_CAPTURE_MATERIAL");\n  SourceDamageMaterialNativeActivity material_capture_before{};\n  bool material_capture_started=false,material_capture_finished=false;\n'),
        ('const auto campaign_capture_entry=capture_model?', 'const auto campaign_capture_entry=capture_material?0xd8110u:capture_model?'),
        ('if(capture_model || capture_record || (campaign_scenery', 'if(capture_material || capture_model || capture_record || (campaign_scenery'),
        ('(capture_model?"model-lookup-capture":capture_record?', '(capture_material?"material-lookup-capture":capture_model?"model-lookup-capture":capture_record?'),
        ('if(capture_model || campaign_scenery || campaign_no_damage || name=="wraith_minigun")campaign_query_observation',
         'if(capture_material || capture_model || campaign_scenery || campaign_no_damage || name=="wraith_minigun")campaign_query_observation'),
        ('    if(campaign_query_capture && entry==campaign_capture_entry) {\n',
         '''    if(campaign_query_capture && entry==campaign_capture_entry) {
      if(capture_material && before && !material_capture_started) {
        material_capture_before=source_damage_material_native_activity();material_capture_started=true;
      }
'''),
        ('      if(capture_model && !before)model_capture_active=false;\n',
         '''      if(capture_model && !before)model_capture_active=false;
      if(capture_material && !before && !material_capture_finished && std::filesystem::exists(campaign_query_directory/"capture.json")) {
        const auto activity=source_damage_material_native_activity();material_capture_finished=true;
        report.data["captured_material_activity"]={{"calls",activity.calls-material_capture_before.calls},
            {"groups_found",activity.groups_found-material_capture_before.groups_found},
            {"modifiers_found",activity.modifiers_found-material_capture_before.modifiers_found}};
      }
''')]
    for needle,replacement in edits:
        if after.count(needle)!=1:raise ValueError('Material entry fixture seam changed')
        after=after.replace(needle,replacement)
    target=source/fixture;target.parent.mkdir(parents=True);target.write_text(after)
    libraries,pins=native_link_inputs(build)
    security=build/'online-protocol/Release/libhalo2_online_security_ffi.so';pins[str(security)]=sha(security)
    # Every header is from the exact native source cohort. No workspace drift
    # or copied test archives are admitted through this diagnostic fixture.
    headers={str(p):sha(p)for parent in ('include','src','tests/helpers')for p in (inputs/parent).rglob('*.hpp')if p.is_file()}
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2LiveMaterialEntry LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(probe tests/destruction_runtime_tu15_tests.cpp)
target_compile_features(probe PRIVATE cxx_std_20)
target_compile_options(probe PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(probe PRIVATE nlohmann_json::nlohmann_json)
'''
    cmake+='target_include_directories(probe PRIVATE '+' '.join('"'+str(inputs/p)+'"'for p in ('include','src','tests'))+' /usr/include/SDL2)\n'
    cmake+='target_link_libraries(probe PRIVATE '+' '.join('"'+token+'"'for token in libraries)+')\n'
    map_path=ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11/coagulation.map'
    cmake+=f'add_test(NAME live_native_material_original COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- "{sys.executable}" -B "{Path(__file__).resolve()}" --run-probe --binary $<TARGET_FILE:probe> --map "{map_path}" --security-library "{security}" --output "{args.output/"actual-entry"}")\n'
    cmake+='set_tests_properties(live_native_material_original PROPERTIES TIMEOUT 360)\n'
    (source/'CMakeLists.txt').write_text(cmake)
    args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,owned_scratch=str(owner),native_owner=str(frozen),fixture_original_sha256=hashlib.sha256(before.encode()).hexdigest(),
        fixture_capture_only_sha256=sha(target),diagnostic_substitutions=edits,native_libraries_sha256=pins,native_headers_sha256=headers,
        fixture_gameplay_and_engine_results_changed=False,tool_sha256=sha(__file__),retained_large_artifacts=[],full_goal_complete=False)
    def save():(args.output/'qualification.json').write_text(json.dumps(report,indent=2)+'\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','probe',
        '--filter','^live_native_material_original$','--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    report['command']=command;save()
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(returncode=result,native_libraries_unchanged=all(Path(path).exists()and sha(path)==pin for path,pin in pins.items()),
        native_headers_unchanged=all(Path(path).exists()and sha(path)==pin for path,pin in headers.items()))
    report['passed']=result==0 and report['native_libraries_unchanged']and report['native_headers_unchanged'];save()
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'qualification.json'))),flush=True)
    return 0 if report['passed']else result or 1


if __name__=='__main__':raise SystemExit(main())
