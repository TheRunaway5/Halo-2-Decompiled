#!/usr/bin/env python3
"""Compare every captured Hunter AI tick with untouched Xbox code and hardware.

The source world owns object/particle updates between these AI calls. This
verifies each AI consumer and its actual API output, not whole object physics,
campaign boot, hostile firing, rendered frames or all weapons.
An explicitly selected prefix verifies only those calls if a later stage fails.
"""
import argparse,concurrent.futures,json,subprocess,time
from pathlib import Path
from audit_rendering import file_sha
from oracle_charged_effect_lifetime_bank_tu15 import regions
from verify_particle_frame_evidence_tu15 import equal_banks
from validate_ai_hardware_step_reference_tu15 import validate as validate_hardware_step, publish as publish_hardware_step
ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',type=Path,required=True);ap.add_argument('--replay-build',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--workers',type=int,default=2)
    ap.add_argument('--first-frames',type=int,help='Verify this completed prefix while a later continuation remains open')
    ap.add_argument('--hardware-estimates',action='store_true',help='Observe original hardware traces when the Unicorn SSE estimate model differs; inject no estimate results')
    a=ap.parse_args();case=a.case.resolve();build=a.replay_build.resolve();out=a.output.resolve()
    assert not out.exists() and 1<=a.workers<=4;out.mkdir(parents=True)
    source=json.loads((case/'build.json').read_text());assert source['returncode'] in (0,1)
    compiled=json.loads((build/'build.json').read_text());assert compiled['passed']
    replay=build/'replay-native';assert file_sha(replay)==compiled['binary_sha256']
    frames=sorted((p for p in case.glob('ai-frame*') if p.is_dir()),key=lambda p:int(p.name[8:]))
    declared_count=source['scope'].get('ai_tick_count',90)
    assert isinstance(declared_count,int) and 1<=declared_count<=1024
    expected_count=a.first_frames or declared_count
    assert 1<=expected_count<=declared_count
    if a.first_frames:
        assert len(frames)>=expected_count;frames=frames[:expected_count]
    else:
        assert source['returncode']==0 and len(frames)==expected_count
    for ordinal,p in enumerate(frames,1):
        metadata=json.loads((p/'native.json').read_text());assert metadata['completed']
        assert metadata['ordinal']==ordinal and metadata['tick']==59+ordinal
    python=ROOT/'.tools/python/bin/python';results=[];failures=[];start=time.monotonic()
    def verify(frame):
        destination=out/frame.name;destination.mkdir();original=destination/'original';native=destination/'native'
        commands=[[str(python),str(ROOT/'tools/probe_original_charge_particle_tu15.py'),'--case',str(frame),'--output',str(original),'--entry','0x1d0670','--phase','before'],
            [str(python),str(ROOT/'tools/verify_original_particle_hardware_tu15.py'),'--case',str(original)],
            [str(replay),str(original),'0x1d0670',str(native)]]
        for ordinal,command in enumerate(commands):
            with (destination/f'command{ordinal}.log').open('w') as log:
                run=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=False,timeout=120)
            if run.returncode:
                if ordinal!=1 or not a.hardware_estimates:run.check_returncode()
                boundary=json.loads((original/'hardware/report.json').read_text())
                assert boundary['completed'] and not boundary['emulator_retained_state_matches_hardware']
                hardware_steps=destination/'hardware-step'
                with (destination/'hardware-step.log').open('w') as log:
                    subprocess.run([str(python),str(ROOT/'tools/trace_original_ai_hardware_tu15.py'),'--case',str(original),'--output',str(hardware_steps)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=600)
                publish_hardware_step(original,hardware_steps)
        r=json.loads((original/'report.json').read_text());h=json.loads((original/'hardware/report.json').read_text())
        hardware_step=validate_hardware_step(original,r,h)
        trace_reference=original/'hardware/hardware-trace.bin' if hardware_step else original/'original-trace.bin'
        store_reference=original/'hardware/hardware-stores.bin' if hardware_step else original/'original-stores.bin'
        # Public entry diagnostics retain CPU/bank/stores at the actual API.
        # Their independent strict replay verifies the instruction stream;
        # do not claim an unavailable source-API instruction trace.
        public=json.loads((frame/'native.json').read_text()).get('public_campaign_placement',False)
        trace_available=(frame/'native-trace.bin').exists()
        assert public or trace_available
        if trace_available:assert (frame/'native-trace.bin').read_bytes()==trace_reference.read_bytes()
        assert (frame/'native-stores.bin').read_bytes()==store_reference.read_bytes()
        expected=bytearray((original/'hardware/hardware-exit.bin').read_bytes());assert expected[161]&2
        expected[161]&=0xfd;assert (frame/'native-exit.bin').read_bytes()==expected
        expected_bytes=sum(len(data) for p,data in regions(original/'before.bin') if not 0x60000000<=p<0x60200000)
        compared=equal_banks(frame/'after.bin',original/'hardware/hardware-after.bin');assert compared==expected_bytes
        result=dict(frame=frame.name,source_build_sha256=file_sha(frame/'build.json'),original_report_sha256=file_sha(original/'report.json'),
            hardware_report_sha256=file_sha(original/'hardware/report.json'),instructions=r['original_instructions'],writes=r['retained_stores'],
            retained_bytes=compared,source_api_trace_available=trace_available,source_api_trace_exact=True if trace_available else None,
            source_api_writes_exact=True,source_api_full_hardware_exit_except_IF=True,
            strict_standalone_native_replay_trace_exact=True,
            observed_original_hardware_estimate_reference=hardware_step,
            unicorn_retained_state_matches_hardware=h['emulator_retained_state_matches_hardware'],
            strict_standalone_native_replay_passed=True,commands=commands,passed=True)
        (destination/'comparison.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');return result
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.workers) as pool:
        futures={pool.submit(verify,p):p for p in frames}
        for future in concurrent.futures.as_completed(futures):
            try:results.append(future.result())
            except Exception as error:failures.append(dict(frame=futures[future].name,error=str(error)))
            if (len(results)+len(failures))%5==0:print('Verified',len(results),'failed',len(failures),'of',expected_count,flush=True)
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),source_build_sha256=file_sha(case/'build.json'),
        replay_build_sha256=file_sha(build/'build.json'),comparisons=sorted(results,key=lambda r:int(r['frame'][8:])),failures=failures,
        expected_ai_calls=expected_count,declared_fixture_ai_calls=declared_count,
        selected_prefix=bool(a.first_frames),whole_fixture_completed=source['returncode']==0,
        elapsed_seconds=round(time.monotonic()-start,2),passed=len(results)==expected_count and not failures,full_weapon_parity=False,physical_xbox_verified=False)
    (out/'report.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=result['passed'],verified=len(results),failures=len(failures))))
    return 0 if result['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
