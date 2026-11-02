#!/usr/bin/env python3
"""Physical normal/final-enter-fault comparison of phantom agent constructors.

Includes the full composed native enter/leave and real-pool regression suite.
Original source EH frame setup executes unchanged in a private i386 TIB. This
does not claim source exception-dispatch/unwind qualification before the first
enter cutoff; native C++ ownership follows the separately pinned FuncInfo.
"""
import inspect
import validate_native_gameplay_phantom_enter_tu15 as enter

leave=enter.parent
base=leave.base


def cases():
    U=leave.U;template=next(c for c in leave.cases()if c['name']=='references_1');rows=[]

    def add(name,first=22,second=23,entry=0x2f78c0,**kw):
        row=dict(template,name='phantom_agent_'+name,entry=entry,receiver=U+0x3400,
                 eax=leave.BODY,arguments=[U+0x3100,U+0x3200,0x9ab01327,0x1234abcd],**kw)
        row['user_words']=[*template['user_words'],
            (0x3100,U+0x3000),(0x310c,0),(0x3200,U+0x3040),(0x320c,0),
            (0x3000,U+0x3300),(0x3014,0),(0x3040,U+0x3340),(0x3048,2),
            (0x3314,0x308540 if first==22 else 0xa9520),
            (0x3354,0x308540 if second==22 else 0xa9520),
            (0x3328,0x1f34d0),(0x3368,0x1f34d0),
            (0x41c,0),(0x4bc,0),(0x1c00,5),(0x1424,0),
            (0x173c,0xffffffff),(0x1d3c,0xffffffff),(0x1708,0),(0x1d08,0),
            (0x19e4,0xffffffff),(0x1fe4,0xffffffff),(0x2a04,0x00007f80)]
        if entry==0x2f7820:row['arguments']=[U+0x3100,U+0x3200,0x1234abcd]
        rows.append(row);return row

    for entry in (0x2f78c0,0x2f7820):
        for first in (22,23):
            for second in (22,23):add(f'{entry:x}_{first}_{second}',first,second,entry)
    for first,second in ((22,23),(23,22),(22,22),(23,23)):
        row=add(f'parent_chains_{first}_{second}',first,second)
        row['arguments'][:2]=[U+0x3600,U+0x3700]
        row['user_words'] += [(0x3600,U+0x3000),(0x360c,U+0x3800),
            (0x380c,U+0x3100),(0x3700,U+0x3040),(0x370c,U+0x3200)]
    for first_type in (0,1,2):
        row=add('root_type_'+str(first_type),22,22)
        row['user_words'] += [(0x3118,first_type)]
    row=add('cold_membership_header',22,23);row['user_words'] += [(0x494,0)]
    row=add('fault_after_allocation_unwind_cutoff',22,23,
        fault=dict(address=U+0x234,bytes=4,occurrence=1,write=False))
    return rows


def original(image,banks,rows,scratch,*,source_transform=None,observer_transform=None):
    metadata=inspect.getsource(base.metadata)
    needle="assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'"
    assert metadata.count(needle)==1
    metadata=metadata.replace(needle,"if op.mem.segment and ins.reg_name(op.mem.segment)=='fs':continue\n                    "+needle)
    exec(metadata,base.__dict__)
    source=inspect.getsource(leave.original)
    needle='    source=inspect.getsource(base.original)\n'
    assert source.count(needle)==1
    extra=r'''    source=inspect.getsource(base.original)
    source=source.replace("sections=['.image", "sections=['.fs 0x71000000 : { *(.fs) }','.image")
    source=source.replace("assembly=[f'.section .image", "assembly=['.section .fs,\"aw\"\\n.zero 4096',f'.section .image")
    source=source.replace(' cld\n fninit',' cld\n mov $123,%eax\n mov $1,%ebx\n mov $descriptor,%ecx\n mov $16,%edx\n int $0x80\n mov $7,%eax\n mov %ax,%fs\n fninit')
    source=source.replace('entry: .long','descriptor:.long 0,0x71000000,0xfffff,0x51\nentry: .long')
'''
    source=source.replace(needle,extra)
    environment=dict(leave.__dict__)
    exec(source,environment)
    return environment['original'](image,banks,rows,scratch,
        source_transform=source_transform,observer_transform=observer_transform)


