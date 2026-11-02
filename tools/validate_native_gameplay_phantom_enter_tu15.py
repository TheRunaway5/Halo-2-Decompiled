#!/usr/bin/env python3
"""Physical TU1.5 phantom admission/enter/growth plus full leave regressions."""
import inspect
import struct
import validate_native_gameplay_phantom_leave_tu15 as parent


def cases():
    rows=parent.cases();U=parent.U
    template=next(c for c in rows if c['name']=='references_1')

    def add(name,entry=0x1da0f0,**kw):
        row=dict(template,name='enter_'+name,entry=entry,receiver=1,eax=parent.BODY,
                 edx=2,arguments=[parent.COMPONENT,7,2],**kw)
        row['user_words']=[*template['user_words'],(0x4bc,0),(0x1d3c,0xffffffff),
                           (0x1d08,0),(0x2a04,0x00007f80)]
        if entry==0x1d9fb0:row['eax']=2;row['arguments']=[]
        if entry==0x1f34d0:
            row['receiver']=U+0x3000;row['arguments']=[U+0x3100,U+0x3200,0x9abc0132]
        rows.append(row);return row

    for refs in (0,1,2,254,255):
        row=add('existing_ref_'+str(refs));row['user_words'] += [(0x800,0x07000902|(refs<<16))]
    for name,words in [('append',[(0x604,0)]),('no_header',[(0x494,0)]),
                       ('none_component',[(0xa04,0xffffffff)]),('same_component',[(0xa04,1)]),
                       ('missing_properties',[(0x134,0),(0x368,4)]),
                       ('reordered_properties',[(0xa00,0x2002),(0xa04,9),(0xa08,0x2001),(0xa0c,2)]),
                       ('missing_positive_properties',[(0xa00,0x1111),(0xa08,0x2222),(0x368,4)]),
                       ('new_owner',[(0x604,0),(0x1fe4,0xffffffff)]),
                       ('other_owner',[(0x604,0),(0x1fe4,99)]),
                       ('negative_count',[(0x604,0xffffffff)]),
                       ('no_owner_policy',[(0x604,0),(0x2a00,0)]),
                       ('signed_shape_index',[(0x604,0),(0x2820,0xffff),
                                              (0x2998,0x1000000),(0x299c,0x00007f80)])]:
        row=add(name);row['user_words'] += words
    for owned in (False,True):
        row=add('growth_'+str(owned));row['owned_header']=owned
        row['user_words'] += [(0x804,99),(0x80c,98),(0x608,2 if owned else 0x80000002)]
    row=add('match_second');row['user_words'] += [(0x804,99),(0x808,0x07010902),(0x80c,4)]
    row=add('signed_keys');row['arguments'][1:]=[0xffffffff,0xfffffffe]
    row['user_words'] += [(0xa0c,0xffffff80),(0x800,0xff0180fe),(0x27f0,0),
                          (0x2a04,0x00007f80)]
    for kind in (0,1,2,7,24,27,31,32,33,255):
        for attached in (False,True):
            row=add(f'policy_type{kind}_attached{attached}',0x1d9fb0)
            row['user_words'] += [(0x1430,kind<<24),(0x1d3c,99 if attached else 0xffffffff)]
    for name,words in [('lower_reject',[(0x4bc,1),(0x2a04,0x00007f02)]),
                       ('upper_reject',[(0x4bc,2),(0x2a04,0x00000180)]),
                       ('upper_equal',[(0x4bc,2),(0x2a04,0x00000280)]),
                       ('lower_equal',[(0x4bc,2),(0x2a04,0x00007f02)]),
                       ('negative_group',[(0x4bc,0x80),(0x2a04,0x00007f80)]),
                       ('zero_group_ignores_limits',[(0x4bc,0),(0x2a04,0x0000807f)]),
                       ('attached_excluded',[(0x2a00,8),(0x1d3c,99)]),
                       ('unattached_excluded',[(0x2a00,16)]),
                       ('flight_excluded',[(0x2a00,0x8000000),(0x1d08,0x00040000)]),
                       ('flight_allowed',[(0x2a00,0),(0x1d08,0x00040000)]),
                       ('type_excluded',[(0x2a00,32)]),
                       ('wrapped_type_excluded',[(0x1430,32<<24),(0x2a00,32)])]:
        row=add('policy_'+name,0x1d9fb0);row['user_words'] += words
    row=add('policy_none',0x1d9fb0);row['edx']=0xffffffff
    for self_type in (0,1,2):
        for other_type in (0,1,2):
            row=add(f'roots_{self_type}_{other_type}',0x1f34d0)
            row['user_words'] += [(0x3118,self_type),(0x3218,other_type)]
    row=add('root_cold_header',0x1f34d0);row['user_words'] += [(0x494,0)]
    row=add('actual_header_allocation_exhaustion',exhaust_header_pool=True,
            fault=dict(address=4,bytes=4,occurrence=1,write=False))
    row['user_words'] += [(0x494,0)]
    for name,address,width,occurrence,write in [
        ('property',U+0x134,4,1,False),('second_backing',U+0x130,4,2,False),
        ('policy_group',U+0x4bc,1,1,False),('policy_flags',U+0x2a00,4,1,False),
        ('header_initial',U+0x494,4,1,False),('new_header_allocator',0x48c938,4,1,False),
        ('append_count',U+0x604,4,1,True),('append_slot',U+0x803,1,1,True),
        ('append_object',U+0x804,4,1,True),('increment_ref',U+0x802,1,1,False),
        ('owner_store',U+0x1fe4,4,1,True)]:
        row=add('fault_'+name,fault=dict(address=address,bytes=width,occurrence=occurrence,write=write))
        row['user_words'] += [(0x604,0),(0x1fe4,0xffffffff)]
        if name=='new_header_allocator':row['user_words'] += [(0x494,0)]
        # Exercise INC's own read on an existing member. A new-record zero
        # store would make diagnostic preimage reads overlap this fault key.
        if name=='increment_ref':row['user_words'] += [(0x604,2)]
    for name,site,offset,value in [('flags_current',0x1da2bd,0x2a00,0),
                                  ('own_object_current',0x1da2bd,0x408,99),
                                  ('ref_after_policy',0x1da301,0x802,254)]:
        row=add(name,mutations=[dict(instruction=site,address=U+offset,
                                    written=(bytes([value])if offset==0x802 else struct.pack('<I',value)).hex())])
        row['user_words'] += [(0x604,0),(0x1fe4,0xffffffff)]
    return rows


