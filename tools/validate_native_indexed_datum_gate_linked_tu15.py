#!/usr/bin/env python3
"""Compare actual linked 3EB10 exports with their independently forced reference."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from oracle_smg_firing_1_1 import Xbe
from validate_native_indexed_datum_gate_frame_tu15 import MAPPACK_SHA, PROFILE, corpus

OWNERS = {
    'primary': ('src/physics/physics_live_support_tu15_generated.cpp', 'tu15'),
    'particle-frame': ('src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp', 'tu15'),
    'death-startup': ('src/gameplay/units/death_startup_runtime_generated.cpp', 'map-pack11'),
}


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file,'sha256').hexdigest()


def compare_accounting(path, rows, owner, reference):
    data=json.loads(path.read_bytes())
    if any(data.get(key,0) for key in ('omitted_records','active_scopes','registration_failures','orphan_literal_events')):
        raise ValueError('Invalid linked 3EB10 accounting state')
    unit,image=OWNERS[owner]
    matched=[]
    for values in data['rows']:
        row=dict(zip(data['row_fields'],values,strict=True))
        if row['symbol']=='3eb10' and data['units'][row['unit']][0]==unit and data['image_names'][row['image']]==image:
            matched.append(row)
    count=len(rows);stops=sum(row['stopped'] for row in rows)
    if len(matched)!=1 or not stops or count!=297:
        raise ValueError('Missing or incomplete actual linked 3EB10 accounting for '+owner)
    row=matched[0]
    if (row['entries'],row['returns'],row['unwinds'],row['literal_sites'])!=(count,count-stops,stops,25):
        raise ValueError('Unexpected actual linked 3EB10 entries or exits for '+owner)
    retained=(row['body_entries'],row['literal_statements'])
    if (reference and retained[0]!=count) or (reference and retained[1]<=count) or (not reference and any(retained)):
        raise ValueError('Wrong actual retained statements for '+owner)
    return row


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('binary','xbe','mappack','report'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--accounting',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not args.xbe.is_file() or not args.mappack.is_file():
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    result=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],modes={})
    try:
        if sha(args.xbe)!=PROFILE.image_sha256 or sha(args.mappack)!=MAPPACK_SHA:
            raise ValueError('Wrong 3EB10 original authority')
        fixture=scratch/'indexed-datum-linked.json'
        fixture.write_text(json.dumps(dict(tu15=corpus(Xbe(args.xbe)),mappack11=corpus(Xbe(args.mappack),True)),
                                      separators=(',',':')))
        baseline=None
        for mode,reference in (('default',False),('own-reference',True)):
            env=os.environ.copy()
            env.pop('HALO2_REFERENCE_INDEXED_DATUM_GATE',None)
            env.pop('HALO2_NATIVE_EXECUTION_REPORT',None)
            if reference:
                env['HALO2_REFERENCE_INDEXED_DATUM_GATE']='1'
            counter=scratch/(mode+'.accounting.json')
            if args.accounting:
                env['HALO2_NATIVE_EXECUTION_REPORT']=str(counter)
            command=[args.binary.resolve(),fixture]
            completed=subprocess.run(command,capture_output=True,env=env,timeout=120)
            if completed.returncode:
                raise ValueError('Actual linked 3EB10 failed: '+completed.stderr.decode(errors='replace')[-8000:])
            rows=json.loads(completed.stdout)
            if baseline is None:
                baseline=rows
            elif baseline!=rows:
                raise ValueError('Native and original retained linked replies differ')
            summary={}
            for owner in OWNERS:
                selected=[row for row in rows if row['owner']==owner]
                sites={pc for row in selected for pc in row['callbacks']}
                if len(sites)!=25 or len(selected)!=297 or not any(row['stopped'] for row in selected):
                    raise ValueError('Actual linked 3EB10 coverage incomplete for '+owner)
                summary[owner]=dict(returns=sum(not row['stopped'] for row in selected),
                                    stops=sum(row['stopped'] for row in selected),sites=len(sites))
                if args.accounting:
                    summary[owner]['accounting']=compare_accounting(counter,selected,owner,reference)
            result['modes'][mode]=dict(packet_sha256=hashlib.sha256(completed.stdout).hexdigest(),owners=summary)
        result.update(passed=True,fixture_sha256=sha(fixture),linked_binary_sha256=sha(args.binary),
                      accounting_checked=args.accounting)
    except Exception as error:
        result['error']=repr(error)
        raise
    finally:
        args.report.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
