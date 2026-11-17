#!/usr/bin/env python3
"""Compare the actual linked Primary 107140 and both genuine 1046C0 routes."""
import argparse
import copy
import filecmp
import hashlib
import json
import os
from pathlib import Path
import subprocess

import oracle_native_campaign_screen_lifecycle_tu15 as oracle
from oracle_smg_firing_1_1 import Xbe
from validate_native_object_record_ratio_frame_tu15 import BANKS, ENTRY, corpus
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN, physical_original

PARENT=0x1046c0
EDGES={0x104815:0x10481a,0x104b6c:0x104b71}
STOP_PCS={0x107168,0x1071b2}
REGIONS=(*BANKS,(0x4ea000,4096))
CONTROL='HALO2_REFERENCE_OBJECT_RECORD_RATIO'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def parent_rows(image):
    rows=corpus(image)[:96]
    for n,row in enumerate(rows):
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
        banks[0x4ea000]=bytearray(4096)

        def locate(addr,width):
            for start,data in banks.items():
                if start<=addr<=start+len(data)-width:
                    return data,addr-start
            raise ValueError(f'1046C0 fixture escaped declared bank at {addr:x}')

        def put(addr,value,width=4):
            data,offset=locate(addr,width)
            data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')

        def get(addr):
            data,offset=locate(addr,4)
            return int.from_bytes(data[offset:offset+4],'little')

        datum=row['gp'][0];slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        records=0x61003300;mode=n%2
        put(0x4eabc4,0x61003a00);put(0x61003a82,0,1)
        put(definition+0x2d0,0);put(definition+0x2c8,mode+1);put(definition+0x2cc,records)
        put(definition+0x164,0x3f800000);put(body+0x180,0);put(body+0x188,0x3f800000)
        put(body+0x20c+mode*12,1 if n//2%2 else 2,1)
        put(body+0x20e+mode*12,(-2,3,17,32767)[n//2%4],2)
        put(records+mode*64+0x1c,0x40000000);put(records+0x28,0x3f000000);put(0x61003804,0x3e800000)
        sp=row['gp'][4];key=(0x1d0007ba,0x11000575)[mode]
        put(sp+4,datum);put(sp+8,key);put(sp+12,0x61003e00);put(sp+16,0x61003e10)
        row.update(entry=PARENT,name=f'1046c0-original-record-ratio-{n}',matrix=datum,
                   source=key,max_steps=250)
        row['banks']=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS]
    return rows


def fixture(image,scratch,report):
    code=oracle._Code(image,'.text')
    for pc,resume in EDGES.items():
        call=image.read(pc,5)
        if call[0]!=0xe8 or resume+int.from_bytes(call[1:],'little',signed=True)!=ENTRY:
            raise ValueError('Authentic 1046C0 local edge changed')
    original=oracle.metadata

    def metadata(image):
        result=original(image)
        for site in result['sites']:
            if code.at(site['instruction']).mnemonic in ('pop','ret'):
                site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result

    oracle.ENTRIES=(PARENT,ENTRY);oracle.metadata=metadata
    sites=metadata(image)['sites']
    if len(sites)!=442:
        raise ValueError('1046C0/107140 complete original body site count changed')
    trace=[dict(entry=pc,arguments=0,gp=True,xmm=True) for pc in sorted({ENTRY,*EDGES,*EDGES.values(),*STOP_PCS})]
    rows=corpus(image)[:96]
    for row in rows:
        row['banks'].append(dict(address=0x4ea000,before=bytes(4096).hex()))
    rows+=parent_rows(image)

    def observe(rows,folder):
        folder.mkdir()
        return physical_original(image,rows,folder,entries=(PARENT,ENTRY),regions=REGIONS,
                                 trace_calls=trace,trace_extended=True)

    complete,report['normal_original_authority']=observe(rows,scratch/'normal')
    prefixes=[]
    original_children={pc:0 for pc in EDGES}
    for n,(row,actual) in enumerate(zip(rows,complete['cases'],strict=True)):
        if not actual['completed']:
            raise ValueError('Original return changed: '+row['name'])
        if row['entry']==PARENT:
            pcs=actual['executed_sites']
            callsite=next((pc for pc in EDGES if pc in pcs),0)
            if not callsite or pcs.count(ENTRY)!=1 or pcs.count(EDGES[callsite])!=1:
                raise ValueError('Expected one original child return: '+row['name'])
            original_children[callsite]+=1
        if n<16:
            for ordinal,read in enumerate(actual['reads']):
                pc=read['instruction']
                if pc in STOP_PCS:
                    stopped=copy.deepcopy(row)
                    stopped.update(name=row['name']+f'-stop-{pc:x}',provider_stop_pc=pc,
                                   fault=dict(phase='before_read',ordinal=ordinal))
                    prefixes.append(stopped)
    if set(original_children.values())!={48} or {row['provider_stop_pc'] for row in prefixes}!=STOP_PCS:
        raise ValueError('Original inherited or post-shift frontier missing')
    stopped,report['prefix_original_authority']=observe(prefixes,scratch/'prefixes')
    all_cases=complete['cases']+stopped['cases']
    rows+=prefixes
    for row,actual in zip(rows,all_cases,strict=True):
        pc=row.get('provider_stop_pc',actual['executed_sites'][-1])
        if actual['completed']==('fault' in row) or ('fault' in row and actual['boundary']['instruction']!=pc):
            raise ValueError('Original physical boundary changed: '+row['name'])
        row['expected']=boundary_packet(actual,row,pc)
        for bank,expected in zip(row['banks'],actual['banks'],strict=True):
            bank['after']=expected['after']
    report.update(original_child_returns={hex(pc):count for pc,count in original_children.items()},
                  original_provider_stops=len(prefixes))
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('xbe','native','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not args.xbe.is_file():
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_bound=True,owned_scratch=str(scratch),retained_large_artifacts=[],runs={})
    try:
        if sha(args.xbe)!=PIN:
            raise ValueError('Wrong original TU1.5 image')
        report['linked_binary_sha256']=sha(args.native)
        rows=fixture(Xbe(args.xbe),scratch,report)
        inputs=scratch/'inputs.json';inputs.write_text(json.dumps(rows,separators=(',',':')))
        report['input_sha256']=sha(inputs)
        env={key:value for key,value in os.environ.items() if not key.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        streams={}
        for name,control in (('default',{}),('own-reference',{CONTROL:'1'}),
                             ('unrelated-reference',{'HALO2_REFERENCE_FLIGHT_PROPERTY_QUERY':'1'})):
            path=scratch/(name+'.stream');streams[name]=path
            tested=subprocess.run([args.native,inputs,path],capture_output=True,text=True,env={**env,**control},timeout=1200)
            if tested.returncode:
                raise ValueError(name+' actual Primary API failed: '+(tested.stdout+tested.stderr)[-6000:])
            counts=json.loads(tested.stdout)
            if (counts['native_entries']!=(0 if name=='own-reference' else len(rows)) or
                counts['normal']!=192 or counts['stopped']!=report['original_provider_stops'] or
                counts['first_callsite_returns']!=48 or counts['second_callsite_returns']!=48):
                raise ValueError(name+' actual Primary route counts changed: '+str(counts))
            report['runs'][name]=dict(result=counts,controls=control,stream_sha256=sha(path))
        if any(not filecmp.cmp(streams['default'],p,shallow=False) for p in streams.values()):
            raise ValueError('Default/reference complete Primary linked streams differ')
        report.update(passed=True,complete_streams_equal=True)
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
