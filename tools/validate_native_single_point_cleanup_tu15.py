#!/usr/bin/env python3
"""Compare actual2FBB10 and inherited native cleanup regressions to TU1.5."""
import validate_native_bv_cleanup_tu15 as bv


def cases():
    address=bv.parent.base.UI
    template=next(row for row in bv.parent.cases() if row['name']=='wrapped_leaf')
    rows=[]

    def add(name,point=13,**extra):
        rows.append(dict(template,name='single_point_'+name,entry=0x2fbb10,
                         user_words=[*template['user_words'],(0x10c,0xa5a50000|point)],
                         **extra))

    for point in (0,1,0x7fff,0x8000,0xfffe,0xffff):
        add(hex(point),point)
    add('none_without_manager',0xffff)
    rows[-1]['user_words'].append((0x108,0))
    for name,offset,size in [('point',0x10c,2),('manager',0x108,4),
                             ('manager_table',0x600,4),('remove_owner',0x848,4),
                             ('self_table',0x100,4)]:
        add('fault_'+name,fault=dict(address=address+offset,bytes=size,
                                     occurrence=1,write=False))
    add('current_destructor',mutations=[dict(instruction=0x2e4d60,
        address=address+0x100,written=(address+0x900).to_bytes(4,'little').hex())])
    return rows


def transform(source):
    old='else if(entry==0x2f0da0u)native_tu15_cleanup_bv_contact_agent(memory,{receiver},fp,services,observe);'
    assert source.count(old)==1
    source=source.replace(old,old+'\n        else if(entry==0x2fbb10u)native_tu15_cleanup_single_point_contact_agent(memory,{receiver},fp,services);')
    old='  for name,before,after in mutants:'
    assert source.count(old)==1
    source=source.replace(old,'''  mutants += [
   ('single_point_skipped','if(point!=0xffffu)','if(false)'),
   ('single_point_width','const auto point=m.word(agent.identity+12u,2);','const auto point=m.word(agent.identity+12u);'),
   ('single_point_id','std::uint16_t(point),fp);','std::uint16_t(point+1u),fp);')]
  for name,before,after in mutants:''')
    return source


if __name__=='__main__':
    raise SystemExit(bv.main(additional_cases=cases(),additional_owners=(0x2fbb10,),
        additional_pins=('tools/validate_native_single_point_cleanup_tu15.py',),
        source_transform=transform))
