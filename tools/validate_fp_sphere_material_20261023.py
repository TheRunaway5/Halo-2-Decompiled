#!/usr/bin/env python3
"""Refresh only the declared sphere-material regression/fix in this owned build."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=BASE/'repair-followup-20261023';pins=json.loads((follow/'source-hashes.json').read_text())
    assert source_hashes(source)==pins
    changes={};cmake=source/'CMakeLists.txt';text=cmake.read_text()
    root=(ROOT/'CMakeLists.txt').read_text()
    a=root.index('    add_executable(mcc_fp_sphere_material_tests ')
    b=root.index('    target_link_libraries(source_bsp_effect_lights_tests',a)
    block=root[a:b]
    if block not in text:
        old='    add_executable(source_bsp_effect_lights_tests tests/source_bsp_effect_lights_tests.cpp)\n'
        assert text.count(old)==1;text=text.replace(old,old+block);cmake.write_text(text)
    for name in ['tests/mcc_fp_sphere_material_tests.cpp']+(['src/rendering/world/world_material.cpp'] if '--fix' in sys.argv else []):
        p=source/name;data=(ROOT/name).read_bytes()
        if p.exists() and p.read_bytes()==data:continue
        before=hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None
        if p.exists():p.unlink()
        p.write_bytes(data);changes[name]=dict(before=before,after=hashlib.sha256(data).hexdigest())
    updated=source_hashes(source)
    assert {p for p in updated if updated[p]!=pins.get(p)}<=({'CMakeLists.txt','src/rendering/world/world_material.cpp'})
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    admission=follow/'diagnostic-admission.json';value=json.loads(admission.read_text())
    value.update(build_completed=False,invalidated_by=__doc__);admission.write_text(json.dumps(value,indent=2)+'\n')
    base=BASE/'repair-native-attached-light-collection'/('sphere-material-fixed' if '--fix' in sys.argv else 'sphere-material-red')
    out=base;attempt=2
    while (out/'receipt.json').exists():out=base.with_name(base.name+'-'+str(attempt));attempt+=1
    cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','mcc_fp_sphere_material_tests','--filter','^original_tu15_mcc_fp_sphere_materials$',
        '--cmake-arg=-DHALO2_MCC_FP_LIGHT_MATERIAL_MAP='+str(ROOT/'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11/coagulation.map'),
        '--jobs','2','--test-jobs','1','--output',str(out)]
    out.mkdir(parents=True,exist_ok=True)
    with (out/'command.log').open('w') as log:code=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT).returncode
    receipt=dict(scope=__doc__,changes=changes,command=cmd,exit_code=code,
        production_snapshot_unchanged=source_hashes(source)==updated,
        launcher_build_or_gameplay_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
