#!/usr/bin/env python3
"""Authenticate body-contact update virtual tables and semantic dependencies."""
import argparse,hashlib,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from build_native_tu15_constraint_before_reference import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');table=0x41f124;raw=image.read(table,0x20);manager={f'{offset:x}':struct.unpack_from('<I',raw,offset)[0]for offset in(0,0x10,0x14,0x18)}
 motion_tables=[]
 for section in image.sections:
  data=image.section_bytes(section)
  for offset in range(0,len(data)-4,4):
   owner=struct.unpack_from('<I',data,offset)[0]
   if owner not in(0x31dd70,0x31edf0,0xa9520,0xab4b0,0x2e8cf0):continue
   at=section.va+offset-0x18
   try:values=image.read(at,0x68)
   except Exception:continue
   # This is a source table-candidate inventory, not a live table identity.
   if all(0x15000<=struct.unpack_from('<I',values,n)[0]<0x400000 for n in(0x18,0x58,0x60)):
    motion_tables.append(dict(table=at,kind_owner=owner,point_velocity_owner=struct.unpack_from('<I',values,0x58)[0],point_impulse_owner=struct.unpack_from('<I',values,0x60)[0],hex=values.hex(),sha256=hashlib.sha256(values).hexdigest()))
 owners=[0x1d77d0,0x1d7b60,0x186c30,0x1e1170,0x315c90,*[manager[k]for k in('14','18')]]
 owners+=list(dict.fromkeys(row[k]for row in motion_tables for k in('point_velocity_owner','point_impulse_owner')))
 rows=[]
 for owner in dict.fromkeys(owners):
  body=complete_body(code,owner);lines=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call':calls.append(dict(site=pc,target=i.operands[0].imm if i.operands[0].type==2 else i.op_str))
  listing=''.join(lines);(a.output/f'{owner:08x}.txt').write_text(listing);rows.append(dict(owner=owner,sites=len(body.code),calls=calls,listing_sha256=hashlib.sha256(listing.encode()).hexdigest()))
 save(a.output/'report.json',dict(read_only=True,original_sha256=NEW_PIN,manager_table=dict(identity=table,hex=raw.hex(),sha256=hashlib.sha256(raw).hexdigest(),owners=manager),source_motion_table_candidates=motion_tables,live_motion_table_identity_claimed=False,owners=rows,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],allocated_bytes_before_cleanup=0,retained_large_artifacts=[]));print(dict(manager=manager,motion_tables=motion_tables,owners=rows));return 0
if __name__=='__main__':raise SystemExit(main())
