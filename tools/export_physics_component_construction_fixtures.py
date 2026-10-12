#!/usr/bin/env python3
"""Export complete original component/pool controls, without native outputs."""
import argparse,json
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--controls',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    original=json.loads(a.controls.read_text());assert not original['instruction_patches']and not original['supplied_callee_results']
    rows=[];full=stores=calls=0
    for index,c in enumerate(original['cases'],1):
        scale=int.from_bytes(bytes.fromhex(c['tick_scale_bits']),'little')
        rows.append('K '+' '.join([str(index),c['entry'],c['argument'],c['object'],*c['slots'],f'{scale:08x}',c['x87_control_word'],c['mxcsr']]))
        rows.append('R '+' '.join(c['returns']));calls+=len(c['returns'])
        for r in c['regions']:rows.append(f'M {r["address"]} {r["before"]} {r["after"]}');full+=len(r['after'])//2
        for s in c['stores']:
            before=int.from_bytes(bytes.fromhex(s['before_hex']),'little');after=int.from_bytes(bytes.fromhex(s['after_hex']),'little')
            rows.append(f'W {s["instruction"]} {s["address"]} {s["bytes"]} {before:08x} {after:08x}');stores+=1
        rows.append('E')
    wrapper_path=ROOT/'analysis/multiplayer/integration/object-physics-construction-native/measured/report.json'
    assert sha(wrapper_path)=='7f24383b35d8d2051c930e915ac962c540903d23e844553f96e6f3a6352c10e9'
    wrapper=json.loads(wrapper_path.read_text())
    capture_path=ROOT/'analysis/multiplayer/integration/physics-component-construction-native/measured/report.json'
    capture=json.loads(capture_path.read_text())
    next_call=next(c for c in wrapper['calls']if c['entry']=='001d2b10')
    assert next_call['instruction']=='001c6fd2'
    before=wrapper['states'][wrapper['initial_state']]['regions']
    after=wrapper['states'][next_call['before_state']]['regions']
    clock_pointer=next(r for r in capture['regions_before']if r['address']=='00518444')
    clock=int.from_bytes(bytes.fromhex(clock_pointer['raw_hex']),'little')
    clock_fields=[clock_pointer,next(r for r in capture['regions_before']if r['address']==f'{clock:08x}')]
    prefix_stores=[wrapper['stores'][0],*capture['stores'],wrapper['stores'][1]]
    index+=1;calls+=1;stores+=len(prefix_stores)
    scale=int.from_bytes(bytes.fromhex(capture['tick_scale_bits']),'little')
    fp=capture['floating_point'];obj=capture['object']
    rows.append(f'P {index} {obj} 00526248 004e78d0 00526230 00518444 00486560 {scale:08x} {fp["x87_control_word"]} {fp["mxcsr"]}')
    for start,end in zip(before+clock_fields,after+clock_fields,strict=True):
        assert start['address']==end['address']
        rows.append(f'M {start["address"]} {start["raw_hex"]} {end["raw_hex"]}');full+=len(end['raw_hex'])//2
    for s in prefix_stores:
        initial=int.from_bytes(bytes.fromhex(s['before_hex']),'little');final=int.from_bytes(bytes.fromhex(s['after_hex']),'little')
        rows.append(f'W {s["instruction"]} {s["address"]} {s["bytes"]} {initial:08x} {final:08x}')
    rows.append('E')
    target=out/'fixtures.tsv';target.write_text('\n'.join(rows)+'\n')
    report=dict(schema='physics-component-construction-fixture-v1',exporter_sha256=sha(Path(__file__)),
        original_controls_sha256=sha(a.controls),fixture_sha256=sha(target),cases=index,calls=calls,stores=stores,
        additional_input_pins={str(wrapper_path):sha(wrapper_path),str(capture_path):sha(capture_path)},
        full_output_bytes=full,expected_from_native=False,recorded_lower_boundaries_replayed=0,
        complete_original_control_cases=510,additional_native_composed_required_rebuild_stops=1,
        private_x86_abi_stack_and_full_fp_sticky_state_excluded=True,
        live_world_activation_or_contact_collision_claimed=False)
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),fixture_sha256=sha(target),full_output_bytes=full)))

if __name__=='__main__':main()
