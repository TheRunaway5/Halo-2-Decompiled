// Included inside namespace halo2. Actual original type0 EP scratch resources;
// this synchronous page view does not implement EP status, execution or clocks.
std::vector<SourceRuntimeSoundDmaResources::GpScratchSpan>
SourceRuntimeSoundDmaResources::ep_scratch_spans(const EpScratchBinding& binding,
    std::uint32_t offset,std::size_t bytes)const {
    require_owner();
    if(!binding.entries)throw std::runtime_error("Native EP scratch has no published count");
    const auto& table=gp_descriptor(0x391450);const auto& data=gp_descriptor(0x391440);
    memory_.validate_mutable_view(binding.context,0x24);
    if(word(memory_,binding.context+8)!=0||word(memory_,binding.context+0xc)!=0x391450||
       word(memory_,binding.context+0x10)!=0x391440||table.source!=binding.table_source||
       table.physical!=binding.table_physical||table.requested!=binding.table_bytes||
       *binding.entries!=word(memory_,binding.context)||*binding.entries!=table.requested/8||
       std::uint64_t(*binding.entries)*4096>data.requested)
        throw std::runtime_error("Native EP scratch lost its actual context/table binding");
    const auto extent=std::uint64_t(*binding.entries)*4096;
    if(!bytes||offset>extent||bytes>extent-offset)
        throw std::out_of_range("Native EP scratch request exceeds its published SGE count");
    std::vector<GpScratchSpan> result;
    while(bytes) {
        const auto page=offset/4096,within=offset&0xfff,at=table.source+page*8;
        if(word(memory_,at+4)||word(memory_,at)!=data.physical+page*4096)
            throw std::invalid_argument("Native EP SGE lost its actual descriptor page/high word");
        const auto count=std::min<std::size_t>(bytes,4096-within);
        result.push_back({data.source+page*4096+within,count});bytes-=count;offset+=std::uint32_t(count);
    }
    return result;
}
void SourceRuntimeSoundDmaResources::publish_ep_scratch(std::uint32_t context,
    std::uint32_t instruction,std::uint32_t address,std::uint32_t value) {
    require_owner();memory_.validate_mutable_view(context,0x24);
    const auto& table=gp_descriptor(0x391450);(void)gp_descriptor(0x391440);
    if(word(memory_,context+8)!=0||word(memory_,context+0xc)!=0x391450||
       word(memory_,context+0x10)!=0x391440)
        throw std::invalid_argument("Native EP publication has no actual EP context descriptors");
    if(instruction==0x38ddf4&&address==0xfe802048) {
        if(value!=table.physical)throw std::invalid_argument("Native EP table base differs from its retained physical resource");
        ep_scratch_=EpScratchBinding{context,table.source,table.physical,table.requested,{}};
        ++gp_pool_epoch_;return;
    }
    if(instruction!=0x38ddfb||address!=0xfe8020dc||!ep_scratch_||ep_scratch_->context!=context)
        throw std::invalid_argument("Native EP count publication has no owned original store/base");
    if(value!=word(memory_,context)||value!=table.requested/8)
        throw std::invalid_argument("Native EP published count differs from its actual CPU context");
    auto next=*ep_scratch_;next.entries=value;
    if(value)(void)ep_scratch_spans(next,0,std::size_t(value)*4096);
    ep_scratch_=next;++gp_pool_epoch_;
}
std::optional<SourceRuntimeSoundDmaResources::EpScratchBinding>
SourceRuntimeSoundDmaResources::ep_scratch_binding()const {
    require_owner();return ep_scratch_;
}
void SourceRuntimeSoundDmaResources::read_ep_scratch(std::uint32_t offset,std::span<std::byte> out)const {
    require_owner();if(!ep_scratch_)throw std::runtime_error("Native EP scratch base is unpublished");
    const auto spans=ep_scratch_spans(*ep_scratch_,offset,out.size());
    for(const auto& span:spans){memory_.read(span.source,out.first(span.bytes));out=out.subspan(span.bytes);}
}
void SourceRuntimeSoundDmaResources::write_ep_scratch(std::uint32_t offset,std::span<const std::byte> input) {
    require_owner();if(!ep_scratch_)throw std::runtime_error("Native EP scratch base is unpublished");
    const auto spans=ep_scratch_spans(*ep_scratch_,offset,input.size());
    try {
        for(const auto& span:spans){memory_.write(span.source,input.first(span.bytes));input=input.subspan(span.bytes);}
        ++gp_pool_epoch_;
    }catch(...){fault_=true;throw;}
}
