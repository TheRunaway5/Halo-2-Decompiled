#!/usr/bin/env python3
"""Compare native phantom ownership with genuine source unwind action bodies.

At each declared input fault the unchanged original prefix supplies its live
registration chain and state. Execute the FuncInfo-selected original cleanup
funclets with that frame, then compare native C++ unwinding against every
retained bank/read/store/request/FP effect. Xbox exception dispatch is neither
recreated nor emulated; only resource-cleanup semantics are qualified here.
"""
import inspect
import validate_native_phantom_callback_agent_tu15 as factory

enter=factory.enter
leave=factory.leave
base=factory.base


def cases():
    rows=factory.cases();template=rows[0];U=leave.U
    for entry in (0x2f78c0,0x2f7820):
        for name,offset in [('before_base_arm',0x310c),('second_parent',0x320c),
                            ('first_shape',0x3100),('first_table',0x3000),
                            ('first_class_owner',0x3314),('second_shape',0x3200),
                            ('second_table',0x3040),('second_class_owner',0x3354),
                            ('enter_after_cutoff',0x234)]:
            if entry==0x2f7820 and name=='enter_after_cutoff':continue
            row=dict(template,name=f'unwind_{entry:x}_{name}',entry=entry,
                     arguments=[*template['arguments']][:3]if entry==0x2f7820 else[*template['arguments']],
                     compose_unwind=True,
                     fault=dict(address=U+offset,bytes=4,occurrence=1,write=False))
            # The constructor has three inputs; its third is user_data.
            if entry==0x2f7820:row['arguments'][2]=template['arguments'][3]
            rows.append(row)
    return rows


def observer_transform(text):
    old='std::set<std::uint32_t> executed;'
    assert text.count(old)==1
    extra=r'''
  struct Action{std::uint32_t entry,frame;};std::vector<Action> actions;
  std::size_t action_cursor{};bool composing_unwind{};J source_actions=J::array();
  std::uint64_t physical_steps{};
  const auto source_word=[&](std::uint32_t p){const auto b=memory(child.pid,p,4);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;};
  const auto start_action=[&]{
    const auto action=actions.at(action_cursor++);const std::uintptr_t sp=0x2007ff0u;
    auto call=regs;call.rip=action.entry;call.rbp=action.frame;call.rsp=sp;
    const auto old_word=std::uint64_t(request(PTRACE_PEEKDATA,child.pid,reinterpret_cast<void*>(sp)));
    const auto word=(old_word&0xffffffff00000000ull)|std::uint32_t(stop);
    request(PTRACE_POKEDATA,child.pid,reinterpret_cast<void*>(sp),reinterpret_cast<void*>(word));
    request(PTRACE_SETREGS,child.pid,nullptr,&call);
  };
  const auto select_actions=[&]{
    auto node=source_word(0x71000000u);std::set<std::uint32_t>seen;
    while(node){
      check(seen.insert(node).second&&seen.size()<=8,"Unqualified source registration chain");
      const auto previous=source_word(node),handler=source_word(node+4u);
      auto state=std::int32_t(source_word(node+8u));
      const auto key=std::to_string(handler);
      check(m.at("unwind_authority").contains(key),"Unqualified source phantom handler");
      const auto&map=m.at("unwind_authority").at(key);
      std::set<std::int32_t>states;
      while(state>=0){
        check(states.insert(state).second&&std::size_t(state)<map.size(),"Unqualified source unwind state");
        const auto&step=map.at(state);const auto entry=step.at("entry").get<std::uint32_t>();
        source_actions.push_back(J{{"handler",handler},{"state",state},{"entry",entry},{"frame",node+12u}});
        state=step.at("next");if(entry)actions.push_back({entry,node+12u});
      }
      check(state==-1,"Unqualified source final unwind state");node=previous;
    }
  };
'''
    text=text.replace(old,old+extra)
    old='if(pc==stop)break;'
    assert text.count(old)==1
    text=text.replace(old,'if(pc==stop){if(composing_unwind&&action_cursor<actions.size()){start_action();continue;}break;}')
    old='if(m.contains("fault")&&!repeated_empty)'
    assert text.count(old)==1
    text=text.replace(old,'if(m.contains("fault")&&!repeated_empty&&!composing_unwind)')
    old='if(declared_fault)break;'
    assert text.count(old)==1
    text=text.replace(old,'''if(declared_fault&&!composing_unwind){
      if(m.value("compose_unwind",false)){select_actions();
        if(!actions.empty()){composing_unwind=true;start_action();continue;}}
      break;
    }''')
    old='request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original array instruction fault at PC="+std::to_string(pc)+" status="+std::to_string(status));'
    assert text.count(old)==1
    text=text.replace(old,old+'++physical_steps;')
    old='check(declared_fault||required_owner||std::uint32_t(regs.rip)==stop,"Original array stop not reached");'
    assert text.count(old)==1
    text=text.replace(old,old+'check(!composing_unwind||(action_cursor==actions.size()&&std::uint32_t(regs.rip)==stop),"Source cleanup actions did not return");')
    old='{"original_instructions",count}'
    assert text.count(old)==1
    return text.replace(old,'{"original_instructions",physical_steps},{"source_unwind_actions",source_actions}')


