#!/usr/bin/env python3
"""Export measured original upper ABI and no-replay native-required stops."""
import argparse,hashlib,json
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--controls',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    capture=json.loads(a.capture.read_text());controls=json.loads(a.controls.read_text())
    operands=json.loads((a.controls.parent/'operands.json').read_text());states=[];ids={}
    for result in(capture,controls):assert not result['instruction_patches']and not result['supplied_callee_results']
    tables=[dict(address=f'{int(x["remap"],16)-128:08x}',raw_hex='')for x in operands['images']]
    # Use the complete original signed-byte lookup backing from the controls.
    for i,table in enumerate(tables):
        c=next(c for c in controls['cases']if c['image']==i+1)
        table['raw_hex']=next(r['before']for r in c['regions']if r['address']==table['address'])
    def state(regions):
        key=hashlib.sha256(json.dumps(regions,sort_keys=True,separators=(',',':')).encode()).hexdigest()
        if key not in ids:ids[key]=len(states);states.append(regions)
        return ids[key]
    mapping={s['id']:state(s['regions']+[tables[0]])for s in capture['states']}
    branches=operands['images'][0]['labels'];base_slots=['00526248','004e78d0',operands['images'][0]['remap'],operands['images'][0]['table']]
    cases=[dict(mode=0,datum=capture['root']['datum'],slots=base_slots,branches=branches,
        before=mapping[capture['initial_state']],after=mapping[capture['final_state']],extra=['0']*6,
        calls=[dict(c,before_state=mapping[c['before_state']],after_state=mapping[c['after_state']])for c in capture['calls']],stores=capture['stores'])]
    for c in controls['cases']:
        before=state([dict(address=r['address'],raw_hex=r['before'])for r in c['regions']])
        after=state([dict(address=r['address'],raw_hex=r['after'])for r in c['regions']])
        cases.append(dict(mode=1,datum=c['datum'],slots=c['slots'],branches=c['branches'],before=before,after=after,extra=['0']*6,
            calls=[dict(c['required_lower'],before_state=after,after_state=after,return_eax='00000000')],stores=c['stores']))
    # Real complete outer+allocator+initializer composition, no lower replay.
    wrapper_path=ROOT/'analysis/multiplayer/integration/object-physics-construction-native/measured/report.json'
    allocation_path=ROOT/'analysis/multiplayer/integration/physics-component-construction-native/measured/report.json'
    wrapper=json.loads(wrapper_path.read_text());allocation=json.loads(allocation_path.read_text())
    assert sha(wrapper_path)=='7f24383b35d8d2051c930e915ac962c540903d23e844553f96e6f3a6352c10e9'
    assert sha(allocation_path)=='767e6731f6709f217f02b687ff5bf801b06a28d7d10a8196de97e89b6b9d3831'
    clock_slot=next(r for r in allocation['regions_before']if r['address']=='00518444')
    clock=int.from_bytes(bytes.fromhex(clock_slot['raw_hex']),'little')
    clock_fields=[clock_slot,next(r for r in allocation['regions_before']if r['address']==f'{clock:08x}')]
    before=state(wrapper['states'][wrapper['initial_state']]['regions']+clock_fields+[tables[0]])
    first=capture['calls'][0];after=state(capture['states'][first['before_state']]['regions']+clock_fields+[tables[0]])
    scale=int.from_bytes(bytes.fromhex(allocation['tick_scale_bits']),'little')
    fp=allocation['floating_point'];prefix=[wrapper['stores'][0],*allocation['stores'],wrapper['stores'][1]]
    cases.append(dict(mode=2,datum=allocation['object'],slots=base_slots,branches=branches,before=before,after=after,
        extra=['00526230','00518444','00486560',f'{scale:08x}',fp['x87_control_word'],fp['mxcsr']],
        calls=[dict(first,before_state=after,after_state=after)],stores=prefix))
    rows=[]
    for i,regions in enumerate(states):
        rows.append(f'S {i}')
        for r in regions:rows.append(f'M {r["address"]} {r["raw_hex"]}')
        rows.append('E')
    for i,c in enumerate(cases,1):
        rows.append('K '+' '.join([str(i),str(c['mode']),c['datum'],*c['slots'],*c['branches'],str(c['before']),str(c['after']),*c['extra']]))
        for call in c['calls']:
            args=call['arguments']+['00000000']*(4-len(call['arguments']))
            rows.append('L '+' '.join([call['instruction'],call['entry'],call['component'],call['object'],call['linear'],call['angular'],
                str(len(call['arguments'])),*args,str(call['before_state']),str(call['after_state']),call['return_eax']]))
        for s in c['stores']:
            left=int.from_bytes(bytes.fromhex(s['before_hex']),'little');right=int.from_bytes(bytes.fromhex(s['after_hex']),'little')
            rows.append(f'W {s["instruction"]} {s["address"]} {s["bytes"]} {left:08x} {right:08x}')
        rows.append('E')
    full=sum(sum(len(r['raw_hex'])//2 for r in states[c['after']])for c in cases);stores=sum(len(c['stores'])for c in cases)
    rows.append(f'Z {len(cases)} {stores} {len(capture["calls"])} {len(cases)-1} {full}')
    target=out/'fixtures.tsv';target.write_text('\n'.join(rows)+'\n')
    result=dict(schema='physics-component-rebuild-fixture-v1',exporter_sha256=sha(Path(__file__)),
        input_pins={str(p):sha(p)for p in(a.capture,a.controls,a.controls.parent/'operands.json',wrapper_path,allocation_path)},
        fixture_sha256=sha(target),cases=len(cases),states=len(states),stores=stores,full_output_bytes=full,
        recorded_lower_boundaries_for_upper_caller_only=len(capture['calls']),no_replay_required_stops=len(cases)-1,
        native_outer_allocator_init_rebuild_required_grouping_stops=1,expected_from_native=False,
        live_collision_world_completion_or_final_unit_restoration_claimed=False)
    (out/'report.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),fixture_sha256=sha(target),cases=len(cases),full_output_bytes=full)))

if __name__=='__main__':main()
