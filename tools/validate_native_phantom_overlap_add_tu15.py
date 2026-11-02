#!/usr/bin/env python3
"""Physical TU1.5 overlap addition, notification and genuine pool growth.

Declared valid phantom/listener DATA, actual initialized pool BEFORE inputs,
unchanged source instructions and authentic RET4 listener callbacks. Source
private event fields are ordinary native DATA; unused padding is not invented.
This does not qualify an unconverted live added-listener owner.
"""
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')

def cases():
    U=base.UI;P=U+0x100;O=U+0x300;D=U+0x500;L=U+0x600;rows=[]
    def add(name,items=(),listeners=(),flag=0,entry=0x315b00,capacity=None,shape=1,owned=False,**extra):
        simple=entry==0x315300;helper=entry==0x314f80;header=P+(0x80 if simple else 0xc0)
        event=base.SP-(0x10 if helper else 0x2c if simple else 0x28)
        cap=len(items)+2 if capacity is None else capacity
        words=[(0x300,shape),(0x400,1),(header-U,D),(header-U+4,len(items)),
            (header-U+8,cap if owned else cap|0x80000000),(0x13c,L),(0x140,len(listeners)),
            (0x700,U+0x800),(0x720,U+0x800),(0x740,U+0x800),(0x800,0x2e4d60)]
        words += [(0x500+4*n,x)for n,x in enumerate(items)]
        words += [(0x600+4*n,x)for n,x in enumerate(listeners)]
        words += [(0x900+4*n,x)for n,x in enumerate([U+0x400,O,U+0x400,U+0x400])]
        words += [(0xa00+4*n,x)for n,x in enumerate(reversed(listeners))]
        row=dict(name=name,entry=entry,receiver=P,arguments=[O,flag],event_packet=event,user_words=words,**extra)
        if owned:row.update(prepare_bytes=cap*4,prepare_header=header,prepare_data=D,prepare_stride=4,count=len(items))
        rows.append(row)
    for entry in(0x315b00,0x315300):
        for items in((),(O,),(U+0x400,O),(U+0x400,U+0x400)):
            for flag in(0,1,0xffffff00,0xffffff80):
                add(f'list_{len(rows)}',items,[U+0x700,0,U+0x720],flag,entry)
        add(f'empty_{entry:x}',entry=entry)
        for count in(0,1,4):
            add(f'growth_borrowed_{entry:x}_{count}',[U+0x400]*count,entry=entry,capacity=count)
        add(f'growth_owned_{entry:x}',[U+0x400]*2,entry=entry,capacity=2,owned=True)
    add('complex_other_shape_null',(O,),[U+0x700],shape=0)
    for flag in(0,1,0xff):
        for listeners in((),(U+0x700,),(U+0x700,0,U+0x720)):
            add(f'notify_{flag}_{len(listeners)}',listeners=listeners,flag=flag,entry=0x314f80)
    for entry in(0x315b00,0x315300,0x314f80):
        event=base.SP-(0x10 if entry==0x314f80 else 0x2c if entry==0x315300 else 0x28)
        header=P+(0x80 if entry==0x315300 else 0xc0)
        for label,address,value in(('decision',event+12,0xffffffff),('backing',header,U+0x900),
            ('count',header+4,3),('listener_backing',P+0x3c,U+0xa00),('listener_count_retained',P+0x40,1)):
            add(f'mutation_{entry:x}_{label}',[U+0x400]*4,[U+0x700,U+0x720,U+0x740],entry=entry,capacity=8,
                mutations=[dict(instruction=0x2e4d60,address=address,written=value.to_bytes(4,'little').hex())])
    faults=[('shape',O,4,1,False),('search_count',P+0xc4,4,1,False),('search_data',P+0xc0,4,1,False),
        ('search_slot',D,4,1,False),('listener_count',P+0x40,4,1,False),
        ('packet_other',base.SP-0x28+4,4,1,True),('packet_flag',base.SP-0x28+8,1,1,True),
        ('listener_data',P+0x3c,4,1,False),('listener_slot',L+8,4,1,False),('listener_owner',U+0x800,4,1,False)]
    for label,address,width,occurrence,writing in faults:
        add('fault_'+label,[U+0x400]*4,[U+0x700,0,U+0x720],flag=1,
            fault=dict(address=address,bytes=width,occurrence=occurrence,write=writing))
    for entry in(0x315b00,0x315300):
        h=P+(0xc0 if entry==0x315b00 else 0x80)
        for label,address,writing in(('capacity',h+8,False),('current_count',h+4,False),('count_store',h+4,True),('slot_store',D,True)):
            add(f'fault_{entry:x}_{label}',entry=entry,
                pre_instruction_rmw_write_fault=entry==0x315300 and label=='count_store',
                fault=dict(address=address,bytes=4,occurrence=1,write=writing))
    return rows

