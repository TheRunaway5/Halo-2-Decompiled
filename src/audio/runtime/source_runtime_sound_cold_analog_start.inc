// Included after real Endpoint and ConsumerLease definitions.
struct SourceRuntimeSoundColdAnalogStart::Impl {
    SourceRuntimeMemory& memory;
    const std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> channels;
    const std::shared_ptr<SourceRuntimeSoundPcmBuffers> pcm;
    const std::shared_ptr<SourceRuntimeSoundCodecRoute> codec;
    SourceRuntimeSoundResetResources::ControlLease control;
    SourceRuntimeSoundCodecEventPort& route;
    const std::thread::id thread=std::this_thread::get_id();
    SourceRuntimeSoundPcmBuffers::Binding previous;
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> old_receipts;
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::ConsumerLease> consumer;
    std::shared_ptr<const std::vector<float>> samples;
    std::vector<std::byte> raw;
    std::uint64_t resets{},successor{};
    bool attempted{},started{},returned{},stopping{},paused{},stopped{},fault{},cancelled{};
    Impl(SourceRuntimeMemory& m,std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> c,
        std::shared_ptr<SourceRuntimeSoundPcmBuffers> p,std::shared_ptr<SourceRuntimeSoundCodecRoute> k,
        SourceRuntimeSoundResetResources::ControlLease owner,SourceRuntimeSoundCodecEventPort& r)
        :memory(m),channels(std::move(c)),pcm(std::move(p)),codec(std::move(k)),control(std::move(owner)),route(r){
        control.require_bank(memory);if(!channels||!pcm||!codec)throw std::invalid_argument("Cold analog start lacks its actual owners");
        channels->require_control(memory,control);codec->require_control(memory,control);
        route.require_start_admission(memory,control,*codec,*pcm,*channels);
        if(channels->cold_start_owner_)throw std::runtime_error("Cold analog source start already has a retained owner");
        const auto view=pcm->buffer(SourceRuntimeSoundPcmBuffers::Kind::Analog);previous=view.binding;
        if(word(memory,previous.channel+4)||word(memory,previous.channel)!=0||word(memory,previous.channel+8)!=channels->controller_||
            word(memory,previous.channel+0xc)||word(memory,previous.channel+0x10)||word(memory,previous.channel+0x14)||
            word(memory,channels->controller_+8)!=1)throw std::invalid_argument("Cold analog source start is not an unstarted default descriptor");
        const auto endpoint=channels->endpoints_.at(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);
        {std::lock_guard lock(endpoint->mutex);const auto status=endpoint->output->status(endpoint->voice);
            if(endpoint->consumer||endpoint->current||endpoint->last||!endpoint->bound||!endpoint->resets||
                endpoint->generation!=previous.endpoint_generation||!status.paused||status.device_removed)
                throw std::runtime_error("Cold analog source start lacks a genuinely stopped native endpoint");resets=endpoint->resets;}
        old_receipts=channels->endpoint(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);
        raw.assign(view.bytes.begin(),view.bytes.end());std::vector<std::int16_t> source(4096);
        for(unsigned i=0;i<source.size();++i){const auto bits=std::uint16_t(std::to_integer<unsigned>(raw[i*2])|(std::to_integer<unsigned>(raw[i*2+1])<<8));source[i]=std::bit_cast<std::int16_t>(bits);}
        samples=std::make_shared<const std::vector<float>>(codec->convert_native(SourceRuntimeSoundCodecRoute::Kind::Analog,source));
    }
    void require_thread()const{if(std::this_thread::get_id()!=thread)throw std::runtime_error("Cold analog source start requires its creating owner thread");}
    void require_live()const{require_thread();if(fault||stopped||cancelled)throw std::runtime_error("Cold analog source start is faulted, stopped or cancelled");channels->require_control(memory,control);codec->require_control(memory,control);}
    void validate_prepared()const{
        const auto prepared=pcm->prepared_borrowers_.find(SourceRuntimeSoundPcmBuffers::Kind::Analog);
        if(prepared!=pcm->prepared_borrowers_.end()&&!prepared->second.expired())throw std::runtime_error("Cold source start cannot replace an active prepared-range borrower");
        const auto view=pcm->buffer(SourceRuntimeSoundPcmBuffers::Kind::Analog);
        if(view.binding.endpoint_generation!=previous.endpoint_generation||word(memory,previous.channel+4)||
            word(memory,previous.channel)!=0||word(memory,previous.channel+8)!=channels->controller_||
            word(memory,previous.channel+0xc)||word(memory,previous.channel+0x10)||word(memory,previous.channel+0x14)||
            word(memory,channels->controller_+8)!=1||!std::ranges::equal(view.bytes,raw))
            throw std::invalid_argument("Cold analog prepared source bytes/descriptor branch changed before execution");
        const auto e=channels->endpoints_.at(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);
        std::lock_guard lock(e->mutex);const auto state=e->output->status(e->voice);
        if(e->generation!=previous.endpoint_generation||e->resets!=resets||e->consumer||e->current||e->last||!e->bound||
            !state.paused||state.device_removed)throw std::runtime_error("Cold analog prepared native endpoint changed before source execution");
    }
    void frame(const SourcePhysicsWorldRuntimeAbi& abi,std::uint32_t returned,std::uint32_t arg)const{
        require_thread();control.require_passive_context(memory,abi.fs_base);memory.view(abi.gp[4],8);if(abi.gp[1]!=previous.channel||word(memory,abi.gp[4])!=returned||word(memory,abi.gp[4]+4)!=arg)
            throw std::invalid_argument("Cold analog source caller/channel/argument frame differs");
    }
};
SourceRuntimeSoundColdAnalogStart::SourceRuntimeSoundColdAnalogStart(SourceRuntimeMemory& m,
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> c,std::shared_ptr<SourceRuntimeSoundPcmBuffers> p,
    std::shared_ptr<SourceRuntimeSoundCodecRoute> k,SourceRuntimeSoundResetResources::ControlLease owner,SourceRuntimeSoundCodecEventPort& r)
    :impl_(std::make_unique<Impl>(m,std::move(c),std::move(p),std::move(k),std::move(owner),r)){impl_->channels->cold_start_owner_=this;}
