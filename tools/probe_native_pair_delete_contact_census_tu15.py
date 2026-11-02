#!/usr/bin/env python3
"""Read-only genuine09 island/contact census for remaining320260 branches."""
import argparse,os
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,save
ROOT=Path(__file__).resolve().parents[1]
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-phantom-owner-original-v1466-20261101/iteration-09-native-census-readonly-guard-alias-range6-frag-vehicle-camera/required-native-constraint-component-before/actual-native-before.tar.zst'
PIN='0fd26e268228c50ea01c07a5fea7b68a1e06c515eac2ec90562f303c5873caac'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  source=BeforeInput(ARCHIVE,PIN,scratch/'actual09');body=source.data['component']['active_body'];world=source.word(body+8)
  rows=[]
  for field,label in((8,'active'),(0x14,'inactive')):
   data,count=source.word(world+field),source.word(world+field+4);assert count<=4096
   for index in range(count):
    island=source.word(data+index*4);records,count_records=source.word(island+0x68),source.word(island+0x6c);assert count_records<=4096
    row=dict(list=label,index=index,island=island,active=source.word(island+0x29,1),record_count=count_records,record_data=records,
      body_count=source.word(island+0x40),pending_count=source.word(island+0x78),records=[])
    for n in range(count_records):
     r=records+n*16;first,second,agent=source.word(r),source.word(r+4),source.word(r+8)
     item=dict(index=n,address=r,first=first,second=second,agent=agent)
     try:
      first_body,second_body=source.word(first-0x10+0x20),source.word(second-0x10+0x20)
      item.update(first_body=first_body,second_body=second_body,first_island=source.word(first_body+0x44),second_island=source.word(second_body+0x44))
      if agent:item.update(agent_table=source.word(agent),cleanup=source.word(source.word(agent)+0x14),manager=source.word(agent+8))
     except (AssertionError,FileNotFoundError)as e:item['unavailable_operand']=str(e)or type(e).__name__
     row['records'].append(item)
    rows.append(row)
  report.update(passed=True,source=source.receipt,world=world,rows=rows,
   multiple_record_islands=sum(r['record_count']>1 for r in rows),physics_mutated=False,guest_ABI_or_results_supplied=False,
   scope='Read-only frame09 DATA census only; no public320260 caller, outcome, contact registration or physics modification.')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
