#!/usr/bin/env python3
"""Complete flush/queue/packet/static-buffer owners, not voice admission."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as authority
OWNERS=(0x2213a0,0x2b4470,0x2b4bf0,0x2b48a0,0x222b80,0x220bf0)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 authority.OWNERS=OWNERS;authority.BOUNDARIES=();authority.run(a.output)
 r=json.loads((a.output/'report.json').read_text());old=[q for q in r['owners']if q['old']=='00220bf0'][0]
 files=[(a.output/'instructions'/f'00220bf0-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
 at=next(i for i,q in enumerate(files[0])if q.startswith('00220cad '))
 queue={tag:f'{int(files[i][at].split()[-1],16):08x}'for i,tag in enumerate(('old','u5'))}
 packet=dict(schema='source-sound-driver-owners-provider-contract-v1',source_static_queue_identity=queue,mutable_owner_not_immutable_default=True,cache_pointer_operand=next(q for q in r['owners']if q['old']=='002b4bf0')['memory'],image_sha256=r['image_sha256'],source_platform_returns=[])
 authority.save(a.output/'providers.json',authority.encoded(packet));authority.save(a.output/'invocation.json',authority.encoded(dict(tool_sha256=authority.sha(__file__),authority_tool_sha256=authority.sha(authority.__file__),source_instruction_patches=[],source_platform_returns=[])))
if __name__=='__main__':main()
