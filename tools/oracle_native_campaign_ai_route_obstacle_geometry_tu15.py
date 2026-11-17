#!/usr/bin/env python3
"""Unchanged physical i386 authored-obstacle marker and append reference.

Inputs are authored banks and real marker groups. No child result or original
instruction is supplied. Raw banks, observer and binaries use owned scratch.
"""
import argparse,copy,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_object_marker_request_tu15 as requests
import oracle_native_campaign_object_markers_tu15 as markers
OWNED=(0x279640,0x279900,0x279b30,0x279600,0x2cab80)
ENTRIES=OWNED+(0xbad60,0xbacd0,0xfc510,0x1e11b0,0x1e1200,0x144910,0x145560)
AXIS,RECORD,LIST,POINT=(markers.BASE+n for n in(0xe000,0xe100,0xf000,0xef00))
def inputs(image):
    seeds=requests.inputs(image);base=seeds[0];rows=[]
    def make(entry,name,profile=None):
        row=copy.deepcopy(base);n=len(rows)
        row.update(id=n,name=name,entry=entry,gp=[0x5a100000+n*23+k*3 for k in range(8)],
            matrix=0,source=0,stack_arguments=[],before_fp=list(profile or search.FP_PROFILES[n%6]),
            live_x87=[],xmm=[[0x6b100000+n*37+r*4+k for k in range(4)]for r in range(8)],
            stop_before_entries=[],max_steps=100000,strict_writes=False)
        row['gp'][4]=search.SP-n%4*4
        banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']}
        def put(at,value,width=4):
            for start,data in banks.items():
                if start<=at and at+width<=start+len(data):
                    data[at-start:at-start+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            banks[at]=bytearray((value&((1<<(width*8))-1)).to_bytes(width,'little'))
        row['_banks']=banks;rows.append(row);return row,put
    families={0x279640:([1,2],[0x0f000526,0x11000527]),
        0x279900:([0x20,0x40,0x80],[0x0f00052a,0x1100052b,0x1000052c]),
        0x279b30:([0x800,0x1000],[0x1100052f,0x10000530])}
    for entry,(bits,names) in families.items():
        for selected in range(1<<len(bits)):
            flags=sum(v for i,v in enumerate(bits)if selected&(1<<i))
            for capacity in(-1,0,1,2,3,7):
                for mode in('authored','fallback','mirrored'):
                    row,put=make(entry,f'{entry:x}_flags{flags:x}_capacity{capacity}_{mode}')
                    row['gp'][0]=capacity&0xffffffff
                    row.update(matrix=0xea000001,source=markers.DEST,stack_arguments=[flags])
                    put(markers.TAG+0x58,len(names))
                    for i,key in enumerate(sorted(names)):
                        put(markers.GROUPS+i*12,key);put(markers.GROUPS+i*12+4,2);put(markers.GROUPS+i*12+8,markers.MARKERS)
                    if mode=='fallback':put(requests.OBJECT_TAG+0x38,0xffffffff)
                    if mode=='mirrored':put(requests.BODY+4,0x400)
    for profile in search.FP_PROFILES:
        for count in(-1,0,1,63,64,65):
            for flags in(0,1,8,9,0x8009):
                row,put=make(0x2cab80,f'append_count{count}_flags{flags:x}_fp{profile}',profile)
                row['gp'][2]=LIST;row['gp'][3]=0x5a000000|flags;row['gp'][6]=POINT
                row.update(matrix=0xea000001,source=search.f(.375))
                put(LIST+2,count,2);put(LIST+4,0xffff,2);put(LIST+6,0x7fff,2)
                put(POINT,search.f(1.25));put(POINT+4,search.f(-2.5))
        for value in(0,search.f(-1.),search.f(.5),search.f(1.),1,0x7fc12345,0x7f812345,0x7f800000):
            for axis in((0.,0.,1.),(1.,.25,-.5)):
                row,put=make(0x279600,f'orientation_value{value:x}_axis{axis}_fp{profile}',profile)
                row['gp'][1]=RECORD;put(0x474d10,AXIS)
                for lane,v in enumerate(axis):put(AXIS+lane*4,search.f(v));put(RECORD+0x3c+lane*4,value)
    for row in rows:
        row['banks']=[dict(address=a,before=bytes(d).hex())for a,d in sorted(row.pop('_banks').items())]
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args()
    assert a.verify or not a.output.exists();assert reference.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=reference.metadata(image,ENTRIES)
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):
            assert len(site['operands'])==1;site['operands'][0].update(read=False,write=True)
    owned=reference.prepare(image,meta,scratch);rows=search.physical(owned,inputs(image),'whole')
    for row in rows:assert row['completed']and row.get('stopped_at',search.STOP)==search.STOP,row['name']
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,stop_entries=[],bank_encoding='sparse-runs-v1',write_encoding='rep-runs-v1',cases=reference.compact(rows))
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==data,'Independent original repeat changed'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    report=dict(passed=True,cases=len(rows),reads=sum(len(r['reads'])for r in rows),stores=sum(len(r['writes'])for r in rows),
        coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code))for e in ENTRIES},
        fixture=str(a.output),fixture_bytes=len(data),fixture_sha256=reference.sha(a.output),original_sha256=NEW_PIN,
        source_sha256={str(Path(__file__).relative_to(reference.ROOT)):reference.sha(__file__)},owned_scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
