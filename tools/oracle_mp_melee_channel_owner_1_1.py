#!/usr/bin/env python3
"""Execute the original melee channel producer on the actual allocated owner.

The neutral original composition stays immutable. Remove only its inherited
115650 negative-animation boundary after preparation; retain all genuine unit,
inventory, physical, manager/cache and clock state. No supplied channel success,
animation, target, contact, event progress or timing is introduced.
"""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import oracle_mp_melee_admission_measured_1_1 as measured

owner=measured.owner;ROOT=owner.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/melee-channel-owner'
PINS={**measured.PINS,
 'tools/oracle_mp_melee_admission_measured_1_1.py':'5323b47ed38ea282cb09f50c88480d8e25307778ff13332c65a2b7b2bc94da55',
 'analysis/multiplayer/integration/inventory-source/melee-admission-measured/report.json':'4891168c3b4bc47da9f793dde25a07b10b6772871ea74f0e4abcc36a1dfe06bc'}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args();pin=owner.digest(__file__)
 for path,h in PINS.items():assert owner.digest(ROOT/path)==h,path
 proc=subprocess.Popen(['wine',str(owner.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owner.command.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());owner.CommandPoseTrace=measured.MeleeTrace;t,neutral=owner.prepare(data)
  assert [(lo,hi,p)for lo,hi,p,_ in t.source_fixture]==list(t.g.mem_regions())
  memory=[dict(lo=f'{lo:08x}',hi=f'{hi:08x}',permissions=p,size=len(raw),sha256=hashlib.sha256(raw).hexdigest())for lo,hi,p,raw in t.source_fixture]
  service=t.services.pop(0x115650)
  removed=dict(entry='00115650',description=service[0],eax=service[1],stack_pop=service[2],scope='Removed after unchanged original neutral preparation; original bytes execute for every subsequent probe call.')
  cases=[]
  for name,flag in [('declared_flag20',0x20),('declared_flag40',0x40),('declared_flag04000000',0x4000000)]:
   cases.append(measured.original.execute(t,name,flag));print(json.dumps(dict(case=name,last=cases[-1]['stages'][-1],calls=cases[-1]['ranges']['calls'])),flush=True)
  rows=owner.listings(t,a.output_dir)
  for path,h in PINS.items():assert owner.digest(ROOT/path)==h,path
  assert owner.digest(__file__)==pin
  report=dict(schema=1,tool_sha256=pin,frozen_pins=PINS,prior_neutral_identity=neutral,restored_exact_source_guest_state=memory,
   removed_inherited_boundary=removed,source_xbe_sha256=owner.command.PIN,cache_sha256=owner.command.CACHE_PIN,
   cases=cases,calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,listings=rows,
   source_descriptors=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/melee-admission/report.json').read_text())['source_descriptors'],
   instruction_patches=[],new_supplied_gameplay_or_physics_outputs=[],
   boundary='Declared command flags only, on genuine source-allocated player/biped/SMG/manager/cache/PVS/world/physical state. Original115650 replaces its earlier inherited negative boundary; all original requests, capability/channel selection, event metadata, shared RNG and first ordinary update execute until actual first unavailable dependency. No animation, target, hit/contact, event progress, tick/frame increment or admission/success is supplied. Retained EFE30 feedback and other inherited platform/resource boundaries are explicit in the report. Source live device/local scheduler/readiness and continuous owning clock remain unavailable. Only TU1.1 dynamic execution; source1.5 full owner mapping is independent static evidence.')
  owner.frozen_write(a.output_dir/'report.json',owner.encoded(report))
  print(json.dumps(dict(output=str(a.output_dir/'report.json'),sha256=owner.digest(a.output_dir/'report.json'),cases=len(cases),calls=len(t.calls),states=len(t.states))))
 finally:
  try:_,err=proc.communicate('x',timeout=20)
  except subprocess.TimeoutExpired:
   proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host worker shutdown timeout'
 assert proc.returncode==0,err
if __name__=='__main__':main()
