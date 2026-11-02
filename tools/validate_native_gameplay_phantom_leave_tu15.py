#!/usr/bin/env python3
"""Compare genuine TU1.5 gameplay phantom leave/property owners to native C++.

Original instructions and actual initialized pool bytes execute on i386. The
fixture changes ordinary before-input data only; no membership, release or
callback outcome is supplied. All generated banks/builds are owned scratch.
"""
import argparse
import hashlib
import inspect
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT, Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

U = base.UI
BODY, SELF, COMPONENT, HEADER, ENTRIES = (U+x for x in (0x100, 0x200, 0x400, 0x600, 0x800))
OWNERS = (0x1d7530, 0x1d75b0, 0x1da320, 0x1f3550, 0x2f7790, 0x2e4d60,
          0x149da0, 0x149e00, 0x233250, 0x233420)
KERNEL = 'src/physics/native_tu15_gameplay_phantom_membership.cpp'
FILES = ('include/halo2/native_tu15_gameplay_phantom_membership.hpp', KERNEL,
         'include/halo2/native_tu15_agent_cleanup.hpp','src/physics/native_tu15_agent_cleanup.cpp',
         'src/physics/native_tu15_array_allocator.cpp', 'src/physics/native_tu15_array_growth.cpp',
         'tools/validate_native_gameplay_phantom_leave_tu15.py',
         'tools/validate_native_array_growth_tu15.py', 'tools/observe_native_array_growth_tu15.cpp',
         'tests/native_tu15_array_growth_tests.cpp')


def sha(p):
    return hashlib.file_digest(Path(p).open('rb'), 'sha256').hexdigest()


def save(p, value):
    p.write_text(json.dumps(value, indent=2, sort_keys=True)+'\n')


