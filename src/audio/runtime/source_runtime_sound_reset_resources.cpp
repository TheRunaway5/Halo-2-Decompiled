#include "halo2/source_runtime_sound_reset_resources.hpp"
#include "halo2/source_sound_reset_tu15.hpp"
#include <algorithm>
#include <array>
#include <condition_variable>
#include <deque>
#include <future>
#include <exception>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    const auto bytes=m.view(p,4);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
}
struct SourceRuntimeSoundResetResources::Impl {
    SourceRuntimeMemory& memory;
    const std::shared_ptr<SdlSoundPlatform> output;
    const std::uint32_t device,fs;
    const std::thread::id thread=std::this_thread::get_id();
    // Native implementation capacity, never returned to a source FIFO poll.
    static constexpr std::size_t command_capacity=64;
    mutable std::mutex queue_mutex;
    std::condition_variable queue_changed;
    std::deque<std::packaged_task<void()>> commands;
    std::thread worker;
    bool stopping{},active_command{},fault{},running_reset{};
    std::uint64_t generation{},completed_commands{},resets{};
    std::uint32_t apu{},pc{};
    NativeRuntimeAddress native_apu{};
    std::recursive_mutex exclusion;
    unsigned exclusion_depth{};
    struct VoiceBinding {std::shared_ptr<VoiceLease> lease;std::uint64_t generation;bool admitted;SdlSoundVoiceParameters parameters;};
    std::map<std::uint16_t,VoiceBinding> voices;
    // Actual native dispatch membership. Retiring a list unlinks dispatch,
    // while the packet owner retains completion/PCM until acknowledgement.
    std::array<std::deque<std::uint16_t>,9> voice_lists;
    bool interrupt_delivery{},frontend_privileged_locked{},frontend_frozen{},sequence_running{};
    struct ProcessorContext {
        // No authenticated program/effect definition has been acquired here.
        // Empty storage is explicitly unconfigured, never a substitute DSP.
        std::vector<std::byte> program;
        std::vector<float> working;
        bool held_reset{};
        void reset(bool hold) {program.clear();working.clear();held_reset=hold;}
    } gp,ep;

