#!/usr/bin/env python3
"""Inventory actual canonical effect gaps in the active owned source cohort."""
import hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
from mcc_content import directory
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';build=owner/'build'
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-client-lens';out=base/'metadata-inventory';n=2
    while out.exists():out=base/('metadata-inventory-'+str(n));n+=1
    out.mkdir();pins=source_hashes(source)
    selected=json.loads((ROOT/'gameplay-runtime-selection.json').read_text());maps=directory(ROOT/selected['cache'])/'h2_maps_win64_dx11'
    cases={name:maps/(name+'.map') for name in ['coagulation','lockout','elongation']}
    cases['original-coagulation']=ROOT/'assets/disc/maps/coagulation.map'
    cases={name:path for name,path in cases.items() if path.is_file()};assert 'coagulation' in cases and 'original-coagulation' in cases
    map_pins={name:sha(path) for name,path in cases.items()}
    helper=source/'tools/audit_effect_lens_inventory_20261024.cpp';shutil.copyfile(ROOT/'tools'/helper.name,helper)
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    with cmake.open('a') as f:
        f.write('\nadd_executable(effect_lens_inventory EXCLUDE_FROM_ALL tools/audit_effect_lens_inventory_20261024.cpp)\ntarget_link_libraries(effect_lens_inventory PRIVATE halo2_cache)\n')
        for name,path in cases.items():f.write(f'add_test(NAME effect_lens_inventory_{name} COMMAND effect_lens_inventory "{path}" "${{CMAKE_BINARY_DIR}}/effect-inventory-{name}.json")\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','effect_lens_inventory','--filter','^effect_lens_inventory_','--jobs','2','--test-jobs','1','--output',str(out/'cmake')]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    reports=[]
    for name,path in cases.items():
        p=build/('effect-inventory-'+name+'.json')
        if p.exists():
            assert p.stat().st_size<1024*1024;shutil.copyfile(p,out/p.name);p.unlink();reports.append(p.name)
    assert source_hashes(source)==pins and all(sha(cases[k])==v for k,v in map_pins.items())
    receipt=dict(scope=__doc__,command=command,exit_code=code,maps={name:dict(path=str(path),sha256=map_pins[name]) for name,path in cases.items()},reports=reports,source_unchanged=True,map_files_unchanged=True,actual_gameplay_or_parity_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
