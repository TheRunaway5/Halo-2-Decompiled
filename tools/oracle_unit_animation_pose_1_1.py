#!/usr/bin/env python3
"""Observe original local sampler ABI on the unchanged genuine allocated owner."""
import argparse,json,os,subprocess
from pathlib import Path
import oracle_mp_spartan_owned_pose_1_1 as owned
from oracle_unit_animation_manager_1_1 import sha,save,encoded,ROOT
from oracle_smg_firing_1_1 import STOP
from unicorn.x86_const import *

ENTRIES=(0x1D0F80,0x1CA950,0x280030,0x1D18F0,0x1D1B00,0x1DE9E0,0x1E13C0)
class SamplingTrace(owned.OwnedPoseTrace):
 def __init__(self,*a,**k):
  self.sampling_enabled=False;self.sampling_calls=[];self.sampling_pending=[]
  super().__init__(*a,**k)
 def sampling_return(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP);match=[x for x in self.sampling_pending if x[0]==a and sp>x[1]]
  if not match:return
  ret,depth,r=match[-1];r['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';r['call_end']=len(self.sampling_calls)
  if 'output'in r:r['after_output']=self.raw(r['output'],r['node_count']*32)
  self.sampling_pending.remove(match[-1])
 def run(self,*a,**k):
  super().run(*a,**k)
  if self.sampling_enabled:self.sampling_return(STOP)
 def hook(self,g,a,n,u):
  if self.sampling_enabled:
   self.sampling_return(a)
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(8)]
    r=dict(id=len(self.sampling_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,registers=reg,stack=[f'{v:08x}'for v in s])
    if a in(0x1D0F80,0x1CA950,0x280030):
     channel=reg['ebx']if a==0x1CA950 else s[0]if a==0x1D0F80 else 0
     nodes=reg['esi']if a==0x1D0F80 else s[2]if a==0x1CA950 else s[1]
     output=s[4]if a==0x1D0F80 else s[3]if a==0x1CA950 else s[6]
     mask=s[3]if a==0x1D0F80 else s[1]if a==0x1CA950 else s[5]
     assert nodes<256
     r.update(channel=channel,node_count=nodes,output=output,mask=mask,before_output=self.raw(output,nodes*32),mask_hex=self.raw(mask,32)if mask else None,
      frequency=self.u(self.u(0x518444)+2,2),feature_dc=self.u(0x4770DC,1),feature_dd=self.u(0x4770DD,1))
     if channel:
      r['channel_hex']=self.raw(channel,32);graph=self.u(channel);packed=self.u(channel+4);root=self.u(self.u(0x4EB114)+(graph&0xffff)*16+8)
      r.update(root_graph=root,graph_masks_hex=self.raw(root+0x5C,64),graph=f'{graph:08x}',packed=f'{packed:08x}')
      if a==0x1D0F80:r['manager_hex']=self.raw(channel,0x90)
      if packed>>16!=0xffff:
       resolved=root
       if packed&0xffff!=0xffff:
        d=self.u(self.u(root+0x50)+(packed&0xffff)*32+4);resolved=self.u(self.u(0x4EB114)+(d&0xffff)*16+8)
       p=self.u(resolved+0x30)+(packed>>16)*0x6c;r['definition_hex']=self.raw(p,0x6c)
    self.sampling_calls.append(r);self.sampling_pending.append((self.u(sp),sp,r))
  super().hook(g,a,n,u)

def execute(data):
 t=owned.owning.player_source.prepare(data,trace_type=SamplingTrace);t.pose_enabled=True;t.sampling_enabled=True
 assert owned.owning.player_source.spawn_and_activate(t,0,activate=False)
 old=json.loads((owned.owning.OUT/'report.json').read_text());t.item_observe=True
 for r in old['stages']:
  assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
 t.pose_returns(STOP);t.sampling_return(STOP);assert not t.sampling_pending
 prior=json.loads((owned.OUT/'report.json').read_text())
 for name,value in(('calls',t.pose_calls),('states',t.pose_states),('reads',t.pose_reads),('stores',t.pose_stores),('points',t.pose_points)):
  assert value==prior[name],('Frozen actual pose runtime changed',name)
 return t
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for f,h in owned.FROZEN.items():assert owned.digest(ROOT/f)==h,f
 pin=sha(ROOT/'tools/oracle_mp_spartan_owned_pose_1_1.py');assert pin=='cee290577a992a2b8b799409da666ba49cc148fe11982bdcb44c49525c81b6e2'
 proc=subprocess.Popen(['wine',str(owned.owning.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owned.owning.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  t=execute(json.loads(proc.stdout.readline()));result=dict(schema='source-unit-local-sampler-v1',tool_sha256=sha(__file__),prior_report_sha256=sha(owned.OUT/'report.json'),prior_runtime_byte_identical=True,
   source_pins=owned.FROZEN,calls=t.sampling_calls,instruction_patches=[],new_guest_inputs=[],new_supplied_pose_or_success=[],
   scope='Same actual allocated owner/loadout/PVS/neutral ordinary schedule as the preserved owned-pose freeze. Finite tick interval is inherited; natural device and full frame ownership remain unresolved. All local outputs here are original sampler/codec results.')
  save(a.output,encoded(result));print(json.dumps(dict(sha256=sha(a.output),calls=len(t.sampling_calls))),flush=True)
 finally:
  try:proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5)
if __name__=='__main__':main()
