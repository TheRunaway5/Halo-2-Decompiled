#!/usr/bin/env python3
"""Compose native323220/3232B0/3231B0/existing31F660 on real04 heap DATA."""
import inspect,os
import validate_native_activation_collector_actual_tu15 as collector

def main():
 assert os.environ.get('HALO2_TEST_SCRATCH')
 source=inspect.getsource(collector.main)
 needle="child.OWNERS=(0x31f660,";assert source.count(needle)==1;source=source.replace(needle,"child.OWNERS=(0x323220,0x3232b0,0x3231b0,0x31f660,")
 begin=source.index(' child.cases=lambda:');end=source.index('\n source=inspect.getsource(child.main)',begin);source=source[:begin]+''' child.cases=lambda:[dict(name=name,entry=entry,receiver=0x10082380,arguments=args,user_words=[])for name,entry,args in [('actual04_manager_base',0x3231b0,[]),('actual04_manager_scalar_0',0x3232b0,[0]),('actual04_manager_scalar_1',0x3232b0,[1]),('actual04_manager_scalar_3',0x3232b0,[3]),('actual04_manager_reference',0x323220,[])]]
'''+source[end:]
 needle=" env=dict(child.__dict__);env['main']=None;";assert source.count(needle)==1
 extra='''
 source=source.replace("'tools/validate_native_activation_collector_actual_tu15.py'", "'tools/validate_native_activation_collector_actual_tu15.py','tools/validate_native_contact_manager_cleanup_actual_tu15.py','include/halo2/native_tu15_contact_manager_cleanup.hpp','src/physics/native_tu15_contact_manager_cleanup.cpp'")
 source=source.replace("'native_tu15_island_activation.cpp'):shutil.copyfile", "'native_tu15_island_activation.cpp','native_tu15_contact_manager_cleanup.cpp'):shutil.copyfile")
 source=source.replace('#include "halo2/native_tu15_activation_collector_cleanup.hpp"', '#include "halo2/native_tu15_contact_manager_cleanup.hpp"')
 source=source.replace('native_tu15_activation_contact_manager.cpp native_tu15_island_activation.cpp)', 'native_tu15_activation_contact_manager.cpp native_tu15_island_activation.cpp native_tu15_contact_manager_cleanup.cpp)')
 declaration='NativeTu15ContactManagerCleanupServices manager_services;manager_services.collector=collector_services;'
 dispatch='if(entry==0x323220u)native_tu15_dispose_contact_reference(memory,receiver,fp,services.agents);else if(entry==0x3231b0u||entry==0x3232b0u){'+declaration+'if(entry==0x3231b0u)native_tu15_destroy_contact_manager(memory,receiver,{},fp,manager_services,observe,&collector_stats);else result=native_tu15_delete_contact_manager(memory,receiver,c.at("arguments")[0],{},fp,manager_services,observe,&collector_stats);}else '
 source=source.replace('if(entry==0x31f660u)native_tu15_destroy_activation_collector',dispatch+'if(entry==0x31f660u)native_tu15_destroy_activation_collector')
 old='check(owner.identity==0x2e4d60u,"Unqualified native deleting callback");'
 replacement='if(owner.identity==0x3232b0u){'+declaration+'native_tu15_delete_contact_manager(memory,object,flag,{},fp,manager_services,observe,&collector_stats);return;}'+old
 assert source.count(old)==1;source=source.replace(old,replacement)
 source=source.replace('if(c.at("entry")==0x2f8090u||', 'if(c.at("entry")==0x3232b0u||c.at("entry")==0x2f8090u||')
 source=source.replace("target=project/'native_tu15_activation_contact_manager.cpp'", "target=project/'native_tu15_contact_manager_cleanup.cpp'")
 begin=source.index("mutants=[('collector_vtable'");end=source.index('\\n  for name,before,after in mutants:',begin)
 source=source[:begin]+"mutants=[('manager_vtable','0x41f124u','0x41f125u'),('manager_base','m.word(0x323209u,manager,0x45edf0u)','m.word(0x323209u,manager,0x45edf1u)'),('delete_class','manager,size,0x1du','manager,size,0x1au')]"+source[end:]
'''
 source=source.replace(needle,extra+needle)
 env=dict(collector.__dict__);env['main']=None;exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
