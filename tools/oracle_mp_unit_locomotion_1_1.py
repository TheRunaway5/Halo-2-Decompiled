#!/usr/bin/env python3
"""Original 1F52F0/1E94E0 with declared request variations on a real owner.

Non-neutral vector/initial classification/flag inputs are explicit fixtures,
not claimed live controller delivery. The source unit/physical state is kept.
"""
import argparse, hashlib, itertools, json, os, subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EIP
import oracle_mp_spartan_owned_pose_1_1 as owner
from oracle_smg_firing_1_1 import STOP
from functions import _trace

ROOT=owner.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/unit-locomotion-measured'
FROZEN={**owner.FROZEN,
 'tools/oracle_mp_spartan_owned_pose_1_1.py':'cee290577a992a2b8b799409da666ba49cc148fe11982bdcb44c49525c81b6e2',
 'tools/oracle_mp_spartan_owned_pose_audit_1_1.py':'b03fdcbf311f1abaa32feab98b8b0232e9a724e286b15fecaef9ae7b109995e5',
 'docs/MP_SPARTAN_OWNED_POSE_RESEARCH.md':'e2c2559d3dfcac5c32a5a133ba4a5eab369701a40365ef45ac61d07713eb61ae',
 'analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json':'4b9e9ed383e0081fe7d64c57636731304d66d322b3aef23259024a27061733cb',
 'analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/audit.json':'26aed7140453fe9e1079dc1da3dfe811f00813e9eb6c51b7c9de848a29092463',
 'analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/manifest.json':'037aba2d042f61a1705fa093fba96961f8328a542a612e23678a9014a8dd1409'}
digest,encoded,frozen_write=owner.digest,owner.encoded,owner.frozen_write
ENTRIES={0x1f52f0,0x1e94e0}

def vectors():
 out=[];seen=set()
 def add(name,values):
  values=tuple(values)
  if values not in seen:seen.add(values);out.append(dict(name=name,bits=[f'{v:08x}'for v in values]))
 for i,v in enumerate(itertools.product((0,0x3f800000,0xbf800000),repeat=3)):add(f'axis_cube_{i}',v)
 for i,v in enumerate(itertools.product((0,0x80000000),repeat=3)):add(f'signed_zero_{i}',v)
 # Exact float bit patterns are supplied operands, not gameplay thresholds.
 for n,b in (('minimum_subnormal',1),('maximum_subnormal',0x007fffff),('minimum_normal',0x00800000),
  ('below_one',0x3f7fffff),('above_one',0x3f800001),('maximum_finite',0x7f7fffff),
  ('infinity',0x7f800000),('quiet_nan',0x7fc12345),('signaling_nan',0x7f812345)):
  for axis in range(3):
   for sign in(0,0x80000000):
    v=[0,0,0];v[axis]=b|sign;add(f'{n}_axis{axis}_sign{sign>>31}',v)
  for x,y in ((b,b),(b,b|0x80000000),(b|0x80000000,b),(b|0x80000000,b|0x80000000),
   (b,0x3f800000),(0x3f800000,b)):
   add(f'{n}_xy_{x:08x}_{y:08x}',(x,y,0))
  for sx,sy in itertools.product((0,0x80000000),repeat=2):
   add(f'{n}_x_vs_one_{sx>>31}_{sy>>31}',(b|sx,0x3f800000|sy,0))
   add(f'{n}_one_vs_y_{sx>>31}_{sy>>31}',(0x3f800000|sx,b|sy,0))
 for x,y in((0x3f7fffff,0x3f800000),(0x3f800000,0x3f7fffff),(0x3f800000,0x3f800001),(0x3f800001,0x3f800000)):
  for sx,sy in itertools.product((0,0x80000000),repeat=2):add(f'adjacent_abs_{x|sx:08x}_{y|sy:08x}',(x|sx,y|sy,0))
 return out

