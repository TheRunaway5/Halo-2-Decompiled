// Included by the reviewed ChannelReset overlay after Endpoint definitions.
// The original source/runtime bank outlives every active consumer operation.
struct SourceRuntimeSoundAc97ChannelReset::ConsumerLease::Domain {
    std::shared_ptr<SourceRuntimeSoundPcmBuffers> pcm;
    std::shared_ptr<Endpoint> endpoint;
    std::shared_ptr<EndpointLease> receipts;
    SourceRuntimeSoundPcmBuffers::Binding binding;
    const std::thread::id thread=std::this_thread::get_id();
    const std::uint64_t control_generation,endpoint_generation,context;
    const std::shared_ptr<const std::vector<float>> samples;
    const std::vector<std::byte> source_bytes;
    ConsumerLease* owner{};
    bool submitted{},paused{},accounted{},cancelled{},source_started{};
    static std::atomic<std::uint64_t> next_context;
    Domain(std::shared_ptr<SourceRuntimeSoundPcmBuffers> p,std::shared_ptr<Endpoint> e,
        std::shared_ptr<EndpointLease> r,SourceRuntimeSoundPcmBuffers::Binding b,
        std::shared_ptr<const std::vector<float>> s,std::vector<std::byte> raw)
        :pcm(std::move(p)),endpoint(std::move(e)),receipts(std::move(r)),binding(b),
         control_generation(endpoint->control_generation),endpoint_generation(endpoint->generation),
         context(next_context.fetch_add(1)),samples(std::move(s)),source_bytes(std::move(raw)) {
        if(!context)throw std::overflow_error("Native codec consumer receipt context exhausted");
    }
    void require_thread()const {
        if(std::this_thread::get_id()!=thread)throw std::runtime_error("Native codec consumer requires its creating owner thread");
    }
    void require_binding()const {
        require_thread();if(cancelled||accounted)throw std::runtime_error("Native codec consumer is cancelled or already accounted");
        endpoint->control.require_bank(pcm->memory_);endpoint->control.require_output(*endpoint->output);pcm->require_live();
        if(endpoint->control.generation()!=control_generation||pcm->retired_||pcm->fault_||
            pcm->generation_!=control_generation||!pcm->control_.same_owner(endpoint->control))
            throw std::runtime_error("Native codec consumer lost its exact PCM/control generation");
        const auto at=pcm->bindings_.find(Kind::Analog);
        if(at==pcm->bindings_.end()||at->second.channel!=binding.channel||
            at->second.native_channel!=binding.native_channel||at->second.native_descriptors!=binding.native_descriptors||
            at->second.native_buffer!=binding.native_buffer||at->second.endpoint_generation!=endpoint_generation||
            at->second.descriptor_source!=binding.descriptor_source||at->second.buffer_source!=binding.buffer_source||
            at->second.buffer_physical!=binding.buffer_physical||at->second.bytes!=0x2000||
            at->second.native_voice!=binding.native_voice||pcm->channels_->endpoint(Kind::Analog)->native_voice()!=binding.native_voice)
            throw std::runtime_error("Native codec consumer descriptor ownership was replaced");
        auto& m=pcm->memory_;
        if(m.native_address(binding.channel,0x30)!=binding.native_channel||
            m.native_address(binding.descriptor_source,0x100)!=binding.native_descriptors||
            m.native_address(binding.buffer_source,0x2000)!=binding.native_buffer||
            word(m,pcm->controller_)!=binding.channel||word(m,binding.channel)!=0||word(m,binding.channel+8)!=pcm->controller_||
            word(m,pcm->controller_+8)!=1||word(m,binding.channel+0xc)||
            word(m,binding.channel+0x10)||word(m,binding.channel+0x14)||
            word(m,binding.channel+0x18)!=binding.descriptor_source||word(m,binding.channel+0x1c)!=endpoint->descriptors.physical||
            word(m,binding.descriptor_source)!=binding.buffer_physical||
            word(m,binding.descriptor_source+4)!=0xc0001000||
            m.view(binding.channel+0x24,2)[0]!=std::byte{1}||m.view(binding.channel+0x24,2)[1]!=std::byte{}||
            !std::ranges::equal(m.view(binding.buffer_source,0x2000),source_bytes))
            throw std::runtime_error("Native codec consumer source descriptor/PCM bytes changed beneath its retained pin");
        std::lock_guard lock(endpoint->mutex);
        endpoint->require_native();
        if(endpoint->consumer!=owner||endpoint->generation!=endpoint_generation||endpoint->faulted||
            endpoint->resetting||endpoint->admission_closed||!endpoint->bound||
            endpoint->current||endpoint->last||
            endpoint->descriptors.native!=binding.native_descriptors||endpoint->descriptors.source!=binding.descriptor_source||
            endpoint->descriptors.channel!=binding.channel||endpoint->descriptors.field!=0x18||endpoint->descriptors.requested!=0x100)
            throw std::runtime_error("Native codec consumer lost its exclusive endpoint generation");
        if(source_started&&submitted){const auto state=endpoint->output->status(endpoint->voice);
            if(state.paused||state.device_removed)throw std::runtime_error("Source-start delivery lost its actual retained running endpoint");}
    }
    bool success(const SdlSoundCompletion& c)const {
        return c.voice==binding.native_voice&&c.channel_context==binding.channel&&c.packet_context==context&&
            c.submission&&c.pcm_frames==2048&&c.status==SdlSoundCompletionStatus::Success;
    }
};
std::atomic<std::uint64_t> SourceRuntimeSoundAc97ChannelReset::ConsumerLease::Domain::next_context{1};
SourceRuntimeSoundAc97ChannelReset::ConsumerLease::ConsumerLease(std::unique_ptr<Domain> d):domain_(std::move(d)){}
SourceRuntimeSoundAc97ChannelReset::ConsumerLease::~ConsumerLease(){
    if(domain_&&std::this_thread::get_id()!=domain_->thread)std::terminate();
    if(domain_)try{cancel();}catch(...){std::terminate();}
}
std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::ConsumerLease>
SourceRuntimeSoundAc97ChannelReset::claim_default_analog_consumer(
    std::shared_ptr<SourceRuntimeSoundPcmBuffers> pcm,std::shared_ptr<const std::vector<float>> samples) {
    require_thread();require_live();
    if(!pcm||&pcm->memory_!=&memory_||pcm->channels_.get()!=this||!pcm->control_.same_owner(control_))
        throw std::invalid_argument("Native codec consumer lacks its identical PCM/channel/worker owner");
    const auto view=pcm->buffer(Kind::Analog);const auto& b=view.binding;
    if(b.bytes!=0x2000||b.endpoint_generation==0||!samples||samples->size()!=4096||
        word(memory_,controller_+8)!=1||word(memory_,b.channel+0xc)||word(memory_,b.channel+0x10)||word(memory_,b.channel+0x14))
        throw std::invalid_argument("Native codec consumer only admits the default immediate analog descriptor");
    std::vector<std::byte> raw(view.bytes.begin(),view.bytes.end());
    for(unsigned i=0;i<4096;++i){const std::uint16_t bits=std::uint16_t(std::to_integer<unsigned>(raw[i*2]))|
        std::uint16_t(std::to_integer<unsigned>(raw[i*2+1])<<8);const auto signed_sample=std::bit_cast<std::int16_t>(bits);
        if((*samples)[i]!=float(signed_sample)/32768.0f)throw std::invalid_argument("Native codec consumer PCM differs from its authenticated S16 descriptor bytes");}
    const auto endpoint=endpoints_.at(Kind::Analog);const auto receipts=this->endpoint(Kind::Analog);
    {std::lock_guard lock(endpoint->mutex);const auto status=endpoint->output->status(endpoint->voice);
        if(endpoint->consumer||endpoint->generation!=b.endpoint_generation||endpoint->current||endpoint->last||
            !status.paused||status.pending_packets||status.completed_packets)
            throw std::runtime_error("Native analog descriptor consumer is already owned or retains unrelated PCM");}
    // Deep-copy the converter result: callers cannot mutate the retained PCM
    // through another shared_ptr while the actual SDL callback consumes it.
    auto pin=std::make_shared<const std::vector<float>>(*samples);
    auto domain=std::make_unique<ConsumerLease::Domain>(pcm,endpoint,receipts,b,std::move(pin),std::move(raw));
    auto lease=std::shared_ptr<ConsumerLease>(new ConsumerLease(std::move(domain)));
    // All allocation precedes the final lock/publication. On rejection the
    // guard unwinds before the staged lease destructor can cancel/fence.
    {std::lock_guard lock(endpoint->mutex);const auto status=endpoint->output->status(endpoint->voice);
        if(endpoint->consumer||endpoint->generation!=b.endpoint_generation||endpoint->faulted||endpoint->resetting||
            endpoint->admission_closed||!endpoint->bound||endpoint->current||endpoint->last||
            !status.paused||status.pending_packets||status.completed_packets)
            throw std::runtime_error("Native analog descriptor consumer changed before exclusive publication");
        lease->domain_->owner=lease.get();endpoint->consumer=lease.get();}
    return lease;
}
void SourceRuntimeSoundAc97ChannelReset::ConsumerLease::validate_delivery()const{domain_->require_binding();}
void SourceRuntimeSoundAc97ChannelReset::ConsumerLease::submit_and_start(){
    auto& d=*domain_;d.require_binding();if(d.submitted)throw std::runtime_error("Native codec descriptor was already submitted");
    d.endpoint->control.run(d.control_generation,[&]{std::lock_guard lock(d.endpoint->mutex);
        if(d.endpoint->consumer!=this||d.endpoint->generation!=d.endpoint_generation||
            d.endpoint->current||d.endpoint->last)
            throw std::runtime_error("Native codec consumer claim changed before actual submission");
        const auto status=d.endpoint->output->status(d.endpoint->voice);
        if(!status.paused||status.pending_packets||status.completed_packets||status.device_removed)
            throw std::runtime_error("Native codec consumer cannot replace an active or unaccounted endpoint");
        if(d.endpoint->output->submit(d.endpoint->voice,{d.samples,0,2048,d.context})!=SdlSoundPlatform::Submission::Accepted)
            throw std::runtime_error("Actual native codec descriptor submission was backpressured");
        d.submitted=true;
        auto parameters=paused_stereo();parameters.paused=false;d.endpoint->output->parameters(d.endpoint->voice,parameters);
    });
}
bool SourceRuntimeSoundAc97ChannelReset::ConsumerLease::pause_after_success(){
    auto& d=*domain_;d.require_binding();if(!d.submitted)throw std::runtime_error("Native codec descriptor has not been submitted");
    const auto receipt=d.receipts->completion();if(!receipt)return false;
    if(!d.success(*receipt))throw std::runtime_error("Native codec descriptor lacks its exact actual Success receipt");
    d.endpoint->control.run(d.control_generation,[&]{std::lock_guard lock(d.endpoint->mutex);
        if(d.endpoint->consumer!=this||d.endpoint->generation!=d.endpoint_generation||
            d.endpoint->current||d.endpoint->last)
            throw std::runtime_error("Native codec consumer changed before actual post-consumption pause");
        d.endpoint->output->parameters(d.endpoint->voice,paused_stereo());
        const auto status=d.endpoint->output->status(d.endpoint->voice);
        if(!status.paused||status.pending_packets||status.completed_packets!=1)
            throw std::runtime_error("Native codec consumer failed actual one-descriptor quiescence");
    });d.paused=true;return true;
}
bool SourceRuntimeSoundAc97ChannelReset::ConsumerLease::observe_success_running(){
    auto& d=*domain_;d.require_binding();if(!d.source_started||!d.submitted)throw std::runtime_error("No retained source-start descriptor consumption exists");
    const auto receipt=d.receipts->completion();if(!receipt)return false;
    if(!d.success(*receipt))throw std::runtime_error("Source-start descriptor lacks its exact actual Success receipt");
    const auto state=d.receipts->status();if(state.paused||state.pending_packets||state.completed_packets!=1||state.device_removed)
        throw std::runtime_error("Source-start consumption lost its actual unpaused one-span lifetime");
    return true;
}
std::optional<SdlSoundCompletion> SourceRuntimeSoundAc97ChannelReset::ConsumerLease::completion()const{
    domain_->require_thread();return domain_->receipts->completion();
}
std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease>
SourceRuntimeSoundAc97ChannelReset::ConsumerLease::receipt_owner()const{
    domain_->require_thread();return domain_->receipts;
}
void SourceRuntimeSoundAc97ChannelReset::ConsumerLease::acknowledge(){
    auto& d=*domain_;d.require_binding();const auto receipt=d.receipts->completion();
    if((!d.paused&&!d.source_started)||!receipt||!d.success(*receipt))throw std::runtime_error("Native codec descriptor cannot acknowledge an unaccounted or unsuccessful receipt");
    d.receipts->acknowledge(receipt->submission);d.accounted=true;
}
void SourceRuntimeSoundAc97ChannelReset::ConsumerLease::cancel(){
    auto& d=*domain_;d.require_thread();if(d.cancelled)return;
    // Faulted shared control still has a real retirement fence. No source
    // metadata read or fake completion is needed to stop this exact lease.
    d.endpoint->control.fence_retirement();
    {std::lock_guard lock(d.endpoint->mutex);
        if(d.endpoint->consumer==this){
            if(!d.endpoint->output->status(d.endpoint->voice).device_removed)d.endpoint->output->parameters(d.endpoint->voice,paused_stereo());
            d.endpoint->output->flush(d.endpoint->voice);d.endpoint->consumer=nullptr;}}
    d.cancelled=true;
}
void SourceRuntimeSoundAc97ChannelReset::cancel_native_consumer(Kind kind){
    require_thread();const auto at=endpoints_.find(kind);if(at==endpoints_.end())return;
    ConsumerLease* owner{};{std::lock_guard lock(at->second->mutex);owner=at->second->consumer;}
    if(owner)owner->cancel();
}
