#!/usr/bin/env python3
"""Export actual variant input/output and ordered resource/RNG boundaries."""
import argparse
import hashlib
import json
import struct
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    report = json.loads(args.report.read_text())
    lines = ['# original-animation-variant-v1',
             '# source-report-sha256 '+hashlib.sha256(args.report.read_bytes()).hexdigest(),
             '# Resource callbacks replay only complete actual original leaf boundaries; no live admission default.']
    for graph in report['graphs']:
        lines.append('GRAPH '+graph['datum']+' '+graph['address'])
        for region in graph['regions']:
            lines.append('MEM '+region['address']+' '+region['raw_hex'])
    lines.append('CONSTANTS '+' '.join(report['constants'][name] for name in ('00465030','004650c0','00465028')))
    count, resources, stores = 0, 0, 0
    for call in report['calls']:
        if call['entry'] not in ('001ceda0','001e1020') or 'return_order' not in call:
            continue
        stack = call['stack']
        if call['entry'] == '001ceda0':
            kind='WRAPPER';graph=call['graph_datum'];packed=stack[0];selector='0'
        else:
            kind='CORE';graph=call['graph_address'];packed=stack[2];selector=stack[3]
        before=struct.unpack('<2I',bytes.fromhex(call['rng_before']['raw8_hex']))
        after=struct.unpack('<2I',bytes.fromhex(call['rng_after']['raw8_hex']))
        selected=int.from_bytes(bytes.fromhex(call['output_after_hex']),'little')
        lines.append('CASE '+kind+' '+str(call['id'])+' '+graph+' '+packed+' '+selector+' '+
                     ' '.join(f'{x:08x}' for x in (*before,*after,selected)))
        if kind == 'WRAPPER':
            lines.append('MANAGER '+call['manager_before_hex'])
            assert call['manager_after_hex']==call['manager_before_hex']
        ordered=[]
        for child in report['calls']:
            if child['entry']=='001e13c0' and call['id'] in child['ancestors']:
                assert 'return_order' in child
                ordered.append((child['order'],'RESOURCE '+child['graph_address']+' '+child['stack'][0]))
                resources+=1
        for store in report['stores']:
            if store['instruction'] not in ('001e10d1','001e10ef') or call['id'] not in store['calls']:
                continue
            stream=0 if store['instruction']=='001e10d1' else 1
            assert int(store['address'],16)==int(call['rng_before']['pointer'],16)+stream*4
            old=int.from_bytes(bytes.fromhex(store['before_hex']),'little')
            new=int.from_bytes(bytes.fromhex(store['written_hex']),'little')
            ordered.append((store['order'],f'RNG {store["instruction"]} {stream:x} {old:08x} {new:08x}'))
            stores+=1
        for _,line in sorted(ordered):lines.append(line)
        lines.append('ENDCASE');count+=1
    raw=('\n'.join(lines)+'\n').encode()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if args.output.exists():assert args.output.read_bytes()==raw,'Immutable fixture differs'
    else:args.output.write_bytes(raw)
    print(json.dumps(dict(cases=count,ordered_resource_calls=resources,ordered_rng_stores=stores,
                          fixture_sha256=hashlib.sha256(raw).hexdigest())))


if __name__=='__main__':main()
