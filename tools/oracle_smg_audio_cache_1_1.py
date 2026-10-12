#!/usr/bin/env python3
"""Original cache/async/pin instructions with genuine host storage and file services.

No audio device/voice/clock/thread result is fabricated. Original async thread
creation stops; one worker iteration is explicitly called in the supplied
single-worker fixture. Provider handles name actual Python locks/semaphore/file.
"""
import argparse,ctypes,hashlib,json,mmap,struct,threading
from pathlib import Path
from oracle_smg_audio_adapter_1_1 import ENTRIES as ADAPTER_ENTRIES
from oracle_smg_audio_adapter_1_1 import AdapterTrace,SliceEnd,ROOT,PIN,CACHE_PIN,AUDIO_ARENA,AUDIO_CAPACITY,CLASS_ARENA,REQUEST_ARENA
from oracle_smg_fp_sound_1_1 import BoundaryReached
from oracle_smg_firing_1_1 import *
from oracle_smg_constructor_equipped_1_1 import _trace
ADAPTER_PIN='5e0a4af31c9a7689da2d753a6bb0cb77f244ca35daa92cc702941e4f239c264d'
OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-cache'
SERVICES={0x33785C:('VirtualAlloc',4),0x2D79AA:('XPhysicalAlloc',4),0x2D7A1A:('XPhysicalProtect',3),
 0x2DC83A:('CreateMutexA',3),0x2DC7B9:('CreateSemaphoreA',4),0x2DC9C8:('WaitForSingleObject',2),
 0x2DC892:('ReleaseMutex',1),0x2DC814:('ReleaseSemaphore',3),0x2D88C8:('SetFilePointer',4),0x2D8681:('ReadFile',5)}
ENTRIES=(0x12CB70,0x21A6E0,0x13E8E0,0x13EAE0,0x13EFA0,0x21A960,0x21AB20,
0x21AAB0,0x21AAF0,0x21A920,0x16D390,0x1221B0,0x121FD0,0x122020,
 0x1220E0,0x122150,0x122360,0x1A47D0,0x1A4870,0x19D8C0,0x128030,0x1280B0,
 0x127640,0x21AA60,0x13E9A0,0x16D4C0)
