#!/usr/bin/env python3
"""Separate read-only consumed-options/network-gate transfer measurement."""
import argparse,json,sys
from pathlib import Path
import oracle_mp_weapon_attachment_1_1 as source
from oracle_mp_item_current_producers_1_1 import ROOT,OUT as CURRENT,FROZEN as PRIOR,digest,encoded,frozen_write
from oracle_mp_inventory_1_1 import POOL
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP
BASE=ROOT/'analysis/multiplayer/integration/inventory-source'
OUT=BASE/'pickup-transfer-options'
FROZEN={**PRIOR,
 'tools/oracle_mp_item_current_producers_1_1.py':'cc04b51cd4ce882c25323ce534c264373356930725cf015a93b2908e8058bb81',
 'tools/oracle_mp_item_current_producers_audit_1_1.py':'a20200230373f9e2e077af989c6dae4bbcfee8934221c72db1e5c8e5eeebbe8e',
 'docs/MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md':'16f291f82614d6cc58bab95793feda8a5ba77cf19de089202cee9a2118aa13e5',
 'analysis/multiplayer/integration/inventory-source/item-current-producers/report.json':'8e0351b19fbae952a6dd895f77489fed31cb9f04723ce2b5814fa0f656181a76',
 'analysis/multiplayer/integration/inventory-source/item-current-producers/manifest.json':'a8574f99b0564da82c41f775ed0f553425b9e3f21df27a4d04d5a2f651738ab2'}
INSTANCES=[]
ENTRIES={0xcfe60,0xcfed0,0xcf8c0,0xcfb00,0xb6760,0xfb120,0xfb3f0}
SITES={0xcfe66,0xcfe9b,0xcfea2,0xcfeb6,0xcff0f,0xcff16,0xcff30,0xcff59}

