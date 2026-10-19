#!/usr/bin/env python3
"""Run the real finite campaign application and check weapon Vulkan commands.

Use run_test.py. Large application logs and all temporary outputs stay in
HALO2_TEST_SCRATCH. Command submission is not visible-pixel or Xbox frame proof.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

from mcc_content import MccCache
from weapon_test_cache import require_mcc_map, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--map',type=Path,required=True)
    parser.add_argument('--source-case',type=Path,required=True)
    parser.add_argument('--scene-case',type=Path,help='Independent full-view CPU weapon geometry/material case')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    args.map=require_mcc_map(args.map)
    source=json.loads(args.source_case.read_text())
    assert source['runtime_cache_source']=='MCC' and source['native_world_constructor']
    assert Path(source['runtime_cache_path']).resolve()==args.map
    assert source['passed'] and source['source_weapon_submission_verified']
    expected={row['definition']for row in source['weapon_submissions']}
    if args.scene_case:
        scene=json.loads(args.scene_case.read_text())
        assert scene['runtime_cache_source']=='MCC' and scene['native_world_constructor']
        assert Path(scene['runtime_cache_path']).resolve()==args.map
        assert scene['passed'] and scene['scene_weapon_geometry_verified']
        scene_expected={row['definition']for row in scene['weapon_submissions']}
        assert expected<=scene_expected
        expected=scene_expected
    cache=MccCache(args.map)
    try:
        squad=source['squad_name']
        authored_weapons={datum:dict(datum=datum,name=tag['name'],group=tag['group'])
            for datum,tag in cache.tags.items()if tag['group']=='weap'}
    finally:
        cache.close()
    assert squad
    events=scratch/'weapon-gpu.jsonl';log=scratch/'application.log'
    command=['xvfb-run','-a','-s','-screen 0 800x600x24',str(args.native.absolute()),
        '--map',str(args.map.absolute()),'--shared-maps',str(args.map.absolute().parent),
        '--play','--place-squad',squad,
        '--frames','4','--viewport','640','480','--hidden','--mute']
    environment=dict(os.environ,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
        HALO2_TEST_CAMPAIGN_WEAPON_GPU_REPORT=str(events),XDG_DATA_HOME=str(scratch/'data'),
        XDG_CACHE_HOME=str(scratch/'cache'))
    timed_out=False;started=time.monotonic()
    with log.open('wb')as output:
        try:result=subprocess.run(command,env=environment,stdout=output,stderr=subprocess.STDOUT,timeout=300);code=result.returncode
        except subprocess.TimeoutExpired:code=124;timed_out=True
    records=[];malformed=0
    if events.exists():
        assert events.stat().st_size<=4*1024*1024
        for line in events.read_text().splitlines():
            try:records.append(json.loads(line))
            except json.JSONDecodeError:malformed+=1
    frame_outcomes=[r for r in records if r['event']=='frame']
    frames={r['frame']for r in frame_outcomes if r['submitted']}
    committed=[];pending=[]
    for record in records:
        if record['event']=='frame':
            if record['submitted']:committed.extend(pending)
            pending=[]
        else:pending.append(record)
    all_draws=[r for r in committed if r['event']=='gpu_draw']
    all_inputs=[r for r in committed if r['event']=='source_weapon']
    draws=[r for r in all_draws if r['definition']in expected]
    inputs=[r for r in all_inputs if r['definition']in expected]
    input_owners={(r['frame'],r['object_index'],r['owner'],r['definition'],r['ordinal'])for r in all_inputs}
    valid=all((r['frame'],r['object_index'],r['owner'],r['definition'],r['ordinal'])in input_owners and
        r['definition']in authored_weapons and r['count']>0 and r['pipeline'] and r['descriptor'] and
        r['vertex_buffer'] and r['owner_constants']for r in all_draws)
    input_pairs={(r['frame'],r['definition'])for r in inputs}
    draw_pairs={(r['frame'],r['definition'])for r in draws}
    complete=all((frame,definition)in input_pairs and (frame,definition)in draw_pairs
        for frame in frames for definition in expected)
    passed=bool(code==0 and len(frames)==4 and draws and inputs and valid and complete and not malformed)
    with log.open('rb')as stream:
        stream.seek(0,2);stream.seek(max(0,stream.tell()-32768));tail=stream.read().decode(errors='replace')
    report=dict(passed=passed,application_exit_code=code,timed_out=timed_out,command=command,
        runtime_cache_source='MCC',runtime_cache_path=str(args.map),native_world_constructor=True,
        application_elapsed_seconds=time.monotonic()-started,frame_outcomes=frame_outcomes,
        diagnostic_events=[r for r in records if r['event']not in ('frame','gpu_draw','source_weapon')],
        uncommitted_attempt_events=len(pending),
        squad=squad,expected_weapon_definitions=sorted(expected),submitted_frames=sorted(frames),
        source_weapon_inputs=inputs,weapon_gpu_commands=draws,malformed_event_lines=malformed,
        all_source_weapon_inputs=all_inputs,all_weapon_gpu_commands=all_draws,
        authored_weapon_identities=[authored_weapons[d]for d in sorted({r['definition']for r in all_inputs})],
        expected_weapon_input_and_command_coverage_per_frame=complete,
        actual_owner_constant_and_vertex_bindings_verified=bool(draws and valid),
        gpu_or_software_vulkan_device='See actual application output; physical GPU execution not asserted',
        visible_pixels_verified=False,matched_xbox_frame_verified=False,full_weapon_parity=False,
        application_log_tail=tail,application_log_sha256=sha(log),
        application_sha256=sha(args.native),map_sha256=sha(args.map),
        source_case_sha256=sha(args.source_case),tool_sha256=sha(Path(__file__)),
        scene_case_sha256=sha(args.scene_case)if args.scene_case else None,
        runner_scratch=str(scratch),large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,exit_code=code,submitted_frames=len(frames),
        source_weapon_inputs=len(inputs),weapon_gpu_commands=len(draws))),flush=True)
    if not passed:print(tail[-6000:],flush=True)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
