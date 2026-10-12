#!/usr/bin/env python3
"""Export observed original entity/property/current-bounds/delivery records.

No native output is used. Platform exception-chain and internal call-stack
writes are excluded; actual descriptor outputs are retained even on stack.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_mp_power_body_bridge_1_1 import ROOT,OUT,verify
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments

REPORT_PIN='6414ec9078212075c2fb0ea29d582688f00fd0404eb2ee5f06826679010ab05c'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def snapshot(state):
    return [(int(x['pointer'],16),bytes.fromhex(x['raw_hex']))for x in state['raw_backing']]+[
        (int(x['address'],16),bytes.fromhex(x['raw_hex']))for x in state['globals']]
def native_write(c,e):
    if c['entry']=='00317890':
        a=int(e['address'],16);p=int(c['registers']['ecx'],16)
        return p<=a and a+e['size']<=p+0xb8
    if c['entry']in ('002e7610','002e65a0'):
        a=int(e['address'],16);p=int(c['output32_pointer'],16)
        return p<=a and a+e['size']<=p+32
    return not e['source_stack'] and e['instruction']not in ('00318afe','00318b7d')
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/exported-owner');args=ap.parse_args()
    verify();p=OUT/'final/measured/report.json';repeat=OUT/'final/repeat/report.json'
    assert sha(p)==REPORT_PIN
    if repeat.exists():assert sha(repeat)==REPORT_PIN
    r=json.loads(p.read_text())
    life=json.loads((ROOT/'analysis/multiplayer/sandbox-source/contact-lifecycle/final/measured/report.json').read_text())
    assert sha(ROOT/'analysis/multiplayer/sandbox-source/contact-lifecycle/final/measured/report.json')=='68221efe9154aa5c958cf30a8847576d1195abfada5df54c26736884f13dd8b2'
    delivered=[q for q in life['calls']if q['entry']=='001d2f10'and q['caller']=='001d4212'];assert len(delivered)==1
    calls=[c for c in r['calls']if c['entry']in
        ('00317890','00312410','00318af0','00318a20','001d4160','002e7610','002e65a0')]
    properties=[c for c in r['calls']if c['entry']=='00318a90'and c['registers']['ecx']=='0601a480'];calls+=properties
    for c in r['calls']:
        if c['entry']in ('002df780','002df360','0022eb20','0022e950')and any(q['order']<c['order']<q['return_order']for q in properties):calls.append(c)
    calls.sort(key=lambda c:c['order']);rows=[];audit=[]
    for c in calls:
        memory={}
        for a,b in snapshot(r['states'][c['before_state']]):add(memory,a,b)
        for key,n in (('info_b8_hex',0xb8),('context14_hex',0x14),('descriptor0c_hex',12),('manager_rows_hex',None)):
            if key not in c:continue
            pointer={'info_b8_hex':'info_pointer','context14_hex':'context_pointer','descriptor0c_hex':'descriptor_pointer','manager_rows_hex':'manager_rows_pointer'}[key]
            b=bytes.fromhex(c[key]);assert n is None or len(b)==n
            if b:add(memory,int(c[pointer],16),b)
        current=dict(memory);native=[];ignored=[]
        events=sorted([('R',x)for x in r['reads']if c['order']<x['order']<c['return_order']]+
            [('W',x)for x in r['stores']if c['order']<x['order']<c['return_order']],key=lambda x:x[1]['order'])
        for kind,e in events:
            a=int(e['address'],16);raw=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
            for n,v in enumerate(raw):
                if a+n not in current:memory[a+n]=v;current[a+n]=v
                assert current[a+n]==v,(c['id'],kind,e)
            if kind=='W':
                (native if native_write(c,e)else ignored).append(e)
                for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[a+n]=v
        after=dict(memory)
        for e in native:
            a=int(e['address'],16)
            assert bytes(after[a+n]for n in range(e['size']))==bytes.fromhex(e['before_hex']),(c['id'],e)
            for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[a+n]=v
        known=0
        for a,b in snapshot(r['states'][c['after_state']]):
            for n,v in enumerate(b):
                if a+n in after:assert after[a+n]==v,(c['id'],f'{a+n:08x}');known+=1
        scratch=0
        if c['entry']=='001d4160':
            reads=[x for x in r['reads']if c['order']<x['order']<c['return_order']and x['instruction']=='001d41fb']
            assert len(reads)<=1
            if reads:scratch=struct.unpack('<I',bytes.fromhex(reads[0]['raw_hex']))[0]
        context=c.get('context_pointer','00000000')
        rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax'],
            c['x87_control_word'],c['mxcsr'],context,f'{scratch:08x}']+c['stack16']))
        for a,b in segments(memory):rows.append(f'M {a:08x} {b.hex()}')
        for a,b in segments(after):rows.append(f'A {a:08x} {b.hex()}')
        for e in native:rows.append(f'W {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
        if c['entry']=='001d4160'and scratch:
            q=delivered[0]
            manager=struct.unpack_from('<I',bytes.fromhex(c['context14_hex']))[0]
            assert q['stack16'][1]==f'{scratch:08x}'and int(q['registers']['eax'],16)==manager
            actual=[(x['instruction'],x['address'],x['before_hex'],x['written_hex'])for x in native]
            previous=[(x['instruction'],x['address'],x['before_hex'],x['written_hex'])for x in life['stores']if q['order']<x['order']<q['return_order']]
            assert actual==previous
            rows.append(' '.join(['D',q['registers']['eax'],q['registers']['ecx']]+q['stack16'][:4]))
        audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),output_bytes=len(after),
            full_retained_snapshot_correlated_bytes=known,ordered_stores=len(native),excluded_platform_or_internal_stack_stores=len(ignored),
            flip_scratch_upper24_from_original_read=bool(scratch),descriptor_padding_unobserved=c['entry']=='00317890'))
    fixture=args.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode())
    result=dict(schema=1,exporter_sha256=sha(__file__),source_report_sha256=REPORT_PIN,fixture_sha256=sha(fixture),cases=audit,
        native_expected_values_used=False,source_fields_or_results_supplied=False,
        boundaries=['Original call-stack and FS exception-chain plumbing is excluded from the portable owner; every actual retained memory write is compared.',
            'Descriptor317890 output includes all observed owner stores. Untouched padding in that stack descriptor was not captured whole and remains unobserved.',
            'Manager delivery uses the actual consumed flip-word upper24; only low8 has source semantic ownership. Empty manager consumes no scratch word.',
            'No motion constructor, shape query, solver admission, repeated clock, damage/destruction or live player/crate contact is fabricated.'])
    frozen_write(args.output/'report.json',encoded(result));print(json.dumps(dict(cases=len(calls),stores=sum(c['ordered_stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
