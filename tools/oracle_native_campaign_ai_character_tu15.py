#!/usr/bin/env python3
"""Unchanged physical TU1.5 inherited character/navigation/movement lookups."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
ENTRIES=(0x1ecd00,0x1ed700,0x27c6c0,0x27c710,0x1ed100)
BASE,POOL,ACTORS,REGISTRY,TAGS,NAV,VEHICLE,SCENARIO,ZONES,ORDERS,SQUADPOOL,SQUADS,ACTIONPOOL,ACTIONS,MOVEMENT=0x1000000,0x1000100,0x1001000,0x1004000,0x1005000,0x1007000,0x1007400,0x1008000,0x1008400,0x1008600,0x1008800,0x1008900,0x1008a00,0x1008b00,0x100a000
sha=common.sha
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'bank':(BASE,0xc000),'scenario':(0x4ec000,0x1000),'tags':(0x4f0000,0x1000),'actor':(0x502000,0x1000),'actions':(0x50e000,0x1000),'squads':(0x52b000,0x1000),'stack':(geometry.STACK,65536)}
 assembly=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
 for name,(_,size)in regions.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
 assembly+='''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''';(p/'reference.s').write_text(assembly)
 (p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{s.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{a:x}:{{*(.{n})}} 'for n,(a,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_geometry_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs():
 rows=[]
 def make(entry,name,actor=0xe3610000,character=0xe24e0000,depth=1,count=2,action='none',squad=True,order=0,zone=0,scenario_count=2,zone_character=0xaabc0002,variant=0,override=0,match=1,vehicle_key=0xfeed1234,squad_datum=0x99fe0000):
  n=len(rows);raw=bytearray(0xc000)
  def put(p,v,width=4):raw[p-BASE:p-BASE+width]=int(v&((1<<(width*8))-1)).to_bytes(width,'little')
  put(POOL+0x44,ACTORS);put(SQUADPOOL+0x44,SQUADS-(squad_datum&0xffff)*0x98);put(ACTIONPOOL+0x44,ACTIONS)
  for k in range(4):put(REGISTRY+k*16+8,TAGS+k*0x400)
  for k,parent in((0,1),(1,3),(2,None),(3,None)):
   tag=TAGS+k*0x400;put(tag+8,0xffffffff if parent is None else 0xe24e0000+parent)
   for offset,pointer in((0x5c,NAV+k*0x40),(0xe4,VEHICLE+k*0x200),(0xa4,MOVEMENT+k*0x100)):
    put(tag+offset,count if k==(0 if depth==0 else 1 if depth==1 else 3 if depth==2 else-1)else 0 if k==0 else-1);put(tag+offset+4,pointer)
   for j in range(3):put(VEHICLE+k*0x200+j*0xb4+4,vehicle_key if j==match else 0xf00d0000+j)
  put(TAGS+2*0x400+0x24,variant,2);put(TAGS+2*0x400+0x2c,override,1)
  if action=='on_null':put(REGISTRY+2*16+8,0)
  for k in range(3):
   record=ACTORS+k*0x888;put(record+0x54,character);put(record+0x858,0xffffffff if action=='none'else 0x198b0000);put(record+0x30,squad_datum if squad else 0xffffffff)
  put(ACTIONS+0x86,int(action not in('none','off')),1);put(ACTIONS+0x88,0xe24e0002)
  put(SQUADS+0x2a,order,2);put(SCENARIO+0x244,ORDERS)
  for k in(-2,0,1):put(ORDERS+k*0x7c+0x20,zone,2)
  put(SCENARIO+0x150,scenario_count);put(SCENARIO+0x154,ZONES)
  put(ZONES+4,zone_character);put(ZONES+12,zone_character)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
  if entry==0x1ecd00:gp[1]=character
  elif entry in(0x1ed700,0x27c710):gp[1]=actor
  elif entry==0x27c6c0:gp[1]=ORDERS
  elif entry==0x1ed100:gp[3]=actor
  else:raise ValueError(entry)
  ranges=((POOL,0x50),(ACTORS,3*0x888),(REGISTRY,0x40),(TAGS,0x1000),(NAV,0x100),(VEHICLE,0x800),(SCENARIO,0x260),(ZONES,0x10),(ORDERS-0x100,0x200),(SQUADPOOL,0x50),(SQUADS,0x98),(ACTIONPOOL,0x50),(ACTIONS,0xd4),(MOVEMENT,0x400))
  banks=[dict(address=p,before=raw[p-BASE:p-BASE+size].hex())for p,size in ranges]
  banks +=[dict(address=p,before=struct.pack('<I',v).hex())for p,v in((0x4ecc20,SCENARIO),(0x4f0414,REGISTRY),(0x5020c8,POOL),(0x50eee0,ACTIONPOOL),(0x52be7c,SQUADPOOL))]
  rows.append(dict(id=n,name=name,entry=entry,matrix=vehicle_key,source=0,actor=actor,character=character,order_address=ORDERS,vehicle_character=vehicle_key,gp=gp,banks=banks,before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|n*7%64],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
 for e in ENTRIES:
  for depth in(0,1,2,3):
   for count in(-1,0,1,2):make(e,f'inherited_{e:x}_{depth}_{count}',depth=depth,count=count,action='on')
 for e in(0x1ecd00,0x1ed700,0x1ed100):make(e,f'character_none_{e:x}',character=0xffffffff)
 for action in('none','off','on','on_null'):
  for squad in(False,True):
   for order in(-2,-1,0):
    for e in(0x27c710,0x1ed100):make(e,f'policy_{e:x}_{action}_{squad}_{order}',action=action,squad=squad,order=order)
 for zone in(-32768,-2,-1,0,1,2,32767):
  for count in(-1,0,1,2):make(0x27c6c0,f'order_bounds_{zone}_{count}',zone=zone,scenario_count=count)
 for e in(0x27c6c0,0x27c710):make(e,f'zone_none_{e:x}',zone_character=0xffffffff)
 for e in(0x27c710,0x1ed100):make(e,f'nonnull_lowFFFF_squad_{e:x}',squad_datum=0x99feffff)
 for override in(0,1,2,3,0x7f,0x80,0xff):
  for variant in(0,1,2,0x7fff,0x8000,0xffff):make(0x1ed100,f'variant_{override}_{variant:x}',override=override,variant=variant,action='on')
 for depth in(0,1,2,3):
  for match in(0,1,2,3):make(0x1ed700,f'vehicle_{depth}_{match}',depth=depth,match=match)
 for e in ENTRIES:
  for salt in(0,0xffff):
   for index in(0,1,2):make(e,f'datum_{e:x}_{salt:x}_{index}',actor=salt<<16|index,character=salt<<16|0)
  make(e,f'fault_seed_{e:x}',depth=2,action='on')
 for e in(0x27c710,0x1ed100):make(e,f'fault_seed_fallback_{e:x}',action='on_null',order=-2)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for n in range(len(seed['reads'])):
   row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_before_read_'+str(n),fault=dict(phase='before_read',ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 assert all(not r['writes']for r in rows)
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),owners=meta['owners'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_geometry_tu15.cpp':sha(ROOT/'tools/observe_native_geometry_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical character comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
