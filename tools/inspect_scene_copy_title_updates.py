#!/usr/bin/env python3
"""Static whole-owner scene-copy correspondence across original Xbox releases.

Masked prefixes locate candidates only. Complete reached instruction tokens,
branch/switch topology, read-only operand bytes and references between admitted
owners constrain correspondence. This does not execute a title-update copy or
relocate captured retail GPU/runtime resource banks into a later image.
"""
import argparse,csv,hashlib,json,re
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM
from oracle_game_over import SPECS
from oracle_camouflage_scene_copy import OWNERS
from inspect_render_primitives_title_updates import prefix,section
from oracle_mp_full_scoreboard_1_5 import _Code,literal,complete_tokens,references
from research_mp_lobby_later_update import table_pair,containing,backing
from xbe import Xbe,FUNCTIONS_CSV
from inventory import read_rows
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/scene-copy-title-updates-20261012'
def sha(raw):return hashlib.sha256(raw).hexdigest()
TABLE_OWNERS=('target_dimensions','target_surface')
def table_pointer(ins,image,original=False):
 rows=[i for i in ins if i.mnemonic=='add' and len(i.operands)==2 and i.operands[1].type==X86_OP_IMM
  and (s:=containing(image,i.operands[1].imm)) and s.name=='.data']
 assert len(rows)==1
 if original:assert rows[0].operands[1].imm==0x4b4b58
 return rows[0]
def tokens(ins,image,owner,original=False):
 result=complete_tokens(ins,image)
 if owner in TABLE_OWNERS:
  pointer=table_pointer(ins,image,original)
  result[ins.index(pointer)]=pointer.mnemonic+' '+pointer.op_str.rsplit(',',1)[0]+', TARGET_RECORD_TABLE_BASE'
 return result
