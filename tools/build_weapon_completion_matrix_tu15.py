#!/usr/bin/env python3
"""Publish all63 weapon completion gates without certifying bounded subsystems."""
import argparse,json
from pathlib import Path
from audit_rendering import file_sha
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/weapons-tu15-goal-20261013'
PIN='6855b4f33585b332ca82e4b688b9e29c6a7a04eca5757c1bc8fe9b20640c6a1b'
GATES={
    'authored_contracts':'Every map variant, tag operands and TU1.5 balance changes',
    'inventory_lifetime':'Pickup, equip, switch, dual hands, drop, ownership and retirement',
    'trigger_resources':'Input, barrels, burst, reload/chamber, interruptions, battery, heat and charge',
    'aim_damage':'Aim/error/recoil, damage regions, headshots, shield/body admission and melee/lunge',
    'projectiles':'Creation, flight, homing/tracking, contacts, sticking, detonation and supercombine',
    'owning_game_rules':'Actual player, AI, vehicle/seat, turret or objective owner and scheduling',
    'poses_models':'Applicable species/hands, first/third-person models, regions, poses, blend, IK and camera',
    'materials_effects':'Textures/mips, shaders, lighting, charge/fire/impact/detonation particles and decals',
    'hud_audio_publication':'HUD/scopes, audio, split-screen and LAN event/hand histories',
    'full_reference_sequence':'Complete native gameplay sequence and substantiated OG Xbox TU1.5 visual/audio comparison'}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--balance-report',type=Path);a=ap.parse_args()
    assert not a.output.exists() and file_sha(BASE/'authored-scope.json')==PIN
    scope=json.loads((BASE/'authored-scope.json').read_text());rows=[]
    balance=None
    if a.balance_report:
        balance=json.loads(a.balance_report.read_text())
        assert balance['passed'] and balance['authored_scope_sha256']==PIN
        assert balance['maps']==38 and balance['weapon_paths']==63 and balance['weapon_occurrences']==1110
        assert len(balance['cases'])==38 and all(c['passed'] for c in balance['cases'])
        balance_maps={c['map']:c for c in balance['cases']}
        assert set(balance_maps)=={m['path'] for m in scope['maps']}
        for c in balance['cases']:
            original=a.balance_report.parent/c['original_report']
            assert file_sha(original)==c['original_report_sha256']
    for weapon in scope['weapons']:
        row=dict(name=weapon['name'],occurrences=weapon['occurrences'],feature_status='incomplete',visual_status='incomplete',
            gates={name:dict(status='unverified',applicability='requires original-owner audit',contract=contract) for name,contract in GATES.items()},bounded_evidence=[])
        if balance:
            row['bounded_evidence'].append(dict(report=str(a.balance_report.resolve().relative_to(ROOT)),
                sha256=file_sha(a.balance_report),component='Original165B30 complete combat-tag postprocessor versus native complete cache bytes',
                maps=[dict(map=o['map'],weapon_datum=o['datum'],
                    native_cache_consumer_checked=balance_maps[o['map']]['native_cache_weapon_damage_and_udp_identity_checked']) for o in weapon['occurrences']],
                component_covers_all_scoped_maps_for_weapon=True,certifies_entire_gate=False,certifies_all_map_variants=False))
            row['gates']['authored_contracts']['status']='partial'
            row['gates']['authored_contracts']['remaining']='Combat postprocessor admission and complete cache bytes match in every scoped map. Complete tag-operand imports, preceding activation and full weapon-specific behavior remain unverified.'
        if weapon['name']=='objects\\characters\\hunter\\hunter_particle_cannon\\hunter_particle_cannon':
            for report in ['hunter-charge-complete-evidence-v1.json','hunter-neutral-cleanup-evidence-v1.json','hunter-player-registration-evidence-v1.json','hunter-ai-frame-tracking-evidence-v1.json',
                'hunter-ai-discharge-180-original-v1/report.json','hunter-weapon-update-original-v1/report.json',
                'hunter-public-ai-first-api-evidence-v1.json','hunter-unit-query-native-v1/report.json',
                'public-hunter-frame631-promotion-v1.json','public-hunter-actor156-promotion-v1.json',
                'hunter-public-ai-180-original-v1/report.json','hunter-unit-action-api-evidence-v1.json',
                'hunter-hit-effect-api-evidence-v1.json','hunter-particle-api-original-v2/report.json',
                'hunter-particle-gpu-evidence-v1.json','checkpoint-coherent-runtime-v4-verified.json',
                'hunter-model-builder-evidence-v1.json','hunter-body-particle-gpu-evidence-v1.json',
                'public-hunter-frame632-promotion-v1.json','hunter-public-186-api-evidence-v2.json',
                'checkpoint-public-hunter-extended.json','object-material-promotion-v1.json',
                'hunter-object-material-api-evidence-v1.json','object-material-controls-v1/report.json',
                'public-hunter-frame633-promotion-v1.json','public-hunter-frame634-promotion-v1.json',
                'hunter-notification-api-evidence-v1.json','hunter-object-link-api-evidence-v1.json',
                'checkpoint-public-hunter-notifications.json','recorded-animation-promotion-v3.json',
                'recorded-query-controls-v1/report.json','hunter-recorded-constructor-api-evidence-v1.json',
                'hunter-recorded-map-reset-api-evidence-v1.json','hunter-recorded-query-api-evidence-v1.json',
                'public-hunter-frame644-promotion-v1.json','hunter-damage-notification-api-evidence-v2.json',
                'public-hunter-frame646-promotion-v1.json','hunter-post-damage-callback-api-evidence-v2.json',
                'public-hunter-frame669-promotion-v1.json','hunter-public-ai224-api-evidence-v1.json',
                'public-hunter-frame672-merged-promotion-v1.json','hunter-public-ai226-api-evidence-v1.json',
                'public-hunter-frame675-promotion-v1.json','hunter-animation-callback-api-evidence-v1.json',
                'hunter-postphase242-api-evidence-v1.json','public-hunter-frame678-promotion-v1.json',
                'hunter-unit-callback-api-evidence-v1.json','hunter-triggered-objects-api-evidence-v1.json']:
                p=BASE/report;assert p.is_file()
                row['bounded_evidence'].append(dict(report=report,sha256=file_sha(p),map='assets/disc/maps/03a_oldmombasa.map',
                    weapon_datum='fad418a0',certifies_entire_gate=False,certifies_all_map_variants=False))
            for gate in ('owning_game_rules','trigger_resources','projectiles','materials_effects','poses_models'):
                row['gates'][gate]['status']='partial'
                row['gates'][gate]['remaining']='The full contract and all map variants still require original-owner and runtime evidence.'
        rows.append(row)
    assert len(rows)==63 and sum(len(r['occurrences']) for r in rows)==1110
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),authored_scope_sha256=PIN,weapon_paths=63,authored_rows=1110,maps=38,
        completion_rule='Every applicable contract must have complete evidence; non-applicability also requires original-source evidence. Shared component tests alone cannot certify a weapon.',
        fully_certified_weapons=0,full_weapon_parity=False,physical_xbox_verified=False,weapons=rows)
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print('Published63 weapon rows and630 explicit completion gates; zero weapons certified')

if __name__=='__main__':main()
