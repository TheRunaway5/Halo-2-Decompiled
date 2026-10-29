#!/usr/bin/env python3
"""Physical originalFF1A0 actual278E60 profile and composed authored jump-link predicate."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,FIRST,SECOND,ALT_FIRST,ALT_SECOND,GRAVITY,SCENARIO,BLOCK=0x1000000,0x1000100,0x1000140,0x1000180,0x10001c0,0x1000400,0x1000800,0x1000c00
ENTRIES=(0xff1a0,0x278e60,0xff580,0x30bd0)
CONSTANTS=(0x46a0cc,0x46a0d0,0x46a0dc,0x46a0e8,0x46a0ec,0x46a11c,0x46a120,0x46a130)
sha=common.sha
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();sec=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(sec));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'bank':(BASE,0x4000),'constants':(0x46a000,0x1000),'default':(0x44a000,0x1000),'pointer':(0x474000,0x1000),'scenario':(0x4ec000,0x1000),'gravity':(0x52b000,0x1000),'stack':(geometry.STACK,65536)}
 a=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
 for n,(_,size)in regions.items():a+=f'.section .{n},"aw"\n.space {size}\n'
 a+='''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
'''
 (p/'reference.s').write_text(a);(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{sec.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{x:x}:{{*(.{n})}} 'for n,(x,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_jump_arc_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert r.returncode==0,r.stderr
 return p

def inputs(image):
 rows=[]
 def make(name,entry=0xff1a0,a=(0.,0.,0.),b=(10.,0.,1.),alt_a=(0.,1.,0.),alt_b=(10.,1.,1.),speed=bits(10.),gravity=bits(1.),kind=0,count=1,scenario=SCENARIO,secondary=True,secondary_from=None,secondary_to=None,maximum_rise=bits(2.),changes=(),constant_changes=(),first=FIRST,second=SECOND,gravity_address=GRAVITY,rounding=None,precision=None,denorm=None,**fields):
  n=len(rows);raw=bytearray(0x1000)
  def put(p,v):struct.pack_into('<I',raw,p-BASE,v&0xffffffff)
  for p,values in((FIRST,a),(SECOND,b),(ALT_FIRST,alt_a),(ALT_SECOND,alt_b)):
   for k,v in enumerate(values):put(p+k*4,bits(v))
  put(GRAVITY,gravity);put(SCENARIO+0xc8,count);put(SCENARIO+0xcc,BLOCK)
  for k in range(6):put(BLOCK+0x80+k*4,speed);put(BLOCK+0x98+k*4,maximum_rise)
  for p,value in changes:put(p,value)
  const={p:bytearray(image.read(p,4))for p in CONSTANTS}
  for p,value in constant_changes:const[p]=bytearray(int(value).to_bytes(4,'little'))
  banks={BASE:raw,**const,0x474d10:bytearray(image.read(0x474d10,4)),0x44ac40:bytearray(image.read(0x44ac40,12)),0x52be68:bytearray(struct.pack('<I',gravity_address)),0x4ecc1c:bytearray(struct.pack('<I',scenario))}
  gp=[0x5a000000+n*29+k*11 for k in range(8)];gp[4]=geometry.SP+n%4*4
  af=ALT_FIRST if secondary else 0;at=ALT_SECOND if secondary else 0
  if secondary_from is not None:af=secondary_from
  if secondary_to is not None:at=secondary_to
  private_direction=gp[4]-0x400
  if entry==0xff1a0:gp[0]=first;gp[1]=second;gp[6]=private_direction
  else:gp[0]=(gp[0]&0xffff0000)|(kind&0xffff);gp[1]=first;gp[3]=at
  r=n%4 if rounding is None else rounding;pc=(0,2,3)[n%3]if precision is None else precision;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  rows.append(dict(id=n,name=name,entry=entry,gp=gp,first=first,second=second,secondary_first=af,secondary_second=at,private_direction=private_direction,kind=kind,speed_bits=speed,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x7f|(pc<<8)|(r<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,2,3)[n%4])],**fields))
 for r in range(4):
  for pc in(0,2,3):
   for d in(0,0x40,0x8000,0x8040):make(f'controls_{r}_{pc}_{d:x}',rounding=r,precision=pc,denorm=d)
 for gravity in(0,0x80000000,1,0x80000001,bits(1e-30),bits(1.),bits(-1.),bits(100.),0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  for speed in(bits(0.),bits(.000000001),bits(1.),bits(10.),bits(1000.)):make(f'gravity_{gravity:x}_{speed:x}',gravity=gravity,speed=speed)
 for speed in(0x80000000,1,0x80000001,bits(-10.),0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'speed_{speed:x}',speed=speed)
 for point in(FIRST,SECOND):
  for lane in range(3):
   for value in(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'exception_{point:x}_{lane}_{value:x}',changes=((point+lane*4,value),))
 for b in((0.,0.,0.),(0.,0.,1.),(0.,0.,-1.),(.0001,0.,0.),(0.,0.,.00010000001),(1e-35,0.,1e-35),(100.,0.,0.)):
  for speed in(bits(.0000000001),bits(1.),bits(10.)):make(f'geometry_{b}_{speed:x}',b=b,speed=speed)
 for kind in(-32768,-1,0,1,2,3,4,5,6,32767):
  for count in(-1,0,1):make(f'link_kind_{kind}_{count}',entry=0x278e60,kind=kind,count=count)
 for maximum in(0,0x80000000,bits(-1.),bits(.5),bits(1.),bits(1.0000001),bits(2.),0x7f800000,0x7fc12345,0x7f812345):
  for second_height in(0.,1.,2.):make(f'link_rise_{maximum:x}_{second_height}',entry=0x278e60,maximum_rise=maximum,b=(10.,0.,second_height))
 for af,at in((0,0),(ALT_FIRST,0),(0,ALT_SECOND),(ALT_FIRST,ALT_SECOND)):
  for first_speed in(bits(1.),bits(10.)):make(f'link_optional_{af:x}_{at:x}_{first_speed:x}',entry=0x278e60,secondary_from=af,secondary_to=at,speed=first_speed)
 for a,b in(((0.,0.,2.),(10.,0.,0.)),((0.,0.,0.),(10.,0.,1.)),((0.,0.,0.),(0.,0.,0.))):make(f'link_pairs_{a}_{b}',entry=0x278e60,a=a,b=b,alt_a=(1.,1.,0.),alt_b=(1.,1.,3.),maximum_rise=bits(3.))
 make('link_null_scenario',entry=0x278e60,scenario=0)
 make('small_velocity_normalization',gravity=bits(1e-10),speed=bits(5e-5))
 make('adversarial_zero_epsilon_straight',gravity=0,a=(0.,0.,0.),b=(0.,0.,0.),speed=bits(1.),constant_changes=((0x46a0ec,0),),adversarial_constants=True)
 make('adversarial_negative_time',constant_changes=((0x46a11c,bits(-2.)),),adversarial_constants=True)
 make('adversarial_default_direction',a=(0.,0.,0.),b=(0.,0.,0.),speed=bits(1.),constant_changes=((0x46a0ec,bits(-1.)),(0x46a0cc,bits(0.))),adversarial_constants=True)
 for name,kwargs in(('solved',dict(b=(10.,2.,1.))),('minimum',dict(speed=bits(1.))),('straight',dict(gravity=0)),('small_velocity',dict(gravity=bits(1e-30),speed=bits(1e-10))),('link',dict(entry=0x278e60))):make('fault_seed_'+name,**kwargs)
 make('actual_gravity_read_fault',gravity_address=0x40000000,actual_read_fault=True)
 make('actual_point_read_fault',second=0x40000000,actual_read_fault=True)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for n in range(len(seed['reads'])):
   row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_before_read_'+str(n),fault=dict(phase='before_read',ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited]
 expected={s['instruction']for s in meta['sites']if s['instruction']in(0xff2db,0xff2e1,0xff2f4,0xff2f8,0xff2fe,0xff4fb,0xff501,0xff50d,0xff513,0xff51f,0xff525,0xff531,0xff537,0xff543,0xff549,0xff676,0xff67e,0xff688)or 0xff317<=s['instruction']<=0xff37c}
 unexpected=sorted(set(uncovered)-expected);assert not unexpected,[hex(pc)for pc in unexpected]
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,profile=dict(caller=0x278e60,arguments=['authored_speed',0x3f800000,0,0,0,1,0,0,0,0,0]),cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original arc repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=2,pending_quantities=sum(bool(r['pending_extended'])for r in rows),original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],uncovered_sites=uncovered,unexpected_uncovered_sites=unexpected,adversarial_constant_cases=sum(r.get('adversarial_constants',False)for r in rows),fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_jump_arc_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_jump_arc_tu15.cpp')},scope_limits=['Actual278E60 FF1A0 fixed-argument profile; other optional-output/low-arc contracts unqualified','Masked original FP with at least five original temporary slots','Private outputs/point values are native mathematical data; no source CPU/private frame supplied','No full path search/update/native campaign acceptance claimed'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} original arc comparisons/{len(visited)} sites; fixture{a.output.stat().st_size} bytes; uncovered{[hex(x)for x in uncovered]}')
if __name__=='__main__':main()
