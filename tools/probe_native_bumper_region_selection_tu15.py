#!/usr/bin/env python3
"""Read only: actual TU1.5 body-region/permutation admission in two Warthog phases."""
import argparse,hashlib,json,os
from pathlib import Path
from probe_native_joint_activation_condition_tu15 import Before
ROOT=Path(__file__).resolve().parents[1]
def snapshot(archive,pin):
 b=Before(archive,pin);w=b.word
 def signed(p,n):return int.from_bytes(b.read(p,n),'little',signed=True)
 tags=w(0x4f0414);datum=3802202162;objects=w(w(0x4ecbd0)+0x44);obj=w(objects+(datum&0xffff)*12+8);definition=w(tags+(w(obj)&0xffff)*16+8);modeldatum=w(definition+0x38);model=w(tags+(modeldatum&0xffff)*16+8)
 count=w(model+0x70);assert count<=128;regions=w(model+0x74);stored=obj+signed(obj+0x11a,2);stored_count=int(signed(obj+0x118,2)/10);out=[]
 for i in range(count):
  row=regions+i*16;perms=w(row+8);assert perms<=256;data=w(row+12);selected=signed(stored+i,1)if i<stored_count else None
  out.append(dict(index=i,name_string_id=w(row),physics_region=signed(row+5,1),stored_permutation=selected,permutation_rows=[dict(index=k,name_string_id=w(data+k*8),flags=w(data+k*8+4,2),physics_permutation=signed(data+k*8+6,1))for k in range(perms)]))
 phmod=w(model+0x24);phmo=w(tags+(phmod&0xffff)*16+8);rigid_count=w(phmo+0x38);rigid=w(phmo+0x3c);assert rigid_count<=128;admitted=[]
 for i in range(rigid_count):
  row=rigid+i*0x90;region=signed(row+2,2);perm=signed(row+4,2);match=next((r for r in out if r['physics_region']==region),None);selected=None if match is None else match['stored_permutation'];chosen=None if selected is None or selected<0 or selected>=len(match['permutation_rows']) else match['permutation_rows'][selected]
  admitted.append(dict(rigid=i,node=signed(row,2),physics_region=region,physics_permutation=perm,selected_model_region=None if match is None else match['index'],selected_model_permutation=selected,selected_physics_permutation=None if chosen is None else chosen['physics_permutation'],source184920_region_admitted=chosen is not None and chosen['physics_permutation']==perm))
 damage_info=w(model+0x64)if signed(model+0x60,4)>0 else 0;damage_rows=[];flags_e0=w(obj+0xe0,2);flags_e2=w(obj+0xe2,2)
 if damage_info:
  damage_count=w(damage_info+0xe0);damage_data=w(damage_info+0xe4);assert damage_count<=16
  for i in range(damage_count):damage_rows.append(dict(index=i,name_string_id=w(damage_data+i*20),match_first=signed(damage_data+i*20+16,2),match_second=signed(damage_data+i*20+18,2),object_e0_bit=bool(flags_e0&(1<<i)),object_e2_bit=bool(flags_e2&(1<<i))))
 edges=[];count=w(phmo+0x30);data=w(phmo+0x34);assert count<=128
 for i in range(count):
  row=data+i*24;conditions=[];n=w(row+8);p=w(row+12);assert n<=64
  for k in range(n):
   first=signed(p+k*12,2);second=signed(p+k*12+2,2);match=next((d for d in damage_rows if d['match_first']==first and d['match_second']==second),None);conditions.append(dict(first=first,second=second,matching_damage_index=None if match is None else match['index'],dda10_e0=False if match is None else match['object_e0_bit'],dda90_e2=False if match is None else match['object_e2_bit']))
  edges.append(dict(index=i,node_a=signed(row+4,2),node_b=signed(row+6,2),damage_conditions=conditions,all_conditions_unbroken=all(not c['dda10_e0']and not c['dda90_e2']for c in conditions)))
 return dict(boundary=b.capture['boundary'],archive_sha256=pin,object=datum,object_address=obj,model_datum=modeldatum,model=model,stored_count=stored_count,model_regions=out,rigid_admission=admitted,damage_info=damage_info,object_damage_e0=flags_e0,object_damage_e2=flags_e2,damage_gate_rows=damage_rows,authored_node_edges=edges,raw_banks_written=False)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--joint-archive',type=Path,required=True);p.add_argument('--joint-sha256',required=True);p.add_argument('--process-archive',type=Path,required=True);p.add_argument('--process-sha256',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH');a.output.mkdir(parents=True,exist_ok=False)
 rows=[snapshot(a.joint_archive,a.joint_sha256),snapshot(a.process_archive,a.process_sha256)];files=['analysis/physics/native-bumper-body-gate-source-20261102-second/00184920.txt','analysis/physics/native-bumper-body-gate-source-20261102-second/00184b90.txt','analysis/physics/native-bumper-body-gate-source-20261102-second/001ddbf0.txt'];sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();report=dict(passed=True,read_only=True,snapshots=rows,source_pins={f:sha(ROOT/f)for f in files},source_scope='TU1.5 184920/184B90 literal signed model-region/permutation selection, no legacy behavior reused',limits=['Different actual phases and gameplay times, not a continuous change trace','Damage transition owner/threshold and sufficient ordinary input remain unproved','No Source DATA, pose, damage or flags modified'],retained_large_artifacts=[]);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(rows,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
