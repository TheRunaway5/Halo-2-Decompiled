#!/usr/bin/env python3
"""Physical lift velocity proof with exact counter transaction fault ordering."""
import inspect

import validate_native_phantom_body_velocity_tu15 as base

ROOT = base.ROOT
STAGE = ROOT / 'analysis/physics/native-phantom-body-velocity-stage-v2-20261103/staged'


def change(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def driver():
    text = base.driver()
    text = '#include "halo2/source_runtime_memory.hpp"\n#include "halo2/source_runtime_write_protection.hpp"\n' + text
    text = change(text, 'int main(int argc,char**argv)', r'''
struct CounterMemory final : Memory, NativeTu15RetainedCounterStorage {
 NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t p,unsigned width,std::uint32_t adjustment)override{
  check(width==2||width==4,"Unqualified counter width");
  if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&width==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};
  const auto target=at(p,width);
  std::array<std::byte,4> before{};read(p,std::span(before).first(width));
  const auto result=native_tu15_adjust_counter_value(before,width,adjustment);
  std::copy_n(result.written.begin(),width,banks.at(target->first).begin()+p-target->first);
  return result;
 }
};

void runtime_controls(){
 constexpr std::uint32_t p=0x100000u;
 SourceRuntimeMemory memory;
 const std::array<std::byte,2> lo{std::byte{0xff},std::byte{0xff}},hi{std::byte{0xff},std::byte{0xff}};
 memory.mount(p,lo,SourceRuntimeMemory::Access::ReadWrite,"counter low");
 memory.mount(p+2,hi,SourceRuntimeMemory::Access::ReadWrite,"counter high");
 unsigned guard{},audit{},events{};bool refuse{};
 source_runtime_register_write_guard(memory,&guard,[&](auto address,auto width){check(address==p&&width==4,"Counter guard extent");++guard;if(refuse)throw Fault{};});
 source_runtime_register_write_observer(memory,&audit,[&](auto,auto){++audit;});
 const SourcePhysicsContactRecordObserver observed=[&](const auto&e){++events;check(e.instruction==0x316d6eu&&e.address==p&&hex(e.before)=="ffffffff"&&hex(e.written)=="00000000","Counter committed event");std::array<std::byte,4>b{};memory.read(p,b);check(hex(b)=="00000000","Event precedes committed bytes");};
 native_tu15_adjust_retained_counter(memory,0x316d6eu,p,4,1,observed);
 check(guard==1&&audit==1&&events==1,"Counter write policy repeated");
 refuse=true;bool caught{};try{native_tu15_adjust_retained_counter(memory,0x316d6eu,p,4,1,observed);}catch(const Fault&){caught=true;}
 std::array<std::byte,4> bytes{};memory.read(p,bytes);check(caught&&hex(bytes)=="00000000"&&guard==2&&audit==1&&events==1,"Rejected cross-region counter changed DATA or audit");
 source_runtime_unregister_write_observer(memory,&audit);source_runtime_unregister_write_guard(memory,&guard);
 const auto word=memory.adjust_counter(p+1,2,0xffffffffu);check(hex(std::span(word.before).first(2))=="0000"&&hex(std::span(word.written).first(2))=="ffff","Unaligned word counter wrap");
 memory.read(p,bytes);check(hex(bytes)=="00ffff00","Word counter touched adjacent bytes");
 SourceRuntimeMemory readonly;readonly.mount(p,lo,SourceRuntimeMemory::Access::ReadWrite,"writable low");readonly.mount(p+2,hi,SourceRuntimeMemory::Access::ReadOnly,"readonly high");
 caught=false;try{readonly.adjust_counter(p,4,1);}catch(const std::runtime_error&){caught=true;}readonly.read(p,bytes);check(caught&&hex(bytes)=="ffffffff","Read-only cross-region counter changed DATA");
 Memory unavailable; caught=false;try{native_tu15_adjust_retained_counter(unavailable,0x316d6eu,p,4,1,{});}catch(const std::runtime_error&e){caught=std::string(e.what())=="Native counter update requires checked read-modify-write storage";}check(caught,"Unknown backing incorrectly admitted counter operation");
}

int main(int argc,char**argv)''')
    text = change(text, '  Memory memory;for', '  CounterMemory memory;for')
    return change(text, ' return 0;\n}catch', ' if(!mutant)runtime_controls();\n return 0;\n}catch')


def main():
    text = inspect.getsource(base.main)
    text = change(text, "activation=ROOT/'src/physics/native_tu15_island_activation.cpp'",
                  "activation=STAGE/'src/physics/native_tu15_island_activation.cpp';runtime=STAGE/'src/runtime/source_runtime_memory.cpp'")
    text = change(text, '(source,activation,STAGE/', '(source,activation,runtime,STAGE/\'src/runtime/source_runtime_write_protection.inc\',STAGE/\'include/halo2/source_runtime_memory.hpp\',STAGE/\'include/halo2/native_tu15_retained_counter_update.hpp\',ROOT/\'tools/validate_native_phantom_body_velocity_tu15.py\',STAGE/')
    text = change(text, "str(unit),str(activation),'-o'", "str(unit),str(activation),str(runtime),'-o'")
    text = change(text, "'-I'+str(STAGE/'include')", "'-I'+str(STAGE/'include'),'-I'+str(STAGE/'src')")
    text = change(text, "'-frounding-math'", "'-frounding-math','-ffunction-sections','-fdata-sections','-Wl,--gc-sections'")
    text = change(text, 'report.update(passed=True,', 'report.update(passed=True,runtime_counter_controls=5,')
    env = dict(base.__dict__, STAGE=STAGE, driver=driver, __file__=__file__, __doc__=__doc__)
    exec(text,env)
    env['main']()


if __name__=='__main__':main()