def cases():
    rows = []

    def add(name, entry=0x1da320, count=2, refs=1, owned=False, **kw):
        # Components1 and2 own objects3 and4. Definition5 -> model6 ->
        # physics7. Property and membership banks are actual input bytes.
        words = [(0x130,U+0xa00),(0x134,2),(0x230,U+0xa20),(0x234,2),
                 (0xa00,0x2001),(0xa04,2),(0xa08,0x2002),(0xa0c,9),
                 (0xa20,0x2001),(0xa24,1),(0xa28,0x2002),(0xa2c,7),
                 (0x408,3),(0x494,HEADER),(0x600,ENTRIES),(0x604,count),
                 (0x608,2 if owned else 0x80000002),
                 (0x800,2 | (9<<8) | (refs<<16) | (7<<24)),(0x804,4),
                 (0x808,0x98765432),(0x80c,0x12345678),
                 (0x1044,COMPONENT-0xa0),(0x4a8,4),
                 (0x1244,U+0x1400),(0x142c,U+0x1600),(0x1430,0),(0x1438,U+0x1c00),
                 (0x1600,5),(0x1fe4,3),
                 (0x2058,U+0x2200),(0x2068,U+0x2400),(0x2078,U+0x2600),
                 (0x2238,6),(0x2424,7),(0x2644,U+0x2800),(0x262c,U+0x2a00),
                 (0x2820,0),(0x2a00,0x1000000),
                 (0x3008,2),(0x3118,1),(0x3120,SELF),(0x3218,1),(0x3220,BODY)]
        words += [(0x3000,U+0x3300),(0x332c,0x1f3550),
                  (0x3400,U+0x3500),(0x340c,U+0x3100),(0x3410,U+0x3200),
                  (0x3414,U+0x3000),(0x3418,U+0x3000),(0x341c,0),(0x3420,0),
                  (0x3500,0x2e4d60),(0x3510,0x2e4d60),(0x534,0),(0x3618,2)]
        args = [COMPONENT,1,7,2] if entry==0x1da320 else [U+0x3100,U+0x3200] if entry==0x1f3550 else []
        receiver = BODY if entry in (0x1d7530,0x1d75b0) else U+(0x3400 if entry==0x2f7790 else 0x3000)
        rows.append(dict(name=name,entry=entry,arguments=args,receiver=receiver,
                         eax=BODY,header=HEADER,user_words=words,
                         owned_header=owned,**kw))
        return rows[-1]

    for entry in (0x1d7530,0x1d75b0):
        for count in (0,1,2,3,0xffffffff,0x80000000):
            row=add(f'property_{entry:x}_{count:x}',entry)
            row['user_words'] += [(0x134,count),(0xa10,0),(0xa14,0xfedcba98)]
        row=add(f'property_missing_{entry:x}',entry)
        row['user_words'] += [(0xa00,0x1111),(0xa08,0x2222)]
        row=add(f'property_duplicate_{entry:x}',entry)
        row['user_words'] += [(0xa08,0x2001 if entry==0x1d7530 else 0x2002)]
    for refs in (0,1,2,255):
        add(f'references_{refs}',refs=refs)
    for n in (0xffffffff,0x80000000):
        add(f'negative_count_{n:x}',count=n)
    add('empty_owned_header',count=0,owned=True)
    row=add('empty_borrowed_backing',count=0,owned=True)
    row['user_words'] += [(0x608,0x80000002)]
    add('last_reference_real_release',count=1,owned=True)
    row=add('none_component');row['user_words'] += [(0xa04,0xffffffff)]
    row=add('same_component');row['user_words'] += [(0xa04,1)]
    row=add('no_header');row['user_words'] += [(0x494,0)]
    row=add('missing_properties');row['user_words'] += [(0x134,0),(0x368,4)]
    row=add('reordered_properties');row['user_words'] += [
        (0xa00,0x2002),(0xa04,9),(0xa08,0x2001),(0xa0c,2)]
    row=add('missing_positive_properties');row['user_words'] += [
        (0xa00,0x1234),(0xa08,0x5678),(0x368,4)]
    for name,offset,value in [('other_object',0x804,77),('own_slot',0x800,0x08010902),
                              ('other_slot',0x800,0x07010802),('phantom',0x800,0x07010903),
                              ('no_owner_policy',0x2a00,0),('other_owner',0x1fe4,99),
                              ('other_type',0x1430,0x01000000),
                              ('other_type_shift32',0x1430,0x20000000)]:
        row=add(name);row['user_words'] += [(offset,value)]
    row=add('match_second');row['user_words'] += [(0x800,0xabcdef00),(0x804,99),
                                               (0x808,0x07010902),(0x80c,4)]
    # Sign-extension controls distinguish signed key comparisons from byte masks.
    row=add('signed_keys');row['arguments'][2:]=[0xffffffff,0xfffffffe]
    row['user_words'] += [(0xa0c,0xffffff80),(0x800,0xff0180fe),
                         (0x27f0,0)]  # phantom[-2].shape index (wraps32)
    row=add('negative_shape_index');row['user_words'] += [(0x2820,0xffff),(0x2998,0x1000000)]
    for name,offset,width,occurrence,write in [
        ('property_count',0x134,4,1,False),('property_backing',0x130,4,2,False),
        ('header',0x494,4,1,False),('member_object',0x804,4,1,False),
        ('signed_slot',0x803,1,1,False),('ref_operand',0x802,1,1,False),
        ('definition',0x1600,4,1,False),('shape_index',0x2820,2,1,False),
        ('swap_count',0x604,4,1,True),('swap_key',0x800,4,1,True),
        ('swap_value',0x80c,4,1,False),('flags_after_swap',0x2a00,4,1,False),
        ('clear_owner',0x1fe4,4,1,True)]:
        add('fault_'+name,fault=dict(address=U+offset,bytes=width,occurrence=occurrence,write=write))
    for name,address,occurrence,write in [('allocator',0x48c938,1,False),
                                         ('allocator_again',0x48c938,2,False),
                                         ('release_owner',0x45f4e0,1,False),
                                         ('clear_after_release',U+0x494,1,True)]:
        add('fault_release_'+name,count=0,owned=True,
            fault=dict(address=address,bytes=4,occurrence=occurrence,write=write))
    for name,site,offset,value in [('count_current',0x1da404,0x604,3),
                                  ('flags_current',0x1da484,0x2a00,0),
                                  ('owner_current',0x1da484,0x408,99),
                                  ('header_disappears',0x1da477,0x494,0)]:
        # header_disappears clears the late owner pointer after count reaches0.
        add(name,count=1 if name=='header_disappears' else 2,
            mutations=[dict(instruction=site,address=U+offset,written=struct.pack('<I',value).hex())])
    for self_type in (0,1,2,0xffffffff):
        for other_type in (0,1,2):
            row=add(f'leave_types_{self_type:x}_{other_type:x}',0x1f3550)
            row['user_words'] += [(0x3118,self_type),(0x3218,other_type)]
    for name,offset,value in [('no_self_body',0x3120,0),('no_other_body',0x3220,0),
                              ('none_self_component',0xa24,0xffffffff)]:
        row=add(name,0x1f3550);row['user_words'] += [(offset,value)]
    for name,offset in [('shape_even_without_body',0x3008),('self_property',0x234)]:
        row=add('fault_leave_'+name,0x1f3550,fault=dict(address=U+offset,bytes=4,occurrence=1,write=False))
        if name=='shape_even_without_body':row['user_words'] += [(0x3118,2)]
    for first in (0,22):
        for second in (0,22):
            row=add(f'agent_classes_{first}_{second}',0x2f7790)
            row['user_words'] += [(0x341c,first),(0x3420,second)]
    for name,offset,value,second in [('second_disabled',0x3420,0,22),
                                    ('second_enabled',0x3420,22,0),
                                    ('second_root_current',0x3410,U+0x3600,22),
                                    ('self_table_current',0x3400,U+0x3510,22)]:
        row=add(name,0x2f7790,mutations=[dict(instruction=0x1da404,
            address=U+offset,written=struct.pack('<I',value).hex())])
        row['user_words'] += [(0x341c,22),(0x3420,second)]
    for name,offset in [('class',0x341c),('first_root',0x3410),('shape',0x3414),
                        ('shape_table',0x3000),('leave_owner',0x332c),('second_class',0x3420),
                        ('self_table',0x3400),('dispose_owner',0x3500)]:
        row=add('fault_agent_'+name,0x2f7790,
                fault=dict(address=U+offset,bytes=4,occurrence=1,write=False))
        row['user_words'] += [(0x341c,22),(0x3420,22)]
    return rows


