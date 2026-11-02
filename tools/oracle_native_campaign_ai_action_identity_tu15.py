#!/usr/bin/env python3
"""Unchanged physical action-history, goal-definition and salted-lookup owners.

Only retained source fields are declared. No action result, private register
state, instruction, dependency return or resumed fault outcome is supplied.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_ai_area_tu15 as areas
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x26c510,0x26c850,0x277d40,0x16e940)
BASE,ACTOR_POOL,ACTORS,ALT_ACTORS,SCENARIO,ZONES,GOALS,DYNAMIC_POOL,DYNAMIC_ROWS,POOL,ROWS,ALT_ROWS=(areas.BASE+n for n in
    (0,0x100,0x1000,0x4000,0x6000,0x7000,0x8000,0x9000,0xa000,0xc000,0xd000,0xe000))


class Banks:
    def __init__(self,seed):
        raw=bytearray(0x10000)
        raw[0x1000:0x6000]=bytes((n*37+seed*13+7)&255 for n in range(0x5000))
        self.data={BASE:raw}

    def put(self,address,value,width=4):
        self.bytes(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def bytes(self,address,raw):
        overlaps=[(p,b)for p,b in self.data.items()if p<address+len(raw)and address<p+len(b)]
        if len(overlaps)==1 and overlaps[0][0]<=address and address+len(raw)<=overlaps[0][0]+len(overlaps[0][1]):
            p,b=overlaps[0];b[address-p:address-p+len(raw)]=raw;return
        low=min([address]+[p for p,b in overlaps]);high=max([address+len(raw)]+[p+len(b)for p,b in overlaps]);data=bytearray(high-low)
        for p,b in overlaps:data[p-low:p-low+len(b)]=b;del self.data[p]
        data[address-low:address-low+len(raw)]=raw;self.data[low]=data

    def rows(self):
        return [dict(address=p,before=b.hex())for p,b in sorted(self.data.items())]


def inputs():
    rows=[]
    def make(kind,name,actor=0xe3610000,goal=0x81020005,flag=0,ring=0,matches=(),none=0xffffffff,
             count=4,index=1,salt=0xe361,datum_salt=0xe361,stride=8,owner=0,active=True,
             changes=(),timeline=()):
        n=len(rows);banks=Banks(n)
        banks.put(0x47d6c0,none);banks.put(0x5020c8,ACTOR_POOL);banks.put(ACTOR_POOL+0x44,ACTORS)
        record=ACTORS+(actor&0xffff)*0x888
        if kind=='history':
            if not(BASE<=record and record+0x888<=BASE+0x10000):
                banks.bytes(record,bytes((k*19+n*5+11)&255 for k in range(0x888)))
            banks.put(record+0x3fe,ring,2)
            for slot in range(4):
                banks.put(record+0x400+slot*6,0x91+slot,1)
                banks.put(record+0x402+slot*6,goal if slot in matches else 0x19700000+slot)
        elif kind in ('goal','dynamic'):
            goal=(((owner|0x8000)<<16)|(index&0xffff)) if kind=='dynamic' else ((owner<<16)|(index&0xffff))
            banks.put(0x4ecc20,SCENARIO);banks.put(SCENARIO+0x16c,ZONES)
            banks.put(ZONES+(owner&0xffff)*0x38+0x28,count)
            banks.put(ZONES+(owner&0xffff)*0x38+0x2c,GOALS)
            banks.put(0x52d87c,DYNAMIC_POOL);banks.put(DYNAMIC_POOL+0x44,DYNAMIC_ROWS)
            dynamic_record=DYNAMIC_ROWS+(owner&0x7fff)*0x484
            banks.put(dynamic_record+8,count,2)
            if index>=0:
                banks.put(dynamic_record+0x40+(index>>5)*4,(1<<(index&31))if active else 0)
        elif kind=='lookup':
            banks.put(POOL+0x38,count);banks.put(POOL+0x24,stride);banks.put(POOL+0x44,ROWS)
            banks.put((ROWS+stride*(index&0xffff))&0xffffffff,salt,2)
            banks.put((ALT_ROWS+stride*(index&0xffff))&0xffffffff,salt,2)
        for address,value,width in changes:banks.put(address,value,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        if kind=='history':entry=0x26c510;gp[0]=actor;gp[3]=(gp[3]&0xffffff00)|(flag&255);arguments=[goal]
        elif kind in ('goal','dynamic'):entry=0x26c850 if kind=='goal' else 0x277d40;arguments=[goal]
        else:entry=0x16e940;gp[0]=((datum_salt&0xffff)<<16)|(index&0xffff);gp[6]=POOL;arguments=[]
        depth=(0,1,3,8)[n%4];top=n%8
        values=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
        mmx=n%9==0
        if mmx:
            depth=8;top=0;values=[struct.pack('<QH',0x5a00000000000000+n*31+k,0xffff).hex()for k in range(8)]
        rows.append(dict(id=n,name=name,kind=kind,entry=entry,gp=gp,arguments=arguments,
            actor=actor,goal=goal,flag=flag&255,pool=POOL,datum=gp[0]if kind=='lookup'else 0,
            banks=banks.rows(),input_timeline=list(timeline),active_mmx=mmx,
            before_fp=[0x27f|((n%4)<<10),(top<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=values))
        return rows[-1]
    for flag in (0,1,2,0x80,0xff):
        for matches in ((),(0,),(1,),(3,),(0,2,3)):
            make('history','history_'+str(flag)+'_'+str(matches),flag=flag,matches=matches)
    for ring in (-32768,-9,-5,-4,-3,-2,-1,0,1,2,3,4,32767):
        make('history','ring_'+str(ring),ring=ring,flag=0xa5)
    for salt in (0,1,0xe361,0xffff):
        for index in (0,1,3):make('history','actor_'+str(salt)+'_'+str(index),actor=(salt<<16)|index)
    make('history','none_goal',goal=0xffffffff)
    make('history','authored_none',goal=0x12345678,none=0x12345678)
    make('history','different_salt_same_goal_index',changes=((ACTORS+0x402,0x91020005,4),))
    for owner in (0,1,0x7fff):
        for index in (-32768,-2,-1,0,1,2,31,32767):
            for count in (-1,0,2,32768):
                make('goal',f'static_{owner}_{index}_{count}',owner=owner,index=index,count=count)
    for owner in (0,1,0x7fff):
        for index in (-2,-1,0,1,31,32,63,64,32766):
            for active in (False,True):
                make('dynamic',f'dynamic_{owner}_{index}_{active}',owner=owner,index=index,count=32767,active=active)
    for index,count in ((0,0),(0,-1),(1,1),(2,2),(32,33),(31,32)):
        for active in (False,True):make('dynamic',f'dynamic_bound_{index}_{count}_{active}',index=index,count=count,active=active)
    for owner,index,count,active in ((0,0,1,True),(1,32,33,True),(0x7fff,31,32,False)):
        row=make('dynamic',f'encoded_dynamic_{owner}_{index}',owner=owner,index=index,count=count,active=active)
        row.update(kind='goal',entry=0x26c850)
    row=make('dynamic','encoded_none',owner=0x7fff,index=-1);row.update(kind='goal',entry=0x26c850)
    for count in (-1,0,1,4,65535,65536,0x7fffffff):
        for index in (0,1,3,65535):make('lookup',f'lookup_bound_{count}_{index}',count=count,index=index)
    for salt in (0,1,0x7fff,0x8000,0xe361,0xffff):
        for datum_salt in (1,0x7fff,0x8000,0xe361,0xffff):
            make('lookup',f'lookup_salt_{salt}_{datum_salt}',salt=salt,datum_salt=datum_salt)
    for stride in (0,1,3,8,0xffffffff):make('lookup','stride_'+hex(stride),stride=stride)
    make('lookup','none_datum',index=0xffff,datum_salt=0xffff)
    def publication(address,occurrence,target,value,width=4):
        return dict(on_read=dict(address=address,occurrence=occurrence),writes=[dict(address=target,bytes=(value&((1<<(8*width))-1)).to_bytes(width,'little').hex())])
    make('history','fresh_ring',timeline=(publication(ACTORS+0x3fe,2,ACTORS+0x3fe,3,2),))
    make('history','fresh_later_match',matches=(0,),timeline=(publication(ACTORS+0x402,1,ACTORS+0x402+2*6,0x81020005),))
    make('lookup','fresh_lookup_rows',timeline=(publication(POOL+0x38,1,POOL+0x44,ALT_ROWS),))
    make('lookup','captured_salt',timeline=(publication(ROWS+8,1,ROWS+8,0,2),))
    for kind in ('history','goal','dynamic','lookup'):make(kind,'fault_seed_'+kind)
    make('history','fault_seed_history_match',matches=(0,2,3))
    make('history','fault_seed_history_negative',ring=-2,flag=0xa5)
    row=make('history','unmapped_actor_pool',changes=((0x5020c8,0xdead0000,4),))
    row['expected_fault']=True
    row=make('lookup','unmapped_lookup_pool');row.update(pool=0xdead0000,expected_fault=True);row['gp'][6]=row['pool']
    row=make('goal','unmapped_scenario_before_negative_index',index=-1,changes=((0x4ecc20,0xdead0000,4),))
    row['expected_fault']=True
    row=make('dynamic','unmapped_dynamic_pool',index=-1,changes=((0x52d87c,0xdead0000,4),))
    row['expected_fault']=True
    # The count remains in the last mapped bank page; the actual bitset read
    # crosses into an unmapped page. Do not declare/map the missing mask.
    row=make('dynamic','unmapped_dynamic_mask',index=0,count=1,
        changes=((DYNAMIC_POOL+0x44,BASE+0x800000-0xc,4),(BASE+0x800000-4,1,2)))
    row['expected_fault']=True
    return rows


def prepare(image,metadata,scratch,declared):
    owned=areas.prepare(image,metadata,scratch)
    known=[(BASE,BASE+0x800000),(geometry.STACK,geometry.STACK+65536)]
    for name in ('.text','.rdata','.data'):
        section=image.section(name);known.append((section.va,0x530000 if name=='.data'else section.va+section.rsize))
    pages=[]
    for case in declared:
        for bank in case['banks']:
            start=bank['address'];end=start+len(bank['before'])//2
            if any(a<=start and end<=b for a,b in known):continue
            pages.append((start&~4095,(end+4095)&~4095))
    merged=[]
    for a,b in sorted(pages):
        if merged and a<=merged[-1][1]:merged[-1]=(merged[-1][0],max(b,merged[-1][1]))
        else:merged.append((a,b))
    assembly=(owned/'reference.s').read_text();linker=(owned/'link.ld').read_text()
    for n,(a,b)in enumerate(merged):
        assembly+=f'.section .far{n},"aw"\n.space {b-a}\n'
        linker=linker.replace('\n}',f'\n.far{n} 0x{a:x}:{{*(.far{n})}}\n}}')
    (owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(linker)
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],
                    ['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],
                    ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_action_identity_tu15.cpp'),'-o',str(owned/'observer')]):
        result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,result.stderr
    return owned


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify',type=Path);parser.add_argument('--complete-only',action='store_true')
    args=parser.parse_args();assert not args.output.exists();assert common.sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;metadata=common.metadata(image);code=_Code(image,'.text')
    metadata['private_stack']=dict(begin=geometry.STACK,bytes=65536)
    for site in metadata['sites']:
        if code.at(site['instruction']).mnemonic in ('cmp','test'):
            for operand in site['operands']:operand.update(read=True,write=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);declared=inputs();owned=prepare(image,metadata,scratch,declared)
    complete=geometry.physical(owned,declared,'complete');prefix=[]
    if not args.complete_only:
        for seed in (r for r in complete if r['name'].startswith('fault_seed')):
            for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
                for ordinal in range(len(events)):
                    row=copy.deepcopy(seed);row.update(id=len(complete)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
    cases=complete+geometry.physical(owned,prefix,'prefix')if prefix else complete
    reached=set().union(*(set(r['executed_sites'])for r in complete));sites={s['instruction']for s in metadata['sites']}
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=metadata['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=cases)
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Original action-identity repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,original_sha256=NEW_PIN,cases=len(cases),complete_calls=sum(r['completed']for r in complete),
        true_memory_faults=sum(not r['completed']for r in complete),fault_prefixes=len(prefix),
        original_sites=len(sites),reached_sites=len(sites&reached),unreached_sites=[hex(pc)for pc in sorted(sites-reached)],
        strict_retained_operands=True,all_original_fp_simd_mmx_preserved=True,fixture=str(args.output),fixture_bytes=len(data),fixture_sha256=common.sha(args.output),
        independent_repeat_identical=True if args.verify else None,native_implementation_qualified=False,actual_world_route_qualified=False,
        source_sha256={str(p.relative_to(ROOT)):common.sha(p)for p in (Path(__file__),ROOT/'tools/observe_native_campaign_ai_action_identity_tu15.cpp')},
        owned_scratch=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('cases','complete_calls','original_sites','reached_sites','unreached_sites')}))


if __name__=='__main__':main()