    Impl(SourceRuntimeMemory& m,std::shared_ptr<SdlSoundPlatform> o,std::uint32_t f)
        :memory(m),output(std::move(o)),device(output?output->device_id():0),fs(f) {
        if(!device||output->output_channels()!=2||!fs||fs>0xffffffdbu||memory.view(fs+0x24,1)[0]!=std::byte{})
            throw std::invalid_argument("Sound reset requires its actual stereo device and passive native thread context");
        worker=std::thread([this] {
            for(;;) {
                std::packaged_task<void()> task;
                {
                    std::unique_lock lock(queue_mutex);
                    queue_changed.wait(lock,[this]{return stopping||!commands.empty();});
                    if(stopping&&commands.empty())return;
                    task=std::move(commands.front());commands.pop_front();active_command=true;
                }
                task();
                // Release this worker's task-state ownership before publishing
                // idle. execute() also releases the future reference and
                // fences both sides before returning or rethrowing.
                task = {};
                {std::lock_guard lock(queue_mutex);++completed_commands;active_command=false;}
                queue_changed.notify_all();
            }
        });
    }
    ~Impl() {
        {std::lock_guard lock(queue_mutex);stopping=true;}
        queue_changed.notify_all();if(worker.joinable())worker.join();
        // No source completion is acknowledged by teardown. Unaccounted
        // packets remain retained by the shared SDL owner until its borrower
        // accounts them or the actual output resource itself is destroyed.
        for(const auto& [index,binding]:voices) {
            (void)index;
            try {const auto voice=binding.lease->native_voice();output->flush(voice);const auto s=output->status(voice);
                if(!s.pending_packets&&!s.completed_packets)binding.lease->retire();
            }catch(...){}
        }
        // Active source transactions must end on their admitting thread.
        // An idle native owner may be destroyed elsewhere after worker join.
        if(exclusion_depth&&std::this_thread::get_id()!=thread)std::terminate();
        while(exclusion_depth){exclusion.unlock();--exclusion_depth;}
    }
    void require_owner(bool permit_fault=false) const {
        // Reject the foreign thread before any mutable lifecycle/device read.
        if(std::this_thread::get_id()!=thread)
            throw std::runtime_error("Sound reset request is on another owner thread");
        if((fault&&!permit_fault)||
            output->device_id()!=device||output->output_channels()!=2)
            throw std::runtime_error("Sound reset owner is faulted, foreign or lost its retained stereo output");
    }
    template<class F> void execute(F action) {
        std::packaged_task<void()> task(std::move(action));auto done=task.get_future();
        {
            std::lock_guard lock(queue_mutex);
            if(stopping||commands.size()==command_capacity)throw std::runtime_error("Native sound control queue cannot accept this request");
            commands.push_back(std::move(task));
        }
        queue_changed.notify_one();
        std::exception_ptr failure;
        try{done.get();}catch(...){failure=std::current_exception();}
        // Future readiness alone does not retire the worker's callable. Both
        // success and failure wait until its captures are destroyed before
        // source resumption or stack/owner unwinding can begin.
        {std::unique_lock lock(queue_mutex);
            queue_changed.wait(lock,[this]{return commands.empty()&&!active_command;});}
        if(failure)std::rethrow_exception(failure);
    }
    SdlSoundPlatform::Voice voice(std::uint16_t index) const {
        require_owner(true);const auto found=voices.find(index);
        if(found==voices.end())throw std::invalid_argument("Native sound reset does not own that voice");
        return found->second.lease->native_voice();
    }
    std::uint32_t control(std::uint32_t address) const {
        if(!running_reset)throw std::runtime_error("Native audio control read is outside authenticated reset");
        switch(address) {
        case 0xfe801004:return interrupt_delivery?1:0;
        case 0xfe801510:return frontend_privileged_locked?1:0;
        case 0xfe801100:return frontend_frozen?0x80:0;
        case 0xfe802000:return sequence_running?8:0;
        case 0xfe83fffc:return gp.held_reset?1:0;
        case 0xfe85fffc:return ep.held_reset?1:0;
        default:
            if(address>=0xfe802054&&address<=0xfe802074&&!(address&3)) {
                const auto& list=voice_lists[(address-0xfe802054)/4];
                return list.empty()?0xffffu:list.front();
            }
            throw std::runtime_error("Unowned source audio hardware read; no synthetic register bank exists");
        }
    }
    void apply(std::uint32_t address,std::uint32_t value) {
        if(!running_reset)throw std::runtime_error("Native audio control write is outside authenticated reset");
        switch(pc) {
        case 0x38e562:
            if(address!=0xfe801004||value)break;
            execute([this]{for(const auto& [index,binding]:voices){(void)index;output->flush(binding.lease->native_voice());}interrupt_delivery=false;});return;
        case 0x38e599:
            if(address!=0xfe801510||value!=1)break;
            execute([this]{frontend_privileged_locked=true;});return;
        case 0x38e5f4:
            if(address!=0xfe801100||value!=0x80)break;
            execute([this]{frontend_frozen=true;});return;
        case 0x38e656:
            if(address!=0xfe802000||value)break;
            execute([this]{sequence_running=false;});return;
        case 0x38ec6e:
            if(address!=0xfe83fffc||value)break;
            execute([this]{gp.reset(false);});return;
        case 0x38ec7b:
            if(address!=0xfe85fffc||value!=1)break;
            execute([this]{ep.reset(true);});return;
        default: {
            constexpr std::array<std::uint32_t,9> sites{0x38ec41,0x38ec50,0x38ec5f,0x38ec46,0x38ec55,0x38ec64,0x38ec4b,0x38ec5a,0x38ec69};
            if(address>=0xfe802054&&address<=0xfe802074&&!(address&3)&&value==0xffff) {
                const auto index=(address-0xfe802054)/4;
                if(pc==sites[index]){execute([this,index]{voice_lists[index].clear();});return;}
            }
        }
        }
        throw std::invalid_argument("Native audio reset control instruction, address or request differs");
    }
    struct Memory final:SourcePhysicsContactMemory {
        Impl& owner;explicit Memory(Impl& o):owner(o){}
        void read(std::uint32_t address,std::span<std::byte> out)const override {
            if(address==owner.fs+0x24&&out.size()==1) {
                // The original scoped exclusion helper consumes this native
                // domain level. Level2 denotes owned serialized exclusion;
                // it never raises the host thread's interrupt priority.
                out[0]=std::byte(owner.exclusion_depth?2:0);return;
            }
            if(address>=0xfe800000) {
                if(out.size()!=4&&!(address==0xfe801100&&out.size()==1))
                    throw std::invalid_argument("Unexpected native audio control read width");
                const auto value=owner.control(address);
                for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(value>>(8*i));return;
            }
            owner.memory.read(address,out);
        }
        void write(std::uint32_t address,std::span<const std::byte> in)override {
            if(address>=0xfe800000) {
                if(in.size()!=4)throw std::invalid_argument("Unexpected native audio control write width");
                std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(in[i])<<(8*i);
                owner.apply(address,value);return;
            }
            owner.memory.write(address,in);
        }
    };
    void synchronization(const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
        const auto sp=abi.gp[4];
        if(call.tail||word(memory,sp)!=call.instruction+6)
            throw std::invalid_argument("Native audio exclusion call frame differs");
        if(call.instruction==0x383d88&&call.target==0x800000a0&&word(memory,0x41b7ac)==call.target&&
            (abi.gp[1]&0xff)==2&&!exclusion_depth) {
            exclusion.lock();++exclusion_depth;
            abi.gp[0]&=0xffffff00u;abi.gp[4]+=4;return;
        }
        if(call.instruction==0x383d9d&&call.target==0x800000a1&&word(memory,0x41b6fc)==call.target&&
            !(abi.gp[1]&0xff)&&exclusion_depth==1) {
            --exclusion_depth;exclusion.unlock();abi.gp[4]+=4;return;
        }
        throw std::runtime_error("Unowned native audio reset resource request");
    }
};
SourceRuntimeSoundResetResources::SourceRuntimeSoundResetResources(SourceRuntimeMemory& m,
    std::shared_ptr<SdlSoundPlatform> o,std::uint32_t fs):impl_(std::make_shared<Impl>(m,std::move(o),fs)){}
