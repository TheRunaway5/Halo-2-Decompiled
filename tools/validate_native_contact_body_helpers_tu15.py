#!/usr/bin/env python3
"""Physical TU1.5 point/property getters and motion point velocity/impulse."""
import inspect,struct
import validate_native_agent_cleanup_children_tu15 as child
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def cases():
 u=child.base.UI;rows=[]
 for owner in(0x323020,0x323040):
  for ordinal,contact in enumerate((0,1,5,0x200,0x7fff,0xabcd0005)):
   for index in(0,1,127,255):
    words=[(0x130,u+0x4000),(0x144,u+0x8000),(0x1a0,u+0x9000),(0x4000+(contact&0xffff),index)]
    rows.append(dict(name=f'getter_{owner:x}_{contact:x}_{index}',entry=owner,receiver=u+0x100,arguments=[contact],user_words=words))
 for owner in(0x323a10,0x324f80):
  for pattern in range(5):
   for alias in range(6):
    words=[]
    def vec(at,values):words.extend((at+n*4,bits(value)if isinstance(value,float)else value)for n,value in enumerate(values))
    words.append((0x12c,bits(.25)));vec(0x140,[1.,2.,3.,4.]);vec(0x150,[.5,-.75,1.25,2.]);vec(0x170,[4.,-2.,8.,.5])
    vec(0x180,[1.,0.,0.,0.]);vec(0x190,[0.,1.,0.,0.]);vec(0x1a0,[0.,0.,1.,0.]);vec(0x1e0,[2.,3.,4.,5.]);vec(0x400,[2.,3.,4.,5.]);vec(0x500,[1.,-3.,2.,0.])
    if pattern==1:vec(0x150,[0.,0.,0.,0.]);words.append((0x12c,0))
    elif pattern==2:vec(0x500,[0x7fc12345,0x7f812345,0xff800000,0x00000001])
    elif pattern==3:vec(0x400,[-0.,1.e-20,1.e10,-1.e-30]);vec(0x140,[1.e30,-1.e20,1.e-30,-0.])
    elif pattern==4:vec(0x500,[-2.5,4.75,-8.125,16.25]);words.append((0x12c,bits(-.5)))
    if owner==0x323a10:
     output=[u+0x600,u+0x140,u+0x150,u+0x170,u+0x400,u+0x180][alias];args=[u+0x400,output]
    else:
     impulse=[u+0x500,u+0x140,u+0x500,u+0x150,u+0x400,u+0x180][alias]
     point=[u+0x400,u+0x400,u+0x140,u+0x170,u+0x400,u+0x1a0][alias];args=[impulse,point]
    rows.append(dict(name=f'motion_{owner:x}_pattern{pattern}_alias{alias}',entry=owner,receiver=u+0x100,arguments=args,user_words=words))
 return rows
def main():
 child.OWNERS=(0x323020,0x323040,0x323a10,0x324f80);child.cases=cases;source=inspect.getsource(child.main)
 source=source.replace("'tools/validate_native_agent_cleanup_children_tu15.py'","'tools/validate_native_agent_cleanup_children_tu15.py','include/halo2/native_tu15_contact_body_update.hpp','src/physics/native_tu15_contact_body_update.cpp','src/physics/native_tu15_activation_services.cpp','tools/validate_native_contact_body_helpers_tu15.py'")
 needle="'native_tu15_contact_unwind_internal.hpp'):shutil.copyfile";assert source.count(needle)==1;source=source.replace(needle,"'native_tu15_contact_unwind_internal.hpp','native_tu15_contact_body_update.cpp','native_tu15_activation_services.cpp'):shutil.copyfile")
 needle="(project/'driver.cpp').write_text(driver)";assert source.count(needle)==1
 start='      try{const auto entry='
 finish='      }catch(const Fault&)'
 replacement='''      try{const auto entry=c.at("entry").get<std::uint32_t>(),receiver=c.at("receiver").get<std::uint32_t>();
        const auto first=c.at("arguments")[0].get<std::uint32_t>();
        if(entry==0x323020u)result=native_tu15_contact_manager_point(memory,receiver,std::uint16_t(first));
        else if(entry==0x323040u)result=native_tu15_contact_manager_properties(memory,receiver,std::uint16_t(first));
        else if(entry==0x323a10u)native_tu15_contact_motion_point_velocity(memory,receiver,first,c.at("arguments")[1],fp,observe);
        else if(entry==0x324f80u)native_tu15_apply_contact_motion_point_impulse(memory,receiver,first,c.at("arguments")[1],fp,observe);
'''
 extra="driver='#include \\\"halo2/native_tu15_contact_body_update.hpp\\\"\\n'+driver;".replace('\\"','"')
 extra+='begin=driver.index('+repr(start)+');end=driver.index('+repr(finish)+',begin);driver=driver[:begin]+'+repr(replacement)+'+driver[end:];'
 extra+='driver=driver.replace(\'      check(calls==\',\'      if(c.at("entry")==0x323020u||c.at("entry")==0x323040u)check(result==c.at("return_word"),name+" native getter result differs");\\n      check(calls==\');'
 source=source.replace(needle,extra+needle)
 source=source.replace('native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)','native_tu15_array_allocator.cpp native_tu15_array_growth.cpp native_tu15_contact_body_update.cpp native_tu15_activation_services.cpp)')
 source=source.replace("target=project/'native_tu15_agent_cleanup_children.cpp'","target=project/'native_tu15_contact_body_update.cpp'")
 begin=source.index("mutants=[('point_stride'");end=source.index('\n  for name,before,after in mutants:',begin)
 source=source[:begin]+"mutants=[('getter_stride','return data+(index<<5u)','return data+(index<<4u)'),('point_velocity_add','output,add_vector(cross,linear)','output,sub_vector(cross,linear)'),('impulse_reread','current_impulse=m.vector(impulse)','current_impulse=first_impulse')]"+source[end:]
 source=source.replace("scope='Declared valid objects", "scope='Actual323020/323040/323A10/324F80 pure native source algorithms; declared before-input vectors/aliases/FP; no whole1D77D0/nativewake claim; declared valid objects")
 environment=dict(child.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
