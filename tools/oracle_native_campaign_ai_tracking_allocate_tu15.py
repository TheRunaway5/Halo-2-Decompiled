#!/usr/bin/env python3
"""Physical unchanged tracking selection/allocation, pressure and reset oracle."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=0x1000000
ACTORS,PROPERTIES,QUERIES,GROUPS,AI,CLOCK=BASE+0x100,BASE+0x200,BASE+0x300,BASE+0x400,BASE+0x500,BASE+0x900
ABITS,QBITS,AROWS,PROWS,QROWS,GROWS,DEFAULT=BASE+0xa00,BASE+0xb00,BASE+0x1000,BASE+0x6000,BASE+0x8000,BASE+0xc000,BASE+0xf000
ENTRIES=(0x266280,0x267870,0x266150,0x1ec970,0x2677d0,0x266490,0x2673a0,0x16e6a0,0x16e8a0)
sha=common.sha
def prepare(image,meta,scratch):
    # Reuse the physical ELF layout, replace only its test observer binary.
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_tracking_allocate_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    return owned
def inputs(image):
    cases=[]
    def make(entry=0x266280,name='allocate',cache=(),priorities=(),times=(),query_count=0,capacity=8,
             actor_count=1,bitmap=None,enabled=1,last=0,now=1000,updated=0,category=0,actor_category=1,
             active=0,property_chain=0,poison=0,threshold=3,changes=(),fp=None,live=0,
             policy=0,iterator_cursor=0xffffffff,iterator_active=0,default=DEFAULT,extension=QROWS+0x70):
        n=len(cases);banks={BASE:bytearray((k*13+7)&255 for k in range(0x10000))}
        for address in (0x502000,0x50e000,0x51d000,0x474000,0x47d000,0x46a000):banks[address]=bytearray(image.read(address,4096))
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError((hex(p),w))
        for p,v in [(0x5020c8,ACTORS),(ACTORS+0x44,AROWS),(ACTORS+0x24,0x888),(ACTORS+0x38,actor_count),(ACTORS+0x20,8),(ACTORS+0x48,ABITS),(ABITS,(1<<actor_count)-1 if bitmap is None else bitmap),(0x50eef0,PROPERTIES),(PROPERTIES+0x44,PROWS),(PROPERTIES+0x24,0x3c),(PROPERTIES+0x38,16),(0x50eeec,QUERIES),(QUERIES+0x44,QROWS),(QUERIES+0x24,0x124),(QUERIES+0x20,capacity),(QUERIES+0x34,query_count),(QUERIES+0x38,query_count),(QUERIES+0x3c,query_count),(QUERIES+0x48,QBITS),(QBITS,(1<<query_count)-1),(0x50eef4,GROUPS),(GROUPS+0x44,GROWS),(0x5020a8,AI),(AI+0x370,last),(0x51df54,CLOCK),(CLOCK+8,now),(0x474ce8,default),(0x474d04,default)]:put(p,v)
        put(QUERIES+0x40,0x8000,2);put(QUERIES+0x2a,poison,1);put(AI+1,enabled,1);put(CLOCK+2,30,2);put(0x474c24,category,2)
        for i in range(8):
            actor=AROWS+i*0x888;put(actor,0xe100,2);put(actor+9,active,1);put(actor+0x3e,actor_category,2);put(actor+0x10,updated)
            put(actor+0x58,0xe2000000 if i==0 and property_chain else 0xffffffff)
            for k in range(8):put(actor+0x5c+k*4,cache[k]if i==0 and k<len(cache)else 0xffffffff)
        for i in range(16):
            member=PROWS+i*0x3c;put(member,0xe200,2);put(member+4,0xe1000000);put(member+8,0xe4000000+i)
            put(member+0x14,0xe3000000+i if i<query_count else 0xffffffff);put(member+0x18,policy,2)
            put(member+0x1a,priorities[i]if i<len(priorities)else 0,2);put(member+0x24,3,2)
            put(member+0x2c,0xe2000000+i+1 if i+1<property_chain else 0xffffffff)
            put(QROWS+i*0x124,0xe300,2);put(QROWS+i*0x124+4,times[i]if i<len(times)else 100+i)
            put(GROWS+i*0xc4+0x58,times[i]if i<len(times)else 100+i)
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4;gp[0]=0xe1000000 if entry==0x266280 else extension
        iterator=geometry.SP+0x400;iterator_before=struct.pack('<IIIIB3xI',0x77777777,ACTORS,0x12345678,iterator_cursor,iterator_active,0x98765432).hex()
        if entry==0x2677d0:matrix=0xe1000000
        elif entry==0x266280:matrix=0xe200000f
        else:matrix=0
        if entry==0x1ec970:gp[6]=iterator
        if entry==0x266490:gp[0]=0xe1000000;gp[3]=0xe2000000
        cases.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=matrix,source=threshold,
            actor=0xe1000000,perception=0xe200000f,priority=threshold,extension=extension,
            iterator_address=iterator,iterator_before=iterator_before,
            banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],
            before_fp=fp or [0x27f,0,0x1f80],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3).hex()for k in range(live)]))
    for hole in range(9):make(cache=tuple(0xe2000000+i for i in range(hole)),name='first_empty_or_full')
    for invalid in ('extent','zero_salt','wrong_salt'):
        edits={'extent':[(PROPERTIES+0x38,0,4)],'zero_salt':[(PROWS,0,2)],'wrong_salt':[(PROWS,0xe201,2)]}[invalid]
        make(cache=(0xe2000000,),changes=edits,name='invalid_'+invalid)
    for threshold in (-32768,-1,0,1,2,3,32767):
        for priorities,times in [((1,)*8,(100,)*8),((2,1,1,2,2,2,2,2),(0,200,100,0,0,0,0,0)),((-1,0,1,2,3,4,5,6),(-2,-1,0,1,2,3,4,5))]:
            make(cache=tuple(0xe2000000+i for i in range(8)),priorities=priorities,times=times,threshold=threshold,name='priority_timestamp_selection')
    for query_count,capacity in ((0,0),(0,1),(1,1),(8,8)):
        for poison in (0,8):make(cache=(0xe2000000,)if query_count else (),query_count=query_count,capacity=capacity,poison=poison,name='real_replacement')
    for kind in (0,1,8):
        for state in (-1,0,1,2,3,4):make(entry=0x266490,query_count=1,cache=(0xe2000000,0xe2000000),policy=kind,changes=[(PROWS+0x24,state,2)],name='standalone_retire_low_states')
    for count in (0,1,2,5,8):
        make(entry=0x2677d0,query_count=count,property_chain=count,cache=(0xe2000000,0xe2000000),poison=8,name='actor_retirement_count')
    for enabled in (0,1):
        for cursor in (0xffffffff,0,2,7,0x7fffffff,0xfffffffe):
            for active_only in (0,1):make(entry=0x1ec970,enabled=enabled,actor_count=4,bitmap=0b1010,iterator_cursor=cursor,iterator_active=active_only,active=cursor&1,name='private_actor_iterator')
    for offset in (0,1,3):
        for default in (DEFAULT,QROWS+0x70+0x94,QROWS+0x70+0x98,QROWS+0x70+0x9c):
            make(entry=0x266150,extension=QROWS+0x70+offset,default=default+offset,name='extension_defaults_alias')
    for enabled,last,now,category,actor_category,updated,active in [(0,0,1000,0,1,0,0),(1,1000,1000,0,1,0,0),(1,0,1000,0,1,1000,0),(1,0,1000,0,0,1000,0),(1,0,1000,0,1,0,1),(1,0,1000,0,1,0,0)]:
        for count in (0,1,5):make(entry=0x267870,enabled=enabled,last=last,now=now,category=category,actor_category=actor_category,updated=updated,active=active,query_count=count,property_chain=count,poison=8,name='three_pass_policy')
    for cw in (0x7f,0x27f,0x37f):
        for rounding in range(4):
            for bits in (0x3eaaaaab,0x7fc12345,0x7f800000,0x80000001):
                make(entry=0x267870,query_count=1,property_chain=1,changes=[(0x46a108,bits,4),(CLOCK+2,-32768 if bits&1 else 32767,2)],fp=[cw|(rounding<<10),((rounding+1)<<11)|1,0x1f80|(rounding<<13)|(0x8040 if bits&1 else 0)],live=(0,1,7)[rounding%3],name='tick_conversion_policy')
    make(query_count=1,capacity=1,property_chain=1,name='real_pressure_retry',changes=[(AROWS+0x58,0xe2000000,4)])
    for kwargs in [dict(name='fault_seed_allocate'),dict(name='fault_seed_replace',cache=tuple(0xe2000000+i for i in range(8)),query_count=8,capacity=8,priorities=(1,)*8,times=(100,)*8,poison=8),dict(entry=0x267870,name='fault_seed_sweep',query_count=1,property_chain=1,poison=8),dict(entry=0x266150,name='fault_seed_reset',default=QROWS+0x70+0x98),dict(entry=0x1ec970,name='fault_seed_iterator',bitmap=0b1010,actor_count=4)]:make(**kwargs)
    return cases
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists()
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image)
    for site in meta['sites']:
        if site['bytes'][:4]in('f3aa','f3ab'):site['repeat_count_register']=1
        if site['instruction']in(0x16e6c5,0x1ec9b4):assert site['bytes']in('852c8b','85048b');site['operands'][0]['read']=True
    owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Tracking allocation independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_tracking_allocate_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_tracking_allocate_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