def original(image,banks,rows,scratch,*,source_transform=None,observer_transform=None):
    # Use the physical observer's proven pool/data/store/fault machinery.
    source=inspect.getsource(base.original)
    if source_transform:source=source_transform(source)
    observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()
    needle='if(pc==stop)break;'
    assert observer.count(needle)==1
    record='''if(pc==0x2e4d60u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t argument{};for(unsigned n=0;n<4;++n)argument|=std::to_integer<unsigned>(raw[n])<<(8u*n);allocator_calls.push_back(J{{"kind","ret4"},{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"argument",argument}});}'''
    observer=observer.replace(needle,record+needle)
    observer_source=scratch/'observer.cpp'
    if observer_transform:observer=observer_transform(observer)
    observer_source.write_text(observer)
    needle="str(ROOT/'tools/observe_native_array_growth_tu15.cpp')"
    assert source.count(needle)==1
    source=source.replace(needle,'str(observer_source)')
    start=source.index("        prepare=''\n")
    end=source.index('        assembly.append(',start)
    prepare=r'''        prepare=''
        if case.get('exhaust_header_pool'):
            prepare=f'mov ${SP-0x100},%esp\n drain_header_pool:\n push $0x12\n push $12\n mov $0x10076040,%ecx\n call 0x233250\n test %eax,%eax\n jne drain_header_pool\n movl $0x10076040,0x48c938\n'
        if case.get('owned_header'):
            prepare=f'mov ${SP-0x100},%esp\n push $0x12\n push $12\n mov $0x486080,%ecx\n call 0x149da0\n mov %eax,{UI+0x80}\n mov %eax,{UI+0x494}\n mov %eax,%edi\n mov ${UI+0x600},%esi\n mov $12,%ecx\n rep movsb\n'
            capacity=next(value for offset,value in reversed(case['user_words']) if offset==0x608)
            if capacity<0x80000000:
                prepare+=f'mov ${SP-0x100},%esp\n push $0x12\n push $16\n mov $0x486080,%ecx\n call 0x149da0\n mov {UI+0x80},%edx\n mov %eax,(%edx)\n mov %eax,%edi\n mov ${UI+0x800},%esi\n mov $16,%ecx\n rep movsb\n'
'''
    source=source[:start]+prepare+source[end:]
    needle="mov ${case['receiver']},%ecx"
    assert source.count(needle)==1
    source=source.replace(needle,needle+"\n mov ${case['eax']},%eax\n mov ${case.get('edx',0)},%edx")
    environment=dict(base.__dict__,observer_source=observer_source)
    exec(source,environment)
    return environment['original'](image,banks,rows,scratch)


def project_driver():
    source=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()
    source='#include "halo2/native_tu15_gameplay_phantom_membership.hpp"\n'+source
    needle='      const auto host_sw='
    assert source.count(needle)==1
    extra='''      NativeTu15PhantomCallbackCleanupServices phantom;
      phantom.agents.dispose=[&](auto object,auto owner,auto flag,auto&){
        check(owner.identity==0x2e4d60u,"Unqualified phantom scalar test owner");
        calls.push_back(J{{"kind","ret4"},{"owner",owner.identity},{"receiver",object},{"argument",flag}});
      };
      phantom.leave=[&](auto shape,auto owner,auto self,auto other,auto&f){
        check(owner.identity==0x1f3550u,"Unqualified gameplay phantom leave owner");
        native_tu15_leave_gameplay_phantom(memory,shape.identity,self.identity,other.identity,f,growth,observe);
      };
'''
    source=source.replace(needle,extra+needle)
    begin=source.index('      try{const auto entry=')
    end=source.index('      }catch(const Fault&)',begin)
    source=source[:begin]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>();
        const auto receiver=c.at("receiver").get<std::uint32_t>();
        const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(entry==0x1d7530u)result=native_tu15_phantom_body_property_2001(memory,receiver);
        else if(entry==0x1d75b0u)result=native_tu15_phantom_body_property_2002(memory,receiver);
        else if(entry==0x1da320u)native_tu15_remove_gameplay_phantom_membership(memory,
          {c.at("eax"),args[0],args[1],args[2],args[3]},fp,growth,observe);
        else if(entry==0x1f3550u)native_tu15_leave_gameplay_phantom(memory,receiver,args[0],args[1],fp,growth,observe);
        else if(entry==0x2f7790u)native_tu15_cleanup_phantom_callback_agent(memory,{receiver},fp,phantom);
        else throw std::runtime_error("Unqualified phantom leave owner");
