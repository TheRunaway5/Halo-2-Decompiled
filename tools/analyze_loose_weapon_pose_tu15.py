#!/usr/bin/env python3
"""Analyze actual source pose/palette submissions and native LAN reception.

Use run_test.py. Exact completed-tick node/palette inputs do not certify an
Xbox framebuffer, interpolation at other fractions, or the full weapon scope.
"""
import argparse
import json
import os
from pathlib import Path
import struct
from original_xbox_pose_case import sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--validation',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();cases=[];references=[]
    for species in ('spartan','elite'):
        path=args.validation/'release/evidence'/f'weapon-source-dual-{species}.json'
        trace=json.loads(path.read_text());metrics=trace['world_pose_metrics'];admission=metrics.get('lan_admission',{})
        scales=[dict(serial=row['serial'],name=row['name'],word=row['transform_words'][0],
            scale=struct.unpack('<f',struct.pack('<I',row['transform_words'][0]))[0])
            for row in metrics.get('original_explosion_locations',[])]
        passed=bool(trace['render_loop_completed'] and metrics['enabled'] and metrics['render_fraction']==1 and
            metrics['publication_checks']>0 and metrics.get('draws',0)>0 and metrics.get('dropped_draws',0)>0 and
            metrics.get('original_palette_matrices',0)>0 and metrics.get('mismatches',1)==0 and
            admission.get('ready') and not admission.get('error') and admission.get('rejected_packets')==0 and
            metrics.get('native_lan_poses',0)>0 and scales and
            metrics.get('native_lan_explosion_locations')==len(scales))
        cases.append(dict(species=species,representation=trace['representation'],source_unit_definition=trace['expected_unit_definition'],
            render_inputs_and_native_transport_passed=passed,metrics=metrics,original_explosion_scales=scales,
            full_native_firing_sequence_passed=False,firing_sound_submissions=trace['observations']['firing_sounds'],
            fired_rounds=trace['observations']['fired_rounds'],framebuffer_parity_verified=False))
        references.append(dict(path=str(path),sha256=sha(path)))
    report=dict(passed=all(row['render_inputs_and_native_transport_passed']for row in cases),
        component='Original completed loose-weapon WORLD/palette submission and native LAN pose ownership',cases=cases,
        original_node_publications=sum(row['metrics']['publication_checks']for row in cases),
        original_completed_draws=sum(row['metrics'].get('draws',0)for row in cases),
        original_palette_matrices=sum(row['metrics'].get('original_palette_matrices',0)for row in cases),
        original_node_and_palette_mismatches=sum(row['metrics'].get('mismatches',0)for row in cases),
        original_instruction_patches=[],supplied_callee_results=[],full_weapon_parity=False,fully_certified_weapons=0,
        framebuffer_parity_verified=False,intermediate_render_fraction_verified=False,
        references=references,runner_scratch=os.environ['HALO2_TEST_SCRATCH'],tool_sha256=sha(Path(__file__)),large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in
        ('passed','original_node_publications','original_completed_draws','original_palette_matrices','original_node_and_palette_mismatches')}),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
