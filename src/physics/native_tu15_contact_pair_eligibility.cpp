#include "halo2/native_tu15_contact_pair_eligibility.hpp"
namespace halo2 {
namespace {
std::uint32_t owner(const SourcePhysicsContactMemory&memory,std::uint32_t dispatcher,
    std::uint32_t first,std::uint32_t second){
    const std::uint32_t index=first*32u+second;
    const std::uint32_t address=dispatcher+index*4u+0x18cu;
    std::array<std::byte,4>bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory&memory,std::uint32_t site,std::uint32_t result,
    bool value,const SourcePhysicsContactRecordObserver&observe){
    const std::array bytes{std::byte(value)};
    SourcePhysicsContactRecordStore event{site,result,{},{}};
    if(observe){event.before.resize(1);memory.read(result,event.before);event.written.assign(bytes.begin(),bytes.end());}
    memory.write(result,bytes);if(observe)observe(event);
}
}
std::uint32_t native_tu15_test_gsk_contact_pair(SourcePhysicsContactMemory&memory,
    std::uint32_t result,std::uint32_t dispatcher,std::uint32_t first,std::uint32_t second,
    const SourcePhysicsContactRecordObserver&observe){
    const bool eligible=owner(memory,dispatcher,first,second)==0x2f83b0u;
    publish(memory,eligible?0x3038e2u:0x3038e6u,result,eligible,observe);return result;
}
std::uint32_t native_tu15_test_contact_pair(SourcePhysicsContactMemory&memory,
    std::uint32_t result,std::uint32_t dispatcher,std::uint32_t first,std::uint32_t second,
    const SourcePhysicsContactRecordObserver&observe){
    if(owner(memory,dispatcher,first,second)==0x2e4eb0u){
        publish(memory,0x2e4f55u,result,true,observe);return result;
    }
    // Actual3038C0 rereads the current dispatch field, rather than consuming
    // the value tested by2E4F30. Preserve the second access/fault boundary.
    return native_tu15_test_gsk_contact_pair(memory,result,dispatcher,first,second,observe);
}
} // namespace halo2
