#!/usr/bin/env python3
"""Declared jump/crouch commands through the frozen real player/world owners.

The complete source guest checkpoint is restored per stimulus. Only incoming
command+0 is changed after the original neutral initializer. No physical mode,
classification, contacts, action words, velocity, pose or return is supplied.
"""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EDX, UC_X86_REG_EIP
import oracle_mp_command_locomotion_pose_1_1 as motion

ROOT=motion.ROOT
DEPENDENCY='tools/oracle_mp_command_locomotion_pose_1_1.py'
ENTRIES={0xe5cb0,0xe5da0,0xe6e10,0xdee00,0xe3b20}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 pins={**motion.FROZEN,DEPENDENCY:motion.digest(ROOT/DEPENDENCY)}
 motion.ENTRIES.update(ENTRIES);motion.STATE_ENTRIES.update(ENTRIES)
 proc=subprocess.Popen(['wine',str(motion.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t,correlation=motion.prepare(data)
  backing,cpu=t.source_fixture,t.source_fixture_cpu
  cases=[]
  print(json.dumps(dict(prior_runtime_identical=True)),flush=True)
  for name,flags in [('neutral',0),('crouch',1),('jump',2),('crouch_and_jump',3)]:
   for lo,hi,permissions,raw in backing:t.g.mem_write(lo,raw)
   t.g.context_restore(cpu);t.probe=name;t.observe=True;stages=[]
   begin={k:len(getattr(t,k))for k in ('calls','states','reads','stores','points')}
   def run(label,e,args=(),regs=()):
    before=t.state_id();ok=t.stage(name+'_'+label,e,args,regs);t.probe_returns(motion.STOP)
    stages.append(dict(name=label,entry=f'{e:08x}',before_state=before,after_state=t.state_id(),completion=t.player_stages[-1]['completion'],invalid=t.new_invalid,stopped_eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}'))
    return ok
   assert run('neutral_initializer',0x14e780,regs=((UC_X86_REG_EDX,motion.command.COMMAND),))
   before=t.raw(motion.command.COMMAND,4);t.w(motion.command.COMMAND,flags)
   record=t.u(0x4d6d44)+0x80+(t.spawned_player&0xffff)*0x88
   for label,e,args in [('submission',0x852a0,(record,motion.command.COMMAND)),('assembly',0x69350,(t.u(0x4d6d44),motion.command.PACKET)),('player',0x14eae0,(motion.command.PACKET,)),('prephysics',0xb7e00,()),('world',0x1c7660,()),('postphysics',0xb7f50,()),('newflags',0xb8040,())]:
    if not run(label,e,args):break
   t.observe=False
   unreturned=[r['id']for _,_,r in t.pending];t.pending=[]
   assert not unreturned or stages[-1]['completion']=='blocked_original_instruction'
   cases.append(dict(case=name,declared_command_write=dict(pointer=f'{motion.command.COMMAND:08x}',before_hex=before,after_hex=flags.to_bytes(4,'little').hex(),only_field='incoming command+0 flags'),ranges={k:[n,len(getattr(t,k))]for k,n in begin.items()},stages=stages,unreturned=unreturned))
   print(json.dumps(dict(case=name,completion=stages[-1]['completion'],calls=cases[-1]['ranges']['calls'][1]-begin['calls'])),flush=True)
  for path,pin in pins.items():assert motion.digest(ROOT/path)==pin,path
  report=dict(schema=1,tool_sha256=motion.digest(__file__),source_pins=pins,prior_runtime_correlation=correlation,
   restored_source_checkpoint='Actual complete early zero-mask player phase before first neutral initializer/submission/assembly/player/world roots',
   cases=cases,calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,instructions=motion.listings(t,a.output.parent),
   inherited_services=t.events,physical_services=t.physical_services,new_supplied_gameplay_fields=[],instruction_patches=[],
   boundaries=['Only the declared incoming command flags are supplied. Source player/unit objects, allocator headers, active physics, poses, contacts, velocities and action components originate in unchanged original owners.',
    'Each case starts at the same real source checkpoint and uses inherited replayed original host inputs. This does not establish repeated advancing match ticks or local-device/session ownership.',
    'Original1.1 instructions execute. The separately verified1.5 owner correspondence remains separate. Any first unavailable dependency is retained, never turned into success.'])
  motion.frozen_write(a.output,motion.encoded(report));print(json.dumps(dict(report=str(a.output),sha256=motion.digest(a.output))),flush=True)
 finally:
  if proc.poll() is None:
   try:proc.stdin.write('x');proc.stdin.flush();proc.wait(timeout=15)
   except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
  for stream in (proc.stdin,proc.stdout,proc.stderr):stream.close()
if __name__=='__main__':main()
