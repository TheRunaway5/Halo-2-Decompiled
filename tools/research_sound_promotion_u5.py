#!/usr/bin/env python3
"""Pair complete original promotion, source clock and map/reset producers."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as a
OWNERS=(0x128610,0x128140,0x126de0,0x18a4f0,0x127c10,0x12d600)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
 a.OWNERS=OWNERS;a.BOUNDARIES=();a.run(args.output)
 report=json.loads((args.output/'report.json').read_text());wanted={0x46513c,0x4650e0,0x465030};words=[]
 for owner in report['owners']:
  for m in owner['memory']:
   if int(m['old_va'],16) in wanted and m['old_va']not in {w['old_va']for w in words}:words.append(m)
 assert len(words)==3
 a.save(args.output/'providers.json',a.encoded(dict(schema='source-sound-promotion-immutable-v1',words=words,order=['milliseconds_per_second_bits','seconds_per_millisecond_bits','one_bits'],mutable_clock_rows_options_not_constants=True,instruction_patches=[])))
if __name__=='__main__':main()
