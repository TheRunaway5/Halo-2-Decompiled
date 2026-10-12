#!/usr/bin/env python3
"""New declared action-input probes on the genuine allocated source owner."""
import argparse,json,os,subprocess,struct,hashlib
from pathlib import Path
import oracle_mp_command_locomotion_pose_1_1 as owner
from oracle_smg_firing_1_1 import STOP,UC_X86_REG_EDX
from functions import _trace
ROOT=owner.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission'
PINS={**owner.FROZEN,
 'tools/oracle_mp_command_locomotion_pose_1_1.py':'807e303f942380bc809816e00b28053cc1c78e8542a1ac92516ab26cc25645ad'}
MELEE={0x150f00,0xe7ef0,0xe7f30,0xeae90,0xeb530,0xeb700,0xeb830,
 0xeb080,0xeb000,0xeade0,0xeac30,0x169f3b,0x1de810,0x1de840,
 0x105470,0x1054b0,0xa9ad0,0xbbd10,0xe0490,0xef630,0xd1320}
owner.ENTRIES=owner.ENTRIES|MELEE
owner.STATE_ENTRIES=owner.STATE_ENTRIES|MELEE
class MeleeTrace(owner.CommandPoseTrace):
 def packets(self,e,reg,s):
  r=super().packets(e,reg,s)
  if e==0xe7f30:
   r.append(dict(field='edi_actual_action_request32',pointer=f"{reg['edi']:08x}",size=32,raw_hex=self.raw(reg['edi'],32)))
  if e in (0xeae90,0xeb530):
   r.append(dict(field='stack1_actual_action_request32',pointer=f'{s[1]:08x}',size=32,raw_hex=self.raw(s[1],32)))
  return r
def execute(t,name,flag):
 for lo,hi,p,raw in t.source_fixture:t.g.mem_write(lo,raw)
 t.g.context_restore(t.source_fixture_cpu)
 t.probe=name;t.observe=True;begin={k:len(getattr(t,k))for k in ('calls','states','reads','stores','points')};stages=[]
 def run(label,e,args=(),regs=()):
  before=t.state_id();start=len(t.events);ok=t.stage(name+'_'+label,e,args,regs);t.probe_returns(STOP)
  stages.append(dict(name=label,entry=f'{e:08x}',before_state=before,after_state=t.state_id(),
   completion=t.player_stages[-1]['completion'],invalid=t.new_invalid,services=t.events[start:],stopped_eip=f'{t.g.reg_read(owner.UC_X86_REG_EIP):08x}'))
  return ok
 assert run('original_neutral_command_initializer',0x14e780,regs=((UC_X86_REG_EDX,owner.command.COMMAND),))
 p=owner.command.COMMAND;before=t.raw(p,4);after=struct.pack('<I',t.u(p)|flag);t.g.mem_write(p,after)
 declared=dict(pointer=f'{p:08x}',only_field='command+0 flags',before_hex=before,after_hex=after.hex(),mask=f'{flag:08x}',source_of_stimulus='explicit declared non-neutral command input; no live keyboard/device admission asserted')
 record=t.u(0x4d6d44)+0x80+(t.spawned_player&0xffff)*0x88
 for label,e,args in [('submission',0x852a0,(record,p)),('assembly',0x69350,(t.u(0x4d6d44),owner.command.PACKET)),
  ('player',0x14eae0,(owner.command.PACKET,)),('prephysics',0xb7e00,()),('world',0x1c7660,()),('postphysics',0xb7f50,()),('newflags',0xb8040,())]:
  if not run(label,e,args):break
 unreturned=[r['id']for _,_,r in t.pending];t.observe=False
 if unreturned:assert stages[-1]['completion']=='blocked_original_instruction'
 t.pending=[]
 return dict(case=name,declared_input=declared,ranges={k:[v,len(getattr(t,k))]for k,v in begin.items()},stages=stages,unreturned_original_calls=unreturned,final_cpu=t.cpu())
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args()
 # Correct pin is checked before any original experiment; this extension never
 # modifies a dependency or supplies a new gameplay/physics result.
 for path,pin in PINS.items():assert owner.digest(ROOT/path)==pin,(path,owner.digest(ROOT/path))
 env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owner.command.CONTEXT_OUT/'wine-prefix'))
 proc=subprocess.Popen(['wine',str(owner.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
 try:
  data=json.loads(proc.stdout.readline())
  # Substitute only the observer class used by the frozen preparation helper.
  owner.CommandPoseTrace=MeleeTrace;t,correlation=owner.prepare(data)
  cases=[]
  for name,flag in [('declared_flag20',0x20),('declared_flag40',0x40),('declared_flag04000000',0x4000000)]:
   cases.append(execute(t,name,flag));print(json.dumps(dict(case=name,last=cases[-1]['stages'][-1])),flush=True)
 finally:
  try:_,err=proc.communicate('x',timeout=20)
  except subprocess.TimeoutExpired:
   proc.stdin.close();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host worker shutdown pipe retained'
 assert proc.returncode==0,err
 rows=owner.listings(t,a.output_dir)
 descriptors=[]
 for action in (26,27):
  ptr=struct.unpack('<I',t.image.read(0x46ec68+action*4,4))[0]
  descriptors.append(dict(type=action,table_va='0046ec68',descriptor_va=f'{ptr:08x}',raw16_hex=t.image.read(ptr,16).hex(),handlers=[f'{v:08x}'for v in struct.unpack('<4I',t.image.read(ptr,16))]))
 report=dict(schema=1,tool_sha256=owner.digest(__file__),frozen_pins=PINS,prior_neutral_identity=correlation,
  source_xbe_sha256=owner.command.PIN,cache_sha256=owner.command.CACHE_PIN,cases=cases,
  calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,listings=rows,source_descriptors=descriptors,
  instruction_patches=[],new_supplied_gameplay_or_physics_outputs=[],
  boundary='Actual original allocated player/biped/SMG and source PVS/world/neutral composition first verified unchanged. Three explicitly supplied command flag stimuli; no target, current pose, contact, action progress, clock increment, admission or damage success supplied. Inherited platform/resource/audio and source timer/delivery boundaries remain explicit. No full source keyboard/controller admission or continuous frame or1.5 execution asserted.')
 owner.frozen_write(a.output_dir/'report.json',owner.encoded(report))
 print(json.dumps(dict(output=str(a.output_dir/'report.json'),sha256=owner.digest(a.output_dir/'report.json'),cases=len(cases),calls=len(t.calls),states=len(t.states))))
if __name__=='__main__':main()
