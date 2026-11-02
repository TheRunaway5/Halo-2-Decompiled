#include "halo2/native_tu15_convex_agent_factory.hpp"
#include "halo2/native_tu15_agent_cleanup_children.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned n=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(8u*k);return v;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned n=4){std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(8u*k));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(observer_){e.before.resize(n);memory_.read(p,e.before);e.written.assign(b.begin(),b.begin()+n);}memory_.write(p,std::span(b).first(n));if(observer_)observer_(e);}
};
std::uint32_t input(const std::function<std::uint32_t()>&f){if(!f)throw std::runtime_error("Native TU1.5 convex agent requires its current formal input");return f();}
}
NativeTu15ContactAgent native_tu15_initialize_convex_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,const NativeTu15ConvexAgentArguments&args,NativeTu15ConvexAgentScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ConvexAgentServices&s,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto p=agent.identity,user_data=input(args.user_data);m.word(0x2f832au,p+8u,user_data);
    const auto second=input(args.second_payload),first=input(args.first_payload);m.word(0x2f8333u,p,0x41cd08u);
    const NativeTu15ContactComponent second_shape{m.word(second)};
    const auto second_transform=m.word(second+8u),first_transform=m.word(first+8u);
    const NativeTu15ContactComponent first_shape{m.word(first)};
    native_contact_detail::UnwindGuard base([&]{native_tu15_destroy_contact_agent_base(memory,agent,observe);});base.arm();
    NativeTu15InverseGeometryMemory inverse(memory,scratch.relative_transform,first_transform+0x20u,second_transform+0x20u,observe);
    native_tu15_compose_inverse_affine_geometry(inverse,fp);
    const auto table=m.word(second_shape.identity);const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};
    if(!s.shape_class)throw std::runtime_error("Native TU1.5 convex agent requires its current shape class owner");
    const auto kind=s.shape_class(second_shape,owner,fp);
    if(kind==5u)native_tu15_initialize_triangle_contact_cache(memory,p+0xcu,first_shape,second_shape,scratch.relative_transform,scratch.cache,fp,s.cache,observe);
    else native_tu15_initialize_convex_contact_cache(memory,p+0xcu,first_shape,second_shape,scratch.relative_transform,scratch.cache,fp,s.cache,observe);
    m.word(0x2f838cu,p+0x18u,0u);m.word(0x2f8394u,p+0x2cu,0xbf800000u);base.disarm();return agent;
}
NativeTu15ContactAgent native_tu15_create_convex_contact_agent(SourcePhysicsContactMemory&memory,
    const NativeTu15ConvexAgentArguments&args,NativeTu15ConvexAgentScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ConvexAgentServices&s,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x10u);
    if(!s.allocation.allocate)throw std::runtime_error("Native TU1.5 convex agent requires its actual allocation owner");
    const NativeTu15ContactAgent agent{s.allocation.allocate({allocator,owner,256u,0x1au},fp)};
    native_contact_detail::UnwindGuard storage([&]{native_tu15_release_contact_agent(memory,agent,fp,s.allocation);});
    m.word(0x2e4edbu,agent.identity+4u,256u,2);
    const auto user_data=input(args.user_data),second=input(args.second_payload),first=input(args.first_payload);storage.arm();
    const NativeTu15ConvexAgentArguments current{[first]{return first;},[second]{return second;},[user_data]{return user_data;}};
    native_tu15_initialize_convex_contact_agent(memory,agent,current,scratch,fp,s,observe);
    m.word(0x2e4f07u,agent.identity,0x41c370u);m.word(0x2e4f0du,agent.identity+0xf0u,0u);storage.disarm();return agent;
}
NativeTu15ContactAgent native_tu15_create_gsk_contact_agent(SourcePhysicsContactMemory&memory,
    const NativeTu15ConvexAgentArguments&args,NativeTu15ConvexAgentScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ConvexAgentServices&s,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x10u);
    if(!s.allocation.allocate)throw std::runtime_error("Native TU1.5 GSK agent requires its actual allocation owner");
    const NativeTu15ContactAgent agent{s.allocation.allocate({allocator,owner,240u,0x1au},fp)};
    native_contact_detail::UnwindGuard storage([&]{native_tu15_release_contact_agent(memory,agent,fp,s.allocation);});
    m.word(0x2f83d8u,agent.identity+4u,240u,2);
    const auto user_data=input(args.user_data),second=input(args.second_payload),first=input(args.first_payload);storage.arm();
    const NativeTu15ConvexAgentArguments current{[first]{return first;},[second]{return second;},[user_data]{return user_data;}};
    const auto result=native_tu15_initialize_convex_contact_agent(memory,agent,current,scratch,fp,s,observe);
    storage.disarm();return result;
}
} // namespace halo2
