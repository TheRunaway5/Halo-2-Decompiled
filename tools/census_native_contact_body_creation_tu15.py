#!/usr/bin/env python3
"""Read-only accepted-body prerequisites from protected1479 typed BEFORE."""
import argparse,json,os,struct
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
import validate_native_contact_candidates_bound_actual_tu15 as candidate
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();r=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],original_execution=False,native_admission=False)
 try:
  archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-2e5320-two-frag625-joint-vehicle-camera/required-native-contact-process-2e5320-before/actual-native-before.tar.zst'
  data=NativeContactProcessBeforeInput(archive,'cc2e90e06e3df1b013361f7fa13e14a9507aa059ececd3d9cf53a2a402e161b5',scratch/'before',0x2e5320)
  def word(p,n=4):return int.from_bytes(data.read(p,n),'little')
  inputs=data.input;rows=[]
  for side in ('first','second'):
   payload=inputs[side];root=payload
   for _ in range(128):
    parent=word(root+12)
    if not parent:break
    root=parent
   else:raise RuntimeError('Cyclic actual payload parent chain')
   row=dict(side=side,payload=payload,root=root,kind=word(root+24),body=word(root+32))
   if row['kind']==1 and row['body']:
    body=row['body'];motion=word(body+0x3c);table=word(motion);listeners=word(body+0x60);count=word(body+0x64)
    row.update(material_byte=word(body+0x48,1),motion=motion,motion_virtual18=word(table+0x18),inverse_mass_bits=word(motion+0x2c),listeners=[])
    if 0<count<64:
     for index in range(count):
      listener=word(listeners+index*4)
      if listener:row['listeners'].append(dict(receiver=listener,owner=word(word(listener)+8)))
   rows.append(row)
  cells={hex(p):word(p)for p in (0x55d700,0x4500b4,0x46a180,0x48d1b0,0x48cf90)}
  manager=word(inputs['agent']+8);constraint=manager+0x10
  current=word(constraint);details=dict(identity=manager,constraint_type=current,
   expected_constraint_type=cells['0x48d1b0'],first_body=word(manager+0x1c),
   virtual8_selected_by_before_type=current==cells['0x48d1b0'])
  # Mirror31F460's genuine type guard. An unselected null receiver is not a
  # source boundary and must not be dereferenced merely for this census.
  if details['virtual8_selected_by_before_type']:
   receiver=word(manager+0x24);details['update_receiver']=receiver
   if receiver:details['update_virtual8']=word(word(receiver)+8)
   else:details['unavailable_selected_receiver']=True
  r.update(passed=True,before=data.receipt,body_prerequisites=rows,source_runtime_cells=cells,
   contact_manager=details,
   scope='Authenticated BEFORE DATA only. A current body/manager or persisted contact is not a captured accepted281710 ABI/event. Prior genuine original-computed1479 listener census remainsflag1 only; no event flag or state was changed.',validator_sha256=sha(__file__))
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
