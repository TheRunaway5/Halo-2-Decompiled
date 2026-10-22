#include "halo2/source_runtime_sound_frontend_events.hpp"
#include "halo2/source_sound_frontend_events_tu15.hpp"
#include "halo2/source_sound_frontend_dpc_tu15.hpp"
#include "halo2/source_sound_pending_aggregate_tu15.hpp"
#include <cstring>
#include <mutex>
#include <algorithm>
#include <array>
#include <deque>
#include <map>
#include <stdexcept>
#include <thread>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p,unsigned n=4) {
    const auto b=m.view(p,n);std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;
}
}
struct SourceRuntimeSoundFrontendEvents::Impl {
    SourceRuntimeMemory& memory;
    SourceRuntimeSoundResetResources::ControlLease control;
    SourceRuntimeSoundDmaResources& dma;
    const std::thread::id thread=std::this_thread::get_id();
    const std::uint64_t generation;
    const std::uint32_t apu;
    const std::size_t capacity;
    struct Binding {std::uint32_t channel;std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease> voice;};
    std::map<std::uint16_t,Binding> voices;
    std::deque<std::uint16_t> commands;
    std::deque<std::shared_ptr<DeferredCall>> dpcs;
    struct DpcRegistration {std::uint32_t source,routine,context;std::uint64_t generation;};
    std::shared_ptr<const DpcRegistration> registered_dpc,timed_dpc;
    std::vector<std::uint16_t> cancelled_commands;
    std::vector<std::shared_ptr<DeferredCall>> cancelled_dpcs;
    std::optional<std::uint16_t> trapped;
    std::uint64_t next_dpc=0x100000000ull;
    Mode mode=Mode::Halted;
    bool idle_enabled{},requested_reason{},fault{},retired{},executing{},executing_dpc{};
    unsigned exclusion_depth{};
    std::recursive_mutex event_domain;
    std::optional<SourceRuntimeSoundDmaResources::Allocation> fence_target;
    std::uint64_t completed_dpcs{},completed_fences{};
    std::uint32_t pending_status{},pc{},memory_fs{};
    Impl(SourceRuntimeMemory& m,SourceRuntimeSoundResetResources& r,SourceRuntimeSoundDmaResources& d,std::size_t n)
        :memory(m),control(r.control_lease()),dma(d),generation(control.generation()),apu(control.source_apu()),capacity(n) {
        control.require_bank(m);
        if(!generation||!apu||!capacity||capacity>4096)throw std::invalid_argument("Native frontend needs a real reset generation and bounded command capacity");
        if(m.native_address(apu,0x540)!=r.state().native_apu)
            throw std::invalid_argument("Native frontend APU is not the authenticated actual reset backing");
        if(dma.device_id()!=r.state().output_device)throw std::invalid_argument("Native frontend DMA and control workers retain different actual outputs");
    }
    void require_thread()const {
        if(std::this_thread::get_id()!=thread)throw std::runtime_error("Native frontend is on a foreign owner thread");
    }
    void require_live()const {
        require_thread();if(fault||retired||control.generation()!=generation)
            throw std::runtime_error("Native frontend generation has retired or faulted");
        if(control.source_apu()!=apu)throw std::runtime_error("Native frontend lost its retained source APU");
    }
    SourceRuntimeSoundDmaResources::Allocation allocation(std::uint32_t descriptor)const {
        const auto rows=dma.allocations();const auto found=std::ranges::find_if(rows,[&](const auto& r){return r.source_descriptor==descriptor;});
        if(found==rows.end()||word(memory,descriptor)!=found->source||word(memory,descriptor+4)!=found->physical||
            word(memory,descriptor+8)!=found->requested||memory.native_address(found->source,found->capacity)!=found->native||
            memory.source_address(found->native,found->capacity)!=found->source)
            throw std::runtime_error("Native frontend source table lacks its actual DMA resource");
        return *found;
    }
    const Binding& binding(std::uint16_t index)const {
        const auto at=voices.find(index);if(at==voices.end())throw std::invalid_argument("Native frontend voice is unowned");
        const auto& b=at->second;const auto channel=b.channel;
        if(control.voice(index)!=b.voice||word(memory,apu+0x88+index*4)!=channel||word(memory,channel+8)!=apu||
            word(memory,channel+0x64,1)!=1||word(memory,channel+0xc,2)!=index||word(memory,channel+0x65,1)!=0)
            throw std::runtime_error("Native frontend source voice slot/channel binding differs");
        const auto records=allocation(0x391390),notifiers=allocation(0x3913a0);
        if(records.capacity<0x8000||notifiers.capacity<0x4020||
            word(memory,channel+0x68)!=notifiers.source+32+index*64)
            throw std::runtime_error("Native frontend voice/notifier extent or original mapping differs");
        const auto head=apu+0x488,entry=channel+0x4c;auto current=word(memory,head);bool listed=false;
        for(unsigned n=0;current!=head;++n) {
            if(n>=256||!current)throw std::runtime_error("Native frontend source active list is malformed");
            const auto next=word(memory,current),previous=word(memory,current+4);
            if(word(memory,next+4)!=current||word(memory,previous)!=current)
                throw std::runtime_error("Native frontend source active list has inconsistent links");
            listed|=current==entry;current=next;
        }
        if(!listed)throw std::runtime_error("Native frontend voice has no genuine source active-list membership");
        return b;
    }
    std::uint32_t status()const{return pending_status?pending_status|1u:0u;}
    std::uint32_t frontend()const {
        return (requested_reason?0xf00u:0u)|(mode==Mode::Halted?0x80u:mode==Mode::Trapped?0xe0u:0u);
    }
    std::uint32_t read_control(std::uint32_t p)const {
        // Generated store observers ask for a preimage even for write-only
        // frontend commands. Supply only the actual typed target/page state
        // at these exact store sites; no source load or ready poll is admitted.
        if(executing_dpc&&pc==0x388941&&p==0xfe801324)return fence_target?fence_target->physical:0;
        if(executing_dpc&&pc==0x38894f&&p==0xfe801334)return word(memory,allocation(0x391380).source);
        switch(p) {
        case 0xfe801000:return status();
        case 0xfe801100:return frontend();
        case 0xfe801510:
            if(executing_dpc)return control.frontend_privileged_access()?1:0;
            break;
        case 0xfe801300:if(trapped)return 0x8000;break;
        case 0xfe801304:if(trapped)return *trapped;break;
        default:
            if(p>=0xfe802054&&p<=0xfe802074&&!(p&3))return control.voice_head((p-0xfe802054)/4);
        }
        throw std::runtime_error("Unowned native frontend control read at source site "+std::to_string(pc)+" address "+std::to_string(p)+"; no register image or FIFO-ready value exists");
    }
    void write_control(std::uint32_t address,std::uint32_t value) {
        if(executing_dpc&&pc==0x38e599&&address==0xfe801510) {
            if(value>1)throw std::invalid_argument("Unknown frontend privileged-access bits remain unowned");
            control.frontend_privileged_access(generation,bool(value));return;
        }
        if(executing_dpc&&pc==0x388941&&address==0xfe801324) {
            if(!control.frontend_privileged_access()||fence_target)throw std::runtime_error("Native frontend fence has no exclusive admitted transfer");
            const auto row=allocation(0x391380);
            if(row.requested!=4||row.capacity<4||row.physical!=value)
                throw std::invalid_argument("Native frontend fence target differs from its actual DMA descriptor");
            memory.validate_mutable_view(row.source,4);fence_target=row;return;
        }
        if(executing_dpc&&pc==0x38894f&&address==0xfe801334) {
            if(!control.frontend_privileged_access()||!fence_target||value!=word(memory,0x3911e4))
                throw std::runtime_error("Native frontend fence write lost its actual ordered target/counter");
            const auto row=allocation(0x391380);
            if(row.native!=fence_target->native||row.source!=fence_target->source||row.physical!=fence_target->physical)
                throw std::runtime_error("Native frontend fence backing changed during its transaction");
            auto bytes=memory.mutable_view(row.source,4);
            // FEMEMDATA is completion of preceding frontend writes. The actual
            // worker serializes the transfer after all previously admitted
            // native control jobs; source polling reads this same real page.
            control.run(generation,[&]{for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));});
            ++completed_fences;fence_target.reset();return;
        }
        if(pc==0x3892e2&&address==0xfe801000) {
            if(value!=status())throw std::invalid_argument("Native frontend interrupt acknowledgement differs");
            control.run(generation,[&]{pending_status&=~value;});return;
        }
        if(pc==0x38e5f4&&address==0xfe801100) {
            const auto bits=value&0xe0u;
            if((value&~0xfe0u)||((value&0xf00u)!=0&&(value&0xf00u)!=0xf00u)||
                (bits!=0&&bits!=0x80&&bits!=0xe0)||((value&0xf00u)!=0)!=requested_reason)
                throw std::runtime_error("Unowned frontend policy/control bits remain required");
            control.run(generation,[&]{mode=bits==0?Mode::Running:bits==0x80?Mode::Halted:Mode::Trapped;});return;
        }
        if(address>=0xfe802054&&address<=0xfe802074&&!(address&3)&&
            (pc==0x38b066||pc==0x38b106||pc==0x38b12a)) {
            control.voice_head(generation,(address-0xfe802054)/4,value);return;
        }
        throw std::runtime_error("Unowned native frontend configuration/write remains required");
    }
    struct Memory final:SourcePhysicsContactMemory {
        Impl& p;explicit Memory(Impl& v):p(v){}
        void read(std::uint32_t address,std::span<std::byte> out)const override {
            if(address>=0xfe800000) {
                if(out.size()!=4)throw std::invalid_argument("Unexpected native frontend control width");
                const auto value=p.read_control(address);for(unsigned i=0;i<4;++i)out[i]=std::byte(value>>(8*i));return;
            }
            if(p.executing_dpc&&address==p.memory_fs+0x24&&out.size()==1){out[0]=std::byte(p.exclusion_depth?2:0);return;}
            p.memory.read(address,out);
        }
        void write(std::uint32_t address,std::span<const std::byte> in)override {
            if(address>=0xfe800000) {
                if(in.size()!=4)throw std::invalid_argument("Unexpected native frontend control width");
                std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(in[i])<<(8*i);p.write_control(address,value);return;
            }
            p.memory.write(address,in);
        }
    };
    bool queue_dpc(const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
        if(call.instruction!=0x38b191&&call.instruction!=0x389314)return false;
        const auto sp=abi.gp[4];
        if(!registered_dpc||call.tail||call.target!=0x80000077||word(memory,0x41b7c4)!=call.target||
            word(memory,sp)!=call.instruction+6||word(memory,sp+4)!=apu+0x4c8||word(memory,sp+8)||word(memory,sp+12))
            throw std::invalid_argument("Native frontend deferred call has no authentic registered source identity/frame");
        const bool added=std::ranges::none_of(dpcs,[&](const auto& d){return d->source_dpc==apu+0x4c8;});
        if(added)control.run(generation,[&]{auto d=std::make_shared<DeferredCall>();
            *d={next_dpc++,generation,apu+0x4c8,0x38918c,apu,NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(d.get())}};
            dpcs.push_back(std::move(d));
        });
        // Explicit native BOOL/fastcall policy. This reports an actual queue
        // insertion/coalescence only, never successful source DPC execution.
        abi.gp[0]=(abi.gp[0]&0xffffff00u)|unsigned(added);abi.gp[4]+=16;return true;
    }
    void run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
        const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
        require_live();if(executing)throw std::runtime_error("Native frontend source callback reentry is unsupported");
        executing=true;memory_fs=abi.fs_base;Memory view(*this);auto scoped=services;
        scoped.instruction=[&](auto site){pc=site;if(services.instruction)services.instruction(site);};
        scoped.required_owner=[&](auto&,const auto& call,auto& state,const auto& observer) {
            if(queue_dpc(call,state))return;
            if(executing_dpc&&call.instruction==0x389122) {
                const auto sp=state.gp[4];
                if(call.tail||call.target!=0x80000099||word(memory,0x41b7f4)!=call.target||
                    word(memory,sp)!=0x389124||word(memory,sp+4)!=0x3912d8||word(memory,sp+8)!=0x3889f1||word(memory,sp+12)!=apu)
                    throw std::invalid_argument("Native deferred synchronization has no authentic callback/frame");
                std::lock_guard lock(event_domain);const auto saved=state.gp;
                // Explicit native synchronization ABI, matching the independent
                // original oracle: retain caller GP except the genuine BOOL
                // result, and execute the whole callback on this real bank.
                std::array<std::byte,8> frame{};
                for(unsigned i=0;i<4;++i){frame[i]=std::byte(0x80000f00u>>(8*i));frame[i+4]=std::byte(apu>>(8*i));}
                memory.write(sp-8,frame);state.gp[4]=sp-8;
                source_run_sound_pending_aggregate_tu15(memory,state,fp,scoped,observer);
                const auto result=state.gp[0];state.gp=saved;state.gp[0]=result;state.gp[4]=sp+16;return;
            }
            if(control.try_exclusion(memory,call,state)) {
                if(executing_dpc){if(call.instruction==0x383d88)++exclusion_depth;else if(exclusion_depth)--exclusion_depth;else throw std::runtime_error("Native DPC exclusion completion lacks its actual scope");}
                return;
            }
            if(services.required_owner){services.required_owner(memory,call,state,observer);return;}
            throw std::runtime_error("Unowned original frontend callback remains required");
        };
        try{
            if(executing_dpc)source_run_sound_frontend_dpc_tu15(view,entry,abi,fp,scoped,observe);
            else source_run_sound_frontend_events_tu15(view,entry,abi,fp,scoped,observe);
            if(exclusion_depth)throw std::runtime_error("Native DPC returned with an unfinished actual exclusion scope");
            executing=false;
        }
        catch(...){executing=false;fault=true;control.abort_transaction();throw;}
    }
};
SourceRuntimeSoundFrontendEvents::SourceRuntimeSoundFrontendEvents(SourceRuntimeMemory& m,
    SourceRuntimeSoundResetResources& r,SourceRuntimeSoundDmaResources& d,std::size_t capacity)
    :impl_(std::make_unique<Impl>(m,r,d,capacity)){}
