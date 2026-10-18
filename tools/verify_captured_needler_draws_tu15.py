#!/usr/bin/env python3
"""Authenticate real Needler draw records and same-queue resource admission.

This verifies submitted vertex inputs and source-program provenance. It does
not establish vertex execution, rasterization, framebuffer or audio parity.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import struct
from audit_rendering import Cache, file_sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--flow',type=Path,required=True)
    parser.add_argument('--queue-api-evidence',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert not args.output.exists()
    flow=args.flow;source=json.loads((flow/'report.json').read_text())
    receipt=json.loads((flow/'run.json').read_text())
    assert receipt['returncode']==0 and file_sha(flow/'report.json')==receipt['actual_report_sha256']
    assert source['passed'] and source['completed_ticks']==240 and source['rounds_consumed']==11
    assert not source['source_state_or_game_results_supplied'] and source['parallel_native_shots']==0
    api=json.loads(args.queue_api_evidence.read_text())
    assert api['passed'] and api['original_entry']==0x1770c0
    captured=flow/'captured-entry'/'build.json'
    capture=json.loads(captured.read_text())
    assert file_sha(captured)==api['actual_API_build_sha256']
    assert capture['source_flow_report_sha256']==file_sha(flow/'report.json')
    assert capture['source_flow_receipt_sha256']==file_sha(flow/'run.json')
    assert capture['returncode']==0 and capture['parent_returncode']==0
    assert capture['binary_sha256']==receipt['binary_sha256']
    assert api['checks']['actual_API_and_strict_complete_nonstack_banks_exact']
    assert api['checks']['actual_API_full_CPU_FP_except_IF_exact']
    comparisons=source['resource_admission_comparisons']
    assert len(comparisons)==405
    checked=[r for r in comparisons if r['tick']==175 and r['view']==0]
    assert len(checked)==1 and checked[0]['source_unchanged_checked']
    assert not checked[0]['firing_only_effects'] and checked[0]['admitted_effects']
    assert all(not r['firing_only_effects'] for r in comparisons)
    cache=Cache(Path(receipt['command'][1]));totals=collections.Counter();programs={}
    wanted={name:next(datum for datum,tag in cache.tags.items() if tag['group']=='effe' and tag['name']==name)
        for name in ('effects\\objects\\weapons\\pistol\\needler\\firing',
            'effects\\impact\\explosion_small\\needler\\standard_detonation',
            'effects\\impact\\explosion_small\\needler\\super_detonation')}
    data=(flow/'particle-draws.bin').read_bytes();offset=0
    for sample in source['particle_draw_samples']:
        for draw in sample['draws']:
            # Eight explicit uint32 header words,3360 vertex-state bytes and
            #32 depth-state bytes. No host pointers or C++ containers enter it.
            assert draw['record_offset']==offset and draw['record_bytes']==3424
            header=struct.unpack_from('<8I',data,offset)
            assert header==(sample['tick'],sample['view'],int(sample['first_person']),draw['effect'],
                draw['owner'],draw['template'],draw['pass'],draw['vertex'])
            assert draw['effect'] in cache.tags and cache.tags[draw['effect']]['group']=='effe'
            assert draw['classification'] in (7,8,9) and 'sort_owner' in draw
            position=struct.unpack_from('<3f',data,offset+32+4*16)
            assert all(abs(x)<float('inf') for x in position)
            key=(draw['vertex'],draw['classification'])
            if key not in programs:
                _,root=cache.tag(key[0],'vrtx');rows=list(cache.block(root+4,28));row=rows[key[1]]
                size=cache.u32(row+12);start=cache.pointer(cache.u32(row+16),cache.magic,size)
                original=bytes(cache.data[start:start+size])
                path=flow/f'vertex-program-{key[0]}-class-{key[1]}.bin'
                assert path.read_bytes()==original and len(original)==draw['source_vertex_program_bytes']
                token,count=struct.unpack_from('<2H',original)
                assert token==0x2078 and len(original)==4+count*16 and 0<count<=136
                programs[key]=dict(vertex_datum=key[0],classification=key[1],bytes=size,
                    instructions=count,source_sha256=hashlib.sha256(original).hexdigest(),file=path.name)
            totals[(sample['first_person'],draw['effect'])]+=1;offset+=3424
    assert offset==len(data) and len(source['particle_draw_samples'])==810
    assert totals[(True,wanted['effects\\objects\\weapons\\pistol\\needler\\firing'])]>0
    for name in ('effects\\impact\\explosion_small\\needler\\standard_detonation',
                 'effects\\impact\\explosion_small\\needler\\super_detonation'):
        assert totals[(False,wanted[name])]>0
    report=dict(passed=True,scope=__doc__,source_report_sha256=file_sha(flow/'report.json'),
        source_receipt_sha256=file_sha(flow/'run.json'),source_draw_bytes=len(data),
        source_draw_sha256=file_sha(flow/'particle-draws.bin'),source_programs=list(programs.values()),
        actual_queue_capture_bound_to_draw_sequence=True,
        draws=[dict(first_person=fp,effect=effect,name=cache.tags[effect]['name'],count=count)
            for (fp,effect),count in sorted(totals.items())],
        original_queue_instructions=api['original_instructions'],original_queue_writes=api['original_nonstack_writes'],
        original_queue_api_evidence_sha256=file_sha(args.queue_api_evidence),
        same_completed_queue_baseline_world_draws=0,
        same_completed_queue_admitted_world_draws=sum(len(r['admitted_effects']) for r in comparisons),
        positive_world_comparison_source_unchanged=True,actual_shader_program_bytes_match_original_map=True,
        tool_sha256=file_sha(Path(__file__)),vertex_execution_verified=False,
        rendered_sequence_verified=False,audible_playback_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','source_draw_bytes','original_queue_instructions',
        'original_queue_writes','same_completed_queue_admitted_world_draws')}))


if __name__=='__main__':main()