def driver():
    source=parent.project_driver()
    source='#include "halo2/native_tu15_gameplay_phantom_enter.hpp"\n'+source
    old='growth.allocate=[&](const auto&r,auto&f){calls.push_back('
    assert source.count(old)==1
    source=source.replace(old,'growth.allocate=[&](const auto&r,auto&f){if(r.owner==0x233250u)return allocation(r,f);calls.push_back(')
    old='        if(entry==0x1d7530u)'
    assert source.count(old)==1
    source=source.replace(old,'''        if(entry==0x1d9fb0u)result=native_tu15_admit_gameplay_phantom_member(memory,{receiver,c.at("edx"),c.at("eax")});
        else if(entry==0x1da0f0u)native_tu15_add_gameplay_phantom_membership(memory,{c.at("eax"),args[0],receiver,args[1],args[2]},fp,growth,observe);
        else if(entry==0x1f34d0u)native_tu15_enter_gameplay_phantom(memory,receiver,args[0],args[1],args[2],fp,growth,observe);
        else if(entry==0x1d7530u)''')
    old='      check(calls=='
    assert source.count(old)==1
    source=source.replace(old,'''      if(!failed&&c.at("entry")==0x1d9fb0u)
        check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" native admission AL differs");
'''+old)
    return source


def main(*, additional_cases=(), additional_owners=(), additional_pins=(),
         additional_sources=(), driver_transform=None, original_override=None,
         source_transform=None):
    source=inspect.getsource(parent.main)
    old="'native_tu15_gameplay_phantom_membership.cpp','native_tu15_agent_cleanup.cpp'"
    assert source.count(old)==1
    source=source.replace(old,"'native_tu15_gameplay_phantom_enter.cpp',"+old)
    old='add_executable(leave driver.cpp native_tu15_gameplay_phantom_membership.cpp'
    assert source.count(old)==1
    source=source.replace(old,'add_executable(leave driver.cpp native_tu15_gameplay_phantom_enter.cpp native_tu15_gameplay_phantom_membership.cpp')
    if additional_sources:
        old="'native_tu15_gameplay_phantom_enter.cpp','native_tu15_gameplay_phantom_membership.cpp'"
        assert source.count(old)==1
        source=source.replace(old,','.join(repr(name)for name in additional_sources)+','+old)
        old='add_executable(leave driver.cpp '
        assert source.count(old)==1
        source=source.replace(old,old+' '.join(additional_sources)+' ')
    old="        executed={pc for c in rows for pc in c['executed_sites']}"
    assert source.count(old)==1
    extra='''        enter_target=project/'native_tu15_gameplay_phantom_enter.cpp';enter_text=enter_target.read_text()
        enter_mutants=[
            ('enter_refcount','(m.word(record+2u,1)+1u)&255u','(m.word(record+2u,1)-1u)&255u'),
            ('enter_policy_bypass','if(!native_tu15_admit_gameplay_phantom_member(memory,{input.component_datum,other_component,input.phantom_index}))return;',''),
            ('enter_slot','record+3u,input.body_slot,1','record+3u,0,1'),
            ('enter_owner_store','m.word(0x1da301u,body+0x3e4u,own);',''),
            ('policy_signed_group','const auto other_body=m.word(other_header+8u),group=m.signed_byte(other_component+0x1cu);','const auto other_body=m.word(other_header+8u),group=m.word(other_component+0x1cu,1);'),
            ('policy_type_shift','1u<<((type+5u)&31u)','1u<<((type+4u)&31u)')]
        for name,before,after in enter_mutants:
            assert enter_text.count(before)==1,(name,enter_text.count(before))
            enter_target.write_text(enter_text.replace(before,after))
            assert run('negative-'+name,a.compilers[0])!=0,'Original did not reject '+name
            enter_target.write_text(enter_text)
        mutants+=enter_mutants
'''
    source=source.replace(old,extra+old)
    source=source.replace("scope='Native composed1F3550/property/1DA320 with real149E00/233420 releases; ordinary declared inputs, every retained bank/ordered read/store/FP compared. No live integration or constructor/enter claim.'",
        "scope='Complete native1D9FB0/1DA0F0/1F34D0 plus composed leave regressions. Real pool allocation/release, growth/copy and actual pool exhaustion to the original null-dereference prefix; ordinary declared inputs and all retained banks/ordered reads/stores/FP compared. No constructor/factory/live claim.'")
    if source_transform:source=source_transform(source)
    environment=dict(parent.__dict__,cases=lambda:[*cases(),*additional_cases],
        project_driver=lambda:driver_transform(driver())if driver_transform else driver(),
        original=original_override or parent.original,__doc__=__doc__,
        OWNERS=(*parent.OWNERS,0x1d9fb0,0x1da0f0,0x1f34d0,0x2e3250,0x2e2e30,*additional_owners),
        FILES=(*parent.FILES,'include/halo2/native_tu15_gameplay_phantom_enter.hpp',
               'src/physics/native_tu15_gameplay_phantom_enter.cpp',
               'tools/validate_native_gameplay_phantom_enter_tu15.py',*additional_pins))
    exec(source,environment)
    return environment['main']()


if __name__=='__main__':
    raise SystemExit(main())
