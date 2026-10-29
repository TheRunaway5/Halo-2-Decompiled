#!/usr/bin/env python3
"""Unchanged physical TU1.5 behavior82 initializer reference, declared inputs only."""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,POOL,ROWS,ACTION=0x1000000,0x1000100,0x1001000,0x1006000
sha=common.sha

def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 (p/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{p/'text.bin'}"
.section .bank,"aw"
.space 65536
.section .binding,"aw"
.space 4096
.section .stack,"aw"
.space 65536
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''');(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{s.va:x}:{{*(.original)}} .binding 0x502000:{{*(.binding)}} .bank 0x{BASE:x}:{{*(.bank)}} .stack 0x{geometry.STACK:x}:{{*(.stack)}} .stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_geometry_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p

def inputs():
 rows=[]
 def make(datum=0xe3610000,rows_base=ROWS,action=ACTION,name='ordinary'):
  n=len(rows);raw=bytearray((k*13+n*7+3)&255 for k in range(0x8000));struct.pack_into('<I',raw,POOL-BASE+0x44,rows_base)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=0x1c9ce0;gp[4]=geometry.SP+n%4*4
  binding=bytearray([0x47]*4096);struct.pack_into('<I',binding,0xc8,POOL)
  banks=[dict(address=BASE,before=raw.hex()),dict(address=0x502000,before=binding.hex()),dict(address=gp[4]+4,before=struct.pack('<II',datum,action).hex())]
  if geometry.STACK<=action<geometry.STACK+65536:banks.append(dict(address=action,before=bytes([0x83]*64).hex()))
  rows.append(dict(id=n,name=f'{name}_{n}',entry=0x1c9ce0,matrix=datum,source=action,gp=gp,banks=sorted(banks,key=lambda b:b['address']),before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
 for salt in(0,1,0xe361,0xffff):
  for index in range(4):make((salt<<16)|index)
 for alignment in(1,3):make(rows_base=ROWS+alignment,action=ACTION+alignment,name='unaligned')
 for offset in(0x500,0x598,0x59c):make(action=ROWS+offset,name='alias_actor')
 make(action=POOL+0x34,name='alias_header');make(action=0x5020b8,name='alias_binding')
 make(action=geometry.SP+0x44,name='stack_action')
 make(name='fault_seed');make(action=ROWS+0x59c,name='fault_seed_alias')
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x1c9ce0,);meta=common.metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));assert len(visited)==21
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 code=_Code(image,'.text');assert int.from_bytes(image.read(0x47b478+0x82*4,4),'little')==0x48b770;assert int.from_bytes(image.read(0x48b770+0x18,4),'little')==0x1c9ce0
 assert code.at(0x1b0d1c).mnemonic=='call'and code.at(0x1b0d1c).op_str=='eax';assert code.at(0x1b0d1e).op_str=='dl, al';assert code.at(0x1b0d20).op_str=='dl, dl'
 caller=[];pc=0x1b0d1e
 while pc<=0x1b0e10:
  i=code.at(pc);caller.append(dict(instruction=pc,bytes=i.bytes.hex(),operation=i.mnemonic,operands=i.op_str));pc+=i.size
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),owners=meta['owners'],original_sha256=NEW_PIN,fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),fixture=str(a.output),behavior_registry=dict(id=0x82,table=0x47b478,record=0x48b770,initializer_offset=0x18),caller_continuation=caller,owed_return='boolAL only;1B0D1E copiesAL toDL,1B0D20 testsDL, all further volatile register use follows own definitions',owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_geometry_tu15.cpp':sha(ROOT/'tools/observe_native_geometry_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical original behavior-init comparisons,21sites; fixture{a.output.stat().st_size}bytes')
if __name__=='__main__':main()
