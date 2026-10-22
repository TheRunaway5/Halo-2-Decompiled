#include "halo2/source_runtime_sound_codec_route.hpp"
#include "halo2/source_sound_codec_initialization_tu15.hpp"
#include <SDL.h>
#include <algorithm>
#include <map>
#include <stdexcept>

namespace halo2 { namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){const auto b=m.view(p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;}
unsigned index(SourceRuntimeSoundCodecRoute::Kind kind){if(kind==SourceRuntimeSoundCodecRoute::Kind::Analog)return 0;if(kind==SourceRuntimeSoundCodecRoute::Kind::Spdif)return 1;throw std::invalid_argument("Unknown native codec route identity");}
void pcm_size(std::span<const std::int16_t> samples){if(samples.empty()||samples.size()%2||samples.size_bytes()>0x2000)throw std::invalid_argument("Native codec probe requires bounded S16 stereo frames");}
struct ControllerClaim {
    const SourceRuntimeSoundCodecRoute* owner;
    const SourceRuntimeMemory* bank;
    std::uint32_t source;
    SourceRuntimeSoundResetResources::ControlLease control;
};
std::mutex claim_mutex;
std::map<std::uint64_t,ControllerClaim> controller_claims;
}
struct SourceRuntimeSoundCodecRoute::Pipe {
    SDL_AudioStream* stream{};
    std::uint64_t resets{};
    explicit Pipe(SDL_AudioStream* s):stream(s){}
    ~Pipe(){if(stream)SDL_FreeAudioStream(stream);}
};
struct SourceRuntimeSoundCodecRoute::CpuMemory final:SourcePhysicsContactMemory {
    SourceRuntimeSoundCodecRoute& owner;
    explicit CpuMemory(SourceRuntimeSoundCodecRoute& p):owner(p){}
    void read(std::uint32_t p,std::span<std::byte> out)const override{
        owner.require_live();
        if(std::uint64_t(p)+out.size()>0xfec00000u&&p<0xfec01000u)
            throw std::runtime_error("Native codec functional ABI has no physical register readback");
        owner.memory_.read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte> bytes)override{
        owner.require_live();
        if(std::uint64_t(p)+bytes.size()>0xfec00000u&&p<0xfec01000u){
            if(bytes.size()!=1&&bytes.size()!=2)throw std::invalid_argument("Unowned codec native action width");
            std::uint32_t v{};for(unsigned i=0;i<bytes.size();++i)v|=std::to_integer<unsigned>(bytes[i])<<(8*i);
            owner.write_action(p,unsigned(bytes.size()),v);return;
        }
        owner.memory_.write(p,bytes);
    }
};
SourceRuntimeSoundCodecRoute::SourceRuntimeSoundCodecRoute(SourceRuntimeMemory& memory,
    std::shared_ptr<SdlSoundPlatform> output,SourceRuntimeSoundResetResources::ControlLease control,Profile profile)
    :memory_(memory),output_(std::move(output)),control_(std::move(control)),profile_(profile),
    generation_(control_.generation()),device_(output_?output_->device_id():0),controller_(control_.source_apu()+0x1c){
    control_.require_bank(memory_);if(!output_)throw std::invalid_argument("Native codec route lacks its actual output");control_.require_output(*output_);native_controller_=memory_.native_address(controller_,0x3c);require_live();
    if(profile_!=Profile::XboxWm9709Reference||native_controller_.value<=0xffffffffu)
        throw std::invalid_argument("Native codec route lacks its native64 controller or an explicit supported reference profile");
    memory_.validate_mutable_view(controller_,0x3c);
    {std::lock_guard lock(claim_mutex);
        if(!controller_claims.emplace(native_controller_.value,ControllerClaim{this,&memory_,controller_,control_}).second)
            throw std::invalid_argument("Actual native codec controller is already owned");
        claimed_=true;
    }
    try{
        if(word(memory_,controller_)||word(memory_,controller_+4)||word(memory_,controller_+8)||word(memory_,controller_+12))
            throw std::invalid_argument("Native codec route lacks the uninitialized source controller");
        cpu_memory_=std::make_unique<CpuMemory>(*this);
    }catch(...){release_claim();throw;}
}
SourceRuntimeSoundCodecRoute::~SourceRuntimeSoundCodecRoute(){if(std::this_thread::get_id()!=thread_)std::terminate();abort();}
void SourceRuntimeSoundCodecRoute::require_thread()const{if(std::this_thread::get_id()!=thread_)throw std::runtime_error("Native codec route requires its creating owner thread");}
void SourceRuntimeSoundCodecRoute::require_live()const{
    require_thread();if(fault_||!output_||!generation_||output_->device_id()!=device_||output_->output_channels()!=2||
        SDL_GetAudioDeviceStatus(device_)!=SDL_AUDIO_PLAYING||control_.generation()!=generation_||
        memory_.native_address(controller_,0x3c)!=native_controller_)
        throw std::runtime_error("Native codec route lost its actual output/control/source generation");
    control_.require_bank(memory_);control_.require_output(*output_);
    if(claimed_){std::lock_guard lock(claim_mutex);const auto at=controller_claims.find(native_controller_.value);
        if(at==controller_claims.end()||at->second.owner!=this||at->second.bank!=&memory_||at->second.source!=controller_||!at->second.control.same_owner(control_))
            throw std::runtime_error("Native codec route lost its exclusive actual controller claim");
    }else if(cpu_memory_)throw std::runtime_error("Native codec route controller ownership is retired");
}
void SourceRuntimeSoundCodecRoute::require_bank(const SourceRuntimeMemory& bank)const{
    require_thread();if(&bank!=&memory_)throw std::invalid_argument("Native codec route belongs to another source bank");require_live();
}
void SourceRuntimeSoundCodecRoute::require_control(const SourceRuntimeMemory& bank,const SourceRuntimeSoundResetResources::ControlLease& control)const{
    require_thread();if(&bank!=&memory_||!control_.same_owner(control))throw std::invalid_argument("Native codec route belongs to another source bank or actual control worker");require_live();
}
bool SourceRuntimeSoundCodecRoute::create_routes(){
    require_live();if(pipes_[0]||pipes_[1]||!commands_.empty()||word(memory_,controller_)||word(memory_,controller_+4))
        throw std::runtime_error("Native codec route was already configured or source channels exist");
    std::array<std::unique_ptr<Pipe>,2> staged;
    bool created=true;control_.run(generation_,[&]{
        for(auto& pipe:staged){auto* stream=SDL_NewAudioStream(AUDIO_S16SYS,2,48000,AUDIO_F32SYS,2,48000);
            if(!stream){created=false;break;}
            std::unique_ptr<SDL_AudioStream,decltype(&SDL_FreeAudioStream)> guarded(stream,SDL_FreeAudioStream);
            pipe=std::make_unique<Pipe>(guarded.get());
            // Pipe metadata allocation may throw; the actual SDL resource
            // remains guarded until its retained owner has been allocated.
            guarded.release();
            if(SDL_AudioStreamAvailable(stream)!=0)throw std::runtime_error("New native codec converter was not empty");
        }
    });if(!created)return false;pipes_=std::move(staged);return true;
}
bool SourceRuntimeSoundCodecRoute::try_call(const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi){
    if(call.target!=0x38ed2c&&call.target!=0x38ecff)return false;
    require_live();const auto sp=abi.gp[4];
    if(call.tail)throw std::invalid_argument("Native codec functional ABI requires an authentic source CALL");
    if(call.target==0x38ed2c){
        if(call.instruction!=0x38f46c||word(memory_,sp)!=0x38f471||abi.gp[1]!=controller_||word(memory_,controller_+8)!=1)
            throw std::invalid_argument("Native codec route acquisition differs from authentic38F45F caller");
        abi.gp[0]=create_routes()?1:0;abi.gp[4]+=4;return true;
    }
    if(call.instruction!=0x38ed90||word(memory_,sp)!=0x38ed95||!pipes_[0]||!pipes_[1]||command_lock_.owns_lock())
        throw std::invalid_argument("Native codec command acquisition lacks authentic source context or routes");
    const auto parent=word(memory_,sp+4),reg=word(memory_,sp+8),value=word(memory_,sp+12);
    constexpr std::array<std::uint32_t,3> parents{0x38edb8,0x38f503,0x38f519},indices{0x13,1,0xc},values{0,0,0x808};
    const auto next=commands_.size();if(next>=3||parent!=parents[next]||reg!=indices[next]||value!=values[next]||
        (next&&(!running_||pipes_[0]->resets!=1||pipes_[1]->resets!=1)))
        throw std::invalid_argument("Unowned native codec register, value, sequence or command caller");
    command_lock_=std::unique_lock<std::mutex>(command_mutex_,std::try_to_lock);
    if(!command_lock_.owns_lock())throw std::runtime_error("Actual native codec command transaction could not acquire its owner");
    command_index_=reg;command_value_=value;
    // A real synchronous worker fence validates retained route resources.
    // No physical CAS byte or device-ready register is supplied.
    control_.run(generation_,[&]{for(const auto& pipe:pipes_)if(SDL_AudioStreamAvailable(pipe->stream)<0)throw std::runtime_error("Native codec command lost its converter");});
    abi.gp[0]=1;abi.gp[4]+=4;return true;
}
void SourceRuntimeSoundCodecRoute::write_action(std::uint32_t address,unsigned bytes,std::uint32_t value){
    require_live();
    if(pc_==0x38eda2&&bytes==2&&command_lock_.owns_lock()&&address==0xfec00000u+command_index_*2u&&value==command_value_){
        control_.run(generation_,[&]{
            // WM9709 PD Rev1.3 explicitly implements no volume, mute or
            // powerdown controls. These three source commands are accepted
            // transactions and ignored by this named reference profile.
            for(const auto& pipe:pipes_)if(SDL_AudioStreamAvailable(pipe->stream)<0)throw std::runtime_error("Native codec command conversion resource failed");
        });commands_.push_back({command_index_,command_value_,true});++completed_transactions_;command_lock_.unlock();return;
    }
    if(pc_==0x38f49e&&bytes==1&&value==2&&commands_.size()==1&&!command_lock_.owns_lock()){
        const unsigned which=address==0xfec0011b?0:address==0xfec0017b?1:2;
        if(which>=2||!pipes_[which]||pipes_[which]->resets||(which&&pipes_[0]->resets!=1))
            throw std::invalid_argument("Native codec reset lacks its authentic independent route");
        control_.run(generation_,[&]{SDL_AudioStreamClear(pipes_[which]->stream);
            if(SDL_AudioStreamAvailable(pipes_[which]->stream)!=0)throw std::runtime_error("Actual native codec route reset did not clear its converter");
            ++pipes_[which]->resets;
        });return;
    }
    throw std::runtime_error("Unowned physical codec action remains required");
}
void SourceRuntimeSoundCodecRoute::run_cpu(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_live();if(running_||!commands_.empty()||pipes_[0]||pipes_[1]||(entry!=0x38e484&&entry!=0x38f45f))throw std::invalid_argument("Native codec constructor entry/reentry is unsupported");
    if(abi.gp[1]!=(entry==0x38e484?controller_-0x14:controller_)||(entry==0x38f45f&&word(memory_,abi.gp[4]+4)!=1))
        throw std::invalid_argument("Native codec constructor source controller/caller policy differs");
    running_=true;auto scoped=services;
    scoped.instruction=[&](auto pc){pc_=pc;if(services.instruction)services.instruction(pc);};
    scoped.required_owner=[&](auto&,const auto& call,auto& cpu,const auto& record){
        if(try_call(call,cpu))return;
        // Only a real IRQ6 producer may eventually own this request. This
        // component exposes the frontier for inspection/component probes.
        at_irq_frontier_=call.instruction==0x38f4c4&&call.target==0x8000002c;
        if(services.required_owner){services.required_owner(memory_,call,cpu,record);at_irq_frontier_=false;return;}
        throw std::runtime_error("Native codec IRQ6/shutdown/channel owner remains required");
    };
    try{source_run_sound_codec_initialization_tu15(*cpu_memory_,entry,abi,fp,scoped,observe);running_=false;}
    catch(...){running_=false;at_irq_frontier_=false;abort();throw;}
}
void SourceRuntimeSoundCodecRoute::run_command_component(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    require_live();if(!running_||!at_irq_frontier_||component_||pc_!=0x38f4c4||commands_.size()<1||commands_.size()>2)
        throw std::runtime_error("Bounded codec command probe requires the actual suspended IRQ6 frontier");
    const auto previous=pc_;component_=true;auto scoped=services;
    scoped.instruction=[&](auto pc){pc_=pc;if(services.instruction)services.instruction(pc);};
    scoped.required_owner=[&](auto&,const auto& call,auto& cpu,const auto&){if(!try_call(call,cpu))throw std::runtime_error("Unowned bounded native codec command request");};
    try{source_run_sound_codec_initialization_tu15(*cpu_memory_,0x38ed90,abi,fp,scoped,observe);component_=false;pc_=previous;}
    catch(...){component_=false;pc_=previous;abort();throw;}
}
void SourceRuntimeSoundCodecRoute::queue_native_probe(Kind kind,std::span<const std::int16_t> samples){
    require_live();pcm_size(samples);auto& pipe=pipes_[index(kind)];if(!pipe||command_lock_.owns_lock())throw std::runtime_error("Native codec conversion route is not available for a probe");
    try{control_.run(generation_,[&]{if(SDL_AudioStreamAvailable(pipe->stream)!=0)throw std::runtime_error("Native codec conversion probe requires an empty queue");
        if(SDL_AudioStreamPut(pipe->stream,samples.data(),int(samples.size_bytes()))<0)throw std::runtime_error(std::string("Actual native codec PCM put failed: ")+SDL_GetError());
    });}catch(...){abort();throw;}
}
std::vector<float> SourceRuntimeSoundCodecRoute::convert_native(Kind kind,std::span<const std::int16_t> samples){
    require_live();pcm_size(samples);std::vector<float> converted(samples.size());queue_native_probe(kind,samples);auto& pipe=pipes_[index(kind)];
    try{control_.run(generation_,[&]{if(SDL_AudioStreamFlush(pipe->stream)<0||SDL_AudioStreamAvailable(pipe->stream)!=int(converted.size()*sizeof(float))||
            SDL_AudioStreamGet(pipe->stream,converted.data(),int(converted.size()*sizeof(float)))!=int(converted.size()*sizeof(float))||SDL_AudioStreamAvailable(pipe->stream)!=0)
            throw std::runtime_error("Actual native codec stereo sample conversion did not preserve its complete frame count");
    });}catch(...){abort();throw;}return converted;
}
SourceRuntimeSoundCodecRoute::State SourceRuntimeSoundCodecRoute::diagnostics(){
    require_thread();try{bool claimed=false;{std::lock_guard lock(claim_mutex);const auto at=controller_claims.find(native_controller_.value);claimed=at!=controller_claims.end()&&at->second.owner==this;}
    State result{profile_,controller_,device_,native_controller_,generation_,completed_transactions_,{},commands_,command_lock_.owns_lock(),fault_,claimed,false,false,false};
    if(!fault_)require_live();
    for(unsigned i=0;i<2;++i)if(pipes_[i]){int available{};control_.run(generation_,[&]{available=SDL_AudioStreamAvailable(pipes_[i]->stream);});if(available<0)throw std::runtime_error("Actual native codec diagnostics lost converter queue");result.pipes.push_back({i?Kind::Spdif:Kind::Analog,NativeRuntimeAddress(reinterpret_cast<std::uintptr_t>(pipes_[i]->stream)),unsigned(available),pipes_[i]->resets});}
    return result;}catch(...){abort();throw;}
}
void SourceRuntimeSoundCodecRoute::abort()noexcept{
    if(std::this_thread::get_id()!=thread_)std::terminate();if(fault_)return;fault_=true;
    if(command_lock_.owns_lock())command_lock_.unlock();
    // Every stream operation is a synchronous worker job. No outstanding
    // callback can borrow these converters, so creator teardown is safe
    // even if a sibling already faulted the shared control generation.
    try{control_.abort_transaction();control_.fence_retirement();}catch(...){std::terminate();}
    for(auto& pipe:pipes_)pipe.reset();
    release_claim();
}
void SourceRuntimeSoundCodecRoute::release_claim()noexcept{
    std::lock_guard lock(claim_mutex);if(!claimed_)return;const auto at=controller_claims.find(native_controller_.value);
    if(at!=controller_claims.end()&&at->second.owner==this)controller_claims.erase(at);claimed_=false;
}
}
