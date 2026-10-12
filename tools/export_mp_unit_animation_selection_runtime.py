#!/usr/bin/env python3
"""Export original complete selection calls from the frozen natural death run.

All graph/secondary/fallback/variant/resource-demand and raw allocator bodies
run natively, without captured lower effects. The original run reaches no
cache IO or platform clock owner inside these calls. All stores, including
source stack packets and saved register cells, are compared. Original call
registers are inputs, not supplied animation/physics outcomes.
"""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import spans,sha
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
ROOTS={0x1cfde0,0x1d0790,0x1ceda0,0x1d1b70,0x111b60,0xbb240,0x114bd0}
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    j=json.loads(a.report.read_text())
    assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
    assert j['capture_entry']=='000de960' and j['failure']is None and j['frozen_prior_byte_identical']
    assert not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
    assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_biped_prephysics_1_1.py'))
    for q in j['instructions']:assert sha(a.report.parent/q['file'])==q['sha256']
    events=sorted([(q['order'],'R',q)for q in j['rebuild_reads']]+[(q['order'],'W',q)for q in j['rebuild_stores']]);orders=[q[0]for q in events]
    rows=[];audit=[]
    for c in j['rebuild_calls']:
        if int(c['entry'],16)not in ROOTS:continue
        ev=events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
        nested=[q for q in j['rebuild_calls']if c['order']<q['order']<c['return_order']]
        assert not any(int(q['entry'],16)in {0x125120,0x337a40}for q in nested)
        initial={};current={};writes=[]
        for _,kind,q in ev:
            at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
            for i,v in enumerate(raw):
                if at+i not in current:initial[at+i]=current[at+i]=v
                assert current[at+i]==v
            if kind=='W':
                writes.append(q)
                for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
        gp=[c['registers'][k]if k!='esp'else c['entry_esp']for k in ('eax','ecx','edx','ebx','esp','ebp','esi','edi')]
        rows.append(' '.join(['C',str(c['id']),c['entry'],c['fpcw'],c['mxcsr'],c['returned_eax'],str(c['return_esp_delta']),*gp]))
        for i in range(8):
            x=int(c['xmm'][str(i)],16)
            rows.append('X '+str(i)+' '+' '.join(f'{(x>>(32*n))&0xffffffff:08x}'for n in range(4)))
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        output={}
        for q in writes:
            rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
            for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
        for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
        audit.append(dict(id=c['id'],entry=c['entry'],stores=len(writes),bytes=len(output),captured_dependency_results=[]))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
    a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