def complete_prefix(image,entry,size,owner):
 # A64-byte locator ending inside an instruction retains unmasked partial
 # relocation bytes. Trim to the final completely decoded instruction first.
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
 decoded=list(cs.disasm(image.read(entry,min(size,64)),entry))
 assert decoded
 end=decoded[-1].address+decoded[-1].size-entry
 if owner not in TABLE_OWNERS:return prefix(image,entry,end)
 raw=image.read(entry,end);mask=bytearray([255]*end)
 for i in decoded:
  if i.mnemonic=='add' and len(i.operands)==2 and i.operands[1].type==X86_OP_IMM and i.operands[1].imm==0x4b4b58:
   at=i.address-entry+i.imm_offset;mask[at:at+i.imm_size]=bytes(i.imm_size)
  elif i.mnemonic.startswith('j'):
   at=i.address-entry+i.imm_offset;mask[at:at+i.imm_size]=bytes(i.imm_size)
 return b''.join(re.escape(bytes([v]))if keep else b'.'for v,keep in zip(raw,mask))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT);args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=True)
 images={};starts={};codes={}
 for name,spec in SPECS.items():
  image=Xbe(ROOT/spec['path']);assert sha(image.data)==spec['pin'];images[name]=image
  starts[name]={int(r['address'],16)for r in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
  codes[name]={s.name:_Code(image,s.name)for s in image.sections if s.name in('.text','D3D')}
 indexed=read_rows(FUNCTIONS_CSV);old=images['retail'];functions={};candidates={};locators=[]
 for owner,entry in OWNERS.items():
  original=literal(codes['retail'][section(old,entry).name],entry,starts['retail']);functions['retail',owner]=original
  pattern=complete_prefix(old,entry,int(indexed[entry]['size']),owner)
  for name in('tu11','u5'):
   possible=[];accepted=[];rejected=[];image=images[name]
   for s in image.sections:
    if s.name not in('.text','D3D'):continue
    for match in re.finditer(pattern,image.section_bytes(s),re.S):
     candidate=s.va+match.start();possible.append(candidate)
     f,ins=literal(codes[name][s.name],candidate,starts[name]);of,oi=original
     tables=table_pair(of,f,oi,ins,old,image)
     shape=tokens(oi,old,owner,True)==tokens(ins,image,owner)
     topology=tables is not None and all(x['topology_equal']for x in tables)
     refs=references(oi,ins,old,image) if shape else []
     constants=all(r['old']['bytes_hex']==r['later']['bytes_hex']for r in refs if r['old']['section']=='.rdata')
     if shape and topology and constants:
       accepted.append(dict(entry=candidate,function=f,instructions=ins))
     else:rejected.append(dict(entry=candidate,complete_instruction_tokens_equal=shape,switch_topology_equal=topology,
      checked_read_only_operand_bytes_equal=constants if shape else None,
      read_only_mismatches=[r for r in refs if r['old']['section']=='.rdata' and r['old']['bytes_hex']!=r['later']['bytes_hex']]))
   candidates[name,owner]=accepted;locators.append(dict(image=name,owner=owner,prefix_candidates=possible,
    complete_candidates=[r['entry']for r in accepted],rejected_candidates=rejected))
 # Infer no arbitrary choice: complete correspondence must be unique, either
 # directly or through a matching literal reference in a settled original owner.
 entries={'retail':OWNERS.copy()};constraints=[];unresolved=[]
 for name in('tu11','u5'):
  pending=set(OWNERS);settled={}
  while pending:
   progress=False
   for owner in sorted(pending):
    allowed=candidates[name,owner]
    for source,target in settled.items():
     old_ins=functions['retail',source][1];new_ins=target['instructions']
     for a,b in zip(old_ins,new_ins):
      if a.mnemonic in('call','push','mov') and a.op_str.endswith(hex(OWNERS[owner])):
       match=re.search(r'0x[0-9a-f]+$',b.op_str)
       if match:
        address=int(match.group(),16);allowed=[row for row in allowed if row['entry']==address]
        constraints.append(dict(image=name,owner=owner,source_owner=source,old_site=a.address,later_site=b.address,later_target=address))
    if len(allowed)==1:
     settled[owner]=allowed[0];pending.remove(owner);progress=True
   if not progress:break
  unresolved.extend(dict(image=name,owner=owner,old_entry=OWNERS[owner],
   remaining_complete_candidates=[r['entry']for r in candidates[name,owner]],
   reason='No unique complete correspondence; prefix search and rejected candidates do not authorize equivalence')for owner in sorted(pending))
  entries[name]={owner:row['entry']for owner,row in settled.items()}
  functions.update({(name,owner):(row['function'],row['instructions'])for owner,row in settled.items()})
 listings=[];comparisons=[]
 for name,image in images.items():
  mapped=dict(zip(OWNERS.values(),entries[name].values())) if name=='retail' else {OWNERS[owner]:entry for owner,entry in entries[name].items()}
  for owner,entry in entries[name].items():
   f,ins=functions[name,owner];path=args.output/f'{name}-{owner}-{entry:08x}.asm'
   path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
   listings.append(dict(image=name,owner=owner,entry=entry,instructions=len(ins),listing=path.name,listing_sha256=sha(path.read_bytes())))
   if name=='retail':continue
   of,oi=functions['retail',owner];refs=references(oi,ins,old,image);calls=[]
   for a,b in zip(oi,ins):
    if a.mnemonic=='call' and a.op_str.startswith('0x') and int(a.op_str,16)in mapped:
     target=mapped[int(a.op_str,16)];assert int(b.op_str,16)==target
     calls.append(dict(old_site=a.address,later_site=b.address,old_target=int(a.op_str,16),later_target=target))
   table_operand=None
   if owner in TABLE_OWNERS:
    a=table_pointer(oi,old,True);b=table_pointer(ins,image)
    table_operand=dict(old_site=a.address,later_site=b.address,old_address=a.operands[1].imm,later_address=b.operands[1].imm,
     kind='Explicit98-byte target-record table address in ADD; all other arithmetic operands unchanged',
     old=backing(old,a.operands[1].imm,4),later=backing(image,b.operands[1].imm,4))
   comparisons.append(dict(image=name,owner=owner,old_entry=OWNERS[owner],later_entry=entry,
    complete_shape_internal_branches_equal=True,instructions=len(ins),switch_tables=table_pair(of,f,oi,ins,old,image),
    relocated_operands=refs,all_referenced_rdata_bytes_equal=True,owner_direct_calls=calls,target_table_operand=table_operand))
 for name in('tu11','u5'):
  tables=[r['target_table_operand']for r in comparisons if r['image']==name and r['owner']in TABLE_OWNERS]
  assert len(tables)==2 and len({r['later_address']for r in tables})==1
 report=dict(schema='original-scene-copy-title-updates-v1',boundary=__doc__,entries=entries,
  images={n:dict(path=str(Path(SPECS[n]['path'])),sha256=SPECS[n]['pin'])for n in images},
  prefix_locators=locators,owning_reference_constraints=constraints,owners=listings,complete_comparisons=comparisons,
  unresolved_correspondences=unresolved,
  dependencies={str(p.relative_to(ROOT)):sha(p.read_bytes())for p in(Path(__file__).resolve(),ROOT/'tools/oracle_camouflage_scene_copy.py',
   ROOT/'tools/inspect_render_primitives_title_updates.py',ROOT/'tools/oracle_mp_full_scoreboard_1_5.py',ROOT/'tools/research_mp_lobby_later_update.py',ROOT/'tools/oracle_game_over.py')},
  instruction_patches=[],supplied_returns=[],limits=['Static bounded owner correspondence only; external callees and mutable title-specific banks remain separate proof obligations.',
   'Retail whole-copy CPU and destination executions do not become title-update executions from this static comparison.',
   'No GPU resource capture, submission/completion, pixels, normal camouflage/water flow or additional variant admission.'])
 path=args.output/'report.json';path.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(owners=len(OWNERS),complete_comparisons=len(comparisons),unresolved=len(unresolved),entries=entries,report_sha256=sha(path.read_bytes())),indent=2))
if __name__=='__main__':main()
