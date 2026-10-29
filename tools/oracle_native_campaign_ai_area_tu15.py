#!/usr/bin/env python3
"""Unchanged physical full1B6F20/1C5870 area-query closure, no supplied callees.

Declared input timelines only modify input records after a completed original
read; they never patch instructions, guest registers, private iterators, callee
results or resumed prefixes. Persistent query writes must remain empty.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,ACTORPOOL,ACTORS,SQUADPOOL,SQUADS,NEWSQUADS,SCENARIO,NEWSCENARIO,DEFS,ORDERS,ALTORDERS,PRIMARY,SECONDARY,ZONES,NEWZONES,AREAS=tuple(0x1000000+x for x in(0,0x100,0x1000,0x3000,0x4000,0x5000,0x6000,0x6800,0x7000,0x8000,0x8800,0x9000,0xa000,0xb000,0xb800,0xc000))
ENTRIES=(0x1b6f20,0x1c5870,0x20d180,0x20d2d0)
sha=common.sha

def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();assembly=['.code32'];sections=[]
 for n,name in enumerate(('.text','.rdata','.data')):
  s=image.section(name);data=image.section_bytes(s);f=p/f'section{n}.bin';f.write_bytes(data)
  assembly += [f'.section .original{n},"'+('ax"'if n==0 else'aw"'),f'.incbin "{f}"']
  if name=='.data':assembly += [f'.space {0x530000-s.va-len(data)}']
  sections += [f'.original{n} 0x{s.va:x}:{{*(.original{n})}}']
 assembly += ['.section .bank,"aw"','.space 8388608','.section .stack,"aw"','.space 65536','.section .stop,"ax"','int3','.section .bridge,"ax"','.globl _start','_start:','fninit','int3','jmp _start']
 sections += [f'.bank 0x{BASE:x}:{{*(.bank)}}',f'.stack 0x{geometry.STACK:x}:{{*(.stack)}}',f'.stop 0x{geometry.STOP:x}:{{*(.stop)}}','.bridge 0x70000000:{*(.bridge)}']
 (p/'reference.s').write_text('\n'.join(assembly)+'\n');(p/'link.ld').write_text('ENTRY(_start)\nSECTIONS{\n'+'\n'.join(sections)+'\n}\n');(p/'metadata.json').write_text(json.dumps(meta))
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_area_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p

def inputs():
 rows=[]
 def make(name='default',path='default',changes=(),mode=0,enabled=1,fallback=0xffffffff,datum=0xe3610000,squad=0,order=0,selector=0,length=2,timeline=(),far=()):
  for entry in(0x1c5870,0x1b6f20):
   n=len(rows);raw=bytearray(0x12000)
   def put(at,value,width=4):
    assert BASE<=at and at+width<=BASE+len(raw),(hex(at),width)
    raw[at-BASE:at-BASE+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
   put(ACTORPOOL+0x44,ACTORS);put(SQUADPOOL+0x44,SQUADS)
   for a in range(3):
    put(ACTORS+a*0x888+0x225,enabled,1);put(ACTORS+a*0x888+0x26c,0xffffffff if mode==0 else (0 if mode==1 else mode));put(ACTORS+a*0x888+0x30,squad,2)
   for scenario in(SCENARIO,NEWSCENARIO):
    put(scenario+0x164,DEFS);put(scenario+0x168,4);put(scenario+0x16c,ZONES);put(scenario+0x244,ORDERS)
   for i in range(4):
    put(DEFS+i*0x74+0x38,0,2)
    for zones in(ZONES,NEWZONES):put(zones+i*0x38+0x30,2);put(zones+i*0x38+0x34,AREAS+i*0x800)
    for area in range(4):put(AREAS+i*0x800+area*0x88+0x20,(area+i)&1)
    for rowsbase in(SQUADS,NEWSQUADS):
     row=rowsbase+i*0x98;put(row+0x2a,0xffff if path=='default'else order,2);put(row+0x30,length,1);put(row+0x60,selector,1);put(row+0x80,0 if path=='pairs'else 0xffffffff)
     for pair in range(4):put(row+0x32+pair*4,0,2);put(row+0x34+pair*4,pair,2)
    for ordersbase in(ORDERS,ALTORDERS):
     row=ordersbase+i*0x7c;put(row+0x24,0,1);put(row+0x4e,0xffff,2);put(row+0x54,length);put(row+0x58,PRIMARY+i*0x100);put(row+0x5c,length);put(row+0x60,SECONDARY+i*0x100)
    for pointer in(PRIMARY+i*0x100,SECONDARY+i*0x100):
     for k in range(4):put(pointer+k*8,k%2*2,1);put(pointer+k*8+4,0,2);put(pointer+k*8+6,k,2)
   for at,value,width in changes:put(at,value,width)
   gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
   live=(0,1,3,8)[n%4];top=n%8;values=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(live)]
   active_mmx=n%7==0
   if active_mmx:top=0;values=[struct.pack('<QH',0x5a00000000000000+n*31+k,0xffff).hex()for k in range(8)]
   args=[datum]+([0xdeadbeef]if entry==0x1b6f20 else[])
   banks=[dict(address=BASE,before=raw.hex()),dict(address=0x5020c8,before=struct.pack('<I',ACTORPOOL).hex()),dict(address=0x52be7c,before=struct.pack('<I',SQUADPOOL).hex()),dict(address=0x4ecc20,before=struct.pack('<I',SCENARIO).hex()),dict(address=0x47c1a4,before=struct.pack('<I',fallback).hex()),dict(address=gp[4]+4,before=b''.join(struct.pack('<I',v)for v in args).hex())]
   for at,data in far:banks.append(dict(address=at,before=data.hex()))
   rows.append(dict(id=n,name=f'{name}_{entry:x}_{n}',entry=entry,matrix=datum,source=0xdeadbeef,gp=gp,banks=sorted(banks,key=lambda b:b['address']),before_fp=[0x27f|((n%4)<<10),(top<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=values,active_mmx=active_mmx,input_timeline=list(timeline)))
 def change(at,value,width=4):return(at,value,width)
 def mutate(at,occurrence,writes):return dict(on_read=dict(address=at,occurrence=occurrence),writes=[dict(address=a,bytes=(v&((1<<(8*w))-1)).to_bytes(w,'little').hex())for a,v,w in writes])
 for path in('default','order','pairs'):
  for mode in(0,1,0x12340000):make(path,path=path,mode=mode)
 for fallback in(0xffffffff,0xdead8000,0xffff0000,0xabcd006b,0x00000100,0x00000082,0x00000083):
  for enabled in(0,2):make('enabled_default',enabled=enabled,fallback=fallback,squad=0xffff)
 for index in(0,1,2):
  for salt in(0,0xe361,0xffff):make('salted',datum=(salt<<16)|index)
 for zone in(0xffff,0,1):
  for count in(-1,0,1):make('initial_gate',path='default',changes=(change(DEFS+0x38,zone,2),change(ZONES+0x30,count)))
 for indicator in(0xffffffff,0,0x12345678):
  for count in(-1,0,1,2):make('pair_indicator_count',path='pairs',changes=(change(SQUADS+0x80,indicator),),length=count,mode=1)
 make('pair_none_terminal',path='pairs',changes=(change(SQUADS+0x34,0xffff,2),change(AREAS-0x88+0x20,0)))
 make('pair_unsigned_zone',path='pairs',changes=(change(SQUADS+0x32,0xffff,2),),far=((ZONES+0xffff*0x38+0x34,struct.pack('<I',AREAS)),))
 make('default_unsigned_zone',changes=(change(DEFS+0x38,0x8000,2),),far=((ZONES+0x8000*0x38+0x30,struct.pack('<II',1,AREAS)),))
 make('pairs_preempted',path='pairs',changes=(change(SQUADS+0x2a,0xffff,2),change(DEFS+0x38,0xffff,2)))
 for selector in(0,1,255):
  for count in(-1,0,1,2):make('selected_count',path='order',selector=selector,length=count)
 for kind in(0,1,2,32,34,255):make('entry_kind',path='order',length=1,changes=(change(PRIMARY,kind,1),))
 for zone in(0xffff,0x8000,0,3,4):make('zone_bound',path='order',length=1,changes=(change(PRIMARY+4,zone,2),))
 for area in(0xffff,0x8000,0,1,2):make('area_bound',path='order',length=1,mode=1,changes=(change(PRIMARY+6,area,2),))
 make('zero_zonecount_default',changes=(change(SCENARIO+0x168,0),))
 make('zero_zonecount_order',path='order',changes=(change(SCENARIO+0x168,0),))
 make('negative_zonecount_order',path='order',changes=(change(SCENARIO+0x168,0xffffffff),))
 for flags,next_squad in((0,1),(0x20,0xffff),(0x20,1)):
  make('redirect',path='order',changes=(change(ORDERS+0x24,flags,1),change(ORDERS+0x4e,next_squad,2),change(SQUADS+0x98+0x2a,1,2)))
 make('redirect_two',path='order',changes=(change(ORDERS+0x24,0x20,1),change(ORDERS+0x4e,1,2),change(SQUADS+0x98+0x2a,1,2),change(ORDERS+0x7c+0x24,0x20,1),change(ORDERS+0x7c+0x4e,2,2),change(SQUADS+0x130+0x2a,0xffff,2)))
 make('extra_area_flags',changes=(change(AREAS+0x20,0xfffffffe),change(AREAS+0x88+0x20,0xffffffff)))
 make('entry_area_alias',path='order',length=1,changes=(change(ORDERS+0x58,AREAS+0x1c),change(AREAS+0x1c,0,1),change(AREAS+0x20,0)))
 make('fresh_selector',path='order',length=1,timeline=(mutate(SQUADS+0x60,1,[change(SQUADS+0x60,1,1)]),))
 make('fresh_pool_rows',path='order',changes=(change(NEWSQUADS+0x80,0),change(NEWSQUADS+0x30,1,1),change(NEWSQUADS+0x34,1,2)),timeline=(mutate(SQUADPOOL+0x44,1,[change(SQUADPOOL+0x44,NEWSQUADS)]),))
 make('duplicate_zone_word',timeline=(mutate(DEFS+0x38,1,[change(DEFS+0x38,0xffff,2)]),))
 make('fresh_order_count',path='order',changes=(change(PRIMARY,1,1),change(PRIMARY+8,0,1),change(PRIMARY+14,0,2)),timeline=(mutate(ORDERS+0x54,1,[change(ORDERS+0x54,1)]),))
 make('post_init_count_zero',timeline=(mutate(ZONES+0x30,1,[change(ZONES+0x30,0)]),))
 make('fresh_zone_base',mode=1,changes=(change(NEWZONES+0x30,0),),timeline=(mutate(ZONES+0x34,1,[change(SCENARIO+0x16c,NEWZONES)]),))
 make('cached_scenario',mode=1,changes=(change(NEWSCENARIO+0x16c,NEWZONES),change(NEWZONES+0x30,0)),timeline=(mutate(0x4ecc20,2,[change(0x4ecc20,NEWSCENARIO)]),))
 for path in('default','order','pairs'):make('fault_seed_'+path,path=path,mode=1)
 make('fault_seed_timeline',path='order',timeline=(mutate(SQUADS+0x60,1,[change(SQUADS+0x60,1,1)]),))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert not a.output.exists(),'Preserve existing independent fixture';assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(),'complete')
 # Independently repeat original whole calls before native comparison. The
 # comparison ignores only diagnostic instruction counts/sets, not results,
 # bytes, mutable reads or declared input publication history.
 repeated=geometry.physical(owned,copy.deepcopy(inputs()),'repeat');assert rows==repeated,'Original reference repeat differs'
 prefixes=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for n in range(len(seed['reads'])):
   row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefixes),name=seed['name']+'_read_'+str(n),fault=dict(phase='before_read',ordinal=n));prefixes.append(row)
 rows+=geometry.physical(owned,prefixes,'prefix');assert all(not r['writes']for r in rows),'Eligibility closure wrote persistent records'
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,profile='1C5870 fixedflags5/mode0|1 plus1B6F20 behavior selector',cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),read_fault_prefixes=len(prefixes),original_repeat_equal=True,executed_sites=len(visited),unexecuted_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],original_instructions=sum(r['original_instructions']for r in rows),ordered_original_reads=sum(len(r['reads'])for r in rows),persistent_original_writes=0,input_timeline_cases=sum(bool(r['input_timeline'])for r in rows),fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_area_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_area_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('cases','complete_calls','read_fault_prefixes','executed_sites','fixture_bytes')}))
if __name__=='__main__':main()
