#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
    void bytes(std::uint32_t site,std::uint32_t p,std::span<const std::byte>value){SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(value.size());memory_.read(p,event.before);event.written.assign(value.begin(),value.end());}memory_.write(p,value);if(observer_)observer_(event);}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));bytes(site,p,std::span(b).first(width));}
    void maximum_vector(std::uint32_t site,std::uint32_t p){if(p&15u)throw std::runtime_error("Native TU1.5 contact agent requires aligned vector storage");constexpr std::array<std::byte,16>maximum{std::byte{0xff},std::byte{0xff},std::byte{0x7f},std::byte{0x7f},std::byte{0xff},std::byte{0xff},std::byte{0x7f},std::byte{0x7f},std::byte{0xff},std::byte{0xff},std::byte{0x7f},std::byte{0x7f},std::byte{0xff},std::byte{0xff},std::byte{0x7f},std::byte{0x7f}};bytes(site,p,maximum);}
};
}
std::uint32_t native_tu15_contact_child_class_23() noexcept{return 23;}
std::uint32_t native_tu15_contact_child_class_24() noexcept{return 24;}
NativeTu15ContactAgent native_tu15_initialize_contact_agent(SourcePhysicsContactMemory&memory,
 NativeTu15ContactAgent agent,std::uint32_t user_data,const SourcePhysicsContactRecordObserver&observer){
 Memory m(memory,observer);const auto p=agent.identity;
 m.word(0x2f362eu,p+8u,user_data);m.word(0x2f3631u,p,0x41cb58u);
 m.word(0x2f3647u,p+0xcu,0);m.word(0x2f364au,p+0x10u,0);m.word(0x2f364du,p+0x14u,0x80000000u);
 m.maximum_vector(0x2f3658u,p+0x30u);m.maximum_vector(0x2f365cu,p+0x20u);return agent;
}
void native_tu15_release_contact_agent(SourcePhysicsContactMemory&memory,NativeTu15ContactAgent agent,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&s){
 const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
 const auto allocator=m.word(0x48c938u),table=m.word(allocator),size=m.word(agent.identity+4u,2),owner=m.word(table+0x14u);
 if(!s.release)throw std::runtime_error("Native TU1.5 contact agent requires actual release ownership");
 s.release({allocator,owner,agent.identity,size,0x1au},fp);
}
NativeTu15ContactAgent native_tu15_create_contact_agent(SourcePhysicsContactMemory&memory,
 NativeTu15ContactAgentFactory kind,const NativeTu15ContactAgentFactoryArguments&arguments,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&s,const SourcePhysicsContactRecordObserver&observer){
 const bool first=kind==NativeTu15ContactAgentFactory::owner_303c10;
 if(!first&&kind!=NativeTu15ContactAgentFactory::owner_3040d0)throw std::runtime_error("Native TU1.5 contact agent factory owner is unqualified");
 Memory m(memory,observer);NativeTu15ContactAgent agent{};
 native_contact_detail::UnwindGuard cleanup([&]{native_tu15_release_contact_agent(memory,agent,fp,s);});
 const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x10u);
 if(!s.allocate)throw std::runtime_error("Native TU1.5 contact agent requires actual allocation ownership");
 agent.identity=s.allocate({allocator,owner,64u,0x1au},fp);
 m.word(first?0x303c38u:0x3040f8u,agent.identity+4u,0x40u,2);
 if(!arguments.user_data)throw std::runtime_error("Native TU1.5 contact agent requires its late user-data input");
 const auto user_data=arguments.user_data();cleanup.arm(); // Actual303C49/304109.
 native_tu15_initialize_contact_agent(memory,agent,user_data,observer);
 m.word(first?0x303c5au:0x30411au,agent.identity,first?0x41d01cu:0x41d03cu);
 cleanup.disarm();return agent;
}
NativeTu15ContactAgent native_tu15_create_contact_agent(SourcePhysicsContactMemory&memory,
 NativeTu15ContactAgentFactory kind,std::uint32_t user_data,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15ArrayGrowthServices&s,const SourcePhysicsContactRecordObserver&observer){
 return native_tu15_create_contact_agent(memory,kind,NativeTu15ContactAgentFactoryArguments{[user_data]{return user_data;}},fp,s,observer);
}
} // namespace halo2
