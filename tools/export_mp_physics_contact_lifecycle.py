#!/usr/bin/env python3
"""Export literal original contact owner states/stores; no native expectations."""
import argparse, hashlib, json, struct
from pathlib import Path
from oracle_mp_physics_contact_lifecycle_1_1 import ROOT, OUT, verify
from oracle_mp_item_current_producers_1_1 import encoded, frozen_write
from audit_mp_power_module_schedule_1_1 import buffers
from oracle_smg_firing_1_1 import Xbe, PIN

REPORT_PIN='68221efe9154aa5c958cf30a8847576d1195abfada5df54c26736884f13dd8b2'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def add(memory,address,raw):
    for n,v in enumerate(raw):
        at=address+n
        if at in memory:assert memory[at]==v,(f'{at:08x}',memory[at],v)
        memory[at]=v
def snapshot(s):
    raw=bytes.fromhex(s['raw_components']['raw_hex']);base=struct.unpack_from('<I',raw,0x44)[0]
    result=buffers(s['source'],base)
    for name in ('raw_havok','raw_components'):
        b=s[name];result.append((int(b['pointer'],16),bytes.fromhex(b['raw_hex'])))
    result += [(int(b['address'],16),bytes.fromhex(b['raw_hex']))for b in s['retained_globals']]
    return result