def driver():
    text=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()
    text='#include "halo2/native_tu15_phantom_overlap_add.hpp"\n#include "halo2/native_tu15_phantom_pair_delete.hpp"\n'+text
    # The physical probe injects this one write failure BEFORE INC[mem], so
    # its RMW read has not happened. Native SourcePhysicsContactMemory exposes
    # separate read/write accesses: keep the real reread, compare unchanged
    # persistent state/stores/FP at failure, and report the strict trace gap.
    # Do not invent an original read or make production ask the fixture whether
    # its next write will fail. Ordinary reads and the other faults stay strict.
    text=text.replace('if(read_pass&&!c.value("mutations",J::array()).empty())continue;',
        'if(read_pass&&(!c.value("mutations",J::array()).empty()||c.value("pre_instruction_rmw_write_fault",false)))continue;')
    anchor='      const auto host_sw=status(),host_cw=control();'
    assert text.count(anchor)==1
    setup=r'''      NativeTu15PhantomOverlapAddServices services;services.allocation=growth;
      services.added=[&](auto listener,auto owner,auto event,auto&){
        check(owner.identity==0x2e4d60u,"Unqualified native added listener");
        const auto first=memory.at(event.identity,9),decision=memory.at(event.identity+12u,4);
        calls.push_back(J{{"owner",owner.identity},{"receiver",listener},{"event",event.identity},
            {"data",hex(std::span(first->second).subspan(event.identity-first->first,9))},
            {"decision",hex(std::span(decision->second).subspan(event.identity+12u-decision->first,4))}});
        native_tu15_ignore_removed_phantom_overlap();
        unsigned n{};for(const auto&mutation:c.value("mutations",J::array())){
          if(mutation.at("instruction")==0x2e4d60u&&applied.insert(n).second){memory.write(mutation.at("address"),unhex(mutation.at("written")));++mutations;}++n;
        }
      };
'''
    text=text.replace(anchor,setup+anchor)
    start=text.index('      try{const auto entry=');end=text.index('      }catch(const Fault&)',start)
    text=text[:start]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(entry==0x315b00u)native_tu15_add_phantom_overlap(memory,c.at("receiver"),{args[0]},args[1],{c.at("event_packet")},fp,services,observe);
        else if(entry==0x315300u)native_tu15_add_simple_phantom_overlap(memory,c.at("receiver"),{args[0]},args[1],{c.at("event_packet")},fp,services,observe);
        else{check(entry==0x314f80u,"Unknown addition entry");result=native_tu15_notify_phantom_overlap_added(memory,c.at("receiver"),{args[0]},std::uint8_t(args[1]),{c.at("event_packet")},fp,services,observe);}
