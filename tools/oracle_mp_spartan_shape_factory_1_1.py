#!/usr/bin/env python3
"""Observe original Spartan phmo initialization from serialized cache records.

This selects the actual existing Spartan definition/tag iterator as an API
request. The unchanged map-postprocess owner supplies every resident shape
pointer, vtable, reference and data word. No native output is consumed.
"""
import argparse,json
from pathlib import Path
from oracle_mp_power_shape_factory_1_1 import ShapeTrace,ROOT,OWNER,sha,verify,ENTRIES,power
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import PIN,CACHE_PIN,_trace
from unicorn import x86_const as reg

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=ShapeTrace)
 t.selected_physics_fixup(0xe9f2086d)
 assert t.shape_call and t.shape_pending==t.g.reg_read(reg.UC_X86_REG_EIP)==0x11ded1
 c=t.shape_call;c.update(return_order=t.shape_order,returned_eax=f'{t.g.reg_read(reg.UC_X86_REG_EAX):08x}',raw_after_hex=bytes(t.g.mem_read(int(c['model_pointer'],16),c['model_size'])).hex(),return_destination_is_exclusive_parent_fragment_stop=True)
 t.shape_order+=1;t.shape_pending=None;listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|ENTRIES);rows=[t.code.at(p)for p in sorted(f.code)]
  for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
  listings.append(dict(entry=f'{entry:08x}',file=p.name,rows=len(rows),sha256=sha(p)))
 report=dict(schema=1,oracle_sha256=sha(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,call=c,reads=t.shape_reads,stores=t.shape_writes,instructions=listings,selected_fixup=t.selected_fixup,instruction_patches=[],new_supplied_guest_fields_or_results=[],boundaries=__doc__)
 frozen_write(a.output,encoded(report));print(json.dumps(dict(report_sha256=sha(a.output),reads=len(t.shape_reads),stores=len(t.shape_writes))))
if __name__=='__main__':main()
