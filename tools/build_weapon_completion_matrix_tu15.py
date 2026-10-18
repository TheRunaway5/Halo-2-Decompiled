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
                'hunter-unit-callback-api-evidence-v1.json','hunter-triggered-objects-api-evidence-v1.json',
                'hunter-ai-weapon-ranges-v2.json','hunter-ranged-game-ready-audit-v1.json',
                'public-hunter-frame681-promotion-v1.json','hunter-ranged-builtin-api-evidence-v2.json',
                'level-handles-constructor-promotion-v1.json','hunter-level-handles-actual-api-evidence-v1.json',
                'hunter-game-start-original-v2/report.json','level-handles-new-map-promotion-v1.json',
                'hunter-level-handles-new-map-api-evidence-v1.json',
                'hunter-resource-cache-api-evidence-v1.json','resource-cache-root-body-correspondence-v2.json',
                'hunter-map-row24-api-evidence-v1.json',
                'public-hunter-frame685-promotion-v1.json','hunter-ranged-script-target-api-evidence-v1.json',
                'hunter-shader-function-startup-api-evidence-v1.json','hunter-shader-function-map-api-evidence-v1.json',
                'hunter-renderer-map-api-evidence-v1.json','hunter-row36-constructor-api-evidence-v1.json',
                'hunter-row43-constructor-api-evidence-v1.json','hunter-row36-map-api-evidence-v1.json',
                'next-audio-context-startup-audit-v1.json','audio-startup-discovery-evidence-v1.json',
                'boot-smc-input-root-test-v1/result/report.json',
                'audio-startup-discovery-evidence-v2.json','audio-startup-discovery-evidence-v3.json',
                'audio-startup-discovery-evidence-v4.json',
                'audio-prefix-native-composition-evidence-v1.json']:
                p=BASE/report;assert p.is_file()
                row['bounded_evidence'].append(dict(report=report,sha256=file_sha(p),map='assets/disc/maps/03a_oldmombasa.map',
                    weapon_datum='fad418a0',certifies_entire_gate=False,certifies_all_map_variants=False))
            for gate in ('owning_game_rules','trigger_resources','projectiles','materials_effects','poses_models'):
                row['gates'][gate]['status']='partial'
                row['gates'][gate]['remaining']='The full contract and all map variants still require original-owner and runtime evidence.'
            row['gates']['owning_game_rules']['remaining']='Cold original191DF0 startup is exercised before object birth, with complete row13/16/24/29/31/36 callback comparisons and original row29/36/43 constructor comparisons. The latest cold fixture reaches row44. The private224EC0 prefix through APU reset and16-descriptor storage matches91,577 original instruction labels,88,421 ordered non-stack writes and60,327,674 complete retained bytes;103 independent host i386 intervals pass under declared native resource ABI. It reaches the unowned38E8C9 configuration. Physical MCPX/FECTL/interrupt/frontend/DSP/PCM ownership, source device initialization, full initializer completion, BSP admission, game-ready publication, natural ranged firing and all scoped map variants remain unverified; the earlier ranged fixture retains game-ready zero.'
        if weapon['name']=='objects\\weapons\\pistol\\needler\\needler':
            for report in ('needler-source-flow-evidence-v1.json','damage-feedback-promotion-v1.json',
                           'needler-feedback-actual-api-evidence-v2.json','damage-feedback-promotion-v2.json',
                           'needler-reaction-actual-api-evidence-v1.json','needler-source-flow-evidence-v2.json',
                           'needler-contact-effect-trace-evidence-v1.json',
                           'needler-contact-effect-actual-api-evidence-v2.json',
                           'needler-contact-reference-provenance-v1.json',
                           'needler-visual-input-evidence-v1.json','needler-visual-input-evidence-v2.json',
                           'needler-local-particle-actual-api-evidence-v1.json',
                           'needler-particle-resource-admission-evidence-v1.json',
                           'needler-live-particle-evidence-v2.json',
                           'needler-particle-lineage-evidence-v2.json',
                           'camera-packet-limits-promotion-v2.json',
                           'needler-completed-draws-evidence-v2.json',
                           'needler-world-queue-api-evidence-v1.json',
                           'live-particle-resource-admission-promotion-v1.json',
                           'needler-native-graphics-evidence-v1.json',
                           'needler-native-particle-sequence-evidence-v1.json',
                           'needler-projectile-native-resource-evidence-v1.json',
                           'needler-native-animation-resource-controls-v1.json',
                           'needler-projectile-actual-api-evidence-v1.json',
                           'needler-super-detonation-actual-api-evidence-v1.json',
                           'needler-standard-detonation-actual-api-evidence-v2.json',
                           'needler-original-reference-evidence-v1.json',
                           'needler-original-live-cadence-v1/report.json',
                           'needler-original-graphics-evidence-v1.json',
                           'needler-original-draw-owner-evidence-v2.json',
                           'needler-weapon-resource-bounds-control-v1.json'):
                p=BASE/report;assert p.is_file()
                row['bounded_evidence'].append(dict(report=report,sha256=file_sha(p),
                    map='assets/disc/maps/lockout.map',certifies_entire_gate=False,certifies_all_map_variants=False))
            for gate in ('trigger_resources','aim_damage','projectiles','owning_game_rules','materials_effects','poses_models'):
                row['gates'][gate]['status']='partial'
            row['gates']['trigger_resources']['remaining']='Actual source player controls consume11 rounds and create11 projectiles in the tested held-fire interval. A separate normally played original TU1.5 Lockout match uses the actual menu Needler starting choice. Original live software consumes two rounds, creates two authored needles and retains60 reserve rounds. Hardware watchpoints observe the first two ammunition decrements at430/434; native source flow80/84 matches the four-tick interval. This establishes one interval, not a complete cadence or input-cycle comparison. Complete reload/release/dual/interruption and all variants remain unverified.'
            row['gates']['projectiles']['remaining']='The native two-player240-tick source flow attaches11 needles, kills the opponent at165 and retires all11 projectiles without supplied contact or damage results. All36 authored Needler projectile contact-effect references are null; actual178E30 agrees with34 original instructions and no writes. Actual first-damage FDEB0 matches58124 selected callbacks,1045 source writes,59183190 retained non-stack bytes and independent host CPU/FP except Linux IF under native animation I/O. The actual naturally retiring projectile at165 creates the authored super-detonation, depletes target health from1 to-6.6222229 and publishes its body flag100; its full pool identity remains until subsequent source cleanup and is absent by end-of-tick. Independent host i386 matches the complete native CPU/FP,59183190 non-stack bytes,365032 architectural instruction observations and13094 ordered source records. Four actual animation/geometry/texture reads deliver1310720 authenticated file bytes; source read/synchronization ownership agrees. REP callbacks, split vector-store observations and the emulator alternate CRT FP path are reported separately. Actual standard-detonation FDEB0 at175 creates the authored standard effect and damages the existing corpse body from-6.6222229 to-6.6533337. Independent host CPU/FP except Linux IF,59183190 non-stack bytes,47217 architectural observations and1338 ordered source stores match; no new resource reads occur. Its full projectile identity remains at call exit and is absent from the end-of-tick live list. The end-of-tick cached player Unit health is not a second raw corpse-body capture. Untaken guidance/contact/fuse branches, original asynchronous scheduling and console locale discovery, complete current-root runtime, matched Xbox visual/audio and every scoped variant remain unverified.'
            row['gates']['materials_effects']['remaining']='Actual source cameras and native authored portal ownership retain live Needler particles and completed world/first-person draws. Live resource admission restores world detonation draws against zero from the same firing-only queues. Actual1770C0 matches4858 original instruction labels,96 ordered writes, independent host CPU/FP and59183190 retained bytes. Every native draw across the latest240-tick flow retains its real live material owner:1990 draws,9 authored particle definitions and231 first-person firing draws. Current particle renderer and current GLSL execute all810 isolated view/pass samples, with3980 complete descriptor-backed input checks,65 world and44 first-person positive samples,691 exactly empty samples and exact repeated RGBA readbacks; validation has zero errors/warnings. Ten populated late world samples produce no changed pixels; their original expected visibility is unverified. A separate live original TU1.5 graphics capture is bounded by actual405080 indexed submissions matching the complete authored1058-index Needler sequence. Eleven original draws identify five first-person Needler/Chief-hand parts through every ordered index and57 complete consumed attribute observations matching original resource bytes. The authored190-index Needler core part is not observed in this capture. This supplies original geometry inputs, not active shader selection or a matched original/native frame. Complete original NV2A arithmetic/rasterization, full scene/model/ordering/glow/contrail/postprocess composition, matched Xbox TU1.5 visual/audio sequences and every scoped variant remain unverified; independent auxiliary visual geometry variation remains unresolved.'
            row['gates']['poses_models']['remaining']='Both living native player cameras now retain the authored70-degree unzoomed FOV after the two complete CRT packet-limit initializers run. Original/host/native constructor comparisons, exact regeneration,19 prior routine arithmetic/calls and current World syntax pass. Full poses/models/regions/species/hands, scope/interruption/death/vehicle camera modes, a complete current-root composed runtime and rendered comparisons remain unverified.'
            row['gates']['aim_damage']['remaining']='Actual1F5620 matches19 original instructions/two writes; actual E3E20 matches11,493 instructions/1,088 writes, host i386 CPU/FP and58,986,582 retained bytes. Both complete bodies retain191 static instructions. Untaken branches and the full damage contract remain unverified.'
            row['gates']['owning_game_rules']['remaining']='Two actual source players complete240 ticks using declared native spawn/aim/fire inputs with no forced target, contact or damage result and no parallel native firing. Complete cold startup, owner lifetimes and every scoped variant remain unverified.'
        rows.append(row)
    assert len(rows)==63 and sum(len(r['occurrences']) for r in rows)==1110
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),authored_scope_sha256=PIN,weapon_paths=63,authored_rows=1110,maps=38,
        completion_rule='Every applicable contract must have complete evidence; non-applicability also requires original-source evidence. Shared component tests alone cannot certify a weapon.',
        fully_certified_weapons=0,full_weapon_parity=False,physical_xbox_verified=False,weapons=rows)
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print('Published63 weapon rows and630 explicit completion gates; zero weapons certified')

if __name__=='__main__':main()
