#!/usr/bin/env python3
"""Unchanged SDK accounting owners on actual partial startup backing.

GetCaps reads genuine retained source counters. APU/SGE construction below is
explicit independent raw-ABI composition after the saved-setting stop; it is
not a successful/resumed DirectSoundCreate or a hardware initialization.
"""
import argparse,json,struct
from pathlib import Path
from unicorn.x86_const import *
from oracle_smg_audio_pool_prefix_1_1 import PoolTrace,FROZEN as PRIOR,ROOT,digest
from oracle_mp_title_update_display import Xbe,OLD,NEW,sha,save,encoded,_Code,_trace

BASE=ROOT/'analysis/multiplayer/integration/sound-capabilities-native'
INPUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement'
INPUT_PINS={'report.json':'b5e6cb66198d911200bf1a4abd943f1182619c9276250a6fe1a2bfec310fb945',
 'raw-host-context.json':'5502cd64e6a7ea04b33a862aa0b6b887f01f342146172d19d267648b1e3be92f'}
ABI_BASE,ABI_STACK,STOP=0xD000000,0xD00F000,0x3000000

class CapTrace(PoolTrace):
 def __init__(self,*a,**kw):
  self.cap_enabled=False;self.cap_writes=[];self.cap_calls=[];self.cap_executed=set();self.cap_regions=[]
  super().__init__(*a,**kw)
 def hook(self,g,a,size,user):
  if self.cap_enabled:
   i=self.ds_code.at(a)
   if i:
    assert i.bytes==self.image.read(a,size)==bytes(g.mem_read(a,size))
    self.cap_executed.add(a)
   if a in(0x381bee,0x380810,0x3804de,0x384c7c,0x38689a,0x3868dc,0x38683d,0x38ad61,0x38b257,0x3817af,0x38034c,0x383ecc,0x383f92,0x3802bf):
    sp=g.reg_read(UC_X86_REG_ESP);self.cap_calls.append(dict(entry=f'{a:08x}',ecx=f'{g.reg_read(UC_X86_REG_ECX):08x}',return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+4*i):08x}'for i in range(4)]))
  super().hook(g,a,size,user)
 def mem_write(self,g,kind,a,n,v,user):
  if self.cap_enabled and (any(lo<=a and a+n<=lo+size for lo,size in self.cap_regions)or any(q['alias']<=a and a+n<=q['alias']+q['capacity']for q in self.pool_blocks)):
   self.cap_writes.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,before_hex=bytes(g.mem_read(a,n)).hex(),after_hex=(v&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
  super().mem_write(g,kind,a,n,v,user)
 def regions(self):
  self.cap_regions=[(0x38D120,0x58),(0x38D74C,4),(0x38D760,4),(0x38D804,4),(self.u(0x526474),0x2AD8)]+[(q['alias'],q['capacity'])for q in self.pool_blocks]
  return [dict(address=f'{a:08x}',raw_hex=bytes(self.g.mem_read(a,n)).hex())for a,n in self.cap_regions]
 def cap_call(self,name,entry,args=(),ecx=None):
  self.cap_enabled=False;before=self.regions();self.active=name
  self.g.mem_write(ABI_STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.g.reg_write(UC_X86_REG_ESP,ABI_STACK)
  if ecx is not None:self.g.reg_write(UC_X86_REG_ECX,ecx)
  start=len(self.cap_writes);cs=len(self.cap_calls);lk=len(self.lock_calls);ps=len(self.pool_services)
  self.cap_enabled=True
  try:self.g.emu_start(entry,STOP,count=2000000)
  finally:self.cap_enabled=False
  assert self.g.reg_read(UC_X86_REG_EIP)==STOP,(name,hex(self.g.reg_read(UC_X86_REG_EIP)))
  return dict(name=name,entry=f'{entry:08x}',arguments=[f'{a:08x}'for a in args],ecx=None if ecx is None else f'{ecx:08x}',return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',initial_regions=before,output_regions=self.regions(),stores=self.cap_writes[start:],source_calls=self.cap_calls[cs:],actual_critical_section_services=self.lock_calls[lk:],actual_raw_pool_services=self.pool_services[ps:])

def execute(out):
 for p,h in PRIOR.items():assert digest(ROOT/p)==h,p
 for p,h in INPUT_PINS.items():assert sha(INPUT/p)==h,p
 pair=json.loads((BASE/'paired-v2/report.json').read_text());data=json.loads((INPUT/'raw-host-context.json').read_text());clocks=json.loads((INPUT/'report.json').read_text())['clock_calls']
 t=CapTrace(data,clocks);pre=t.execute();stopped=t.continue_device();assert t.replay_cursor==len(clocks)
 assert stopped['first_unprovided_dependency']['kernel_ordinal']==24
 saved_registers=t.registers();saved_eip=t.g.reg_read(UC_X86_REG_EIP);saved_esp=t.g.reg_read(UC_X86_REG_ESP)
 t.g.mem_map(ABI_BASE,0x10000)
 driver=t.u(0x526474);device=t.u(0x38D7D8);outaddr=driver+0x2AA0
 params=next(int(c['registers']['ecx'],16)for c in stopped['original_device_calls']if c['entry']=='003832aa')
 cases=[]
 cases.append(t.cap_call('partial_startup_GetCaps_NULL',0x381BEE,(0,outaddr)))
 cases.append(t.cap_call('partial_startup_GetCaps_actual_partial_device',0x381BEE,(device+8,outaddr)))
 allocated=t.cap_call('explicit_APU_raw_pool_allocation',0x3802BF,(0x540,));apu=int(allocated['return_eax'],16);assert apu
 cases.append(allocated)
 cases.append(t.cap_call('independent_complete_APU_constructor',0x384C7C,(params,),apu))
 cases.append(t.cap_call('constructed_APU_GetCaps',0x381BEE,(device+8,outaddr)))
 # Exact authored CPU argument from original3858D8; its preceding hardware
 # initializer is NOT executed or reported successful here.
 authored=t.ds_code.at(0x3858F3)
 assert authored and authored.mnemonic=='push'and authored.operands[0].imm==0x7FF,(authored.address,authored.op_str)
 cases.append(t.cap_call('independent_authored_SGE_descriptor_initializer',0x3868DC,(0x7FF,),apu+0x60))
 cases.append(t.cap_call('constructed_SGE_GetCaps',0x381BEE,(device+8,outaddr)))
 # Declared negative raw-ABI fault controls only. No positive admission or
 # counter value is manufactured; the original reader must preserve output.
 for fault in(1,0x80000000,0xFFFFFFFF):
  t.w(0x38D14C,fault)
  q=t.cap_call(f'negative_raw_fault_{fault:08x}',0x381BEE,(device+8,outaddr));q['declared_negative_fault_control']=f'{fault:08x}';cases.append(q)
 listings={}
 for entry in sorted({int(c['entry'],16)for c in t.cap_calls}):
  f=_trace(t.ds_code,entry,t.starts|{int(c['entry'],16)for c in t.cap_calls});ins=[t.ds_code.at(a)for a in sorted(f.code)]
  p=out/'instructions'/f'{entry:08x}.asm';save(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings[f'{entry:08x}']=dict(path=str(p.relative_to(out)),sha256=sha(p),rows=len(ins))
 report=dict(schema='source-sound-capabilities-original-v1',oracle_sha256=sha(__file__),source_sha256=sha(OLD),paired_report_sha256=sha(BASE/'paired-v2/report.json'),prior_pins=PRIOR,input_pins=INPUT_PINS,original_startup_preparation=pre,source_device_stop=stopped,saved_original_frame=dict(eip=f'{saved_eip:08x}',esp=f'{saved_esp:08x}',registers=saved_registers),cases=cases,listings=listings,
  genuine_partial_device=f'{device:08x}',genuine_partial_parameters=f'{params:08x}',independently_constructed_APU=f'{apu:08x}',source_initialized_78=t.u(t.u(0x4ED950)+0x78,1),
  bounds=['Whole recorded primary compatibility context and measured original clock input are replayed unchanged; fresh source instructions and actual anonymous raw OS pool storage run again. Ordinary Xbox thread/hardware authority is not claimed.',
   'Normal126B20→2205C0→DirectSoundCreate remains stopped at actual ExQueryNonVolatileSetting(XC_AUDIO). Its original stack/lock/partial objects are retained. No saved-setting result or device success is supplied.',
   'Subsequent calls use a separately declared raw ABI stack. Actual partial source device/parameters and source-allocated raw APU storage are composed into independently complete CPU owners; this is not continuation of the saved frame or natural successful startup.',
   'The authored0x7FF SGE argument is read from3858D8 PUSH. Its hardware38B1EB prerequisite is not supplied or reported executed; only the separate complete CPU descriptor initializer is invoked.',
   'GetCaps output is the actual source driver16-byte reserved output. Device argument is adjusted by the public wrapper but never dereferenced by the complete internal reader. Its success means only counter-copy completion, not valid device admission.',
   'Actual pool rounding is native4096-byte backing, not retail Xbox memory rounding. Original allocator owns capacity accounting; original constructors own counters/list heads/refcount. No cap/readiness/voice/status or hardware field is assigned.'],instruction_patches=[],source_capability_fields_assigned=[])
 save(out/'report.json',encoded(report));return report
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=execute(a.output);print(json.dumps(dict(cases=len(r['cases']),stores=sum(len(q['stores'])for q in r['cases']),source_initialized_78=r['source_initialized_78'],report_sha256=sha(a.output/'report.json'))))