def segments(memory):
    result=[];at=None;raw=bytearray()
    for p in sorted(memory):
        if at is not None and p!=at+len(raw):result.append((at,bytes(raw)));raw=bytearray();at=None
        if at is None:at=p
        raw.append(memory[p])
    if at is not None:result.append((at,bytes(raw)))
    return result
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/exported-math');args=ap.parse_args()
    verify();source=OUT/'final/measured';repeat=OUT/'final/repeat'
    assert sha(source/'report.json')==sha(repeat/'report.json')==REPORT_PIN
    r=json.loads((source/'report.json').read_text())
    imagepath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(imagepath)==PIN
    image=Xbe(imagepath);sites=set()
    for l in r['original_listings']:
        p=source/l['file'];assert sha(p)==sha(repeat/l['file'])==l['sha256']
        for line in p.read_text().splitlines():
            a,h,*_=line.split();a=int(a,16);b=bytes.fromhex(h);assert image.read(a,len(b))==b;sites.add(a)
    # Constructor and adapter bodies are already immutable original listings.
    for entry in ('001d2df0','00148a10'):
        p=ROOT/'analysis/multiplayer/sandbox-source/power-module-schedule/frozen/measured/instructions'/f'{entry}.asm'
        for line in p.read_text().splitlines():
            a,h,*_=line.split();a=int(a,16);b=bytes.fromhex(h);assert image.read(a,len(b))==b;sites.add(a)
    calls=[c for c in r['calls']if c['entry'] in ('001d3df0','001d2f10','001d3fc0','001d40b0')]
    bank=calls[0]
    calls += [c for c in r['calls']if bank['order']<c['order']<bank['return_order']and c['entry']in ('002df710','0022e950')]
    calls.sort(key=lambda c:c['order']);rows=[];audit=[]
    for c in calls:
        memory={}
        for a,b in snapshot(r['states'][c['before_state']]):add(memory,a,b)
        if 'callback_packet_pointer'in c:add(memory,int(c['callback_packet_pointer'],16),bytes.fromhex(c['callback_consumed_index_hex']))
        current=dict(memory);events=sorted([('R',x)for x in r['reads']if c['order']<x['order']<c['return_order']]+
            [('W',x)for x in r['stores']if c['order']<x['order']<c['return_order']],key=lambda x:x[1]['order'])
        writes=[]
        for kind,e in events:
            a=int(e['address'],16);raw=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
            for n,v in enumerate(raw):
                if a+n not in current:memory[a+n]=v;current[a+n]=v
                assert current[a+n]==v,(c['id'],kind,e)
            if kind=='W':
                assert int(e['instruction'],16)in sites
                writes.append(e)
                for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[a+n]=v
        known=0
        for a,b in snapshot(r['states'][c['after_state']]):
            for n,v in enumerate(b):
                if a+n in current:assert current[a+n]==v,(c['id'],f'{a+n:08x}');known+=1
        math=[m for m in r['math']if c['order']<m['order']<c['return_order']]
        cw,mx=('0000','00000000')if not math else(math[0]['x87_control_word'],math[0]['mxcsr'])
        label=f'call_{c["id"]}_{c["entry"]}'
        rows.append(' '.join(['C',label,c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax'],cw,mx]+c['stack16']))
        for a,b in segments(memory):rows.append(f'M {a:08x} {b.hex()}')
        for a,b in segments(current):rows.append(f'A {a:08x} {b.hex()}')
        for w in writes:rows.append(f'W {w["instruction"]} {w["address"]} {w["before_hex"]} {w["written_hex"]}')
        if c['entry']=='001d2f10':
            for m in math:
                site=m['instruction']
                if site in ('001d2ff8','001d300e','001d3029','001d3037'):
                    words=[m['xmm'][str(n)][-8:]for n in (0,1)]
                elif site=='001d30a8':words=[m['xmm']['0'][-8:]]
                elif site=='001d30b6':words=[f'{struct.unpack("<I",bytes.fromhex(m["local18_hex"]))[0]:08x}']
                else:continue
                rows.append(' '.join(['F',site]+words))
            for d in r['calls']:
                if not c['order']<d['order']<c['return_order']or d['entry']not in ('0031f4b0','0031f4d0','0009d900'):continue
                site=int(d['caller'],16)-3
                assert image.read(site,2)[0]==0xff and site in sites
                a=[]if d['entry']=='0009d900'else d['stack16'][:1]
                rows.append(' '.join(['V',f'{site:08x}',d['entry'],d['registers']['ecx']]+a))
        audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),source_output_bytes=len(current),
            full_snapshot_correlated_bytes=known,ordered_stores=len(writes),source_math_samples=len(math)))
    for c in r['calls']:
        if c['entry']not in ('0031f4b0','0031f4d0'):continue
        memory={};add(memory,int(c['manager_pointer'],16),bytes.fromhex(c['manager120_hex']))
        for b in c['banks']:
            if b['raw_capacity_hex']:add(memory,int(b['pointer'],16),bytes.fromhex(b['raw_capacity_hex']))
        rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax'],'0000','00000000']+c['stack16']))
        for tag in ('M','A'):
            for a,b in segments(memory):rows.append(f'{tag} {a:08x} {b.hex()}')
        emitted=bytes.fromhex(c['emitted_full_record_hex']);p=int(c['emitted_pointer'],16)
        assert bytes(memory[p+n]for n in range(len(emitted)))==emitted
        audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),source_output_bytes=len(memory),
            emitted_record_bytes=len(emitted),ordered_stores=0))
    assert len(audit)==11 and sum(c['ordered_stores']for c in audit)==90
    args.output.mkdir(parents=True,exist_ok=True);fixture=args.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode())
    result=dict(schema=1,exporter_sha256=sha(__file__),source_report_sha256=REPORT_PIN,prior_artifacts_verified=verify(),
        independent_original_repeat_byte_identical=True,source_instruction_sites_verified=len(sites),cases=audit,
        fixture_sha256=sha(fixture),native_expected_values_used=False,
        exact_removal_consumption=r['removal_packet_consumption'],
        limits=['Only unchanged source-created original1.1 current state is exported. Full1.5 lifecycle execution is not claimed.',
            'Expected buffers are actual original snapshots/reads/ordered stores, independently correlated to complete original exit snapshots; no native kernel produces expectations.',
            'The source same-frame contact is created by the actual world schedule. No new contact, timing, pose, damage or body identity is supplied.'])
    frozen_write(args.output/'report.json',encoded(result));print(json.dumps(dict(cases=len(audit),stores=90,fixture=sha(fixture),report=sha(args.output/'report.json'))))
if __name__=='__main__':main()