def original(image,banks,rows,scratch):
    authority={}
    for handler in (0x37e908,0x37e92b):
        ins=base._Code(image,'.text').at(handler)
        assert ins.mnemonic=='mov'and ins.op_str.startswith('eax, ')
        info=int(ins.op_str.split(', ')[1],0)
        word=lambda p:int.from_bytes(image.read(p,4),'little')
        assert word(info)==0x19930520
        count=word(info+4);table=word(info+8);assert count==1
        authority[str(handler)]=[dict(next=int.from_bytes(image.read(table+n*8,4),'little',signed=True),
                                    entry=word(table+n*8+4))for n in range(count)]
    def source_transform(text):
        old="m,source=metadata(image,{**banks,UI:bytes(0x10000)},any(c.get('full_abi')for c in all_cases));"
        assert text.count(old)==1
        text=text.replace(old,old+"m['sites']=list({r['instruction']:r for r in m['sites']}.values());m['unwind_authority']="+repr(authority)+';')
        old="('fault','mutations') if k in case"
        assert text.count(old)==1
        return text.replace(old,"('fault','mutations','compose_unwind') if k in case")
    return factory.original(image,banks,rows,scratch,source_transform=source_transform,
                            observer_transform=observer_transform)


def transform(source):
    source=factory.transform(source)
    old="        executed={pc for c in rows for pc in c['executed_sites']}"
    assert source.count(old)==1
    extra='''        unwind_target=project/'native_tu15_phantom_callback_agent.cpp';unwind_text=unwind_target.read_text()
        before="    const auto first_parent=m.word(first+12u);\\n    native_contact_detail::UnwindGuard base([&]{m.word(0x2f7a10u,object,0x45edf0u);});\\n    base.arm();"
        after="    native_contact_detail::UnwindGuard base([&]{m.word(0x2f7a10u,object,0x45edf0u);});\\n    base.arm();\\n    const auto first_parent=m.word(first+12u);"
        unwind_mutants=[('unwind_early_base_arm',before,after),
            ('unwind_missing_base','base.arm(); // Actual state0 at2F7854, FuncInfo46BA90 ->2F7A10.',''),
            ('unwind_missing_allocation','allocation.arm(); // Actual state0 at2F7904; FuncInfo46BAB4 ->2F0380.','')]
        for name,before,after in unwind_mutants:
            assert unwind_text.count(before)==1,(name,unwind_text.count(before))
            unwind_target.write_text(unwind_text.replace(before,after))
            assert run('negative-'+name,a.compilers[0])!=0,'Source cleanup did not reject '+name
            unwind_target.write_text(unwind_text)
        mutants+=unwind_mutants
'''
    source=source.replace(old,extra+old)
    old="scope='Native phantom2F78C0/2F7820normal returns and first-enter fault after allocation-unwind cutoff; full composed enter/leave/real-pool regressions. Original TIB ABI setup executes unchanged. Pre-cutoff source exception dispatch/unwind and live delivery remain unqualified.'"
    assert source.count(old)==1
    return source.replace(old,"scope='Native phantom constructor/factory normal and resource-cleanup semantics match original prefixes plus FuncInfo actions selected from live source registration states. Original funclets/pool instructions unchanged; native C++ cleanup only, no Xbox FS or exception dispatcher emulation. Full enter/leave regressions retained; live/termination/platform exception behavior not claimed.'")


if __name__=='__main__':
    raise SystemExit(enter.main(additional_cases=cases(),
        additional_owners=(0x2f78c0,0x2f7820,0x308540,0xa9520,0x37e900,0x37e920,0x2f7a10,0x2f0380),
        additional_pins=('include/halo2/native_tu15_phantom_callback_agent.hpp',
            'src/physics/native_tu15_phantom_callback_agent.cpp','src/physics/native_tu15_contact_agent_factory.cpp',
            'src/physics/native_tu15_contact_unwind_internal.hpp',
            'tools/validate_native_phantom_callback_agent_tu15.py','tools/validate_native_phantom_callback_unwind_tu15.py'),
        additional_sources=('native_tu15_phantom_callback_agent.cpp',
            'native_tu15_contact_agent_factory.cpp','native_tu15_contact_unwind_internal.hpp'),
        driver_transform=factory.driver,original_override=original,source_transform=transform))
