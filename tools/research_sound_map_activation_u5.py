#!/usr/bin/env python3
"""Complete original sound map-activation CPU/platform request correspondence."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as authority
from oracle_mp_title_update_display import PINS

OWNERS=(0x126de0,0x127540,0x2217e0,0x2204c0,0x192ca0)
BOUNDARIES=(0x2230e0,0x2b4e30,0x2b5260,0x337a40)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for image in(authority.OLD,authority.NEW):assert authority.sha(image)==PINS[str(image)]
 authority.OWNERS=OWNERS;authority.BOUNDARIES=BOUNDARIES;authority.run(a.output)
 r=json.loads((a.output/'report.json').read_text());rows={q['old']:q for q in r['owners']}
 constants={}
 for name,va in(('one_bits','00465030'),('ambient_transition_bits','004457bc')):
  q=next(q for q in rows['00126de0']['memory']if q['old_va']==va);assert q['old_hex']==q['new_hex']
  constants[name]=dict(old_va=q['old_va'],u5_va=q['new_va'],raw4_hex=q['old_hex'])
 listings=[(a.output/'instructions'/f'002204c0-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
 index=next(i for i,q in enumerate(listings[0])if q.startswith('0022050c '));addresses=[int(rows[index].split()[-1],16)for rows in listings]
 packet=dict(schema='source-sound-map-activation-providers-v1',constants=constants,image_sha256=r['image_sha256'],
   runtime_environment_packet=dict(old_address=f'{addresses[0]:08x}',u5_address=f'{addresses[1]:08x}',
      runtime_owner='2230E0 and its authentic update counterpart; fresh image contents are not active map defaults',
      copied_into_constants=False),
   boundaries=['Current matg/tag table/system/driver/environment/options objects remain actual retained runtime owner inputs.',
    'Driver+1 is the original saved-audio-setting predicate, not host stereo capability. Plugin methods and four effect writes require genuine separately named native services; no callback success supplied.',
    'Separate54-class fade map reset is not inside126DE0. No cache reset or direct+7A shortcut.'])
 authority.save(a.output/'providers.json',authority.encoded(packet))
 authority.save(a.output/'invocation.json',authority.encoded(dict(tool_sha256=authority.sha(__file__),authority_tool_sha256=authority.sha(authority.__file__),image_pins=PINS,instruction_patches=[],platform_returns_supplied=[])))
 print(json.dumps(dict(providers_sha256=authority.sha(a.output/'providers.json'))))
if __name__=='__main__':main()
