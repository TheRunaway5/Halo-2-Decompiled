#!/usr/bin/env python3
"""Run real effect encounters in the current owned diagnostic build.

This records reproduction evidence, not a blanket effects-parity verdict.
Use through tools/run_test.py or its finite owned command coordinator.
"""
import argparse,hashlib,json,os,re,struct,subprocess,tempfile
from pathlib import Path
from build_launcher_engine import source_hashes
from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import keep_log

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',choices=('plasma-pistol','plasma-light-control','joined-frag','joined-banshee','joined-rocket','joined-rocket-airburst','joined-needler','joined-spark'),required=True)
    ap.add_argument('--omit-function-symbols',action='store_true',help='Private prototype omission control only')
    ap.add_argument('--audio',action='store_true',help='Run the actual native Audio device/output path')
    ap.add_argument('--omit-light-lenses',action='store_true',help='Private diagnostic clone omits only standalone light lens submissions')
    args=ap.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    followup=BASE/'repair-followup-20261023';pins=json.loads((followup/'source-hashes.json').read_text())
    assert source_hashes(source)==pins,'Diagnostic build has unadmitted production changes'
    admission=json.loads((followup/'diagnostic-admission.json').read_text())
    if args.omit_light_lenses:assert admission.get('standalone_lens_omission_control'),'Current diagnostic binary lacks the requested omission control'
    plasma=args.case in ('plasma-pistol','plasma-light-control')
    fixture='audit_combat_presentation_20261022.cpp' if plasma else 'audit_joined_effects_20261022.cpp'
    key='combat_fixture_sha256' if plasma else 'joined_fixture_sha256'
    assert sha(source/'tools'/fixture)==admission[key]
    out=followup/'gameplay'/args.case;attempt=2
    while out.exists():out=followup/'gameplay'/(args.case+'-'+str(attempt));attempt+=1
    out.mkdir(parents=True)
    scratch=Path(tempfile.mkdtemp(prefix=args.case+'-',dir=owner))
    map_=ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11/coagulation.map'
    if args.case=='joined-spark':map_=ROOT/'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11/elongation.map'
    report=out/'actual.json';capture=scratch/'frame.ppm';log=scratch/'runtime.log'
    if plasma:
        exe=owner/'build/combat_presentation_audit';cmd=[str(exe),'fire',str(map_),str(report),'plasma_pistol','0']
    else:
        exe=owner/'build/joined_effects_audit';cmd=[str(exe),str(map_),str(report),str(capture)]
        if args.case=='joined-banshee':cmd.append(str(BASE/'repair-attachment-material/banshee-setup.json'))
    pin=sha(exe)
    assert admission.get('build_completed') and admission['executables'][exe.name]==pin,'Current diagnostic build has not passed'
    deadline=int(os.environ.get('HALO2_COMBAT_EFFECTS_DEADLINE','600'))
    with private_display(scratch) as display,log.open('w') as stream:
        env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'),HALO2_COMBAT_AUDIT_CAPTURE=str(capture))
        if args.case=='plasma-light-control':env['HALO2_AUDIT_EXPLICIT_LIGHT_UPDATE']='1'
        if args.case.startswith('joined-rocket'):env['HALO2_JOINED_ROCKET']='1'
        if args.case=='joined-rocket-airburst':env['HALO2_JOINED_ROCKET_AIRBURST']='1'
        if args.case=='joined-needler':env['HALO2_JOINED_NEEDLER']='1'
        if args.case=='joined-spark':env['HALO2_JOINED_MAP_SPARK']='1'
        if args.audio:env['HALO2_AUDIT_ENABLE_AUDIO']='1'
        if args.omit_light_lenses:env['HALO2_AUDIT_OMIT_STANDALONE_LENS']='1'
        if args.omit_function_symbols:
            assert 'Private compiled-symbol' in admission['source_snapshot_role']['role']
            env['HALO2_AUDIT_DISABLE_FUNCTION_SYMBOLS']='1'
        try:code=subprocess.run(cmd,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=deadline).returncode
        except subprocess.TimeoutExpired:code=124
    raw=log.read_text(errors='replace');keep_log(log,out/'runtime.log')
    actual=json.loads(report.read_text()) if report.exists() else {}
    authority_failure=Path(str(report)+'.authority-failure.json')
    samples=[tuple(map(int,row)) for row in re.findall(r'\[COMBAT-JOIN-LENS\] tick=(\d+) view=(\d+) queries=(\d+) flares=(\d+)',raw)]
    completions=[tuple(map(int,row)) for row in re.findall(r'\[COMBAT-REPLICA-GPU\] id=(\d+) generation=(\d+) samples=(\d+)',raw)]
    result=dict(case=args.case,exit_code=code,command=cmd,timeout_seconds=deadline,executable_sha256=pin,executable_unchanged=sha(exe)==pin,
                private_compiled_symbol_omission_control=args.omit_function_symbols,
                private_standalone_lens_omission_control=args.omit_light_lenses,
                source_snapshot_role=admission.get('source_snapshot_role'),
                production_source_unchanged=source_hashes(source)==pins,
                scope='Actual SDL/UDP and source gameplay with declared fixture setup; completed lens spans are observed, never supplied. Original Xbox framebuffer and speakers are not compared.')
    if authority_failure.exists():result['authority_failure']=json.loads(authority_failure.read_text())
    result['actual_GPU_completions']=len(completions);result['positive_actual_GPU_completions']=sum(row[2]>0 for row in completions)
    if plasma:
        held=[bytes.fromhex(row['actual_record_hex']) for row in actual.get('actual_source_first_person_light_census',[]) if 'actual_record_hex' in row]
        result.update(encounter_completed=code==0 and bool(actual.get('live_projectiles')),
            actual_source_projectiles=len(actual.get('live_projectiles',[])),
            builtin_controls=actual.get('actual_original_builtin_controls',[]),
            held_light_samples=len(held),active_held_light_samples=sum(bool(raw[2]&2) for raw in held),
            held_light_positive_diffuse_samples=sum(sum(v*v for v in struct.unpack_from('<3f',raw,0xd4))>.05 for raw in held),
            raw_first_person_light_packets=len(actual.get('actual_raw_first_person_light_packets',[])),
            original_first_person_light_census=actual.get('actual_source_first_person_light_census',[]),
            native_first_person_light_records=actual.get('actual_native_first_person_light_records',[]))
        result['lighting_gap_reproduced']=False
        result['lighting_boundary']='A live original first-person record and its actual expected receiver/implementation must be qualified before declaring a missing pass.'
        result['actual_native_audio_packets']=len(actual.get('audio_packets',[]))
        if args.audio:
            result['audio_path_verified']=result['actual_native_audio_packets']>0 and not re.search(r'^(Audio:|Source weapon audio:)',raw,re.M)
            result['encounter_completed'] &= result['audio_path_verified']
    else:
        result.update(encounter_completed=code==0 and actual.get('actual_encounter_completed',False),
            actual_view_samples=len(samples),query_draws=sum(s[2] for s in samples),flare_draws=sum(s[3] for s in samples),
            actual_top_glows=actual.get('top_actual_projectile_glow_draws',0),
            authored_projectile_lenses=actual.get('authority',{}).get('projectile_lenses',[]))
        result['missing_lens_reproduced']=bool(args.case=='joined-banshee' and result['encounter_completed'] and result['actual_view_samples'] and result['authored_projectile_lenses'] and result['query_draws']==0 and result['flare_draws']==0)
        result['local_positive_reference']='repair-paired-1/airborne-trace.log: genuine local Banshee queries and flares; same prepared Coagulation attachment definition'
        if args.case=='joined-frag':result['boundary']='Authored grenade light lens references alone do not prove expected original requests; this is an encounter/transport diagnostic.'
        if args.case.startswith('joined-rocket') or args.case=='joined-needler':
            result['actual_received_detonations']=actual.get('received_detonations',[])
            result['missing_lens_reproduced']=bool(result['encounter_completed'] and result['query_draws']==0)
            result['remote_render_provenance_preserved']=bool(result['actual_received_detonations']) and all(
                row['local_pool_identity']==0xffffffff and row['replica_render_present']
                for row in result['actual_received_detonations'])
            result['replica_lens_repair_verified']=bool(result['encounter_completed'] and
                result['remote_render_provenance_preserved'] and result['query_draws']>0 and result['flare_draws']>0)
        if args.case=='joined-spark':
            result['actual_spark_snapshots']=actual.get('actual_spark_snapshots',0)
            result['actual_SDL_spark_mouse_events']=actual.get('actual_SDL_spark_mouse_events',0)
            result['replica_lens_repair_verified']=bool(result['encounter_completed'] and result['query_draws']>0 and result['flare_draws']>0)
    (out/'encounter.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if not isinstance(v,list)}),flush=True)
    return 0 if result['encounter_completed'] and result['executable_unchanged'] and result['production_source_unchanged'] else 1

if __name__=='__main__':raise SystemExit(main())
