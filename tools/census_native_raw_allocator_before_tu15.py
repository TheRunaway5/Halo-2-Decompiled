#!/usr/bin/env python3
"""Read actual09 raw heap virtual owners; no allocation/state changes."""
import argparse,os
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,save
from validate_native_world_island_merge_stage_tu15 import ARCHIVE,PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=BeforeInput(ARCHIVE,PIN,scratch/'actual09');word=source.word;rows=[]
 for kind,address in [('fixed',0x486068),('primary',0x48606c),('secondary',0x486070)]:
  receiver=word(address);row=dict(kind=kind,pointer_address=address,receiver=receiver)
  if receiver:
   table=word(receiver);row.update(table=table,raw_allocate_owner=word(table),raw_release_owner=word(table+4),array_allocate_owner=word(table+0x10),array_release_owner=word(table+0x14),secondary_start=word(receiver+0x18),secondary_size=word(receiver+0x1c))
  rows.append(row)
 save(a.output/'report.json',dict(source.receipt,mode=word(0x486064),heaps=rows,source_bytes_changed=0,owned_scratch=str(scratch),retained_large_artifacts=[]));print(rows);return 0
if __name__=='__main__':raise SystemExit(main())