class ObserveOptions:
 def __init__(self,*args,**kwargs):
  self.options_calls=[];self.options_sites=[];self.options_stores=[];self.options_pending=[];self.options_order=0
  INSTANCES.append(self);super().__init__(*args,**kwargs)
 def selected_unit(self,datum):
  return self.u(self.u(POOL+0x44)+(datum&0xffff)*12+8)
 def hook(self,g,a,size,user):
  for ret,call in reversed(self.options_pending[:]):
   if a==ret:
    call['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';call['return_order']=self.options_order;self.options_order+=1
    if 'unit_address'in call:call['unit_after_hex']=bytes(g.mem_read(int(call['unit_address'],16),0x800)).hex()
    self.options_pending.remove((ret,call));break
  if a in ENTRIES:
   sp=g.reg_read(UC_X86_REG_ESP);call=dict(id=len(self.options_calls),entry=f'{a:08x}',phase=self.active,
    order=self.options_order,caller=f'{self.u(sp):08x}',registers=self.registers(),stack=[f'{self.u(sp+4+n*4):08x}'for n in range(4)])
   self.options_order+=1
   if a in (0xcfe60,0xcfed0):
    datum=g.reg_read(UC_X86_REG_EAX)if a==0xcfe60 else self.u(sp+4);unit=self.selected_unit(datum)
    call.update(unit_datum=f'{datum:08x}',unit_address=f'{unit:08x}',unit_before_hex=bytes(g.mem_read(unit,0x800)).hex())
   self.options_calls.append(call);self.options_pending.append((self.u(sp),call))
  if a in SITES:
   row=dict(instruction=f'{a:08x}',phase=self.active,order=self.options_order,registers=self.registers())
   self.options_order+=1
   if a==0xcfe66:
    p=g.reg_read(UC_X86_REG_ECX)
    row.update(options_global_pointer=f'{self.u(0x4edf18):08x}',consumed_options_pointer=f'{p:08x}',
     options_prefix_hex=bytes(g.mem_read(p,16)).hex(),consumed_options_byte_0c=f'{self.u(p+12,1):02x}',
     cfe60_call_id=next(c['id']for c in reversed(self.options_calls)if c['entry']=='000cfe60'))
   if a in (0xcff59,0xcfeb6):
    unit=g.reg_read(UC_X86_REG_EBP)if a==0xcff59 else self.selected_unit(next(int(c['unit_datum'],16)for c in reversed(self.options_calls)if c['entry']=='000cfe60'))
    row.update(unit_address=f'{unit:08x}',stored_network_entity=f'{self.u(unit+0xd4):08x}',
     compared_network_entity=f'{g.reg_read(UC_X86_REG_EAX):08x}')
   self.options_sites.append(row)
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  a=g.reg_read(UC_X86_REG_EIP)
  if a in SITES:
   self.options_stores.append(dict(instruction=f'{a:08x}',phase=self.active,order=self.options_order,address=f'{address:08x}',
    size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
   self.options_order+=1
  super().mem_write(g,access,address,size,value,user)

def repeat(out,name,measured):
 original_class=source.AttachmentTrace;write=Path.write_text;argv=sys.argv;canonical=BASE/'weapon-attachment/report.json'
 def protect(path,data,*args,**kwargs):
  raw=data.encode(kwargs.get('encoding')or'utf-8');p=path.resolve()
  if p.parent==canonical.parent/'instructions':assert p.read_bytes()==raw,p
  else:
   assert p==canonical and canonical.read_bytes()==raw,p
   frozen_write(out/name,raw)
  return len(data)
 try:
  if measured:source.AttachmentTrace=type('OptionsAttachmentTrace',(ObserveOptions,original_class),{})
  Path.write_text=protect;sys.argv=['attachment','--output',str(canonical)];source.main()
 finally:source.AttachmentTrace=original_class;Path.write_text=write;sys.argv=argv
 assert (out/name).read_bytes()==canonical.read_bytes()
 return dict(file=name,sha256=digest(out/name),byte_identical=True,frozen_listing_writes_compared_and_suppressed=True)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,default=OUT);args=ap.parse_args();out=args.output_dir
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 preflight=repeat(out,'preflight.json',False);measured=repeat(out,'measured-baseline.json',True)
 old=json.loads((BASE/'pickup-transfer-measured/report.json').read_text());cases=[];correlated=0
 for instance,prior in zip(INSTANCES,old['cases']):
  assert not instance.options_pending
  for entry in ('000cfe60','000cfed0'):
   aa=[r for r in instance.options_calls if r['entry']==entry];bb=[r for r in prior['calls']if r['entry']==entry]
   assert len(aa)==len(bb)
   for a,b in zip(aa,bb):
    assert a['phase']==b['phase'] and a['registers']==b['registers'] and a['stack']==b['stack'][:4] and a['return_eax']==b['return_eax']
    assert a['unit_before_hex']==prior['states'][b['before_state']]['unit']['body_prefix_hex']
    assert a['unit_after_hex']==prior['states'][b['after_state']]['unit']['body_prefix_hex']
    a['frozen_transfer_call_id']=b['id'];correlated+=1
  cases.append(dict(name=prior['name'],calls=instance.options_calls,sites=instance.options_sites,stores=instance.options_stores))
 instructions=[]
 for a in sorted(SITES|ENTRIES):
  i=INSTANCES[0].code.at(a);assert INSTANCES[0].image.read(a,len(i.bytes))==i.bytes
  assert all(bytes(t.g.mem_read(a,len(i.bytes)))==i.bytes for t in INSTANCES)
  instructions.append(dict(address=f'{a:08x}',raw_original_hex=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str))
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 d=dict(schema=1,tool_sha256=digest(__file__),source_pins=FROZEN,preflight=preflight,measured_baseline=measured,cases=cases,
  original_instruction_bytes=instructions,correlated_frozen_transfer_calls=correlated,instruction_patches=[],
  boundary='Read-only repeat of unchanged frozen allocated/equipped attachment fixture. Options byte is observed at owningCFE66 compare, never written by this extension or inferred from setup. Earlier supplied root/storage/residency/scheduler and direct-drop Havok result remain explicit. NetworkdirtyB6760 neverexecutes because sourceunitD4NONE; no unexecuted callback result/input is invented. Live player/world/contact/animatedpose remain separate.')
 frozen_write(out/'report.json',encoded(d))
 print(json.dumps(dict(report_sha256=digest(out/'report.json'),cases=len(cases),correlated=correlated,
  options=[r['consumed_options_byte_0c']for c in cases for r in c['sites']if r['instruction']=='000cfe66'])))
if __name__=='__main__':main()
