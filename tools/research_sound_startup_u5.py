#!/usr/bin/env python3
"""Complete CPU sound-startup correspondence; no platform admission supplied."""
import argparse,json,struct
from pathlib import Path
import research_animation_selection_u5 as authority
from oracle_mp_title_update_display import PINS

OWNERS=(0x126b20,0x11e340,0x16d070,0x16d0d0,0x16d220,0x1656a0,
        0x165700,0x21bc50,0x21c1d0,0x21f5a0,0x21feb0,0x12c710)
BOUNDARIES=(0x21a6e0,0x2205c0,0x128380,0x128320,0x129450)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for image in(authority.OLD,authority.NEW):assert authority.sha(image)==PINS[str(image)]
 authority.OWNERS=OWNERS;authority.BOUNDARIES=BOUNDARIES;authority.run(a.output)
 report=json.loads((a.output/'report.json').read_text());rows={r['old']:r for r in report['owners']}
 images=[authority.Xbe(q)for q in(authority.OLD,authority.NEW)]
 r=next(q for q in rows['00126b20']['memory']if q['old_va']=='004457bc')
 assert r['old_hex']==r['new_hex']
 files=[a.output/'instructions/00126b20-old.asm',a.output/'instructions/00126b20-u5.asm']
 lines=[q.read_text().splitlines()for q in files]
 at=next(i for i,q in enumerate(lines[0])if q.startswith('00126c8b '))
 addresses=[int(q[at].split()[-1],16)for q in lines]
 raw=[im.read(v,16).hex()for im,v in zip(images,addresses)];assert raw[0]==raw[1]
 packet=dict(schema='source-sound-startup-immutable-providers-v1',
  ambient_transition_bits=dict(old_va=r['old_va'],u5_va=r['new_va'],raw4_hex=r['old_hex']),
  voice_policy=dict(old_va=f'{addresses[0]:08x}',u5_va=f'{addresses[1]:08x}',raw16_hex=raw[0],
       signed_short_fields=list(struct.unpack('<8h',bytes.fromhex(raw[0])))),
  image_sha256=report['image_sha256'],
  scope='Only direct immutable startup operands. Arena/root allocator identity, cache, CRT/local/controller RNG, raw host clocks, map activation and real platform success remain required owning inputs; no guest driver success or sound availability is supplied.')
 authority.save(a.output/'providers.json',authority.encoded(packet))
 authority.save(a.output/'invocation.json',authority.encoded(dict(tool_sha256=authority.sha(__file__),authority_tool_sha256=authority.sha(authority.__file__),image_pins=PINS,declared_successful_platform_returns=[],instruction_patches=[])))
 print(json.dumps(dict(providers_sha256=authority.sha(a.output/'providers.json'))))

if __name__=='__main__':main()
