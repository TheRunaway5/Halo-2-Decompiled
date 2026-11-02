#include "halo2/native_tu15_phantom_callback_agent.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t address)const {
        std::array<std::byte,4>bytes{};memory_.read(address,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    }
    void word(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        const auto written=std::span(bytes).first(width);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(observe_){event.before.resize(width);memory_.read(address,event.before);event.written.assign(written.begin(),written.end());}
        memory_.write(address,written);if(observe_)observe_(event);
    }
    std::uint32_t finish_parent_chain(std::uint32_t current,std::uint32_t parent)const {
        while(parent){current=parent;parent=word(current+12u);}return current;
    }
};
std::uint32_t input(const std::function<std::uint32_t()>&provider){
    if(!provider)throw std::runtime_error("Native phantom callback agent requires its current ordinary input");
    return provider();
}
}
std::uint32_t native_tu15_phantom_shape_class_22() noexcept{return 22u;}
NativeTu15ContactAgent native_tu15_initialize_phantom_callback_agent(
    SourcePhysicsContactMemory&memory,NativeTu15ContactAgent agent,
    const NativeTu15PhantomCallbackAgentArguments&arguments,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15PhantomCallbackAgentServices&services,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto object=agent.identity;
    const auto user=input(arguments.user_data);m.word(0x2f7842u,object+8u,user);
    const auto first=input(arguments.first);m.word(0x2f7849u,object,0x41cca4u);
    const auto first_parent=m.word(first+12u);
    native_contact_detail::UnwindGuard base([&]{m.word(0x2f7a10u,object,0x45edf0u);});
    base.arm(); // Actual state0 at2F7854, FuncInfo46BA90 ->2F7A10.
    const auto first_root=m.finish_parent_chain(first,first_parent);
    const auto second=input(arguments.second);m.word(0x2f786du,object+12u,first_root);
    const auto second_parent=m.word(second+12u);
    const auto second_root=m.finish_parent_chain(second,second_parent);
    m.word(0x2f7889u,object+16u,second_root);
    const auto first_shape=m.word(first),first_table=m.word(first_shape);
    const NativeTu15ContactVirtualOwner first_owner{m.word(first_table+20u)};
    if(!services.classify_shape)throw std::runtime_error("Native phantom callback requires its actual shape-class owner");
    const auto first_class=services.classify_shape({first_shape},first_owner,fp);
    m.word(0x2f7893u,object+28u,first_class);
    const auto second_shape=m.word(second),second_table=m.word(second_shape);
    const NativeTu15ContactVirtualOwner second_owner{m.word(second_table+20u)};
    const auto second_class=services.classify_shape({second_shape},second_owner,fp);
    m.word(0x2f78a1u,object+32u,second_class);
    base.disarm();return agent;
}
NativeTu15ContactAgent native_tu15_create_phantom_callback_agent(
    SourcePhysicsContactMemory&memory,const NativeTu15PhantomCallbackAgentArguments&arguments,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15PhantomCallbackAgentServices&services,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto allocator=m.word(0x48c938u),table=m.word(allocator);
    const auto allocation_owner=m.word(table+16u);
    if(!services.allocation.allocate)throw std::runtime_error("Native phantom callback agent requires its actual allocation owner");
    const NativeTu15ContactAgent agent{services.allocation.allocate({allocator,allocation_owner,36u,0x1au},fp)};
    m.word(0x2f78e9u,agent.identity+4u,36u,2);
    const auto user=input(arguments.user_data),second=input(arguments.second),first=input(arguments.first);
    const NativeTu15PhantomCallbackAgentArguments current{
        [&]{return first;},[&]{return second;},arguments.context,[&]{return user;}};
    native_contact_detail::UnwindGuard allocation([&]{native_tu15_release_contact_agent(memory,agent,fp,services.allocation);});
    allocation.arm(); // Actual state0 at2F7904; FuncInfo46BAB4 ->2F0380.
    native_tu15_initialize_phantom_callback_agent(memory,agent,current,fp,services,observe);
    const bool first_callback=m.word(agent.identity+28u)==22u;
    allocation.disarm(); // Actual2F7917: no allocation cleanup during enter.
    if(first_callback){
        const auto other_parent=m.word(second+12u),shape=m.word(first);
        const auto other_root=m.finish_parent_chain(second,other_parent);
        const auto self_parent=m.word(first+12u),self_root=m.finish_parent_chain(first,self_parent);
        const auto context=input(arguments.context),shape_table=m.word(shape);
        const NativeTu15ContactVirtualOwner enter_owner{m.word(shape_table+40u)};
        if(!services.enter)throw std::runtime_error("Native phantom callback requires its actual enter owner");
        services.enter({shape},enter_owner,{self_root},{other_root},{context},fp);
        m.word(0x2f795au,agent.identity+20u,shape);
    }
    if(m.word(agent.identity+32u)==22u){
        const auto other_parent=m.word(first+12u),shape=m.word(second);
        const auto other_root=m.finish_parent_chain(first,other_parent);
        const auto self_parent=m.word(second+12u),self_root=m.finish_parent_chain(second,self_parent);
        const auto context=input(arguments.context),shape_table=m.word(shape);
        const NativeTu15ContactVirtualOwner enter_owner{m.word(shape_table+40u)};
        if(!services.enter)throw std::runtime_error("Native phantom callback requires its actual enter owner");
        services.enter({shape},enter_owner,{self_root},{other_root},{context},fp);
        m.word(0x2f799bu,agent.identity+24u,shape);
    }
    return agent;
}
} // namespace halo2