SourceRuntimeSoundResetResources::~SourceRuntimeSoundResetResources()=default;
bool SourceRuntimeSoundResetResources::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,
    const SourcePhysicsContactRecordObserver& observe) {
    const bool exclusion=call.instruction==0x383d88||call.instruction==0x383d9d;
    if(call.instruction!=0x38ec8f&&!exclusion)return false;
    auto& p=*impl_;p.require_owner();
    if(exclusion) {
        if(&bank!=&p.memory||abi.fs_base!=p.fs||!p.resets)
            throw std::invalid_argument("Native sound storage exclusion lacks its retained reset/output owner");
        p.synchronization(call,abi);return true;
    }
    const auto sp=abi.gp[4],sub=abi.gp[1];
    if(&bank!=&p.memory||call.target!=0x38ec0e||call.tail||abi.fs_base!=p.fs||
        word(p.memory,sp)!=0x38ec94||word(p.memory,sp+12)!=0x389389||sub!=abi.gp[6]||sub<8)
        throw std::invalid_argument("Native sound reset bank, source subobject or original caller differs");
    const auto apu=sub-8;p.memory.validate_mutable_view(apu,0x540);
    const auto native=p.memory.native_address(apu,0x540);
    if(native.value<=0xffffffffu||word(p.memory,apu)!=0x4212b0||word(p.memory,apu+4)!=1||
        word(p.memory,sub)!=0x4212ac||word(p.memory,0x391200)!=sub+0x14||
        (p.apu&&(p.apu!=apu||p.native_apu!=native)))
        throw std::invalid_argument("Native sound reset lost the genuine retained APU constructor binding");
    // Authentic cold constructor has no timer, event or DSP object. Such
    // resources need their own genuine cleanup before warm SDK reset can run.
    const auto timer_tag=p.memory.view(sub+0x30,2);
    if(word(p.memory,0x391208)||word(p.memory,sub+0x14)||word(p.memory,sub+0x18)||
        word(p.memory,sub+0x20)||(timer_tag[0]==std::byte{0x13}&&timer_tag[1]==std::byte{}))
        throw std::runtime_error("Native sound reset reached an unowned source timer/event/processor cleanup");
    if(p.running_reset||p.exclusion_depth||p.memory.view(p.fs+0x24,1)[0]!=std::byte{})
        throw std::runtime_error("Native sound reset is not at its owned passive transaction boundary");
    p.apu=apu;p.native_apu=native;p.running_reset=true;Impl::Memory memory(p);
    auto scoped=services;
    scoped.instruction=[&](auto pc){p.pc=pc;if(services.instruction)services.instruction(pc);};
    scoped.required_owner=[&](auto&,const auto& request,auto& state,const auto&){p.synchronization(request,state);};
    try {
        source_run_sound_reset_tu15(memory,0x38ec0e,abi,fp,scoped,observe);
        if(p.exclusion_depth)throw std::runtime_error("Original sound reset left native exclusion owned");
        p.execute([&]{++p.generation;++p.resets;});p.running_reset=false;return true;
    }catch(...) {
        p.fault=true;p.running_reset=false;
        while(p.exclusion_depth){p.exclusion.unlock();--p.exclusion_depth;}
        throw;
    }
}
SourceRuntimeSoundResetResources::State SourceRuntimeSoundResetResources::state()const {
    const auto& p=*impl_;p.require_owner(true);std::lock_guard lock(p.queue_mutex);
    return {p.generation,p.completed_commands,p.resets,p.commands.size()+std::size_t(p.active_command),Impl::command_capacity,
        p.voices.size(),p.apu,p.device,p.native_apu,!p.gp.program.empty(),!p.ep.program.empty(),p.fault};
}
std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease> SourceRuntimeSoundResetResources::bind_voice(std::uint16_t index,std::uint64_t context,
    const SdlSoundVoiceParameters& parameters) {
    auto& p=*impl_;p.require_owner();
    if(index>=256||p.voices.contains(index))throw std::invalid_argument("Native stereo voice binding is invalid or already owned");
    std::shared_ptr<VoiceLease> lease;
    p.execute([&]{const auto v=p.output->create_voice(context,parameters);
        try{lease=std::make_shared<VoiceLease>(p.output,index,context,v);p.voices.emplace(index,Impl::VoiceBinding{lease,p.generation,true,parameters});p.voice_lists[0].push_back(index);}
        catch(...){p.voices.erase(index);p.output->destroy_voice(v);throw;}});
    return lease;
}
SdlSoundPlatform::Voice SourceRuntimeSoundResetResources::native_voice(std::uint16_t index)const{return impl_->voice(index);}
SdlSoundPlatform::Submission SourceRuntimeSoundResetResources::submit(std::uint16_t index,SdlSoundPacket packet) {
    auto& p=*impl_;p.require_owner();const auto v=p.voice(index);
    if(p.voices.at(index).generation!=p.generation||!p.voices.at(index).admitted)
        throw std::runtime_error("Reset retired this voice's PCM dispatch; only completion accounting remains available");
    SdlSoundPlatform::Submission result{};p.execute([&]{result=p.output->submit(v,std::move(packet));});return result;
}
std::optional<SdlSoundCompletion> SourceRuntimeSoundResetResources::completion(std::uint16_t index)const {
    const auto& p=*impl_;return p.output->completion(p.voice(index));
}
void SourceRuntimeSoundResetResources::acknowledge(std::uint16_t index,std::uint64_t submission) {
    auto& p=*impl_;p.require_owner(true);p.output->acknowledge(p.voice(index),submission);
}
void SourceRuntimeSoundResetResources::retire_voice(std::uint16_t index) {
    auto& p=*impl_;p.require_owner(true);(void)p.voice(index);p.voices.at(index).lease->retire();
    for(auto& list:p.voice_lists)std::erase(list,index);p.voices.erase(index);
}
bool SourceRuntimeSoundResetResources::quiescent()const noexcept {
    const auto& p=*impl_;if(std::this_thread::get_id()!=p.thread)return false;
    if(p.fault||p.running_reset||p.exclusion_depth||!p.voices.empty())return false;
    std::lock_guard lock(p.queue_mutex);return p.commands.empty()&&!p.active_command;
}
SourceRuntimeSoundResetResources::VoiceLease::VoiceLease(std::shared_ptr<SdlSoundPlatform> output,
    std::uint16_t index,std::uint64_t context,SdlSoundPlatform::Voice voice)
    :output_(std::move(output)),index_(index),context_(context),voice_(voice),device_(output_?output_->device_id():0) {
    if(!device_||output_->output_channels()!=2||index_>=256||!voice_)
        throw std::invalid_argument("Native sound voice lease requires an actual owned stereo binding");
}
void SourceRuntimeSoundResetResources::VoiceLease::require_live()const {
    if(retired_||output_->device_id()!=device_||output_->output_channels()!=2)
        throw std::runtime_error("Native sound voice lease has retired or lost its actual stereo output");
}
std::optional<SdlSoundCompletion> SourceRuntimeSoundResetResources::VoiceLease::completion()const {
    std::lock_guard lock(mutex_);
    require_live();return output_->completion(voice_);
}
SdlSoundVoiceStatus SourceRuntimeSoundResetResources::VoiceLease::status()const {
    std::lock_guard lock(mutex_);require_live();return output_->status(voice_);
}
void SourceRuntimeSoundResetResources::VoiceLease::acknowledge(std::uint64_t submission) {
    std::lock_guard lock(mutex_);
    require_live();output_->acknowledge(voice_,submission);
}
void SourceRuntimeSoundResetResources::VoiceLease::retire() {
    std::lock_guard lock(mutex_);
    if(retired_)return;require_live();output_->destroy_voice(voice_);retired_=true;
}
SourceRuntimeSoundResetResources::CallScope::CallScope(SourceRuntimeSoundResetResources* owner)
    :owner_(owner),exceptions_(std::uncaught_exceptions()) {
    if(owner_)owner_->impl_->require_owner();
}
SourceRuntimeSoundResetResources::CallScope::~CallScope() {
    if(owner_&&std::uncaught_exceptions()>exceptions_) {
        auto& p=*owner_->impl_;p.fault=true;
        while(p.exclusion_depth){p.exclusion.unlock();--p.exclusion_depth;}
    }
}
SourceRuntimeSoundResetResources::ControlLease SourceRuntimeSoundResetResources::control_lease()const {
    impl_->require_owner();return ControlLease(impl_);
}
std::uint64_t SourceRuntimeSoundResetResources::ControlLease::generation()const {
    impl_->require_owner();return impl_->generation;
}
std::uint32_t SourceRuntimeSoundResetResources::ControlLease::source_apu()const {
    impl_->require_owner();return impl_->apu;
}
void SourceRuntimeSoundResetResources::ControlLease::require_bank(const SourceRuntimeMemory& memory)const {
    const auto& p=*impl_;
    if(std::this_thread::get_id()!=p.thread)throw std::runtime_error("Native control bank is on a foreign owner thread");
    if(&memory!=&p.memory)throw std::invalid_argument("Native control lease belongs to another source memory bank");
    p.require_owner();
    if(p.apu&&memory.native_address(p.apu,0x540)!=p.native_apu)
        throw std::runtime_error("Native control APU backing changed beneath its retained owner");
}
void SourceRuntimeSoundResetResources::ControlLease::require_passive_context(const SourceRuntimeMemory& memory,std::uint32_t fs)const {
    require_bank(memory);const auto& p=*impl_;
    if(fs!=p.fs||!p.resets||p.exclusion_depth||memory.view(p.fs+0x24,1)[0]!=std::byte{})
        throw std::invalid_argument("Native control source call lacks its actual passive FS context");
}
void SourceRuntimeSoundResetResources::ControlLease::require_output(const SdlSoundPlatform& output)const {
    const auto& p=*impl_;
    if(std::this_thread::get_id()!=p.thread)throw std::runtime_error("Native control output is on a foreign owner thread");
    p.require_owner();
    if(p.output.get()!=&output||output.device_id()!=p.device||output.output_channels()!=2)
        throw std::invalid_argument("Native control lease belongs to another actual output object or device");
}
std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease> SourceRuntimeSoundResetResources::ControlLease::voice(std::uint16_t index)const {
    impl_->require_owner();(void)impl_->voice(index);return impl_->voices.at(index).lease;
}
void SourceRuntimeSoundResetResources::ControlLease::fence_retirement()const {
    auto& p=*impl_;
    if(std::this_thread::get_id()!=p.thread)
        throw std::runtime_error("Native control retirement fence is on a foreign owner thread");
    if(p.running_reset)throw std::runtime_error("Cannot retire a native control resource during source reset");
    std::unique_lock lock(p.queue_mutex);
    p.queue_changed.wait(lock,[&]{return p.commands.empty()&&!p.active_command;});
}
void SourceRuntimeSoundResetResources::ControlLease::run(std::uint64_t generation,const std::function<void()>& action)const {
    auto& p=*impl_;p.require_owner();
    if(!p.resets||p.generation!=generation||p.running_reset||!action)
        throw std::runtime_error("Native frontend job has no live matching reset generation");
    try{p.execute(action);}catch(...){p.fault=true;throw;}
}
std::uint32_t SourceRuntimeSoundResetResources::ControlLease::voice_head(unsigned list)const {
    auto& p=*impl_;p.require_owner();if(list>=p.voice_lists.size())throw std::invalid_argument("Native voice dispatch list is unknown");
    const auto& rows=p.voice_lists[list];return rows.empty()?0xffffu:rows.front();
}
void SourceRuntimeSoundResetResources::ControlLease::voice_head(std::uint64_t generation,unsigned list,std::uint32_t value)const {
    auto& p=*impl_;p.require_owner();
    if(list>=p.voice_lists.size()||(value!=0xffffu&&(value>=256||!p.voices.contains(value))))
        throw std::invalid_argument("Native voice dispatch head lacks an actual owned binding");
    run(generation,[&]{auto& rows=p.voice_lists[list];
        if(value==0xffffu){rows.clear();return;}
        const auto at=std::ranges::find(rows,std::uint16_t(value));
        if(at==rows.end())throw std::invalid_argument("Native voice is not in this dispatch list");
        rows.erase(rows.begin(),at);
    });
}
void SourceRuntimeSoundResetResources::ControlLease::retire_dispatch(std::uint64_t generation,std::uint16_t index)const {
    auto& p=*impl_;p.require_owner();(void)p.voice(index);
    run(generation,[&]{auto& row=p.voices.at(index);auto parameters=row.parameters;parameters.paused=true;
        p.output->parameters(row.lease->native_voice(),parameters);row.parameters=std::move(parameters);row.admitted=false;
        for(auto& list:p.voice_lists)std::erase(list,index);
    });
}
bool SourceRuntimeSoundResetResources::ControlLease::try_exclusion(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi)const {
    if(call.instruction!=0x383d88&&call.instruction!=0x383d9d)return false;
    auto& p=*impl_;p.require_owner();
    if(&bank!=&p.memory||abi.fs_base!=p.fs||!p.resets)
        throw std::invalid_argument("Native frontend exclusion lost its actual reset/thread owner");
    p.synchronization(call,abi);return true;
}
bool SourceRuntimeSoundResetResources::ControlLease::frontend_privileged_access()const {
    impl_->require_owner();return impl_->frontend_privileged_locked;
}
void SourceRuntimeSoundResetResources::ControlLease::frontend_privileged_access(std::uint64_t generation,bool enabled)const {
    auto& p=*impl_;p.require_owner();run(generation,[&]{p.frontend_privileged_locked=enabled;});
}
void SourceRuntimeSoundResetResources::ControlLease::abort_transaction()const {
    auto& p=*impl_;
    if(std::this_thread::get_id()!=p.thread)throw std::runtime_error("Native frontend abort is on another owner thread");
    p.fault=true;while(p.exclusion_depth){p.exclusion.unlock();--p.exclusion_depth;}
}
}
