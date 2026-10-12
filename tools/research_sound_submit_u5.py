#!/usr/bin/env python3
"""Complete raw sound submit/controller/metadata owners; no admission inputs."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as a
OWNERS=(0x21f220,0x21fe40,0x18eac0,0x18eba0,0x21b5b0,0x21fd70,0x21fe00,0x16d2c0,0x16d4c0,0x21bb40,0x21bba0,0x21ba70,0x16d680,0x21bd90,0x21bfa0)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();a.OWNERS=OWNERS;a.BOUNDARIES=();a.run(args.output)
 r=json.loads((args.output/'report.json').read_text());row=next(q for q in r['owners']if q['old']=='0021fd70');lines=[(args.output/'instructions'/f'0021fd70-{tag}.asm').read_text().splitlines()for tag in('old','u5')];i=next(i for i,l in enumerate(lines[0])if l.startswith('0021fdbe '));ptrs=[int(x[i].split()[-1],16)for x in lines]
 images=[a.Xbe(x)for x in(a.OLD,a.NEW)];callbacks=[dict(image=t,address=f'{at:08x}',raw32_hex=x.read(at,32).hex())for t,x,at in zip(('old','u5'),images,ptrs)]
 a.save(args.output/'providers.json',a.encoded(dict(schema='source-sound-submit-provider-contract-v1',effect_callback_tables=callbacks,mutable_globals_not_default=True,raw_request_bytes=160,raw_location_copy_bytes=68,source_platform_returns=[],image_sha256=r['image_sha256'])))
 a.save(args.output/'invocation.json',a.encoded(dict(tool_sha256=a.sha(__file__),authority_tool_sha256=a.sha(a.__file__),instruction_patches=[],source_platform_returns=[])))
if __name__=='__main__':main()