SourceRuntimeSoundFrontendEvents::~SourceRuntimeSoundFrontendEvents()=default;
void SourceRuntimeSoundFrontendEvents::require_binding(const SourceRuntimeMemory& memory,
    const SourceRuntimeSoundResetResources::ControlLease& control)const {
    const auto& p=*impl_;p.require_thread();p.control.require_bank(memory);control.require_bank(memory);
    if(&memory!=&p.memory||!p.control.same_owner(control))
        throw std::invalid_argument("Native sound frontend belongs to another actual bank/control worker");
    p.require_live();
}
void SourceRuntimeSoundFrontendEvents::initialize_timed_dpc(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    auto& p=*impl_;p.require_live();const auto sp=abi.gp[4];
    if(&bank!=&p.memory||call.instruction!=0x3893db||call.tail||call.target!=0x8000006b||abi.gp[3]!=call.target||
        word(p.memory,0x41b70c)!=call.target||word(p.memory,sp)!=0x3893dd||
        word(p.memory,sp+4)!=p.apu+0x510||word(p.memory,sp+8)!=0x38901d||word(p.memory,sp+12)!=p.apu||p.timed_dpc)
        throw std::invalid_argument("Native timed DPC registration differs from original3893DB");
    p.control.run(p.generation,[&]{p.timed_dpc=std::make_shared<Impl::DpcRegistration>(
        Impl::DpcRegistration{p.apu+0x510,0x38901d,p.apu,p.generation});});abi.gp[4]+=16;
}
bool SourceRuntimeSoundFrontendEvents::queue_timed_dpc() {
    auto& p=*impl_;p.require_live();if(!p.timed_dpc)throw std::runtime_error("Native timed DPC has no actual registration");
    if(std::ranges::any_of(p.dpcs,[&](const auto& d){return d->source_dpc==p.apu+0x510;}))return false;
    p.control.run(p.generation,[&]{auto d=std::make_shared<DeferredCall>();
        *d={p.next_dpc++,p.generation,p.apu+0x510,0x38901d,p.apu,NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(d.get())}};
        p.dpcs.push_back(std::move(d));});return true;
}
bool SourceRuntimeSoundFrontendEvents::remove_timed_dpc() {
    auto& p=*impl_;p.require_thread();
    if(p.executing_dpc)throw std::runtime_error("Cannot cancel an executing actual deferred sound callback");
    const auto found=std::ranges::find_if(p.dpcs,[&](const auto& d){return d->source_dpc==p.apu+0x510;});
    if(found==p.dpcs.end())return false;p.cancelled_dpcs.push_back(*found);p.dpcs.erase(found);return true;
}
void SourceRuntimeSoundFrontendEvents::require_timer_queue_retired()const {
    const auto& p=*impl_;p.require_thread();
    if(p.executing||p.executing_dpc||std::ranges::any_of(p.dpcs,[&](const auto& d){return d->source_dpc==p.apu+0x510;}))
        throw std::runtime_error("Native timer owner still has an executing or queued source callback");
}
void SourceRuntimeSoundFrontendEvents::require_timed_voice(std::uint32_t channel,std::uint32_t record)const {
    auto& p=*impl_;p.require_live();
    const auto found=std::ranges::find_if(p.voices,[&](const auto& row){return row.second.channel==channel;});
    const auto notifiers=p.allocation(0x3913a0);
    if(found==p.voices.end()||p.control.voice(found->first)!=found->second.voice||word(p.memory,channel+8)!=p.apu||
        word(p.memory,p.apu+0x88+found->first*4)!=channel||word(p.memory,channel+0xc,2)!=found->first||word(p.memory,channel+0x64,1)!=1||
        word(p.memory,channel+0x68)!=notifiers.source+32+found->first*64||
        word(p.memory,channel)!=0x42132c||!p.memory.is_read_only(0x42132c,0x28)||word(p.memory,0x421350)!=0x38d1dd||
        record!=channel+0x108||word(p.memory,record+8)&~2u||word(p.memory,record+12)!=channel||word(p.memory,record+16)!=4||
        (word(p.memory,channel+0x12,1)&3)==3)
        throw std::invalid_argument("Native timed callback lacks an owned genuine inactive base voice/index4 record");
    (void)p.memory.view(channel,0x158);(void)p.memory.view(word(p.memory,channel+0x80),0xe0);
}
bool SourceRuntimeSoundFrontendEvents::dispatch_timed_dpc(SourcePhysicsWorldRuntimeAbi& abi,const std::function<void()>& execute) {
    auto& p=*impl_;p.require_live();if(p.dpcs.empty())return false;
    const auto dpc=p.dpcs.front();const auto sp=abi.gp[4];
    if(!execute||!p.timed_dpc||dpc->generation!=p.generation||dpc->source_dpc!=p.apu+0x510||dpc->routine!=0x38901d||dpc->context!=p.apu||
        word(p.memory,sp+4)!=dpc->source_dpc||word(p.memory,sp+8)!=p.apu||word(p.memory,sp+12)||word(p.memory,sp+16))
        throw std::invalid_argument("Native timed thunk frame differs from its actual registered queued callback");
    if(p.executing||p.trapped||!p.commands.empty()||p.pending_status||p.fence_target)
        throw std::runtime_error("Native timed drain requires prior frontend commands/events to complete");
    std::lock_guard lock(p.event_domain);p.executing=true;p.executing_dpc=true;
    try{execute();p.executing=false;p.executing_dpc=false;p.dpcs.pop_front();++p.completed_dpcs;return true;}
    catch(...){p.executing=false;p.executing_dpc=false;p.fault=true;p.control.abort_transaction();throw;}
}
bool SourceRuntimeSoundFrontendEvents::try_initialize_dpc(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.instruction!=0x3893bd)return false;auto& p=*impl_;p.require_live();const auto sp=abi.gp[4];
    if(&bank!=&p.memory||call.tail||call.target!=0x8000006b||abi.gp[3]!=call.target||word(p.memory,0x41b70c)!=call.target||
        word(p.memory,sp)!=0x3893bf||word(p.memory,sp+4)!=p.apu+0x4c8||word(p.memory,sp+8)!=0x38918c||
        word(p.memory,sp+12)!=p.apu||p.registered_dpc)
        throw std::invalid_argument("Native frontend DPC registration differs from original3893BD");
    p.control.run(p.generation,[&]{p.registered_dpc=std::make_shared<Impl::DpcRegistration>(
        Impl::DpcRegistration{p.apu+0x4c8,0x38918c,p.apu,p.generation});});abi.gp[4]+=16;return true;
}
void SourceRuntimeSoundFrontendEvents::prepare_idle_trap(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
    auto& p=*impl_;p.require_live();
    if(pc!=0x38e8eb||address!=0xfe801504||value!=0x8000)throw std::invalid_argument("Native idle trap request differs from authenticated38E8EB");
    p.control.run(p.generation,[&]{p.idle_enabled=true;});
}
void SourceRuntimeSoundFrontendEvents::bind(std::uint16_t index,std::uint32_t channel,
    std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease> voice) {
    auto& p=*impl_;p.require_live();
    if(!voice||voice->source_index()!=index||p.voices.contains(index)||p.control.voice(index)!=voice)
        throw std::invalid_argument("Native frontend voice lease identity differs");
    p.voices.emplace(index,Impl::Binding{channel,std::move(voice)});
    try{(void)p.binding(index);}catch(...){p.voices.erase(index);throw;}
}
SourceRuntimeSoundFrontendEvents::Admission SourceRuntimeSoundFrontendEvents::idle_voice(std::uint16_t index) {
    auto& p=*impl_;p.require_live();
    if(!p.idle_enabled||!p.registered_dpc)throw std::runtime_error("Native idle command lacks actual trap/deferred-call resources");
    const auto& b=p.binding(index);const auto record=p.allocation(0x391390).source+index*0x80;
    const auto completion=b.voice->completion();
    const auto status=b.voice->status();
    if(!completion||completion->status!=SdlSoundCompletionStatus::Success||
        status.device_removed||status.pending_packets||status.completed_packets!=1||
        (word(p.memory,record+0x54)&0x200000u)||(word(p.memory,record+4)&0x2800000u)||
        word(p.memory,p.apu+0x84))
        throw std::runtime_error("Actual native completion/source inactive nonpersistent voice prerequisite is unavailable");
    if(p.commands.size()==p.capacity)return Admission::Backpressure;
    if(std::ranges::find(p.commands,index)!=p.commands.end()||p.trapped==index)
        throw std::invalid_argument("Native idle command is already admitted");
    p.control.run(p.generation,[&]{p.commands.push_back(index);});return Admission::Accepted;
}
void SourceRuntimeSoundFrontendEvents::process() {
    auto& p=*impl_;p.require_live();if(p.mode!=Mode::Running||p.commands.empty())return;
    const auto index=p.commands.front();(void)p.binding(index);
    p.control.run(p.generation,[&]{p.commands.pop_front();p.trapped=index;p.mode=Mode::Trapped;p.requested_reason=true;p.pending_status|=0x10;});
}
void SourceRuntimeSoundFrontendEvents::source_mode_helper(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    auto& p=*impl_;p.require_live();
    if(abi.gp[1]!=p.apu+8||word(p.memory,abi.gp[4]+4)>1)
        throw std::invalid_argument("Native frontend supports only the genuine halted/running mode helper requests");
    p.run(0x38e5ab,abi,fp,services,observe);
}
bool SourceRuntimeSoundFrontendEvents::dispatch_interrupt(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    return dispatch_interrupt_entry(0x3892d0,abi,fp,services,observe);
}
bool SourceRuntimeSoundFrontendEvents::dispatch_registered_interrupt(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    auto& p=*impl_;p.require_live();const auto sp=abi.gp[4];
    if(word(p.memory,sp+4)!=0x3912d8||word(p.memory,sp+8)!=p.apu||word(p.memory,0x3912d8)!=0x38936c||word(p.memory,0x3912dc)!=p.apu)
        throw std::invalid_argument("Native registered interrupt does not match its actual source routine/context frame");
    return dispatch_interrupt_entry(0x38936c,abi,fp,services,observe);
}
bool SourceRuntimeSoundFrontendEvents::dispatch_interrupt_entry(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    auto& p=*impl_;p.require_live();if(!p.pending_status)return false;
    if(!p.trapped||(entry==0x3892d0&&abi.gp[1]!=p.apu))throw std::invalid_argument("Native frontend interrupt lacks its actual trapped source APU");
    const auto index=*p.trapped;const auto channel=p.binding(index).channel;
    std::lock_guard event_lock(p.event_domain);p.run(entry,abi,fp,services,observe);
    // The genuine source handler has unlinked the channel and published the
    // notifier/idle-list state. Retire actual native PCM dispatch afterwards.
    if(word(p.memory,channel+0x65,1)!=0xff||word(p.memory,word(p.memory,channel+0x68)+0x3f,1)!=1||
        word(p.memory,channel+0x4c)!=channel+0x4c||word(p.memory,channel+0x50)!=channel+0x4c) {
        p.fault=true;p.control.abort_transaction();throw std::runtime_error("Original idle trap did not complete its required source unlink/notification");
    }
    try{p.control.retire_dispatch(p.generation,index);p.trapped.reset();return true;}
    catch(...){p.fault=true;p.control.abort_transaction();throw;}
}
bool SourceRuntimeSoundFrontendEvents::dispatch_dpc(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    auto& p=*impl_;p.require_live();if(p.dpcs.empty())return false;
    if(p.executing||p.trapped||!p.commands.empty()||p.pending_status||p.fence_target)
        throw std::runtime_error("Native deferred drain requires all prior admitted frontend commands/events to complete");
    const auto dpc=p.dpcs.front();const auto sp=abi.gp[4];
    if(!p.registered_dpc||dpc->generation!=p.generation||dpc->routine!=0x38918c||dpc->context!=p.apu||
        word(p.memory,sp+4)!=dpc->source_dpc||word(p.memory,sp+8)!=p.apu||word(p.memory,sp+12)||word(p.memory,sp+16))
        throw std::invalid_argument("Native deferred thunk does not match its retained actual registration/frame");
    // Authenticate every retained original virtual target and notifier backing.
    // Source playback activation remains a declared input in the component
    // fixture; this does not construct or advertise a hardware voice engine.
    const auto notifiers=p.allocation(0x3913a0);
    for(const auto& [index,binding]:p.voices) {
        const auto channel=binding.channel,vtable=word(p.memory,channel);
        if(p.control.voice(index)!=binding.voice||word(p.memory,channel+8)!=p.apu||
            vtable!=0x42132c||!p.memory.is_read_only(vtable,0x24)||
            word(p.memory,vtable+0x18)!=0x38cad7||word(p.memory,vtable+0x1c)!=0x38cbb9||word(p.memory,vtable+0x20)!=0x38cbe8||
            word(p.memory,channel+0x68)!=notifiers.source+32+index*64)
            throw std::invalid_argument("Native deferred voice lacks authenticated original virtual/notifier resources");
        (void)p.memory.view(channel,0x158);(void)p.memory.view(word(p.memory,channel+0x80),0xe0);
    }
    std::array<std::uint32_t,256> seen{};unsigned seen_count=0;
    for(unsigned list=0;list<4;++list) {
        const auto head=p.apu+(list==3?0x4a0:0x488+list*8),offset=list==3?0x54u:0x4cu;
        auto current=word(p.memory,head);
        for(unsigned n=0;current!=head;++n) {
            if(n>=256||!current||current<offset)throw std::invalid_argument("Native deferred source list is malformed");
            const auto channel=current-offset;
            const auto owned=std::ranges::find_if(p.voices,[&](const auto& row){return row.second.channel==channel;});
            if(owned==p.voices.end()||std::ranges::find(std::span(seen).first(seen_count),channel)!=std::span(seen).first(seen_count).end())
                throw std::invalid_argument("Native deferred list contains an unowned or multiply-dispatched source voice");
            const auto next=word(p.memory,current),previous=word(p.memory,current+4);
            if(word(p.memory,next+4)!=current||word(p.memory,previous)!=current||
                word(p.memory,p.apu+0x88+owned->first*4)!=channel||word(p.memory,channel+0x64,1)!=1||
                word(p.memory,channel+0xc,2)!=owned->first||word(p.memory,channel+0x65,1)!=(list==3?0xffu:list))
                throw std::invalid_argument("Native deferred list/channel source ownership differs");
            if(word(p.memory,word(p.memory,channel+0x68)+0x3f,1)==1) {
                const auto receipt=owned->second.voice->completion();const auto status=owned->second.voice->status();
                if(!receipt||receipt->status!=SdlSoundCompletionStatus::Success||status.device_removed||status.pending_packets||status.completed_packets!=1)
                    throw std::runtime_error("Source idle notification lacks its actual retained native completion");
            }
            seen[seen_count++]=channel;current=next;
        }
    }
    std::lock_guard event_lock(p.event_domain);p.executing_dpc=true;
    try{p.run(0x38918c,abi,fp,services,observe);
        if(p.fence_target)throw std::runtime_error("Native deferred thunk left an incomplete actual fence transfer");
        p.executing_dpc=false;p.dpcs.pop_front();++p.completed_dpcs;return true;
    }catch(...){p.executing_dpc=false;p.fault=true;p.control.abort_transaction();throw;}
}
std::optional<SourceRuntimeSoundFrontendEvents::DeferredCall> SourceRuntimeSoundFrontendEvents::pending_dpc()const {
    auto& p=*impl_;p.require_live();if(p.dpcs.empty())return {};return *p.dpcs.front();
}
SourceRuntimeSoundFrontendEvents::State SourceRuntimeSoundFrontendEvents::state()const {
    auto& p=*impl_;p.require_thread();return {p.generation,p.mode,p.capacity,p.commands.size(),p.dpcs.size(),p.cancelled_commands.size(),p.cancelled_dpcs.size(),p.status(),p.idle_enabled,p.fault,p.completed_dpcs,p.completed_fences};
}
void SourceRuntimeSoundFrontendEvents::retire_generation() {
    auto& p=*impl_;p.require_thread();if(p.executing)throw std::runtime_error("Native frontend source transaction is still active");
    p.cancelled_commands.insert(p.cancelled_commands.end(),p.commands.begin(),p.commands.end());
    if(p.trapped)p.cancelled_commands.push_back(*p.trapped);
    p.cancelled_dpcs.insert(p.cancelled_dpcs.end(),p.dpcs.begin(),p.dpcs.end());
    p.commands.clear();p.dpcs.clear();p.trapped.reset();p.pending_status=0;p.mode=Mode::Halted;p.retired=true;
}
}
