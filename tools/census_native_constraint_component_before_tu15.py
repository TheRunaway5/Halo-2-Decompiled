#!/usr/bin/env python3
"""Read actual eligible frame-level component DATA without invoking physics."""
import argparse,os
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,save
def census(source):
 m=source;captured=m.data['component'];datum=captured['datum'];pool=m.word(0x52be5c);rows=m.word(pool+0x44);stride=m.word(pool+0x24);index=datum&0xffff
 assert stride==160 and index<m.word(pool+0x38)
 assert m.word(m.word(pool+0x48)+(index>>5)*4)&(1<<(index&31))
 component=rows+index*stride;assert component==captured['row']and m.word(component,2)==datum>>16
 flags=m.word(component+4);count=m.word(component+0x80);bank=m.word(component+0x7c);assert flags&0x800 and count>0
 object_datum=m.word(component+8);objects=m.word(0x4ecbd0);object_header=m.word(objects+0x44)+(object_datum&0xffff)*12;object_data=m.word(object_header+8)
 object_definition=m.word(object_data);tags=m.word(0x4f0414);definition=m.word(tags+(object_definition&0xffff)*16+8)
 model_datum=m.word(definition+0x38);model=m.word(tags+(model_datum&0xffff)*16+8);physics_datum=m.word(model+0x24);physics=m.word(tags+(physics_datum&0xffff)*16+8)
 records=[];offsets=[0xb0,0xe8,0xb8,0xf8,0xf0,0x100];strides=[0x78,0x84,0x94,0x7c,0x78,0x84]
 for n in range(min(count,64)):
  row=bank+n*12;type_=m.word(row,2);ordinal=m.word(row+2,2);handle=m.word(row+4);constraint=m.word(row+8)
  record=dict(row=row,raw=m.read(row,12).hex(),type=type_,ordinal=ordinal,current_handle=handle,physical_constraint=constraint)
  if type_<6:
   block=physics+offsets[type_];extent=m.word(block);authored=m.word(block+4)+ordinal*strides[type_];record.update(block_count=extent,authored_definition=authored)
   if ordinal<extent:
    edge=int.from_bytes(m.read(authored+0x70,2),'little',signed=True);record['edge_index']=edge
    if edge!=-1:
     physical_edge=m.word(physics+0x34)+edge*24;record.update(edge=physical_edge,edge_raw=m.read(physical_edge,24).hex())
  records.append(record)
 bodies=[];body_bank=m.word(component+0x70);body_count=m.word(component+0x74)
 for n in range(min(body_count,64)):
  body=m.word(body_bank+n*0x60+0x40);island=m.word(body+0x44);properties=m.word(body+0x30);extent=m.word(body+0x34)
  bodies.append(dict(slot=n,body=body,island=island,active=bool(m.word(island+0x29,1)),properties=[{'key':m.word(properties+k*8),'value':m.word(properties+k*8+4)}for k in range(min(extent,32))]))
 return dict(source.receipt,read_only=True,current_MCC_input=True,frame_level_component_data=True,captured22D460_caller_ABI=False,
  component=component,component_datum=datum,flags=flags,object_datum=object_datum,object_header=object_header,
  object_data=object_data,object_header_salt=m.word(object_header,2),object_type_signed_byte=int.from_bytes(m.read(object_data+0xaa,1),'little',signed=True),
  object_definition_datum=object_definition,object_definition=definition,model_datum=model_datum,physics_model_datum=physics_datum,
  physics_model=physics,authored_count=count,records=records,bodies=bodies,
  original_formal_entry=dict(owner=0x22d460,stack_arguments=[datum],return_bytes=4),
  constraint_creation_executed=False,ragdoll_identity_inferred=False,manufactured_rows=False)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 source=BeforeInput(a.capture,a.archive_sha256,Path(os.environ['HALO2_TEST_SCRATCH'])/'before');report=census(source)
 report.update(owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]);save(a.output,report)
 print({k:report[k]for k in['component_datum','object_datum','object_type_signed_byte','object_definition_datum','authored_count','records']})
 return 0
if __name__=='__main__':raise SystemExit(main())
