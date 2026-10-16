#include "halo2/source_runtime_sound_codec_event_port.hpp"
#include "halo2/source_runtime_sound_cold_analog_start.hpp"
#include "halo2/source_sound_codec_events_tu15.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cerrno>
#include <exception>
#include <map>
#include <system_error>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace halo2 {namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p,unsigned n=4){const auto bytes=m.view(p,n);std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(bytes[i])<<(8*i);return v;}
void put(SourceRuntimeMemory& m,std::uint32_t site,std::uint32_t p,unsigned n,std::uint32_t v,const SourcePhysicsContactRecordObserver& observe){
    SourcePhysicsContactRecordStore row{site,p,std::vector<std::byte>(n),std::vector<std::byte>(n)};m.read(p,row.before);
    for(unsigned i=0;i<n;++i)row.written[i]=std::byte(v>>(8*i));m.write(p,row.written);if(observe)observe(row);
}
NativeRuntimeAddress owned_controller(SourceRuntimeMemory& m,const SourceRuntimeSoundResetResources::ControlLease& c,std::uint32_t p){c.require_bank(m);return m.native_address(p,0x3c);}
std::mutex claim_mutex;std::map<std::uint64_t,const void*> claims;std::atomic<std::uint64_t> next_identity{1};
}
struct SourceRuntimeSoundCodecEventPort::Impl {
    SourceRuntimeMemory& memory;
    const std::shared_ptr<SdlSoundPlatform> output;
    SourceRuntimeSoundResetResources::ControlLease control;
    const std::shared_ptr<SourceRuntimeSoundCodecRoute> codec;
    const std::shared_ptr<SourceRuntimeSoundPcmBuffers> pcm;
    SourceProcessShutdownRegistry& registry;
    const std::thread::id thread=std::this_thread::get_id();
    const std::uint64_t generation,identity;
    const std::uint32_t controller,cookie;
    const NativeRuntimeAddress native_controller;
    int event=-1,shutdown_event=-1,dispatcher=-1;
    bool source_start_mode{};
    bool claimed{},initialized{},connected{},registered{},registration_attempted{},shutdown_connected{},pending{},shutdown_pending{},retired{},fault{},running{},channel_ack{},global_ack{};
    std::uint32_t pc{};std::uint64_t delivered{},cancelled{};
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::ConsumerLease> consumer;
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> receipts;
    std::optional<SdlSoundCompletion> success;
    struct Memory final:SourcePhysicsContactMemory {
        Impl& p;explicit Memory(Impl& o):p(o){}
        void read(std::uint32_t at,std::span<std::byte> bytes)const override {
            p.require_live();
            if(at>=0xfec00000){const auto v=p.read_status(at,unsigned(bytes.size()));for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(v>>(8*i));return;}
            p.pcm->pages_->cpu_memory().read(at,bytes);
        }
        void write(std::uint32_t at,std::span<const std::byte> bytes)override {
            p.require_live();
            if(at>=0xfec00000){std::uint32_t v{};if(bytes.size()!=2&&bytes.size()!=4)throw std::invalid_argument("Unowned native codec acknowledgement width");for(unsigned i=0;i<bytes.size();++i)v|=std::to_integer<unsigned>(bytes[i])<<(8*i);p.write_status(at,unsigned(bytes.size()),v);return;}
            p.pcm->pages_->cpu_memory().write(at,bytes);
        }
    } cpu_memory{*this};
    Impl(SourceRuntimeMemory& m,std::shared_ptr<SdlSoundPlatform> o,SourceRuntimeSoundResetResources::ControlLease c,
        std::shared_ptr<SourceRuntimeSoundCodecRoute> k,std::shared_ptr<SourceRuntimeSoundPcmBuffers> b,SourceProcessShutdownRegistry& r)
        :memory(m),output(std::move(o)),control(std::move(c)),codec(std::move(k)),pcm(std::move(b)),registry(r),
         generation(control.generation()),identity(next_identity.fetch_add(1)),controller(control.source_apu()+0x1c),
         cookie(0xf9070000u+std::uint32_t(identity)),native_controller(owned_controller(m,control,controller)) {
        control.require_bank(memory);if(!output||!codec||!pcm||!registry.backs(memory)||identity>=0x10000)
            throw std::invalid_argument("Native codec event route lacks its actual bank/resources or cookie capacity");
        control.require_output(*output);codec->require_control(memory,control);
        if(&pcm->memory_!=&memory||!pcm->control_.same_owner(control)||pcm->controller_!=controller||native_controller.value<=0xffffffffu)
            throw std::invalid_argument("Native codec event route belongs to another PCM bank/worker/controller");
        pcm->require_live();
        {std::lock_guard lock(claim_mutex);if(!claims.emplace(native_controller.value,this).second)
            throw std::runtime_error("Actual native codec completion domain is already owned");claimed=true;}
        try{dispatcher=epoll_create1(EPOLL_CLOEXEC);if(dispatcher<0)throw std::system_error(errno,std::generic_category(),"create codec completion dispatcher");}
        catch(...){release_claim();throw;}
    }
    ~Impl(){if(event>=0)close(event);if(shutdown_event>=0)close(shutdown_event);if(dispatcher>=0)close(dispatcher);release_claim();}
    void release_claim()noexcept{std::lock_guard lock(claim_mutex);const auto at=claims.find(native_controller.value);if(at!=claims.end()&&at->second==this)claims.erase(at);claimed=false;}
    void require_thread()const{if(std::this_thread::get_id()!=thread)throw std::runtime_error("Native codec event route requires its creating owner thread");}
    void require_live()const{
        require_thread();if(fault||retired)throw std::runtime_error("Native codec event route is faulted or retired");
        control.require_bank(memory);control.require_output(*output);codec->require_control(memory,control);
        if(control.generation()!=generation||memory.native_address(controller,0x3c)!=native_controller||!registry.backs(memory))
            throw std::runtime_error("Native codec event route lost its actual bank/worker generation");
        std::lock_guard lock(claim_mutex);const auto at=claims.find(native_controller.value);
        if(!claimed||at==claims.end()||at->second!=this)throw std::runtime_error("Native codec event route lost its exact native claim");
    }
    void frame(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,const SourcePhysicsWorldRuntimeAbi& abi,
        std::uint32_t target,std::uint32_t slot,std::uint32_t returned,unsigned bytes)const{
        require_thread();if(&bank!=&memory)throw std::invalid_argument("Native codec event request belongs to another source bank");
        require_live();if(call.tail||call.target!=target||word(memory,slot)!=target)
            throw std::invalid_argument("Native codec event source import/bank differs");
        memory.view(abi.gp[4],bytes);if(word(memory,abi.gp[4])!=returned)
            throw std::invalid_argument("Native codec event source return frame differs");
    }
    bool ready(unsigned tag)const{
        std::array<epoll_event,2> events{};const auto n=epoll_wait(dispatcher,events.data(),events.size(),0);
        if(n<0)throw std::system_error(errno,std::generic_category(),"poll actual codec event dispatcher");
        return std::any_of(events.begin(),events.begin()+n,[&](const auto& e){return e.data.u64==tag;});
    }
    void add(int descriptor,unsigned tag){epoll_event e{};e.events=EPOLLIN;e.data.u64=tag;if(epoll_ctl(dispatcher,EPOLL_CTL_ADD,descriptor,&e)<0)throw std::system_error(errno,std::generic_category(),"connect actual codec event resource");}
    void remove(int descriptor){if(epoll_ctl(dispatcher,EPOLL_CTL_DEL,descriptor,nullptr)<0)throw std::system_error(errno,std::generic_category(),"disconnect actual codec event resource");}
    static void signal(int descriptor){const std::uint64_t one=1;if(write(descriptor,&one,8)!=8)throw std::system_error(errno,std::generic_category(),"signal actual codec event resource");}
    static void take(int descriptor){std::uint64_t count{};if(read(descriptor,&count,8)!=8||count!=1)throw std::runtime_error("Native codec route lost its actual unique event receipt");}
    void cancel_consumer(){if(consumer)consumer->cancel();control.fence_retirement();if(pending){++cancelled;pending=false;}success.reset();channel_ack=global_ack=false;}
    void disconnect(){if(pending&&ready(1))take(event);cancel_consumer();if(connected){remove(event);connected=false;}}
    void unregister(){
        if(shutdown_connected){remove(shutdown_event);shutdown_connected=false;}
        if(registration_attempted){registry.registration(controller+0xc,false);registration_attempted=false;}
        registered=false;shutdown_pending=false;
    }
    bool authentic_success()const{
        if(!consumer||!success||!receipts)return false;const auto c=receipts->completion();return c&&c->voice==success->voice&&
            c->channel_context==success->channel_context&&c->packet_context==success->packet_context&&c->submission==success->submission&&
            c->status==SdlSoundCompletionStatus::Success&&c->pcm_frames==2048;
    }
    std::uint32_t read_status(std::uint32_t at,unsigned bytes)const{
        if(!running||!pending||!authentic_success())throw std::runtime_error("Codec source status lacks its actual descriptor Success receipt");
        if(at==0xfec00130&&bytes==4&&(pc==0x38f254||pc==0x38f30d))return 0x40;
        if(at==0xfec00116&&bytes==2&&(pc==0x38f299||pc==0x38f2d6))return 8;
        throw std::runtime_error("Unowned codec source status/descriptor branch remains required");
    }
    void write_status(std::uint32_t at,unsigned bytes,std::uint32_t v){
        if(!running||!pending||!authentic_success())throw std::runtime_error("Codec source acknowledgement lacks its actual descriptor Success receipt");
        if(pc==0x38f2d6&&at==0xfec00116&&bytes==2&&v==8&&!channel_ack){channel_ack=true;return;}
        if(pc==0x38f30d&&at==0xfec00130&&bytes==4&&v==0x40&&channel_ack&&!global_ack){global_ack=true;return;}
        throw std::runtime_error("Unowned codec source cause acknowledgement remains required");
    }
    void run(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
        const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
        if(running)throw std::runtime_error("Native codec callback reentry is unsupported");running=true;auto scoped=services;
        scoped.instruction=[&](auto site){pc=site;if(services.instruction)services.instruction(site);};
        scoped.required_owner=[&](auto&,const auto& call,auto& cpu,const auto& record){
            if(pcm->pages_->try_exclusion(call,cpu))return;
            if(call.instruction==0x38f3f7){
                frame(memory,call,cpu,0x80000064,0x41b7dc,0x38f3fd,8);
                if(word(memory,cpu.gp[4]+4)!=0x391208||!connected)throw std::invalid_argument("Native codec disconnect lacks its exact live registration");
                disconnect();cpu.gp[0]=(cpu.gp[0]&0xffffff00u)|1;cpu.gp[4]+=8;return;
            }
            if(call.instruction==0x38f420){
                frame(memory,call,cpu,0x8000002f,0x41b670,0x38f426,12);
                if(word(memory,cpu.gp[4]+4)!=controller+0xc||word(memory,cpu.gp[4]+8)||!registered)
                    throw std::invalid_argument("Native codec shutdown removal lost its exact record");
                unregister();cpu.gp[4]+=12;return;
            }
            if(call.target==0x38f395){
                const auto index=word(memory,cpu.gp[4]+4);if(index>=2||cpu.gp[1]!=controller)
                    throw std::invalid_argument("Native codec shutdown channel retirement differs");
                pcm->retire(index?SourceRuntimeSoundPcmBuffers::Kind::Spdif:SourceRuntimeSoundPcmBuffers::Kind::Analog);
            }
            if(call.target==0x38f395){
                if(services.required_owner){services.required_owner(memory,call,cpu,record);return;}
                pcm->channels_->run_cpu(call.target,cpu,fp,scoped,record);return;
            }
            if(call.target==0x38edf4){pcm->channels_->run_cpu(call.target,cpu,fp,scoped,record);return;}
            if(services.required_owner){services.required_owner(memory,call,cpu,record);return;}
            throw std::runtime_error("Unowned original codec callback resource remains required");
        };
        try{source_run_sound_codec_events_tu15(cpu_memory,entry,abi,fp,scoped,observe);running=false;}
        catch(...){running=false;throw;}
    }
};
SourceRuntimeSoundCodecEventPort::SourceRuntimeSoundCodecEventPort(SourceRuntimeMemory& m,std::shared_ptr<SdlSoundPlatform> o,
    SourceRuntimeSoundResetResources::ControlLease c,std::shared_ptr<SourceRuntimeSoundCodecRoute> k,
    std::shared_ptr<SourceRuntimeSoundPcmBuffers> p,SourceProcessShutdownRegistry& r)
    :impl_(std::make_unique<Impl>(m,std::move(o),std::move(c),std::move(k),std::move(p),r)){}
