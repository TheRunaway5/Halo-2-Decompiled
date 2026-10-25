#!/usr/bin/env python3
"""Collate exact installed gameplay evidence and prune redundant reviewed captures."""
import hashlib,json,os,re
from pathlib import Path
from build_launcher_engine import atomic_json,source_hashes
from gameplay_runtime_selection import selected_gameplay_runtime
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def load(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    selected=selected_gameplay_runtime(ROOT);runtime=ROOT/selected['runtime'];pin=selected['runtime_sha256']
    assert runtime.parent.name=='8cfb6e3ddda845bc876f4150741bb646'
    pins=load(runtime.parent/'source-hashes.json');assert source_hashes(owner/'source')==pins
    suite_path=BASE/'repair-installed-requalification/8cfb6e3d-full/report.json';suite=load(suite_path)
    assert suite['passed'] and suite['visual_acceptance_complete'] and not suite['visual_reviews_pending']
    assert len(suite['cases'])==7 and all(r['execution_passed'] for r in suite['cases'])
    paths=[Path(r['report']) for r in suite['cases']]
    paths += [BASE/'repair-installed-teleporter/8cfb6e3d-native-pad/report.json',
        BASE/'repair-installed-fp-light/8cfb6e3d-muzzle/report.json',
        BASE/'repair-installed-joined-lens/8cfb6e3d-needler/report.json',
        BASE/'repair-installed-joined-lens/8cfb6e3d-spark-qualified-2/report.json']
    reports={str(p.relative_to(BASE)):load(p) for p in paths}
    assert all(j['runtime_sha256']==pin for j in reports.values())
    assert all(load(p.parent/'visual-review.json')['passed'] for p in paths[-4:])
    banshee=load(BASE/'repair-installed-banshee/8cfb6e3d-full/report.json')
    assert banshee['gameplay_completed'] and banshee['audio_completed']
    assert banshee['actual_boost_consumed_sides']==banshee['actual_contrail_consumed_sides']==3
    assert banshee['actual_native_audio_consumption_verified'] and not banshee['remaining_object_audio_gaps']
    pad=reports[str(paths[-4].relative_to(BASE))];muzzle=reports[str(paths[-3].relative_to(BASE))]
    assert pad['pad_hum_verified'] and pad['pad_visual_observation_completed'] and pad['exit_code']==0
    assert muzzle['passed'] and len(muzzle['completed_draw_capture']['packets'])==3
    joined=[reports[str(p.relative_to(BASE))] for p in paths[-2:]]
    assert all(j['passed'] and j['client_exit_code']==j['authority_exit_code']==0 for j in joined)
    proof=load(ROOT/selected['qualification']);assert proof['passed'] and len(proof['smoke'])==15
    testlog=(BASE/'repair-client-lens/validation-10/cmake/tests.log').read_text()
    assert '92% tests passed, 1 tests failed out of 12' in testlog
    assert 'native_tu15_attached_effect_udp (Failed)' in testlog
    removed=[];kept=[]
    # Every candidate was inspected before this pruning step. Keep the clear
    # final Banshee chain, Needler impact and third scenery-burst references.
    discard=banshee['retained_flight_images'][:]
    for j in joined:
        chosen='firing-60.png' if j['case']=='needler' else 'spark-3.png'
        discard += [r for r in j['retained_references'] if Path(r['path']).name!=chosen]
    for row in discard:
        p=Path(row['path']);assert p.resolve().is_relative_to(BASE) and '8cfb6e3d-' in str(p)
        assert p.stat().st_size==row['bytes'] and sha(p)==row['sha256']
        p.unlink();removed.append(dict(**row,removed=not p.exists()))
    gone={r['path'] for r in removed}
    suite['required_references']=[r for r in suite['required_references'] if r['path'] not in gone]
    retention_path=BASE/'repair-scenery-audio/8cfb-capture-retention.json'
    suite['capture_retention_receipt']=str(retention_path);atomic_json(suite_path,suite)
    for j,p in zip(joined,paths[-2:]):
        for row in j['retained_references']:row['retained_after_visual_review']=row['path'] not in gone
        j['capture_retention_receipt']=str(retention_path);atomic_json(p,j)
    references=suite['required_references']+[pad['retained_reference_image'],muzzle['retained_reference']]
    references += [r for j in joined for r in j['retained_references'] if r['path'] not in gone]
    for row in references:
        p=Path(row['path']);assert p.stat().st_size==row['bytes'] and sha(p)==row['sha256'];kept.append(row)
    atomic_json(retention_path,dict(reason='Required inspected current-runtime gameplay references retained; redundant reviewed intermediates removed',removed=removed,removed_bytes=sum(r['bytes'] for r in removed),retained=kept,retained_bytes=sum(r['bytes'] for r in kept)))
    child_roots=set()
    for j in reports.values():
        child_roots.update(Path(p) for p in re.findall(r'(/home/eric/Developer/Halo-2-Decompiled/\.local/test-scratch/halo2-test-[^/" ]+)',json.dumps(j)))
    children=[dict(path=str(p),removed=not p.exists()) for p in sorted(child_roots) if p!=owner]
    assert all(p['removed'] for p in children)
    current=source_hashes(ROOT);differences=sorted(p for p in current.keys()|pins.keys() if current.get(p)!=pins.get(p))
    result=dict(status='gameplay_accepted_cleanup_pending',requested_gameplay_checks_passed=True,
        runtime=selected['runtime'],runtime_sha256=pin,launcher_sha256=sha(ROOT/'Halo2-Launcher'),
        qualification=selected['qualification'],qualified_map_view_smokes=15,frontend_stages=4,repeated_weapon_audio_cases=4,
        accepted_gameplay_cases=[str(p.relative_to(BASE)) for p in paths],case_count=len(paths),
        exact_local_reproduction='Coagulation local/hosting, two horizontal views, top KBM through Halo2-Launcher',
        banshee=dict(boost_sides=3,contrail_sides=3,consumed_vehicle_loop_voices=banshee['actual_vehicle_loop_consumed_voices'],native_output_peak=banshee['native_audio_output']['peak'],remaining_object_audio_gaps=[]),
        joined_needler=dict(queries=joined[0]['query_draws'],flares=joined[0]['flare_draws'],positive_GPU_completions=joined[0]['positive_actual_GPU_completions'],detonations=joined[0]['actual_authority_detonations']),
        joined_scenery=dict(queries=joined[1]['query_draws'],flares=joined[1]['flare_draws'],positive_GPU_completions=joined[1]['positive_actual_GPU_completions'],view_samples=joined[1]['actual_view_samples']),
        focused_validation=dict(passed=11,total=12,remaining_offline_fixture='Original Turf reaches unported2F5600 input owner before UDP transport',failure_preserved=True,report='repair-client-lens/validation-10/receipt.json'),
        original_request_fixtures=dict(effect_part_cases=168,light_callback_cases=225,light_callback_requests=90),
        original_Xbox_framebuffer_or_speaker_waveform_compared=False,all_map_or_full_campaign_parity_claimed=False,
        selected_runtime_retained_bytes=sum(p.stat().st_size for p in runtime.parent.rglob('*') if p.is_file()),
        capture_retention=str(retention_path.relative_to(BASE)),retained_reference_bytes=sum(r['bytes'] for r in kept),
        raw_audio_or_GPU_banks_retained=False,child_capture_cleanup=children,
        active_build_owner=str(owner),active_build_cleanup_pending=True,
        source_cohort_files=len(pins),later_checkout_differences_preserved=differences)
    atomic_json(BASE/'repair-installed-requalification/8cfb6e3d-accepted.json',result)
    storage_path=BASE/'repair-scenery-audio/storage-status.json';storage=load(storage_path)
    storage.update(active_build_owner=str(owner),active_build_cleanup_pending=True,current_selected_runtime=selected['runtime'],current_selected_runtime_sha256=pin,current_selected_runtime_retained_bytes=result['selected_runtime_retained_bytes'],current_capture_retention=str(retention_path),current_acceptance_report='repair-installed-requalification/8cfb6e3d-accepted.json')
    atomic_json(storage_path,storage)
    print(json.dumps(dict(accepted_cases=len(paths),retained_images=len(kept),retained_reference_bytes=result['retained_reference_bytes'],pruned_images=len(removed),runtime_retained_bytes=result['selected_runtime_retained_bytes'],later_checkout_differences=len(differences),cleanup_pending=True)),flush=True)
if __name__=='__main__':main()
