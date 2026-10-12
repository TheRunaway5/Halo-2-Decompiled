#!/usr/bin/env python3
"""Complete original sound cache-constructor correspondence and operands."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as authority
from oracle_mp_title_update_display import PINS

OWNERS=(0x21a6e0,0x13e8e0,0x16d070,0x16d0d0,0x16d220)
BOUNDARIES=(0x21aaf0,0x21aab0,0x11e150)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for image in(authority.OLD,authority.NEW):assert authority.sha(image)==PINS[str(image)]
 authority.OWNERS=OWNERS;authority.BOUNDARIES=BOUNDARIES;authority.run(a.output)
 report=json.loads((a.output/'report.json').read_text());mapping={q['old']:q['u5']for q in report['owners']}
 images=[authority.Xbe(p)for p in(authority.OLD,authority.NEW)]
 listings=[(a.output/'instructions/0021a6e0-old.asm').read_text().splitlines(),
           (a.output/'instructions/0021a6e0-u5.asm').read_text().splitlines()]
 def operand(site):
  index=next(i for i,q in enumerate(listings[0])if q.startswith(site+' '))
  return [int(rows[index].split()[-1],16)for rows in listings]
 callbacks={}
 for name,site,target in(('delete_callback','0021a736','0021aaf0'),('busy_callback','0021a731','0021aab0')):
  addresses=operand(site);assert addresses==[int(target,16),int(mapping[target],16)]
  callbacks[name]=dict(old_identity=f'{addresses[0]:08x}',u5_identity=f'{addresses[1]:08x}',
      old_instruction_hex=images[0].read(int(site,16),5).hex(),
      u5_instruction_hex=images[1].read(int(listings[1][next(i for i,q in enumerate(listings[0])if q.startswith(site+' '))].split()[0],16),5).hex(),
      complete_target_body_verified=True)
 names={}
 for name,site in(('entry_pool','0021a6fb'),('page_allocator','0021a749')):
  addresses=operand(site);raw=[im.read(va,32).split(b'\0',1)[0]for im,va in zip(images,addresses)]
  assert raw[0]==raw[1] and len(raw[0])<32
  names[name]=dict(old_va=f'{addresses[0]:08x}',u5_va=f'{addresses[1]:08x}',ascii=raw[0].decode('ascii'),
      strncpy32_output_hex=raw[0].ljust(32,b'\0').hex())
 packet=dict(schema='source-sound-cache-startup-immutable-providers-v1',callbacks=callbacks,names=names,
     image_sha256=report['image_sha256'],
     scope='Only original immutable constructor names and callback identities. Allocator interfaces, raw storage, physical-arena index/limits and actual protection are required runtime/platform owners; no ready/admission/map/device result supplied.')
 authority.save(a.output/'providers.json',authority.encoded(packet))
 authority.save(a.output/'invocation.json',authority.encoded(dict(tool_sha256=authority.sha(__file__),authority_tool_sha256=authority.sha(authority.__file__),image_pins=PINS,instruction_patches=[],successful_platform_results=[])))
 print(json.dumps(dict(providers_sha256=authority.sha(a.output/'providers.json'))))

if __name__=='__main__':main()
