#!/usr/bin/env python3
"""Observe actual authored scenery effect allocations and their native producer."""
import json,os,shutil,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';build=owner/'build'
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-client-lens';out=base/'map-spark';n=2
    while out.exists():out=base/('map-spark-'+str(n));n+=1
    out.mkdir();pins=source_hashes(source)
    helper=source/'tests/source_map_effect_lens_tu15_tests.cpp';shutil.copyfile(ROOT/'tests'/helper.name,helper)
    map_=ROOT/'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11/elongation.map'
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    with cmake.open('a') as f:f.write('\nadd_executable(map_effect_lens_audit EXCLUDE_FROM_ALL tests/source_map_effect_lens_tu15_tests.cpp)\ntarget_link_libraries(map_effect_lens_audit PRIVATE halo2_systems)\nadd_test(NAME map_effect_lens_audit COMMAND map_effect_lens_audit "'+str(map_)+'" "${CMAKE_BINARY_DIR}/map-effect-lens.json")\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','map_effect_lens_audit','--filter','^map_effect_lens_audit$','--jobs','2','--test-jobs','1','--output',str(out/'cmake')]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    report=build/'map-effect-lens.json'
    if report.exists():shutil.copyfile(report,out/'actual.json');report.unlink()
    assert source_hashes(source)==pins
    (out/'receipt.json').write_text(json.dumps(dict(scope=__doc__,command=command,exit_code=code,source_unchanged=True,gameplay_or_pixels_verified=False),indent=2)+'\n')
    print(json.dumps(dict(report=str(out/'receipt.json'),exit_code=code)),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
