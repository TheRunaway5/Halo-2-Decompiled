#!/usr/bin/env python3
"""Replay the complete original 1046C0 corpus through the actual World owner."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from oracle_smg_firing_1_1 import Xbe
from validate_native_flight_property_query_frame_tu15 import sha
from validate_native_object_ratio_caller_middle_frame_tu15 import (
    COMBAT_EDGES, COMBAT_REGIONS, ENTRY, LOW_EDGES, whole_corpus)
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN, physical_original

COAGULATION='1de37727dbb9c51aa08e03bfea86c0641ad67250dba28c5dcb3ba3dadaaf93fa'
OLD_STACK,STACK=0x62000000,0x600f0000
CONSTANTS=(0x46a0c4,0x46a0d0,0x46a178)
GLOBALS=(0x4eabc4,0x4ecbd0,0x4f0414,0x4f3218,0x51df54,0x52d710)
OWNERS=(ENTRY,0x107140,0xfc510,0x165590,0x1655f0,0x24b430)
SWITCHES=('HALO2_REFERENCE_OBJECT_RATIO_CALLER','HALO2_REFERENCE_OBJECT_RECORD_RATIO',
          'HALO2_REFERENCE_OBJECT_PARENT_MODE','HALO2_REFERENCE_OBJECT_DEFINITION_MODE_QUERY',
          'HALO2_REFERENCE_OBJECT_TABLE_RATIO','HALO2_REFERENCE_OBJECT_TABLE_RATIO_MODE')


def world_inputs(image):
    rows=whole_corpus(image)
    for row in rows:
        stack=next(bank for bank in row['banks'] if bank['address']==OLD_STACK)
        saved=bytearray.fromhex(stack['before'])
        at=row['gp'][4]-OLD_STACK
        for field in (at+12,at+16):
            pointer=int.from_bytes(saved[field:field+4],'little')
            if OLD_STACK<=pointer<OLD_STACK+len(saved):
                saved[field:field+4]=(pointer-OLD_STACK+STACK).to_bytes(4,'little')
        row['gp'][4]+=STACK-OLD_STACK
        stack.update(address=STACK,before=saved.hex())
        data=next(bank for bank in row['banks'] if bank['address']==0x46a000)
        constants=bytearray.fromhex(data['before'])
        for address in CONSTANTS:
            offset=address-data['address']
            constants[offset:offset+4]=image.read(address,4)
        data['before']=constants.hex()
    return rows


def extract(banks,address,width,key):
    for bank in banks:
        start=bank['address']
        if start<=address<start+len(bank[key])//2 and address+width<=start+len(bank[key])//2:
            at=2*(address-start)
            return bytes.fromhex(bank[key][at:at+2*width])
    raise ValueError(f'1046C0 fixture escaped its declared bank at {address:x}')


def reference(image,scratch,report):
    all_edges=LOW_EDGES|COMBAT_EDGES
    for pc,(target,ret) in all_edges.items():
        call=image.read(pc,5)
        if call[0]!=0xe8 or ret+int.from_bytes(call[1:],'little',signed=True)!=target:
            raise ValueError(f'Original 1046C0 caller changed at {pc:x}')
    rows=world_inputs(image)
    regions=[(STACK if base==OLD_STACK else base,width) for base,width in COMBAT_REGIONS]
    original,report['original_authority']=physical_original(image,rows,scratch,entries=OWNERS,regions=regions)
    counts=dict.fromkeys(all_edges,0)
    total=0; expected=[]
    for row,case in zip(rows,original['cases'],strict=True):
        if not case['completed']:raise ValueError('Original did not return: '+row['name'])
        pcs=case['executed_sites']
        for pc,(target,ret) in all_edges.items():
            if pcs.count(pc)!=pcs.count(ret) or pcs.count(pc)>pcs.count(target):
                raise ValueError('Original child did not return: '+row['name'])
            counts[pc]+=pcs.count(pc)
        read_globals={read['address'] for read in case['reads'] if read['address'] in GLOBALS}
        cells=[(address,4) for address in sorted({*CONSTANTS,*read_globals})]+[(STACK,4096),(0x61000000,16384)]
        sampled=[];changes=[]
        for address,width in cells:
            before=extract(row['banks'],address,width,'before')
            after=extract(case['banks'],address,width,'after')
            sampled.append(dict(address=address,before=before.hex()))
            changes.extend([address+i,value] for i,(old,value) in enumerate(zip(before,after,strict=True)) if old!=value)
        row['banks']=sampled
        terminal=boundary_packet(case,row,pcs[-1])
        stores=[[store[key] for key in ('instruction','address','before','written')] for store in case['writes']]
        total+=len(stores)
        expected.append(dict(name=row['name'],abi={key:terminal[key] for key in ('gp','eflags','xmm','fp')}|
            dict(x87=terminal['live_x87'],fs_base=0),changes=changes,stores=stores))
    if not all(counts.values()):raise ValueError('The World corpus missed an authentic child route')
    report.update(original_returns=len(rows),original_writes=total,authentic_child_returns={hex(pc):n for pc,n in counts.items()})
    return rows,expected



def parent_accounting(path,count,forced):
    data=json.loads(path.read_bytes())
    if data['native_closure_proved'] or any(data[key] for key in
        ('omitted_records','active_scopes','registration_failures','orphan_literal_events')):
        raise ValueError('Invalid 1046C0 accounting packet')
    matches=[]
    for values in data['rows']:
        row=dict(zip(data['row_fields'],values,strict=True))
        if (data['units'][row['unit']][0]=='src/physics/physics_live_support_tu15_generated.cpp' and
            data['image_names'][row['image']]=='tu15' and row['symbol']=='1046c0'):
            matches.append(row)
    if len(matches)!=1:raise ValueError('Expected one separate 1046C0 accounting owner')
    row=matches[0]
    if (row['literal_sites'],row['entries'],row['returns'],row['unwinds'])!=(406,count,count,0):
        raise ValueError('1046C0 accounting entry or return changed: '+str(row))
    if forced:
        if row['body_entries']!=count or row['literal_statements']<count:
            raise ValueError('Forced 1046C0 did not execute the retained body')
    elif row['body_entries'] or row['literal_statements'] or row['returns_without_body']!=count:
        raise ValueError('Native 1046C0 executed retained statements')
    return {key:row[key] for key in ('literal_sites','entries','returns','body_entries','literal_statements')}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('binary','map','xbe','report'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--accounting',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not args.xbe.is_file() or not args.map.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],routes={})
    try:
        if sha(args.xbe)!=PIN or sha(args.map)!=COAGULATION:
            raise ValueError('Expected the authenticated original English TU1.5 and Coagulation')
        report.update(binary_sha256=sha(args.binary),map_sha256=COAGULATION,xbe_sha256=PIN)
        rows,expected=reference(Xbe(args.xbe),scratch,report)
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(rows,separators=(',',':')))
        report['fixture_sha256']=sha(fixture)
        base={key:value for key,value in os.environ.items() if not key.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        modes=[('quiet-default','quiet',None),('observed-default','observed',None)]
        modes.extend((f'observed-{switch.lower()}','observed',switch) for switch in SWITCHES)
        for label,observation,switch in modes:
            mode={switch:'1'} if switch else {}
            counter=scratch/(label+'-accounting.json')
            env={**base,**mode}
            if args.accounting:env['HALO2_NATIVE_EXECUTION_REPORT']=str(counter)
            command=[args.binary.resolve(),args.map.resolve(),args.xbe.resolve(),scratch/label,fixture,observation]
            run=subprocess.run(command,env=env,capture_output=True,text=True,timeout=600)
            if run.returncode:raise ValueError(label+' actual World failed: '+run.stderr[-6000:])
            result=json.loads(run.stdout)
            entries=0 if switch==SWITCHES[0] else len(rows)
            if result['native_entries']!=entries:
                raise ValueError(f'{label} has {result["native_entries"]} entries, expected {entries}')
            for actual,wanted in zip(result['rows'],expected,strict=True):
                for key,value in wanted.items():
                    if key=='stores' and observation=='quiet':value=[]
                    if actual[key]==value:continue
                    if key in ('stores','changes'):
                        first=next(((a,b) for a,b in zip(actual[key],value) if a!=b),None)
                        raise ValueError(f'{label} {actual["name"]} {key} {len(actual[key])}/{len(value)} first={first}')
                    raise ValueError(f'{label} {actual["name"]} {key}: actual={actual[key]} expected={value}')
            report['routes'][label]=dict(compared=len(rows),native_entries=entries,control=mode,
                matched_writes=report['original_writes'] if observation=='observed' else 0)
            if args.accounting:
                report['routes'][label]['accounting']=parent_accounting(counter,len(rows),switch==SWITCHES[0])
        report.update(passed=True,actual_live_world=True)
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
