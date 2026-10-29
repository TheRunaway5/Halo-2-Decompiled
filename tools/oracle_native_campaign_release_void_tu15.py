#!/usr/bin/env python3
"""Physical release map_reset/time_code_reset, real void HS completion and RET."""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_level_transition_tu15 as level
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=0x1000000;THREADPOOL=BASE+0x100;NODEPOOL=BASE+0x200;THREADS=BASE+0x1000;NODES=BASE+0x4000;FRAME=BASE+0x5000;PREVIOUS=BASE+0x6000;OUTPUT=BASE+0x7000
ENTRIES=(0x2ac980,0x211da0);sha=common.sha
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);profile=json.loads((ROOT/'tools/mcc-script-layout.json').read_text());definitions=[r for r in profile['builtins']if r['name']in('map_reset','time_code_reset')];assert len(definitions)==2
    for r in definitions:assert int.from_bytes(image.read(int(r['xbox_descriptor'],16)+4,4),'little')==0x2ac980 and r['return_type']==r['xbox_return_type']==4 and r['arguments']==[]
    common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=level.prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=[]
    for definition in definitions:
        for thread in (0,1,3,7):
            for syntax in (0,7):
                n=len(rows);banks={BASE:bytearray([0x79]*0x10000)}
                def put(p,v,w=4):
                    for first,data in banks.items():
                        if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
                    banks[p]=bytearray((v&((1<<(8*w))-1)).to_bytes(w,'little'))
                record=THREADS+thread*0x418;frame=FRAME+n*0x20;previous=PREVIOUS+n*0x10;output=OUTPUT+n*4;node=NODES+syntax*20;datum=(0xe000+syntax)<<16|syntax
                for p,v in [(0x505e5c,THREADPOOL),(0x505e6c,NODEPOOL),(THREADPOOL+0x44,THREADS),(NODEPOOL+0x44,NODES),(record+0x10,frame),(frame,previous),(frame+4,datum),(previous+8,output),(output,0x87654321)]:put(p,v)
                put(node,0xe000+syntax,2);put(node+2,definition['xbox_opcode'],2);put(node+4,4,2);put(node+6,8,2)
                for p in (0x480c00+definition['xbox_opcode']*4,int(definition['xbox_descriptor'],16)):banks[p]=bytearray(image.read(p,4))
                for p,size in ((0x47c904,16),(0x556dff,8),(0x4f9c6d,12)):banks[p]=bytearray([0x63]*size)
                gp=[0x5a000000+n*37+i*11 for i in range(8)];gp[4]=geometry.SP+n%4*4
                rows.append(dict(id=n,name=definition['name']+'_'+str(n),entry=0x2ac980,gp=gp,matrix=definition['xbox_opcode'],source=0x9abc0000|thread,child_output=0,private_output=False,thread_record=record,previous_frame=previous,result_output=output,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x27f|(n%4<<10),(n%8<<11)|0x41,0x1f80|(n%4<<13)|0x8040],live_x87=[]))
    rows=geometry.physical(owned,rows,'release-void')
    for row in rows:
        assert row['completed']and row['boundary_gp'][4]==row['gp'][4]+16
        assert row['after_fp']==row['before_fp'] and row['live_x87_unchanged']
        assert [(w['instruction'],w['address'],int.from_bytes(bytes.fromhex(w['written']),'little'))for w in row['writes']]==[(0x211e5e,row['result_output'],0),(0x211e6c,row['thread_record']+0x10,row['previous_frame'])]
        for bank in row['banks']:
            if bank['address']!=BASE:assert bank['before']==bank['after']
    visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,definitions=definitions,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Release void independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,definitions=definitions,owners=meta['owners'],physical_calls=len(rows),complete_calls=len(rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),unchanged_instructions=sum(r['original_instructions']for r in rows),evaluator_sites_executed=sorted(hex(s)for s in visited if 0x2ac980<=s<0x2ac990),hs_completion_profile='Actual builtin return4 and compiled-context4. Only ordinary result0 publication and frame unlink; no coercion callee or fabricated return.',gameplay_reset_or_save_stores=False,true_return_bytes=16,generic_211DA0_coercion_closed=False,script_vm_is_native_bytecode_interpreter=True,runtime_original_execution=False,fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),tool_sha256=sha(__file__),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('definitions','owners')}))
if __name__=='__main__':main()
