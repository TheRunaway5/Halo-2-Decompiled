#!/usr/bin/env python3
"""Export exact listener controls/stores after checking complete original output hashes."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_mp_title_update_display import sha, save, encoded


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();report=json.loads(args.report.read_text())
    rows=['# Original full listener outputs verified; explicit raw controls are not live admission.']
    count=stores=full_bytes=0;outcomes={}
    for image in report['images']:
        tag=image['image'];constants=image['constants']
        rows.append('\t'.join(['state',tag,image['driver_pointer'],image['input_address'],
                               constants['orientation_epsilon'],constants['environment_epsilon'],constants['default72_hex']]))
        initial={int(q['address'],16):bytearray.fromhex(q['raw_hex']) for q in image['regions']}
        for address,raw in sorted(initial.items()):rows.append('\t'.join(['region',tag,f'{address:08x}',raw.hex()]))
        for case in image['cases']:
            name=case['name'];regions={a:bytearray(b)for a,b in initial.items()}
            rows.append('\t'.join(['case',name,tag,case['outcome']]))
            def apply(address,extent,before,after):
                owner=next(a for a,b in regions.items()if a<=address and address+extent<=a+len(b))
                raw=regions[owner];offset=address-owner
                assert raw[offset:offset+extent]==before,(name,hex(address),'before')
                raw[offset:offset+extent]=after
            for control in case['controls']:
                extent=control['bytes'];before=int(control['before'],16);after=int(control['after'],16)
                apply(int(control['address'],16),extent,before.to_bytes(extent,'little'),after.to_bytes(extent,'little'))
                rows.append('\t'.join(['control',name,control['address'],str(extent),f'{before:08x}',f'{after:08x}']))
            for store in case['stores']:
                before=bytes.fromhex(store['before']);after=bytes.fromhex(store['after'])
                apply(int(store['address'],16),store['bytes'],before,after)
                rows.append('\t'.join(['store',name,store['instruction'],store['address'],str(store['bytes']),
                                       f'{int.from_bytes(before,"little"):08x}',f'{int.from_bytes(after,"little"):08x}']))
                stores+=1
            for request in case['requests']:
                rows.append('\t'.join(['request',name,request['operation'],request['device'],*request['words']]))
            for invalid in case['invalid']:
                rows.append('\t'.join(['invalid',name,invalid['address'],str(invalid['bytes'])]))
            for output in case['outputs']:
                raw=regions[int(output['address'],16)]
                assert len(raw)==output['bytes'] and hashlib.sha256(raw).hexdigest()==output['sha256']
                full_bytes+=len(raw)
            count+=1;outcomes[case['outcome']]=outcomes.get(case['outcome'],0)+1
    assert count==128 and stores==2286
    save(args.output,('\n'.join(rows)+'\n').encode())
    metadata=dict(schema='sound-driver-listener-fixtures-v1',exporter_sha256=sha(__file__),
                  source_report_sha256=sha(args.report),fixture_sha256=sha(args.output),cases=count,
                  stores=stores,complete_output_bytes=full_bytes,outcomes=outcomes,
                  complete_original_output_hashes_checked=True,source_platform_returns=[])
    save(args.output.with_suffix('.json'),encoded(metadata));print(json.dumps(metadata))


if __name__=='__main__':main()
