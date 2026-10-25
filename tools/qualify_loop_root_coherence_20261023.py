#!/usr/bin/env python3
"""Isolate the existing sound-pipeline count failure from lsnd root projection."""
import hashlib,json,os,re,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    pins=source_hashes(source);base=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
    out=base/'loop-root-coherence-control';attempt=2
    while out.exists():out=base/('loop-root-coherence-control-'+str(attempt));attempt+=1
    out.mkdir()
    text=(source/'src/runtime/mcc_runtime_delivery.cpp').read_text()
    a=text.index('    // Original object/audio callers read lsnd+0 flags,');b=text.index('    // The source binder follows matg+C0/C4',a)
    omitted=text[:a]+text[b:]
    (source/'tools/loop-coherence-omitted.cpp').write_text(omitted)
    test=(source/'tests/mcc_sound_pipeline_coherence_tests.cpp').read_text()
    anchor='check(pproof.corrections().size()==pc.at(cache.name()),"Qualifiedpermutation lost mandatory corrections");'
    assert test.count(anchor)==1
    diagnostic='''const auto serialized=final_json.dump();std::cout<<"LOOP_COHERENCE_DIAGNOSTIC "<<J({{"map",cache.name()},{"corrections",pproof.corrections().size()},{"expected",pc.at(cache.name())},{"native_sound_plan_sha256",hex(sha(std::as_bytes(std::span(serialized))))}}).dump()<<std::endl;'''
    (source/'tools/loop-coherence-test.cpp').write_text(test.replace(anchor,diagnostic+anchor))
    maps=ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11'
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    cmake.write_bytes(saved+('''
add_executable(loop_coherence_current EXCLUDE_FROM_ALL tools/loop-coherence-test.cpp)
add_executable(loop_coherence_omitted EXCLUDE_FROM_ALL tools/loop-coherence-test.cpp tools/loop-coherence-omitted.cpp)
foreach(t loop_coherence_current loop_coherence_omitted)
  target_link_libraries(${t} PRIVATE halo2_cache OpenSSL::Crypto nlohmann_json::nlohmann_json)
  add_test(NAME ${t} COMMAND ${t} "'''+str(maps/'lockout.map')+'''")
endforeach()
add_custom_target(loop_coherence_control DEPENDS loop_coherence_current loop_coherence_omitted)
''').encode())
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','loop_coherence_control','--filter','^loop_coherence_(current|omitted)$',
        '--jobs','4','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    log=(out/'tests.log').read_text() if (out/'tests.log').exists() else ''
    controls=[json.loads(x) for x in re.findall(r'LOOP_COHERENCE_DIAGNOSTIC (\{[^\n]+\})',log)]
    isolated=code==8 and len(controls)==2 and controls[0]==controls[1] and log.count('Qualifiedpermutation lost mandatory corrections')==2
    receipt=dict(scope=__doc__,command=command,ctest_exit_code=code,controls=controls,
        same_count_failure_and_complete_native_sound_plan=isolated,
        full_sound_pipeline_coherence_passed=False,production_unchanged=source_hashes(source)==pins)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return 0 if isolated and receipt['production_unchanged'] else 1
if __name__=='__main__':raise SystemExit(main())
