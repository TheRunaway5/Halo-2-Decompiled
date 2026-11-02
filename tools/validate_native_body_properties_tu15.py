#!/usr/bin/env python3
"""Strict physical31C450/31C5E0 body-property operands, aliasing and native growth.

The separate1D7660 semantic parent is implemented but not admitted as a raw
private-output full-return proof by this direct ordinary-DATA qualification.
"""
import inspect,struct
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT

def cases(quick=False):
    u,d=base.UI,base.DATA;rows=[]
    def add(name,owner,args,count=4,capacity=None,**extra):
        words=[(0x130,d),(0x134,count),(0x138,(count+1 if capacity is None else capacity)|0x80000000)]
        for n in range(16):words.extend([(0x1000+n*8,0x2000+n),(0x1004+n*8,0xabc00000+n*17),
            (0x1800+n*8,0x3000+n),(0x1804+n*8,0xdef00000+n*19)])
        rows.append(dict(name=name,entry=owner,receiver=u+0x100,arguments=args,
            header=u+0x130,user_words=words,**extra))
    for count in (0,1,2,4,9):
        for index in range(count+1):
            key=0x2000+index
            add(f'remove_count{count}_key{index}',0x31c450,[u+0x300,key],count)
            add(f'add_count{count}_key{index}',0x31c5e0,[key,0x76fedcba],count)
    for count in (0,1,2,4,8):add(f'genuine_growth_{count}',0x31c5e0,[0x88776655,0x12345678],count,count)
    for output in (u+0x134,d,d+4,d+8):add(f'remove_output_alias_{output:x}',0x31c450,[output,0x2000],4)
    for name,entry,args,address,nth,write in (
        ('remove_count',0x31c450,[u+0x300,0x2000],u+0x134,1,False),
        ('remove_current_count',0x31c450,[u+0x300,0x2000],u+0x134,2,False),
        ('remove_key_reload',0x31c450,[u+0x300,0x2000],d,2,False),
        ('remove_shift',0x31c450,[u+0x300,0x2000],d+8,1,False),
        ('remove_output',0x31c450,[u+0x300,0x2000],u+0x300,1,True),
        ('append_capacity',0x31c5e0,[0x88776655,0x12345678],u+0x138,1,False),
        ('append_current_count',0x31c5e0,[0x88776655,0x12345678],u+0x134,2,False),
        ('append_key',0x31c5e0,[0x88776655,0x12345678],d+32,1,True)):
        add('fault_'+name,entry,args,4,fault=dict(address=address,bytes=4,occurrence=nth,write=write))
    for name,site,address,value in (
        ('remove_current_backing',0x31c4a3,u+0x130,d+0x800),
        ('remove_current_count',0x31c4ba,u+0x134,1),
        ('remove_value_after_key_store',0x31c4ba,d+12,0x10203040),
        ('append_current_count_after_key',0x31c625,u+0x134,7)):
        owner=0x31c5e0 if name.startswith('append')else 0x31c450
        args=[0x88776655,0x12345678]if owner==0x31c5e0 else[u+0x300,0x2000]
        add('alias_'+name,owner,args,4,mutations=[dict(instruction=site,address=address,written=struct.pack('<I',value).hex())])
    return rows

def main():
    base.OWNERS=(0x31c450,0x31c5e0,0x2e3250,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420)
    base.cases=cases
    base.FILES=(*base.FILES,'include/halo2/native_tu15_body_properties.hpp','src/physics/native_tu15_body_properties.cpp','tools/validate_native_body_properties_tu15.py')
    source=inspect.getsource(base.main)
    source=source.replace('tests/native_tu15_array_growth_tests.cpp\'','src/physics/native_tu15_body_properties.cpp tests/native_tu15_array_growth_tests.cpp\'')
    needle="        (project/'CMakeLists.txt').write_text("
    assert source.count(needle)==1
    extra='''        driver=project/'tests/native_tu15_array_growth_tests.cpp';text=driver.read_text();text='#include "halo2/native_tu15_body_properties.hpp"\\n'+text
        marker='if(entry==0x2e3250u)'
        text=text.replace(marker,'if(entry==0x31c450u)native_tu15_remove_body_property(memory,receiver,args[1],args[0],observe);else if(entry==0x31c5e0u)native_tu15_add_body_property(memory,receiver,args[0],args[1],fp,growth,observe);else '+marker)
        driver.write_text(text)
'''
    source=source.replace(needle,extra+needle)
    begin=source.index('        changes=((');end=source.index('\n        for name,path,before,after in changes:',begin)
    source=source[:begin]+'''        changes=(('property_stride','src/physics/native_tu15_body_properties.cpp','entries+index*8u','entries+index*4u'),('removed_value','src/physics/native_tu15_body_properties.cpp','write(0x31c4ceu,output,value)','write(0x31c4ceu,output,0)'),('growth_stride','src/physics/native_tu15_body_properties.cpp','native_tu15_grow_array(array,8u','native_tu15_grow_array(array,4u'))
'''+source[end:]
    source=source.replace("assert text.count(before)==1;target.write_text(text.replace(before,after,1))","assert text.count(before)>=1;target.write_text(text.replace(before,after,1))")
    source=source.replace("'actual allocator request order differs'","'actual allocator request order differs','array read order differs'")
    source=source.replace("remaining_explicit_owners=['default allocator149930'", "remaining_explicit_owners=['1D7660 private removed-output lifetime/full-parent proof','1E0A60 positive part/contact recomputation','default allocator149930'")
    environment=dict(base.__dict__);exec(source,environment);return environment['main']()

if __name__=='__main__':main()
