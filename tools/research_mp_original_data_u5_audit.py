#!/usr/bin/env python3
"""Independent raw-section, instruction and balance-constant evidence audit."""
import argparse, hashlib, json, struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_mp_title_update_display import OLD, NEW, PINS, sha, save, encoded

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/original-data-u5'
COMBAT=ROOT/'analysis/multiplayer/title-update-1.5/combat-postprocess/measured/report.json'
COMBAT_PIN='9300d1ac08b64a4f1eb815e01ac8e1d365b37b2483a89bf495aa99922815da11'

def fnv(raw):
 value=0xcbf29ce484222325
 for byte in raw:value=((value^byte)*0x100000001b3)&0xffffffffffffffff
 return f'{value:016x}'

class Image:
 def __init__(self,path):
  self.raw=Path(path).read_bytes()
  base,count,table=(struct.unpack_from('<I',self.raw,p)[0]for p in (0x104,0x11c,0x120))
  self.sections=[]
  for n in range(count):
   p=table-base+n*56
   va,size,offset,raw_size=struct.unpack_from('<IIII',self.raw,p+4)
   self.sections.append((va,size,offset,raw_size))
 def offset(self,address,length):
  matches=[offset+address-va for va,_,offset,size in self.sections if va<=address and address+length<=va+size]
  assert len(matches)==1,(address,length)
  return matches[0]
 def read(self,address,length):
  matches=[s for s in self.sections if s[0]<=address and address+length<=s[0]+s[1]]
  assert len(matches)==1,(address,length)
  va,_,offset,raw_size=matches[0];start=address-va
  return bytes(self.raw[offset+p]if p<raw_size else 0 for p in range(start,start+length))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);out=p.parse_args().output_dir
 r=json.loads((out/'report.json').read_text())
 assert sha(Path(__file__).with_name('research_mp_original_data_u5.py'))==r['tool_sha256']
 assert r['source_pins']==PINS and not r['production_changes']and not r['annotation_names_or_comments_used']
 for path,pin in PINS.items():assert sha(path)==pin
 old,new=Image(OLD),Image(NEW);decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
 assert fnv(old.raw)==r['old_full_file_fnv1a64']and fnv(new.raw)==r['u5_full_file_fnv1a64']
 rows=memory_count=0
 for table in r['tables']:
  a,b=int(table['old_va'],16),int(table['u5_va'],16);n=table['bytes']
  raw=old.read(a,n);assert raw==new.read(b,n)and fnv(raw)==table['fnv1a64']
  assert hashlib.sha256(raw).hexdigest()==table['sha256']
  assert new.offset(b,n)==int(table['u5_raw_offset'],16)
  pos=new.raw.find(raw);assert pos==new.offset(b,n)and new.raw.find(raw,pos+1)==-1
  payload=out/table['table_payload']['file'];assert payload.read_bytes()==raw and sha(payload)==table['table_payload']['sha256']
  consumer=table['consumer'];sites={}
  for label,image,listing in zip(('old','u5'),(old,new),consumer['listings']):
   path=out/listing['file'];assert sha(path)==listing['sha256']
   lines=path.read_text().splitlines();assert len(lines)==listing['rows']
   for line in lines:
    address,hex_,text=line.split(' ',2);address=int(address,16);bytes_=bytes.fromhex(hex_)
    assert image.read(address,len(bytes_))==bytes_
    decoded=list(decoder.disasm(bytes_,address));assert len(decoded)==1 and decoded[0].size==len(bytes_)
    i=decoded[0];assert i.mnemonic+' '+i.op_str==text
    sites[(label,address)]=i;rows+=1
  for use in consumer['actual_original_table_use_instructions']:
   for label,prefix in (('old','old'),('u5','u5')):
    i=sites[(label,int(use[prefix+'_site'],16))]
    assert i.bytes.hex()==use[prefix+'_bytes']and i.mnemonic+' '+i.op_str==use[prefix+'_instruction']
    pointer=int(consumer[prefix+'_reference'],16)
    assert any(o.mem.disp==pointer if o.type==X86_OP_MEM else o.imm==pointer for o in i.operands if o.type in (X86_OP_IMM,X86_OP_MEM))
  assert consumer['all_paired_memory_bytes_equal']
  for q in consumer['paired_image_memory_operands']:
   left=old.read(int(q['old_pointer'],16),q['width']);right=new.read(int(q['u5_pointer'],16),q['width'])
   assert left==right and left.hex()==q['old_hex']and right.hex()==q['u5_hex']and q['bytes_equal'];memory_count+=1
  if table['name']=='fade_curve5':
   assert int(consumer['u5_reference'],16)+5*1024==b
   assert any(i.mnemonic=='movsx'and i.op_str=='esi, ax'for (label,_),i in sites.items()if label=='u5')
   assert any(i.mnemonic=='shl'and i.op_str=='esi, 0xa'for (label,_),i in sites.items()if label=='u5')
 # The independently frozen combat report establishes original constant reads.
 assert sha(COMBAT)==COMBAT_PIN
 combat=json.loads(COMBAT.read_text());excluded={'004e7920','004e791c','004eb114'};seen=set();constants=[]
 for q in combat['paired_image_memory_operands']:
  if q['disc_address']in excluded or q['disc_address']in seen:continue
  seen.add(q['disc_address']);assert q['size']==4 and q['bytes_equal']
  for image,site,address,hex_key in ((old,'disc_site','disc_address','disc_hex'),(new,'later_site','later_address','later_hex')):
   pointer=int(q[address],16);raw=image.read(pointer,4);assert raw.hex()==q[hex_key]
   i=next(decoder.disasm(image.read(int(q[site],16),15),int(q[site],16)))
   assert any(o.type==X86_OP_MEM and not o.mem.base and o.mem.disp==pointer and o.size==4 for o in i.operands)
  constants.append(dict(index=len(constants),old_va=q['disc_address'],u5_va=q['later_address'],raw4_hex=q['disc_hex'],old_first_read=q['disc_site'],u5_first_read=q['later_site']))
 assert len(constants)==19
 payload=b''.join(bytes.fromhex(q['raw4_hex'])for q in constants)
 balance=dict(schema=1,combat_report_sha256=COMBAT_PIN,order='unique source memory constants in first-encounter paired_memory_operands order',excluded_old_globals=sorted(excluded),
  source_pins=PINS,constants=constants,packed76_sha256=hashlib.sha256(payload).hexdigest(),packed76_fnv1a64=fnv(payload),raw76_hex=payload.hex(),base_has_patch=False)
 save(out/'balance-constants.json',encoded(balance))
 result=dict(schema=1,auditor_sha256=sha(__file__),report_sha256=sha(out/'report.json'),original_instruction_rows_verified=rows,
  paired_image_memory_operand_values_verified=memory_count,unique_complete_table_matches=3,source_balance_words_verified=19,balance_constants_sha256=sha(out/'balance-constants.json'),
  independent_raw_section_mapping=True,dynamic_consumer_execution=False)
 save(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
