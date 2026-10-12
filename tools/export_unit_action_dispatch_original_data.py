#!/usr/bin/env python3
"""Literal selected-image callback/base-extent provider packet; no labels/data defaults."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,_Code,_trace
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--correspondence',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.correspondence.read_text());rows=[]
 sandbox_path=ROOT/'analysis/multiplayer/sandbox-source/later-update-functions/report.json';sandbox=json.loads(sandbox_path.read_text());type_proof=next(x for x in sandbox['routines']if x['disc_1_1_entry']=='000b8a30')
 assert type_proof['normalized_instruction_and_internal_branch_topology_equal'] and len(type_proof['candidates'])==1
 assert sandbox['disc_1_1_sha256']==sha(OLD) and sandbox['later_owned_xbe_sha256']==sha(NEW)
 for path,kind,entry,type_entry in ((OLD,'TU1.1',0xe7f30,0xb8a30),(NEW,'authenticated-u5',int(next(x['u5']for x in r['owners']if x['old']=='000e7f30'),16),int(type_proof['later_entry'],16))):
  image=Xbe(path);code=_Code(image,'.text');f=_trace(code,entry,{entry});sites=[]
  for pc in sorted(f.code):
   i=code.at(pc)
   if i.mnemonic=='mov' and i.op_str.startswith('ebx, dword ptr [eax*4 + '):sites.append(i)
  assert len(sites)==1
  ins=sites[0];bank=ins.operands[1].mem.disp;descriptors=[]
  for t in range(60):
   pointer=struct.unpack('<I',image.read(bank+t*4,4))[0];raw=image.read(pointer,16)
   descriptors.append(dict(type=t,pointer_slot=f'{bank+t*4:08x}',descriptor_address=f'{pointer:08x}',raw10_hex=raw.hex(),callbacks=[f'{v:08x}'for v in struct.unpack('<4I',raw)]))
  # Full B8A30/B9B70 correspondence is already independently frozen in the
  # sandbox source report. Here retain its real indexed table read and unchanged
  # byte+8 load / direct call into our independently matched BD170 owner.
  f=_trace(code,type_entry,{type_entry});sites=[]
  for pc in sorted(f.code):
   i=code.at(pc)
   if i.mnemonic=='mov' and i.op_str.startswith('esi, dword ptr [ecx*4 + '):sites.append(i)
  assert len(sites)==1;type_ins=sites[0];type_bank=type_ins.operands[1].mem.disp;types=[]
  for t in (0,2,5):
   ptr=struct.unpack('<I',image.read(type_bank+t*4,4))[0]
   types.append(dict(type=t,pointer_slot=f'{type_bank+t*4:08x}',record_address=f'{ptr:08x}',raw10_hex=image.read(ptr,16).hex(),base_extent_address=f'{ptr+8:08x}',base_extent_hex=image.read(ptr+8,2).hex(),signed_base_extent=struct.unpack('<h',image.read(ptr+8,2))[0]))
  rows.append(dict(image=kind,path=str(path),sha256=sha(path),action_bank=f'{bank:08x}',action_reader=dict(address=f'{ins.address:08x}',bytes_hex=ins.bytes.hex(),assembly=ins.mnemonic+' '+ins.op_str),descriptors=descriptors,type_bank=f'{type_bank:08x}',type_reader=dict(address=f'{type_ins.address:08x}',bytes_hex=type_ins.bytes.hex(),assembly=type_ins.mnemonic+' '+type_ins.op_str),supported_base_extents=types))
 value=dict(schema='unit-action-original-data-contract-v1',tool_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),type_constructor_correspondence=dict(path=str(sandbox_path.relative_to(ROOT)),sha256=sha(sandbox_path),proof=type_proof),images=rows,
  API=dict(descriptor='array<array<byte,16>,60>, retaining original pointers and allfour callback words without rebasing to host function pointers',base_extent='signed16 literal type-record+8 for actually supported type0/type2/type5; owning B8A30 passes it to complete BD170; no generic1124/604 defaults'),
  boundaries=['These literal tables/extent words do not construct native object bodies, retain animation/actions, bind a player/view, admit source resource residency or implement the referenced callbacks.',
   'The authenticated u5 object constructor identity/table read is corroborated by the existing full sandbox original-byte correspondence; this packet does not relabel annotations as authority. Base-disc image is not imported by this bounded contract.'])
 raw=(json.dumps(value,sort_keys=True,indent=2)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),banks=[(x['image'],x['action_bank'],x['type_bank'])for x in rows])))
if __name__=='__main__':main()
