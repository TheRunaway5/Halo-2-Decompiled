#include "halo2/native_tu15_collection_contact_agent.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observer_;
public:
    Memory(SourcePhysicsContactMemory& m,const SourcePhysicsContactRecordObserver& observer):memory_(m),observer_(observer){}
    std::uint32_t word(std::uint32_t address,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory_.read(address,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void word(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4) {
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(observer_){event.before.resize(width);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
        memory_.write(address,std::span(bytes).first(width));if(observer_)observer_(event);
    }
};
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::uint32_t input(const std::function<std::uint32_t()>& provider) {
    if(!provider)throw std::runtime_error("Native collection contact requires its current ordinary input");return provider();
}
// Complete actual2E15C0. No array field is reset by the source destructor.
void release_array(Memory& m,std::uint32_t header,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ArrayGrowthServices& services) {
    const auto capacity=m.word(header+8u);
    if(capacity&0x80000000u)return;
    const auto allocator=m.word(0x48c938u),table=m.word(allocator);
    const auto bytes=(capacity&0x7fffffffu)*4u,data=m.word(header);
    const auto owner=m.word(table+20u);
    if(!services.release)throw std::runtime_error("Native collection contact requires its actual array release owner");
    services.release({allocator,owner,data,bytes,0x12u},fp);
}
}
void native_tu15_initialize_collection_contact_agent(SourcePhysicsContactMemory& memory,
    NativeTu15ContactAgent agent,const NativeTu15CollectionContactArguments& arguments,
    NativeTu15CollectionContactScratch scratch,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15CollectionContactServices& services,const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory,observe);const auto object=agent.identity,array=object+12u;
    const auto initial_user=input(arguments.user_data);
    m.word(0x2f7d46u,object+8u,initial_user);m.word(0x2f7d4eu,object,0x41ccc8u);
    native_contact_detail::UnwindGuard base([&]{m.word(0x2f7a10u,object,0x45edf0u);});
    base.arm(); // State0 at2F7D58;37E940 ->2F7A10.
    m.word(0x2f7d5cu,array,object+24u);m.word(0x2f7d5eu,array+4u,0);
    m.word(0x2f7d61u,array+8u,0x80000004u);
    const auto initial_first=input(arguments.first),extra=m.word(initial_first+8u),shape=m.word(initial_first);
    m.word(0x2f7d71u,scratch.child_payload+12u,initial_first);
    m.word(0x2f7d75u,scratch.child_payload+8u,extra);
    const auto table=m.word(shape);
    native_contact_detail::UnwindGuard storage([&]{release_array(m,array,fp,services.allocation);});
    storage.arm(); // State1 at2F7D7D;37E948 ->2E15C0, then base state0.
    const NativeTu15ContactVirtualOwner count_owner{m.word(table+40u)};
    if(!services.count_children)throw std::runtime_error("Native collection contact requires its real child-count owner");
    const auto count=services.count_children({shape},count_owner,fp);
    const auto capacity=m.word(array+8u)&0x7fffffffu;
    if(signed_word(capacity)<signed_word(count)) {
        const auto doubled=capacity+capacity;
        const auto requested=signed_word(count)<signed_word(doubled)?doubled:count;
        native_tu15_reserve_array(memory,{array},requested,4u,fp,services.allocation,observe);
    }
    m.word(0x2f7dabu,array+4u,count);
    const auto initial_count=m.word(object+16u);
    if(signed_word(initial_count)>0) {
        const NativeTu15ContactContext context{input(arguments.context)};
        std::uint32_t index{};
        do {
            const auto children=m.word(shape+12u),child=m.word(children+index*8u);
            const auto filter=m.word(context.identity+4u);
            m.word(0x2f7dcau,scratch.child_payload,child);
            const NativeTu15ContactPayload first{input(arguments.first)},second{input(arguments.second)};
            m.word(0x2f7ddeu,scratch.child_payload+4u,index);
            const auto filter_table=m.word(filter);
            const NativeTu15ContactVirtualOwner filter_owner{m.word(filter_table)};
            if(!services.filter_child)throw std::runtime_error("Native collection contact requires its real child filter owner");
            const auto result=services.filter_child(filter,filter_owner,scratch.filter_result,context,second,first,{shape},index,fp);
            if(m.word(result,1)) {
                const auto current_child=m.word(scratch.child_payload),dispatch=m.word(context.identity);
                const auto first_table=m.word(current_child);
                const NativeTu15ContactVirtualOwner first_owner{m.word(first_table+20u)};
                if(!services.dispatch.classify_shape)throw std::runtime_error("Native collection contact requires its real class owners");
                const auto first_class=services.dispatch.classify_shape({current_child},first_owner,fp);
                const NativeTu15ContactPayload classified_second{input(arguments.second)};
                const auto second_shape=m.word(classified_second.identity),second_table=m.word(second_shape);
                const NativeTu15ContactVirtualOwner second_owner{m.word(second_table+20u)};
                const auto second_class=services.dispatch.classify_shape({second_shape},second_owner,fp);
                const auto data=m.word(array),user=input(arguments.user_data);
                const auto key=(first_class<<5u)+second_class;
                const NativeTu15ContactVirtualOwner factory{m.word(dispatch+key*4u+0x18cu)};
                const auto destination=data+index*4u;
                const NativeTu15ContactPayload current_second{input(arguments.second)};
                if(!services.dispatch.create_agent)throw std::runtime_error("Native collection contact requires its real selected child factory");
                const auto nested=services.dispatch.create_agent(factory,{scratch.child_payload},current_second,context,user,fp);
                m.word(0x2f7e30u,destination,nested.identity);
            } else {
                const auto destination=m.word(array)+index*4u;
                m.word(0x2f7e4du,destination,native_tu15_invalid_contact_agent().identity);
            }
            ++index;
        }while(signed_word(index)<signed_word(m.word(object+16u)));
    }
    storage.disarm();base.disarm();
}
NativeTu15ContactAgent native_tu15_create_collection_contact_agent(SourcePhysicsContactMemory& memory,
    const NativeTu15CollectionContactArguments& arguments,NativeTu15CollectionContactScratch scratch,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15CollectionContactServices& services,
    const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory,observe);const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+16u);
    if(!services.allocation.allocate)throw std::runtime_error("Native collection contact requires its actual allocation owner");
    const NativeTu15ContactAgent agent{services.allocation.allocate({allocator,owner,40u,0x1au},fp)};
    m.word(0x2f8218u,agent.identity+4u,40u,2);
    const auto user=input(arguments.user_data),context=input(arguments.context);
    const auto first=input(arguments.first),second=input(arguments.second);
    const NativeTu15CollectionContactArguments current{[&]{return second;},[&]{return first;},[&]{return context;},[&]{return user;}};
    native_contact_detail::UnwindGuard allocation([&]{native_tu15_release_contact_agent(memory,agent,fp,services.allocation);});
    allocation.arm(); // State0 at2F8238;37E9E0 ->2F0380.
    native_tu15_initialize_collection_contact_agent(memory,agent,current,scratch,fp,services,observe);
    m.word(0x2f8249u,agent.identity,0x41cce8u);allocation.disarm();return agent;
}
}
