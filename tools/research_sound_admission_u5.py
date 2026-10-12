#!/usr/bin/env python3
"""Pair complete source sound admission and direct CPU query dependencies."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as a
OWNERS=(0x128380,0x128320,0x126760,0x129450,0x129570,0x223eb0,0x12c370,0x126710,0x126930,0x21aef0,0x21ae80,0x21b700,0x21b900,0x128760,0x223920,0x14cc00)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);q=p.parse_args();a.OWNERS=OWNERS;a.BOUNDARIES=(0x24be30,);a.run(q.output)
 report=json.loads((q.output/'report.json').read_text());rows={r['old']:r for r in report['owners']}
 wanted=(0x452044,0x4650c0,0x465028,0x465030,0x45120c,0x465048,0x46508c,0x451214,0x465078,0x4650e0,0x4651cc)
 words=[]
 for addr in wanted:
  uses=[m for r in report['owners']for m in r['memory']if int(m['old_va'],16)==addr];assert uses and all(m['old_hex']==m['new_hex']for m in uses);words.append(dict(old_va=f'{addr:08x}',u5_va=uses[0]['new_va'],raw_hex=uses[0]['old_hex'],uses=uses))
 a.save(q.output/'providers.json',a.encoded(dict(schema='sound-admission-immutable-v1',words=words,mutable_system_rng_listener_codec_fields_excluded=True)))
if __name__=='__main__':main()
