#!/usr/bin/env python3
"""Admit only recovered secondary-intent production and the host input mapping."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins;changes={}
    def save(relative,data):
        path=source/relative;before=path.read_bytes() if path.exists() else None
        if before!=data:
            path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
            changes[relative]=dict(before=sha(before) if before else None,after=sha(data))
    for relative in ('include/halo2/source_secondary_trigger_intent_tu15.hpp',
            'include/halo2/multiplayer_kbm.hpp','src/input/multiplayer_kbm.cpp',
            'tests/multiplayer_kbm_tests.cpp','tests/secondary_trigger_intent_tu15_tests.cpp'):
        save(relative,(ROOT/relative).read_bytes())
    relative='src/gameplay/multiplayer/multiplayer.cpp';text=(source/relative).read_text();root=(ROOT/relative).read_text()
    if '#include "halo2/source_secondary_trigger_intent_tu15.hpp"' not in text:
        anchor='#include "halo2/player_command.hpp"';assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n#include "halo2/source_secondary_trigger_intent_tu15.hpp"')
    if 'put(0,source_secondary_trigger_intent_tu15' not in text:
        a=text.index('      put(0,unsigned(input.crouch)');b=text.index('      put(4,',a)
        c=root.index('      // Actual189617..18963D');d=root.index('      put(4,',c)
        text=text[:a]+root[c:d]+text[b:]
    save(relative,text.encode())
    relative='src/gameplay/multiplayer/multiplayer_runtime.cpp';text=(source/relative).read_text()
    if '  const auto vehicle_secondary_fire=' in text:
        a=text.index('  const auto vehicle_secondary_fire=');b=text.index('  const auto deliver_source_weapon_audio=',a)
        text=text[:a]+text[b:]
    old='        pad_input.fire_secondary=(dual || vehicle_secondary_fire(i))&&mapped.secondary;'
    if old in text:text=text.replace(old,'        const bool seated=i<ids.size() && current.players[ids[i]].source_parent!=null_datum;\n        pad_input.fire_secondary=(dual || seated)&&mapped.secondary;')
    if '        if(armed_vehicle)pad_input.crouch|=mapped.secondary;' not in text:
        anchor='        pad_input.grenade=!dual&&mapped.grenade;';assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'\n        if(armed_vehicle)pad_input.crouch|=mapped.secondary;')
    text=text.replace('input.gameplay_controls(dual,seated,inverted,vehicle_secondary_fire(keyboard_view))',
        'input.gameplay_controls(dual,seated,inverted)')
    assert 'vehicle_secondary_fire' not in text;save(relative,text.encode())
    relative='CMakeLists.txt';text=(source/relative).read_text();root=(ROOT/relative).read_text()
    a=root.index('    add_executable(secondary_trigger_intent_tu15_tests ');b=root.index('    add_executable(scenery_audio_angle_tu15_tests ',a)
    if '    add_executable(secondary_trigger_intent_tu15_tests ' not in text:
        anchor='    add_executable(scenery_audio_angle_tu15_tests ';assert text.count(anchor)==1
        save(relative,text.replace(anchor,root[a:b]+anchor).encode())
    updated=source_hashes(source);assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';admission=json.loads(path.read_text());admission.update(build_completed=False,invalidated_by=__doc__);path.write_text(json.dumps(admission,indent=2)+'\n')
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/secondary-intent-native';attempt=2
    while out.exists():out=out.with_name('secondary-intent-native-'+str(attempt));attempt+=1
    out.mkdir()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','secondary_trigger_intent_tu15_tests','--target','multiplayer_kbm_tests',
        '--filter','^(original_secondary_trigger_intent_tu15|multiplayer_kbm_bindings)$',
        '--jobs','3','--test-jobs','1','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    receipt=dict(scope=__doc__,changes=changes,command=command,exit_code=code,
        production_unchanged=source_hashes(source)==updated,source_boost_result_supplied=False,gameplay_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
