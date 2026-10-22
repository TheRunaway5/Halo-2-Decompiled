// Included inside namespace halo2 by the actual sound DMA resource owner.
// Original TU1.5 call sites/import ordinals and parent frames define this
// native ABI. Page tokens are an explicit host allocation policy, not a
// physical Xbox/MMU implementation. No GP program, SGE limit or ready word
// is initialized by these services.
const SourceRuntimeSoundDmaResources::GpPoolAllocation&
SourceRuntimeSoundDmaResources::resolve_gp_pool(std::uint32_t source)const{
    require_owner();const auto found=gp_pools_.find(source);
    if(found==gp_pools_.end())throw std::invalid_argument("Native GP pool allocation is not owned");
    const auto& row=found->second;
    if(memory_.native_address(row.source,row.capacity)!=row.native)
        throw std::runtime_error("Native GP pool lost its retained page backing");
    memory_.validate_mutable_view(row.source,row.capacity);return row;
}
std::vector<SourceRuntimeSoundDmaResources::GpPoolAllocation>
SourceRuntimeSoundDmaResources::gp_pool_allocations()const{
    require_owner();std::vector<GpPoolAllocation> result;result.reserve(gp_pools_.size());
    for(const auto& [source,row]:gp_pools_){(void)resolve_gp_pool(source);result.push_back(row);}return result;
}
void SourceRuntimeSoundDmaResources::read_gp_pool(std::uint32_t physical,std::span<std::byte> out)const{
    require_owner();if(out.empty())throw std::invalid_argument("Native GP page read has no extent");
    for(const auto& [source,entry]:gp_pools_){
        const auto& row=resolve_gp_pool(source);
        if(physical<row.physical)continue;const auto offset=std::uint64_t(physical)-row.physical;
        if(offset>=row.capacity||out.size()>row.capacity-offset)continue;
        if(!entry.locked)throw std::runtime_error("Native GP physical read requires its actual locked pages");
        memory_.read(row.source+std::uint32_t(offset),out);return;
    }
    throw std::out_of_range("Native GP physical read has no single retained pool range");
}
void SourceRuntimeSoundDmaResources::write_gp_pool(std::uint32_t physical,std::span<const std::byte> input){
    require_owner();if(input.empty())throw std::invalid_argument("Native GP page write has no extent");
    for(const auto& [source,entry]:gp_pools_){
        const auto& row=resolve_gp_pool(source);
        if(physical<row.physical)continue;const auto offset=std::uint64_t(physical)-row.physical;
        if(offset>=row.capacity||input.size()>row.capacity-offset)continue;
        if(!entry.locked)throw std::runtime_error("Native GP physical write requires its actual locked pages");
        try{memory_.write(row.source+std::uint32_t(offset),input);++gp_pool_epoch_;}
        catch(...){fault_=true;throw;}return;
    }
    throw std::out_of_range("Native GP physical write has no single retained pool range");
}
bool SourceRuntimeSoundDmaResources::dispatch_gp_pool(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi){
    const auto site=call.instruction;
    const bool allocate=site==0x387975,capacity=site==0x387982,free_capacity=site==0x3879b7;
    const bool destroy_unlock=site==0x38dd3b;
    const bool lock=site==0x38deea,unlock=site==0x38df40||destroy_unlock,address=site==0x38df0a,release=site==0x3879c5;
    if(!allocate&&!capacity&&!free_capacity&&!lock&&!unlock&&!address&&!release)return false;
    require_owner();if(&bank!=&memory_)throw std::invalid_argument("Native GP pool bank differs from its page owner");
    const auto sp=abi.gp[4];
    // The generic SDK allocation helpers have other callers too. They remain
    // with their existing owner; only the genuine38DE81 parent is admitted.
    if(allocate&&word(memory_,sp+16)!=0x38ded0)return false;
    if(capacity&&word(memory_,sp+12)!=0x38ded0)return false;
    const auto free_parent=free_capacity?word(memory_,sp+8):release?word(memory_,sp):0u;
    if((free_capacity||release)&&free_parent!=0x38df50&&free_parent!=0x38dd4e)return false;
    const bool destroying=destroy_unlock||free_parent==0x38dd4e;
    const auto slot=allocate?0x41b79cu:(capacity||free_capacity)?0x41b7ecu:
        (lock||unlock)?0x41b7c0u:address?0x41b7bcu:0x41b798u;
    const auto target=allocate?0x8000000fu:(capacity||free_capacity)?0x80000017u:
        (lock||unlock)?0x800000afu:address?0x800000adu:0x80000011u;
    const auto ret=(lock||unlock)&&!destroy_unlock?site+2u:site+6u;
    if(call.target!=target||word(memory_,slot)!=target||!memory_.is_read_only(slot,4)||
       call.tail!=release||(!release&&word(memory_,sp)!=ret))
        throw std::invalid_argument("Native GP pool call/import/return identity differs");
    //38DEBC/BD/BE/C2 copy four words beginning at context14, including
    // the otherwise unnamed word20. Retain that actual read footprint.
    const auto context=abi.gp[destroying?6:3];memory_.validate_mutable_view(context,0x24);
    if(allocate){
        const auto bytes=word(memory_,sp+4);
        // Large GP pools retain the actual native page allocation capacity.
        // Smaller/general SDK pools keep their existing allocator contract.
        if(bytes<0x1000||word(memory_,sp+8)!=0x61645344||
           word(memory_,sp+20)!=0x61645344||word(memory_,sp+24)!=bytes||word(memory_,sp+28)!=1||
           word(memory_,context+0x1c)!=bytes)
            throw std::invalid_argument("Native GP pool request differs from its original large DSad frame");
        // Share the actual DMA owner's first-fit namespace, preventing a
        // second independent allocator from reusing existing sound pages.
        const auto source=physical_.allocate({bytes,0x05000000,0xffffffffu,0x1000,4});
        if(!source){abi.gp[0]=0;abi.gp[4]+=12;return true;}
        try{
            const auto ownership=physical_.ownership_checkpoint();
            const auto allocation=std::ranges::find_if(ownership.allocations,[&](const auto& row){return row.base==source;});
            if(allocation==ownership.allocations.end())throw std::runtime_error("Native GP pool has no physical page owner");
            const auto native=memory_.native_address(source,allocation->bytes);
            if(native.value<=0xffffffffu)throw std::runtime_error("Native GP pool page backing is not native64");
            if(!gp_pools_.emplace(source,GpPoolAllocation{source,source&0x7fffffffu,bytes,allocation->bytes,context,native,false}).second)
                throw std::runtime_error("Native GP pool allocation identity collision");
        }catch(...){try{physical_.release(source);}catch(...){fault_=true;}throw;}
        ++gp_pool_epoch_;abi.gp[0]=source;abi.gp[4]+=12;return true;
    }
    const auto pointer=word(memory_,sp+4);
    if(address){
        auto found=gp_pools_.upper_bound(pointer);
        if(found==gp_pools_.begin())throw std::invalid_argument("Native GP physical query has no owned page");
        --found;const auto& row=resolve_gp_pool(found->first);const auto offset=std::uint64_t(pointer)-row.source;
        if(offset>=row.capacity||(offset&0xfff)||!row.locked||row.context!=context||abi.gp[0]!=pointer)
            throw std::invalid_argument("Native GP physical query is foreign, unlocked or outside its retained pages");
        abi.gp[0]=row.physical+std::uint32_t(offset);abi.gp[4]+=8;return true;
    }
    const auto row=resolve_gp_pool(pointer);
    if(row.context!=context)throw std::invalid_argument("Native GP pool request lost its original context");
    if(capacity||free_capacity){
        if((capacity&&abi.gp[6]!=pointer)||(free_capacity&&row.locked))
            throw std::invalid_argument("Native GP capacity/free request lost its allocation or unlock");
        abi.gp[0]=row.capacity;abi.gp[4]+=8;return true;
    }
    if(lock||unlock){
        const auto bytes=word(memory_,sp+8),flag=word(memory_,sp+12);
        if(bytes!=row.requested||flag!=(unlock?1u:0u)||row.locked!=unlock||(!destroy_unlock&&abi.gp[7]!=target)||
           ((lock||destroy_unlock)&&(word(memory_,context+0x14)!=pointer||word(memory_,context+0x1c)!=bytes)))
            throw std::invalid_argument("Native GP page lock/unlock lost its exact retained allocation");
        gp_pools_.at(pointer).locked=lock;++gp_pool_epoch_;abi.gp[4]+=16;return true;
    }
    if(row.locked)throw std::runtime_error("Native GP pool cannot release locked pages");
    physical_.release(pointer);gp_pools_.erase(pointer);++gp_pool_epoch_;abi.gp[4]+=8;return true;
}
