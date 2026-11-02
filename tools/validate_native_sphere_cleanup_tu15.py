#!/usr/bin/env python3
"""Actual2F9960 sphere cleanup plus inherited BV/single-point regressions."""
import validate_native_single_point_cleanup_tu15 as single
bv=single.bv

def cases():
 address=bv.parent.base.UI
 template=next(r for r in bv.parent.cases()if r['name']=='wrapped_leaf')
 rows=[]
 def add(name,point=13,**more):
  rows.append(dict(template,name='sphere_'+name,entry=0x2f9960,
                   user_words=[*template['user_words'],(0x10c,0xa5a50000|point)],**more))
 for point in(0,1,0x7fff,0x8000,0xfffe,0xffff):add(hex(point),point)
 add('none_without_manager',0xffff);rows[-1]['user_words'].append((0x108,0))
 for name,offset,size,write in(('point',0x10c,2,False),('manager',0x108,4,False),
     ('manager_table',0x600,4,False),('remove_owner',0x848,4,False),
     ('reset',0x10c,2,True),('self_table',0x100,4,False)):
  add('fault_'+name,fault=dict(address=address+offset,bytes=size,occurrence=1,write=write))
 for name,pc in(('current_destructor_after_callback',0x2e4d60),('current_destructor_after_reset',0x2f9978)):
  add(name,mutations=[dict(instruction=pc,address=address+0x100,written=(address+0x900).to_bytes(4,'little').hex())])
 return rows

def transform(source):
 source=single.transform(source)
 old='else if(entry==0x2fbb10u)native_tu15_cleanup_single_point_contact_agent(memory,{receiver},fp,services);'
 assert source.count(old)==1
 source=source.replace(old,old+'\n        else if(entry==0x2f9960u)native_tu15_cleanup_sphere_contact_agent(memory,{receiver},fp,services,observe);')
 old='  for name,before,after in mutants:';assert source.count(old)==1
 source=source.replace(old,'''  mutants += [
   ('sphere_reset_missing','m.word(0x2f9978u,agent.identity+12u,0xffffu,2);',''),
   ('sphere_reset_width','m.word(0x2f9978u,agent.identity+12u,0xffffu,2);','m.word(0x2f9978u,agent.identity+12u,0xffffu,4);'),
   ('sphere_point_sign','std::uint16_t(cached),fp);','std::uint16_t(cached+1u),fp);')]
  for name,before,after in mutants:''')
 return source

if __name__=='__main__':
 raise SystemExit(bv.main(additional_cases=[*single.cases(),*cases()],additional_owners=(0x2fbb10,0x2f9960),
     additional_pins=('tools/validate_native_single_point_cleanup_tu15.py','tools/validate_native_sphere_cleanup_tu15.py'),source_transform=transform))
