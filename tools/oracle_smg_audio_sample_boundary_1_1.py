#!/usr/bin/env python3
"""Source-loaded compressed SMG bytes, original formats and real driver stop.

No local user/event/voice/device/admission/ready result is supplied. Original
cache loading publishes completion after actual verified file reads. Bounded
format/data-pointer/metadata calls do not represent accepted playback.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_smg_audio_valid_owner_1_1 import ValidOwnerTrace,prepare,ROOT,PIN,CACHE_PIN,digest
from oracle_smg_audio_fp_owner_1_1 import FROZEN as OLDER
from oracle_smg_firing_1_1 import STOP
from functions import _Code,_trace

OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-sample-boundary'
ARENA=0x0C300000
FROZEN={**OLDER,
 'tools/oracle_smg_audio_fp_owner_1_1.py':'7884ca932216a3ad5ba049eeef1ab30e0f9214291e3eeb37daa9faee420e4960',
 'tools/audit_smg_audio_fp_owner_1_1.py':'ff1528175496d11c9f678a769b33934ea9ac6e7bf565b0839ccabd310a98e7d2',
 'docs/SMG_AUDIO_FP_OWNER_GATE_1_1_RESEARCH.md':'f0595c4e20aa2c76edd90e490da7dd9fe2ef013e8ffe4206d72a59095a2a9e59',
 'analysis/multiplayer/map-pack/smg-audio-fp-owner/frozen/suite.json':'6f66fef608a00494ca5a59d2cac88ce7a10d96ace5fe2aa40f29ba2e49ef0f36',
 'analysis/multiplayer/map-pack/smg-audio-fp-owner/source-fixture.json':'09ac664b0cce7a759e47478535b880bd3f84955885fc8591cdc2f9537aacdd1f',
 'analysis/multiplayer/map-pack/smg-audio-fp-owner/audit.json':'62299784de85d1823d767e849f3d8189e229efd39098f6a874aa3c98b660b268',
 'analysis/multiplayer/map-pack/smg-audio-fp-owner/measured/report.json':'cd64367b1c0191046b13bb2a873dcf0ad968e318e537dfc1bdc7345f6ca96b05',
 'analysis/multiplayer/map-pack/smg-audio-fp-owner/reexecution/report.json':'cd64367b1c0191046b13bb2a873dcf0ad968e318e537dfc1bdc7345f6ca96b05'}
ENTRIES=(0x21AA60,0x21ABD0,0x21AD70,0x220520,0x32909C,0x3840C7,0x384057,0x2215F0,0x2B4970)
STATIC=ENTRIES+(0x21A960,0x21AB20,0x122360,0x19D8C0,0x2B4CE0,0x2B4D60,0x2B4D90,0x2B4DD0,0x2B4E10,
 0x2B4B30,0x222840,0x383B22,0x381365,0x3865E6,0x3864F7,0x386F8B,0x382E65,0x221B90)

class SampleBoundary(Exception):pass
class SampleTrace(ValidOwnerTrace):
 def __init__(self,data,replay):
  self.sample_enabled=False;self.sample_calls=[];self.sample_returns=[];self.sample_stores=[];self.sample_boundaries=[];self.sample_driver_executed=set()
  super().__init__(data,replay);self.driver_code=_Code(self.image,'DSOUND');self.sample_enabled=True
 def finish_sample(self,a):
  for ret,row in self.sample_returns[:]:
   if ret==a:row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';self.sample_returns.remove((ret,row))
 def hook(self,g,a,size,user):
  if self.sample_enabled:
   self.finish_sample(a)
   if a==0x2B4975 and self.active=='direct_unadmitted_original_stream_constructor_from_authored_encoding':
    assert self.u(0x526474)==0
    sp=g.reg_read(UC_X86_REG_ESP);esi=g.reg_read(UC_X86_REG_ESI);desc=g.reg_read(UC_X86_REG_EDI)
    fmt=self.u(desc+8)
    self.sample_boundaries.append({'phase':self.active,'instruction':f'{a:08x}',
     'instruction_bytes':self.image.read(a,3).hex(),'dependency':'absent original driver-owned channel storage before codec assignment',
     'driver_pointer':'00000000','registers':self.registers(),'stack_40_hex':bytes(g.mem_read(sp,0x40)).hex(),
     'channel_address':f'{esi:08x}','first_unexecuted_store_address':f'{esi+0x30:08x}',
     'original_stream_descriptor_pointer':f'{desc:08x}','original_stream_descriptor_18_hex':bytes(g.mem_read(desc,0x18)).hex(),
     'original_waveformat_pointer':f'{fmt:08x}','original_waveformat_14_hex':bytes(g.mem_read(fmt,0x14)).hex(),
     'source_driver_channel_voice_device_or_success_result_supplied':False})
    raise SampleBoundary()
   if self.driver_code.inside(a):self.sample_driver_executed.add(a)
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}',
     'registers':self.registers(),'return_site':f'{self.u(sp):08x}','stack':[f'{self.u(sp+4+i*4):08x}'for i in range(4)]}
    self.sample_calls.append(row);self.sample_returns.append((self.u(sp),row))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.sample_enabled and self.active.startswith(('direct_original_sample','direct_original_wave','direct_original_compact','direct_unadmitted_original_stream','direct_original_remaining_chunk')):
   self.sample_stores.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
    'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def sample_action(self,name,entry,args=(),regs=()):
  self.fp_action(name,entry,args,regs);self.finish_sample(STOP)
  return {'entry':f'{entry:08x}','phase':name,'return_eax':f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
   'stack_arguments':[f'{v:08x}'for v in args],'register_arguments':[{ 'register':r,'value':f'{v:08x}'}for r,v in regs]}
 def sample_case(self,scenario,directory):
  owning=self.valid_case(scenario);c=self.cache;sound=int(owning['source_sound_datum'],16);tag=c.tags[sound];off=tag['offset']
  src=bytes.fromhex(owning['before_removal']['audio']['source_cache']['source_bc_hex'])
  matg=next(z for z in c.tags.values()if z['group']=='matg');mr=c.block(matg['offset']+0xC0,0x24)[0]
  ug=c.tags[c.u(mr+0x20)];assert ug['group']=='ugh!'and self.u(0x526464)==ug['offset']+c.magic
  ranges=c.block(ug['offset']+0x20,12);perms=c.block(ug['offset']+0x28,16);chunks=c.block(ug['offset']+0x40,12)
  pitch=src[0x9C];permutation=src[0x9D];chunk_index=int.from_bytes(src[0x9E:0xA0],'little')
  r=ranges[c.h(off+8)+pitch];p=perms[c.h(r+8)+permutation];first=c.h(p+12);count=c.h(p+14)
  assert chunk_index==0 and chunks[first]+c.magic==self.chunk_pointer
  def metadata(address,n):return {'file_offset':address,'original_va':f'{address+c.magic:08x}','raw_hex':bytes(c.data[address:address+n]).hex()}
  # Original fastcall ECX is the authored permutation pointer.
  query=self.sample_action('direct_original_sample_permutation_byte_total',0x21ABD0,
   regs=((UC_X86_REG_ECX,p+c.magic),(UC_X86_REG_EDX,count)))
  neutral=self.sample_action('direct_original_sample_permutation_word6',0x21AD70,(pitch,permutation),((UC_X86_REG_EAX,sound),))
  samples=[];raw_concat=b''
  directory.mkdir(exist_ok=True)
  for i in range(count):
   fileoff=chunks[first+i];ptr=fileoff+c.magic;self.chunk_pointer=ptr
   loading=None
   if i:
    assert self.u(ptr+8)==0xFFFFFFFF
    before=self.cache_state();loading=self.sample_action('direct_original_remaining_chunk_load_metadata_input',0x21A960,(sound,ptr,2))
    pending=self.cache_state();assert int(loading['return_eax'],16)&1 and not int(loading['return_eax'],16)&2
    done=self.worker_once();loading.update(before=before,pending=pending,after_original_worker=done,
     direct_cache_request_and_one_worker_iteration_explicit_inputs=True,ordinary_stream_prefetch_order_not_executed=True)
   ready=self.sample_action('direct_original_sample_loaded_cache_query',0x21A960,(sound,ptr,0))
   assert int(ready['return_eax'],16)&2
   before=self.cache_state();data=self.sample_action('direct_original_sample_cache_data_pointer',0x21AA60,regs=((UC_X86_REG_EDX,ptr),))
   after=self.cache_state();bp=int(data['return_eax'],16);size=self.u(ptr+4)&0x3FFFFFFF;aligned=(size+511)&~511
   raw=bytes(self.g.mem_read(bp,size));allraw=bytes(self.g.mem_read(bp,aligned));raw_concat+=raw
   q=directory/f'{scenario}-chunk-{i}.compressed.bin';q.write_bytes(raw)
   a=directory/f'{scenario}-chunk-{i}.aligned.bin';a.write_bytes(allraw)
   samples.append({'ordinal':i,'authored_chunk':metadata(fileoff,12),'runtime_chunk_0c_hex':bytes(self.g.mem_read(ptr,12)).hex(),
    'load':loading,'ready_query':ready,'data_query':data,'before_data_query':before,'after_data_query':after,
    'source_buffer_pointer':f'{bp:08x}','authored_bytes':size,'aligned_bytes':aligned,
    'file_offset':c.u(fileoff)&0x3FFFFFFF,'file_location':c.u(fileoff)>>30,
    'compressed_file':q.name,'compressed_sha256':digest(q),'aligned_file':a.name,'aligned_sha256':digest(a)})
  q=directory/f'{scenario}-selected-permutation.compressed.bin';q.write_bytes(raw_concat)
  assert int(query['return_eax'],16)==len(raw_concat)
  assert all(ARENA+0x1000<=a or ARENA>b for a,b,_ in self.g.mem_regions());self.g.mem_map(ARENA,0x1000)
  encoding=self.u(off+c.magic+4,1)
  fmt=self.sample_action('direct_original_wave_format_from_authored_encoding',0x220520,
   regs=((UC_X86_REG_EAX,encoding),(UC_X86_REG_ESI,ARENA),(UC_X86_REG_EDI,ARENA+0x20)))
  wave=bytes(self.g.mem_read(ARENA,0x14));codec=self.u(ARENA+0x20);vt=self.u(codec)
  assert bytes(self.g.mem_read(codec,4))==self.image.read(codec,4)
  assert bytes(self.g.mem_read(vt,36))==self.image.read(vt,36)
  compact=self.sample_action('direct_original_compact_format_from_actual_wave_format',0x3840C7,(ARENA+0x40,ARENA))
  compact.update(original_compact_0c_hex=bytes(self.g.mem_read(ARENA+0x40,12)).hex(),bounded_format_kernel_not_voice_initialization=True)
  policy=self.image.read(0x451220,16);channel=sum(struct.unpack_from('<3H',policy)[:encoding])
  self.active='direct_unadmitted_original_stream_constructor_from_authored_encoding'
  try:self.sample_action(self.active,0x2215F0,(channel,),((UC_X86_REG_EBX,encoding),))
  except SampleBoundary:pass
  else:raise AssertionError('absent driver constructor unexpectedly returned')
  assert len(self.sample_boundaries)==1
  for _,row in self.sample_returns:row['stopped_before_driver_owned_channel_storage']=True
  self.sample_returns.clear()
  return {'scenario':scenario,'owning_request_and_unvoiced_remove':owning,'tag':tag,'definition_14':metadata(off,20),
   'ugh_tag':ug,'selected_range_0c':metadata(r,12),'selected_permutation_10':metadata(p,16),
   'original_selected_indices':{'pitch':pitch,'permutation':permutation,'initial_chunk':chunk_index},
   'original_byte_total_query':query,'original_word6_query':neutral,'source_loaded_chunks':samples,
   'selected_permutation_file':q.name,'selected_permutation_sha256':digest(q),'source_loaded_total_bytes':len(raw_concat),
   'original_format_kernel':fmt,'original_wave_format_14_hex':wave.hex(),'codec_instance':f'{codec:08x}',
   'original_codec_instance_4_hex':self.image.read(codec,4).hex(),'codec_vtable':f'{vt:08x}',
   'original_codec_vtable_24_hex':self.image.read(vt,36).hex(),'compact_format_kernel':compact,
   'original_driver_policy_10_hex':policy.hex(),'explicit_unadmitted_stream_index':channel,
   'actual_driver_stop':self.sample_boundaries[0],'sample_calls':self.sample_calls,'sample_stores':self.sample_stores,
   'cache_provider_calls':self.provider_calls,'clock_calls':self.clock_calls,'final_source_state':self.source_state(),
   'original_cpu_adpcm_decode_or_packet_submission_executed':False,'driver_voice_device_admission_or_ready_results_supplied':False,
   'native_gameplay_update_applied':False}

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 baseline=ROOT/'analysis/multiplayer/map-pack/smg-audio-valid-owner/measured/report.json';r=json.loads(baseline.read_text())
 context=baseline.parent/'raw-host-context.json';assert digest(context)==r['context_snapshot_sha256'];data=json.loads(context.read_text())
 args.output.parent.mkdir(parents=True,exist_ok=True);directory=args.output.parent/'samples';traces=[];cases=[]
 for i,scenario in enumerate(('ready','reload')):
  t=prepare(data,trace_type=lambda d:SampleTrace(d,r['cases'][i]['clock_calls']));cases.append(t.sample_case(scenario,directory));traces.append(t)
  assert t.replay_cursor==len(r['cases'][i]['clock_calls'])
  print(json.dumps({'scenario':scenario,'chunks':len(cases[-1]['source_loaded_chunks']),'source_loaded_bytes':cases[-1]['source_loaded_total_bytes'],'source_driver_stop':cases[-1]['actual_driver_stop']['instruction']}),flush=True)
 t=traces[0];listings=[];directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
 for e in STATIC:
  code=t.driver_code if t.driver_code.inside(e)else t.code
  f=_trace(code,e,t.starts|set(STATIC));ins=[code.at(a)for a in sorted(f.code)];assert ins,e
  for z in ins:assert t.image.read(z.address,len(z.bytes))==z.bytes and all(bytes(q.g.mem_read(z.address,len(z.bytes)))==z.bytes for q in traces)
  p=directory/f'{e:08x}.asm';p.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins))
  listings.append({'entry':f'{e:08x}','section':code.section.name,'file':p.name,'sha256':digest(p),'instructions':len(ins),
   'calls':[{'site':f'{a:08x}','target':f'{b:08x}','kind':k}for a,b,k in f.sites]})
 executed=set().union(*(q.owner_executed|q.sample_driver_executed for q in traces))
 ins=[(t.driver_code if t.driver_code.inside(a)else t.code).at(a)for a in sorted(executed)]
 for z in ins:assert t.image.read(z.address,len(z.bytes))==z.bytes and all(bytes(q.g.mem_read(z.address,len(z.bytes)))==z.bytes for q in traces)
 p=directory/'executed-original-instructions.asm';p.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins))
 for q,h in FROZEN.items():assert digest(ROOT/q)==h,q
 result={'oracle_sha256':digest(__file__),'source_sha256':PIN,'cache_sha256':CACHE_PIN,'frozen_prior_pins':FROZEN,
  'whole_context_and_clock_source_report':str(baseline.relative_to(ROOT)),'whole_context_sha256':digest(context),
  'context_mode':'unchanged_whole_measured_compatibility_context_replay','clock_mode':'verbatim_measured_valid_owner_inputs',
  'cases':cases,'instruction_listings':listings,'executed_original_listing':{'file':p.name,'sha256':digest(p),'instructions':len(ins)},
  'original_decoded_PCM_packet_submission_or_playback_proven':False,'native_gameplay_update_applied':False}
 args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'output':str(args.output),'prior_pins':len(FROZEN)}))
if __name__=='__main__':main()
