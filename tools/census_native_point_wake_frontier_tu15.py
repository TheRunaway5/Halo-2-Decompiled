#!/usr/bin/env python3
"""Read genuine1470 inactive receiver wake dependencies without changing physics."""
import argparse,os
from pathlib import Path
from build_native_tu15_contact_point_before_reference import PointBeforeInput,save
from census_native_point_regroup_before_tu15 import ARCHIVE,PIN
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=PointBeforeInput(ARCHIVE,PIN,scratch/'before');w=source.word;manager=source.data['input']['manager'];collector=manager+16;world=w(manager+8);island=w(collector+0x14)
 def listeners(receiver,offset,slot):
  count=w(receiver+offset+4);assert count<=128;data=w(receiver+offset);rows=[]
  for n in range(count):
   item=w(data+4*n)
   rows.append(dict(index=n,listener=item,table=w(item)if item else 0,owner=w(w(item)+slot)if item else 0))
  return dict(data=data,count=count,slot=slot,rows=rows)
 bodies=[]
 for n in range(w(island+0x40)):
  body=w(w(island+0x3c)+4*n);bodies.append(dict(body=body,world=w(body+8),island=w(body+0x44),listeners=listeners(body,0x70,8)))
 contacts=[]
 for n in range(w(island+0x6c)):
  row=w(island+0x68)+16*n;agent=w(row+8);first=w(row);second=w(row+4);contacts.append(dict(record=row,first=first,second=second,first_body=w(first+16),second_body=w(second+16),agent=agent,agent_table=w(agent)if agent else 0))
 result=dict(source.receipt,passed=True,world=world,world_regroup_requested=w(world+0x23c,1),island=island,active=w(island+0x29,1),partition_flag=w(island+0x28,1),index=w(island+0x24),world_activation_listeners=listeners(world,0x78,4),bodies=bodies,contacts=contacts,source_input_fields_overridden=False,captured_wake_call=False,owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[])
 save(a.output/'report.json',result);print({k:result[k]for k in('world_regroup_requested','active','partition_flag','world_activation_listeners','bodies','contacts')});return 0
if __name__=='__main__':raise SystemExit(main())
