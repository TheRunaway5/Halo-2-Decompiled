#!/usr/bin/env python3
"""Replay the original C9070→1046C0→107140 return through real World linkage."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess

from validate_native_object_function_query_public_tu15 import (
    COAGULATION, INHERITED_ORIGINAL_PACKET, PIN, PRIMARY, Xbe, corpus, digest, verify_calls)

OWNERS=(('c9070',144,'HALO2_REFERENCE_OBJECT_FUNCTION_QUERY'),
        ('1046c0',406,'HALO2_REFERENCE_OBJECT_RATIO_CALLER'),
        ('107140',36,'HALO2_REFERENCE_OBJECT_RECORD_RATIO'))


def accounting(path,forced):
    receipt=json.loads(path.read_bytes())
    if receipt['native_closure_proved'] or any(receipt[key] for key in
        ('omitted_records','active_scopes','registration_failures','orphan_literal_events')):
        raise ValueError('Invalid inherited execution accounting packet')
    names={name for name,_,_ in OWNERS}
    records={}
    for values in receipt['rows']:
        row=dict(zip(receipt['row_fields'],values,strict=True))
        symbol=row['symbol']
        if (receipt['units'][row['unit']][0]==PRIMARY and receipt['image_names'][row['image']]=='tu15'
            and symbol in names):
            if symbol in records:raise ValueError('Duplicate accounting row: '+symbol)
            records[symbol]=row
    for name,sites,key in OWNERS:
        row=records[name]
        if (row['literal_sites'],row['entries'],row['returns'],row['unwinds'])!=(sites,1,1,0):
            raise ValueError('Inherited owner did not return exactly once: '+name)
        if key in forced:
            if row['body_entries']!=1 or row['literal_statements']<=0:
                raise ValueError('Owner reference switch did not execute retained statements: '+name)
        elif row['body_entries'] or row['literal_statements'] or row['returns_without_body']!=1:
            raise ValueError('Inherited native owner executed retained statements: '+name)
    return {name:{key:row[key] for key in ('literal_sites','entries','returns','body_entries','literal_statements')}
            for name,row in records.items()}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('binary','map','xbe','report'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--no-accounting',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if not args.xbe.is_file() or not args.map.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    proof=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],routes={})
    try:
        if digest(args.xbe)!=PIN or digest(args.map)!=COAGULATION:
            raise ValueError('Wrong unchanged English TU1.5 or Coagulation')
        image=Xbe(args.xbe);verify_calls(image)
        rows=[row for row in corpus(image) if row['name']=='linked-records-5']
        if len(rows)!=1:raise ValueError('Missing genuine inherited fixture')
        fixture=scratch/'inherited.json'
        fixture.write_text(json.dumps(rows,separators=(',',':')))
        proof.update(fixture_sha256=digest(fixture),linked_binary_sha256=digest(args.binary))
        base={key:value for key,value in os.environ.items() if not key.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        for bits in itertools.product((0,1),repeat=len(OWNERS)):
            label='inherited-'+''.join(map(str,bits))
            control={key:'1' for (_,_,key),forced in zip(OWNERS,bits,strict=True) if forced}
            counters=scratch/(label+'-accounting.json')
            env={**base,**control}
            env.pop('HALO2_NATIVE_EXECUTION_REPORT',None)
            if not args.no_accounting:env['HALO2_NATIVE_EXECUTION_REPORT']=str(counters)
            run=subprocess.run([args.binary.resolve(),args.map.resolve(),args.xbe.resolve(),scratch/label,
                                fixture,'inherited'],capture_output=True,text=True,timeout=180,env=env)
            if run.returncode:raise ValueError(label+' actual C9070 failed: '+run.stderr[-5000:])
            result=json.loads(run.stdout)
            if (result['complete'],result['stopped'],result['query_native_entries'])!=(1,0,1-bits[0]):
                raise ValueError(label+' did not complete the actual C9070 path')
            row,=result['rows'];events=[store[0] for store in row['stores']]
            if events.count(0xc92a8)!=1 or events.count(0x104815)!=1:
                raise ValueError('Missing an authentic pushed child return')
            packet=hashlib.sha256(json.dumps(row,sort_keys=True).encode()).hexdigest()
            if packet!=INHERITED_ORIGINAL_PACKET:raise ValueError(label+' differs from original packet '+packet)
            proof['routes'][label]=dict(controls=control,original_packet_sha256=packet)
            if not args.no_accounting:proof['routes'][label]['counters']=accounting(counters,control)
        proof.update(passed=True,actual_live_world=True,actual_inherited_107140=True,whole_runtime_proved=False)
    except BaseException as error:
        proof['error']=repr(error);raise
    finally:
        args.report.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
