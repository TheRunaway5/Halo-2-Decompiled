#!/usr/bin/env python3
"""Read real World activation/merge operands from typed frame BEFORE banks."""
import argparse,os
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 m=BeforeInput(a.capture,a.archive_sha256,Path(os.environ['HALO2_TEST_SCRATCH'])/'before');world=m.word(0x52be48)
 def island(p):return dict(identity=p,index=m.word(p+0x24),active=m.word(p+0x29,1),partition_flag=m.word(p+0x28,1),bodies=m.word(p+0x40),actions=m.word(p+0x50),constraints=m.word(p+0x5c),agents=m.word(p+0x6c),destructor=m.word(m.word(p)))
 cohorts={}
 for name,offset in[('active',8),('inactive',0x14)]:
  bank=m.word(world+offset);count=m.word(world+offset+4);assert count<=4096
  cohorts[name]=[island(m.word(bank+n*4))for n in range(count)]
 count=m.word(world+0x24);bank=m.word(world+0x20);assert count<=4096;pairs=[]
 for n in range(count):
  first=m.word(bank+n*8);second=m.word(bank+n*8+4);pairs.append(dict(first_body=first,second_body=second,first=island(m.word(first+0x44)),second=island(m.word(second+0x44))))
 c=m.data['component'];bodies=[];bank=m.word(c['row']+0x70)
 for n in range(m.word(c['row']+0x74)):
  body=m.word(bank+n*0x60+0x40);listeners=m.word(body+0x70);count=m.word(body+0x74);assert count<=4096
  bodies.append(dict(body=body,slot=n,island=island(m.word(body+0x44)),listeners=[{'identity':m.word(listeners+k*4),'activation_owner':m.word(m.word(m.word(listeners+k*4))+8)if m.word(listeners+k*4)else None}for k in range(count)]))
 listeners=m.word(world+0x78);count=m.word(world+0x7c);assert count<=4096
 result=dict(m.receipt,read_only=True,captured_caller_ABI=False,manufactured_rows=False,world=world,
  islands=cohorts,pending_merge_pairs=pairs,authored_component_bodies=bodies,
  world_activation_listeners=[{'identity':m.word(listeners+k*4),'owner':m.word(m.word(m.word(listeners+k*4))+4)if m.word(listeners+k*4)else None}for k in range(count)],
  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
 save(a.output,result);print({'world':world,'active':len(cohorts['active']),'inactive':len(cohorts['inactive']),'pending_merges':len(pairs),'component_bodies':bodies,'world_listeners':result['world_activation_listeners']});return 0
if __name__=='__main__':raise SystemExit(main())
