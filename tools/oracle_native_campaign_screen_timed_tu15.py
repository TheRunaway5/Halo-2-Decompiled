#!/usr/bin/env python3
"""Offline unchanged physical TU1.5 timed-screen/DOF reference; declared inputs only."""
import argparse,copy,gzip,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as lifecycle
import validate_native_geometry_tu15 as geometry
ENTRIES=(0x1fc10,0x1fd00)
BANK=0x1000100
sha=lifecycle.sha
bits=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]

def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();assembly=['.code32'];sections=[]
 for n,name in enumerate(('.text','.rdata','.data')):
  s=image.section(name);raw=image.section_bytes(s);f=p/f'section{n}.bin';f.write_bytes(raw)
  assembly += [f'.section .original{n},"'+('ax"'if n==0 else'aw"'),f'.incbin "{f}"']
  if name=='.data':assembly += [f'.space {0x520000-s.va-len(raw)}']
  sections += [f'.original{n} 0x{s.va:x}:{{*(.original{n})}}']
 assembly += ['.section .bank,"aw"','.space 8192','.section .stack,"aw"','.space 65536','.section .stop,"ax"','int3','.section .bridge,"ax"','.globl _start','_start:','fninit','int3','jmp _start']
 sections += ['.bank 0x1000000:{*(.bank)}',f'.stack 0x{geometry.STACK:x}:{{*(.stack)}}',f'.stop 0x{geometry.STOP:x}:{{*(.stop)}}','.bridge 0x70000000:{*(.bridge)}']
 (p/'reference.s').write_text('\n'.join(assembly)+'\n');(p/'link.ld').write_text('ENTRY(_start)\nSECTIONS{\n'+'\n'.join(sections)+'\n}\n');(p/'metadata.json').write_text(json.dumps(meta))
 for c in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_screen_timed_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(c,text=True,capture_output=True);assert r.returncode==0,r.stderr
 return p

