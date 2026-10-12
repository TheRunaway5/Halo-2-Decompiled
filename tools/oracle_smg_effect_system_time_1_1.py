#!/usr/bin/env python3
"""Natural outer frame with an actual source-initialized platform system clock."""
import argparse,json,os,subprocess,struct,hashlib,re
from pathlib import Path
from unicorn.x86_const import *
import oracle_smg_effect_lifecycle_1_1 as life
from oracle_smg_effect_owner_1_1 import sha,save,encoded
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_mp_spawn_local_map_clock import body

ROOT=life.ROOT;OUT=ROOT/'analysis/multiplayer/integration/inventory-source/smg-effect-system-time'
NATIVE=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-clock'
SDK=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h'
FROZEN={**life.FROZEN,
 'tools/oracle_smg_effect_lifecycle_1_1.py':'b29f06502ec9cb9f3d126c015da0116f24d8a1875e4c4a8437fef255d313e3e2',
 'docs/MP_SMG_EFFECT_LIFECYCLE_RESEARCH.md':'ee4f0670193615fdd18b5e680956e86601fba153c80400ffd0a4eb8b81af247e',
 'analysis/multiplayer/integration/inventory-source/smg-effect-lifecycle/measured/report.json':'98767fc0a385746d3d0906b3b9b9d491f72c960e718ae060e3b077fa98596e97',
 'analysis/multiplayer/integration/inventory-source/smg-effect-lifecycle/manifest.json':'ba977abb0d53b343c89a17329eb3a234f9b7dd3616990c6292de87ccbd403441'}

def provider_source(out):
 prior=json.loads((NATIVE/'report.json').read_text())
 # Read immutable compiled source/primary material; do not run its builder.
 old=(NATIVE/'compiled-timer.cpp').read_text();ke=(NATIVE/'primary-source/EmuKrnlKe.cpp').read_text();sdk=SDK.read_text()
 previous=prior['compiled_compatibility_timer'];assert sha(NATIVE/'compiled-timer.cpp')==previous['source_sha256']
 for name,pin in previous['primary_sources'].items():assert sha(NATIVE/'primary-source'/name)==pin,name
 assert 'CurrentTime The system time in 100-nanosecond intervals since January 1, 1601, in GMT.'in sdk
 assert 'XBSYSAPI EXPORTNUM(128) xbox::void_xt NTAPI xbox::KeQuerySystemTime'in ke
 query=body(ke,r'xbox::KeQuerySystemTime\s*\([^)]*\)')
 prefix=old[:old.index('static void Hex')];report=old[old.index('static void Hex'):old.index('int main()')]
 source=prefix+'\n#define LOG_FUNC_ONE_ARG(x)\nnamespace xbox {void_xt NTAPI KeQuerySystemTime(PLARGE_INTEGER);}\nxbox::void_xt NTAPI xbox::KeQuerySystemTime(xbox::PLARGE_INTEGER CurrentTime)\n'+query+'\n'+report+r'''
int main() {
 char op;while(std::scanf(" %c",&op)==1) {
  if(op=='x')return 0;
  if(op=='r') {unsigned n;assert(std::scanf("%u",&n)==1);replay.clear();replay_at=0;for(unsigned i=0;i<n;++i){Observation o;assert(std::scanf("%u%lld",&o.kind,&o.value)==2);replay.push_back(o);}replay_mode=true;assert(std::scanf(" %c",&op)==1);}else replay_mode=false;
  if(op=='i') {timer_init();xbox::KiInitSystem();Report("initialize",0,0);}
  else if(op=='q') {
   auto now=get_now();auto next=pit_next(now);xbox::LARGE_INTEGER output{};auto before=xbox::KeSystemTime;
   xbox::KeQuerySystemTime(&output);Report("query",now,next);
   std::printf("{\"output8_hex\":\"");Hex(&output,8);std::printf("\",\"system_before12_hex\":\"");Hex(&before,12);
   std::printf("\",\"system_after12_hex\":\"");Hex(&xbox::KeSystemTime,12);std::printf("\"}\n");std::fflush(stdout);
  }else std::abort();
  assert(!replay_mode || replay_at==replay.size());
 }
}
'''
 cpp=out/'compiled-system-clock.cpp';exe=out/'compiled-system-clock.exe';save(cpp,source.encode())
 command=['i686-w64-mingw32-g++','-std=c++20','-O0','-static','-Wl,--no-insert-timestamp',str(cpp),'-o',str(exe)]
 result=subprocess.run(command,capture_output=True,text=True);save(out/'compiler.log',(result.stdout+result.stderr).encode());assert result.returncode==0,result.stderr
 pins={str(p.relative_to(ROOT)):sha(p)for p in(NATIVE/'report.json',NATIVE/'compiled-timer.cpp',NATIVE/'primary-source/EmuKrnlKe.cpp',NATIVE/'primary-source/Timer.cpp',NATIVE/'primary-source/EmuKrnlKi.cpp',SDK)}
 row=dict(schema='source-system-clock-provider-v1',inputs=pins,compiled_source_sha256=sha(cpp),compiled_binary_sha256=sha(exe),original_query_body_sha256=hashlib.sha256(query.encode()).hexdigest(),original_timer_prefix_bytes_unchanged=True,compiler_command=command,kernel_ordinal=128,abi='NTAPI void KeQuerySystemTime(PLARGE_INTEGER); output8; stdcall one pointer; EAX has no defined output.',units='100-nanosecond intervals since January 1,1601 GMT; exact local XboxDev SDK declaration.',source_startup='Unchanged pinned Cxbx timer_init→get_now→real QPF/QPC/FILETIME and KiInitSystem. Actual get_now→pit_next→KiClockIsr update precedes each query; no clock value or elapsed delta chosen.',provider_boundary='Actual32-bit Wine host clocks and compatibility timer bodies; not Xbox oscillator/thread/event-loop continuity or whole authored title startup.',prior_primary_provenance=previous)
 save(out/'provider-contract-v2.json',encoded(row));return exe,row

