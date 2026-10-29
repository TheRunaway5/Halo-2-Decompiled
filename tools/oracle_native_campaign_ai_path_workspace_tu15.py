#!/usr/bin/env python3
"""Unchanged physical TU1.5 27B010 workspace initialization; no supplied outcomes."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
WORK,MAIN,POLICY,TRANSPORT,BSP=0x1000100,0x1020000,0x1020100,0x1020200,0x1021000
sha=common.sha
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 (p/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{p/'text.bin'}"
.section .bank,"aw"
.space 262144
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
''');(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{s.va:x}:{{*(.original)}} .bank 0x1000000:{{*(.bank)}} .binding 0x4ec000:{{*(.binding)}} .stack 0x{geometry.STACK:x}:{{*(.stack)}} .stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_geometry_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs():
 rows=[]
 def make(name,policy=POLICY,transport=TRANSPORT,main=MAIN,count=1,context=0,shift=0):
  n=len(rows);work=WORK+shift;regions={work-0x10:bytearray((k*13+0x53)&255 for k in range(0x120)),work+0x110b0:bytearray(b'\xab\xcd'),work+0x140b8:bytearray((k*7+0x83)&255 for k in range(36)),work+0x14188:bytearray(b'\xef\x91'),MAIN:bytearray().join(struct.pack('<I',0x5a000000+k*137+n*3)for k in range(20)),POLICY:bytearray().join(struct.pack('<I',0x72000000+k*97+n*5)for k in range(7)),TRANSPORT:bytearray().join(struct.pack('<I',0x81000000+k*59+n*7)for k in range(7)),BSP:bytearray(0xd0),0x4ecc18:bytearray(struct.pack('<I',BSP))}
  struct.pack_into('<iI',regions[BSP],0xc4,count,0x1022000+n*0x10)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=work;gp[1]=policy;gp[3]=transport;gp[4]=geometry.SP+n%4*4
  rows.append(dict(id=n,name=name,entry=0x27b010,gp=gp,matrix=main,source=context,workspace=work,parameters=main,policy=policy,transport=transport,application_context=context,banks=[dict(address=p,before=b.hex())for p,b in sorted(regions.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|n*7%64],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
 for policy in(0,POLICY):
  for transport in(0,TRANSPORT):
   for count in(-1,0,1):
    for context in(0,0xffffffff,0x81122334):make(f'ordinary_{policy:x}_{transport:x}_{count}_{context:x}',policy=policy,transport=transport,count=count,context=context)
 for main in(WORK-4,WORK,WORK+4,WORK+0x54):make(f'alias_main_{main:x}',main=main)
 for policy in(WORK+0x68,WORK+0x70,WORK+0x74,WORK+0x140b8):make(f'alias_policy_{policy:x}',policy=policy)
 for transport in(WORK+0x140b8,WORK+0x140bc,WORK+0x70):make(f'alias_transport_{transport:x}',transport=transport)
 make('alias_all',main=WORK-4,policy=WORK+0x68,transport=WORK+0x70)
 for shift in(1,3):make(f'unaligned_{shift}',shift=shift)
 make('fault_seed_present');make('fault_seed_absent',policy=0,transport=0,count=-1);make('fault_seed_alias',main=WORK-4,policy=WORK+0x68,transport=WORK+0x70)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x27b010,);meta=common.metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    # REP MOVSD performs source read+store in one physical instruction step.
    # Before-read and completed-store prefixes are real; no midpoint is made up.
    if phase=='before_write'and events[n]['instruction']in(0x27b03c,0x27b073,0x27b091):continue
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),owners=meta['owners'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],copy_operand_limit='Forward normal-ABI copies; no PTRACE intra-MOVSD read/store midpoint or MMIO bus micro-order claimed',source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_geometry_tu15.cpp':sha(ROOT/'tools/observe_native_geometry_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical workspace comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
