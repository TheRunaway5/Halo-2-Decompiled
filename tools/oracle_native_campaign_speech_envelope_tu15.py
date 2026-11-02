#!/usr/bin/env python3
"""Physical unchanged21F860 speech byte-envelope lookup and normalization."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_ai_action_identity_tu15 as storage
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

BASE,TAGS,DEFINITION,GESTALT,RANGES,PERMUTATIONS,RESOURCES,RESOURCE,CURVES,SAMPLES=(storage.BASE+n for n in
    (0,0x100,0x1000,0x2000,0x4000,0x6000,0x7000,0x8000,0xa000,0x10000))


def inputs(image):
    rows=[]
    def make(name,sound=0xe3610001,range_base=0,range_index=0,permutation=0,permutation_count=3,
             first=0,resource_index=0,curve_index=0,count=4,sample=1,value=137,changes=(),timeline=()):
        n=len(rows);banks=object.__new__(storage.Banks);banks.data={}
        banks.put(0x4f0414,TAGS);banks.put(TAGS+(sound&0xffff)*16+8,DEFINITION)
        banks.put(DEFINITION+8,range_base,2);banks.put(DEFINITION+0xe,resource_index,2)
        banks.put(0x52d664,GESTALT);banks.put(GESTALT+0x24,RANGES)
        row=RANGES+(range_base+range_index)*12;banks.put(row+8,first,2);banks.put(row+0xa,permutation_count,2)
        banks.put(GESTALT+0x54,RESOURCES);banks.put(GESTALT+0x2c,PERMUTATIONS)
        banks.put(RESOURCES+resource_index*0x2c+4,RESOURCE);banks.put(RESOURCE+0xc,CURVES)
        banks.put(PERMUTATIONS+(first+permutation)*16+5,curve_index,1)
        banks.put(CURVES+curve_index*16+4,count);banks.put(CURVES+curve_index*16,0x63)
        banks.put(RESOURCE+4,SAMPLES)
        # A real declared sample extent; no selected sample/result is supplied
        # by the oracle. The original resolves its index and reads this bank.
        banks.bytes(SAMPLES,bytes([value])*0x10000)
        banks.bytes(0x46a0c4,image.read(0x46a0c4,4))
        for address,new,width in changes:banks.put(address,new,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=sound;gp[1]=(gp[1]&0xffff0000)|(permutation&0xffff);gp[2]=(gp[2]&0xffff0000)|(range_index&0xffff);gp[4]=geometry.SP+n%4*4
        sample_argument=(0xa53b0000|(sample&0xffff));banks.bytes(gp[4],struct.pack('<II',geometry.STOP,sample_argument))
        depth=(0,1,3,8)[n%4];top=n%8
        values=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
        mmx=n%9==0
        if mmx:depth=8;top=0;values=[struct.pack('<QH',0x5a00000000000000+n*31+k,0xffff).hex()for k in range(8)]
        rows.append(dict(id=n,name=name,entry=0x21f860,gp=gp,arguments=[sample_argument],sound=sound,range=range_index,
            permutation=permutation,sample=sample,active_mmx=mmx,banks=banks.rows(),input_timeline=list(timeline),
            before_fp=[0x27f|((n%4)<<10),(top<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=values))
        return rows[-1]
    for value in range(256):make('sample_byte_'+str(value),value=value)
    for permutation in(-32768,-2,-1,0,1,2,3,32767):
        for count in(-32768,-1,0,1,3,32767):make(f'permutation_{permutation}_{count}',permutation=permutation,permutation_count=count)
    for sample in(-32768,-1,0,1,3,4,32767):
        for count in(-1,0,1,4,32768,0x7fffffff):make(f'sample_{sample}_{count}',sample=sample,count=count)
    for base,offset in((-32768,0),(-1,1),(0,-1),(32767,0),(0,32767)):
        make(f'range_{base}_{offset}',range_base=base,range_index=offset)
    for resource in(-32768,-1,0,32767):make('resource_'+str(resource),resource_index=resource)
    for curve in(-128,-1,0,127):make('curve_'+str(curve),curve_index=curve)
    for first in(-32768,-1,1,32767):make('first_'+str(first),first=first)
    for salt in(0,1,0xffff):make('sound_salt_'+str(salt),sound=(salt<<16)|1)
    for n in range(16):make('FP_'+str(n),value=(n*19)&255)
    def publication(address,occurrence,target,value,width=4):
        return dict(on_read=dict(address=address,occurrence=occurrence),writes=[dict(address=target,bytes=(value&((1<<(width*8))-1)).to_bytes(width,'little').hex())])
    make('cached_gestalt',timeline=(publication(GESTALT+0x24,1,0x52d664,0xdead0000),))
    make('fresh_curve_offset',timeline=(publication(CURVES+4,1,CURVES,0x64),))
    for options in(dict(),dict(permutation=-1),dict(count=0),dict(sample=-1)):
        make('fault_seed_'+str(len(rows)),**options)
    for label,address in(('tags',0x4f0414),('gestalt',0x52d664),('resource',RESOURCES+4)):
        row=make('unmapped_'+label,changes=((address,0xdead0000,4),));row['expected_fault']=True
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--verify',type=Path)
    args=parser.parse_args();assert not args.output.exists();assert common.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x21f860,);metadata=common.metadata(image);code=_Code(image,'.text')
    metadata['private_stack']=dict(begin=geometry.STACK,bytes=65536)
    for site in metadata['sites']:
        if code.at(site['instruction']).mnemonic in('cmp','test'):
            for operand in site['operands']:operand.update(read=True,write=False)
        if code.at(site['instruction']).mnemonic=='ret':
            site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);declared=inputs(image);owned=storage.prepare(image,metadata,scratch,declared)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_speech_envelope_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert result.returncode==0,result.stderr
    complete=geometry.physical(owned,declared,'complete');prefix=[]
    for seed in(r for r in complete if r['name'].startswith('fault_seed')):
        for ordinal in range(len(seed['reads'])):
            row=copy.deepcopy(seed);row.update(id=len(complete)+len(prefix),name=seed['name']+'_before_read_'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal));prefix.append(row)
    rows=complete+geometry.physical(owned,prefix,'prefix');assert all(not row['writes']for row in rows)
    reached=set().union(*(set(row['executed_sites'])for row in complete));sites={site['instruction']for site in metadata['sites']};assert sites<=reached,sorted(sites-reached)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=metadata['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Original speech-envelope repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,original_sha256=NEW_PIN,cases=len(rows),complete_calls=sum(row['completed']for row in complete),
        true_memory_faults=sum(not row['completed']for row in complete),fault_prefixes=len(prefix),
        original_sites=len(sites),reached_sites=len(reached),ordered_reads=sum(len(row['reads'])for row in rows),
        fixture=str(args.output),fixture_bytes=len(data),fixture_sha256=common.sha(args.output),independent_repeat_identical=True if args.verify else None,
        source_sha256={str(path.relative_to(ROOT)):common.sha(path)for path in(Path(__file__),ROOT/'tools/observe_native_campaign_speech_envelope_tu15.cpp')},
        owned_scratch=str(scratch),retained_large_artifacts=[],native_implementation_qualified=False,whole_dialogue_qualified=False,
        whole_audio_qualified=False,live_playback_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in('cases','complete_calls','original_sites','ordered_reads')}))


if __name__=='__main__':main()