SourceRuntimeSoundColdAnalogStart::~SourceRuntimeSoundColdAnalogStart(){
    auto& p=*impl_;if(std::this_thread::get_id()!=p.thread)std::terminate();if(p.attempted&&!p.stopped&&!p.cancelled)abort();
    if(p.channels->cold_start_owner_==this)p.channels->cold_start_owner_=nullptr;
}
std::uint32_t SourceRuntimeSoundColdAnalogStart::read_halted(std::uint32_t at,unsigned n)const{
    const auto& p=*impl_;p.require_live();if(!p.attempted||p.started||p.stopping||at!=0xfec00116||n!=1)
        throw std::runtime_error("Unsupported warm/after-span/source status remains required");
    const auto e=p.channels->endpoints_.at(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);
    std::lock_guard lock(e->mutex);const auto status=e->output->status(e->voice);
    if(e->generation!=p.previous.endpoint_generation||e->resets!=p.resets||e->consumer||!status.paused||status.device_removed)
        throw std::runtime_error("Cold source status lost its actually stopped endpoint generation");
    // This is the exact owned native halted lifecycle observation, not MMIO.
    return 1;
}
void SourceRuntimeSoundColdAnalogStart::write_run(std::uint32_t at,std::span<const std::byte> bytes){
    auto& p=*impl_;p.require_live();if(!p.attempted||p.started||p.stopping||at!=0xfec0011b||bytes.size()!=1||bytes[0]!=std::byte{0x1d})
        throw std::runtime_error("Unsupported source-start run action remains required");
    const auto e=p.channels->endpoints_.at(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);
    {std::lock_guard lock(e->mutex);const auto status=e->output->status(e->voice);
        if(e->generation!=p.previous.endpoint_generation+1||e->resets!=p.resets+1||e->consumer||e->resetting||e->faulted||e->admission_closed||
            !e->bound||e->current||e->last||!status.paused||status.device_removed||status.pending_packets||status.completed_packets||
            e->descriptors.native!=p.previous.native_descriptors||e->descriptors.source!=p.previous.descriptor_source||
            e->descriptors.physical!=word(p.memory,p.previous.channel+0x1c)||e->descriptors.channel!=p.previous.channel)
            throw std::runtime_error("Cold start successor lacks its completed real reset/binding or retains old receipts");
        p.successor=e->generation;}
    const auto& old=p.pcm->bindings_.at(SourceRuntimeSoundPcmBuffers::Kind::Analog);
    if(old.endpoint_generation!=p.previous.endpoint_generation||old.native_channel!=p.previous.native_channel||old.native_descriptors!=p.previous.native_descriptors||
        old.native_buffer!=p.previous.native_buffer||old.descriptor_source!=p.previous.descriptor_source||old.buffer_source!=p.previous.buffer_source||
        old.buffer_physical!=p.previous.buffer_physical||old.bytes!=p.previous.bytes||old.native_voice!=p.previous.native_voice||
        word(p.memory,p.previous.descriptor_source)!=p.previous.buffer_physical||word(p.memory,p.previous.descriptor_source+4)!=0xc0001000||
        p.memory.view(p.previous.channel+0x24,2)[0]!=std::byte{1}||p.memory.view(p.previous.channel+0x24,2)[1]!=std::byte{}||
        !std::ranges::equal(p.memory.view(p.previous.buffer_source,0x2000),p.raw))
        throw std::runtime_error("Cold start successor differs from its retained exact descriptor/source bytes");
    // Allocate the actual successor handle before publishing its new epoch.
    auto successor=p.channels->endpoint(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);
    auto binding=p.previous;binding.endpoint_generation=p.successor;
    p.pcm->endpoints_.at(SourceRuntimeSoundPcmBuffers::Kind::Analog)=std::move(successor);
    p.pcm->bindings_.at(SourceRuntimeSoundPcmBuffers::Kind::Analog)=binding;
    p.consumer=p.channels->claim_default_analog_consumer(p.pcm,p.samples);
    p.consumer->domain_->source_started=true;p.route.adopt_source_start(p.consumer);
    p.consumer->submit_and_start();p.started=true;
}
void SourceRuntimeSoundColdAnalogStart::did_pause(){
    auto& p=*impl_;p.require_thread();if(!p.stopping||!p.started||p.paused)throw std::runtime_error("Cold source stop lacks its exact genuine native pause action");
    if(!p.consumer->receipt_owner()->status().paused)throw std::runtime_error("Source stop did not actually pause its native endpoint");
    p.route.cancel_source_start();p.paused=true;
}
void SourceRuntimeSoundColdAnalogStart::registered_shutdown_completed(SourceRuntimeSoundCodecEventPort& route,
    const std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::ConsumerLease>& consumer){
    auto& p=*impl_;p.require_thread();
    if(&route!=&p.route||consumer!=p.consumer||!p.attempted||!p.started||!p.returned||p.fault)
        throw std::runtime_error("Registered source cancellation lost its exact retained start owner");
    if(p.stopped)return;
    p.control.require_bank(p.memory);const auto state=route.state();const auto native=consumer->receipt_owner()->status();
    if(state.connected||state.shutdown_registered||state.pending||state.faulted||!native.paused||native.pending_packets||native.device_removed)
        throw std::runtime_error("Whole registered source cancellation failed actual native closure");
    // This notification is issued only after whole original38F443 returned
    // through its authenticated route. Cleared source bits alone cannot close
    // the run owner, and registered cancellation is not a source38F05C return.
    p.cancelled=true;
}
void SourceRuntimeSoundColdAnalogStart::start(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    auto& p=*impl_;p.require_live();if(p.attempted||p.started||p.channels->cold_start_)
        throw std::runtime_error("Cold analog start rejects warm/repeated/new-cycle source admission");
    p.frame(abi,0x38ecf2,0xffffffff);if(abi.gp[6]!=p.control.source_apu()+8)throw std::invalid_argument("Cold analog start lost its authentic constructor subobject context");
    p.route.require_start_admission(p.memory,p.control,*p.codec,*p.pcm,*p.channels);
    p.validate_prepared();
    p.attempted=true;p.channels->cold_start_=this;
    try{p.channels->run_cpu(0x38f0aa,abi,fp,services,observe);p.channels->cold_start_=nullptr;
        if(!p.started||p.successor!=p.previous.endpoint_generation+1||p.consumer->receipt_owner()->status().paused)
            throw std::runtime_error("Whole original cold analog start failed actual run/successor ownership");
        p.returned=true;
    }catch(...){p.channels->cold_start_=nullptr;abort();throw;}
}
void SourceRuntimeSoundColdAnalogStart::stop(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    auto& p=*impl_;p.require_live();if(!p.started||p.stopping||p.channels->cold_start_)
        throw std::runtime_error("Cold analog stop has no unique retained source run");
    p.frame(abi,0x38f1c3,0);
    if(word(p.memory,p.previous.channel)!=0||word(p.memory,p.previous.channel+8)!=p.channels->controller_||
        word(p.memory,p.previous.channel+0xc)||word(p.memory,p.previous.channel+0x10)||word(p.memory,p.previous.channel+0x14)||
        word(p.memory,p.previous.channel+0x18)!=p.previous.descriptor_source||p.memory.view(p.previous.channel+0x24,2)[0]!=std::byte{1}||
        p.memory.view(p.previous.channel+0x24,2)[1]!=std::byte{})throw std::runtime_error("Unsupported source-stop channel/descriptor branch remains required");
    {const auto endpoint=p.channels->endpoints_.at(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);std::lock_guard lock(endpoint->mutex);
        if(endpoint->generation!=p.successor||endpoint->consumer!=p.consumer.get()||endpoint->current||endpoint->last||!endpoint->bound)
            throw std::runtime_error("Source-stop ownership changed before source execution");}
    p.stopping=true;p.channels->cold_start_=this;
    try{p.channels->run_cpu(0x38f05c,abi,fp,services,observe);p.channels->cold_start_=nullptr;
        if(!p.paused||!p.consumer->receipt_owner()->status().paused||p.memory.view(p.previous.channel+0x24,2)[0]!=std::byte{}||
            p.memory.view(p.previous.channel+0x24,2)[1]!=std::byte{})throw std::runtime_error("Whole original source stop failed actual native/descriptor closure");
        p.stopped=true;
    }catch(...){p.channels->cold_start_=nullptr;abort();throw;}
}
std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> SourceRuntimeSoundColdAnalogStart::previous_receipt_owner()const{impl_->require_thread();return impl_->old_receipts;}
SourceRuntimeSoundColdAnalogStart::State SourceRuntimeSoundColdAnalogStart::state()const{
    const auto& p=*impl_;p.require_thread();bool running=false;if(p.started&&!p.stopped&&!p.fault&&!p.cancelled){const auto state=p.consumer->receipt_owner()->status();running=!state.paused&&!state.device_removed&&!p.route.state().faulted;}
    return {p.previous.endpoint_generation,p.successor,p.attempted,p.returned,running,p.stopped,p.fault||p.channels->fault_||p.route.state().faulted,p.cancelled};
}
void SourceRuntimeSoundColdAnalogStart::abort()noexcept{
    auto& p=*impl_;if(std::this_thread::get_id()!=p.thread)std::terminate();if(p.fault)return;p.fault=true;
    p.channels->abort();p.route.abort();
}
