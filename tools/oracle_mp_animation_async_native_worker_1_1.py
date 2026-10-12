#!/usr/bin/env python3
"""Compose unchanged original request then worker on real file services.

The native thread implementation is independently tested by the C++ composition.
This original instruction oracle uses the frozen synchronous named-native
providers and two explicit caller roots; it makes no Xbox scheduler/TLS claim.
"""
import argparse,json
from pathlib import Path
import oracle_mp_animation_async_worker_1_1 as prior
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_mp_title_update_display import sha,save,encoded,ROOT
PIN='65259e171526eb9ad42a2d49ed3d3c314c1ba39ec00d790e3117d1d912e107dc'
NativeUc=prior.Uc
def compose_uc(*ctor_args,**ctor_kwargs):
 g=NativeUc(*ctor_args,**ctor_kwargs);original=g.emu_start
 def execute(entry,until,*args,**kwargs):
  original(entry,until,*args,**kwargs)
  if entry==0x19d8c0:
   assert g.reg_read(UC_X86_REG_EIP)==prior.STOP
   # Separate original no-arg worker root with a declared host caller stack.
   # No instruction or game/queue field is patched between the two bodies.
   g.reg_write(UC_X86_REG_ESP,prior.STACK);g.mem_write(prior.STACK,prior.pack([prior.STOP]))
   original(0x122360,until,*args,**kwargs)
 g.emu_start=execute;return g
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 manifest=ROOT/'analysis/multiplayer/integration/spawn-source/animation-async-worker/manifest.json';assert sha(manifest)=='c08523918642751fc570cc7b3949327ae2ee19443c2f1d04a9e88bf871d18475'
 assert sha(ROOT/'analysis/multiplayer/integration/spawn-source/animation-async-worker/measured/report.json')==PIN
 saved=prior.Uc;prior.Uc=compose_uc
 try:c=prior.execute('actual_request_then_owned_file_worker',0x19d8c0,lambda context:0xbb4800,'file')
 finally:prior.Uc=saved
 report=dict(schema='animation-async-native-worker-source-v1',tool_sha256=sha(__file__),prior_manifest_sha256=sha(manifest),source_pins=prior.PINS,cache_sha256=prior.MAP_PIN,cases=[c],
  scope='Two explicit unchanged source roots19D8C0 then122360; source constructor-owned bank prefix and real native file/sync providers inherited unchanged. Synchronous original execution, not an Xbox thread/kernel/TLS scheduling claim.',
  native_thread_boundary='Separate C++ case runs actual NativeThreads and constructor worker creation adaptation, retaining unsupported source/native priority and original TLS/SEH/stack translation as explicit boundaries.',instruction_patches=[],resource_success=[])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),stores=len(c['stores']),calls=len(c['services']))))
if __name__=='__main__':main()
