// Original creation epoch/latch and per-object stamp fields; no live state.
{
constexpr std::array<std::array<std::uint32_t,2>,3> fields{{
    {0x4eabc8u,1u},{0x4eabccu,4u},{0x4eabd0u,8192u}}};
auto owned=[&](std::uint32_t address) {
    try {(void)memory.view(address,1);return true;}
    catch(const std::out_of_range&){return false;}
};
// Check every existing owner before admitting missing fields. Existing
// mutable values and backing lifetimes belong to their original producers.
for(const auto& [address,count]:fields)for(std::uint32_t i=0;i<count;++i)
    if(owned(address+i) && memory.is_read_only(address+i,1))
        throw std::invalid_argument("Original property creation storage is read-only");
for(const auto& [address,count]:fields)for(std::uint32_t i=0;i<count;) {
    if(owned(address+i)){++i;continue;}
    const auto first=i++;
    while(i<count && !owned(address+i))++i;
    memory.mount(address+first,std::vector<std::byte>(i-first),SourceRuntimeMemory::Access::ReadWrite,
        "Original TU1.5 loader-zero property creation epoch/latch/object stamps");
}
}
