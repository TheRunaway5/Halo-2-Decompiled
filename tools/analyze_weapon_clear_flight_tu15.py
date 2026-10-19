#!/usr/bin/env python3
"""Verify retained live source flight/glow observations; use run_test.py."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--validation',type=Path,required=True)
    parser.add_argument('--source-analysis',type=Path,required=True)
    parser.add_argument('--baseline',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    path=args.validation/'release/evidence/weapon-source-firing-pinned.json'
    trace=json.loads(path.read_text());source=json.loads(args.source_analysis.read_text())
    baseline=json.loads(args.baseline.read_text())
    assert source['passed'] and source['full_body_correspondence_reverified']
    assert source['cases'][0]['source_report_sha256']==sha(path)
    assert baseline['cases'][0]['all_created_bolts_contact_and_retire_during_birth_tick']
    assert trace['render_loop_completed'] and trace['source_shot_tick_equality_checked']
    lane=trace['clear_firing_lane'];assert lane['host_geometry_used_only_for_scene_selection']
    assert lane['original_position_owner'] and lane['host_selection_ray_minimum_clearance']>=8
    created={row['returned_object']for row in trace['projectile_births']if row['completed'] and row['returned_object']!=0xffffffff}
    owners=set(trace['flight_owners']);assert len(created)==4 and owners==created
    assert set(trace['glow_owners'])==owners and trace['glow_draws']>0 and trace['glow_vertices']>0
    assert trace['glow_views']==trace['glow_screen_views']==1
    observations={owner:[]for owner in owners}
    for sample in trace['samples']:
        assert sample['legacy_shot_tick']==sample['weapon_16c_words'][55]
        for projectile in sample['live_projectiles']:
            assert projectile['object']in owners
            distance=struct.unpack('<f',struct.pack('<I',projectile['traveled_bits']))[0]
            assert math.isfinite(distance) and distance>=0
            observations[projectile['object']].append((sample['tick'],distance))
    for rows in observations.values():
        assert len(rows)>=2
        assert all(b[0]>a[0] and b[1]>=a[1]for a,b in zip(rows,rows[1:]))
    maximum=max(distance for rows in observations.values()for _,distance in rows)
    assert maximum>2 and trace['maximum_projectile_travel']>=maximum
    retirements=source['cases'][0]['retirements']
    assert {row['object']for row in retirements}==owners
    assert all(row['elapsed_source_ticks']>0 and row['completed_contact_effects']>0 for row in retirements)
    cache=ROOT/'assets/disc/maps/lockout.map'
    assert sha(cache)=='14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
    report=dict(passed=True,component='Actual source bolt flight publication and authored glow submission',
        authored_scene=lane,original_map_sha256=sha(cache),source_constructor_entry='B9B70',
        original_xbe_sha256=source['original_xbe_sha256'],
        source_created_objects=len(created),live_flight_owners=len(owners),
        checked_live_samples=sum(len(rows)for rows in observations.values()),
        observed_source_travel_maximum=maximum,maximum_live_bolts=trace['maximum_live_bolts'],
        observed_retirements=len(retirements),observed_flight_ticks=[row['elapsed_source_ticks']for row in retirements],
        observed_source_contact_effects=len(source['cases'][0]['published_source_contacts']),
        authored_glow_draws=trace['glow_draws'],authored_glow_vertices=trace['glow_vertices'],
        glow_owners=trace['glow_owners'],actual_glow_view_mask=trace['glow_screen_views'],
        previous_scene_immediate_contact_distance=baseline['cases'][0]['observed_birth_to_contact_distance_range'],
        firing_sound_submissions=trace['firing_sound_submissions'],
        full_firing_sound_sequence_passed=trace['firing_sound_submissions']==trace['legacy_fired_rounds'],
        original_framebuffer_verified=False,original_glow_input_comparison_for_this_sequence_verified=False,
        complete_projectile_lifecycle_verified=False,full_weapon_parity=False,fully_certified_weapons=0,
        references=[dict(path=str(p),sha256=sha(p))for p in (path,args.source_analysis,args.baseline)],
        source_test_sha256=sha(ROOT/'tests/native_covenant_weapon_runtime_tests.cpp'),
        tool_sha256=sha(Path(__file__)),runner_scratch=os.environ['HALO2_TEST_SCRATCH'],
        large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in ('passed','checked_live_samples',
        'observed_source_travel_maximum','authored_glow_draws','full_firing_sound_sequence_passed')}),flush=True)


if __name__=='__main__':main()
