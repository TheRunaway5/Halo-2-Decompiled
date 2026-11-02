#!/usr/bin/env python3
"""Physical full1E0A60 response with native material no-conveyor inputs.

Positive contacts/normals/materials/hit branches are genuine source execution
on declared valid DATA. Positive shader functions remain a required child.
"""
import inspect,struct
import validate_native_empty_part_contact_tu15 as parent
from oracle_smg_firing_1_1 import ROOT

def cases(quick=False):
 rows=parent.cases();u,d=parent.parent.prop.base.UI,parent.parent.prop.base.DATA
 for contacts in (1,2,3,4,5,8,9):
  for mode in (0,1,2,3):
   count=4;words=[(0x188,u+0x3000),(0x18c,contacts),(0x104,(1<<18)if mode&1 else 0),
    (0x440,u+0x700),(0x730,d),(0x734,count),(0x738,0x80000000|count),(0x402c,u+0x4200),(0x5010,u+0x5200)]
   for n in range(16):words.extend([(0x1000+n*8,0x2003 if n==1 else 0x3000+n),(0x1004+n*8,0xcafe0000+n)])
   for n in range(4):words.extend([(0x4200+n*8+4,n<<16),(0x5200+n*20+8,0xffff0000)])
   for n in range(contacts):
    row=0x3000+n*72;special=(n==0)if mode<2 else n<4
    normal=(0,0,0)if mode==3 else(0x3f800000 if special or mode==0 else 0xbf800000,0,0)
    words.extend([(row+0xc,n%4),(row+0x28,normal[0]),(row+0x2c,normal[1]),(row+0x30,normal[2]),(row+0x44,(0xff<<8)|(0x20 if special else 0))])
   rows.append(dict(name=f'positive_part_{contacts}_{mode}',entry=0x1e0a60,receiver=u+0x700,
    arguments=[u+0x400,0xffffffff,u+0x100],header=u+0x730,user_words=words,poison_ecx=0xa5a5a5a5,private_seed=0x5a))
 # Existing no-response profiles never inspect scenario/material globals.
 return rows

def main():
 base=parent.parent.prop.base;old_inputs=base.reference_inputs
 def inputs(image):
  banks,proof=old_inputs(image);banks=dict(banks)
  banks[0x4ecc10]=struct.pack('<I',base.UI+0x4000);banks[0x4ecc18]=struct.pack('<I',base.UI+0x5000)
  for p in (0x46a0ec,0x46a0d0,0x46a130,0x46a104,0x474d10):banks[p]=image.read(p,4)
  pointer=int.from_bytes(banks[0x474d10],'little');banks[pointer]=image.read(pointer,12)
  return banks,dict(proof,declared_materials='Source-valid no-conveyor metadata, ordinary scenario/material DATA; no shader results supplied')
 base.reference_inputs=inputs
 source=inspect.getsource(parent.main)
 needle=' environment=dict(parent.__dict__,cases=cases);exec(source,environment)'
 assert source.count(needle)==1
 extra=''' source=source.replace('base.OWNERS=(0x1e0a60,0x1d7660,0x31c450,','base.OWNERS=(0x1e0a60,0x184c50,0x355d0,0x1d7660,0x31c450,')
 source=source.replace("'src/physics/native_tu15_empty_part_contact.cpp')", "'src/physics/native_tu15_empty_part_contact.cpp','include/halo2/native_tu15_part_contact_response.hpp','src/physics/native_tu15_part_contact_response.cpp','include/halo2/native_tu15_surface_conveyor.hpp','src/physics/native_tu15_surface_conveyor.cpp')")
 source=source.replace('src/physics/native_tu15_empty_part_contact.cpp tests/native_tu15_array_growth_tests.cpp', 'src/physics/native_tu15_empty_part_contact.cpp src/physics/native_tu15_part_contact_response.cpp src/physics/native_tu15_surface_conveyor.cpp tests/native_tu15_array_growth_tests.cpp')
 source=source.replace('#include "halo2/native_tu15_empty_part_contact.hpp"', '#include "halo2/native_tu15_part_contact_response.hpp"')
 source=source.replace('native_tu15_release_empty_part_contact(memory,args[0],std::bit_cast<std::int32_t>(args[1]),args[2],PRIVATE,fp,growth,observe)', 'native_tu15_update_part_contact_response(memory,args[0],std::bit_cast<std::int32_t>(args[1]),args[2],{PRIVATE+0x100u,PRIVATE,{PRIVATE+0x200u,PRIVATE+0x204u}},fp,0,growth,{},observe)')
'''
 source=source.replace(needle,extra+needle)
 environment=dict(parent.__dict__,cases=cases);exec(source,environment)
 try:return environment['main']()
 finally:
  import sys,json
  from pathlib import Path
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   d=json.loads(p.read_text());d['scope']='Declared valid full1E0A60 response composed with native1D7660/property/array/pool and native184C50 material no-conveyor route. Positive normals/materials/signedpart/hits/propertybranches, allpersistentDATA+orderedoperandpreimages/stores+FP. Positive shader evaluator branches and rawsourceprivateledger unclaimed.';d['positive_shader_functions_qualified']=False;p.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
