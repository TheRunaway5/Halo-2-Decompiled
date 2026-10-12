#!/usr/bin/env python3
"""Verify and pin the captured pickup component and native delivery evidence."""
import datetime,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build-weapon-pickup-20261011'
SNAP=ROOT/'.local/weapon-pickup-source-20261011'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def artifact(path):return {'path':str(path),'sha256':sha(path)}
def main():
    manifest=SNAP/'source-manifest-final.json';m=json.loads(manifest.read_text())
    mismatch=[rel for rel,pin in m['files'].items()if sha(SNAP/rel)!=pin];assert not mismatch,mismatch
    generated=json.loads((SNAP/'tests/fixtures/unit_pickup_runtime_manifest.json').read_text())
    assert generated['generated_cpp_sha256']==sha(SNAP/'src/unit_pickup_runtime.cpp')
    assert len(generated['complete_native_entries'])==26 and len(generated['lowered_instructions'])==2144
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution'] and not generated['captured_runtime_results_used']
    fixture=SNAP/'tests/fixtures/unit_pickup_runtime_original.tsv';original=json.loads((SNAP/'tests/fixtures/unit_pickup_runtime_original.provenance.json').read_text())
    assert sha(fixture)==original['fixture_sha256']==sha(BUILD/'unit_pickup_original_repeat.tsv')
    assert len(original['cases'])==269 and sum(c['stopped_owner']is not None for c in original['cases'])==20
    assert sum(c['stores']for c in original['cases'])==10235 and len(original['tested_native_instruction_addresses'])==1407
    assert sum(len(c['external_calls'])for c in original['cases'])==182
    assert all(q['origin']=='original instruction execution'for c in original['cases']for q in c['external_calls'])
    versions=json.loads((BUILD/'player_inventory_versions.json').read_text())
    assert len(versions['images'])==3 and len({r['normalized_complete_body_sha256']for r in versions['images']})==1
    for row in versions['images']:assert row['instructions']==294 and sha(Path(row['path']))==row['sha256']
    red=json.loads((BUILD/'carry_admission_red_green.json').read_text());assert red['red_exit']==1
    partial=json.loads((BUILD/'partial_owner_abi_red_green.json').read_text());assert partial['red_exit']==1
    tests=[]
    for name,log,needle in (
        ('focused source and native regressions','weapon-pickup-final-focused-tests.log','100% tests passed out of 4'),
        ('ASAN UBSAN and leak checks','weapon-pickup-sanitizer-tests.log','249 returned original pickup calls and 20 exact required-owner stops'),
        ('previous live implementation regression control','weapon-pickup-red-test.log','Live player carry limit ignored original flag4 exemption'),
        ('previous callback interruption regression control','weapon-pickup-partial-abi-red.log','partial lower-owner ABI must survive failure'),
        ('actual SDL dual pickup fire reload and throw','weapon-pickup-final-dual-runtime.log','PASS actual SDL dual pickup, two independently firing/reloading weapon datums'),
        ('actual SDL four-view grenade throw','weapon-pickup-final-grenade-runtime.log','PASS authored grenade world geometry submitted independently to every local view'),
        ('shipping runtime with TU1.5 and four views','weapon-pickup-final-shipping-runtime.log','Multiplayer frames=10')):
        path=ROOT/'build'/log;text=path.read_text();assert needle in text,(name,text[-2000:])
        assert not re.search(r'Validation (?:Error|Warning)|VUID-|ERROR: AddressSanitizer|runtime error:',text),name
        tests.append({'name':name,'passed':True,'log':artifact(path)})
    assert 'split_views=4'in(ROOT/'build/weapon-pickup-final-shipping-runtime.log').read_text()
    assert 'views=4'in(ROOT/'build/weapon-pickup-final-grenade-runtime.log').read_text()
    assert '88 partial-owner ABI failure controls'in(ROOT/'build/weapon-pickup-sanitizer-tests.log').read_text()
    launchers=[]
    for name in ('Halo2-Launcher','Halo2-Title-Launcher'):
        path=BUILD/name;result=subprocess.check_output([str(path),'--launcher-info'],text=True)
        assert 'Runtime: '+str(BUILD/'halo2_native')in result,result
        out=BUILD/(name+'-info.txt');out.write_text(result);launchers.append({'launcher':artifact(path),'info':artifact(out)})
    # Other active chats publish the shared root launchers independently. Pin
    # their observed selection without claiming those binaries passed our tests
    # or overwriting their newer delivery.
    shared=[]
    for name in ('Halo2-Launcher','Halo2-Title-Launcher'):
        path=ROOT/name;result=subprocess.check_output([str(path),'--launcher-info'],text=True)
        selected=re.search(r'^Runtime: (.+)$',result,re.MULTILINE);assert selected,result
        selected_path=Path(selected.group(1));assert selected_path.is_file(),result
        out=BUILD/(name+'-shared-info.txt');out.write_text(result)
        shared.append({'launcher':artifact(path),'info':artifact(out),'selected_runtime':artifact(selected_path),'validated_by_this_report':False})
    report={'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'complete_original_gameplay_or_rendered_parity':False,'live_complete_CEC00':False,'live_original_transition_manager':False,'live_original_Havok':False,
        'source_snapshot':str(SNAP),'source_manifest':artifact(manifest),'captured_files':len(m['files']),'manifest_mismatches':mismatch,
        'validator':artifact(Path(__file__)),'generated_manifest':artifact(SNAP/'tests/fixtures/unit_pickup_runtime_manifest.json'),
        'original_fixture':artifact(fixture),'original_repeat':artifact(BUILD/'unit_pickup_original_repeat.tsv'),'player_restriction_versions':artifact(BUILD/'player_inventory_versions.json'),
        'cases':269,'returned_cases':249,'required_owner_stops':20,'ordered_native_owner_stores':10235,'tested_native_instructions':1407,
        'original_lower_differential_boundaries':182,'typed_player_restriction_cases':160,'missing_owner_rejections':88,'partial_owner_abi_failure_controls':88,
        'tests':tests,'runtime':artifact(BUILD/'halo2_native'),
        'binaries':[artifact(BUILD/name)for name in ('unit_pickup_runtime_tests','unit_pickup_runtime_sanitized','native_weapon_gameplay_tests','native_weapon_runtime_tests','multiplayer_grenade_runtime_tests')],
        'preserved_social_sdk':artifact(BUILD/'libdiscord_partner_sdk.so'),'social_sdk_notices':artifact(BUILD/'Discord-Social-SDK-Notices.txt'),
        'build_configuration':artifact(BUILD/'CMakeCache.txt'),'launchers':launchers,'shared_launchers_observed_without_replacement':shared,
        'remaining':['Complete live animation manager/extension/transition/saved-pose ownership','Third-person fire/reload and secondary blend composition','Full live capability/admission/action/inventory/drop routing','Source bounds and physical world/Havok producers','Grenade particle/angular/world-contact parity','Non-SMG charge/heat/lunge state','Matched original rendered sequences and physical Xbox/platform proof']}
    path=BUILD/'weapon-pickup-validation.json';path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'tests':len(tests),'cases':269,'manifest_mismatches':0,'validation_sha256':sha(path),'complete_original_gameplay_or_rendered_parity':False}))
if __name__=='__main__':main()