def cases():
 rows=[]
 def make(entry=0x1fd00,name='finite',changes=(),now=1.,arguments=None,focus=0x3f800000,present=True,active=True,flags=None,views=1,rounding=None,clock_bits=None,vector=0x44aafc,replacement=0x44ab1c):
  n=len(rows);raw=bytearray((k*13+n*7+3)&255 for k in range(0x3fc))
  def put(off,v):struct.pack_into('<I',raw,off,v)
  for off in range(0,0x180,4):put(off,0)
  raw[0xac]=int(active);raw[0x170]=1;raw[0xa0]=0
  for a,b in ((0xb8,0xbc),(0xd0,0xd4),(0xe4,0xe8),(0xf4,0xf8),(0x124,0x128),(0x178,0x17c)):put(a,bits(0.));put(b,bits(2.))
  for off,v in ((0xb0,.25),(0xb4,.75),(0xc0,.1),(0xc4,.9),(0xc8,.2),(0xcc,.8),(0xd8,3.),(0xdc,1.),(0xe0,3.),(0xec,2.),(0xf0,4.),(0x174,.9)):put(off,bits(v))
  for channel in range(5):put(0xfc+channel*8,bits(.125*(channel+1)));put(0x100+channel*8,bits(.75+channel*.25))
  for i in range(12):put(0x12c+i*4,bits(.25*(i+1)))
  for i in range(4):raw[0x15c+i]=(n if flags is None else flags)>>i&1
  for off in (0x160,0x164,0x168,0x16c):put(off,0x12345678+off)
  for off,v in changes:put(off,v)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+(n%4)*4;gp[0]=0
  args=arguments or ([bits(v)for v in(1.,3.,2.,2.,4.,2.)]if entry==0x1fc10 else[])
  r=n%4 if rounding is None else rounding;pc=(0,2,3)[n%3];top=n%8;depth=(0,1,3,5)[n%4]
  banks=[dict(address=BANK,before=raw.hex()),dict(address=0x515ec0,before=struct.pack('<I',BANK if present else 0).hex()),dict(address=0x4920c0,before=struct.pack('<Q',clock_bits).hex()if clock_bits is not None else struct.pack('<d',now).hex()),dict(address=0x4c686c,before=struct.pack('<i',views).hex())]
  banks += [dict(address=0x474c78,before=struct.pack('<I',vector).hex()),dict(address=0x474c80,before=struct.pack('<I',replacement).hex()),dict(address=0x44aafc,before=bytes(12).hex()),dict(address=0x44ab1c,before=struct.pack('<3I',0,0x3f800000,0).hex())]
  if args:banks.append(dict(address=gp[4]+4,before=b''.join(struct.pack('<I',x)for x in args).hex()))
  rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=0,source=0,arguments=args,focus=focus,banks=sorted(banks,key=lambda b:b['address']),before_fp=[0x7f|(pc<<8)|(r<<10),(top<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]))
 for entry in ENTRIES:
  make(entry,'absent',present=False);make(entry,'inactive',active=False)
 for r in range(4):
  for now in(-1.,0.,.5,1.,2.,3.,float('nan'),float('inf'),-float('inf')):make(name='time',now=now,rounding=r)
 for mask in range(16):make(name='rgb',flags=mask)
 for pair in((0xb8,0xbc),(0xd0,0xd4),(0xe4,0xe8),(0xf4,0xf8),(0x124,0x128),(0x178,0x17c)):
  for start,end in((0,0),(0x80000000,0),(bits(2.),bits(0.)),(0x7fc12345,bits(1.)),(bits(0.),0x7f812345),(0x7fc12345,0xffc54321),(0x7f800000,0xff800000)):
   make(name=f'endpoints_{pair[0]:x}',changes=((pair[0],start),(pair[1],end)))
 for off in(0xb0,0xb4,0xc0,0xc4,0xc8,0xcc,0xdc,0xe0,0xec,0xf0,0xfc,0x100,0x104,0x108,0x10c,0x110,0x114,0x118,0x11c,0x120,0x12c,0x150):
  for value in(0,0x80000000,1,0x00800000,0xbf800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(name=f'exception_{off:x}_{value:x}',changes=((off,value),))
 for off in(0x18,0x1c,0x20):make(name='vector_mismatch',changes=((off,0x80000000),))
 for views in(-1,0,1,2,4):
  for opacity in(0,0x38d1b716,0x38d1b717,0x38d1b718,0x7fc12345):make(name='clear',views=views,changes=((0xb0,opacity),(0xb4,opacity),(0xc0,0),(0xc4,0),(0xc8,0),(0xcc,0)))
 for far,near in((0,0),(bits(-1.),0),(0x7fc12345,0),(bits(1.),0),(0,bits(1.))):
  for now in(-1.,2.,3.):make(name='dof_retire',now=now,changes=((0xdc,far),(0xe0,0),(0xec,near),(0xf0,0)))
 for pending in(0,1):
  for now in(-1.,0.,1.,2.,3.):make(name='capture',now=now,changes=((0x170,pending),))
 special=(0x80000000,0,1,0x00800000,0x3f800000,0xbf800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345)
 for focus in special:
  for duration in special:make(0x1fc10,'setup',focus=focus,arguments=[0x7fc12345,0xffc54321,duration,0x7f812345,0xff854321,duration],now=16777217.25)
 for now in(-1.,0.,1.,16777217.25,float('inf'),float('nan')):make(0x1fc10,'setup_clock',now=now)
 for vector,replacement in((BANK+0x18,BANK+0x18),(BANK+0x14,BANK+0x18),(BANK+0x18,BANK+0x14)):
  make(name='vector_alias',vector=vector,replacement=replacement)
 make(name='completed_color_clear',now=3.,changes=((0xc0,0),(0xc4,0),(0xc8,0),(0xcc,0)))
 make(0x1fc10,'fault_seed_setup');make(name='fault_seed_timed')
 make(name='fault_seed_equal',changes=[(off,0)for off in(0xb8,0xbc,0xd0,0xd4,0xdc,0xe0,0xec,0xf0,0x124,0x128,0x178,0x17c)])
 return rows

def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--output',type=Path,required=True);a.add_argument('--report',type=Path,required=True);args=a.parse_args()
 assert not args.output.exists(),'Preserve independent original fixture';assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);lifecycle.ENTRIES=ENTRIES;meta=lifecycle.metadata(image);code=_Code(image,'.text')
 # Capstone labels FST/FSTP memory operands READ; architectural direction is store.
 for site in meta['sites']:
  mnemonic=code.at(site['instruction']).mnemonic
  if mnemonic in ('fst','fstp'):
   for operand in site['operands']:operand.update(read=False,write=True)
  if mnemonic=='repe cmpsd':
   for operand in site['operands']:operand.update(read=True,write=False)
  assert all(op['read'] or op['write'] for op in site['operands']),site
 p=prepare(image,meta,scratch)
 rows=geometry.physical(p,cases(),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   cmpsd_reads=0
   for n,event in enumerate(events):
    if phase=='before_read' and event['instruction']==0x1feb5:
     cmpsd_reads+=1
     if cmpsd_reads%2==0:continue
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(p,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited]
 unreachable=[0x2044b,0x2044e,0x20455,0x20458]
 assert set(uncovered)<=set(unreachable),('Reachable original sites untested',uncovered)
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows)
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 report=dict(passed=set(uncovered)<=set(unreachable),unreachable_sites={hex(pc):'After elapsed is clamped to [0,1] and spilled float, 1-elapsed cannot be <0 or >1 under the qualified rounding modes; unordered stays unordered' for pc in uncovered},original_sha256=NEW_PIN,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),compound_read_boundary='CMPSD second operand cannot be stopped independently with ptrace; six duplicate before-instruction prefixes omitted. Completed reads/order and per-instruction failure state remain compared; no intra-instruction bus/MMIO micro-order claim.',executed_sites=len(visited),uncovered_sites=uncovered,fixture=str(args.output),fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),original_instructions=sum(r['original_instructions']for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_screen_timed_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_screen_timed_tu15.cpp')})
 args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('cases','executed_sites','uncovered_sites','fixture_bytes')}),flush=True)
 return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
