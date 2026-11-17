#!/usr/bin/env python3
"""Compare linked CombatAction table-ratio entries and actual Primary call sites."""
import argparse
import copy
import filecmp
import json
import os
from pathlib import Path
import subprocess

import oracle_native_campaign_screen_lifecycle_tu15 as oracle
from oracle_smg_firing_1_1 import Xbe
from validate_native_flight_property_query_frame_tu15 import sha
from validate_native_object_table_ratio_frame_tu15 import BANKS, ENTRY, corpus
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN, physical_original

PRIMARY,DEFINITION,MODE=0x1046c0,0x165590,0x1655f0
EDGES={0x104a10:(0x104a15,DEFINITION),0x104a1e:(0x104a23,MODE),
       0x104b35:(0x104b3a,DEFINITION),0x104b43:(0x104b48,MODE),0x165602:(0x165607,ENTRY)}
STOP_PCS={0x24b479,0x24b48c}
CONTROL=('HALO2_REFERENCE_OBJECT_TABLE_RATIO','HALO2_REFERENCE_OBJECT_TABLE_RATIO_MODE',
         'HALO2_REFERENCE_OBJECT_DEFINITION_MODE_QUERY')


def seeded_rows(image):
    direct=corpus(image)[:96];mode=[];primary=[]
    for n,row in enumerate(direct):
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}

        def locate(addr,width):
            for base,data in banks.items():
                if base<=addr<=base+len(data)-width:
                    return data,addr-base
            raise ValueError(f'1046C0 external ratio fixture escaped {addr:x}')

        def put(addr,value,width=4):
            data,offset=locate(addr,width)
            data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')

        def get(addr):
            data,offset=locate(addr,4)
            return int.from_bytes(data[offset:offset+4],'little')

        def snapshot():return [dict(address=base,before=banks[base].hex()) for base,_ in BANKS]

        datum=row['gp'][0];slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        options=get(0x4f3218);table=get(0x52d710);scale=get(0x51df54);sp=row['gp'][4]
        put(options+0x180,8 if n//8%4==0 else 9)
        put(sp+4,datum)
        mode.append(dict(row,entry=MODE,name=f'actual-mode-owner-{n}',matrix=datum,banks=snapshot()))

        idx=(-3,0,1,6)[n//2%4]
        put(body+0x17e,idx,2);put(definition+0x290,1,2);put(options+8,2);put(options+0x180,9)
        put(table+0x170+idx,8 if n//8%2 else 0,1);put(table+0x1a0+idx*2,(-2,4,13,0)[n//4%4],2)
        put(scale+4,0x3e000000)
        key=(0xe0006e6,0x120006e4)[n%2]
        put(sp+8,key);put(sp+12,0x61003e00);put(sp+16,0x61003e10)
        primary.append(dict(row,entry=PRIMARY,name=f'actual-primary-combat-ratio-{n}',matrix=datum,source=key,
                            max_steps=250,banks=snapshot()))
    return direct+mode+primary


def fixture(image,scratch,report):
    code=oracle._Code(image,'.text')
    for pc,(resume,target) in EDGES.items():
        call=image.read(pc,5)
        if call[0]!=0xe8 or resume+int.from_bytes(call[1:],'little',signed=True)!=target:
            raise ValueError('Authentic table-ratio edge changed')
    original=oracle.metadata

    def metadata(image):
        result=original(image)
        for site in result['sites']:
            if code.at(site['instruction']).mnemonic in ('pop','ret'):
                site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result

    entries=(PRIMARY,DEFINITION,MODE,ENTRY)
    oracle.ENTRIES=entries;oracle.metadata=metadata
    if len(metadata(image)['sites'])!=471:
        raise ValueError('Exact Primary and CombatAction original site count changed')
    trace=[dict(entry=pc,arguments=0,gp=True,xmm=True) for pc in sorted({*entries,*EDGES,*STOP_PCS,
                                                                     *(resume for resume,_ in EDGES.values())})]
    rows=seeded_rows(image)

    def observe(rows,folder):
        folder.mkdir()
        return physical_original(image,rows,folder,entries=entries,regions=BANKS,trace_calls=trace,trace_extended=True)

    complete,report['normal_original_authority']=observe(rows,scratch/'normal')
    prefixes=[]
    child_returns={pc:0 for pc in EDGES}
    for n,(row,actual) in enumerate(zip(rows,complete['cases'],strict=True)):
        if not actual['completed']:
            raise ValueError('Expected original normal return: '+row['name'])
        pcs=actual['executed_sites']
        for pc,(resume,target) in EDGES.items():
            if pc in pcs:
                if pcs.count(resume)!=1 or pcs.count(target)!=1:
                    raise ValueError('Original child did not return once: '+row['name'])
                child_returns[pc]+=1
        if n<8:
            for ordinal,read in enumerate(actual['reads']):
                pc=read['instruction']
                if pc in STOP_PCS:
                    stopped=copy.deepcopy(row)
                    stopped.update(name=row['name']+f'-stop-{pc:x}',provider_stop_pc=pc,
                                   fault=dict(phase='before_read',ordinal=ordinal))
                    prefixes.append(stopped)
    if child_returns!={0x104a10:48,0x104a1e:48,0x104b35:48,0x104b43:48,0x165602:168} or not prefixes:
        raise ValueError('Authentic original table-ratio route counts changed')
    stopped,report['prefix_original_authority']=observe(prefixes,scratch/'prefixes')
    rows+=prefixes
    owner_entries=[0,0,0]
    for row,actual in zip(rows,complete['cases']+stopped['cases'],strict=True):
        pc=row.get('provider_stop_pc',actual['executed_sites'][-1])
        if actual['completed']==('fault' in row) or ('fault' in row and actual['boundary']['instruction']!=pc):
            raise ValueError('Original physical boundary changed: '+row['name'])
        counts=[actual['executed_sites'].count(entry) for entry in (ENTRY,MODE,DEFINITION)]
        owner_entries=[total+count for total,count in zip(owner_entries,counts,strict=True)]
        row.update(expected=boundary_packet(actual,row,pc),original_owner_entries=counts)
        for bank,expected in zip(row['banks'],actual['banks'],strict=True):
            bank['after']=expected['after']
    report.update(original_child_returns={hex(pc):count for pc,count in child_returns.items()},
                  original_provider_stops=len(prefixes),authentic_owner_entries=owner_entries)
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
    report=dict(passed=False,production_bound=True,actual_world_dispatch_proved=False,
                owned_scratch=str(scratch),retained_large_artifacts=[],runs={})
    try:
        if sha(args.xbe)!=PIN:
            raise ValueError('Wrong original TU1.5 image')
        report['linked_binary_sha256']=sha(args.native)
        rows=fixture(Xbe(args.xbe),scratch,report)
        inputs=scratch/'inputs.json';inputs.write_text(json.dumps(rows,separators=(',',':')))
        report['input_sha256']=sha(inputs)
        env={key:value for key,value in os.environ.items() if not key.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        streams={}
        modes=(('default',None,{}),('leaf-reference',0,{CONTROL[0]:'1'}),('mode-reference',1,{CONTROL[1]:'1'}),
               ('definition-reference',2,{CONTROL[2]:'1'}),('unrelated-reference',None,{'HALO2_REFERENCE_PROJECTILE_FUNCTION_QUERY':'1'}))
        for name,index,control in modes:
            path=scratch/(name+'.stream');streams[name]=path
            tested=subprocess.run([args.native,inputs,path],capture_output=True,text=True,env={**env,**control},timeout=1200)
            if tested.returncode:
                raise ValueError(name+' actual CombatAction APIs failed: '+(tested.stdout+tested.stderr)[-6000:])
            counts=json.loads(tested.stdout);expected=list(report['authentic_owner_entries'])
            if index is not None:
                expected[index]=0
            if (counts['native_entries']!=expected or counts['authored_entries']!=report['authentic_owner_entries'] or
                counts['normal']!=288 or counts['stopped']!=report['original_provider_stops'] or counts['actual_external_calls']!=192):
                raise ValueError(name+' actual CombatAction route counts changed: '+str(counts))
            report['runs'][name]=dict(result=counts,controls=control,stream_sha256=sha(path))
        if any(not filecmp.cmp(streams['default'],p,shallow=False) for p in streams.values()):
            raise ValueError('Default/reference complete CombatAction linked streams differ')
        report.update(passed=True,complete_streams_equal=True)
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