'''+text[end:]
    anchor='      if(!failed&&!required&&(c.at("entry")==0x233250u||c.at("entry")==0x149da0u))'
    assert text.count(anchor)==1;text=text.replace(anchor,'      if(!failed&&!required&&c.at("entry")==0x314f80u)')
    text=text.replace('std::size_t cases{},stores{},read_count{},faults{},mutations{};',
        'std::size_t cases{},stores{},read_count{},faults{},mutations{},event_lifetime_comparisons{},dead_event_raw_differences{};J dead_event_examples=J::array();')
    anchor='      for(const auto&bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),name+" complete array/pool bank differs");'
    assert text.count(anchor)==1
    text=text.replace(anchor,r'''      for(const auto&bank:c.at("banks")){
        const auto address=bank.at("address").get<std::uint32_t>(),event=c.at("event_packet").get<std::uint32_t>();
        auto expected=file(path.parent_path()/bank.at("after_file").get<std::string>());
        if(!c.at("event_at_notification_return").is_null()&&(address==event||address==event+12u)){
          const auto live=unhex(c.at("event_at_notification_return").at(address==event?"data":"decision"));
          if(live!=expected){++dead_event_raw_differences;if(dead_event_examples.size()<12)dead_event_examples.push_back(J{{"case",name},{"address",address},{"original_end_of_function",hex(expected)},{"original_end_of_notification",hex(live)}});}
          expected=live;++event_lifetime_comparisons;
        }
        check(memory.banks.at(address)==expected,name+" persistent state or live event fields differ at "+std::to_string(address));
      }''')
    text=text.replace('J{{"passed",true},{"cases",cases}',
        'J{{"passed",true},{"event_lifetime_comparisons",event_lifetime_comparisons},{"dead_event_raw_differences",dead_event_raw_differences},{"dead_event_examples",dead_event_examples},{"cases",cases}')
    return text

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args()
    a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    files=['include/halo2/native_tu15_phantom_overlap_add.hpp','src/physics/native_tu15_phantom_overlap_add.cpp',
        'include/halo2/native_tu15_phantom_pair_delete.hpp','src/physics/native_tu15_phantom_pair_delete.cpp',
        *base.FILES,'tools/validate_native_phantom_overlap_add_tu15.py']
    pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),
        actual_live_added_listener_claimed=False,allocator_outcomes_supplied=False,retained_large_artifacts=[])
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image)
        base.OWNERS=(0x315b00,0x315300,0x314f80,0x2e4d60,0x2e3250,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420)
        observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();needle='if(pc==stop)break;';assert observer.count(needle)==1
        record='''if(pc==0x2e4d60u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t event{};for(unsigned n=0;n<4;++n)event|=std::to_integer<unsigned>(raw[n])<<(8u*n);allocator_calls.push_back(J{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"event",event},{"data",hex(memory(child.pid,event,9))},{"decision",hex(memory(child.pid,event+12u,4))}});}'''
        observer=observer.replace(needle,record+needle).replace('pc==mutation.at("instruction")&&!pending.empty()',
            'pc==mutation.at("instruction")&&(!pending.empty()||pc==0x2e4d60u)')
        observer=observer.replace('J writes=J::array(),reads=J::array(),estimates=J::array(),allocator_calls=J::array();',
            'J writes=J::array(),reads=J::array(),estimates=J::array(),allocator_calls=J::array(),event_at_notification_return=nullptr;')
        completion='''if(pc==0x314fcfu){const auto event=m.at("event_packet").get<std::uint32_t>();event_at_notification_return=J{{"data",hex(memory(child.pid,event,9))},{"decision",hex(memory(child.pid,event+12u,4))}};}'''
        observer=observer.replace(needle,completion+needle)
        observer=observer.replace('{"initial_banks",before}',
            '{"event_at_notification_return",event_at_notification_return},{"initial_banks",before}')
        observer_source=scratch/'observer.cpp';observer_source.write_text(observer)
        source=inspect.getsource(base.original);needle="str(ROOT/'tools/observe_native_array_growth_tu15.cpp')";assert source.count(needle)==1
        source=source.replace(needle,'str(observer_source)')
        anchor='        if case.get(\'retained_arguments\'):';assert source.count(anchor)==1
        source=source.replace(anchor,"        active['regions']=[*m['regions'],dict(address=case['event_packet'],bytes=9),dict(address=case['event_packet']+12,bytes=4)];active['event_packet']=case['event_packet']\n"+anchor)
        source=source.replace('mov %eax,{HEADER}','mov %eax,{case["prepare_header"]}')
        source=source.replace('mov ${DATA},%esi','mov ${case.get("prepare_data",DATA)},%esi')
        source=source.replace('{case.get("count",0)*case["arguments"][1]}','{case.get("count",0)*case.get("prepare_stride",case["arguments"][1])}')
        environment=dict(base.__dict__,observer_source=observer_source);exec(source,environment)
        all_cases=cases();fixture,authority=environment['original'](image,banks,all_cases,scratch)
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for name in('native_tu15_phantom_overlap_add.cpp','native_tu15_phantom_pair_delete.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp'):
            shutil.copyfile(ROOT/'src/physics'/name,project/name)
        (project/'driver.cpp').write_text(driver())
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(overlap_add LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(overlap driver.cpp native_tu15_phantom_overlap_add.cpp native_tu15_phantom_pair_delete.cpp native_tu15_array_growth.cpp native_tu15_array_allocator.cpp)\ntarget_include_directories(overlap PRIVATE include)\ntarget_compile_features(overlap PRIVATE cxx_std_20)\ntarget_link_libraries(overlap PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME overlap_add COMMAND overlap "'+str(fixture/'original.json')+'")\n')
        commands=[];results=[]
        def run(label,compiler):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','overlap','--filter','^overlap_add$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
            commands.append(command);r=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
        for n,compiler in enumerate(a.compilers):assert run('compiler-'+str(n),compiler)==0,'Native overlap addition mismatch'
        target=project/'native_tu15_phantom_overlap_add.cpp';text=target.read_text()
        controls=[('duplicate','if(std::uint8_t(flag_word))','if(false)'),
            ('complex_store_order','m.word(0x315b8bu,header+4u,current_count+1u);\n        m.word(0x315b8eu,destination,other.identity);','m.word(0x315b8eu,destination,other.identity);\n        m.word(0x315b8bu,header+4u,current_count+1u);'),
            ('simple_store_order','m.word(0x315385u,destination,other.identity);\n        m.word(0x315388u,header+4u,m.word(header+4u)+1u);','m.word(0x315388u,header+4u,m.word(header+4u)+1u);\n        m.word(0x315385u,destination,other.identity);'),
            ('listener_reverse','--index;','index=0xffffffffu;'),('decision','return m.word(event.identity+0xcu);','return 0u;'),
            ('growth','native_tu15_grow_array(storage,4u,fp,services.allocation);','(void)storage;')]
        for label,before,after in controls:
            assert text.count(before)==1,(label,text.count(before));target.write_text(text.replace(before,after));assert run('negative-'+label,a.compilers[0])!=0,label+' was not rejected';target.write_text(text)
        code=_Code(image,'.text');executed={p for c in all_cases for p in c['executed_sites']}
        report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),original_xbe_sha256=NEW_PIN,authority=authority,input_provenance=provenance,
            cases=len(all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),
            actual_allocator_and_listener_entries=sum(len(c['allocator_calls'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),
            declared_input_mutations=sum(c['mutations_applied']for c in all_cases),commands=commands,results=results,negative_controls=[r[0]for r in controls],
            body_coverage={hex(p):dict(total=len(complete_body(code,p).code),executed=len(set(complete_body(code,p).code)&executed))for p in base.OWNERS},
            scope=__doc__,source_event_fields_compared=[0,4,8,12],uninitialized_event_padding_not_supplied=[9,10,11],
            event_final_comparison_boundary='End of original314F80 at314FCF, before the parent can reuse its dead call-stack bytes',
            original_private_event_snapshots=sum(c['event_at_notification_return']is not None for c in all_cases),
            strict_synthetic_rmw_write_fault_read_trace_matched=False,
            synthetic_rmw_fault_scope={'case':'fault_315300_count_store','original_instruction':0x315388,
                'native_extra_read_address':base.UI+0x184,'bytes':4,
                'reason':'Physical fixture stops before the entire original INC[mem]; native memory interface faults its write after the required read',
                'stores_persistent_state_and_FP_compared':True,
                'strict_failure_report':str(a.output.parent/'overlap-component-v3/compiler-0/tests.log')})
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:save(a.output/'report.json',report)
    print(json.dumps({k:report[k]for k in('passed','cases','original_instructions','ordered_reads','ordered_stores','fault_prefixes')}),flush=True)

if __name__=='__main__':main()
