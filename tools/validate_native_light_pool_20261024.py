#!/usr/bin/env python3
"""Qualify shared snapshot extraction and original light collection/history in the owned worker."""
import json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    pins=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023/source-hashes.json').read_text());assert source_hashes(source)==pins
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-native-attached-light-collection';out=base/'compatibility';n=2
    while out.exists():out=base/f'compatibility-{n}';n+=1
    out.mkdir()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source)]
    for target in ['pstencil_attached_lights_tests','effect_light_view_tests','effect_light_admission_tests','mcc_fp_sphere_material_tests']:command+=['--target',target]
    command+=['--filter','^(pstencil_native_attached_light|original_effect_light_view_history|original_effect_light_render_admission|original_effect_light_candidates|original_effect_light_visibility_visit|original_effect_light_visibility_collection|original_tu15_mcc_fp_sphere_materials)$',
        '--cmake-arg=-DHALO2_MCC_FP_LIGHT_MATERIAL_MAP='+str(ROOT/'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11/coagulation.map'),
        '--jobs','2','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    assert source_hashes(source)==pins
    (out/'receipt.json').write_text(json.dumps(dict(scope=__doc__,command=command,exit_code=code,production_unchanged=True,launcher_gameplay_verified=False),indent=2)+'\n')
    print(json.dumps(dict(output=str(out),exit_code=code)),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
