#!/usr/bin/env python3
"""Unchanged TU1.1 Needler overlay lookup, frame selector and partial pose sampler.

Inherits the declared registered SMG/platform fixture. Needler FP resources are
literal cache bytes. Cases explicitly supply loaded/reserve/reload fields and
manager categories; whole weapon reload, Unit lifecycle and Havok are excluded.
No instruction is replaced and no native output supplies an expected result.
"""
import hashlib,json,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from oracle_smg_fp_clock_1_1 import ClockTrace,ROOT,CAMERA,PIN,CACHE_PIN
from oracle_smg_firing_1_1 import (HEADERS,STACK,UC_X86_REG_EAX,UC_X86_REG_EBX,
 UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)

def main():
 t=ClockTrace();t.fp_enabled=False;c=t.cache;graph=c.tags[0xe3c50250]
 executed=set()
 def observe(g,address,size,user):
  if t.active=='original169728_ammunition_selector_and_sampler':executed.add(address)
 t.g.hook_add(UC_HOOK_CODE,observe)
 resident=CAMERA+0x100000;resources=[]
 for i,off in enumerate(c.block(graph['offset']+0xac,20)):
  size,pointer=c.u(off+4),c.u(off+8);location=pointer>>30;offset=pointer&0x3fffffff
  source=ROOT/'assets/disc/maps'/('lockout.map','mainmenu.map','shared.map','single_player_shared.map')[location]
  with source.open('rb')as f:f.seek(offset);raw=f.read(size)
  assert len(raw)==size
  t.g.mem_write(resident,raw);t.resident[off+c.magic]={'resident_pointer':resident}
  resources.append({'index':i,'source':str(source.relative_to(ROOT)),'offset':offset,'size':size,'sha256':hashlib.sha256(raw).hexdigest()});resident+=(size+4095)&~4095
 assert resident<CAMERA+0x300000
 hand=CAMERA+0x4000;manager=hand+0x14;out=hand+0x400;locals_=hand+0x500;framebase=CAMERA+0x8000
 t.g.mem_write(manager,bytes(t.g.mem_read(t.u(0x4f1198)+0x14,0x90)))
 t.w(manager+0x68,0xe3c50250);t.w(manager+0x70,0x030000d9);t.w(manager+0x78,0x030000d9)
 weapon=t.u(HEADERS+(t.initial_datum&65535)*12+8);definition=c.tags[0xe34b01d6]['offset']+c.magic
 t.w(weapon,0xe34b01d6)
 nodes=len(c.block(graph['offset']+0xc,32));assert nodes==55
 # Nonidentity locals prove that only the authored components are replaced.
 base=b''.join(struct.pack('<8f',0,0,0,1,7+i*.125,8+i*.25,9+i*.5,2+i*.03125)for i in range(nodes))
 rows=[];lookup=[]
 for dual in range(2):
  t.w(manager+0x74,0x0400054b if dual else 0x030000d9)
  t.active='original169965_ammunition_lookup'
  t.run(0x1cf3c0,(0x07000101,),((UC_X86_REG_EAX,manager),(UC_X86_REG_EBX,out),(UC_X86_REG_EDX,0x07000101),(UC_X86_REG_EDI,0x0a000069)))
  identifier=t.u(out);assert identifier>>16==dual
  lookup.append({'dual':dual,'identifier':f'{identifier:08x}'})
  t.w(hand+0xe0,identifier);t.w(hand+0x300,nodes)
  cases=[(loaded,120,0,0x0400000c)for loaded in (-32768,-1,*range(34),32767)]
  cases +=[(loaded,reserve,elapsed,activity)for loaded in (0,7,29)for reserve in(0,3,120)
    for elapsed in(0,43,44,45,46,47,48,49,50,64,32767)for activity in(0x0c000064,0x0b000065)]
  for loaded,reserve,elapsed,activity in cases:
   t.fp_enabled=False;t.sh(weapon+0x22c,loaded);t.sh(weapon+0x22a,reserve)
   t.sh(weapon+0x226,0);t.sh(weapon+0x228,elapsed);t.w(manager+0x7c,activity)
   t.w(framebase-4,locals_);t.w(framebase-8,manager);t.w(framebase-0x10,hand)
   t.w(framebase-0x18,definition);t.w(framebase-0x20,weapon);t.w(framebase-0x24,t.initial_datum)
   t.g.mem_write(locals_,base);t.active='original169728_ammunition_selector_and_sampler';t.fp_enabled=True;t.count=0
   t.g.reg_write(UC_X86_REG_ESP,STACK);t.g.reg_write(UC_X86_REG_EBP,framebase);t.g.reg_write(UC_X86_REG_EDI,hand)
   t.g.emu_start(0x169728,0x16984b,count=1000000)
   assert t.g.reg_read(UC_X86_REG_EIP)==0x16984b
   channel_frame=t.u(framebase-0x44+0x1c);raw=bytes(t.g.mem_read(locals_,nodes*32))
   rows.append((dual,loaded,reserve,elapsed,activity,channel_frame,raw))
 fixture=ROOT/'tests/fixtures/needler_ammunition_original.tsv'
 fixture.write_text('# dual loaded reserve elapsed activity channel_frame_bits and 55 original local pose records\n'+''.join(
  f'{dual} {loaded} {reserve} {elapsed} {activity:08x} {frame:08x} '+raw.hex()+'\n'for dual,loaded,reserve,elapsed,activity,frame,raw in rows))
 report={'scope':__doc__,'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
  'resources':resources,'lookup':lookup,'nodes':nodes,'cases':len(rows),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),
  'inherited_platform_services':sorted({e['service']for e in t.events if 'service'in e}),
  'range':['00169728','0016984b'],'original_reload_fraction_constant_bits':f'{t.u(0x4650cc):08x}',
  'whole_unit_and_reload_lifecycle_proven':False,
  'executed_unique_instructions':len(executed),'executed_addresses_sha256':hashlib.sha256(b''.join(struct.pack('<I',a)for a in sorted(executed))).hexdigest(),
  'executed_ammunition_decoder_entries':[f'{a:08x}'for a in (0x1ca950,0x293200,0x293350)if a in executed],
  'reciprocal_convention':'Unicorn RCPSS binary32 reciprocal; not a physical Xbox approximation-seed proof'}
 path=ROOT/'analysis/multiplayer/integration/needler-ammunition-original-20261012.json';path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({'cases':len(rows),'lookup':lookup,'fixture':str(fixture)}))
if __name__=='__main__':main()
