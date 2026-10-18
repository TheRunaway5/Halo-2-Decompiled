// Included inside namespace halo2. This owns native scratch resources, not
// an MCPX register bank. All published values come from original CPU stores.
const SourceRuntimeSoundDmaResources::Allocation&
SourceRuntimeSoundDmaResources::gp_descriptor(std::uint32_t descriptor)const{
    require_owner();
    for(const auto& [source,entry]:allocations_)if(entry.source_descriptor==descriptor){
        const auto& row=resolve(source);
        if(word(memory_,descriptor)!=row.source||word(memory_,descriptor+4)!=row.physical||
           word(memory_,descriptor+8)!=row.requested)
            throw std::runtime_error("Native GP descriptor lost its original resource publication");
        return row;
    }
    throw std::invalid_argument("Native GP descriptor has no retained allocation");
}
bool SourceRuntimeSoundDmaResources::dispatch_gp_scratch_translation(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi){
    if(call.instruction!=0x38e66f)return false;
    require_owner();if(&bank!=&memory_)throw std::invalid_argument("Native GP prefix translation uses another bank");
    const auto sp=abi.gp[4];
    // Only the actual38DD54 ->38E6DF ->38E668 page-cursor chain. Other
    // translation callers retain their owner and never get a guessed result.
    if(word(memory_,sp+16)!=0x38e6f9||word(memory_,sp+48)!=0x38ddc0)return false;
    if(call.tail||call.target!=0x800000ad||word(memory_,sp)!=0x38e675||
       !memory_.is_read_only(0x41b7bc,4)||word(memory_,0x41b7bc)!=call.target)
        throw std::invalid_argument("Native GP prefix translation lost its original import/frame");
    const auto& row=gp_descriptor(0x391420);const auto pointer=word(memory_,sp+4);
    if(pointer<row.source||std::uint64_t(pointer)-row.source>=row.requested)
        throw std::invalid_argument("Native GP prefix translation is outside its actual descriptor");
    abi.gp[0]=row.physical+(pointer-row.source);abi.gp[4]+=8;return true;
}
std::uint32_t SourceRuntimeSoundDmaResources::gp_page_source(std::uint32_t context,std::uint32_t physical)const{
    if(physical&0xfff)throw std::invalid_argument("Native GP SGE has an unaligned physical page");
    for(const auto& [source,entry]:allocations_){
        if(physical>=entry.physical&&std::uint64_t(physical)-entry.physical+4096<=entry.capacity){
            const auto& row=gp_descriptor(entry.source_descriptor);
            return row.source+(physical-row.physical);
        }
    }
    for(const auto& [source,entry]:gp_pools_){
        const auto& row=resolve_gp_pool(source);
        if(physical>=row.physical&&std::uint64_t(physical)-row.physical+4096<=row.capacity){
            if(!entry.locked||row.context!=context)
                throw std::runtime_error("Native GP SGE lost its locked context-owned page");
            return row.source+(physical-row.physical);
        }
    }
    throw std::out_of_range("Native GP SGE has no retained prefix or pool page");
}
std::vector<SourceRuntimeSoundDmaResources::GpScratchSpan>
SourceRuntimeSoundDmaResources::gp_scratch_spans(const GpScratchBinding& binding,
    std::uint32_t offset,std::size_t bytes)const{
    require_owner();if(!binding.entries)throw std::runtime_error("Native GP scratch has no published count");
    const auto& table=gp_descriptor(0x391430);
    memory_.validate_mutable_view(binding.context,0x24);
    if(word(memory_,binding.context+8)!=1||word(memory_,binding.context+0xc)!=0x391430||
       word(memory_,binding.context+0x10)!=0x391420||table.source!=binding.table_source||
       table.physical!=binding.table_physical||table.requested!=binding.table_bytes||
       *binding.entries>table.requested/8)
        throw std::runtime_error("Native GP scratch lost its actual context/table binding");
    const auto extent=std::uint64_t(*binding.entries)*4096;
    if(!bytes||offset>extent||bytes>extent-offset)
        throw std::out_of_range("Native GP scratch request exceeds its published SGE count");
    std::vector<GpScratchSpan> result;
    while(bytes){
        const auto page=offset/4096,within=offset&0xfff;
        const auto at=table.source+page*8;
        if(word(memory_,at+4))throw std::invalid_argument("Native GP SGE high word is unsupported");
        const auto source=gp_page_source(binding.context,word(memory_,at));
        const auto count=std::min<std::size_t>(bytes,4096-within);
        result.push_back({source+within,count});bytes-=count;offset+=std::uint32_t(count);
    }
    return result;
}
void SourceRuntimeSoundDmaResources::publish_gp_scratch(std::uint32_t context,std::uint32_t instruction,
    std::uint32_t address,std::uint32_t value){
    require_owner();memory_.validate_mutable_view(context,0x24);
    const auto& table=gp_descriptor(0x391430);(void)gp_descriptor(0x391420);
    if(word(memory_,context+8)!=1||word(memory_,context+0xc)!=0x391430||
       word(memory_,context+0x10)!=0x391420)
        throw std::invalid_argument("Native GP publication has no actual GP context descriptors");
    if(instruction==0x38dde3&&address==0xfe802040){
        if(value!=table.physical)throw std::invalid_argument("Native GP table base differs from its retained physical resource");
        gp_scratch_=GpScratchBinding{context,table.source,table.physical,table.requested,{}};
        ++gp_pool_epoch_;return;
    }
    if(address!=0xfe8020d4||(instruction!=0x38dded&&instruction!=0x38de4d&&instruction!=0x38df31)||
       !gp_scratch_||gp_scratch_->context!=context)
        throw std::invalid_argument("Native GP count publication has no owned original store/base");
    const auto total=word(memory_,context),reserved=word(memory_,context+4);
    if(reserved>total||total!=table.requested/8)
        throw std::invalid_argument("Native GP reservation differs from its original table resource");
    const auto expected=std::uint64_t(total-reserved)+(instruction==0x38df31?word(memory_,context+0x1c)/4096:0u);
    if(value!=expected||value>table.requested/8)
        throw std::invalid_argument("Native GP published count differs from its actual CPU context");
    auto next=*gp_scratch_;next.entries=value;
    // Validate every row before publishing any new resource state. A count
    // is not accepted merely because its integer fits the table allocation.
    if(value)(void)gp_scratch_spans(next,0,std::size_t(value)*4096);
    gp_scratch_=next;++gp_pool_epoch_;
}
std::optional<SourceRuntimeSoundDmaResources::GpScratchBinding>
SourceRuntimeSoundDmaResources::gp_scratch_binding()const{
    require_owner();return gp_scratch_;
}
void SourceRuntimeSoundDmaResources::read_gp_scratch(std::uint32_t offset,std::span<std::byte> out)const{
    require_owner();if(!gp_scratch_)throw std::runtime_error("Native GP scratch base is unpublished");
    const auto spans=gp_scratch_spans(*gp_scratch_,offset,out.size());
    for(const auto& span:spans){memory_.read(span.source,out.first(span.bytes));out=out.subspan(span.bytes);}
}
void SourceRuntimeSoundDmaResources::write_gp_scratch(std::uint32_t offset,std::span<const std::byte> input){
    require_owner();if(!gp_scratch_)throw std::runtime_error("Native GP scratch base is unpublished");
    const auto spans=gp_scratch_spans(*gp_scratch_,offset,input.size());
    try{
        for(const auto& span:spans){memory_.write(span.source,input.first(span.bytes));input=input.subspan(span.bytes);}
        ++gp_pool_epoch_;
    }catch(...){fault_=true;throw;}
}
