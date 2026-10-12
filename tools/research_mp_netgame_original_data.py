#!/usr/bin/env python3
"""Original netgame postprocess constant operands and source import packets.

Static unchanged original instructions/bytes only. Dynamic coverage is linked
to the independently frozen six-case original report, never native outputs.
"""
import argparse, hashlib, json
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM
from oracle_mp_title_update_display import OLD, NEW, PINS, sha, save, encoded
from research_mp_original_data_u5_audit import Image, fnv

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/netgame-original-data'
PRIOR=ROOT/'analysis/multiplayer/title-update-1.5/netgame-postprocess/measured'
PRIOR_PIN='d3a369d8e75267d1505e282f82836dbb4c3ceb48b4dc6fc3cc2314f9e05ff40c'
CONFIG=(('MapPack11',OLD,'beavercreek-disc_1_1',0x11df00,0x4e7920),
        ('TitleUpdate15',NEW,'beavercreek-owned_1_5',0x11f060,0x4ecc20))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');out=p.parse_args().output
 assert sha(PRIOR/'report.json')==PRIOR_PIN
 prior=json.loads((PRIOR/'report.json').read_text());decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;images=[]
 for label,path,folder,entry,excluded in CONFIG:
  assert sha(path)==PINS[str(path)];image=Image(path);listing=PRIOR/folder/f'{entry:08x}.asm';seen={};all_operands=[];instructions=[]
  cases=[c for c in prior['cases']if c['entry']==f'{entry:08x}'];assert len(cases)==3
  dynamic={int(a,16)for c in cases for a in c['executed_instruction_addresses']}
  for line in listing.read_text().splitlines():
   address,hex_,text=line.split(' ',2);address=int(address,16);raw=bytes.fromhex(hex_);assert image.read(address,len(raw))==raw
   decoded=list(decoder.disasm(raw,address));assert len(decoded)==1 and decoded[0].size==len(raw)
   i=decoded[0];assert i.mnemonic+' '+i.op_str==text
   instructions.append(dict(address=f'{address:08x}',raw_hex=hex_,text=text,covered_by_frozen_original_cases=address in dynamic))
   for o in i.operands:
    if o.type!=X86_OP_MEM or o.mem.base or o.mem.index:continue
    pointer=o.mem.disp;assert o.size==4
    record=dict(instruction=f'{address:08x}',raw_instruction_hex=hex_,text=text,address=f'{pointer:08x}',size=4,
                source_raw4_hex=image.read(pointer,4).hex(),covered_by_frozen_original_cases=address in dynamic,excluded_scenario_global=pointer==excluded)
    all_operands.append(record)
    if pointer==excluded:continue
    assert i.mnemonic=='movss'and o==i.operands[-1]
    if pointer not in seen:
     seen[pointer]=dict(index=len(seen),address=f'{pointer:08x}',raw_file_offset=f'{image.offset(pointer,4):08x}',source_raw4_hex=record['source_raw4_hex'],first_instruction=f'{address:08x}',read_sites=[])
    seen[pointer]['read_sites'].append(f'{address:08x}')
  constants=list(seen.values());assert len(constants)==(7 if label=='MapPack11'else 14)
  packet=b''.join(bytes.fromhex(q['source_raw4_hex'])for q in constants)
  packet_file=out.parent/f'{label}-source-words.bin';save(packet_file,packet)
  images.append(dict(image=label,path=str(path),sha256=sha(path),entry=f'{entry:08x}',excluded_scenario_global=f'{excluded:08x}',
   listing_path=str(listing.relative_to(ROOT)),listing_sha256=sha(listing),instruction_count=len(instructions),instructions=instructions,
   absolute_memory_operands=all_operands,constants_in_unique_first_instruction_order=constants,
   packet_file=packet_file.name,packet_bytes=len(packet),packet_sha256=hashlib.sha256(packet).hexdigest(),packet_fnv1a64=fnv(packet),
   prior_case_correlations=[dict(cache_sha256=c['cache_sha256'],map_id=c['source_map_id'],actual_changed_stores=c['actual_changed_stores'])for c in cases]))
 report=dict(schema=1,tool_sha256=sha(__file__),source_pins=PINS,prior_report_sha256=PRIOR_PIN,prior_doc_sha256=sha(ROOT/'docs/MP_NETGAME_POSTPROCESS_1_1_TO_1_5_RESEARCH.md'),
  images=images,base_optional_patch_absent=True,source_operand_extraction_only=True,native_expected_outputs_used=False,
  scope='All original constant operands and raw words; dynamic admission/stores correlate only to the existing unchanged six-case original freeze. No new execution, map activation, teleport traversal or complete updated game claim.')
 save(out,encoded(report));print(json.dumps(dict(report=str(out),sha256=sha(out),constant_counts=[len(q['constants_in_unique_first_instruction_order'])for q in images])))
if __name__=='__main__':main()
