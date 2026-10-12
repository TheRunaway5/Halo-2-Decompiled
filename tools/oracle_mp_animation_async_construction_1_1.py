#!/usr/bin/env python3
"""Execute actual1221B0 from the frozen genuine constructed-unit resource state.

No kernel create, handle, wait, thread, priority or IO outcome is supplied.
"""
import argparse,json,os,subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ESI
import oracle_mp_animation_residency_1_1 as prior
ROOT=prior.ROOT;BASE=ROOT/'analysis/multiplayer/integration/spawn-source/animation-async-construction'
PIN='13e6d76c9085fb21c50ead88fc569d518bb26489237f505fc78f017131291a38'
ENTRIES=tuple(dict.fromkeys(prior.ENTRIES+(0x1221b0,0x121fd0,0x122020,0x1220e0,0x122150,0x122350,0x122360,
 0x2dc83a,0x2dc7b9,0x2d8606,0x2d83f6,0x2dc9c8,0x2dc8b2,0x2dc892,0x2dc814,0x2d84c4,0x2d837a,0x2d8532,0x2d856e)))
def verify():
 b=ROOT/'analysis/multiplayer/integration/spawn-source/animation-residency';m=b/'manifest.json';assert prior.sha(m)==PIN
 d=json.loads(m.read_text())
 for group in('new_source_pins','artifact_pins','dependency_pins','protected_prior_pins'):
  for n,h in d[group].items():
   p=Path(n);p=p if p.is_absolute()else ROOT/p;assert prior.sha(p)==h,n
 return d
class AsyncTrace(prior.ResidencyTrace):
 def snapshot(self):
  out=super().snapshot();out['async_raw_state']=self.raw(0x4e7924,0x25a0);out['source_worker_stack_descriptor12']=self.raw(0x5098a0,12)
  return out

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 frozen=verify();access=prior.adapter(out);old=prior.original.ResolutionTrace;old_entries=prior.original.ENTRIES
 prior.original.ENTRIES=ENTRIES;prior.original.ResolutionTrace=AsyncTrace
 with(out/'host-provider.stderr.log').open('w')as err:
  ctor=prior.original.selection.ctor
  proc=subprocess.Popen(['wine',str(ctor.owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(ctor.owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(proc.stdout.readline());t=prior.original.execute(data);t.resource_finish(ctor.STOP);assert not t.resource_pending
   t.selection_enabled=False;t.resource_scope=True;t.source_raw_storage=True;t.new_scope=True;t.access_adapter=access;t.io_inputs=[]
   for name,entry in [('original_resource_constructor',0x1248d0),('original_resource_enable',0x124af0)]:assert t.attempt(name,entry)
   assert t.attempt('original_budget_mode_from_actual_options',0x1908e0,regs=((UC_X86_REG_ESI,t.u(0x4edf18)+8),))
   mode=t.g.reg_read(prior.UC_X86_REG_EAX);assert t.attempt('original_animation_capacity_callback_from_actual_mode',0x124980,(mode,))
   baseline=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/animation-residency/measured/report.json').read_text())
   for actual,expected in zip(t.residency_stages,baseline['stages'][:4]):
    normalized=dict(actual)
    for key in('before','after'):
     normalized[key]=dict(normalized[key]);normalized[key].pop('async_raw_state');normalized[key].pop('source_worker_stack_descriptor12')
    assert normalized==expected,actual['name']
   first_call=len(t.resource_calls);first_store=len(t.resource_stores)
   t.residency=False
   completed=t.attempt('original_async_request_bank_mutex_and_worker_constructor',0x1221b0)
   assert not completed
   listings=[]
   for e,r in t.resource_routines.items():
    ins=[t.code.at(i)for i in sorted(r.code)]
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    f=out/'instructions'/f'{e:08x}.asm';prior.original.selection.save(f,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
    listings.append(dict(entry=f'{e:08x}',file=str(f.relative_to(out)),sha256=prior.sha(f),rows=len(ins),executed_rows=sum(i.address in t.resource_executed for i in ins)))
   report=dict(schema='animation-async-construction-owning-v1',tool_sha256=prior.sha(__file__),prior_manifest_sha256=PIN,source_sha256=ctor.PIN,cache_sha256=ctor.CACHE_PIN,stages=t.residency_stages,first_new_call=first_call,first_new_store=first_store,resource_calls=t.resource_calls,resource_stores=t.resource_stores,host_raw_storage=t.host_storage,applied_access=t.applied_access,listings=listings,instruction_patches=[],new_guest_fields=[],supplied_kernel_or_io_success=[],source_prefix_correlates_exactly=True,boundaries=['Same frozen genuine unit/selector preparation and original resource pool/enable/capacity roots; no complete title ordering asserted.','Actual1221B0 executes unchanged and owns its full raw150x64 bank/free links/head state before first real kernel creation stop.','No create/handle/wait/release/worker/priority/file result supplied. Original platform ownership remains required.','Copied pinned compatibility protection and actual184mmap backing remain inherited explicit platform boundaries.'])
   prior.original.selection.save(out/'report.json',prior.encoded(report));print(json.dumps(dict(report=str(out/'report.json'),sha256=prior.sha(out/'report.json'),new_calls=len(t.resource_calls)-first_call,new_stores=len(t.resource_stores)-first_store,stop=t.residency_stages[-1]['invalid'])))
  finally:
   prior.original.ResolutionTrace=old;prior.original.ENTRIES=old_entries
   if proc.poll()is None:
    proc.stdin.write('x\n');proc.stdin.flush()
    try:proc.wait(timeout=15)
    except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=5)
   proc.stdin.close();proc.stdout.close()
 verify()
if __name__=='__main__':main()
