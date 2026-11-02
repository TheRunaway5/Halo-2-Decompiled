#!/usr/bin/env python3
"""Actual2F81B0 scalar destructor over qualified native2F7E90/pool lifetime."""
import inspect
import validate_native_agent_cleanup_children_tu15 as child
original_cases=child.cases
def cases():
 result=[]
 for c in original_cases():
  if c['entry']==0x2f8090:
   c=dict(c);c['entry']=0x2f81b0;c['name']=c['name'].replace('2f8090','2f81b0');result.append(c)
 return result
def main():
 child.OWNERS=(0x2f81b0,0x2f81e0,0x2f7e90,0x2f7a10,0x149da0,0x149e00,0x233250,0x233420);child.cases=cases
 source=inspect.getsource(child.main)
 source=source.replace("'tools/validate_native_agent_cleanup_children_tu15.py'","'tools/validate_native_agent_cleanup_children_tu15.py','include/halo2/native_tu15_reversed_list_agent_destructor.hpp','tools/validate_native_reversed_list_agent_destructor_tu15.py'")
 needle="(project/'driver.cpp').write_text(driver)";assert source.count(needle)==1
 changes=[('#include "halo2/native_tu15_agent_cleanup_children.hpp"','#include "halo2/native_tu15_reversed_list_agent_destructor.hpp"'),('check(cases>20,','check(cases>0,'),('else result=native_tu15_destroy_contact_agent(memory,entry==0x2f8090u?NativeTu15AgentDestructor::owner_2f8090:entry==0x3051a0u?NativeTu15AgentDestructor::owner_3051a0:NativeTu15AgentDestructor::owner_2fc540,{receiver},c.at("arguments")[0],fp,services,observe).identity;','else result=native_tu15_delete_reversed_list_contact_agent(memory,{receiver},c.at("arguments")[0],fp,services,observe).identity;'),('c.at("entry")==0x2f8090u','c.at("entry")==0x2f81b0u')]
 extra=''.join('assert driver.count('+repr(a)+')>=1;driver=driver.replace('+repr(a)+','+repr(b)+');'for a,b in changes)
 source=source.replace(needle,extra+needle)
 begin=source.index("mutants=[('point_stride'");end=source.index('\n  for name,before,after in mutants:',begin)
 source=source[:begin]+"mutants=[('disposing_bit','if(disposing&1u)','if(disposing&2u)'),('borrowed_storage','if(!(capacity&0x80000000u))','if(false)'),('delete_class','agent.identity,size,0x1au','agent.identity,size,0x12u')]"+source[end:]
 source=source.replace("scope='Declared valid objects", "scope='Actual2F81B0/2F81E0 +same sole native2F7E90 storage/class1A release; declared valid objects")
 environment=dict(child.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
