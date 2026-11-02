#include "halo2/native_tu15_agent_cleanup.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t address){
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
void cleanup(SourcePhysicsContactMemory& memory,std::uint32_t child,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15AgentCleanupServices& services){
    const auto table=word(memory,child);
    const NativeTu15ContactVirtualOwner owner{word(memory,table+0x14u)};
    if(!services.cleanup_child)throw std::runtime_error("Native TU1.5 agent cleanup requires its current child owner");
    services.cleanup_child({child},owner,fp);
}
void dispose(SourcePhysicsContactMemory& memory,std::uint32_t object,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15AgentCleanupServices& services){
    const auto table=word(memory,object);
    const NativeTu15ContactVirtualOwner owner{word(memory,table)};
    if(!services.dispose)throw std::runtime_error("Native TU1.5 agent cleanup requires its current deleting destructor");
    services.dispose(object,owner,1u,fp);
}
}
void native_tu15_cleanup_contact_agent(SourcePhysicsContactMemory& memory,
    NativeTu15AgentCleanupKind kind,NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15AgentCleanupServices& services){
    const auto self=agent.identity;
    if(kind==NativeTu15AgentCleanupKind::records_2f3100){
        auto cursor=word(memory,self+0xcu);const auto count=word(memory,self+0x10u),end=cursor+count*12u;
        while(cursor!=end){const auto child=word(memory,cursor+8u);if(child)cleanup(memory,child,fp,services);cursor+=12u;}
    }else if(kind==NativeTu15AgentCleanupKind::pointers_2f7a20){
        auto count=word(memory,self+0x10u);std::uint32_t index{};
        if(less(0u,count))do{
            const auto data=word(memory,self+0xcu),child=word(memory,data+index*4u);
            cleanup(memory,child,fp,services);
            count=word(memory,self+0x10u);++index;
        }while(less(index,count));
    }else throw std::runtime_error("Unqualified native TU1.5 agent cleanup owner");
    dispose(memory,self,fp,services);
}
void native_tu15_dispose_contact_reference(SourcePhysicsContactMemory& memory,
    std::uint32_t object,NativeTu15PhysicsFrameFp& fp,const NativeTu15AgentCleanupServices& services){
    if(object)dispose(memory,object,fp,services);
}
} // namespace halo2
