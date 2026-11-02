#include "halo2/native_tu15_wrapped_contact_agent.hpp"
#include "halo2/native_tu15_geometry.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observer_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observer):memory_(memory),observer_(observer){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory_.read(p,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    }
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte> bytes) {
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observer_){event.before.resize(bytes.size());memory_.read(p,event.before);event.written.assign(bytes.begin(),bytes.end());}
        memory_.write(p,bytes);if(observer_)observer_(event);
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4) {
        std::array<std::byte,4> value_bytes{};for(unsigned n=0;n<width;++n)value_bytes[n]=std::byte(value>>(8u*n));
        bytes(pc,p,std::span(value_bytes).first(width));
    }
    std::array<std::byte,16> vector(std::uint32_t p)const {
        if(p&15u)throw std::runtime_error("Native transformed contact requires aligned source vectors");
        std::array<std::byte,16> value{};memory_.read_vector(p,value);return value;
    }
    void vector(std::uint32_t pc,std::uint32_t p,const std::array<std::byte,16>& value) {
        if(p&15u)throw std::runtime_error("Native transformed contact requires aligned retained vectors");
        bytes(pc,p,value);
    }
};
std::uint32_t input(const std::function<std::uint32_t()>& provider) {
    if(!provider)throw std::runtime_error("Native transformed contact requires its current ordinary input");
    return provider();
}
}
NativeTu15ContactAgent native_tu15_create_reversed_transformed_contact_agent(
    SourcePhysicsContactMemory& memory,const NativeTu15WrappedContactArguments& arguments,
    NativeTu15TransformedContactScratch scratch,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15WrappedContactServices& services,const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory,observe);const auto allocator=m.word(0x48c938u),table=m.word(allocator);
    const auto owner=m.word(table+16u);
    if(!services.allocation.allocate)throw std::runtime_error("Native transformed contact requires its actual allocation owner");
    const NativeTu15ContactAgent agent{services.allocation.allocate({allocator,owner,16u,0x1au},fp)};
    m.word(0x2f7625u,agent.identity+4u,16u,2);
    const auto initial_user=input(arguments.user_data);
    native_contact_detail::UnwindGuard allocation([&]{native_tu15_release_contact_agent(memory,agent,fp,services.allocation);});
    allocation.arm();
    m.word(0x2f763du,agent.identity+8u,initial_user);
    const auto transformed=input(arguments.second);
    m.word(0x2f7643u,agent.identity,0x41cc64u);
    const auto extra=m.word(transformed+8u);const auto first_vector=m.vector(extra);
    const auto shape=m.word(transformed),child=m.word(shape+12u);
    m.vector(0x2f7654u,scratch.geometry,first_vector);
    m.vector(0x2f765du,scratch.geometry+16u,m.vector(extra+16u));
    m.vector(0x2f7666u,scratch.geometry+32u,m.vector(extra+32u));
    m.vector(0x2f766fu,scratch.geometry+48u,m.vector(extra+48u));
    m.vector(0x2f767bu,scratch.geometry+64u,m.vector(extra+64u));
    const auto last_vector=m.vector(extra+80u);
    native_contact_detail::UnwindGuard base([&]{m.word(0x2f7a10u,agent.identity,0x45edf0u);});
    base.arm();
    m.vector(0x2f7695u,scratch.geometry+80u,last_vector);
    NativeTu15GeometryMemory geometry(memory,scratch.geometry+32u,extra+32u,shape+16u,observe);
    native_tu15_compose_affine_geometry(geometry,fp);
    const auto initial_context=input(arguments.context);
    m.word(0x2f76a9u,scratch.child_payload+8u,scratch.geometry);
    const auto dispatch=m.word(initial_context);
    m.word(0x2f76afu,scratch.child_payload+12u,transformed);
    const auto transform=m.word(transformed+4u);
    m.word(0x2f76b6u,scratch.child_payload+4u,transform);
    m.word(0x2f76bau,scratch.child_payload,child);
    const auto first_table=m.word(child);
    const NativeTu15ContactVirtualOwner first_owner{m.word(first_table+20u)};
    if(!services.dispatch.classify_shape)throw std::runtime_error("Native transformed contact requires the real shape class owner");
    const auto first_class=services.dispatch.classify_shape({child},first_owner,fp);
    const NativeTu15ContactPayload first{input(arguments.first)};
    const auto second_shape=m.word(first.identity),second_table=m.word(second_shape);
    const NativeTu15ContactVirtualOwner second_owner{m.word(second_table+20u)};
    const auto second_class=services.dispatch.classify_shape({second_shape},second_owner,fp);
    const auto current_user=input(arguments.user_data),current_context=input(arguments.context);
    const auto index=(first_class<<5u)+second_class;
    const NativeTu15ContactVirtualOwner factory{m.word(dispatch+index*4u+0x18cu)};
    if(!services.dispatch.create_agent)throw std::runtime_error("Native transformed contact requires its selected child factory");
    const auto nested=services.dispatch.create_agent(factory,{scratch.child_payload},first,{current_context},current_user,fp);
    m.word(0x2f76ffu,agent.identity+12u,nested.identity);
    m.word(0x2f7703u,agent.identity,0x41cc84u);
    base.disarm();allocation.disarm();return agent;
}
}
