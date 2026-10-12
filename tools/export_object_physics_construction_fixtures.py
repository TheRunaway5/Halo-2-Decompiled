#!/usr/bin/env python3
"""Export original physical-construction controls and labeled lower replay ABI."""
import argparse,hashlib,json
from pathlib import Path
from freeze_mp_native_sound_storage import sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--controls',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    original=json.loads(a.capture.read_text());controls=json.loads(a.controls.read_text())
    for report in(original,controls):assert not report['instruction_patches']and not report['supplied_lower_results']
    states=[];ids={}
    def state(regions):
        text=json.dumps(regions,sort_keys=True,separators=(',',':'));key=hashlib.sha256(text.encode()).hexdigest()
        if key not in ids:ids[key]=len(states);states.append(regions)
        return ids[key]
    mapping={s['id']:state(s['regions'])for s in original['states']};cases=[]
    cases.append(dict(entry=original['root']['entry'],first=original['root']['root'],second=original['root']['object'],request='00000000',
        slots=['004e78d0','00526248','00486560','00526230'],before=mapping[original['initial_state']],after=mapping[original['final_state']],
        result='00000000',calls=[dict(c,before_state=mapping[c['before_state']],after_state=mapping[c['after_state']])for c in original['calls']],stores=original['stores']))
    for c in controls['cases']:
        before=state([dict(address=r['address'],raw_hex=r['before'])for r in c['regions']])
        after=state([dict(address=r['address'],raw_hex=r['after'])for r in c['regions']])
        cases.append(dict(entry=c['entry'],first=c['first'],second=c['second'],request=c['request'],slots=c['state_slots'],before=before,after=after,
            result=c['return_eax'],calls=[],stores=c['stores']))
    rows=[]
    for i,regions in enumerate(states):
        rows.append(f'S {i}')
        for r in regions:rows.append(f'M {r["address"]} {r["raw_hex"]}')
        rows.append('E')
    full=sum(sum(len(r['raw_hex'])//2 for r in states[c['after']])for c in cases)
    for index,c in enumerate(cases,1):
        rows.append('K '+' '.join([str(index),c['entry'],c['first'],c['second'],c['request'],*c['slots'],str(c['before']),str(c['after']),c['result']]))
        for call in c['calls']:
            rows.append('L '+' '.join([call['instruction'],call['entry'],call['object'],call['component'],str(call['before_state']),str(call['after_state']),call['return_eax']]))
        for s in c['stores']:
            before=int.from_bytes(bytes.fromhex(s['before_hex']),'little');after=int.from_bytes(bytes.fromhex(s['after_hex']),'little')
            rows.append(f'W {s["instruction"]} {s["address"]} {s["bytes"]} {before:08x} {after:08x}')
        rows.append('E')
    target=out/'fixtures.tsv';target.write_text('\n'.join(rows)+'\n')
    report=dict(schema='object-physics-construction-fixture-v1',exporter_sha256=sha(Path(__file__)),
        input_pins={str(p):sha(p)for p in(a.capture,a.controls)},fixture_sha256=sha(target),cases=len(cases),
        states=len(states),stores=sum(len(c['stores'])for c in cases),full_output_bytes=full,
        original_actual_upper_caller_cases=1,recorded_lower_boundaries_for_upper_caller_only=len(original['calls']),
        complete_no_lower_control_cases=len(controls['cases']),expected_from_native=False,
        whole_native_physics_construction_or_live_collision_claimed=False)
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(fixture_sha256=sha(target),report_sha256=sha(out/'report.json'),cases=len(cases),full_output_bytes=full)))

if __name__=='__main__':main()