'''+source[end:]
    needle='      check(calls=='
    assert source.count(needle)==1
    source=source.replace(needle,'''      if(!failed&&(c.at("entry")==0x1d7530u||c.at("entry")==0x1d75b0u))
        check(result==c.at("return_word"),name+" property result differs");
'''+needle)
    return source


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compilers',nargs='+',default=['g++','clang++'])
    a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    pins={name:sha(ROOT/name)for name in FILES}
    report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),
                retained_large_artifacts=[],outcomes_supplied=False)
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN
        image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image)
        # Before-input fixture globals, never afterimages or source call results.
        for address,value in [(0x52be5c,U+0x1000),(0x4ecbd0,U+0x1200),(0x4f0414,U+0x2000)]:
            assert not any(first<=address<first+len(raw)for first,raw in banks.items())
            banks[address]=struct.pack('<I',value)
        base.OWNERS=OWNERS
        rows=cases();fixture,authority=original(image,banks,rows,scratch)
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,
             oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for name in ('native_tu15_gameplay_phantom_membership.cpp','native_tu15_agent_cleanup.cpp','native_tu15_array_allocator.cpp','native_tu15_array_growth.cpp'):
            shutil.copyfile(ROOT/'src/physics'/name,project/name)
        (project/'driver.cpp').write_text(project_driver())
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(phantom_leave LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(leave driver.cpp native_tu15_gameplay_phantom_membership.cpp native_tu15_agent_cleanup.cpp native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)\ntarget_include_directories(leave PRIVATE include)\ntarget_compile_features(leave PRIVATE cxx_std_20)\ntarget_link_libraries(leave PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME phantom_leave COMMAND leave "'+str(fixture/'original.json')+'")\n')
        commands=[];results=[]
        def run(label,compiler):
            cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
                 '--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),
                 '--target','leave','--filter','^phantom_leave$',
                 '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1']
            commands.append(cmd);r=subprocess.run(cmd);results.append(dict(label=label,exit_code=r.returncode))
            return r.returncode
        for compiler in a.compilers:
            assert run(compiler,compiler)==0,'Native gameplay phantom leave differs'
        target=project/'native_tu15_gameplay_phantom_membership.cpp';text=target.read_text()
        mutants=[('signed_slot','return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(address,1)))));','return word(address,1);'),
                 ('refcount','(m.word(entry+2u,1)-1u)&255u','(m.word(entry+2u,1)+1u)&255u'),
                 ('skip_owner_clear','if(m.word(shape)&0x1000000u)','if(false)'),
                 ('borrowed_backing','if(std::bit_cast<std::int32_t>(capacity)>=0)','if(false)'),
                 ('skip_header_release','release(allocation,{allocator,owner,owned,12u,0x12u},fp);',''),
                 ('early_flags','const auto last=m.word(header+4u)-1u,data=m.word(header);','const auto early_flags=m.word(shape);\n                    const auto last=m.word(header+4u)-1u,data=m.word(header);')]
        for name,before,after in mutants:
            assert text.count(before)==1,(name,text.count(before))
            target.write_text(text.replace(before,after))
            assert run('negative-'+name,a.compilers[0])!=0,'Original comparison did not reject '+name
            target.write_text(text)
        executed={pc for c in rows for pc in c['executed_sites']};code=base._Code(image,'.text')
        report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),
            original_sha256=NEW_PIN,input_provenance=provenance,authority=authority,
            original_cases=len(rows),original_instructions=sum(c['original_instructions']for c in rows),
            ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),
            fault_prefixes=sum(c['declared_fault']for c in rows),input_changes=sum(c['mutations_applied']for c in rows),
            actual_release_calls=sum(len(c['allocator_calls'])for c in rows),
            negative_controls=[m[0]for m in mutants],commands=commands,results=results,
            body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),
                executed=len(set(base.complete_body(code,owner).code)&executed))for owner in OWNERS},
            scope='Native composed1F3550/property/1DA320 with real149E00/233420 releases; ordinary declared inputs, every retained bank/ordered read/store/FP compared. No live integration or constructor/enter claim.')
    except BaseException as error:
        report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
        save(a.output/'report.json',report)
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
