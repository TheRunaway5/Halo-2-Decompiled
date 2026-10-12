#!/usr/bin/env python3
"""Complete original CF300 on the genuine one-SMG allocated owner.

Only selector ABI arguments are declared. No inventory/body/manager/capability
write or service result is supplied. Prior neutral source composition is first
verified byte-identical; each selector starts from that exact captured backing.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import oracle_mp_command_locomotion_pose_1_1 as owner
from oracle_smg_firing_1_1 import STOP
from functions import _trace

ROOT=owner.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/single-weapon-selection'
PINS={**owner.FROZEN,
 'tools/oracle_mp_command_locomotion_pose_1_1.py':'807e303f942380bc809816e00b28053cc1c78e8542a1ac92516ab26cc25645ad',
 'analysis/multiplayer/integration/inventory-source/inventory-switch-u5/report.json':'358511090ee3be1cce652923afc1979b759d2d48b63b45287ca25ff48d3251d2'}
SELECTORS={0xcf300,0xceaf0,0x111490}
owner.ENTRIES=owner.ENTRIES|SELECTORS
owner.STATE_ENTRIES=owner.STATE_ENTRIES|SELECTORS

class SelectionTrace(owner.CommandPoseTrace):
 def packets(self,e,reg,s):
  r=super().packets(e,reg,s)
  if e in SELECTORS:
   unit=s[0]if e!=0x111490 else reg['eax']
   h=self.u(0x4e78d0);p=self.u(self.u(h+0x44)+(unit&0xffff)*12+8)
   r.append(dict(field='actual_unit800',pointer=f'{p:08x}',size=0x800,raw_hex=self.raw(p,0x800)))
  if e==0xceaf0:
   item=reg['edx'];p=self.u(self.u(self.u(0x4e78d0)+0x44)+(item&0xffff)*12+8)
   tag=self.u(p);definition=self.u(self.u(0x4eb114)+(tag&0xffff)*16+8)
   r.append(dict(field='actual_weapon_definition360',datum=f'{item:08x}',tag=f'{tag:08x}',pointer=f'{definition:08x}',size=0x360,raw_hex=self.raw(definition,0x360)))
  return r

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args()
 pin=owner.digest(__file__)
 for path,h in PINS.items():assert owner.digest(ROOT/path)==h,path
 proc=subprocess.Popen(['wine',str(owner.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owner.command.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());owner.CommandPoseTrace=SelectionTrace;t,correlation=owner.prepare(data)
  backing=t.source_fixture;cpu=t.source_fixture_cpu
  assert [(lo,hi,p)for lo,hi,p,_ in backing]==list(t.g.mem_regions())
  memory=[dict(lo=f'{lo:08x}',hi=f'{hi:08x}',permissions=p,size=len(raw),sha256=hashlib.sha256(raw).hexdigest())for lo,hi,p,raw in backing]
  cases=[]
  for previous in (0,-1):
   for direction in (-1,0,1):
    for lo,hi,permissions,raw in backing:t.g.mem_write(lo,raw)
    t.g.context_restore(cpu)
    unit=t.spawned_unit;ub=t.u(t.u(t.u(0x4e78d0)+0x44)+(unit&0xffff)*12+8)
    slots=struct.unpack('<4I',bytes(t.g.mem_read(ub+0x218,16)))
    assert slots[0]!=0xffffffff and slots[1:]==(0xffffffff,)*3
    assert t.u(ub+0x212,1)==0 and t.u(ub+0x213,1)==0xff
    name=f'one_smg_previous{previous}_direction{direction}';t.probe=name;t.observe=True
    begin={k:len(getattr(t,k))for k in('calls','states','reads','stores','points')}
    before=t.state_id();events=len(t.events);args=(unit,0xffffffff,previous&0xffffffff,direction&0xffffffff)
    complete=t.stage(name,0xcf300,args);t.probe_returns(STOP);t.observe=False
    assert complete and not t.pending
    result=t.g.reg_read(owner.UC_X86_REG_EAX)&0xffff
    case=dict(case=name,entry='000cf300',arguments=[f'{x:08x}'for x in args],before_state=before,after_state=t.state_id(),
     completion=t.player_stages[-1]['completion'],return_ax=f'{result:04x}',return_signed_slot=struct.unpack('<h',struct.pack('<H',result))[0],
     services=t.events[events:],ranges={k:[v,len(getattr(t,k))]for k,v in begin.items()},final_cpu=t.cpu())
    cases.append(case);print(json.dumps(dict(case=name,slot=case['return_signed_slot'],services=case['services'])),flush=True)
  rows=owner.listings(t,a.output_dir)
  mapping=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/inventory-switch-u5/report.json').read_text())
  paired=next(r for r in mapping['owners']if r['old_entry']=='000cf300')
  for path,h in PINS.items():assert owner.digest(ROOT/path)==h,path
  assert owner.digest(__file__)==pin
  report=dict(schema=1,tool_sha256=pin,frozen_pins=PINS,prior_neutral_identity=correlation,restored_source_guest_state=memory,
   source_xbe_sha256=owner.command.PIN,cache_sha256=owner.command.CACHE_PIN,cases=cases,calls=t.calls,states=t.states,
   reads=t.reads,stores=t.stores,points=t.points,listings=rows,static_u5_selector=paired,instruction_patches=[],new_supplied_gameplay_or_physics_outputs=[],
   boundary='Actual original allocated player/biped/one SMG and source manager/PVS/world first verified unchanged. Six declared selector argument pairs only: previous0/NONE and direction-1/0/1; modeNONE from the original188630 caller. OriginalCF300,CEAF0 and111490 execute fully. No inventory, parent, body mode, graph, pose, clock, target, capability or success input changed. Inherited startup/platform/resource boundaries retained. TU1.5D0390 has complete static body correspondence only; no TU1.5 owning runtime call claimed.')
  owner.frozen_write(a.output_dir/'report.json',owner.encoded(report))
  print(json.dumps(dict(output=str(a.output_dir/'report.json'),sha256=owner.digest(a.output_dir/'report.json'),cases=len(cases),calls=len(t.calls),states=len(t.states))))
 finally:
  try:_,err=proc.communicate('x',timeout=20)
  except subprocess.TimeoutExpired:
   proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host worker shutdown timeout'
 assert proc.returncode==0,err
if __name__=='__main__':main()
