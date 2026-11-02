#!/usr/bin/env python3
"""Read actual09 regroup operands/virtual owners without changing partition flags."""
import argparse,os
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,save
from validate_native_world_island_merge_stage_tu15 import ARCHIVE,PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=BeforeInput(ARCHIVE,PIN,scratch/'actual09');word=source.word;world=word(0x52be48);islands=[]
 def owner(object,offset):return word(word(object)+offset)
 for category,array_offset,count_offset in[('active',8,12),('inactive',20,24)]:
  count=word(world+count_offset);assert count<4096;bank=word(world+array_offset)
  for n in range(count):
   island=word(bank+n*4);row=dict(category=category,ordinal=n,island=island,partition_flag=word(island+0x28,1),active=word(island+0x29,1),bodies=[],actions=[],constraints=[],contacts=[])
   for index in range(word(island+0x40)):
    body=word(word(island+0x3c)+index*4);motion=word(body+0x3c);row['bodies'].append(dict(identity=body,index=word(body+0x58),body40=word(body+0x40,1),motion=motion,partition_weight_owner=owner(motion,0x14)))
   for index in range(word(island+0x50)):
    action=word(word(island+0x4c)+index*4);row['actions'].append(dict(identity=action,enumerate_bodies_owner=owner(action,0x18)))
   for index in range(word(island+0x5c)):
    constraint=word(word(island+0x58)+index*4);first=word(constraint+0xc);second=word(constraint+0x10);row['constraints'].append(dict(identity=constraint,first_body=first,second_body=second,first_body40=word(first+0x40,1),second_body40=word(second+0x40,1),info_owner=owner(constraint,0x28)))
   for index in range(word(island+0x6c)):
    record=word(island+0x68)+index*16;first=word(record);second=word(record+4);item=dict(record=record,first_payload=first,second_payload=second)
    try:
     first_body=word(first+0x10);second_body=word(second+0x10);item.update(first_body=first_body,second_body=second_body,first_body40=word(first_body+0x40,1),second_body40=word(second_body+0x40,1),first_body_index=word(first_body+0x58),second_body_index=word(second_body+0x58))
    except(AssertionError,ValueError)as e:item['exact321EB0_operand_unavailable']=str(e)or type(e).__name__
    row['contacts'].append(item)
   islands.append(row)
 allocator=word(0x48c938);slots={hex(offset):owner(allocator,offset)for offset in(0,4,0x10,0x14,0x24,0x28)}
 save(a.output/'report.json',dict(source.receipt,world=world,allocator=allocator,allocator_virtual_owners=slots,islands=islands,partition_positive=sum(bool(r['partition_flag'])for r in islands),changed_source_bytes=0,declared_partition_flags_supplied=False,original_caller_ABI_captured=False,owned_scratch=str(scratch),retained_large_artifacts=[]));print(dict(islands=len(islands),partition_positive=sum(bool(r['partition_flag'])for r in islands),allocator_owners=slots));return 0
if __name__=='__main__':raise SystemExit(main())