class Capture:
 def __init__(self,t):
  self.t=t;self.current=None;self.order=0;self.executed=set()
  self.routines={e:_trace(t.code,e,t.starts|ENTRIES)for e in ENTRIES}
  self.sites=set().union(*(r.code for r in self.routines.values()))
  t.g.hook_add(unicorn.UC_HOOK_CODE,self.code)
  t.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.read)
  t.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.write)
 def code(self,g,a,size,user):
  if self.current is not None and a in self.sites:
   self.executed.add(a);i=self.t.code.at(a)
   self.current['points'].append(dict(order=self.order,instruction=f'{a:08x}',bytes=i.bytes.hex(),cpu=self.t.cpu()))
   self.order+=1
 def read(self,g,access,p,size,value,user):
  a=g.reg_read(UC_X86_REG_EIP)
  if self.current is not None and a in self.sites:
   self.current['reads'].append(dict(order=self.order,instruction=f'{a:08x}',address=f'{p:08x}',size=size,raw_hex=self.t.raw(p,size)))
   self.order+=1
 def write(self,g,access,p,size,value,user):
  a=g.reg_read(UC_X86_REG_EIP)
  if self.current is not None:
   assert a in self.sites,('Unbounded classifier store',hex(a))
   self.current['stores'].append(dict(order=self.order,instruction=f'{a:08x}',address=f'{p:08x}',size=size,
    before_hex=self.t.raw(p,size),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
   self.order+=1

def execute(data,out):
 t,prior=owner.execute(data)
 d=json.loads((owner.OUT/'report.json').read_text())
 same=dict(calls=t.pose_calls,states=t.pose_states,reads=t.pose_reads,stores=t.pose_stores,points=t.pose_points,
  metadata=owner.metadata(t),resources=t.resource_requests,services=t.events,stages=t.item_stages,
  executed_in_scope=[f'{a:08x}'for a in sorted(t.pose_executed)])
 for k,v in same.items():assert d[k]==v,('Frozen owner replay differs',k)
 replay=dict(report=str((owner.OUT/'report.json').relative_to(ROOT)),report_sha256=digest(owner.OUT/'report.json'),
  keys=list(same),runtime_sha256=hashlib.sha256(encoded(same)).hexdigest(),byte_identical=True,prior_active_runtime=prior)
 leaf=next(r for r in t.pose_calls if r['entry']=='001f52f0')
 request,result=leaf['packets'];rp,op=int(request['pointer'],16),int(result['pointer'],16)
 unit=next(o for o in t.pose_states[leaf['before_state']]['objects']if o['datum']=='e26f0000')
 body=int(unit['body_address'],16);physical=body+0x3dc
 assert t.u(physical,1)==1
 live=dict(body_address=unit['body_address'],body_size=unit['body_size'],body_hex=t.raw(body,unit['body_size']),
  physical_pointer=f'{physical:08x}',physical_raw10_hex=t.raw(physical,16),unit_datum=unit['datum'],
  source_unit_control_1b0_hex=t.raw(body+0x1b0,12),source_header_hex=t.raw(int(unit['header_address'],16),12))
 t.pose_enabled=False;t.item_observe=False;c=Capture(t);cases=[]
 # Original call-stack packet backing is reused, with the original measured
 # packet restored in full. Only listed fixture offsets change. No unit field,
 # game clock, admission, contact, pose or world record is changed.
 for vector in vectors():
  for seed in((0,0),(0x12345678,0xa5)):
   for route in('leaf','wrapper','wrapper_flag20'):
    initial_request=bytearray.fromhex(request['raw_hex']);initial_result=bytearray.fromhex(result['raw_hex'])
    for n,b in enumerate(vector['bits']):initial_request[0xdc+4*n:0xe0+4*n]=int(b,16).to_bytes(4,'little')
    initial_result[4:8]=seed[0].to_bytes(4,'little');initial_result[8]=seed[1]
    if route=='wrapper_flag20':initial_request[0x14]|=0x20
    else:initial_request[0x14]&=0xdf
    t.g.mem_write(rp,bytes(initial_request));t.g.mem_write(op,bytes(initial_result))
    q=dict(id=len(cases),vector=vector,route=route,source_owner_call_id=leaf['id'],
     request_pointer=f'{rp:08x}',result_pointer=f'{op:08x}',physical_pointer=f'{physical:08x}',
     request_before_hex=t.raw(rp,0x154),result_before_hex=t.raw(op,0x70),unit_before_sha256=hashlib.sha256(bytes(t.g.mem_read(body,unit['body_size']))).hexdigest(),
     physical_before_hex=t.raw(physical,16),reads=[],stores=[],points=[])
    c.current=q;c.order=0;before_events=len(t.events)
    entry=0x1f52f0 if route=='leaf'else 0x1e94e0
    regs=((UC_X86_REG_ECX,rp),(UC_X86_REG_EDX,op))if route=='leaf'else((UC_X86_REG_EAX,rp),(UC_X86_REG_ECX,physical),(UC_X86_REG_ESI,op))
    t.active=f'explicit_request_{q["id"]}';t.run(entry,(),regs)
    q.update(entry=f'{entry:08x}',return_cpu=t.cpu(),request_after_hex=t.raw(rp,0x154),result_after_hex=t.raw(op,0x70),
     physical_after_hex=t.raw(physical,16),unit_after_sha256=hashlib.sha256(bytes(t.g.mem_read(body,unit['body_size']))).hexdigest(),services=t.events[before_events:])
    c.current=None
    assert q['request_before_hex']==q['request_after_hex'] and q['physical_before_hex']==q['physical_after_hex']
    assert q['unit_before_sha256']==q['unit_after_sha256']==hashlib.sha256(bytes.fromhex(live['body_hex'])).hexdigest() and not q['services']
    cases.append(q)
 instructions=[]
 for e,r in sorted(c.routines.items()):
  ins=[t.code.at(a)for a in sorted(r.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=out/'instructions'/f'{e:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  instructions.append(dict(entry=f'{e:08x}',file=str(p.relative_to(out)),sha256=digest(p),rows=len(ins),executed_rows=sum(a in c.executed for a in r.code)))
 return dict(schema=1,source_pins=FROZEN,prior_owner_correlation=replay,source_owner=live,
  source_packet_call=dict(call_id=leaf['id'],before_state=leaf['before_state'],request=request,result=result),
  jump_table=dict(pointer='001e9520',raw18_hex=t.raw(0x1e9520,24)),cases=cases,instructions=instructions,
  executed_addresses=[f'{a:08x}'for a in sorted(c.executed)],instruction_patches=[],
  supplied_inputs=['Each case restores the exact original source-produced request154/result70 into their recorded original stack backing.',
   'Only request+DC/E0/E4 vector bits, initial result+4 classification/+8 blend byte, and request+14 bit20 vary as declared fixtures.',
   'Root-call CPU reset FPCW027F/MXCSR1F80 and ABI registers are the inherited explicit execution boundary.'],
  boundaries=['Non-neutral vectors are supplied inputs, not source-produced live device/control or actual moved-unit state.',
   'The actual source unit, physical state1, authored root, inventory, current/local pose, PVS, queue, world and clock remain unchanged.',
   'Wrapper states2/3/4/5/6 and invalid dispatch indexes are source-listed but unexecuted; no lower branch result is supplied.',
   'This measures classifier output buffers and ordered stores; not a full physics request producer for non-neutral controls, frame, motion, clip selection, hardware FP exception or Xbox parity.'])

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');a=p.parse_args()
 for name,h in FROZEN.items():assert digest(ROOT/name)==h,name
 own=digest(__file__)
 proc=subprocess.Popen(['wine',str(owner.owning.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owner.owning.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());report=execute(data,a.output.parent);report['oracle_sha256']=own
  for name,h in FROZEN.items():assert digest(ROOT/name)==h,name
  assert digest(__file__)==own;frozen_write(a.output,encoded(report))
  print(json.dumps(dict(output=str(a.output),sha256=digest(a.output),vectors=len(vectors()),cases=len(report['cases']),stores=sum(len(c['stores'])for c in report['cases']))),flush=True)
 finally:
  forced=False
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:
   forced=True;proc.kill()
   try:_,err=proc.communicate(timeout=5)
   except subprocess.TimeoutExpired:proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host helper retained shutdown pipe'
 assert proc.returncode==0 or forced,err
if __name__=='__main__':main()