SourceRuntimeSoundCodecEventPort::~SourceRuntimeSoundCodecEventPort(){if(impl_&&std::this_thread::get_id()!=impl_->thread)std::terminate();if(impl_)try{retire();}catch(...){std::terminate();}}
bool SourceRuntimeSoundCodecEventPort::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsWorldRuntimeAbi& abi,const SourcePhysicsContactRecordObserver& observe){
    auto& p=*impl_;p.require_thread();const auto sp=abi.gp[4];
    if(call.instruction==0x38f4c4){
        p.frame(bank,call,abi,0x8000002c,0x41b7a8,0x38f4ca,12);const auto out=word(p.memory,sp+8);
        if(p.event>=0||word(p.memory,sp+4)!=6||out!=abi.gp[5]+8||word(p.memory,p.controller+8)!=1)
            throw std::invalid_argument("Native codec route request differs from authentic IRQ6 caller");p.memory.validate_mutable_view(out,1);
        try{p.event=eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);if(p.event<0)throw std::system_error(errno,std::generic_category(),"allocate actual codec completion endpoint");
            put(p.memory,call.instruction,out,1,0,observe);abi.gp[0]=p.cookie;abi.gp[4]=sp+12;return true;}catch(...){abort();throw;}
    }
    if(call.instruction==0x38f4de){
        p.frame(bank,call,abi,0x8000006d,0x41b7b4,0x38f4e4,32);
        if(p.event<0||p.initialized||word(p.memory,sp+4)!=0x391208||word(p.memory,sp+8)!=0x38f378||word(p.memory,sp+12)!=p.controller||
            word(p.memory,sp+16)!=p.cookie||(word(p.memory,sp+20)&255)||word(p.memory,sp+24)||word(p.memory,sp+28)!=1||word(p.memory,0x391208)||word(p.memory,0x39120c))
            throw std::invalid_argument("Native codec initializer lost its actual endpoint/callback/domain");
        p.memory.validate_mutable_view(0x391208,4);p.memory.validate_mutable_view(0x39120c,4);
        try{p.initialized=true;put(p.memory,call.instruction,0x391208,4,0x38f378,observe);put(p.memory,call.instruction,0x39120c,4,p.controller,observe);abi.gp[4]=sp+32;return true;}catch(...){abort();throw;}
    }
    if(call.instruction==0x38f4e5){
        p.frame(bank,call,abi,0x80000062,0x41b7b0,0x38f4eb,8);
        if(word(p.memory,sp+4)!=0x391208||!p.initialized||p.connected||word(p.memory,0x391208)!=0x38f378||word(p.memory,0x39120c)!=p.controller)
            throw std::invalid_argument("Native codec connection lacks its initialized endpoint");
        try{p.add(p.event,1);p.connected=true;abi.gp[0]=(abi.gp[0]&0xffffff00u)|1;abi.gp[4]=sp+8;return true;}catch(...){abort();throw;}
    }
    if(call.instruction==0x38f52f){
        p.frame(bank,call,abi,0x8000002f,0x41b670,0x38f535,12);
        if(word(p.memory,sp+4)!=p.controller+0xc||word(p.memory,sp+8)!=1||!p.connected||p.registered||word(p.memory,p.controller+0xc)!=0x38f443)
            throw std::invalid_argument("Native codec shutdown registration lacks its exact live callback record");
        p.memory.validate_mutable_view(p.controller+0xc,16);
        const auto prior=p.registry.records();if(std::ranges::any_of(prior,[&](const auto& row){return row.address==p.controller+0xc;}))
            throw std::invalid_argument("Codec shutdown source record is already linked by another registration");
        try{p.shutdown_event=eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);if(p.shutdown_event<0)throw std::system_error(errno,std::generic_category(),"allocate actual codec shutdown endpoint");
            p.add(p.shutdown_event,2);p.shutdown_connected=true;p.registration_attempted=true;p.registry.registration(p.controller+0xc,true,[&](const auto& row){if(observe)observe({call.instruction,row.address,row.before,row.written});});
            p.registered=true;abi.gp[4]=sp+12;return true;}catch(...){abort();throw;}
    }
    return false;
}
void SourceRuntimeSoundCodecEventPort::require_start_admission(SourceRuntimeMemory& m,const SourceRuntimeSoundResetResources::ControlLease& c,
    const SourceRuntimeSoundCodecRoute& codec,const SourceRuntimeSoundPcmBuffers& pcm,const SourceRuntimeSoundAc97ChannelReset& channels)const{
    const auto& p=*impl_;p.require_live();if(&m!=&p.memory||!c.same_owner(p.control)||&codec!=p.codec.get()||&pcm!=p.pcm.get()||
        &channels!=p.pcm->channels_.get()||!p.connected||!p.registered||p.consumer||p.pending||p.delivered||p.source_start_mode)
        throw std::invalid_argument("Cold analog source start lacks its identical unused registered codec owner");
    if(word(p.memory,0x391208)!=0x38f378||word(p.memory,0x39120c)!=p.controller||word(p.memory,p.controller+0xc)!=0x38f443||
        word(p.memory,0x41b7ac)!=0x800000a0||word(p.memory,0x41b6fc)!=0x800000a1)
        throw std::invalid_argument("Cold source start lost its actual registered callback or exclusion imports");
    const auto rows=p.registry.records();if(!std::ranges::any_of(rows,[&](const auto& row){return row.address==p.controller+0xc&&row.routine==0x38f443;}))
        throw std::invalid_argument("Cold source start lost its traversable native shutdown registration");
}
void SourceRuntimeSoundCodecEventPort::adopt_source_start(std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::ConsumerLease> c){
    auto& p=*impl_;p.require_live();if(!c||p.consumer||p.pending||p.delivered||!p.connected||!p.registered)throw std::runtime_error("Cold source-start publication lost its unique route");
    p.receipts=c->receipt_owner();p.consumer=std::move(c);p.source_start_mode=true;
}
void SourceRuntimeSoundCodecEventPort::cancel_source_start(){auto& p=*impl_;p.require_thread();if(!p.source_start_mode||!p.consumer)throw std::runtime_error("No source-start route is owned for cancellation");
    if(p.pending)Impl::take(p.event);p.cancel_consumer();
}
void SourceRuntimeSoundCodecEventPort::consume_default_analog(){
    auto& p=*impl_;p.require_live();if(!p.connected||!p.registered||p.consumer||p.pending||word(p.memory,p.controller+8)!=1)
        throw std::runtime_error("Native codec default descriptor has no unique registered consumer");
    const auto view=p.pcm->buffer(SourceRuntimeSoundPcmBuffers::Kind::Analog);std::vector<std::int16_t> source(4096);
    for(unsigned i=0;i<source.size();++i){const std::uint16_t bits=std::to_integer<unsigned>(view.bytes[i*2])|(std::to_integer<unsigned>(view.bytes[i*2+1])<<8);source[i]=std::bit_cast<std::int16_t>(bits);}
    auto converted=std::make_shared<const std::vector<float>>(p.codec->convert_native(SourceRuntimeSoundCodecRoute::Kind::Analog,source));
    p.consumer=p.pcm->channels_->claim_default_analog_consumer(p.pcm,std::move(converted));p.receipts=p.consumer->receipt_owner();
    try{p.consumer->submit_and_start();}catch(...){abort();throw;}
}
bool SourceRuntimeSoundCodecEventPort::collect_success(){
    auto& p=*impl_;p.require_thread();if(!p.consumer||p.pending)throw std::runtime_error("Native codec has no unique outstanding descriptor consumption");
    if(p.source_start_mode&&p.delivered)throw std::runtime_error("Further source-start descriptor cycles remain required");
    try{p.require_live();if(!(p.source_start_mode?p.consumer->observe_success_running():p.consumer->pause_after_success()))return false;p.success=p.consumer->completion();
        if(!p.authentic_success())throw std::runtime_error("Actual codec Success receipt changed before publication");
        Impl::signal(p.event);p.pending=true;return true;}catch(...){abort();throw;}
}
bool SourceRuntimeSoundCodecEventPort::dispatch(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    auto& p=*impl_;p.require_live();const auto sp=abi.gp[4];p.memory.view(sp,12);
    if(!p.connected||!p.registered||word(p.memory,sp+4)!=0x391208||word(p.memory,sp+8)!=p.controller||
        word(p.memory,0x391208)!=0x38f378||word(p.memory,0x39120c)!=p.controller)
        throw std::invalid_argument("Native codec registered ISR frame/resource differs before event consumption");
    if(!p.ready(1))return false;
    if(!p.pending||!p.authentic_success())throw std::runtime_error("Native codec event has no retained descriptor Success receipt");
    p.consumer->validate_delivery();
    if(!p.source_start_mode)(void)p.pcm->buffer(SourceRuntimeSoundPcmBuffers::Kind::Analog);const auto channel=word(p.memory,p.controller),before=word(p.memory,channel+4);
    try{Impl::take(p.event);p.run(0x38f378,abi,fp,services,observe);
        if(!p.channel_ack||!p.global_ack||!(abi.gp[0]&255)||abi.gp[4]!=sp+12||word(p.memory,channel+4)!=before+0x1000||
            p.memory.view(channel+0x24,2)[0]!=std::byte{1}||p.memory.view(channel+0x24,2)[1]!=std::byte{})
            throw std::runtime_error("Whole original codec ISR failed its actual descriptor/accounting postcondition");
        p.consumer->acknowledge();p.pending=false;p.success.reset();++p.delivered;return true;
    }catch(...){abort();throw;}
}
void SourceRuntimeSoundCodecEventPort::request_shutdown(){auto& p=*impl_;p.require_live();if(!p.registered||p.shutdown_pending)throw std::runtime_error("Native codec has no unique registered shutdown delivery");Impl::signal(p.shutdown_event);p.shutdown_pending=true;}
bool SourceRuntimeSoundCodecEventPort::dispatch_shutdown(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    auto& p=*impl_;p.require_live();p.memory.view(abi.gp[4],8);
    const auto records=p.registry.records();const auto row=std::find_if(records.begin(),records.end(),[&](const auto& r){return r.address==p.controller+0xc&&r.routine==0x38f443;});
    if(!p.registered||row==records.end()||word(p.memory,abi.gp[4]+4)!=p.controller+0xc||word(p.memory,p.controller+0xc)!=0x38f443||word(p.memory,p.controller+0x1c,2)==0x13)
        throw std::invalid_argument("Native codec shutdown frame/registration or optional DPC differs before delivery");
    if(!p.ready(2))return false;
    try{Impl::take(p.shutdown_event);p.shutdown_pending=false;p.run(0x38f443,abi,fp,services,observe);
        if(p.connected||p.registered||word(p.memory,0x391208)||word(p.memory,p.controller+0xc))
            throw std::runtime_error("Original registered codec shutdown did not finish disconnect/unregister");
        if(p.source_start_mode&&p.pcm->channels_->cold_start_owner_)
            p.pcm->channels_->cold_start_owner_->registered_shutdown_completed(*this,p.consumer);
        return true;
    }catch(...){abort();throw;}
}
std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> SourceRuntimeSoundCodecEventPort::receipt_owner()const{const auto& p=*impl_;p.require_thread();return p.receipts;}
SourceRuntimeSoundCodecEventPort::State SourceRuntimeSoundCodecEventPort::state()const{const auto& p=*impl_;p.require_thread();return {p.generation,p.identity,p.delivered,p.cancelled,p.cookie,p.controller,p.event,p.shutdown_event,p.dispatcher,p.initialized,p.connected,p.registered,p.pending,p.retired,p.fault,bool(p.consumer)};}
void SourceRuntimeSoundCodecEventPort::retire(){auto& p=*impl_;p.require_thread();if(p.retired)return;p.disconnect();p.unregister();p.control.fence_retirement();p.retired=true;p.release_claim();}
void SourceRuntimeSoundCodecEventPort::abort()noexcept{auto& p=*impl_;if(std::this_thread::get_id()!=p.thread)std::terminate();if(p.fault)return;p.fault=true;try{p.control.abort_transaction();p.disconnect();p.unregister();p.control.fence_retirement();
        for(auto* fd:{&p.event,&p.shutdown_event,&p.dispatcher})if(*fd>=0){close(*fd);*fd=-1;}
    }catch(...){std::terminate();}}
SourceRuntimeSoundCodecEventPort::CallScope::CallScope(SourceRuntimeSoundCodecEventPort* p):owner_(p),exceptions_(std::uncaught_exceptions()){if(owner_)owner_->impl_->require_live();}
SourceRuntimeSoundCodecEventPort::CallScope::~CallScope(){if(owner_&&std::uncaught_exceptions()>exceptions_)owner_->abort();}
}
