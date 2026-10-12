#!/usr/bin/env python3
"""Literal Spartan phmo records, with no constructed body/constraint result."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_multiplayer import ROOT,Cache
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 path=ROOT/'assets/disc/maps/lockout.map';c=Cache(path)
 def record(p,n):return dict(source_address=f'{p+c.magic:08x}',file_offset=p,raw_hex=c.data[p:p+n].hex())
 def string_name(p):
  value=c.u(p);index=value&0xffff
  count,size,indices,strings=struct.unpack_from('<4I',c.data,356)
  assert index<count and c.u(indices+index*4)<size
  return dict(string_id=f'{value:08x}',text=c.text(strings+c.u(indices+index*4)))
 biped=0xe9f2086d;b=c.tags[biped]['offset'];model=c.u(b+0x38);m=c.tags[model]['offset'];physics=c.u(m+0x24);p=c.tags[physics]['offset']
 blocks={}
 for name,offset,stride in [('rigid',0x38,0x90),('node_edges',0x30,0x18),('nodes',0xc8,0xc),
    ('pills',0x58,0x50),('boxes',0x60,0x90),
    ('hinge',0xb0,0x78),('ragdoll',0xb8,0x94),('limited_hinge',0xe8,0x84),
    ('ball_socket',0xf0,0x78),('spring',0xf8,0x7c),('prismatic',0x100,0x84)]:
  blocks[name]=dict(offset=offset,stride=stride,records=[dict(index=i,**record(q,stride))for i,q in enumerate(c.block(p+offset,stride))])
 # Interpret only literal schema fields, retaining every raw byte and pointer.
 # These serialized frames are not constructed Havok body or solver output.
 for row in blocks['nodes']['records']:
  q=row['file_offset'];row['fields']=dict(name=string_name(q),flags=f'{struct.unpack_from("<H",c.data,q+4)[0]:04x}',
   parent=c.h(q+6),sibling=c.h(q+8),child=c.h(q+10))
 for row in blocks['rigid']['records']:
  q=row['file_offset'];row['fields']=dict(node=c.h(q),region=c.h(q+2),permutation=c.h(q+4),
   flags=f'{struct.unpack_from("<H",c.data,q+0x18)[0]:04x}',motion_selector=c.h(q+0x1a),
   shape_type=c.h(q+0x38),shape_index=c.h(q+0x3a),mass_word=f'{c.u(q+0x3c):08x}')
 for row in blocks['node_edges']['records']:
  q=row['file_offset'];row['fields']=dict(node_a=c.h(q+4),node_b=c.h(q+6),
   constraints=[dict(index=i,**record(a,12),type=c.h(a),constraint_index=c.h(a+2),
    flags=f'{c.u(a+4):08x}',friction_word=f'{c.u(a+8):08x}') for i,a in enumerate(c.block(q+8,12))])
 for key in ('hinge','ragdoll','limited_hinge','ball_socket','spring','prismatic'):
  for row in blocks[key]['records']:
   q=row['file_offset'];row['fields']=dict(name=string_name(q),node_a=c.h(q+4),node_b=c.h(q+6),edge=c.h(q+0x70),
    authored_frame_a_words=[f'{c.u(q+a):08x}' for a in range(8,0x3c,4)],
    authored_frame_b_words=[f'{c.u(q+a):08x}' for a in range(0x3c,0x70,4)])
 schema=ROOT/'tools/reference/assembly/src/Assembly/Plugins/Halo2/phmo.xml'
 report=dict(schema=2,tool_sha256=sha(Path(__file__)),navigation_schema_sha256=sha(schema),cache_sha256=sha(path),biped=c.reference(biped),model=c.reference(model),physics=c.reference(physics),
  biped_root=record(b,0x314),model_root=record(m,0xfc),physics_root=record(p,0x110),blocks=blocks,
  boundaries=['Raw original metadata; labels/strides use Assembly schema navigation.',
   'Name strings are read from literal map header356/360/364/368 index/string storage.',
   'Decoded field labels are schema navigation, not proof of runtime constraint construction/admission.',
   'No source rigid body, solver/constraint, PVS, pose, flag, contact, gravity or timing result is supplied.'])
 raw=(json.dumps(report,indent=2,sort_keys=True)+'\n').encode();args.output.parent.mkdir(parents=True,exist_ok=True)
 if args.output.exists():assert args.output.read_bytes()==raw
 else:args.output.write_bytes(raw)
 print(json.dumps(dict(physics=report['physics'],counts={k:len(v['records'])for k,v in blocks.items()})))
if __name__=='__main__':main()
