#include "halo2/source_runtime_sound_ac97_page_release.hpp"
#include "halo2/source_runtime_sound_pcm_buffers.hpp"
#include "halo2/source_sound_ac97_page_release_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){const auto bytes=m.view(p,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[i])<<(i*8);return value;}
}
void mount_native_tu15_sound_ac97_page_release_data(SourceRuntimeMemory& memory){
    constexpr std::array raw{std::byte{0xab},std::byte{},std::byte{},std::byte{0x80}};
    bool present=false;memory.visit_committed([&](auto p,auto bytes){if(std::uint64_t(p)<0x41b610&&std::uint64_t(p)+bytes.size()>0x41b60c)present=true;});
    if(present){if(!memory.is_read_only(0x41b60c,4)||!std::ranges::equal(memory.view(0x41b60c,4),raw))throw std::invalid_argument("Native AC97 physical-free import identity differs");return;}
    memory.mount(0x41b60c,raw,SourceRuntimeMemory::Access::ReadOnly,"exact TU1.5 MmFreeContiguousMemory import identity");
}
SourceRuntimeSoundAc97PageRelease::SourceRuntimeSoundAc97PageRelease(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages,std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> channels,
    SourceRuntimeSoundResetResources::ControlLease control)
    :memory_(memory),pages_(std::move(pages)),channels_(std::move(channels)),control_(std::move(control)),generation_(control_.generation()){
    require_live();
}
void SourceRuntimeSoundAc97PageRelease::require_thread()const{
    if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native AC97 page retirement requires its creating owner thread");
}
void SourceRuntimeSoundAc97PageRelease::require_live()const{
    require_thread();if(fault_||!pages_||!channels_||!generation_||control_.generation()!=generation_)
        throw std::runtime_error("Native AC97 page retirement lost its actual owner generation");
    control_.require_bank(memory_);pages_->require_control(memory_,control_);(void)pages_->cpu_memory();channels_->require_control(memory_,control_);
}
bool SourceRuntimeSoundAc97PageRelease::dispatch(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi){
    const bool capacity=call.instruction==0x387a1e,release=call.instruction==0x387a2c;
    if(!capacity&&!release)return false;
    require_thread();if(&bank!=&memory_)throw std::invalid_argument("Native AC97 page retirement uses a foreign bank");require_live();
    const auto sp=abi.gp[4],source=word(memory_,sp+4);
    if(capacity){
        const auto parent=word(memory_,sp+8);const auto field=parent==0x38f1d0?0x18u:parent==0x38f1e1?0x20u:0;
        if(call.tail||call.target!=0x800000b4||word(memory_,0x41b644)!=call.target||word(memory_,sp)!=0x387a24||
            !field||pending_||word(memory_,sp+12)!=source)
            throw std::invalid_argument("Native AC97 release-capacity frame/import differs");
        const auto rows=pages_->diagnostics();const auto at=std::ranges::find_if(rows.allocations,[&](const auto& row){return row.source==source&&row.channel==abi.gp[6]&&row.field==field;});
        if(at==rows.allocations.end()||rows.source_exclusion_level!=2||word(memory_,at->channel+field)!=source||
            memory_.native_address(source,at->capacity)!=at->native||source_runtime_sound_pcm_descriptor_bound(at->native))
            throw std::invalid_argument("Native AC97 free lacks its exact unborrowed physical page");
        statistics_=word(memory_,word(memory_,0x3912a0));if(statistics_<at->capacity)
            throw std::invalid_argument("Native AC97 source physical accounting cannot release this actual capacity");
        released_.reserve(released_.size()+1);channels_->prepare_page_release(*at);pending_=*at;caller_sp_=sp+8;parent_=parent;
        abi.gp[0]=at->capacity;abi.gp[4]+=8;return true;
    }
    if(!call.tail||call.target!=0x800000ab||word(memory_,0x41b60c)!=call.target||!pending_||sp!=caller_sp_||
        word(memory_,sp)!=parent_||source!=pending_->source||abi.gp[6]!=pending_->channel||abi.gp[0]!=pending_->capacity||
        word(memory_,word(memory_,0x3912a0))!=statistics_-pending_->capacity||source_runtime_sound_pcm_descriptor_bound(pending_->native))
        throw std::invalid_argument("Native AC97 physical-free tail lost its authentic source/accounting frame");
    const auto row=*pending_;pages_->release_owned_page(row);released_.push_back(row);pending_.reset();abi.gp[4]+=8;return true;
}
void SourceRuntimeSoundAc97PageRelease::run_retirement(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer){
    require_live();if(running_||pending_)throw std::runtime_error("Native AC97 page retirement cannot reenter its source transaction");
    const auto controller=pages_->diagnostics().controller,index=word(memory_,abi.gp[4]+4);
    if(abi.gp[1]!=controller||index>=2)throw std::invalid_argument("Native AC97 retirement caller lacks its genuine source controller/index");
    const auto channel=word(memory_,controller+index*4);const auto rows=pages_->diagnostics().allocations;
    for(const auto& row:rows)if(row.channel==channel&&source_runtime_sound_pcm_descriptor_bound(row.native))
        throw std::runtime_error("Native PCM buffer binding must retire before source AC97 teardown");
    auto scoped=services;scoped.required_owner=[&](auto& bank,const auto& call,auto& cpu,const auto& record){
        if(dispatch(bank,call,cpu))return;
        if(source_sound_ac97_page_release_tu15_owns_entry(call.target)){source_run_sound_ac97_page_release_tu15(memory_,call.target,cpu,fp,scoped,record);return;}
        if(services.required_owner){services.required_owner(bank,call,cpu,record);return;}
        throw std::runtime_error("Unowned native resource remains required during AC97 physical-page teardown");
    };
    running_=true;try{channels_->run_cpu(0x38f395,abi,fp,scoped,observer);running_=false;
        if(pending_||word(memory_,controller+index*4)||std::ranges::any_of(pages_->diagnostics().allocations,[&](const auto& row){return row.channel==channel;}))
            throw std::runtime_error("Original AC97 teardown did not finish its owned page/channel cleanup");
    }catch(...){running_=false;fault_=true;control_.abort_transaction();throw;}
}
SourceRuntimeSoundAc97PageRelease::State SourceRuntimeSoundAc97PageRelease::diagnostics()const{require_thread();return {released_,fault_,false,false};}
}
