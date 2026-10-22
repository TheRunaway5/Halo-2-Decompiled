#include "halo2/native_tu15_object_storage.hpp"
#include <cstdio>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
std::string string(const SourceRuntimeMemory& memory,std::uint32_t address) {
    std::string value;
    for(unsigned i=0;i<256;++i) {
        const auto ch=std::to_integer<unsigned char>(memory.view(address+i,1)[0]);
        if(!ch)return value;
        value.push_back(char(ch));
    }
    throw std::invalid_argument("Storage constructor CRT string exceeds its owned extent");
}
}
bool NativeTu15StorageFormatting::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x32ba80u)return false;
    // The existing arena module reports its authenticated canonical site;
    // both forms resolve to the same TU1.5 call and actual return address.
    const bool site=call.instruction==0x11f39fu || call.instruction==0x11e0ffu;
    if(&bank!=&memory_ || std::this_thread::get_id()!=thread_ || !site || call.tail)
        throw std::invalid_argument("Storage constructor CRT caller has a foreign bank, thread or site");
    const auto sp=abi.gp[4];
    if(word(memory_,sp)!=0x11f3a4u || word(memory_,sp+4)!=abi.gp[6] || word(memory_,sp+8)!=abi.gp[7]-1u ||
        word(memory_,sp+16)!=sp+0x1cu || word(memory_,sp+8)!=255u)
        throw std::invalid_argument("Storage constructor CRT caller frame differs");
    const auto destination=word(memory_,sp+4),capacity=word(memory_,sp+8),format=word(memory_,sp+12);
    const auto pattern=string(memory_,format);
    if(!((format==0x461944u && pattern=="cluster %s") || (format==0x461938u && pattern=="%s cluster") ||
         (format==0x466e00u && pattern=="%s reference")))
        throw std::invalid_argument("Storage constructor CRT format is not an authored operand");
    const auto value=string(memory_,word(memory_,word(memory_,sp+16)));
    memory_.native_address(destination,capacity);
    std::array<char,255> output{};
    const auto count=std::snprintf(output.data(),output.size(),pattern.c_str(),value.c_str());
    if(count<0 || std::size_t(count)>=output.size())
        throw std::runtime_error("Storage constructor CRT formatting did not fit its real destination");
    memory_.write(destination,std::as_bytes(std::span(output).first(std::size_t(count)+1)));
    abi.gp[0]=std::uint32_t(count);abi.gp[4]+=4; ++calls_;
    return true;
}
}
