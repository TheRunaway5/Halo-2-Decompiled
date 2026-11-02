#!/usr/bin/env python3
"""Physical original322F70/3224D0/31FEE0/325D90 source-DATA qualification.

Declared valid point/listener containers, genuine RET4/RET8 default callbacks,
and explicit entry prefixes at remaining316AA0. No supplied callee outcomes,
original GP postimages, byte waivers or source control-frame model.
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
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

OWNERS = (0x322f70, 0x3224d0, 0x31fee0, 0x325d90, 0x2e4d60, 0x8fdb0)
U, SP = base.UI, base.SP
MANAGER, WORLD, FIRST, SECOND = U+0x100, U+0x400, U+0x800, U+0xa00
UPDATE, TABLE, MAP, POINTS = U+0xc00, U+0xc80, U+0x1800, U+0x2000
EVENT = U+0x3000


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True)+'\n')


def cases():
    rows = []

    def add(name, entry=0x322f70, count=3, index=1, patterns=((), (), ()), **extra):
        point = 5
        words = [(0x108, WORLD), (0x11c, FIRST), (0x120, SECOND),
                 (0x124, UPDATE), (0x130, MAP), (0x134, 12), (0x148, count),
                 (0x160, 0x10), (0x1a0, POINTS), (0x1a4, count),
                 (0xc00, TABLE), (0xc8c, 0x8fdb0), (0xf04, 0x2e4d60)]
        # Every non-FF map byte denotes a real current point index. The target
        # ID is5; additional map bytes cover less/equal/greater index fixups.
        raw_map = bytearray([0xff]*12)
        for n in range(count):
            raw_map[n] = n
        raw_map[point] = index
        for n in range(0, 12, 4):
            words.append((0x1800+n, int.from_bytes(raw_map[n:n+4], 'little')))
        for obj, offset, data, pattern in zip((WORLD, FIRST, SECOND), (0xb4, 0x60, 0x60),
                                               (U+0x1000, U+0x1100, U+0x1200), patterns):
            words.extend([(obj-U+offset, data), (obj-U+offset+4, len(pattern))])
            for n, present in enumerate(pattern):
                listener = U+0x1400+(data-(U+0x1000))*2+n*0x10
                words.extend([(data-U+n*4, listener if present else 0), (listener-U, U+0xf00)])
        # Source reuses the now-dead point formal's low byte for316AA0 output.
        # It is ordinary formal DATA and is never consumed again before RET4.
        event, info, result = SP-20, SP-60, SP+4
        receiver, args = MANAGER, [point]
        if entry in (0x3224d0, 0x31fee0):
            event, info, result = EVENT, 0, 0
            receiver, args = 0, [WORLD if entry==0x3224d0 else FIRST, event]
        elif entry==0x325d90:
            event, info, result = 0, SP-16, 0
            receiver, args = MANAGER+0x10, [point]
        row = dict(name=name, entry=entry, receiver=receiver, arguments=args,
                   user_words=words, event=event, info=info, result_byte=result, **extra)
        rows.append(row)
        return row

    patterns = ((), (0,), (1,), (0, 1), (1, 0), (0, 1, 0, 1),
                (1, 0, 1, 0), (0, 0, 0), (1, 1, 1))
    for n, pattern in enumerate(patterns):
        add('parent_listeners_'+str(n), patterns=(pattern, pattern, pattern))
        for entry in (0x3224d0, 0x31fee0):
            add('listener_'+hex(entry)+'_'+str(n), entry=entry, patterns=(pattern, pattern, pattern))
    for count in (1, 2, 3, 5):
        for index in range(count):
            add('collector_'+str(count)+'_'+str(index), entry=0x325d90, count=count, index=index)
            add('parent_'+str(count)+'_'+str(index), count=count, index=index)
    for n in range(3):
        row = add('remaining_world_update_prefix_'+str(n), count=1, index=0,
                  stop_boundary=0x316aa0, required_owner=True)
        row['user_words'] += [(0x52c, (0, 1, 0xff)[n])]
    # Positive+A4 skips world update. This is the source manager condition,
    # separate from collector+94 point count at the same manager+A4 address.
    # Removing its final point naturally selects the316AA0 prefix, so retain
    # explicit prefixes for count1 instead of inventing a nonzero postcount.
    for row in rows:
        if row['entry']==0x322f70 and row['name'].startswith(('parent_1_',)):
            row.update(stop_boundary=0x316aa0, required_owner=True)
    def mutation(site, address, value, size=4):
        return dict(instruction=site, address=address, written=value.to_bytes(size, 'little').hex())
    row = add('world_callback_changes_first_count', patterns=((1,), (1, 1), (1,)))
    row['mutations'] = [mutation(0x2e4d60, FIRST+0x64, 0)]
    row = add('callback_reloads_backing', patterns=((1, 1, 1), (), ()))
    row['user_words'] += [(0x1300, U+0x1400), (0x1304, 0), (0x1308, U+0x1420)]
    row['mutations'] = [mutation(0x2e4d60, WORLD+0xb4, U+0x1300)]
    row = add('callback_retains_original_second_body', patterns=((1,), (), (1,)))
    row['user_words'] += [(0x650, 0)]
    row['mutations'] = [mutation(0x2e4d60, MANAGER+0x20, U+0x600)]
    row = add('collector_callback_flags_reread')
    row['mutations'] = [mutation(0x8fdb0, MANAGER+0x60, 0x80000020)]
    row = add('map_backing_reread', entry=0x325d90, count=5, index=1)
    row['user_words'] += [(0x1900, 0x03020100), (0x1904, 0xff010404), (0x1908, 0xffffffff)]
    row['mutations'] = [mutation(0x325e44, MANAGER+0x30, U+0x1900)]
    for entry, obj, offset in ((0x3224d0,WORLD,0xb4),(0x31fee0,FIRST,0x60)):
        row=add('direct_callback_reloads_'+hex(entry),entry=entry,patterns=((1,1,1),(1,1,1),()))
        row['user_words'] += [(0x1300,U+0x1400),(0x1304,0),(0x1308,U+0x1420)]
        row['mutations']=[mutation(0x2e4d60,obj+offset,U+0x1300)]
        row=add('direct_compaction_count_reread_'+hex(entry),entry=entry,patterns=((0,1,1,1),(0,1,1,1),()))
        row['mutations']=[mutation(0x32253e if entry==0x3224d0 else 0x31ff3a,obj+offset+4,2)]
    row=add('direct_record_backing_reread',entry=0x325d90,count=5,index=0)
    row['mutations']=[mutation(0x325e05,MANAGER+0xa0,POINTS+0x400)]
    row=add('direct_info_current_fields',entry=0x325d90,count=2,index=0)
    row['mutations']=[mutation(0x325dd9,SP-12,0x100)]
    row=add('direct_collector_callback_flags',entry=0x325d90,count=3,index=2)
    row['mutations']=[mutation(0x8fdb0,MANAGER+0x60,0x80000020)]
    for name, address, size, occurrence, writing in (
        ('manager_world', MANAGER+8, 4, 1, False),
        ('first_body', MANAGER+0x1c, 4, 1, False),
        ('event_point', SP-20, 2, 1, True),
        ('event_manager', SP-16, 4, 1, True),
        ('world_subject', SP-4, 4, 1, True),
        ('world_count', WORLD+0xb8, 4, 1, False),
        ('first_count', FIRST+0x64, 4, 1, False),
        ('point_map', MANAGER+0x30, 4, 1, False),
        ('point_index', MAP+5, 1, 1, False),
        ('point_deleted', MAP+5, 1, 1, True),
        ('old_count', MANAGER+0xa4, 4, 1, False),
        ('info_zero', SP-60, 4, 1, True),
        ('point_copy', POINTS+56, 4, 1, False),
        ('tail_word', POINTS+28+0x18, 2, 1, True),
        ('published_count', MANAGER+0xa4, 4, 1, True),
        ('collector_receiver', MANAGER+0x24, 4, 1, False),
        ('collector_owner', TABLE+0xc, 4, 1, False),
        ('flags', MANAGER+0x60, 4, 1, False),
    ):
        add('fault_'+name, fault=dict(address=address, bytes=size, occurrence=occurrence, write=writing))
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compilers',nargs='+',default=['g++','clang++'])
    parser.add_argument('--children-only',action='store_true',help='Strict direct94-site listener +85-site collector qualification; fullparent raw-before ledger remains separate')
    args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    files=('include/halo2/native_tu15_contact_point_removal.hpp','src/physics/native_tu15_contact_point_removal.cpp',
           'tools/validate_native_contact_point_removal_tu15.py','tools/validate_native_array_growth_tu15.py',
           'tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp')
    pins={name:sha(ROOT/name)for name in files}
    report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,
                retained_large_artifacts=[],supplied_original_outcomes=False,
                actual_live_positive_input_claimed=False)
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN
        image=Xbe(NEW_DEFAULT);base.OWNERS=OWNERS
        assert image.read(0x2e4d60,3)==bytes.fromhex('c20400')
        assert image.read(0x8fdb0,3)==bytes.fromhex('c20800')
        observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()
        anchor='if(pc==stop)break;'
        record='''if(pc==0x2e4d60u||pc==0x8fdb0u||pc==0x316aa0u){
          const auto argument=[&](unsigned n){const auto b=memory(child.pid,std::uint32_t(regs.rsp)+4u+4u*n,4);std::uint32_t v{};for(unsigned k=0;k<4;++k)v|=std::to_integer<unsigned>(b[k])<<(8*k);return v;};
          J call{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)}};
          if(pc==0x2e4d60u){const auto event=argument(0);call["event"]=event;call["data"]=hex(memory(child.pid,event,20));}
          else if(pc==0x8fdb0u){call["collector"]=argument(0);call["info"]=argument(1);call["data"]=hex(memory(child.pid,argument(1),16));}
          else{call["result_byte"]=argument(0);call["collector"]=argument(1);}
          allocator_calls.push_back(call);
        }'''
        assert observer.count(anchor)==1;observer=observer.replace(anchor,record+anchor)
        observer=observer.replace('pc==mutation.at("instruction")&&!pending.empty()',
                                  'pc==mutation.at("instruction")&&(!pending.empty()||pc==0x2e4d60u||pc==0x8fdb0u)')
        observer_path=scratch/'observer.cpp';observer_path.write_text(observer)
        source=inspect.getsource(base.original)
        source=source.replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)')
        source=source.replace("stack=bytearray(0x10000);struct.pack_into", "stack=bytearray(0x10000);stack[SP-STACK-18:SP-STACK-16]=bytes([all_cases.index(case)%256,0xa5]);struct.pack_into")
        anchor="        active=dict(m,**{k:case[k] for k in ('fault','mutations') if k in case})"
        assert source.count(anchor)==1
        source=source.replace(anchor,anchor+'''
        private=[]
        if case['entry']==0x322f70:private=[dict(address=SP-20,bytes=20),dict(address=SP-60,bytes=16)]
        elif case['entry']==0x325d90:private=[dict(address=SP-16,bytes=16)]
        active['regions']=[*m['regions'],*private]
''')
        source=source.replace('hex(STOP)]','hex(case.get("stop_boundary",STOP))]')
        environment=dict(base.__dict__,observer_path=observer_path);exec(source,environment)
        all_cases=[case for case in cases()if not args.children_only or case['entry']!=0x322f70]
        fixture,authority=environment['original'](image,{},all_cases,scratch)
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        shutil.copyfile(ROOT/'src/physics/native_tu15_contact_point_removal.cpp',project/'native.cpp')
        driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()
        begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)
        replacement='''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
      NativeTu15ContactPointRemovalServices services;
      const auto payload=[&](std::uint32_t p,unsigned bytes){const auto i=memory.at(p,bytes);return hex(std::span(i->second).subspan(p-i->first,bytes));};
      const auto mutate=[&](std::uint32_t owner){unsigned n{};for(const auto&change:c.value("mutations",J::array())){if(change.at("instruction")==owner&&applied.insert(n).second){memory.write(change.at("address"),unhex(change.at("written")));++mutations;}++n;}};
      services.listener_removed=[&](auto receiver,auto owner,auto event,auto&){check(owner.identity==0x2e4d60u,"Unqualified component point listener");calls.push_back(J{{"owner",owner.identity},{"receiver",receiver},{"event",event},{"data",payload(event,20)}});mutate(owner.identity);};
      services.collector_removed=[&](auto receiver,auto owner,auto collector,auto info,auto&){check(owner.identity==0x8fdb0u,"Unqualified component collector update");calls.push_back(J{{"owner",owner.identity},{"receiver",receiver},{"collector",collector},{"info",info},{"data",payload(info,16)}});mutate(owner.identity);};
      services.update_world=[&](auto world,auto result,auto collector,auto&){calls.push_back(J{{"owner",0x316aa0u},{"receiver",world},{"result_byte",result},{"collector",collector}});throw Required{};};
'''
        driver='#include "halo2/native_tu15_contact_point_removal.hpp"\n'+driver[:begin]+replacement+driver[end:]
        begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
        driver=driver[:begin]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>(),receiver=c.at("receiver").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(entry==0x322f70u)native_tu15_remove_contact_point(memory,receiver,args[0],{c.at("event"),c.at("info"),c.at("result_byte")},fp,services,observe);
        else if(entry==0x325d90u)native_tu15_remove_collector_contact_point(memory,receiver,args[0],c.at("info"),fp,services,observe);
        else native_tu15_notify_removed_contact_point(memory,entry==0x3224d0u,args[0],args[1],fp,services,observe);
'''+driver[end:]
        driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','')
        (project/'driver.cpp').write_text(driver)
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(contact_point LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(point driver.cpp native.cpp)\ntarget_include_directories(point PRIVATE include)\ntarget_compile_features(point PRIVATE cxx_std_20)\ntarget_link_libraries(point PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME point_removal COMMAND point "'+str(fixture/'original.json')+'")\n')
        results=[];commands=[]
        def run(label,compiler):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/label),'--target','point','--filter','^point_removal$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1']
            commands.append(command);result=subprocess.run(command);results.append(dict(label=label,exit_code=result.returncode));return result.returncode
        for compiler in args.compilers:
            assert run(compiler,compiler)==0,'Native point removal differs'
        kernel=project/'native.cpp';text=kernel.read_text()
        mutants=(('wrong_copy_stride','destination+28u+n*4u','destination+24u+n*4u'),
                 ('wrong_event_subject','world?0u:object','object'),
                 ('wrong_map_fixup','less(index,value)','less(value,index)'))
        for name,before,after in mutants:
            assert text.count(before)==1
            kernel.write_text(text.replace(before,after));assert run('negative-'+name,args.compilers[0])!=0,'Original failed to reject '+name;kernel.write_text(text)
        executed={pc for case in all_cases for pc in case['executed_sites']};code=_Code(image,'.text')
        report.update(passed=all(sha(ROOT/name)==value for name,value in pins.items()),authority=authority,
                      original_sha256=NEW_PIN,original_cases=len(all_cases),
                      original_instructions=sum(case['original_instructions']for case in all_cases),
                      ordered_reads=sum(len(case['reads'])for case in all_cases),
                      ordered_stores=sum(len(case['writes'])for case in all_cases),
                      genuine_callbacks=sum(len(case['allocator_calls'])for case in all_cases),
                      fault_prefixes=sum(case['declared_fault']for case in all_cases),
                      declared_input_mutations=sum(case['mutations_applied']for case in all_cases),
                      remaining_world_update_prefixes=sum(case.get('required_owner',False)for case in all_cases),
                      body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),executed=len(set(base.complete_body(code,owner).code)&executed))for owner in OWNERS},
                      results=results,commands=commands,negative_controls=[m[0]for m in mutants],
                      scope='Declared valid point/map/listener DATA; all DATA bytes including preserved event padding and all ordered operands compared. Actual RET4/RET8 stubs are declared component services only. Remaining316AA0 stops at genuine entry; no full positive live point-removal claim.')
    except BaseException as error:
        report['error']=str(error)or repr(error)
        if isinstance(error,subprocess.CalledProcessError):report['child_stderr']=str(error.stderr)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(path.stat().st_blocks*512 for path in scratch.rglob('*')if path.is_file())
        save(args.output/'report.json',report)
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
