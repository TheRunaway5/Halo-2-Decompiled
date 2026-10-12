#!/usr/bin/env python3
"""Export actual110BF0 requests from the original natural death phases.

The complete installation caller,114F00 overlay delivery and114F70 duration
arbitration run natively. Graph selection, availability, local transition and
pose request producers remain explicit test-only lower boundaries. Captured
effects never serve as runtime animation providers.
"""
import argparse,bisect,json
from export_mp_ragdoll_owners import spans,sha
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
from pathlib import Path
NATIVE={0x110bf0,0x114f00,0x114f70,0x1cf100,0x1cf070,0x1ca2b0,0x1ca090,0x1c9f50,0x1ca3a0,0x1ca5a0,0x1c9e30,0x1de890,0x1cae40,0x1e13c0,0x1e1230,0x1de9b0,0x1150d0,0x1dcd10,0x10f430,0x1102b0,0x1105e0,0xe7e30,0x1d0a60,0x1d0590,0x1cee00,0x1c9e60,0x1cacf0,0xb7650,0x1d06c0}
NATIVE |= {0x1d0790,0x1de7d0,0x1cfde0,0x1cfce0,0x1deb60,0x1df670,0x1dea60,0x1e02b0,0x29c6a0,0x1e0f50,0x1deb10,0x1e16f0,0x1e1660,0x1e1580,0x1ceda0,0x1e1020,0x1e0fc0,0x1de890,0x1de9b0,0x1e13c0,0x1e1230,0x124e40,0x124dd0,0x16d2c0,0x1d1b70}

NATIVE |= {0xb8250,0xbb240,0xc83d0,0x111b60,0x114bd0,0x115af0,0x1c9e60,0x1c9f50,0x1ca3a0,0x1cee00,0x1cef10,0x1cefe0,0x1cf360,0x1cf3c0,0x1cf4c0,0x1cfb90,0x1dcc30,0x1dce20,0x1cec20,0x1cfa90,0x1c9de0}

NATIVE |= {0x110b00,0x1cf310,0x1d1bd0}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    j=json.loads(a.report.read_text());assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
    assert j['capture_entry']=='000de960'and j['failure']is None and j['frozen_prior_byte_identical']
    assert not j['instruction_patches']and not j['new_supplied_guest_fields_or_results']
    assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_biped_prephysics_1_1.py'))
    for q in j['instructions']:assert sha(a.report.parent/q['file'])==q['sha256']
    events=sorted([(q['order'],'R',q)for q in j['rebuild_reads']]+[(q['order'],'W',q)for q in j['rebuild_stores']]);orders=[q[0]for q in events]
    within=lambda c:events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
    rows=[];audit=[]
    native=NATIVE
    for c in j['rebuild_calls']:
        if c['entry']!='00110bf0':continue
        frame=int(c['entry_esp'],16)-0x14
        def retained(q):return not 0x2000000<=int(q['address'],16)<0x2200000 and q['instruction']not in {'00278ce1','00278ce8'}
        children=[];end=0
        for q in j['rebuild_calls']:
            if not c['order']<q['order']<c['return_order']or q['order']<end:continue
            if int(q['entry'],16)in native:
                if int(q['entry'],16)not in {0x114f00,0x1d0a60,0x1cee00,0x1cacf0,0x1c9f50,0x1c9e30,0x1de890,0x1e13c0,0x1d06c0,0x1d0790,0x1de7d0,0x1cfde0,0x1cfce0,0x1deb60,0x1df670,0x1dea60,0x1e02b0,0x29c6a0,0x1e0f50,0x1deb10,0x1e16f0,0x1e1660,0x1e1580,0x1ceda0,0x1e1020,0x1e0fc0,0x1de890,0x1de9b0,0x1e13c0,0x1e1230,0x124e40,0x124dd0,0x16d2c0,0x1d1b70,0xb8250,0xbb240,0xc83d0,0x111b60,0x114bd0,0x115af0,0x1c9e60,0x1c9f50,0x1ca3a0,0x1cee00,0x1cef10,0x1cefe0,0x1cf360,0x1cf3c0,0x1cf4c0,0x1cfb90,0x1dcc30,0x1dce20,0x1cec20,0x1cfa90,0x1c9de0,0x110b00,0x1cf310,0x1d1bd0}:end=q['return_order']
                continue
            children.append(q);end=q['return_order']
        initial={};current={};writes=[]
        for _,kind,q in within(c):
            at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
            for i,v in enumerate(raw):
                if at+i not in current:initial[at+i]=current[at+i]=v
                assert current[at+i]==v
            if kind=='W':
                if retained(q):writes.append(q)
                for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
        rows.append(' '.join(['C',str(c['id']),*c['stack16'][:7],c['registers']['eax'],c['entry_esp'],c['fpcw'],c['mxcsr'],c['returned_eax']]))
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        output={}
        for q in writes:
            for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
        for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
        for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
        for q in children:
            rows.append(' '.join(['P',q['entry'],q['entry_esp'],q['returned_eax']]+[q['registers'][k]for k in ('eax','ecx','edx','esi','edi','ebx')]+q['stack16']))
            locals=[(frame+0x10,frame+0x14)]
            for parent_call in j['rebuild_calls']:
                if parent_call['order']<q['order']<parent_call['return_order']:
                    if parent_call['entry']=='001d06c0':
                        base=int(parent_call['entry_esp'],16)-0x2c
                        locals.append((base+0xc,base+0x44))
                    elif parent_call['entry']=='001d0a60':
                        base=int(parent_call['entry_esp'],16)-0x48
                        locals.append((base+0x13,base+0x60))
                    elif parent_call['entry']=='001cee00':
                        base=int(parent_call['entry_esp'],16)
                        locals.append((base+8,base+12))
                    elif parent_call['entry']=='001e8f60':
                        base=((int(parent_call['entry_esp'],16)-4)&~15)-0xa0
                        locals.append((base+0x14,base+0xa0))
                    elif parent_call['entry']=='001f0080':
                        base=((int(parent_call['entry_esp'],16)-4)&~15)-0xe0
                        locals.append((base+0x14,base+0xe0))
            for _,kind,t in within(q):
                if kind!='W':continue
                at=int(t['address'],16)
                if retained(t)or any(lo<=at and at+t['size']<=hi for lo,hi in locals):
                    rows.append('L '+t['instruction']+' '+t['address']+' '+t['before_hex']+' '+t['written_hex'])
        audit.append(dict(id=c['id'],stores=len(writes),retained_bytes=len(output),lower_owners=[q['entry']for q in children]))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
    a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_animation_request=True,native_expected_data_used=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