class NativeClock:
 def __init__(self,path,replay=None):
  self.rows=[];self.private=[];self.replay=replay;self.p=subprocess.Popen(['wine',str(path)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(life.birth.local.base.owner.player_source.CONTEXT_OUT/'wine-prefix')));self.request('i')
 def request(self,op):
  if self.replay is None:self.p.stdin.write(op+'\n')
  else:
   old=self.replay[len(self.rows)];assert old['phase']==('initialize'if op=='i'else'query');obs=old['observations'];self.p.stdin.write('r '+str(len(obs))+'\n'+''.join(f"{q['kind']} {q['value']}\n"for q in obs)+op+'\n')
  self.p.stdin.flush();line=self.p.stdout.readline();assert line,line;row=json.loads(line);self.private.append(row.pop('private_native_records'))
  if op=='q':
   query=json.loads(self.p.stdout.readline());assert query['system_before12_hex']==query['system_after12_hex']==row['system12_hex'];assert query['output8_hex']==row['system12_hex'][:16];row['actual_query']=query
  if self.replay is not None:assert row==old,('native clock replay differs',len(self.rows))
  self.rows.append(row);return row
 def close(self):
  _,err=self.p.communicate('x\n',timeout=15);assert self.p.returncode==0,err;return err

class TimeTrace(life.LifecycleTrace):
 def __init__(self,*a,**kw):self.native_clock=None;self.clock_inputs=[];super().__init__(*a,**kw)
 def hook(self,g,a,n,user):
  if self.life_on and a==0x80000080 and self.native_clock is not None:
   sp=g.reg_read(UC_X86_REG_ESP);pointer=self.u(sp+4);before=self.raw(pointer,8);cpu=self.cpu();row=self.native_clock.request('q');value=bytes.fromhex(row['actual_query']['output8_hex'])
   g.mem_write(pointer,value);g.reg_write(UC_X86_REG_ESP,sp+8);g.reg_write(UC_X86_REG_EIP,self.u(sp))
   self.clock_inputs.append(dict(order=self.life_order,phase=self.active,import_thunk='00336983',kernel_ordinal=128,encoded_target='80000080',original_return=f'{self.u(sp):08x}',output_pointer=f'{pointer:08x}',before8_hex=before,actual_output8_hex=value.hex(),native_observation_index=len(self.native_clock.rows)-1,before_cpu=cpu,after_cpu=self.cpu(),scope='Actual captured native compatibility KeQuerySystemTime output, initialized by unchanged primary timer startup; only requested output8 and ABI return routing are transported. Void EAX preserved.'))
   self.life_order+=1;return
  if self.life_on and a==0x337a45 and g.reg_read(UC_X86_REG_EAX)==0x8000009c:
   self.new_invalid=dict(boundary='Unresolved ordinal156 data backing; encoded target aliases cache and is stopped before consumption.',instruction=f'{a:08x}',kernel_ordinal=156,target='8000009c',return_address=f'{self.u(g.reg_read(UC_X86_REG_ESP)):08x}',cpu=self.cpu(),aliased4_hex=self.raw(0x8000009c,4),instruction_unexecuted=True);raise KernelBoundary()
  super().hook(g,a,n,user)

