#!/usr/bin/env python3
"""Unchanged physical TU1.5 speech queue closure versus semantic native owners."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
sys.dont_write_bytecode=True
from validate_native_campaign_audio_leaves_tu15 import ROOT,ORIGINAL,PIN,text_section,sha,save,checked

BASE,SIZE,POOL,ROWS,QUEUE=0x1000000,0x20000,0x1000000,0x1001000,0x1003000
GLOBALS,SETTINGS,DEFINITION,ACTIONS,TAGS,CLOCK=0x1004000,0x1004400,0x1005000,0x1006000,0x1007000,0x1008000
OBJECTS,HEADERS,BODY=0x1009000,0x100a000,0x100b000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4ecc1c,4),(0x4f0414,4),(0x505e70,4),(0x51df54,4),(0x4ecbd0,4),(0x46a16c,4),(0x505e74,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x217970);rows=[];configs=[]
    for start in (90,100,101,102,103,119,120,121):configs.append(dict(kind='postpone',label=f'window-{start}',start=start))
    for priority in (-32768,-1,0,4,5,6,10,11,32767):
        for active in (0,1):configs.append(dict(kind='postpone',label=f'priority-{priority}-{active}',priority=priority,active=active,start=101))
    configs += [dict(kind='postpone',label=label,**options)for label,options in [
        ('active-other-unit',dict(active=1,unit=1,block_unit=2,start=101)),
        ('active-high',dict(active=1,unit=1,block_unit=1,start=101,priority=11)),
        ('threshold-equal',dict(threshold=5,start=110)),('threshold-above',dict(threshold=6,start=110)),
        ('dependent-independent',dict(start=101,dependent=2)),('both-dependent',dict(start=101,dependent=2,block_dependent=3)),
        ('self-independent',dict(start=101,dependent=0xffffffff)),
        ('no-globals-before-window',dict(start=90,globals=0)),('zero-settings-before-window',dict(start=90,settings_count=0)),
        ('negative-settings-before-window',dict(start=90,settings_count=-1)),('missing-tag-before-window',dict(start=90,tag=0xffffffff)),
        ('wrapped-upper',dict(start=0x7fffffff,block_start=0x7ffffff0,duration=32)),
        ('negative-time',dict(start=-95,block_start=-100,duration=20)),('negative-action',dict(start=110,action=-1)),
        ('negative-duration',dict(start=101,duration=-1))]]
    for precision in range(3):
        for rounding in range(4):
            for rate in (-32768,-1,0,1,30,60,32767):configs.append(dict(kind='postpone',label=f'fp-{precision}-{rounding}-{rate}',start=110,precision=precision,rounding=rounding,rate=rate))
    configs += [dict(kind='cancel',label='cancel-'+label,**options)for label,options in [
        ('empty',dict(chain=[])),('no-dependency',dict(chain=[1,2])),('propagate',dict(chain=[1,2],dependencies={1:0,2:1})),
        ('already-related',dict(chain=[1,2],dependencies={1:3,2:1},cancelled=[3])),('different-salt',dict(chain=[1],bad_salt=True,dependencies={1:0})),
        ('zero-salt',dict(chain=[1],zero_salt=True,dependencies={1:0})),('extent-boundary',dict(chain=[1],limit=1,dependencies={1:3})),
        ('negative-extent',dict(chain=[1],limit=-1,dependencies={1:0})),('negative-salt',dict(chain=[1],negative_salt=True,dependencies={1:0})),
        ('unrelated-active',dict(chain=[1],dependencies={1:3}))]]
    configs += [dict(kind='reclaim',label='reclaim-'+label,**options)for label,options in [
        ('empty',dict(chain=[])),('keep',dict(chain=[0,1,2])),('first',dict(chain=[0,1,2],cancelled=[0])),
        ('middle',dict(chain=[0,1,2],cancelled=[1])),('last',dict(chain=[0,1,2],cancelled=[2])),
        ('all',dict(chain=[0,1,2],cancelled=[0,1,2])),('debug-all',dict(chain=[0,1,2],cancelled=[0,1,2],debug=True)),
        ('unit-ref',dict(chain=[0,1],cancelled=[0],units={0:1},unit_ref=0)),
        ('unit-other-ref',dict(chain=[0,1],cancelled=[0],units={0:1},unit_ref=1)),
        ('unit-negative-offset',dict(chain=[0,1],cancelled=[0],units={0:1},unit_ref=0,unit_offset=-0x100)),
        ('bitmap-alias-head',dict(chain=[0,1],cancelled=[0,1],bitmap_head=True)),
        ('highwater',dict(chain=[3,2,1],cancelled=[3,2,1],limit=4)),
        ('debug-tail',dict(chain=[0],cancelled=[0],debug=True,stride=0x55,limit=1))]]
    configs += [dict(kind='admit',label='admit-'+label,**options)for label,options in [
        ('empty',dict(chain=[])),('restricted-reject',dict(chain=[1],restriction=123)),('restricted-unit',dict(chain=[],restriction=1,unit=1)),
        ('restricted-target',dict(chain=[],restriction=2,target_unit=2)),('late-reject',dict(chain=[],start=200,allowed=99)),
        ('late-equal',dict(chain=[],start=200,allowed=100)),('dependent-reject',dict(chain=[],same_class=1,dependent=2)),
        ('dependent-none',dict(chain=[],same_class=1)),('dependent-match',dict(chain=[1],start=200,same_class=1,dependent=1,durations={1:0})),
        ('head',dict(chain=[1,2,3],start=90,starts={1:100,2:200,3:300})),
        ('middle',dict(chain=[1,2,3],start=150,starts={1:100,2:200,3:300},durations={1:1})),
        ('tail',dict(chain=[1,2,3],start=400,starts={1:100,2:200,3:300},durations={1:1,2:1,3:1})),
        ('collision',dict(chain=[1,2,3],start=105,starts={1:100,2:110,3:120})),
        ('equal',dict(chain=[1],start=100,starts={1:100},threshold=20)),
        ('separation-retry',dict(chain=[1,2],start=100,starts={1:100,2:100},threshold=20)),
        ('suffix-expire',dict(chain=[1,2],start=100,starts={1:101,2:102},threshold=20,allowances={1:0,2:0})),
        ('suffix-cancelled',dict(chain=[1,2],start=100,starts={1:101,2:102},threshold=20,cancelled=[1])),
        ('suffix-dependent-reject',dict(chain=[1,2],start=100,starts={1:110,2:120},threshold=20,classes={1:1},dependencies={1:3})),
        ('suffix-dependent-none',dict(chain=[1,2],start=100,starts={1:110,2:120},threshold=20,classes={1:1})),
        ('suffix-dependent-match',dict(chain=[1,2],start=100,starts={1:110,2:120},threshold=20,classes={1:1},dependencies={1:0})),
        ('cancel-then-propagate',dict(chain=[1,2,3],start=100,starts={1:101,2:102,3:103},threshold=20,allowances={1:0},dependencies={2:1,3:2})),
        ('reject-still-reclaims',dict(chain=[1],restriction=123,cancelled=[1],debug=True)),
        ('negative-rate',dict(chain=[1,2],start=90,rate=-30,starts={1:100,2:100},threshold=20)),
        ('clock-alias-candidate',dict(chain=[1,2],start=90,starts={1:100,2:100},threshold=20,clock_candidate=True)),
        ('clock-follows-postponed-candidate',dict(chain=[1,2,3],start=105,starts={1:100,2:130,3:160},clock_candidate=True)),
        ('queue-count-alias-candidate-time',dict(chain=[1],start=90,starts={1:200},threshold=20,count_alias=True))]]
    configs += [dict(kind='admit',label='actual-publication-block-'+str(team),chain=[1,2],start=90,starts={1:100,2:200},threshold=20,caller_team=team)for team in (1,-1)]
    for options in configs:
        main=bytearray(rng.randbytes(SIZE))
        def put(p,v,n=4):main[p-BASE:p-BASE+n]=int(v&((1<<(n*8))-1)).to_bytes(n,'little')
        def datum(i):return (((0xf000 if options.get('negative_salt')else 0x1000)+i)<<16)|i
        def record(i):return ROWS+i*0x54
        for p,v in [(POOL+0x20,16),(POOL+0x24,options.get('stride',0x54)),(POOL+0x34,16),(POOL+0x38,options.get('limit',8)),(POOL+0x3c,8),(POOL+0x44,ROWS),(POOL+0x48,QUEUE+0x7d0 if options.get('bitmap_head')else BASE+0x200),(BASE+0x200,255),(GLOBALS+0xc8,options.get('settings_count',1)),(GLOBALS+0xcc,SETTINGS),(SETTINGS+0x64,options.get('tag',1)),(TAGS+24,DEFINITION),(DEFINITION+4,ACTIONS),(OBJECTS+0x44,HEADERS),(HEADERS+20,BODY)]:put(p,v)
        put(POOL+0x2a,8 if options.get('debug')else 0,1);put(CLOCK+2,options.get('rate',30),2)
        unit_offset=options.get('unit_offset',0x400);put(BODY+0x342,unit_offset,2);put(BODY+0x138,options.get('caller_team',1),2);put(BODY+unit_offset+0x1c,datum(options.get('unit_ref',7)))
        for i in range(12):
            p=record(i);put(p,datum(i)>>16,2);put(p+2,options.get('action',0)if i==0 else 0,2)
            for offset,value,n in [(4,options.get('units',{}).get(i,0xffffffff),4),(0xe,options.get('classes',{}).get(i,0),1),(0x10,100,4),(0x14,options.get('starts',{}).get(i,100),4),(0x1c,options.get('durations',{}).get(i,20),2),(0x1e,options.get('allowances',{}).get(i,1000),2),(0x22,5,2),(0x28,0xffffffff,4),(0x40,0,1),(0x44,datum(options['dependencies'][i])if i in options.get('dependencies',{})else 0xffffffff,4),(0x48,int(i in options.get('cancelled',[])),1),(0x50,0xffffffff,4)]:put(p+offset,value,n)
        for i in range(-1,12):put(ACTIONS+i*0x60+0x1a,options.get('threshold',0),2)
        chain=options.get('chain',[])
        for i,j in zip(chain,chain[1:]):put(record(i)+0x50,datum(j))
        put(QUEUE+0x7d0,datum(chain[0])if chain else 0xffffffff);put(QUEUE+0x7d4,len(chain),2);put(QUEUE+0x7d8,options.get('restriction',0xffffffff))
        p=record(0);put(p+0x14,options.get('start',110));put(p+0x1e,options.get('allowed',1000),2);put(p+4,options.get('unit',options.get('units',{}).get(0,0xffffffff)));put(p+0x28,options.get('target_unit',0xffffffff));put(p+0xe,options.get('same_class',0),1);put(p+0x22,options.get('priority',5),2)
        if 'dependent'in options:put(p+0x44,datum(options['dependent'])if options['dependent']!=0xffffffff else 0xffffffff)
        if options['kind']=='postpone':
            put(record(1)+0x14,options.get('block_start',100));put(record(1)+0x1c,options.get('duration',20),2);put(record(1)+4,options.get('block_unit',0xffffffff));put(record(1)+0x40,options.get('active',0),1)
            if 'block_dependent'in options:put(record(1)+0x44,datum(options['block_dependent']))
        if options['kind']=='cancel':
            put(record(0)+0x50,datum(chain[0])if chain else 0xffffffff)
            if options.get('bad_salt'):put(record(0),0x1234,2)
            if options.get('zero_salt'):put(record(0),0,2)
        entry={'postpone':0x217870,'admit':0x217970,'cancel':0x218070,'reclaim':0x2180f0}[options['kind']]
        actual_queue=QUEUE
        if options.get('count_alias'):
            actual_queue=p+0x14-0x7d4;put(actual_queue+0x7d0,datum(chain[0]));put(actual_queue+0x7d8,0xffffffff)
        clock=record(0)+0x12 if options.get('clock_candidate')else CLOCK
        values=[bytes(main)]+[struct.pack('<I',v)for v in (options.get('globals',GLOBALS),TAGS,POOL,clock,OBJECTS)]+[original_bytes(image,0x46a16c,4),struct.pack('<I',QUEUE-(0 if options.get('caller_team',1)==1 else 1)*0x7dc)]
        number=len(rows);depth=number%8;precision=options.get('precision',number%3);rounding=options.get('rounding',(number//3)%4)
        gp=[datum(0),0x23456789,POOL,0x456789ab,SP,0x56789abc,record(1),record(0)]
        arguments=[actual_queue,datum(0)]if options['kind']=='admit'else[actual_queue]if options['kind']=='reclaim'else[]
        before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        rows.append(dict(name=options['label'],kind=options['kind'],entry=entry,queue=actual_queue,datum=datum(0),candidate=record(0),blocker=record(1),capacity=8-depth,gp=gp,xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)],stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,before_fp=[(0x7f,0x27f,0x37f)[precision]|rounding<<10,((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|(((number//12)%4)<<13)|(0,0x40,0x8000,0x8040)[(number//48)%4]],banks=[dict(address=p,before=v.hex())for(p,_),v in zip(REGIONS,values)]))
        if 'caller_team'in options:
            row=rows[-1];row.update(entry=0x217812,capture_entry=0x217970,stop=0x21783c,arguments=[],stack=SP+12);row['gp'][4]=SP+12;row['gp'][1]=BODY
            row['stack_locals']=[(SP+0x24,datum(0))]
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_speech_queue_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        regions=[(bank['address'],len(bank['before'])//2)for bank in row['banks']];(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in regions))
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            if STACK<=bank['address']<STACK+0x10000:continue
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,row['stack']-STACK,STOP)
        for index,value in enumerate(row['arguments']):struct.pack_into('<I',stack,SP-STACK+4+index*4,value)
        for at,value in row.get('stack_locals',[]):struct.pack_into('<I',stack,at-STACK,value)
        for bank in row['banks']:
            if STACK<=bank['address']<STACK+0x10000:
                values=bytes.fromhex(bank['before']);stack[bank['address']-STACK:bank['address']-STACK+len(values)]=values
        (owned/'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=row['before_fp'];depth=len(row['before_x87']);sw=(sw&~0x3800)|(((((sw>>11)+depth)&7))<<11);environment=bytearray(28);struct.pack_into('<H',environment,0,cw);struct.pack_into('<H',environment,4,sw);struct.pack_into('<H',environment,8,0xffff);(owned/'environment.bin').write_bytes(environment)
        assembly.append('.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n fldenv environment\n ldmxcsr sse_control')
        for index in range(depth-1,-1,-1):assembly.append(f' fldt extended{index}')
        for index in range(8):assembly.append(f' movdqu vector{index},%xmm{index}')
        for reg,value in zip(('eax','ecx','edx','ebx','esp','ebp','esi','edi'),row['gp']):assembly.append(f' mov ${value},%{reg}')
        assembly.append(f' int3\n jmp *entry\n.balign 4\nentry: .long {row["entry"]}\nsse_control: .long {mxcsr}\nenvironment: .incbin "{owned/"environment.bin"}"')
        for index,vector in enumerate(row['xmm']):assembly.append(f'vector{index}: .long '+','.join(map(str,vector)))
        for index,value in enumerate(row['before_x87']):assembly.append(f'extended{index}: .byte '+','.join(str(byte)for byte in bytes.fromhex(value)))
        sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly)+'\n');(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        checked(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')]);checked(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')])
        try:checked([str(observer),str(owned/'reference'),hex(row['stop']),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')]+([hex(row['capture_entry'])]if 'capture_entry'in row else []))
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        if 'capture_entry'in row:
            captured=(owned/'output.bin.capture').read_bytes();cw,sw,mxcsr,prefix_steps=struct.unpack_from('<4I',captured);position=16
            row['gp']=list(struct.unpack_from('<8I',captured,position));position+=32;row['xmm']=[list(struct.unpack_from('<4I',captured,position+i*16))for i in range(8)];position+=128
            row['before_x87']=[captured[position+i*16:position+i*16+10].hex()for i in range(len(row['before_x87']))];position+=128;row['before_fp']=[cw,sw,mxcsr]
            for bank,(_,length)in zip(row['banks'],regions):bank['before']=captured[position:position+length].hex();position+=length
            actual_queue,actual_datum=struct.unpack_from('<2I',captured,position);position+=8;count,=struct.unpack_from('<I',captured,position);position+=4
            prefix_sites=list(struct.unpack_from('<'+'I'*count,captured,position));position+=count*4
            count,=struct.unpack_from('<I',captured,position);position+=4;prefix_stores=[]
            for _ in range(count):
                site,destination,length=struct.unpack_from('<III',captured,position);position+=12;prefix_stores.append(dict(instruction=site,address=destination,before=captured[position:position+length].hex(),written=captured[position+4:position+4+length].hex()));position+=8
            if position!=len(captured)or 0x217837 not in prefix_sites:raise ValueError('Actual admission caller receipt differs')
            row['caller']=dict(entry=0x217812,call_site=0x217837,callee=row['capture_entry'],return_site=0x21783c,prefix_steps=prefix_steps,prefix_sites=prefix_sites,prefix_engine_stores=prefix_stores,supplied_returns=[])
            row.update(entry=row['capture_entry'],arguments=[actual_queue,actual_datum],stack=row['gp'][4],queue=actual_queue,datum=actual_datum)
        data=(owned/'output.bin').read_bytes();cw,sw,mxcsr,instructions=struct.unpack_from('<4I',data);offset=16;row['after_gp']=list(struct.unpack_from('<8I',data,offset));offset+=32;row['after_xmm']=[list(struct.unpack_from('<4I',data,offset+i*16))for i in range(8)];row['after_xmm0']=row['after_xmm'][0];offset+=128;row['after_x87']=[data[offset+i*16:offset+i*16+10].hex()for i in range(len(row['before_x87']))];offset+=128;row.update(after_fp=[cw,sw,mxcsr],instructions=instructions);total+=instructions
        for bank,(_,length)in zip(row['banks'],regions):bank['after']=data[offset:offset+length].hex();offset+=length
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['reads']=[]
        for _ in range(count):
            destination,length,stores_before=struct.unpack_from('<III',data,offset);offset+=12;row['reads'].append(dict(address=destination,stores_before=stores_before,value=data[offset:offset+length].hex()));offset+=4
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['writes']=[]
        for _ in range(count):
            site,destination,length=struct.unpack_from('<III',data,offset);offset+=12;row['writes'].append(dict(instruction=site,address=destination,before=data[offset:offset+length].hex(),written=data[offset+4:offset+4+length].hex()));offset+=8
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['sites']=list(struct.unpack_from('<'+'I'*count,data,offset));offset+=count*4
        if offset!=len(data):raise ValueError('Path heap observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_speech_queue_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x217970:149,0x217870:79,0x218070:45,0x2180f0:49,0x16e8a0:62}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Speech queue closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        unreachable=set()
        missing=(expected-unreachable)-reached
        if missing:raise ValueError('Speech queue reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected-unreachable),reached_sites=len((expected-unreachable)&reached),unreachable_sites=sorted(unreachable),missing_sites=sorted(missing))
    return dict(image_sha256=PIN,backend='unchanged-tu15-physical-i386-interleaved-inputs-stores-fp',cases=rows,instructions=total,coverage=coverage,
                original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[])

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--native',type=Path)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not ORIGINAL.is_file():
        args.output.mkdir(parents=True,exist_ok=True)
        save(args.output/'report.json',dict(passed=False,status='skipped',validation_not_run=True,
            reason='Required owned TU1.5 offline reference is unavailable',required_reference=str(ORIGINAL),retained_large_artifacts=[]))
        print('SKIP: required owned TU1.5 offline reference is unavailable')
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output.mkdir(parents=True,exist_ok=True);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);save(args.output/'report.json',report)
    sources=[ROOT/'include/halo2/native_campaign_ai_speech_queue_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_speech_queue_tu15.cpp',ROOT/'tests/native_campaign_ai_speech_queue_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_speech_queue_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_speech_queue_tu15_sites.json',ROOT/'include/halo2/native_campaign_audio_ducking.hpp',ROOT/'include/halo2/native_campaign_audio_leaves.hpp',ROOT/'cmake/CampaignNativeAiSpeechQueue.cmake',ROOT/'src/core/native_tu15_data_array_delete.cpp',ROOT/'include/halo2/native_tu15_data_array_delete.hpp',ROOT/'cmake/NativeTu15DataArrayDelete.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-speech-queue.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(actual_original_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],caller_contract='actual queue address and salted speech datum; forward DF; one available extended temporary only on timing arithmetic',original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'window_equal':('if(signed32(start)>=signed32(end))return;','if(signed32(start)>signed32(end))return;'),
            'priority_equal':('if(word(m,action_definition+0x1au)>=priority)return;','if(word(m,action_definition+0x1au)>priority)return;'),
            'delay_equal':('bool valid=signed32(start-base)<=word(m,candidate+0x1eu);','bool valid=signed32(start-base)<word(m,candidate+0x1eu);'),
            'cascade_salt':('if(salt!=0&&salt==signed32(dependency)>>16&&read(m,related+0x48u,1))','if(salt!=0&&read(m,related+0x48u,1))'),
            'reclaim_unit_ref':('if(read(m,speech+0x1cu)==datum)write(m,speech+0x1cu,none);','write(m,speech+0x1cu,none);'),
            'head_read_order':('const auto pool=read(m,0x505e70u);\n    do {','const auto pool=early_pool;\n    do {')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=60)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered speech store','ordered speech input','speech result differs','speech FP differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected speech mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_speech_queue LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Speech mutation location changed: '+name)
                core=project/(name+'.cpp');changed=text.replace(*mutation)
                if name=='head_read_order':changed=changed.replace('    auto link=queue+0x7d0u;','    const auto early_pool=read(m,0x505e70u);\n    auto link=queue+0x7d0u;')
                core.write_text(changed)
            target='campaign_speech_queue_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}" "{ROOT}/src/core/native_tu15_data_array_delete.cpp")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_speech_queue_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['include blocking end equality','allow priority threshold equality','reject permitted delay equality','ignore dependency salt mismatch','clear another unit reference','read pool before actual head-empty test'],
            excluded=['full213D30/1BD8F0/constant34speech dispatch','candidate collection214EA0/publication215390','unmapped input faults','no extended scratch on timing paths','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
