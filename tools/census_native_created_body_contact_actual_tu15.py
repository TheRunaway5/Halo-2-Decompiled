#!/usr/bin/env python3
"""Readonly actual281710 required scalar topology, no Source/kernel execution."""
import argparse,json,os
from pathlib import Path
from native_created_body_contact_before_input_tu15 import NativeCreatedBodyContactBeforeInput,sha
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(scratch),original_execution=False,native_admission=False,retained_large_artifacts=[])
 try:
  data=NativeCreatedBodyContactBeforeInput(a.archive,a.sha256,scratch/'before');w=data.word;rows=[]
  for side,key in(('first','first_payload'),('second','second_payload')):
   payload=data.event_fields[key];node=payload;chain=[]
   for n in range(128):
    parent=w(node+0xc);chain.append(dict(node=node,parent=parent))
    if not parent:break
    node=parent
   else:raise ValueError('Actual payload parent chain exceeds bound')
   row=dict(side=side,payload=payload,chain=chain,root=node,root_kind=w(node+0x18),body=w(node+0x20));rows.append(row)
   shape=w(payload);table=w(shape);row['shape']=dict(identity=shape,virtual14=w(table+0x14),material_record=w(shape+8))
   if row['shape']['virtual14']==0xa9520:
    child=w(shape+0x30);row['shape']['sweep_child']=dict(identity=child,virtual14=w(w(child)+0x14),material_record=w(child+8))
   if row['root_kind']!=1 or not row['body']:continue
   body=row['body'];motion=w(body+0x3c);row.update(material_byte=w(body+0x48,1),material_scalars=[w(body+0x4c),w(body+0x50)],motion=dict(identity=motion,virtual18=w(w(motion)+0x18),inverse_mass_bits=w(motion+0x2c)))
   count=w(body+0x34);signed=count if count<2**31 else count-2**32;row['property_count']=signed;row['properties']={}
   if signed>0:
    if signed>4096:raise ValueError('Actual property count exceeds census bound')
    props=w(body+0x30)
    for n in range(signed):
     prop=w(props+n*8)
     if prop in(0x2001,0x2002):row['properties'].setdefault(hex(prop),w(props+n*8+4))
   datum=row['properties'].get('0x2001',0);components=w(0x52be5c);entries=w(components+0x44);component=entries+(datum&0xffff)*160
   row['component']=dict(datum=datum,identity=component,flags=w(component+4),object_datum=w(component+8),contact_rows=w(component+0x88),contact_count=w(component+0x8c),contact_capacity=w(component+0x90))
  manager=data.event_fields['manager'];constraint=manager+0x10;current=w(constraint);expected=w(0x48d1b0);receiver=w(manager+0x24)
  details=dict(identity=manager,type=current,expected_type=expected,first_body=w(manager+0x1c),update_receiver=receiver)
  if receiver:details['update_virtual8']=w(w(receiver)+8)
  r.update(passed=True,before=data.receipt,input=data.input,event_fields=data.event_fields,body_prerequisites=rows,manager=details,policy_cells={hex(p):w(p)for p in(0x55d700,0x4500b4,0x46a180,0x48cf90)},nested_words_outside_owned_arena=True,scope='Only authenticated actual typedBefore event/fragments and backing scalars outside source-owned moving arena. No child result, Source execution, FP computation or accepted native admission.',tool_sha256=sha(__file__))
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
