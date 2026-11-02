#!/usr/bin/env python3
"""Actual1470 world partition flags and edge-owner readiness, without writes."""
import argparse,os
from pathlib import Path
from build_native_tu15_contact_point_before_reference import PointBeforeInput,save
ROOT=Path(__file__).resolve().parents[1]
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-merge-point-original-v1470-20261101/iteration-00-native-world-merge-manager-factory-cleanup-vehicle-camera/required-native-contact-point-before/actual-native-before.tar.zst'
PIN='e234b565b08de52ca00ffd8027b0778dbcc1984659b211f265363e8635c9171f'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=PointBeforeInput(ARCHIVE,PIN,scratch/'before');word=source.word;manager=source.data['input']['manager'];collector=manager+16;world=word(manager+8);rows=[]
 for label,bank_offset,count_offset in[('active',8,12),('inactive',20,24)]:
  bank=word(world+bank_offset);count=word(world+count_offset);assert count<4096
  for index in range(count):
   island=word(bank+index*4);constraints=[]
   for n in range(word(island+0x5c)):
    item=word(word(island+0x58)+n*4);constraints.append(dict(identity=item,table=word(item),info_owner=word(word(item)+0x28),refs=word(item+6,2)))
   rows.append(dict(category=label,index=index,island=island,flag=word(island+0x28,1),active=word(island+0x29,1),bodies=word(island+0x40),actions=word(island+0x50),constraints=constraints,contacts=word(island+0x6c)))
 result=dict(source.receipt,world=world,collector=collector,detach_receiver=word(collector+0x14),collector_refs=word(collector+6,2),point_count=word(collector+0x94),islands=rows,source_input_fields_overridden=False,owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[]);save(a.output/'report.json',result);print(rows);return 0
if __name__=='__main__':raise SystemExit(main())
