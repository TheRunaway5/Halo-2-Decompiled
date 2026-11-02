#!/usr/bin/env python3
"""Physical1E0A60 no-eligible-contact native body/property/growth closure.

The nonempty normal/material/conveyor branch remains refused and unclaimed.
Source-private dead output is ordinary independent native DATA, never a GP save.
"""
import inspect,struct
import validate_native_part_property_parent_tu15 as parent
from oracle_smg_firing_1_1 import ROOT

def cases(quick=False):
 u,d=parent.prop.base.UI,parent.prop.base.DATA;rows=[]
 for count in (0,1,4,8):
  for selected in sorted({-1,0,count//2,count-1}):
   if selected>=count:continue
   for poison,seed in ((0,0),(0xa5a5a5a5,0xa5),(0x5a5a5a5a,0x5a)):
    words=[(0x18c,0),(0x440,u+0x700),(0x730,d),(0x734,count),(0x738,0x80000000|count)]
    for n in range(16):words.extend([(0x1000+n*8,0x2003 if n==selected else 0x3000+n),(0x1004+n*8,0xcafe0000+n)])
    rows.append(dict(name=f'empty_part_{count}_{selected}_{seed}',entry=0x1e0a60,receiver=u+0x700,
        arguments=[u+0x400,0xffffffff,u+0x100],header=u+0x730,user_words=words,poison_ecx=poison,private_seed=seed))
 for contacts in (1,4,9):
  for part in (-128,-1,0,127):
   for poison,seed in ((0,0),(0xa5a5a5a5,0xa5),(0x5a5a5a5a,0x5a)):
    words=[(0x188,u+0x3000),(0x18c,contacts),(0x440,u+0x700),(0x730,d),(0x734,4),(0x738,0x80000004)]
    for n in range(contacts):
     matches=n%2==0;row=0x3000+n*72
     words.extend([(row+0x44,((part&255)<<8)if matches else((((part+1)&255)<<8)|0x20))])
    for n in range(16):words.extend([(0x1000+n*8,0x2003 if n==1 else 0x3000+n),(0x1004+n*8,0xcafe0000+n)])
    rows.append(dict(name=f'inactive_part_{contacts}_{part}_{seed}',entry=0x1e0a60,receiver=u+0x700,
        arguments=[u+0x400,part&0xffffffff,u+0x100],header=u+0x730,user_words=words,poison_ecx=poison,private_seed=seed))
 return rows

def main():
 original_inputs=parent.prop.base.reference_inputs
 def inputs(image):
  banks,proof=original_inputs(image);address=0x474d04;raw=image.read(address,4);pointer=int.from_bytes(raw,'little')
  banks=dict(banks);banks[address]=raw;banks[pointer]=image.read(pointer,12)
  proof=dict(proof,source_default_vector=dict(pointer_address=address,pointer=pointer,authenticated_bytes=image.read(pointer,12).hex()))
  return banks,proof
 parent.prop.base.reference_inputs=inputs
 source=inspect.getsource(parent.main)
 source=source.replace("source=source.replace('base.OWNERS=(0x31c450,','base.OWNERS=(0x1d7660,0x31c450,')", "source=source.replace('base.OWNERS=(0x31c450,','base.OWNERS=(0x1e0a60,0x1d7660,0x31c450,')")
 needle="    environment=dict(prop.__dict__,cases=cases);exec(source,environment)"
 assert source.count(needle)==1
 extra='''    source=source.replace("base.FILES=(*base.FILES,'tools/validate_native_part_property_parent_tu15.py')", "base.FILES=(*base.FILES,'tools/validate_native_part_property_parent_tu15.py','tools/validate_native_empty_part_contact_tu15.py','include/halo2/native_tu15_empty_part_contact.hpp','src/physics/native_tu15_empty_part_contact.cpp')")
    source=source.replace("src/physics/native_tu15_body_properties.cpp tests/native_tu15_array_growth_tests.cpp", "src/physics/native_tu15_body_properties.cpp src/physics/native_tu15_empty_part_contact.cpp tests/native_tu15_array_growth_tests.cpp")
    source=source.replace('#include "halo2/native_tu15_body_properties.hpp"', '#include "halo2/native_tu15_empty_part_contact.hpp"')
    source=source.replace('if(entry==0x1d7660u)native_tu15_replace_part_contact_property', 'if(entry==0x1e0a60u)native_tu15_release_empty_part_contact(memory,args[0],std::bit_cast<std::int32_t>(args[1]),args[2],PRIVATE,fp,growth,observe);else if(entry==0x1d7660u)native_tu15_replace_part_contact_property')
'''
 source=source.replace(needle,extra+needle)
 environment=dict(parent.__dict__,cases=cases);exec(source,environment)
 try:return environment['main']()
 finally:
  import sys,json
  from pathlib import Path
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   d=json.loads(p.read_text());d['scope']='Declared valid no-eligible response domain: source1E0A60 zero and nonempty matching-part/flag scans composed with native1D7660/31C450/31C5E0/nativearraygrowth. AllpersistentDATA+orderedoperands/preimages/stores+FP. Positive normal/material/conveyor and rawprivatecontrolledger remain unclaimed.';d['positive_part_contact_qualified']=False;p.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
