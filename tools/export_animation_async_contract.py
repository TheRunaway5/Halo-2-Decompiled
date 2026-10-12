#!/usr/bin/env python3
"""Retain exact async wrapper bytes/imports and resolve ambiguous tiny owners by xrefs."""
import argparse,csv,json,struct
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,sha,save,encoded,Xbe,_Code,_trace

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--comparison',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();d=json.loads(a.comparison.read_text())
 for name,h in PINS.items():assert sha(name)==h
 images=[Xbe(OLD),Xbe(NEW)];codes=[_Code(x,'.text')for x in images]
 inventories=[ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']
 starts=[{int(q['address'],16)for q in csv.DictReader(p.open(),delimiter='\t')}for p in inventories]
 owners={q['old_entry']:q for q in d['owners']};resolved=[]
 # The actual constructor function-pointer operand, not a semantic label,
 # distinguishes the tiny worker entry from other call/INT3 thunks.
 for old_target,caller,site_kind in [(0x122350,0x1221b0,'push_function_pointer'),(0x2dc892,0x121fd0,'direct_call')]:
  parent=owners[f'{caller:08x}'];assert len(parent['full_body_matches'])==1
  oldfn=_trace(codes[0],caller,starts[0]);newfn=_trace(codes[1],int(parent['u5_entry'],16),starts[1])
  oldins=[codes[0].at(i)for i in sorted(oldfn.code)];newins=[codes[1].at(i)for i in sorted(newfn.code)]
  hits=[]
  for x,y in zip(oldins,newins):
   wanted='push'if site_kind=='push_function_pointer'else'call'
   if x.mnemonic==wanted and len(x.operands)==1 and x.operands[0].type==2 and x.operands[0].imm==old_target:
    assert y.mnemonic==wanted and y.operands[0].type==2
    hits.append((x,y))
  assert len(hits)==1
  x,y=hits[0];new_target=y.operands[0].imm;row=owners[f'{old_target:08x}'];assert f'{new_target:08x}'in row['full_body_matches']
  listings=[]
  for label,entry,code,image,st in [('old',old_target,codes[0],images[0],starts[0]),('u5',new_target,codes[1],images[1],starts[1])]:
   fn=_trace(code,entry,st);ins=[code.at(i)for i in sorted(fn.code)]
   for i in ins:assert image.read(i.address,i.size)==i.bytes
   f=a.output/'instructions'/f'{old_target:08x}-{label}.asm';save(f,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(f.relative_to(a.output)),sha256=sha(f),rows=len(ins)))
  resolved.append(dict(old_entry=f'{old_target:08x}',u5_entry=f'{new_target:08x}',old_site=f'{x.address:08x}',old_bytes=x.bytes.hex(),u5_site=f'{y.address:08x}',u5_bytes=y.bytes.hex(),owning_parent=parent['old_entry'],parent_u5=parent['u5_entry'],kind=site_kind,all_static_candidates=row['full_body_matches'],selected_by_original_owning_reference=True,listings=listings))
 # Additional complete original owner bodies expose TLS/error/time/teardown
 # boundaries. They are not claimed paired u5 or executed by this exporter.
 extra=[]
 for entry in (0x12080,0x2dc70a,0x2d81a6,0x2d81d4,0x1a47d0):
  fn=_trace(codes[0],entry,starts[0]);ins=[codes[0].at(i)for i in sorted(fn.code)]
  for i in ins:assert images[0].read(i.address,i.size)==i.bytes
  f=a.output/'instructions'/f'{entry:08x}-old.asm';save(f,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());extra.append(dict(entry=f'{entry:08x}',file=str(f.relative_to(a.output)),rows=len(ins),sha256=sha(f)))
 kernel=[]
 targets=[0x2dc83a,0x2dc7b9,0x2d8606,0x2d83f6,0x2dc8b2,0x2dc892,0x2dc814,0x2d84c4,0x2d837a,0x2d81d4]
 mapping={q['old_entry']:q['u5_entry']for q in d['owners']if len(q['full_body_matches'])==1};mapping.update({q['old_entry']:q['u5_entry']for q in resolved})
 for entry in targets:
  pairs=[]
  for label,image,code,st,e in [('old',images[0],codes[0],starts[0],entry),('u5',images[1],codes[1],starts[1],int(mapping[f'{entry:08x}'],16)if f'{entry:08x}'in mapping else None)]:
   if e is None:continue
   fn=_trace(code,e,st);imports=[]
   for i in [code.at(v)for v in sorted(fn.code)]:
    for op in i.operands:
     if op.type==3 and not op.mem.base and not op.mem.index and op.size==4:
      try:raw=image.read(op.mem.disp,4)
      except ValueError:continue # Header/FS backing remains a literal runtime input, not an import.
      value=struct.unpack('<I',raw)[0]
      if value&0xffff0000==0x80000000:
       imports.append(dict(site=f'{i.address:08x}',slot=f'{op.mem.disp:08x}',raw4_hex=raw.hex(),encoded=f'{value:08x}',ordinal=value&0xffff,operand_instruction=i.mnemonic+' '+i.op_str))
   pairs.append(dict(image=label,entry=f'{e:08x}',imports=imports))
  if len(pairs)==2:assert [q['ordinal']for q in pairs[0]['imports']]==[q['ordinal']for q in pairs[1]['imports']],hex(entry)
  kernel.append(dict(old_entry=f'{entry:08x}',packets=pairs))
 system=[]
 for label,image,code,entry,end in [('old',images[0],codes[0],0x2d856e,0x2d8606),('u5',images[1],codes[1],int(mapping['002d856e'],16),int(mapping['002d8606'],16))]:
  ins=[];address=entry
  while address<end:
   i=code.at(address);assert image.read(i.address,i.size)==i.bytes;ins.append(i);address+=i.size
  assert address==end and len(ins)==49
  table=ins[1].operands[0].imm;raw=image.read(table,12);words=struct.unpack('<3I',raw)
  assert words[0]==0xffffffff and entry<=words[1]<words[2]<end
  path=a.output/'instructions'/f'002d856e-full-extent-{label}.asm';save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  system.append(dict(image=label,entry=f'{entry:08x}',end_exclusive=f'{end:08x}',file=str(path.relative_to(a.output)),rows=len(ins),sha256=sha(path),scope_table_address=f'{table:08x}',scope_table_raw12_hex=raw.hex(),filter=f'{words[1]:08x}',handler=f'{words[2]:08x}'))
 assert system[0]['end_exclusive']!='' and int(system[0]['end_exclusive'],16)-int(system[0]['entry'],16)==int(system[1]['end_exclusive'],16)-int(system[1]['entry'],16)
 v=dict(schema='animation-async-platform-contract-v1',tool_sha256=sha(__file__),comparison_sha256=sha(a.comparison),source_pins=PINS,ambiguous_owners_resolved_by_actual_xrefs=resolved,additional_complete_original_bodies=extra,kernel_import_packets=kernel,thread_system_routine_full_extent_and_SEH_scope=system,
 scope='Complete raw arguments/returns/alias lifetime are documented separately from platform provider. Original memory fields/handles/status/worker/TLS/IO success are never supplied by this export. Selected tiny owners are resolved by proven caller encoded operands and preserve all alternative candidates. Added original helper/shutdown/file callback listings are not claimed u5-equivalent or executed.',production_edits=[],instruction_patches=[])
 save(a.output/'report.json',encoded(v));print(json.dumps(dict(output=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),resolved=len(resolved),extra_bodies=len(extra))))
if __name__=='__main__':main()