class CacheTrace(AdapterTrace):
 def __init__(self):
  self.cache_trace_enabled=False;self.provider_calls=[];self.backing=[];self.live_objects={}
  self.provider_next_vm=0x9000000;self.provider_next_handle=0x77000001
  self.cache_calls=[];self.cache_returns=[];self.cache_writes=[];self.cache_executed=set()
  self.worker_wait_count=None;self.worker_end=None
  self.source_pointer=None;self.chunk_pointer=None
  self.created_sources=[]
  super().__init__();self.cache_trace_enabled=True
 def provider_object(self,kind,obj,extra=None):
  alias=self.provider_next_handle;self.provider_next_handle+=1
  self.live_objects[alias]={'kind':kind,'object':obj,**(extra or {})};return alias
 def storage(self,alias,size,kind):
  size=(size+0xFFF)&~0xFFF
  overlays=[]
  for a,b,_ in list(self.g.mem_regions()):
   start=max(a,alias);end=min(b+1,alias+size)
   if start<end:
    raw=bytes(self.g.mem_read(start,end-start));overlays.append((start,raw))
    self.g.mem_unmap(start,end-start)
  mm=mmap.mmap(-1,size,prot=mmap.PROT_READ|mmap.PROT_WRITE)
  hp=ctypes.addressof(ctypes.c_char.from_buffer(mm))
  self.g.mem_map_ptr(alias,size,3,hp)
  for a,b in overlays:self.g.mem_write(a,b)
  self.backing.append({'object':mm,'host_pointer':hp,'alias':alias,'size':size,'kind':kind})
  return {'mapped_alias':f'{alias:08x}','size':size,'kind':kind,'real_host_mmap_backing':True,
   'preserved_supplied_memory_overlays':[{'address':f'{a:08x}','size':len(b),'sha256':hashlib.sha256(b).hexdigest()}for a,b in overlays]}
 def ret(self,pop,result):
  sp=self.g.reg_read(UC_X86_REG_ESP);dest=self.u(sp)
  self.g.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF)
  self.g.reg_write(UC_X86_REG_ESP,sp+4+pop*4);self.g.reg_write(UC_X86_REG_EIP,dest)
 def provide(self,a):
  name,n=SERVICES[a];sp=self.g.reg_read(UC_X86_REG_ESP);v=[self.u(sp+4+4*i)for i in range(n)]
  row={'phase':self.active,'entry':f'{a:08x}','provider':name,'original_args':[f'{x:08x}'for x in v],
   'real_host_service':True,'source_constructor_result_fabricated':False}
  result=0
  if name=='VirtualAlloc':
   assert v[0]==0 and v[2:]==[0x101000,4]
   result=self.provider_next_vm;rounded=(v[1]+0xFFF)&~0xFFF;self.provider_next_vm+=rounded
   row['storage']=self.storage(result,v[1],'original_cache_VirtualAlloc_request')
  elif name=='XPhysicalAlloc':
   assert v==[0x30E4000,0x61000,0,4]
   result=v[1]|0x80000000;row['storage']=self.storage(result,v[0],'original_physical_arena_request')
  elif name=='XPhysicalProtect':
   assert v[2]==4
   p=next(x for x in self.backing if x['alias']<=v[0] and v[0]+v[1]<=x['alias']+x['size'])
   address=p['host_pointer']+v[0]-p['alias'];lib=ctypes.CDLL(None)
   assert lib.mprotect(ctypes.c_void_p(address),ctypes.c_size_t(v[1]),3)==0
   self.g.mem_protect(v[0],v[1],3);row['real_host_protection_applied']=True
  elif name=='CreateMutexA':
   assert v==[0,0,0];result=self.provider_object('mutex',threading.RLock())
  elif name=='CreateSemaphoreA':
   assert v==[0,0,150,0];result=self.provider_object('semaphore',threading.Semaphore(0),{'count':0,'maximum':150})
  elif name=='WaitForSingleObject':
   assert v[1]==0xFFFFFFFF
   ob=self.live_objects[v[0]]
   if ob['kind']=='semaphore':
    if self.worker_wait_count is not None:
     if self.worker_wait_count==1:
      self.worker_wait_count=None;row['stopped_before_second_worker_wait']=True;self.provider_calls.append(row);raise SliceEnd()
     self.worker_wait_count+=1
    assert ob['count']>0 and ob['object'].acquire(blocking=False)
    ob['count']-=1
   else:assert ob['kind']=='mutex';ob['object'].acquire()
  elif name=='ReleaseMutex':self.live_objects[v[0]]['object'].release();result=1
  elif name=='ReleaseSemaphore':
   ob=self.live_objects[v[0]];assert ob['kind']=='semaphore' and ob['count']+v[1]<=ob['maximum']
   if v[2]:self.w(v[2],ob['count'])
   for _ in range(v[1]):ob['object'].release()
   ob['count']+=v[1];result=1
  elif name=='SetFilePointer':
   ob=self.live_objects[v[0]];assert ob['kind']=='file' and v[3]==0
   high=self.u(v[2])if v[2]else 0
   offset=(high<<32)|v[1];assert offset<0x8000000000000000
   actual=ob['object'].seek(offset)
   if v[2]:self.w(v[2],actual>>32)
   result=actual&0xFFFFFFFF;row.update(verified_file=ob['path'],original_high_word=f'{high:08x}',resulting_offset=actual)
  elif name=='ReadFile':
   ob=self.live_objects[v[0]];assert ob['kind']=='file' and v[4]==0
   start=ob['object'].tell();raw=ob['object'].read(v[2]);assert len(raw)==v[2]
   assert start%512==0 and v[2]%512==0
   self.g.mem_write(v[1],raw);self.w(v[3],len(raw));result=1
   row.update(verified_file=ob['path'],file_offset=start,bytes_read=len(raw),raw_sha256=hashlib.sha256(raw).hexdigest(),
    completion_flag_not_written_by_provider=True)
  row['return_eax']=f'{result&0xFFFFFFFF:08x}';self.provider_calls.append(row);self.ret(n,result)
 def hook(self,g,a,size,user):
  if self.cache_trace_enabled:
   self.cache_executed.add(a)
   for ret,row in self.cache_returns[:]:
    if a==ret:row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';self.cache_returns.remove((ret,row))
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}','registers':self.registers(),
     'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(9)],'return_address':f'{self.u(sp):08x}'}
    self.cache_calls.append(row);self.cache_returns.append((self.u(sp),row))
   if a in SERVICES:self.provide(a);return
   if a==0x2D8606:
    sp=g.reg_read(UC_X86_REG_ESP)
    self.boundaries.append({'phase':self.active,'instruction':f'{a:08x}','dependency':'original async CreateThread unavailable; no source thread handle supplied',
     'original_stack_args':[f'{self.u(sp+4+4*i):08x}'for i in range(6)],'platform_result_supplied':False})
    raise BoundaryReached('original async CreateThread')
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.cache_trace_enabled and (0x4E7900<=address<=0x4E9F00 or 0x5098DC<=address<0x509900 or
    0x9000000<=address<0x9010000 or 0x82E45000<=address<0x83145000):
   self.cache_writes.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
    'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def run(self,a,argv=(),regs=()):
  super().run(a,argv,regs)
  for ret,row in self.cache_returns[:]:
   if ret==STOP:row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';self.cache_returns.remove((ret,row))
 def setup_cache(self):
  self.fp_action('original_physical_arena_producer_real_host_backing',0x12CB70)
  self.g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);self.w(0x517FF0,AUDIO_ARENA);self.w(0x517FF4,0)
  init=self.bounded_action('original_full_audio_constructor_until_genuine_controller_clock',0x126B20)
  assert not init['completed'] and self.u(0x5098DC)!=0 and self.u(0x5098E4)!=0 and self.u(0x5098E0)!=0
  reset=self.bounded_action('original_sound_map_reset_until_genuine_clock',0x126DE0)
  classes=self.initialize_classes_and_ambient()
  async_init=self.bounded_action('original_async_pool_and_lock_producers_until_unavailable_thread',0x1221B0)
  for _,row in self.cache_returns:row['halted_at_dependency']=self.boundaries[-1]['dependency']
  self.cache_returns.clear();self.adapter_returns.clear()
  p=ROOT/'assets/disc/maps/shared.map'
  sha=hashlib.sha256(p.read_bytes()).hexdigest();assert sha=='084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'
  file=p.open('rb');h=self.provider_object('file',file,{'path':str(p.relative_to(ROOT))})
  self.w(0x55F798,h)
  assert self.u(self.u(0x4ED950)+0x78,1)==0
  return {'audio_constructor':init,'map_reset':reset,'async_constructor':async_init,'class_and_ambient_producers':classes,
   'static_audio_backing_input':{'pointer':f'{AUDIO_ARENA:08x}','size':AUDIO_CAPACITY,'initial_used':0,
    'source_startup_static_arena_provider_not_executed':True},
   'file_binding_input':{'slot':'0055f798','provider_handle':f'{h:08x}','real_open_file':str(p.relative_to(ROOT)),'file_sha256':sha,
    'full_original_file_registry_producer_not_claimed':True},'state':self.cache_state()}
 def cache_state(self):
  r={'audio_initialized':self.u(self.u(0x4ED950)+0x78,1),'driver_pointer':f'{self.u(0x526474):08x}',
   'globals':{f'{a:08x}':f'{self.u(a):08x}'for a in(0x5098DC,0x5098E0,0x5098E4,0x4E9EB8,0x4E9EBC,0x4E9EC0,0x4E7934)},
   'pools':[],'async_2580_hex':bytes(self.g.mem_read(0x4E7938,0x2580)).hex()}
  for slot in (0x5098DC,):
   p=self.u(slot)
   if p:
    r['pools'].append({'slot':f'{slot:08x}','pointer':f'{p:08x}','header_hex':bytes(self.g.mem_read(p,0x4C)).hex(),
     'data_hex':bytes(self.g.mem_read(self.u(p+0x44),self.u(p+0x38)*self.u(p+0x24))).hex(),
     'bitmap_hex':bytes(self.g.mem_read(self.u(p+0x48),(self.u(p+0x20)+31)//32*4)).hex()})
  p=self.u(0x5098E4)
  if p:
   r['page_allocator_70_hex']=bytes(self.g.mem_read(p,0x70)).hex();pool=self.u(p+0x64)
   r['page_pool']={'pointer':f'{pool:08x}','header_hex':bytes(self.g.mem_read(pool,0x4C)).hex(),
    'data_hex':bytes(self.g.mem_read(self.u(pool+0x44),self.u(pool+0x38)*self.u(pool+0x24))).hex(),
     'bitmap_hex':bytes(self.g.mem_read(self.u(pool+0x48),(self.u(pool+0x20)+31)//32*4)).hex()}
  if self.source_pointer:
   r['source_bc_hex']=bytes(self.g.mem_read(self.source_pointer,0xBC)).hex()
   r['created_sources']=[{'datum':x['datum'],'pointer':x['pointer'],
    'live_bc_hex':bytes(self.g.mem_read(int(x['pointer'],16),0xBC)).hex()}for x in self.created_sources]
   pool=self.u(0x4ED94C)
   r['source_pool']={'pointer':f'{pool:08x}','header_4c_hex':bytes(self.g.mem_read(pool,0x4C)).hex(),
    'bitmap_hex':bytes(self.g.mem_read(self.u(pool+0x48),(self.u(pool+0x20)+31)//32*4)).hex()}
  if self.chunk_pointer:
   raw=bytes(self.g.mem_read(self.chunk_pointer,12));datum=self.u(self.chunk_pointer+8)
   r['selected_chunk']={'pointer':f'{self.chunk_pointer:08x}','runtime_0c_hex':raw.hex(),'datum':f'{datum:08x}'}
   if datum!=0xFFFFFFFF:
    entry=self.u(self.u(0x5098DC)+0x44)+(datum&0xFFFF)*16
    page=self.u(self.u(self.u(0x5098E4)+0x64)+0x44)+(datum&0xFFFF)*24
    buffer=self.u(0x5098E0)+(self.u(page+8)<<self.u(self.u(0x5098E4)+0x34))
    size=self.u(self.chunk_pointer+4)&0x3FFFFFFF;rounded=(size+511)&~511
    r['selected_chunk'].update(cache_entry_pointer=f'{entry:08x}',cache_entry_10_hex=bytes(self.g.mem_read(entry,16)).hex())
    r['selected_chunk']['buffer']={'pointer':f'{buffer:08x}','authored_bytes':size,'aligned_bytes':rounded,
     'aligned_sha256':hashlib.sha256(bytes(self.g.mem_read(buffer,rounded))).hexdigest(),
     'authored_sha256':hashlib.sha256(bytes(self.g.mem_read(buffer,size))).hexdigest()}
  return r
 def worker_once(self):
  self.worker_wait_count=0;self.active='direct_original_async_worker_one_iteration'
  try:self.run(0x122360,regs=((UC_X86_REG_ECX,0),))
  except SliceEnd:pass
  assert self.worker_wait_count is None
  for _,row in self.cache_returns:row['stopped_before_second_wait']=True
  self.cache_returns.clear();return self.cache_state()
 def creation(self,scenario):
  if scenario=='ready':self.g.mem_write(UNIT+0x212,b'\xff')
  self.execute_fp({'ready':2,'reload':48}[scenario],'idle'if scenario=='ready'else scenario)
  req=[x for x in self.audio_calls if x['entry']=='0021f220'][-1];assert req['return_eax']=='ffffffff'
  self.g.mem_map(REQUEST_ARENA,0x1000)
  return self.create_from_request(req,'direct_original_source_creation_with_real_cache_provider')
 def create_from_request(self,req,name):
  raw=bytes.fromhex(req['play_state_before_hex']);self.g.mem_write(REQUEST_ARENA,raw)
  self.fp_action(name+'_original_listener',0x129450,(REQUEST_ARENA+0x0C,0x3F800000,REQUEST_ARENA+0xA0))
  listener=self.g.reg_read(UC_X86_REG_EAX);assert listener==0 and self.u(REQUEST_ARENA+0xA0)==0
  self.fp_action(name,0x127750,(int(req['stack'][0],16),listener,REQUEST_ARENA,0xFFFFFFFF))
  datum=self.g.reg_read(UC_X86_REG_EAX);assert datum!=0xFFFFFFFF
  p=self.u(self.u(0x4ED94C)+0x44)+(datum&0xFFFF)*0xBC
  self.source_pointer=p
  query=[x for x in self.cache_calls if x['entry']=='0021a960'][-1]
  self.chunk_pointer=int(query['stack'][1],16)
  self.created_sources.append({'datum':f'{datum:08x}','pointer':f'{p:08x}'})
  return {'actual_rejected_fp_request':req,'copied_actual_request_hex':raw.hex(),'created_original_source_datum':f'{datum:08x}',
   'created_source_pointer':f'{p:08x}','created_source_bc_hex':bytes(self.g.mem_read(p,0xBC)).hex(),
   'original_listener':listener,'creation_kernel_call_site_supplied':True,'ordinary_available_admission_or_playback_not_claimed':True}
 def probe(self,name,entry,argv=(),regs=()):
  before=self.cache_state();self.fp_action(name,entry,argv,regs)
  return {'name':name,'entry':f'{entry:08x}','supplied_stack_args':[f'{x:08x}'for x in argv],
   'supplied_registers':[{ 'register':r,'value':f'{v:08x}'}for r,v in regs],
   'return_eax':f'{self.g.reg_read(UC_X86_REG_EAX):08x}','before':before,'after':self.cache_state(),
   'kernel_call_site_supplied':True}
 def lifecycle(self):
  p=self.source_pointer;chunk=self.chunk_pointer;datum=self.u(chunk+8);allocator=self.u(0x5098E4)
  rows=[]
  def add(name,entry,argv=(),regs=()):rows.append(self.probe(name,entry,argv,regs));return rows[-1]
  add('pending_query_without_pin',0x21A960,(0xFFFFFFFF,chunk,0))
  add('pending_source_pin_guard',0x128030,regs=((UC_X86_REG_EAX,p),))
  add('pending_pinned_busy',0x21AAB0,(datum,))
  add('pending_source_unpin',0x1280B0,regs=((UC_X86_REG_ECX,p),))
  add('pending_unpinned_still_busy',0x21AAB0,(datum,))
  add('pending_frame_advance',0x21A920)
  add('pending_full_capacity_request_rejected',0x13EAE0,(allocator,0x300000,0))
  add('pending_source_repin',0x128030,regs=((UC_X86_REG_EAX,p),))
  before=self.cache_state();after=self.worker_once()
  rows.append({'name':'actual_read_and_original_completion','before':before,'after':after,
   'one_worker_iteration_call_site_supplied':True})
  add('loaded_pinned_busy',0x21AAB0,(datum,))
  add('loaded_query_marks_used_without_pin',0x21A960,(0xFFFFFFFF,chunk,0))
  add('loaded_source_pin_guard',0x128030,regs=((UC_X86_REG_EAX,p),))
  add('loaded_pinned_frame_advance',0x21A920)
  add('loaded_pinned_full_capacity_request_rejected',0x13EAE0,(allocator,0x300000,0))
  add('loaded_source_unpin',0x1280B0,regs=((UC_X86_REG_ECX,p),))
  add('loaded_unpinned_not_busy',0x21AAB0,(datum,))
  add('loaded_eligible_frame_advance',0x21A920)
  add('loaded_full_capacity_request_evicts_unpinned_entry',0x13EAE0,(allocator,0x300000,0))
  return rows
 def repeated(self,first):
  before=self.cache_state()
  second=self.create_from_request(first['actual_rejected_fp_request'],'direct_repeated_kernel_from_same_actual_rejected_request')
  created=self.cache_state();workers=[]
  while self.u(0x4E9EBC):workers.append(self.worker_once())
  releases=[]
  for n,item in enumerate((first,second)):
   p=int(item['created_source_pointer'],16);chunk_before=self.cache_state()
   self.fp_action(f'original_repeated_source_{n}_unpin',0x1280B0,regs=((UC_X86_REG_ECX,p),))
   datum=self.u(self.chunk_pointer+8)
   busy=self.probe(f'original_current_chunk_busy_after_source_{n}_unpin',0x21AAB0,(datum,))
   releases.append({'source':item['created_original_source_datum'],'before':chunk_before,'after':self.cache_state(),'busy_probe':busy})
  return {'before_second_creation':before,'second_creation':second,'after_second_creation':created,
   'worker_iterations':workers,'unpin_steps':releases,'repeated_creation_call_site_is_supplied':True,
   'same_request_does_not_claim_second_actual_fp_event':True}

def dump_cache_instructions(t,directory):
 directory.mkdir(parents=True,exist_ok=True);rows=[]
 entries=set(ENTRIES)|set(ADAPTER_ENTRIES)|set(SERVICES)|{0x126B20,0x126DE0,0x2D8606,0x11E150,0x1656A0}
 for entry in sorted(entries):
  f=_trace(t.code,entry,t.starts|entries);ins=[t.code.at(a)for a in sorted(f.code)]
  p=directory/f'{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  rows.append({'entry':f'{entry:08x}','path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
   'instructions':len(ins),'executed_original_instruction_rows':sum(i.address in t.cache_executed and i.address not in SERVICES for i in ins),
   'host_provider_entry_instructions_substituted':entry in SERVICES,
   'calls':[{'site':f'{s:08x}','target':f'{d:08x}'}for s,d,k in f.sites if k=='call']})
 p=directory/'inventory.json';p.write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')
 return {'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--case',choices=('ready','reload','ready_lifecycle','reload_lifecycle','ready_queued','reload_shared'),default='ready');ap.add_argument('--output',type=Path,default=OUT/'working.json');ap.add_argument('--instructions',type=Path);args=ap.parse_args()
 assert hashlib.sha256((ROOT/'tools/oracle_smg_audio_adapter_1_1.py').read_bytes()).hexdigest()==ADAPTER_PIN
 t=CacheTrace();setup=t.setup_cache();created=t.creation(args.case.split('_')[0]);queued=t.cache_state();lifecycle=[];repeated=None
 if args.case.endswith('_lifecycle'):
  lifecycle=t.lifecycle();done=lifecycle[8]['after']
 elif args.case.endswith(('_queued','_shared')):
  repeated=t.repeated(created);done=repeated['worker_iterations'][-1]
 else:
  done=t.worker_once();p=int(created['created_source_pointer'],16);t.fp_action('original_source_chunk_unpin',0x1280B0,regs=((UC_X86_REG_ECX,p),))
 result={'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'adapter_dependency_sha256':ADAPTER_PIN,
  'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'case':args.case,'setup':setup,'created':created,'queued':queued,
  'completed':done,'final_state':t.cache_state(),'lifecycle_probes':lifecycle,'repeated_probe':repeated,'calls':t.cache_calls,'writes':t.cache_writes,
  'source_calls':t.adapter_calls,'source_writes':t.adapter_writes,'rng_stores':t.adapter_rng,'authored_sound':t.authored_sound(),
  'provider_calls':t.provider_calls,'boundaries':t.boundaries,'audio_available_forced':False,
  'real_audio_device_or_driver_executed':False,'original_async_thread_execution_claimed':False,
  'original_worker_one_iteration_call_site_supplied':True,'native_gameplay_update_applied':False,
  'supplied_inherited_services':{f'{a:08x}':{'name':name,'return':value,'cleanup':pop}for a,(name,value,pop)in t.services.items()},
  'host_provider_virtual_addresses_and_handles_are_supplied_boundaries':True,
  'source_cache_fields_or_completion_flags_written_by_fixture':False,
  'scope':'Actual original cache constructors/allocation/read queue/pin/eviction instructions. Real host providers own storage, locks, semaphore and file. The original thread creation and clocks remain unavailable; one original worker iteration and direct source creation/probe call sites are explicit supplied boundaries. Initialized78 stays0 and no ordinary sound availability, decoder, driver or audible playback parity is claimed.'}
 if args.instructions:result['instruction_inventory']=dump_cache_instructions(t,args.instructions.resolve())
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'path':str(args.output),'source':created['created_original_source_datum'],'provider_calls':len(t.provider_calls)}))
if __name__=='__main__':main()