def driver(source):
    source='#include "halo2/native_tu15_phantom_callback_agent.hpp"\n'+source
    old='      const auto host_sw='
    assert source.count(old)==1
    extra='''      NativeTu15PhantomCallbackAgentServices callback;callback.allocation=growth;
      callback.classify_shape=[&](auto,auto owner,auto&)->std::uint32_t{
        if(owner.identity==0x308540u)return native_tu15_phantom_shape_class_22();
        if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();
        throw std::runtime_error("Unqualified native phantom class owner");
      };
      callback.enter=[&](auto shape,auto owner,auto self,auto other,auto context,auto&f){
        check(owner.identity==0x1f34d0u,"Unqualified native phantom enter owner");
        native_tu15_enter_gameplay_phantom(memory,shape.identity,self.identity,other.identity,context.identity,f,growth,observe);
      };
'''
    source=source.replace(old,extra+old)
    old='        if(entry==0x1d9fb0u)'
    assert source.count(old)==1
    source=source.replace(old,'''        if(entry==0x2f78c0u||entry==0x2f7820u){
          const NativeTu15PhantomCallbackAgentArguments arguments{
            [&]{return args[0];},[&]{return args[1];},[&]{return args[2];},
            [&]{return args[entry==0x2f7820u?2u:3u];}};
          result=(entry==0x2f78c0u?native_tu15_create_phantom_callback_agent(memory,arguments,fp,callback,observe):
            native_tu15_initialize_phantom_callback_agent(memory,{receiver},arguments,fp,callback,observe)).identity;
        }
        else if(entry==0x1d9fb0u)''')
    old='      check(calls=='
    assert source.count(old)==1
    source=source.replace(old,'''      if(!failed&&(c.at("entry")==0x2f78c0u||c.at("entry")==0x2f7820u))
        check(result==c.at("return_word"),name+" phantom agent result differs");
'''+old)
    return source


def transform(source):
    old="        executed={pc for c in rows for pc in c['executed_sites']}"
    assert source.count(old)==1
    extra='''        callback_target=project/'native_tu15_phantom_callback_agent.cpp';callback_text=callback_target.read_text()
        callback_mutants=[
            ('callback_size','agent.identity+4u,36u,2','agent.identity+4u,40u,2'),
            ('callback_class','return 22u;','return 23u;'),
            ('callback_parent_root','m.word(0x2f786du,object+12u,first_root);','m.word(0x2f786du,object+12u,first);'),
            ('callback_enter_cutoff','allocation.disarm(); // Actual2F7917: no allocation cleanup during enter.',''),
            ('callback_retained_shape','m.word(0x2f795au,agent.identity+20u,shape);','')]
        for name,before,after in callback_mutants:
            assert callback_text.count(before)==1,(name,callback_text.count(before))
            callback_target.write_text(callback_text.replace(before,after))
            assert run('negative-'+name,a.compilers[0])!=0,'Original did not reject '+name
            callback_target.write_text(callback_text)
        mutants+=callback_mutants
'''
    source=source.replace(old,extra+old)
    old="scope='Complete native1D9FB0/1DA0F0/1F34D0 plus composed leave regressions. Real pool allocation/release, growth/copy and actual pool exhaustion to the original null-dereference prefix; ordinary declared inputs and all retained banks/ordered reads/stores/FP compared. No constructor/factory/live claim.'"
    assert source.count(old)==1
    return source.replace(old,"scope='Native phantom2F78C0/2F7820normal returns and first-enter fault after allocation-unwind cutoff; full composed enter/leave/real-pool regressions. Original TIB ABI setup executes unchanged. Pre-cutoff source exception dispatch/unwind and live delivery remain unqualified.'")


if __name__=='__main__':
    raise SystemExit(enter.main(additional_cases=cases(),
        additional_owners=(0x2f78c0,0x2f7820,0x308540,0xa9520),
        additional_pins=('include/halo2/native_tu15_phantom_callback_agent.hpp',
            'src/physics/native_tu15_phantom_callback_agent.cpp',
            'src/physics/native_tu15_contact_agent_factory.cpp',
            'src/physics/native_tu15_contact_unwind_internal.hpp',
            'tools/validate_native_phantom_callback_agent_tu15.py'),
        additional_sources=('native_tu15_phantom_callback_agent.cpp',
            'native_tu15_contact_agent_factory.cpp','native_tu15_contact_unwind_internal.hpp'),
        driver_transform=driver,original_override=original,source_transform=transform))
