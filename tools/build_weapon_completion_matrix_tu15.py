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
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    assert not a.output.exists() and file_sha(BASE/'authored-scope.json')==PIN
    scope=json.loads((BASE/'authored-scope.json').read_text());rows=[]
    for weapon in scope['weapons']:
        row=dict(name=weapon['name'],occurrences=weapon['occurrences'],feature_status='incomplete',visual_status='incomplete',
            gates={name:dict(status='unverified',applicability='requires original-owner audit',contract=contract) for name,contract in GATES.items()},bounded_evidence=[])
        if weapon['name']=='objects\\characters\\hunter\\hunter_particle_cannon\\hunter_particle_cannon':
            for report in ['hunter-charge-complete-evidence-v1.json','hunter-neutral-cleanup-evidence-v1.json','hunter-player-registration-evidence-v1.json','hunter-ai-frame-tracking-evidence-v1.json']:
                p=BASE/report;assert p.is_file()
                row['bounded_evidence'].append(dict(report=report,sha256=file_sha(p),map='assets/disc/maps/03a_oldmombasa.map',
                    weapon_datum='fad418a0',certifies_entire_gate=False,certifies_all_map_variants=False))
        rows.append(row)
    assert len(rows)==63 and sum(len(r['occurrences']) for r in rows)==1110
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),authored_scope_sha256=PIN,weapon_paths=63,authored_rows=1110,maps=38,
        completion_rule='Every applicable contract must have complete evidence; non-applicability also requires original-source evidence. Shared component tests alone cannot certify a weapon.',
        fully_certified_weapons=0,full_weapon_parity=False,physical_xbox_verified=False,weapons=rows)
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print('Published63 weapon rows and630 explicit completion gates; zero weapons certified')

if __name__=='__main__':main()
