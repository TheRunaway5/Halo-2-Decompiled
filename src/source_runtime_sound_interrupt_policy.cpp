#include "halo2/source_runtime_sound_interrupt_policy.hpp"
#include "halo2/source_sound_reset_tu15.hpp"
#include "source_runtime_sound_interrupt_gate.hpp"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
struct SourceRuntimeSoundInterruptPolicy::CpuMemory final:SourcePhysicsContactMemory {
    SourceRuntimeSoundInterruptPolicy& owner;
    explicit CpuMemory(SourceRuntimeSoundInterruptPolicy& value):owner(value){}
    void read(std::uint32_t address,std::span<std::byte> out)const override{
        owner.require_live();
        if(address==0xfe801004&&out.size()==4&&(owner.pc_==0x38e54e||owner.pc_==0x38e562)){
            const auto value=owner.gate_->pending_mask;for(unsigned i=0;i<4;++i)out[i]=std::byte(value>>(i*8));return;
        }
        if(address>=0xfe800000)throw std::runtime_error("Unowned source audio policy read remains required");
        // The native serialized source executor has an actual ControlLease
        // exclusion depth; the bank's passive FS byte remains untouched.
        if(owner.running_&&address==owner.cpu_fs_+0x24&&out.size()==1){out[0]=std::byte(owner.source_level_);return;}
        owner.memory_.read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes)override{
        owner.require_live();
        if(address==0xfe801004&&bytes.size()==4){
            std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);
            if(owner.pc_==0x38e562&&owner.gate_->prepared&&value==0xd8)return;
            throw std::runtime_error("Native source interrupt enable requires the unresolved actual D8 routes before delivery");
        }
        if(address>=0xfe800000)throw std::runtime_error("Unowned source audio policy write remains required");
        owner.memory_.write(address,bytes);
    }
};
SourceRuntimeSoundInterruptPolicy::SourceRuntimeSoundInterruptPolicy(SourceRuntimeMemory& memory,
    SourceRuntimeSoundEventPort& port,SourceRuntimeSoundResetResources::ControlLease control):memory_(memory){
    // Local allocation precedes dispatcher claim publication. Allocation
    // failure cannot strand an unowned gate on the actual retained port.
    auto cpu=std::make_unique<CpuMemory>(*this);
    gate_=port.attach_disabled_source_policy(memory,control);cpu_=std::move(cpu);
}
SourceRuntimeSoundInterruptPolicy::~SourceRuntimeSoundInterruptPolicy(){
    if(std::this_thread::get_id()!=thread_)std::terminate();retire();
}
void SourceRuntimeSoundInterruptPolicy::require_thread()const{
    if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native source interrupt policy requires its creating owner thread");
}
void SourceRuntimeSoundInterruptPolicy::require_live()const{require_thread();gate_->require_live();}
void SourceRuntimeSoundInterruptPolicy::apply_disabled_source_store(std::uint32_t instruction,std::uint32_t address,std::uint32_t value){
    require_live();if(instruction!=0x38e8c9||address!=0xfe801004||value!=0xd8||gate_->prepared)
        throw std::invalid_argument("Native disabled source interrupt request differs from exact38E8C9 IEN D8");
    gate_->pending_mask=value;gate_->prepared=true;
}
std::uint32_t SourceRuntimeSoundInterruptPolicy::run_initial_prefix(SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_live();if(running_||gate_->prepared||abi.gp[1]!=gate_->control.source_apu()+8)
        throw std::invalid_argument("Native disabled interrupt prefix lost its genuine source APU/frame");
    (void)fp;
    // Authenticated original c705041080fed8000000: one MOV action store,
    // with no register/flags change and no hardware preimage read. The empty
    // observer.before explicitly records that original preimage as unknown.
    running_=true;try{
        pc_=0x38e8c9;if(services.instruction)services.instruction(pc_);
        apply_disabled_source_store(pc_,0xfe801004,0xd8);
        if(observe)observe({pc_,0xfe801004,{},std::vector<std::byte>{std::byte{0xd8},std::byte{},std::byte{},std::byte{}}});
        running_=false;return 0x38e8d3;
    }
    catch(...){running_=false;abort();throw;}
}
void SourceRuntimeSoundInterruptPolicy::source_global_helper(SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_live();if(running_||!gate_->prepared||abi.gp[1]!=gate_->control.source_apu()+8||source_level_)
        throw std::invalid_argument("Native source interrupt helper lacks its actual disabled prepared policy");
    cpu_fs_=abi.fs_base;auto scoped=services;scoped.instruction=[&](auto pc){pc_=pc;if(services.instruction)services.instruction(pc);};
    scoped.required_owner=[&](auto&,const auto& call,auto& state,const auto&){
        if(gate_->control.try_exclusion(memory_,call,state)){source_level_=call.instruction==0x383d88?2:0;return;}
        throw std::runtime_error("Unowned source interrupt helper platform resource remains required");
    };
    running_=true;try{source_run_sound_reset_tu15(*cpu_,0x38e53d,abi,fp,scoped,observe);running_=false;}
    catch(...){running_=false;abort();throw;}
}
SourceRuntimeSoundInterruptPolicy::State SourceRuntimeSoundInterruptPolicy::diagnostics()const{
    require_thread();return {gate_->generation,NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(gate_.get())},gate_->dispatcher,
        gate_->pending_mask,gate_->pending_mask&0xc8,gate_->prepared,false,gate_->retired,gate_->faulted,false};
}
void SourceRuntimeSoundInterruptPolicy::abort(){gate_->faulted=true;gate_->control.abort_transaction();source_level_=0;}
void SourceRuntimeSoundInterruptPolicy::retire(){require_thread();if(gate_)gate_->retired=true;}
}
