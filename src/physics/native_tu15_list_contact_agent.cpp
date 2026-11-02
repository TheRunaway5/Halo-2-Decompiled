#include "halo2/native_tu15_list_contact_agent.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;SourcePhysicsContactRecordObserver observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe = {}):memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory_.read(p,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4) {
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
        memory_.write(p,std::span(bytes).first(width));if(observe_)observe_(event);
    }
};
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::uint32_t input(const std::function<std::uint32_t()>& provider) {
    if(!provider)throw std::runtime_error("Native list contact requires its current ordinary input");
    return provider();
}
}
void native_tu15_destroy_contact_pointer_array(SourcePhysicsContactMemory& memory,
    NativeTu15ArrayHeader header,NativeTu15PhysicsFrameFp& fp,const NativeTu15ArrayGrowthServices& services) {
    Memory m(memory);const auto flags=m.word(header.identity+8u);
    if(flags&0x80000000u)return;
    const auto allocator=m.word(0x48c938u),table=m.word(allocator);
    const auto bytes=(flags&0x7fffffffu)*4u,data=m.word(header.identity);
    const auto owner=m.word(table+20u);
    if(!services.release)throw std::runtime_error("Native list contact requires its actual array release owner");
    services.release({allocator,owner,data,bytes,0x12u},fp);
}
NativeTu15ContactAgent native_tu15_initialize_list_contact_agent(
    SourcePhysicsContactMemory& memory,NativeTu15ContactAgent agent,
    const NativeTu15ListContactArguments& arguments,NativeTu15ListContactScratch scratch,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ListContactServices& services,
    const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory,observe);const auto object=agent.identity,header=object+12u;
    m.word(0x2f7d46u,object+8u,input(arguments.user_data));
    m.word(0x2f7d4eu,object,0x41ccc8u);
    native_contact_detail::UnwindGuard base([&]{m.word(0x2f7a10u,object,0x45edf0u);});
    base.arm(); // State0 at2F7D58; action37E940 ->2F7A10.
    m.word(0x2f7d5cu,header,header+12u);
    m.word(0x2f7d5eu,header+4u,0u);
    m.word(0x2f7d61u,header+8u,0x80000004u);
    const auto first=input(arguments.first),extra=m.word(first+8u),collection=m.word(first);
    m.word(0x2f7d71u,scratch.child_payload+12u,first);
    m.word(0x2f7d75u,scratch.child_payload+8u,extra);
    const auto table=m.word(collection);
    native_contact_detail::UnwindGuard array([&]{native_tu15_destroy_contact_pointer_array(memory,{header},fp,services.allocation);});
    array.arm(); // State1 at2F7D7D; action37E948 ->2E15C0.
    const NativeTu15ContactVirtualOwner count_owner{m.word(table+40u)};
    if(!services.child_count)throw std::runtime_error("Native list contact requires its real collection count owner");
    const auto count=services.child_count({collection},count_owner,fp);
    const auto capacity=m.word(header+8u)&0x7fffffffu;
    if(signed_word(capacity)<signed_word(count)) {
        auto desired=capacity+capacity;
        if(signed_word(count)>=signed_word(desired))desired=count;
        native_tu15_reserve_array(memory,{header},desired,4u,fp,services.allocation,observe);
    }
    m.word(0x2f7dabu,header+4u,count);
    if(signed_word(m.word(object+16u))>0) {
        const auto context=input(arguments.context);std::uint32_t index{};
        do {
            const auto children=m.word(collection+12u),shape=m.word(children+index*8u);
            const auto filter=m.word(context+4u);
            m.word(0x2f7dcau,scratch.child_payload,shape);
            const NativeTu15ContactPayload current_first{input(arguments.first)},current_second{input(arguments.second)};
            m.word(0x2f7ddeu,scratch.child_payload+4u,index);
            const auto filter_table=m.word(filter);const NativeTu15ContactVirtualOwner filter_owner{m.word(filter_table)};
            if(!services.filter_child)throw std::runtime_error("Native list contact requires its real child filter owner");
            const auto allowed=services.filter_child(filter,filter_owner,scratch.output,{context},current_second,current_first,{collection},index,fp);
            if(m.word(allowed,1)) {
                const auto current_shape=m.word(scratch.child_payload),dispatch=m.word(context),first_table=m.word(current_shape);
                const NativeTu15ContactVirtualOwner first_owner{m.word(first_table+20u)};
                if(!services.dispatch.classify_shape)throw std::runtime_error("Native list contact requires its real shape class owner");
                const auto first_class=services.dispatch.classify_shape({current_shape},first_owner,fp);
                const auto second_shape=m.word(input(arguments.second)),second_table=m.word(second_shape);
                const NativeTu15ContactVirtualOwner second_owner{m.word(second_table+20u)};
                const auto second_class=services.dispatch.classify_shape({second_shape},second_owner,fp);
                const auto data=m.word(header),user_data=input(arguments.user_data);
                const auto pair=(first_class<<5u)+second_class;
                const NativeTu15ContactVirtualOwner factory{m.word(dispatch+pair*4u+0x18cu)};
                const auto destination=data+index*4u;
                const NativeTu15ContactPayload second{input(arguments.second)};
                if(!services.dispatch.create_agent)throw std::runtime_error("Native list contact requires its selected child factory");
                const auto child=services.dispatch.create_agent(factory,{scratch.child_payload},second,{context},user_data,fp);
                m.word(0x2f7e30u,destination,child.identity);
            } else {
                const auto data=m.word(header),destination=data+index*4u;
                m.word(0x2f7e4du,destination,native_tu15_invalid_contact_agent().identity);
            }
            const auto current_count=m.word(object+16u);++index;
            if(signed_word(index)>=signed_word(current_count))break;
        } while(true);
    }
    array.disarm();base.disarm();return agent;
}
NativeTu15ContactAgent native_tu15_create_list_contact_agent(
    SourcePhysicsContactMemory& memory,const NativeTu15ListContactArguments& arguments,
    NativeTu15ListContactScratch scratch,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ListContactServices& services,const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory,observe);const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+16u);
    if(!services.allocation.allocate)throw std::runtime_error("Native list contact requires its actual allocation owner");
    const NativeTu15ContactAgent agent{services.allocation.allocate({allocator,owner,40u,0x1au},fp)};
    m.word(0x2f8055u,agent.identity+4u,40u,2);
    const auto user_data=input(arguments.user_data),context=input(arguments.context);
    const auto second=input(arguments.second),first=input(arguments.first);
    const NativeTu15ListContactArguments current{[&]{return first;},[&]{return second;},[&]{return context;},[&]{return user_data;}};
    native_contact_detail::UnwindGuard storage([&]{native_tu15_release_contact_agent(memory,agent,fp,services.allocation);});
    storage.arm(); // State0 at2F8074; action37E9E0 ->2F0380.
    native_tu15_initialize_list_contact_agent(memory,agent,current,scratch,fp,services,observe);
    storage.disarm();return agent;
}
NativeTu15ContactAgent native_tu15_create_reversed_list_contact_agent(
    SourcePhysicsContactMemory& memory,const NativeTu15ListContactArguments& arguments,
    NativeTu15ListContactScratch scratch,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ListContactServices& services,const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory,observe);const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+16u);
    if(!services.allocation.allocate)throw std::runtime_error("Native reversed list contact requires its actual allocation owner");
    const NativeTu15ContactAgent agent{services.allocation.allocate({allocator,owner,40u,0x1au},fp)};
    m.word(0x2f8218u,agent.identity+4u,40u,2);
    const auto user_data=input(arguments.user_data),context=input(arguments.context);
    const auto first=input(arguments.first),second=input(arguments.second);
    const NativeTu15ListContactArguments current{[&]{return second;},[&]{return first;},[&]{return context;},[&]{return user_data;}};
    native_contact_detail::UnwindGuard storage([&]{native_tu15_release_contact_agent(memory,agent,fp,services.allocation);});
    storage.arm(); // State0 at2F8238; the same37E9E0 ->2F0380 action.
    native_tu15_initialize_list_contact_agent(memory,agent,current,scratch,fp,services,observe);
    m.word(0x2f8249u,agent.identity,0x41cce8u);
    storage.disarm();return agent;
}
}
