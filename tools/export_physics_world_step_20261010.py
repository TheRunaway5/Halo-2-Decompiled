#!/usr/bin/env python3
"""Export genuine original Lockout world/island calls; never reads native output."""
import argparse, hashlib, json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN, _Code
from export_mp_power_body_slots import full_snapshot
from export_mp_physics_contact_lifecycle import add, segments

SOURCE=ROOT/'analysis/multiplayer/sandbox-source/power-agent-scheduler/final/measured/report.json'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--controls',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--connect-current-broadphase',action='store_true')
    ap.add_argument('--connect-contact-dispatch',action='store_true')
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    assert not a.connect_contact_dispatch or a.connect_current_broadphase
    manifest=SOURCE.parent.parent/'manifest.json';pins=json.loads(manifest.read_text())['files']
    assert pins[str(SOURCE.relative_to(ROOT))]==sha(SOURCE)
    r=json.loads(SOURCE.read_text());calls=r['calls'];out=json.loads(a.controls.read_text())
    if a.connect_current_broadphase:out['cases']=[]
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    code=_Code(image,'.text')
    cases=[]
    # Full natural original island integration, and actual world-phase prefixes.
    selected=[('0031cfe0',None),('00316200','0031db70'),('00316440','00313310')]
    if a.connect_current_broadphase:selected=[('00316200','00300870')]
    if a.connect_contact_dispatch:selected.append(('00316200','00300400'))
    for entry,boundary_entry in selected:
        c=next(q for q in calls if q['entry']==entry)
        boundary=None
        if boundary_entry:
            boundary=next(q for q in calls if c['order']<q['order']<c['return_order'] and q['entry']==boundary_entry)
        end=boundary['order'] if boundary else c['return_order']
        memory={}
        for p,b in full_snapshot(r['states'][c['before_state']]):add(memory,p,b)
        for q in c['packets']:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
        current=dict(memory);writes=[];estimates=[];last_estimate=None;abi_stores=[]
        events=sorted([('R',q)for q in r['reads']if c['order']<q['order']<end]+[('W',q)for q in r['stores']if c['order']<q['order']<end],key=lambda q:q[1]['order'])
        for kind,q in events:
            p=int(q['address'],16);raw=bytes.fromhex(q['raw_hex'] if kind=='R' else q['before_hex'])
            for i,b in enumerate(raw):
                if p+i not in current:memory[p+i]=b;current[p+i]=b
                assert current[p+i]==b,(entry,kind,q)
            if kind=='W':
                site=int(q['instruction'],16);raw=bytes.fromhex(q['written_hex'])
                for i,b in enumerate(raw):current[p+i]=b
                if site==0x1dc77e:last_estimate=int.from_bytes(raw[:4],'little')
                if site==0x1dc786:estimates.append([last_estimate,int.from_bytes(raw,'little')]);last_estimate=None
                if site==0x30bb1d and p%16==0:last_estimate=int.from_bytes(raw[:4],'little')
                if site==0x30bb26:estimates.append([last_estimate,int.from_bytes(raw,'little')]);last_estimate=None
                if q['source_stack'] or 'fs:' in code.at(site).op_str:abi_stores.append(q)
                else:writes.append(dict(instruction=site,address=p,before=q['before_hex'],written=q['written_hex']))
        # Stack ABI is excluded from the raw-state contract. Native temporaries
        # have real writable backing; no finished contact/pose is copied in.
        memory={p:b for p,b in memory.items() if not 0x2000000<=p<0x2200000}
        for p,n in ((0x465030,4),(0x465048,4),(0x465044,4),(0x46502c,4),(0x41b760,16),
            (0x487674,8),(0x41b870,36),(0x41b8f0,36),(0x418c00,0x30)):add(memory,p,image.read(p,n) if p!=0x487674 else bytes.fromhex(next(q['raw_hex']for q in r['states'][c['before_state']]['globals']if q['address']=='00487674')))
        after=dict(memory)
        for q in writes:
            p=q['address'];raw=bytes.fromhex(q['written'])
            assert bytes(after[p+i] for i in range(len(raw)))==bytes.fromhex(q['before'])
            for i,b in enumerate(raw):after[p+i]=b
        if not boundary:
            for p,b in full_snapshot(r['states'][c['after_state']]):
                for i,value in enumerate(b):
                    if p+i in after:assert after[p+i]==value,(entry,hex(p+i))
        b=None
        if boundary:
            argc={'0031db70':2,'00300870':4,'00300400':0,'00313310':0}[boundary_entry]
            b=dict(instruction=int(boundary['call_site'],16),entry=int(boundary_entry,16),ecx=int(boundary['registers']['ecx'],16),
                entry_esp=int(boundary['entry_esp'],16),arguments=[int(q,16)for q in boundary['stack16'][:argc]])
        cases.append(dict(name='natural_lockout_'+entry+'_'+str(boundary_entry),entry=int(entry,16),receiver=int(c['registers']['ecx'],16),
            connect_current_broadphase=a.connect_current_broadphase,
            connect_contact_dispatch=boundary_entry=='00300400',
            args=[int(c['stack16'][0],16)],entry_esp=int(c['entry_esp'],16),iterations=1,
            fp=[int(c['x87_control_word'],16),int(c['mxcsr'],16)],
            stack_inputs=[[int(q['pointer'],16),q['before_hex']]for q in c['packets']
                if 0x2000000<=int(q['pointer'],16)<0x2200000],
            banks=[[p,b.hex()]for p,b in segments(memory)],after_banks=[[p,b.hex()]for p,b in segments(after)],
            writes=writes,estimates=estimates,boundary=b))
        cases[-1]['excluded_abi_stores']=len(abi_stores)
    out['cases']+=cases;out['natural_source_sha256']=sha(SOURCE);out['natural_source_manifest_sha256']=sha(manifest)
    out['exporter_sha256']=sha(__file__)
    out['connected_current_broadphase']=a.connect_current_broadphase
    out['connected_contact_dispatch']=a.connect_contact_dispatch
    p=a.output/'fixture.json';p.write_text(json.dumps(out,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(natural_calls=len(cases),complete=sum(q['boundary'] is None for q in cases),
        ordered_persistent_writes=sum(len(q['writes'])for q in cases),fixture_sha256=sha(p),
        source_report_sha256=sha(SOURCE),native_expected_data_used=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