def prepare(data):
 # The immutable helper fixes its observer class. Repeat its actual owning
 # roots with our separate read-only observer, then compare all prior fields.
 birth=life.birth;local=birth.local;t=local.base.owner.player_source.prepare(data,trace_type=TimeTrace);assert local.base.owner.player_source.spawn_and_activate(t,0,activate=False)
 prior=json.loads((local.base.owner.OUT/'report.json').read_text());t.item_observe=True
 for r in prior['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
 fields=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{v:08x}'for v in sorted(t.executed)],metadata=local.base.owner.metadata(t))
 for k,v in fields.items():assert v==prior[k],('prior item flow changed',k)
 t.item_observe=False;t.fp_resources();t.effect_on=True;t.birth_on=True;prior=json.loads((local.OUT/'audio-unavailable/report.json').read_text())
 for r in prior['stages']:
  if r['name']=='declared_local_view_request_owner':
   for e in(0x168866,0x16a356,0x107b50,0x179b90):t.services.pop(e,None)
   t.services[0x18ad70]=(prior['audio_boundary']['scope'],-1,24)
  if r['name']=='source_control_conversion':
   cmd=int(prior['declared_requests']['command']['pointer'],16);assert t.raw(cmd,0x5c)==prior['declared_requests']['command']['before5c_hex'];t.w(cmd,0x50000);t.w(cmd+0x14,0x3f800000)
  assert t.effect_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['registers'].items()))
 t.finish_birth(local.base.STOP);assert not t.birth_pending
 for k,v in(dict(stages=t.effect_stages,calls=t.effect_calls,states=t.effect_states,reads=t.effect_reads,stores=t.effect_stores,points=t.effect_points,final_state=t.snapshot())).items():assert v==prior[k],('prior local flow changed',k)
 prior_birth=json.loads((birth.OUT/'report.json').read_text())
 for k,v in dict(calls=t.birth_calls,points=t.birth_points,reads=t.birth_reads,stores=t.birth_stores).items():assert v==prior_birth[k],('prior birth flow changed',k)
 t.birth_on=False;t.effect_on=False;t.life_on=True;return t

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'measured-real/report.json');p.add_argument('--clock-replay',type=Path);p.add_argument('--provider-only',action='store_true');a=p.parse_args();pin=sha(__file__)
 for file,h in FROZEN.items():assert sha(ROOT/file)==h,file
 exe,contract=provider_source(OUT/'provider')
 if a.provider_only:print(json.dumps(contract));return
 source=life.birth.local.base.owner.player_source;worker=subprocess.Popen(['wine',str(source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(source.CONTEXT_OUT/'wine-prefix')));clock=None
 try:
  data=json.loads(worker.stdout.readline());t=prepare(data);assert t.life_stage('original_fixed_effect_phase_after_actual_birth',0x1788b0)
  prior=json.loads((life.OUT/'measured/report.json').read_text());phase=prior['stages'][0]['name'];fixed={q['id']for q in prior['calls']if q['phase']==phase};assert t.life_stages[0]==prior['stages'][0]
  assert t.life_calls==[q for q in prior['calls']if q['id']in fixed];assert t.life_states==prior['states'];assert t.life_reads==[q for q in prior['reads']if q['order']<194];assert t.life_stores==[q for q in prior['stores']if q['order']<194]
  replay=json.loads(a.clock_replay.read_text())if a.clock_replay else None;clock=NativeClock(exe,replay);t.native_clock=clock
  t.life_stage('original_outer_frame_with_source_platform_system_time',0x12cbc0);print(json.dumps(t.life_stages[-1]),flush=True)
  listings=[]
  entries=set(life.ENTRIES)|{int(q['entry'],16)for q in t.life_calls}
  for label,addresses in [(f'{e:08x}',life._trace(t.code,e,t.starts|entries).code)for e in sorted(entries)]+[('all-executed',t.life_executed),('all-stores',{int(q['instruction'],16)for q in t.life_stores if t.code.at(int(q['instruction'],16))is not None})]:
   ins=[t.code.at(q)for q in sorted(addresses)]
   for i in ins:assert i.bytes==t.image.read(i.address,i.size)==bytes(t.g.mem_read(i.address,i.size))
   path=a.output.parent/'instructions'/f'{label}.asm';save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(a.output.parent)),sha256=sha(path),rows=len(ins),executed_rows=sum(i.address in t.life_executed for i in ins)))
  save(a.output.parent/'native-clock-inputs.json',encoded(clock.rows));save(a.output.parent/'native-private-records.json',encoded(clock.private));save(a.output.parent/'native-clock-stderr.log',clock.close().encode());clock=None
  for file,h in FROZEN.items():assert sha(ROOT/file)==h,file
  assert sha(__file__)==pin
  report=dict(schema='source-smg-effect-system-time-v1',oracle_sha256=pin,frozen_pins=FROZEN,provider_contract=contract,native_clock_inputs_sha256=sha(a.output.parent/'native-clock-inputs.json'),prior_item_local_birth_fixed_phase_fields_byte_identical=True,stages=t.life_stages,calls=t.life_calls,states=t.life_states,reads=t.life_reads,stores=t.life_stores,points=t.life_points,actual_platform_clock_delivery=t.clock_inputs,instructions=listings,outside_source_text=t.life_external,new_game_or_particle_state_inputs=[],instruction_patches=[],boundaries=['Only actual primary compatibility system-time output is transported at encoded ordinal128; no hand-entered elapsed, game tick, readiness, pool/pose/particle/contact/admission/success.','Native provider startup is bounded and begins after the unchanged fixed-effect phase; prior finitegameclock and existing platform/context/resource/audio scopes are preserved. Whole boot/title/frame continuity is not asserted.','Unresolved ordinal156 encoded data target is stopped before cache alias consumption; no new tick backing is supplied.','Natural outer-frame original owner continues until its first unavailable dependency. Listed-only downstream aging/free and graphics submission remain explicit.'])
  save(a.output,encoded(report));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),clock_calls=len(t.clock_inputs),calls=len(t.life_calls),states=len(t.life_states),stores=len(t.life_stores))),flush=True)
 finally:
  if clock is not None:clock.close()
  try:_,err=worker.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:worker.kill();worker.stdout.close();worker.stderr.close();worker.wait(timeout=5);err='context worker shutdown timeout'
 if worker.returncode:raise RuntimeError((worker.returncode,err))
if __name__=='__main__':main()
