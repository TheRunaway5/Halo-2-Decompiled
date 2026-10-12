#!/usr/bin/env python3
"""Observe actual retained unit reset/variant/default-selection producers.

Extends the unchanged genuine constructor execution with complete state/CPU,
callee and literal metadata observation only. No guest fields or returns added.
"""
import argparse,json,sys,struct
from pathlib import Path
import oracle_mp_unit_constructors_1_1 as original
from unicorn.x86_const import (UC_X86_REG_ESP,UC_X86_REG_EFLAGS,UC_X86_REG_XMM0,
 UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR,UC_X86_REG_FP0)
ROOT=original.ROOT
DIRECT=(0x110a20,0x116680,0x110bf0)
PURE=(0x110800,0x1100e0,0x10f380,0x115030,0x16f510,0x1e0aa0)
LOWER=(0x1e13c0,0x114f00,0x1cfa90,0x114f70,0xbb240,0x114bd0,0x1cf4c0,0x1cfb90,0x115af0,0x1cec20,0x1df670)
OLD_OWNERS=original.OWNERS;OLD_LEAVES=original.LEAVES
class TailTrace(original.ConstructorTrace):
 def __init__(self,*a,**k):
  self.literal_tags={};super().__init__(*a,**k)
 def cpu(self):
  g=self.g
  return dict(registers=self.registers(),esp=f'{g.reg_read(UC_X86_REG_ESP):08x}',eflags=f'{g.reg_read(UC_X86_REG_EFLAGS):08x}',
   xmm128=[f'{g.reg_read(UC_X86_REG_XMM0+i):032x}'for i in range(8)],fpcw=f'{g.reg_read(UC_X86_REG_FPCW):04x}',
   fpsw=f'{g.reg_read(UC_X86_REG_FPSW):04x}',fptag=f'{g.reg_read(UC_X86_REG_FPTAG):04x}',mxcsr=f'{g.reg_read(UC_X86_REG_MXCSR):08x}',
   fp80=[list(g.reg_read(UC_X86_REG_FP0+i))for i in range(8)])
 def buffers(self,row):
  result=super().buffers(row)
  if 'entry_cpu'not in row:row['entry_cpu']=self.cpu()
  else:row['return_cpu']=self.cpu()
  if int(row['entry'],16)==0x1e0aa0:
   row['packed_output']=int(row['stack'][0],16);result['P']=self.raw(row['packed_output'],4)
  return result
 def label(self,p,n,row):
  if 'packed_output'in row and row['packed_output']<=p and p+n<=row['packed_output']+4:return ('P',p-row['packed_output'])
  return super().label(p,n,row)
 def tag_at_span(self,p,n):
  if not n:return
  owners=[t for t in self.cache.tags.values()if t['offset']is not None and t['offset']+self.cache.magic<=p and p+n<=t['offset']+self.cache.magic+t['size']]
  assert len(owners)==1,(hex(p),n,owners)
  self.tag(int(owners[0]['datum'],16))
 def tag(self,datum):
  if datum==0xffffffff:return
  tag=self.cache.tags[datum];at=self.u(self.u(0x4eb114)+(datum&65535)*16+8);n=tag['size']
  assert n>0
  raw=self.raw(at,n)
  key=f'{datum:08x}'
  if key not in self.literal_tags:self.literal_tags[key]=dict(**tag,address=f'{at:08x}',raw_hex=raw)
  # Tags may have actual source runtime postprocessing. Each direct owner also
  # retains its own literal read operands; do not restore a cached tag snapshot.
 def metadata(self,row):
  result=super().metadata(row);entry=int(row['entry'],16)
  if entry in DIRECT+PURE and 'unit'in row:
   obj=self.object(row['unit']);tag=self.u(int(obj['body'],16));self.tag(tag)
   d=self.u(self.u(0x4eb114)+(tag&65535)*16+8);model=self.u(d+0x38)
   result['unit_tag']=f'{tag:08x}';result['model_tag']=f'{model:08x}';self.tag(model)
   if model!=0xffffffff:
    m=self.u(self.u(0x4eb114)+(model&65535)*16+8)
    count=self.u(m+0x50);variants=self.u(m+0x54);assert count<10000
    self.tag_at_span(self.u(m+0x7c),self.u(m+0x78)*0x5c)
    self.tag(self.u(m+0x90))
    for i in range(count):self.tag(self.u(variants+i*0x38+0x34))
  if entry==0x16f510:self.tag(row['registers']['ecx'])
  return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 original.OWNERS=OLD_OWNERS+DIRECT+PURE
 original.LEAVES=tuple(x for x in OLD_LEAVES if x not in DIRECT)+LOWER
 original.ConstructorTrace=TailTrace
 saved=sys.argv
 try:
  sys.argv=[saved[0],'--output',str(out/'owning')];original.main()
 finally:sys.argv=saved
 report=json.loads((out/'owning/report.json').read_text());prior_path=ROOT/'analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/report.json';prior=json.loads(prior_path.read_text())
 assert report['source_stages']==prior['source_stages'] and report['source_state']==prior['source_state']
 common=set(OLD_OWNERS+OLD_LEAVES)
 old=[c for c in prior['calls']if int(c['entry'],16)in common]
 now=[c for c in report['calls']if int(c['entry'],16)in common]
 keys=('entry','caller','phase','registers','stack','before','after','return_eax')
 assert [{k:c.get(k)for k in keys}for c in old]==[{k:c.get(k)for k in keys}for c in now]
 # Full source tags are collected by reading only the actual constructor's
 # original tag-table tokens. A small final metadata query reads all literal
 # graph packets already used by the inherited graph/resource owners.
 # Access the live trace without altering or replaying any source body: the
 # observer's captured reads also retain every runtime pointer/field exactly.
 # Tag capture is written by the temporary wrapper below during the first run;
 # it does not perform another preparation or modify guest memory.
 packets=json.loads((out/'literal-tags.json').read_text())
 value=dict(schema='source-unit-constructor-tail-v1',tool_sha256=original.sha(__file__),owning_report_sha256=original.sha(out/'owning/report.json'),prior_constructor_report_sha256=original.sha(prior_path),
  prior_complete_owner_calls_identical=True,prior_full_source_state_identical=True,owners=[f'{x:08x}'for x in DIRECT+PURE],lower_owners=[f'{x:08x}'for x in LOWER],
  tag_packet_sha256=original.sha(out/'literal-tags.json'),source_calls=len(report['calls']),new_guest_inputs=[],new_source_returns=[],instruction_patches=[],
  boundaries=report['boundaries']+['Read-only original constructor tail observation. Required lower animation resource/transform/callback owners remain explicit; no local controller/device or live grenade is admitted.',
   'Full literal generic tag packets are source-owned read backing, not constructors or invented postprocess/resource fields. Escaped pointer spans must be rejected by any native provider.'])
 original.save(out/'report.json',original.encoded(value));print(json.dumps(dict(report=str(out/'report.json'),sha256=original.sha(out/'report.json'),source_calls=len(report['calls']),tags=len(packets['tags']))))
if __name__=='__main__':
 # Observe the completed live trace once to publish actual tag packets.
 execute=original.execute
 def capture(data):
  t=execute(data)
  for tag in t.cache.tags.values():
   if tag['group']=='jmad':t.tag(int(tag['datum'],16))
  output=Path(sys.argv[sys.argv.index('--output')+1]);assert output.name=='owning'
  value=dict(schema='unit-constructor-literal-tag-packets-v1',source_xbe_sha256=original.PIN,source_cache_sha256=original.CACHE_PIN,tags=t.literal_tags,
   scope='Read-only actual tag-table full payloads after unchanged original constructor/equip/bind composition; source runtime mutations retained. No metadata field is supplied.')
  original.save(output.parent/'literal-tags.json',original.encoded(value));return t
 original.execute=capture
 main()
