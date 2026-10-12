#!/usr/bin/env python3
"""Read-only continuation of the frozen looping-owned grenade preparation."""
import json,os,subprocess
from pathlib import Path
import oracle_mp_grenade_sound_pool_1_1 as pool
from oracle_mp_title_update_display import sha,save,encoded
OUT=pool.ROOT/'analysis/multiplayer/integration/live-grenade-20261010/release-probe'
assert not OUT.exists()
process=subprocess.Popen(['wine',str(pool.motion.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=open('/tmp/live-grenade-generator-20261010.stderr','w'),text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(pool.motion.command.CONTEXT_OUT/'wine-prefix')))
try:
 context=json.loads(process.stdout.readline());pool.release.OwnedReleaseTrace=pool.SoundPoolTrace;t,prior=pool.release.prepare(context)
 stages=[]
 for label,entry in [('source_loop_ctor',0x18a200),('source_loop_reset_unreturned_driver',0x18a2a0),('source_classes_ctor',0x223960),('source_classes_reset',0x2239b0),('source_classes_engine',0x18a790)]:
  before=t.state_id();ok=t.stage(label,entry);stages.append(dict(label=label,entry=f'{entry:08x}',returned=ok,before=before,after=t.state_id(),invalid=t.new_invalid));assert ok==(entry!=0x18a2a0)
 continuation=pool.release.continue_updates(t,12)
 report=dict(schema='live-grenade-release-probe-v1',tool_sha256=sha(__file__),prior=prior,explicit_separate_storage_roots=stages,continuation=continuation,calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,inherited_services=t.events,boundaries=['Unchanged frozen preparation, actual original neutral continuation.18A2A0 remains unreturned at original driver stop; later class entries are separate roots.','No newly supplied gameplay state or return. Host neutral commands and inherited fixture boundaries remain explicit.'])
 save(OUT/'report.json',encoded(report));print(json.dumps(dict(report=str(OUT/'report.json'),sha256=sha(OUT/'report.json'),final=continuation['final_state']['grenade'],last=continuation['stages'][-1])),flush=True)
finally:
 if process.poll()is None:
  process.stdin.write('x');process.stdin.flush()
  try:process.wait(timeout=15)
  except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
 process.stdin.close();process.stdout.close()
