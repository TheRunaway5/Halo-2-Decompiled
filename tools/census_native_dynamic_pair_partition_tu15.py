#!/usr/bin/env python3
"""Source-authentic04 dynamic pair graph membership without any DATA writes."""
import argparse,os
from pathlib import Path
from build_native_tu15_contact_time_before_reference import ContactTimeBeforeInput,save
ROOT=Path(__file__).resolve().parents[1]
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-04-native-sphere-feature-processor-simplex-rockets-vehicle-camera/required-native-dynamic-pair-before/actual-native-before.tar.zst'
PIN='9403d05a5d2ea0fd27ad22b0f734b1b2d1f0c007e673b325d64df5ef3a47577d'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=ContactTimeBeforeInput(ARCHIVE,PIN,scratch/'before');w=source.word;island=source.data['input']['island'];rows=[]
 for n in range(w(island+0x6c)):
  record=w(island+0x68)+16*n;first=w(record);second=w(record+4);a_body=w(first+16);b_body=w(second+16);rows.append(dict(record=record,first=first,second=second,first_body=a_body,second_body=b_body,first_static=w(a_body+0x40,1),second_static=w(b_body+0x40,1),first_index=w(a_body+0x58),second_index=w(b_body+0x58),first_island=w(a_body+0x44),second_island=w(b_body+0x44)))
 result=dict(source.receipt,passed=True,source_input=source.data['input'],island=island,bodies=w(island+0x40),actions=w(island+0x50),constraints=w(island+0x5c),partition_flag=w(island+0x28,1),contacts=rows,source_physics_fields_overridden=False,owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[]);save(a.output/'report.json',result);print({k:result[k]for k in('island','bodies','actions','constraints','partition_flag','contacts')});return 0
if __name__=='__main__':raise SystemExit(main())
