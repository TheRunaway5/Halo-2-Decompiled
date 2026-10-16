#include "halo2/source_runtime_sound_ac97_channel_reset.hpp"
#include "halo2/source_runtime_sound_pcm_buffers.hpp"
#include "halo2/source_runtime_sound_cold_analog_start.hpp"
#include <atomic>
#include <bit>
#include "halo2/source_sound_ac97_output_tu15.hpp"
#include "halo2/source_sound_hardware_storage_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/source_sound_ac97_retire_tu15.hpp"
#include <SDL.h>
#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){const auto b=m.view(p,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(b[i])<<(8*i);return value;}
constexpr SdlSoundVoiceParameters paused_stereo(){return {0x3f800000,0,true,{{0,0,1.0f},{1,1,1.0f}}};}
}
struct SourceRuntimeSoundAc97ChannelReset::Endpoint {
    const Kind kind;
    const std::uint32_t channel,device;
    const NativeRuntimeAddress native_channel;
    const SdlSoundPlatform::Voice voice;
    const std::thread::id thread;
    SourceRuntimeSoundResetResources::ControlLease control;
    const std::uint64_t control_generation;
    mutable std::mutex mutex;
    std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages;
    std::shared_ptr<SdlSoundPlatform> output;
    std::uint64_t generation=1,resets{};
    SourceRuntimeSoundAc97OutputPages::Allocation descriptors{};
    std::uint8_t current{},last{};
    std::uint32_t channel_status{};
    bool resetting{},bound{},profile_bound{},admission_closed{},retired{},faulted{};
    ConsumerLease* consumer{};
    Endpoint(Kind k,std::uint32_t c,NativeRuntimeAddress n,SdlSoundPlatform::Voice v,
        std::shared_ptr<SourceRuntimeSoundAc97OutputPages> p,std::shared_ptr<SdlSoundPlatform> o,
        SourceRuntimeSoundResetResources::ControlLease control_owner,std::uint64_t epoch)
        :kind(k),channel(c),device(o->device_id()),native_channel(n),voice(v),thread(std::this_thread::get_id()),control(std::move(control_owner)),control_generation(epoch),pages(std::move(p)),output(std::move(o)){}
    ~Endpoint(){
        if(retired)return;
        // Explicit final-retirement contract: a live page/output lease stays
        // on its creator. Retained PCM remains in the actual output stream;
        // teardown never acknowledges it on behalf of its source caller.
        if(std::this_thread::get_id()!=thread)std::terminate();
        try{output->flush(voice);const auto s=output->status(voice);if(!s.pending_packets&&!s.completed_packets)output->destroy_voice(voice);}catch(...){}
    }
    void require_native()const{
        if(retired||!output||output->device_id()!=device)throw std::runtime_error("Native AC97 endpoint lease is retired or lost its actual output");
    }
    void flush_reset(){
        std::lock_guard lock(mutex);require_native();
        if(faulted||generation==std::numeric_limits<std::uint64_t>::max()||resets==std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("Native AC97 reset endpoint is faulted or its generation exhausted");
        resetting=true;++generation;consumer=nullptr;
        try{
            // Only this owned endpoint changes. The shared device continues
            // playing other retained streams throughout the worker fence.
            output->parameters(voice,paused_stereo());output->flush(voice);
            const auto status=output->status(voice);
            if(status.device_removed||!status.paused||status.pending_packets)
                throw std::runtime_error("Actual AC97 endpoint did not finish native pause/flush/quiescence");
            descriptors={};bound=profile_bound=false;channel_status=0;current=last=0;++resets;resetting=false;
        }catch(...){faulted=true;throw;}
    }
    void abort()noexcept{
        std::lock_guard lock(mutex);if(retired)return;faulted=true;consumer=nullptr;
        if(generation!=std::numeric_limits<std::uint64_t>::max())++generation;
        try{output->flush(voice);}catch(...){}
    }
    void retire(){
        if(std::this_thread::get_id()!=thread)throw std::runtime_error("Native AC97 endpoint retirement requires its creating owner thread");
        std::shared_ptr<SourceRuntimeSoundAc97OutputPages> release_pages;std::shared_ptr<SdlSoundPlatform> release_output;
        {std::lock_guard lock(mutex);if(retired)return;require_native();const auto status=output->status(voice);
            if(status.pending_packets||status.completed_packets)throw std::runtime_error("Native AC97 endpoint still retains unaccounted PCM receipts");
            output->destroy_voice(voice);retired=true;bound=false;descriptors={};
            if(generation!=std::numeric_limits<std::uint64_t>::max())++generation;
            release_pages=std::move(pages);release_output=std::move(output);}
    }
};
struct SourceRuntimeSoundAc97ChannelReset::CpuMemory final:SourcePhysicsContactMemory{
    SourceRuntimeSoundAc97ChannelReset& owner;
    explicit CpuMemory(SourceRuntimeSoundAc97ChannelReset& p):owner(p){}
    void read(std::uint32_t p,std::span<std::byte> out)const override{
        owner.require_thread();if(owner.fault_||owner.control_.generation()!=owner.generation_)
            throw std::runtime_error("Faulted or retired AC97 reset CPU view cannot read source data");
        if(p>=0xfec00000){const auto value=owner.cold_start_&&owner.pc_==0x38f0c7?owner.cold_start_->read_halted(p,unsigned(out.size())):owner.read_control(p,unsigned(out.size()));for(unsigned i=0;i<out.size();++i)out[i]=std::byte(value>>(8*i));return;}
        owner.pages_->cpu_memory().read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override{
        owner.require_thread();if(owner.fault_||owner.control_.generation()!=owner.generation_)
            throw std::runtime_error("Faulted or retired AC97 reset CPU view cannot write source data");
        if(owner.cold_start_&&owner.pc_==0x38f0f3){owner.cold_start_->write_run(p,in);return;}
        if(p>=0xfec00000){if(in.size()!=1&&in.size()!=4)throw std::invalid_argument("Unowned native AC97 control width");std::uint32_t value{};for(unsigned i=0;i<in.size();++i)value|=std::to_integer<unsigned>(in[i])<<(8*i);owner.write_control(p,unsigned(in.size()),value);return;}
        owner.pages_->cpu_memory().write(p,in);
    }
};
SourceRuntimeSoundAc97ChannelReset::EndpointLease::EndpointLease(std::shared_ptr<Endpoint> endpoint,std::uint64_t generation)
    :endpoint_(std::move(endpoint)),generation_(generation){}
SourceRuntimeSoundAc97ChannelReset::Kind SourceRuntimeSoundAc97ChannelReset::EndpointLease::kind()const noexcept{return endpoint_->kind;}
std::uint32_t SourceRuntimeSoundAc97ChannelReset::EndpointLease::source_channel()const noexcept{return endpoint_->channel;}
SdlSoundPlatform::Voice SourceRuntimeSoundAc97ChannelReset::EndpointLease::native_voice()const noexcept{return endpoint_->voice;}
SdlSoundPlatform::Submission SourceRuntimeSoundAc97ChannelReset::EndpointLease::submit_native(SdlSoundPacket packet){
    if(std::this_thread::get_id()!=endpoint_->thread)throw std::runtime_error("Native AC97 PCM submission requires its creating owner thread");
    bool current=false;try{current=endpoint_->control.generation()==endpoint_->control_generation;}catch(...){}
    if(!current){endpoint_->abort();throw std::runtime_error("Whole native APU reset retired this endpoint control generation and flushed its PCM");}
    std::lock_guard lock(endpoint_->mutex);endpoint_->require_native();
    if(endpoint_->faulted||endpoint_->resetting||endpoint_->admission_closed||endpoint_->consumer||endpoint_->generation!=generation_||SDL_GetAudioDeviceStatus(endpoint_->device)!=SDL_AUDIO_PLAYING)
        throw std::runtime_error("Native AC97 reset retired this PCM submission lease");
    return endpoint_->output->submit(endpoint_->voice,std::move(packet));
}
SdlSoundVoiceStatus SourceRuntimeSoundAc97ChannelReset::EndpointLease::status()const{std::lock_guard lock(endpoint_->mutex);endpoint_->require_native();return endpoint_->output->status(endpoint_->voice);}
std::optional<SdlSoundCompletion> SourceRuntimeSoundAc97ChannelReset::EndpointLease::completion()const{std::lock_guard lock(endpoint_->mutex);endpoint_->require_native();return endpoint_->output->completion(endpoint_->voice);}
void SourceRuntimeSoundAc97ChannelReset::EndpointLease::acknowledge(std::uint64_t submission){std::lock_guard lock(endpoint_->mutex);endpoint_->require_native();endpoint_->output->acknowledge(endpoint_->voice,submission);}
void SourceRuntimeSoundAc97ChannelReset::EndpointLease::retire(){endpoint_->retire();}
SourceRuntimeSoundAc97ChannelReset::SourceRuntimeSoundAc97ChannelReset(SourceRuntimeMemory& memory,
    std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages,std::shared_ptr<SdlSoundPlatform> output,
    SourceRuntimeSoundResetResources::ControlLease control)
    :memory_(memory),pages_(std::move(pages)),output_(std::move(output)),control_(std::move(control)),
     generation_(control_.generation()),device_(output_?output_->device_id():0),controller_(control_.source_apu()+0x1c){
    control_.require_bank(memory_);if(!pages_)throw std::invalid_argument("Native AC97 reset lacks its actual page owner");pages_->require_control(memory_,control_);
    require_live();cpu_memory_=std::make_unique<CpuMemory>(*this);
}
SourceRuntimeSoundAc97ChannelReset::~SourceRuntimeSoundAc97ChannelReset(){
    if(std::this_thread::get_id()!=thread_)std::terminate();abort();
    // A borrowed EndpointLease keeps receipts, output and pages alive. Retire
    // it on this creator after accounting; a stale retired handle is inert.
    for(const auto& [kind,endpoint]:endpoints_){(void)kind;try{endpoint->retire();}catch(...){} }
}
void SourceRuntimeSoundAc97ChannelReset::require_thread()const{if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native AC97 reset requires its creating owner thread");}
void SourceRuntimeSoundAc97ChannelReset::require_bank(const SourceRuntimeMemory& bank)const{
    require_thread();if(&bank!=&memory_)throw std::invalid_argument("Native AC97 reset belongs to a different source bank");
    control_.require_bank(bank);pages_->require_bank(bank);require_live();
}
void SourceRuntimeSoundAc97ChannelReset::require_control(const SourceRuntimeMemory& bank,const SourceRuntimeSoundResetResources::ControlLease& control)const{
    require_thread();if(&bank!=&memory_)throw std::invalid_argument("Native AC97 reset belongs to a different source bank");
    if(!control_.same_owner(control))throw std::invalid_argument("Native AC97 reset belongs to another actual control worker");
    require_bank(bank);
}
void SourceRuntimeSoundAc97ChannelReset::require_live()const{
    require_thread();if(fault_||!generation_||!output_||output_->device_id()!=device_||output_->output_channels()!=2||
        SDL_GetAudioDeviceStatus(device_)!=SDL_AUDIO_PLAYING||control_.generation()!=generation_)
        throw std::runtime_error("Native AC97 reset lost its actual control/output generation");
    pages_->require_control(memory_,control_);const auto pages=pages_->diagnostics();if(pages.faulted||pages.generation!=generation_||pages.output_device!=device_||pages.controller!=controller_)
        throw std::runtime_error("Native AC97 reset page/control binding differs");
}
void SourceRuntimeSoundAc97ChannelReset::bind_channel(std::uint32_t channel){
    require_live();memory_.validate_mutable_view(channel,0x30);const auto index=word(memory_,channel);
    const auto kind=index==0?Kind::Analog:Kind::Spdif;
    if(index>=2||endpoints_.contains(kind)||word(memory_,controller_+index*4)!=channel||word(memory_,channel+8)!=controller_||
        word(memory_,channel+0x18)||word(memory_,channel+0x1c)||memory_.native_address(channel,0x30).value<=0xffffffffu)
        throw std::invalid_argument("Native AC97 endpoint lacks its genuine uninitialized source channel");
    SdlSoundPlatform::Voice voice{};control_.run(generation_,[&]{voice=output_->create_voice(channel,paused_stereo());});
    try{endpoints_.emplace(kind,std::make_shared<Endpoint>(kind,channel,memory_.native_address(channel,0x30),voice,pages_,output_,control_,generation_));}
    catch(...){control_.run(generation_,[&]{output_->destroy_voice(voice);});throw;}
}
std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> SourceRuntimeSoundAc97ChannelReset::endpoint(Kind kind)const{
    require_live();const auto at=endpoints_.find(kind);if(at==endpoints_.end())throw std::invalid_argument("Native AC97 endpoint identity is unbound");
    std::lock_guard lock(at->second->mutex);at->second->require_native();if(at->second->faulted||at->second->admission_closed)throw std::runtime_error("Native AC97 endpoint admission is retired or faulted");
    return std::shared_ptr<EndpointLease>(new EndpointLease(at->second,at->second->generation));
}
void SourceRuntimeSoundAc97ChannelReset::prepare_page_release(const SourceRuntimeSoundAc97OutputPages::Allocation& row){
    require_bank(memory_);const auto kind=row.index==0?Kind::Analog:Kind::Spdif;
    const auto at=endpoints_.find(kind);if(row.index>=2||at==endpoints_.end())throw std::invalid_argument("Native AC97 page detach lacks its real endpoint");
    const auto& endpoint=at->second;const auto rows=pages_->diagnostics();
    const auto owned=std::ranges::find_if(rows.allocations,[&](const auto& p){return p.source==row.source&&p.native==row.native&&p.channel==row.channel&&p.field==row.field&&p.capacity==row.capacity;});
    if(owned==rows.allocations.end()||rows.source_exclusion_level!=2||endpoint->channel!=row.channel||
        memory_.native_address(row.channel,0x30)!=endpoint->native_channel||word(memory_,row.channel+row.field)!=row.source||
        word(memory_,row.channel+4)||word(memory_,row.channel+0x2c)||memory_.view(row.channel+0x24,2)[0]!=std::byte{}||
        memory_.view(row.channel+0x24,2)[1]!=std::byte{})
        throw std::invalid_argument("Native AC97 page detach precedes authentic source cleanup or lost its actual backing");
    control_.run(generation_,[endpoint,row]{std::lock_guard lock(endpoint->mutex);endpoint->require_native();const auto status=endpoint->output->status(endpoint->voice);
        if(endpoint->faulted||endpoint->resetting||endpoint->resets<2||endpoint->current||endpoint->last||status.device_removed||!status.paused||status.pending_packets||
            endpoint->generation==std::numeric_limits<std::uint64_t>::max()||
            (row.field==0x18&&(!endpoint->bound||endpoint->descriptors.source!=row.source||endpoint->descriptors.native!=row.native)))
            throw std::runtime_error("Native AC97 page detach lacks completed native stop/reset/quiescence");
        endpoint->admission_closed=true;if(row.field==0x18){endpoint->descriptors={};endpoint->bound=false;}++endpoint->generation;
    });
}
void SourceRuntimeSoundAc97ChannelReset::close_source_channel(std::uint32_t channel){
    require_bank(memory_);const auto at=std::ranges::find_if(endpoints_,[&](const auto& row){return row.second->channel==channel;});
    if(at==endpoints_.end())throw std::invalid_argument("Retired source AC97 channel lacks its actual native endpoint");
    const auto endpoint=at->second;const auto index=endpoint->kind==Kind::Analog?0u:1u;
    if(word(memory_,controller_+index*4))throw std::runtime_error("Native AC97 admission cannot close before source channel retirement");
    control_.run(generation_,[endpoint]{std::lock_guard lock(endpoint->mutex);endpoint->require_native();endpoint->admission_closed=true;
        endpoint->output->parameters(endpoint->voice,paused_stereo());endpoint->output->flush(endpoint->voice);const auto status=endpoint->output->status(endpoint->voice);
        if(status.device_removed||!status.paused||status.pending_packets)throw std::runtime_error("Retired source AC97 endpoint did not finish actual native closure");
    });
}
std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::Endpoint> SourceRuntimeSoundAc97ChannelReset::at_address(std::uint32_t address,unsigned bytes)const{
    for(const auto& [kind,endpoint]:endpoints_){
        const auto base=kind==Kind::Analog?0xfec00110u:0xfec00170u;
        if((address==base&&bytes==4)||(kind==Kind::Spdif&&address==base+12&&bytes==4)||((address==base+4||address==base+5||address==base+11)&&bytes==1))return endpoint;
    }
    throw std::runtime_error("Unowned native AC97 codec/start/status/configuration remains required at source PC "+std::to_string(pc_)+" address "+std::to_string(address)+" width "+std::to_string(bytes));
}
std::uint32_t SourceRuntimeSoundAc97ChannelReset::read_control(std::uint32_t address,unsigned bytes)const{
    require_live();const auto endpoint=at_address(address,bytes);const auto base=endpoint->kind==Kind::Analog?0xfec00110u:0xfec00170u;
    std::lock_guard lock(endpoint->mutex);endpoint->require_native();
    if(address==base+11&&(pc_==0x38f007||pc_==0x38f017||pc_==0x38ede5))return endpoint->resetting?2:0;
    if(address==base&&pc_==0x38f027)return endpoint->bound?endpoint->descriptors.physical:0;
    if(address==base+4&&pc_==0x38ee52)return endpoint->current;
    if(address==base+5&&pc_==0x38ee67)return endpoint->last;
    if(endpoint->kind==Kind::Spdif&&address==base+12&&pc_==0x38f035)return endpoint->profile_bound?endpoint->channel_status:0;
    throw std::runtime_error("Unowned native AC97 source read instruction remains required");
}
void SourceRuntimeSoundAc97ChannelReset::write_control(std::uint32_t address,unsigned bytes,std::uint32_t value){
    require_live();const auto endpoint=at_address(address,bytes);const auto base=endpoint->kind==Kind::Analog?0xfec00110u:0xfec00170u;
    if(pc_==0x38f007&&address==base+11&&value==2){
        control_.run(generation_,[endpoint]{endpoint->flush_reset();});return;
    }
    if(pc_==0x38ede5&&address==base+11&&value==0){
        control_.run(generation_,[endpoint]{std::lock_guard lock(endpoint->mutex);endpoint->require_native();endpoint->output->parameters(endpoint->voice,paused_stereo());if(!endpoint->output->status(endpoint->voice).paused)throw std::runtime_error("Actual native AC97 endpoint did not stop");});if(cold_start_)cold_start_->did_pause();return;
    }
    if(pc_==0x38f027&&address==base){
        const auto rows=pages_->diagnostics().allocations;const auto at=std::ranges::find_if(rows,[&](const auto& row){return row.channel==endpoint->channel&&row.field==0x18;});
        if(!value&&at==rows.end()&&!word(memory_,endpoint->channel+0x18)&&!word(memory_,endpoint->channel+0x1c)){
            control_.run(generation_,[endpoint]{std::lock_guard lock(endpoint->mutex);endpoint->require_native();if(endpoint->faulted||endpoint->resetting||!endpoint->resets)throw std::runtime_error("Unused AC97 table cannot retire before actual endpoint reset");endpoint->descriptors={};endpoint->bound=false;});return;
        }
        if(at==rows.end()||at->requested!=0x100||at->capacity<0x100||at->physical!=value||
            word(memory_,endpoint->channel+0x18)!=at->source||word(memory_,endpoint->channel+0x1c)!=at->physical||
            memory_.native_address(at->source,at->capacity)!=at->native||memory_.native_address(endpoint->channel,0x30)!=endpoint->native_channel)
            throw std::invalid_argument("Native AC97 descriptor binding lacks its actual source/physical/native64 pages");
        const auto binding=*at;control_.run(generation_,[endpoint,binding]{std::lock_guard lock(endpoint->mutex);endpoint->require_native();if(endpoint->faulted||endpoint->resetting||!endpoint->resets)throw std::runtime_error("Native AC97 descriptor table cannot bind before endpoint reset");endpoint->descriptors=binding;endpoint->bound=true;});return;
    }
    if((pc_==0x38ee52&&address==base+4)||(pc_==0x38ee67&&address==base+5)){
        if(value>=32)throw std::invalid_argument("Native AC97 source descriptor index exceeds its actual32-entry table");
        control_.run(generation_,[endpoint,value,address,base]{std::lock_guard lock(endpoint->mutex);endpoint->require_native();if((!endpoint->bound&&value)||endpoint->faulted||endpoint->resetting||!endpoint->resets)throw std::runtime_error("Native AC97 source index lacks an actual reset/native table");if(address==base+4)endpoint->current=std::uint8_t(value);else endpoint->last=std::uint8_t(value);});return;
    }
    if(pc_==0x38f035&&endpoint->kind==Kind::Spdif&&address==base+12&&bytes==4){
        // Exact IEC958 consumer audio/no-emphasis/48kHz profile, corroborated
        // by the source PCM/AC3 branch, vendor ProgramFormatTag and AES masks.
        // This configures this native PCM endpoint, not a host digital port.
        if(value!=0x02000000||word(memory_,endpoint->channel+0x28)!=value||word(memory_,0x390be4)!=0)
            throw std::runtime_error("Native AC97 endpoint lacks its authenticated consumer PCM48k profile; encoded/non-audio routes remain required");
        control_.run(generation_,[endpoint,value]{std::lock_guard lock(endpoint->mutex);endpoint->require_native();const auto status=endpoint->output->status(endpoint->voice);
            if(endpoint->faulted||endpoint->resetting||!endpoint->resets||status.device_removed||!status.paused||status.pending_packets)
                throw std::runtime_error("Native IEC958 PCM profile requires its actual reset/bound/quiescent48k stereo endpoint");
            endpoint->channel_status=value;endpoint->profile_bound=true;});return;
    }
    throw std::runtime_error("Unowned native AC97 source write/start request remains required");
}
void SourceRuntimeSoundAc97ChannelReset::run_cpu(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_live();if(running_)throw std::runtime_error("Native AC97 source reset CPU reentry is unsupported");running_=true;auto scoped=services;
    scoped.instruction=[&](auto pc){pc_=pc;if(services.instruction)services.instruction(pc);};
    auto retire_body=[&](std::uint32_t target,auto& cpu,const auto& record){
        std::uint32_t channel{};
        if(target==0x38f395){const auto index=word(memory_,cpu.gp[4]+4);if(index>=2||cpu.gp[1]!=controller_)throw std::invalid_argument("Native AC97 source retirement controller/index differs");channel=word(memory_,controller_+index*4);}
        source_run_sound_ac97_retire_tu15(*cpu_memory_,target,cpu,fp,scoped,record);
        if(channel)close_source_channel(channel);
    };
    scoped.required_owner=[&](auto&,const auto& call,auto& cpu,const auto& record){
        if(pages_->try_exclusion(call,cpu))return;
        if(source_sound_ac97_retire_tu15_owns_entry(call.target)){retire_body(call.target,cpu,record);return;}
        if(source_sound_ac97_output_tu15_owns_entry(call.target)){source_run_sound_ac97_output_tu15(*cpu_memory_,call.target,cpu,fp,scoped,record);return;}
        if(source_sound_hardware_storage_tu15_owns_entry(call.target)){source_run_sound_hardware_storage_tu15(*cpu_memory_,call.target,cpu,fp,scoped,record);return;}
        if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(*cpu_memory_,call.target,cpu,fp,scoped,record);return;}
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(*cpu_memory_,call.target,cpu,fp,scoped,record);return;}
        if(services.required_owner){services.required_owner(memory_,call,cpu,record);return;}
        throw std::runtime_error("Unowned actual AC97 native platform resource remains required");
    };
    try{if(source_sound_ac97_retire_tu15_owns_entry(entry))retire_body(entry,abi,observe);else source_run_sound_ac97_output_tu15(*cpu_memory_,entry,abi,fp,scoped,observe);running_=false;}
    catch(...){running_=false;abort();throw;}
}
#include "source_runtime_sound_codec_consumer.inc"
#include "source_runtime_sound_cold_analog_start.inc"
SourceRuntimeSoundAc97ChannelReset::State SourceRuntimeSoundAc97ChannelReset::diagnostics()const{
    require_thread();std::vector<EndpointState> rows;
    for(const auto& [kind,endpoint]:endpoints_){std::lock_guard lock(endpoint->mutex);rows.push_back({kind,endpoint->channel,endpoint->bound?endpoint->descriptors.physical:0,endpoint->channel_status,endpoint->native_channel,endpoint->bound?endpoint->descriptors.native:NativeRuntimeAddress{},endpoint->voice,endpoint->generation,endpoint->resets,endpoint->current,endpoint->last,endpoint->resetting,endpoint->bound,endpoint->profile_bound,endpoint->admission_closed,endpoint->retired,endpoint->faulted});}
    return {std::move(rows),fault_,false,false};
}
void SourceRuntimeSoundAc97ChannelReset::abort()noexcept{
    if(std::this_thread::get_id()!=thread_)std::terminate();if(fault_)return;fault_=true;
    for(const auto& [kind,endpoint]:endpoints_){(void)kind;endpoint->abort();}
    try{control_.abort_transaction();}catch(...){std::terminate();}
}
SourceRuntimeSoundAc97ChannelReset::CallScope::CallScope(SourceRuntimeSoundAc97ChannelReset* owner):owner_(owner),exceptions_(std::uncaught_exceptions()){if(owner_)owner_->require_live();}
SourceRuntimeSoundAc97ChannelReset::CallScope::~CallScope(){if(owner_&&std::uncaught_exceptions()>exceptions_)owner_->abort();}
}
