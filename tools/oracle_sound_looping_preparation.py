#!/usr/bin/env python3
"""Real grenade owner plus original loop/class storage; never admits a driver."""
import argparse,json,os,subprocess
from pathlib import Path
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EAX
import oracle_mp_grenade_sound_pool_1_1 as pool
from oracle_mp_title_update_display import sha,save,encoded

ROOT=pool.ROOT
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 process=subprocess.Popen(['wine',str(pool.motion.command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(pool.motion.command.CONTEXT_OUT/'wine-prefix')))
 try:
  context=json.loads(process.stdout.readline());pool.release.OwnedReleaseTrace=pool.SoundPoolTrace;t,prior=pool.release.prepare(context)
  active=False;executed=set();stores=[];stages=[];requests=[];pending=[]
  def hook(g,pc,n,u):
   if active and t.code.at(pc):executed.add(pc)
   if pc==0x18a9c0:
    sp=g.reg_read(UC_X86_REG_ESP);row=dict(caller_return=f'{t.u(sp):08x}',definition=f'{g.reg_read(UC_X86_REG_ESI):08x}',object=f'{g.reg_read(UC_X86_REG_EDI):08x}',marker=f'{t.u(sp+4,2):04x}',parameter=f'{t.u(sp+8):08x}',before=state());requests.append(row);pending.append((t.u(sp),sp+12,row))
   for ret,sp,row in pending[:]:
    if pc==ret and g.reg_read(UC_X86_REG_ESP)==sp:row.update(result=f'{g.reg_read(UC_X86_REG_EAX):08x}',after=state());pending.remove((ret,sp,row))
  def write(g,k,address,n,value,u):
   if active and not(0x2000000<=address<0x2200000):stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',bytes=n,before=bytes(g.mem_read(address,n)).hex(),after=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
  t.g.hook_add(UC_HOOK_CODE,hook);t.g.hook_add(UC_HOOK_MEM_WRITE,write)
  def state():
   base=t.u(0x4ed650);used=t.u(0x4ed654);engine=t.u(0x4f10b8);index=t.u(engine+0xc14)
   return dict(arena_base=f'{base:08x}',arena_used=f'{used:08x}',arena_crc=f'{t.u(0x4ed65c):08x}',arena_hex=t.raw(base,used),arena_backing_hex=t.raw(base,0x1d0000),allocator=f'{t.u(0x51841c):08x}',pool=f'{t.u(0x4f485c):08x}',state=f'{t.u(0x4f4858):08x}',class_fades=f'{t.u(0x5098f0):08x}',class_fades_flag=f'{t.u(0x5098f4,1):02x}',engine=f'{engine:08x}',engine_index=f'{index:08x}',engine_table44_hex=t.raw(0x566080,0x44),source_driver=f'{t.u(0x526474):08x}')
  before=state();active=True
  for name,entry in(('actual_loop_constructor',0x18a200),('actual_loop_reset_until_absent_driver',0x18a2a0),('actual_class_constructor',0x223960),('actual_class_reset',0x2239b0),('actual_engine_gated_class_setup',0x18a790)):
   first=len(stores);pre=state();ok=t.stage(name,entry);stages.append(dict(name=name,entry=f'{entry:08x}',returned=ok,stopped_eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',invalid=t.new_invalid,before=pre,after=state(),stores=stores[first:]));print(json.dumps(dict(name=name,returned=ok,eip=stages[-1]['stopped_eip'])),flush=True)
   assert ok==(entry!=0x18a2a0)
  active=False
  final=state();seq=[t.code.at(pc)for pc in sorted(executed)]
  for i in seq:assert t.image.read(i.address,len(i.bytes))==i.bytes
  save(a.output/'instructions/executed.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq).encode())
  # Keep the next genuine ordinary grenade owner separate from the constructor
  # ABI report. No dropped call is reported as a returned map reset.
  continuation=pool.release.continue_updates(t,1)
  assert len(requests)==1 and not pending and requests[0]['result']!='ffffffff'
  report=dict(schema='sound-looping-actual-preparation-v1',tool_sha256=sha(__file__),prior=prior,before=before,stages=stages,final=final,actual_request_calls=requests,continuation=continuation,source_returns_or_driver_results_supplied=[],instruction_patches=[],limits=['Reexecuted unchanged admitted grenade fixture plus declared original storage entries, not whole startup/map dispatch.','18A2A0 remains unreturned at genuine absent driver22310C. Later class owners and18A790 are separate explicitly composed entries.','Original current-engine and engine-table producers are inherited from the actual player fixture. No selected engine or class-fade state is assigned.','The subsequent ordinary grenade continuation uses the actually produced loop pool despite the separately recorded incomplete audio map owner; it cannot establish completed sound startup/admission.'])
  save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),final_driver=final['source_driver'],stages=len(stages),continuation_stages=len(continuation['stages']))),flush=True)
 finally:
  if process.poll()is None:
   try:process.stdin.write('x');process.stdin.flush();process.wait(timeout=15)
   except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
  for stream in(process.stdin,process.stdout,process.stderr):stream.close()
if __name__=='__main__':main()
