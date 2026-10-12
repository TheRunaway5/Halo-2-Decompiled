#!/usr/bin/env python3
"""Independent complete generic-body mapping and literal sentinel operand packet."""
import sys
from pathlib import Path
import research_unit_pose_requests_u5 as pairing
from oracle_mp_title_update_display import ROOT, OLD, NEW, Xbe, sha, save, encoded
if __name__=='__main__':
 pairing.ENTRIES=(0xb8a30,)
 pairing.main()
 out=Path(sys.argv[sys.argv.index('--output-dir')+1])if'--output-dir'in sys.argv else pairing.OUT
 packet=dict(schema=1,tool_sha256=sha(__file__),source_images=[dict(path=str(p),sha256=sha(p))for p in(OLD,NEW)],
  operands=[dict(image=label,address=f'{address:08x}',bytes=2,raw_hex=Xbe(path).read(address,2).hex(),
   references=[f'{pc:08x}'for pc in sites])for label,path,address,sites in(
    ('title-update-1.1',OLD,0x46fb64,(0xb8b33,0xb8c68)),
    ('u5English',NEW,0x474c24,(0xb9c73,0xb9da8)))],
  scope='Literal selected-image low byte and full word at the two proved B8A30 operand sites. No mutable world, body, pool or readiness fields are included.')
 # The paired instruction report owns the relocated operands, independently
 # of the addresses named above. Verify both references against that report.
 import json
 report=json.loads((out/'report.json').read_text());row=report['owners'][0]
 assert row['old_rows']==614 and row['full_body_matches']==['000b9b70']
 refs=[r for r in row['image_memory_operands']if r['old_va']=='0046fb64']
 assert [(r['old_site'],r['u5_site'],r['u5_va'],r['bytes'])for r in refs]==[
  ('000b8b33','000b9c73','00474c24',1),('000b8c68','000b9da8','00474c24',2)]
 assert all(r['old_hex']==r['u5_hex']for r in refs)
 save(out/'operand-packet.json',encoded(packet));print(sha(out/'operand-packet.json'))
