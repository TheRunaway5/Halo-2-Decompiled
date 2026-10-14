#!/usr/bin/env python3
"""Complete charged constructor/update/preparation/postpass/birth correspondence.

Relocated absolute pointer operands are followed to their literal12-byte vectors.
Other callees retain independent ownership; this is static body evidence, not
complete original world execution or native weapon parity.
"""
import argparse,json,struct
from pathlib import Path
import research_weapon_effect_u5 as correspondence
from weapon_reference_versions import images
OWNERS=(0x101bb0,0x1021d0,0x107160,0x108300,0x101a60,0x101990,0x1ed030,0xfc2e0)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 correspondence.OWNERS=OWNERS;correspondence.BOUNDARIES=();correspondence.run(a.output)
 path=a.output/'report.json';report=json.loads(path.read_text());bank={name:image for name,image,_ in images()}
 differences=[m for owner in report['owners'] for m in owner['memory'] if m['old_hex']!=m['new_hex']]
 assert len(differences)==3
 vectors=[]
 for m in differences:
  assert m['old_site'] in ('001084ca','00108634','000fc343') and m['bytes']==4
  old=struct.unpack('<I',bytes.fromhex(m['old_hex']))[0];new=struct.unpack('<I',bytes.fromhex(m['new_hex']))[0]
  before=bank['TU1.1'].read(old,12);after=bank['TU1.5'].read(new,12);assert before==after
  vectors.append(dict(old_site=m['old_site'],old_pointer=f'{old:08x}',new_pointer=f'{new:08x}',vector_hex=before.hex()))
 report.update(scope=__doc__,total_instruction_rows=sum(owner['rows'] for owner in report['owners']),
  correspondence_tool_sha256=report['tool_sha256'],tool_sha256=correspondence.sha(__file__),relocated_indirected_vectors=vectors,
  external_callees=sorted({c['old_target'] for owner in report['owners'] for c in owner['calls'] if not c['callee_proven_here']}))
 path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print('owners',len(report['owners']),'rows',report['total_instruction_rows'],'relocated equal vectors',len(vectors),flush=True)
if __name__=='__main__':main()
