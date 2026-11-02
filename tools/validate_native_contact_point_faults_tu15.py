#!/usr/bin/env python3
"""Strict child DATA fault/unknown-service prefixes; no private-byte waiver."""
import copy
import inspect
import validate_native_contact_point_removal_tu15 as parent


def cases():
    rows=[case for case in parent.cases()if case['entry']!=0x322f70]
    world=next(case for case in rows if case['name']=='listener_0x3224d0_5')
    body=next(case for case in rows if case['name']=='listener_0x31fee0_5')
    collector=next(case for case in rows if case['name']=='collector_3_1')
    special=next(case for case in rows if case['name']=='collector_2_0')
    def add(name,source,address,size=4,occurrence=1,write=False):
        case=copy.deepcopy(source);case.update(name='fault_'+name,fault=dict(address=address,bytes=size,occurrence=occurrence,write=write));rows.append(case)
    for label,source,obj,offset,data,listener in (
            ('world',world,parent.WORLD,0xb4,parent.U+0x1000,parent.U+0x1430),
            ('body',body,parent.FIRST,0x60,parent.U+0x1100,parent.U+0x1630)):
        add(label+'_subject',source,parent.EVENT+16,write=True)
        add(label+'_count',source,obj+offset+4)
        add(label+'_backing',source,obj+offset)
        add(label+'_reverse_slot',source,data+12)
        add(label+'_listener_table',source,listener)
        add(label+'_listener_owner',source,parent.U+0xf04)
        add(label+'_compaction_count',source,obj+offset+4,occurrence=2)
        add(label+'_compaction_publication',source,obj+offset+4,write=True)
        add(label+'_compaction_copy',source,data+8,write=True)
    for label,address,size,occurrence,writing in (
            ('map_pointer',parent.MANAGER+0x30,4,1,False),
            ('point_index',parent.MAP+5,1,1,False),
            ('point_deleted',parent.MAP+5,1,1,True),
            ('old_count',parent.MANAGER+0xa4,4,1,False),
            ('info0',parent.SP-16,4,1,True),
            ('info1',parent.SP-12,4,1,True),
            ('info2',parent.SP-8,4,1,True),
            ('info3',parent.SP-4,4,1,True),
            ('point_copy_source',parent.POINTS+56,4,1,False),
            ('tail_word',parent.POINTS+28+24,2,1,True),
            ('point_count_publication',parent.MANAGER+0xa4,4,1,True),
            ('active_count',parent.MANAGER+0x48,4,1,False),
            ('map_count',parent.MANAGER+0x34,4,1,False),
            ('collector_receiver',parent.MANAGER+0x24,4,1,False),
            ('collector_table',parent.UPDATE,4,1,False),
            ('collector_owner',parent.TABLE+12,4,1,False),
            ('flags_read',parent.MANAGER+0x60,4,1,False),
            ('flags_write',parent.MANAGER+0x60,4,1,True)):
        add(label,collector,address,size,occurrence,writing)
    add('two_point_info1',special,parent.SP-12,occurrence=2,write=True)
    add('two_point_info2',special,parent.SP-8,occurrence=2,write=True)
    add('two_point_info3',special,parent.SP-4,occurrence=2,write=True)
    for name,source,stop in (('listener',world,0x2e4d60),('collector',collector,0x8fdb0)):
        case=copy.deepcopy(source);case.update(name='unbound_'+name,stop_boundary=stop,required_owner=True);rows.append(case)
    return rows


def main():
    source=inspect.getsource(parent.main)
    source=source.replace("all_cases=[case for case in cases()if not args.children_only or case['entry']!=0x322f70]",'all_cases=cases()')
    anchor="        fixture,authority=environment['original'](image,{},all_cases,scratch)"
    source=source.replace(anchor,anchor+"\n        assert all(case['declared_fault']for case in all_cases if 'fault'in case)")
    source=source.replace(');mutate(owner.identity);};',');if(c.value("required_owner",false))throw Required{};mutate(owner.identity);};')
    source=source.replace("'tools/validate_native_contact_point_removal_tu15.py','tools/validate_native_array_growth_tu15.py',", "'tools/validate_native_contact_point_removal_tu15.py','tools/validate_native_contact_point_faults_tu15.py','tools/validate_native_array_growth_tu15.py',")
    source=source.replace("scope='Declared valid point/map/listener DATA; all DATA bytes including preserved event padding and all ordered operands compared. Actual RET4/RET8 stubs are declared component services only. Remaining316AA0 stops at genuine entry; no full positive live point-removal claim.'", "scope='Strict direct179-site DATA and valid fault/service-entry prefixes. Every ordered read/store/preimage/full owned bank/FP compared. Atomic read-modify-write prewrite and REP copy write-fault instruction atomicity are not claimed; no source-private parent preinitialization bytes waived. Genuine RET4/RET8 stubs are declared component services only.'")
    environment=dict(parent.__dict__,cases=cases)
    exec(source,environment)
    return environment['main']()


if __name__=='__main__':
    raise SystemExit(main())
