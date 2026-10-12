#!/usr/bin/env python3
"""Complete paired request preparation/pressure owners and original operands."""
import sys,json
from pathlib import Path
import research_unit_pose_requests_u5 as pairing
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
if __name__=='__main__':
 pairing.ENTRIES=(0xddac0,0x148360,0x148740,0x22ecb0,0x148510)
 pairing.main();out=Path(sys.argv[sys.argv.index('--output-dir')+1])
 r=json.loads((out/'report.json').read_text());assert all(len(q['full_body_matches'])==1 for q in r['owners'])
 addresses=(0x4650a4,0x451ff8,0x4650a0,0x46509c,0x465098,0x465094)
 rows=[]
 for address in addresses:
  refs=[v for q in r['owners']for v in q['image_memory_operands']if int(v['old_va'],16)==address]
  assert len(refs)==1 and refs[0]['bytes']==4
  rows.append(refs[0])
 packet=dict(schema=1,tool_sha256=sha(__file__),images=[dict(path=str(p),sha256=sha(p))for p in(OLD,NEW)],operands=rows,
  order=['main_usage_scale_bits','top_limit_bits','high_limit_bits','medium_limit_bits','low_limit_bits','statistic_usage_scale_bits'],
  scope='Six immutable original scalar words only. Actual heap records/counts, busy/collecting bytes, clock and lower reclaim/world owners remain required runtime storage/providers.')
 save(out/'operand-packet.json',encoded(packet));print(sha(out/'operand-packet.json'))
