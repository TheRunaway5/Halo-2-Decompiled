#include "halo2/native_tu15_contact_creation_gameplay.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8u);return v;
}
std::uint32_t root(const SourcePhysicsContactMemory&m,std::uint32_t p){
    auto parent=word(m,p+12u);while(parent){p=parent;parent=word(m,p+12u);}return p;
}
template<class F>auto call(NativeTu15ContactCreationGameplayStats&stats,F&&f){
    try{auto result=f();++stats.completed;return result;}catch(...){++stats.refusals;throw;}
}
[[noreturn]]void missing(NativeTu15ContactCreationGameplayStats&s,std::uint32_t owner){
    ++s.unqualified_owners[owner];throw std::runtime_error("Native contact creation requires owner="+std::to_string(owner));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace joint_constraint_numeric;
Vector scalar(std::uint32_t bits){Vector v{};std::memcpy(&v,&bits,4);return v;}
Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(Vector a,Vector b){std::uint8_t yes;asm volatile("comiss %2,%1;seta %0":"=qm"(yes):"x"(a),"x"(b):"cc","memory");return yes;}
std::uint32_t motion_kind(SourcePhysicsContactMemory&m,std::uint32_t body,NativeTu15ContactCreationGameplayStats&s){
    const auto motion=word(m,body+0x3cu),table=word(m,motion),owner=word(m,table+0x18u);++s.motion_queries;
    if(auto kind=native_tu15_activation_type(owner))return*kind;missing(s,owner);
}
std::uint32_t shape_kind(SourcePhysicsContactMemory&m,std::uint32_t payload,NativeTu15ContactCreationGameplayStats&s){
    const auto shape=word(m,payload),table=word(m,shape),owner=word(m,table+0x14u);++s.class_queries;
    if(owner==0xa9520u)return native_tu15_contact_child_class_23();
    if(owner==0xab4b0u)return native_tu15_contact_child_class_24();
    if(auto kind=native_tu15_activation_type(owner))return*kind;missing(s,owner);
}
#endif
}
bool native_tu15_test_contact_bumper(SourcePhysicsContactMemory&memory,
    NativeTu15ContactBumperInput input,NativeTu15ContactCreationGameplayScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ContactCreationGameplayServices&services,
    NativeTu15ContactCreationGameplayStats*activity){
    NativeTu15ContactCreationGameplayStats local;auto&s=activity?*activity:local;++s.bumper_calls;
    return call(s,[&]()->bool{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth)throw std::runtime_error("Native contact bumper requires empty x87 DATA");
        const auto count=word(memory,input.first_body+0x34u);std::uint32_t datum{};
        if(std::bit_cast<std::int32_t>(count)>0){const auto data=word(memory,input.first_body+0x30u);std::uint32_t i{};
            do{if(word(memory,data+i*8u)==0x2001u){datum=word(memory,data+i*8u+4u);break;}++i;}while(std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(count));}
        // The source reads property2002 even though its local is overwritten
        // by the later speed scalar. Preserve this current read and no value.
        if(std::bit_cast<std::int32_t>(count)>0){const auto data=word(memory,input.first_body+0x30u);std::uint32_t i{};
            do{if(word(memory,data+i*8u)==0x2002u){(void)word(memory,data+i*8u+4u);break;}++i;}while(std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(count));}
        const auto second_count=word(memory,input.second_body+0x34u);std::uint32_t second_datum{};
        if(std::bit_cast<std::int32_t>(second_count)>0){const auto data=word(memory,input.second_body+0x30u);std::uint32_t i{};
            do{if(word(memory,data+i*8u)==0x2001u){second_datum=word(memory,data+i*8u+4u);break;}++i;}while(std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(second_count));}
        const auto manager=word(memory,0x52be5cu),components=word(memory,manager+0x44u);
        const auto component=components+(datum&0xffffu)*160u;
        const auto objects=word(memory,0x4ecbd0u),headers=word(memory,objects+0x44u);
        const auto first_object=word(memory,component+8u),object=word(memory,headers+(first_object&0xffffu)*12u+8u);
        std::uint32_t secondary_object{};
        if(second_datum!=0xffffffffu&&component){const auto current=word(memory,component+8u);secondary_object=word(memory,headers+(current&0xffffu)*12u+8u);}
        const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
        std::uint32_t speed_bits;
        {
            Environment phase(fp,depth);
            const auto motion=m.word(input.first_body+0x3cu);
            const auto velocity=m.vector(motion+0x40u),squared=multiply(velocity,velocity);
            auto speed=add_scalar(lane<0xaau>(squared),add_scalar(lane<0x55u>(squared),squared));
            m.vector(0x283231u,scratch.bumper_vector,speed);asm volatile("sqrtss %0,%0":"+x"(speed)::"memory");
            m.word(0x28323au,scratch.bumper_vector,scalar_bits(speed));speed=m.vector(scratch.bumper_vector);
            m.word(0x283249u,scratch.bumper_speed,scalar_bits(speed));
            const auto other_motion=m.word(input.second_body+0x3cu);
            speed=scalar(m.word(scratch.bumper_speed));const auto table=m.word(other_motion);
            m.word(0x283258u,scratch.bumper_original_speed,scalar_bits(speed));
            const auto owner=m.word(table+0x18u);++s.motion_queries;
            const auto kind=native_tu15_activation_type(owner);if(!kind)missing(s,owner);
            if(*kind!=7u){
                const auto second_now=m.word(input.second_body+0x3cu);
                const auto other=m.vector(second_now+0x40u);
                const auto first_now=m.word(input.first_body+0x3cu);
                const auto first=m.vector(first_now+0x40u),difference=subtract(first,other),square=multiply(difference,difference);
                auto relative=add_scalar(lane<0xaau>(square),add_scalar(lane<0x55u>(square),square));
                const auto current_speed=scalar(m.word(scratch.bumper_speed));
                m.vector(0x28329fu,scratch.bumper_vector,relative);asm volatile("sqrtss %0,%0":"+x"(relative)::"memory");
                m.word(0x2832a8u,scratch.bumper_vector,scalar_bits(relative));relative=m.vector(scratch.bumper_vector);
                // Source publishes/reloads this scalar before COMISS.
                m.word(0x2832b7u,scratch.bumper_relative_speed,scalar_bits(relative));relative=scalar(m.word(scratch.bumper_relative_speed));
                if(above(current_speed,relative))m.word(0x2832c6u,scratch.bumper_original_speed,scalar_bits(relative));
            }
            speed_bits=m.word(scratch.bumper_original_speed);
        }
        bool direct=word(memory,object+0xaau,1u)!=0;
        if(!direct){const auto current_object=word(memory,component+8u),current_manager=word(memory,0x4ecbd0u),current_headers=word(memory,current_manager+0x44u),current=word(memory,current_headers+(current_object&0xffffu)*12u+8u);
            if(word(memory,current+0x3ddu,1u)&&secondary_object&&!word(memory,secondary_object+0xaau,1u))return false;
            direct=word(memory,current+0x34bu,1u)!=0;}
        bool apply{};
        {
            Environment phase(fp,depth);const auto speed=scalar(speed_bits),radius=scalar(input.radius_bits);
            if(direct){if(!above(speed,radius))return false;apply=true;}
            else{
                auto limited=scalar(input.relative_velocity_bits);if(above(limited,speed))limited=speed;
                if(above(limited,radius))apply=true;
                else if(!above(speed,radius))return false;
            }
        }
        if(!apply){++s.event_tests;if(!services.test_event)missing(s,0x282fd0u);
            if(services.test_event(memory,input.event,fp))return false;}
        std::uint32_t delta;
        {Environment phase(fp,depth);auto speed=scalar(speed_bits);const auto difference=scalar(input.radius_difference_bits);asm volatile("subss %1,%0":"+x"(speed):"x"(difference):"memory");delta=scalar_bits(speed);}
        ++s.damage_requests;if(!services.damage)missing(s,0x1d7fc0u);
        services.damage(memory,input.event,component,delta,fp);return true;
#else
        (void)memory;(void)input;(void)scratch;(void)fp;(void)depth;(void)services;
        throw std::runtime_error("Exact native bumper backend unavailable");
#endif
    });
}

void native_tu15_world_contact_creation_listener(SourcePhysicsContactMemory&memory,
    std::uint32_t listener,std::uint32_t event,NativeTu15ContactCreationGameplayScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ContactCreationGameplayServices&services,
    NativeTu15ContactCreationGameplayStats*activity){
    NativeTu15ContactCreationGameplayStats local;auto&s=activity?*activity:local;++s.world_calls;
    call(s,[&](){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth)throw std::runtime_error("Native creation world listener requires empty x87 DATA");
        const auto first_root=root(memory,word(memory,event+8u));
        auto second_root=root(memory,word(memory,event+0xcu));
        const auto first_world=word(memory,listener+8u);
        if(!native_tu15_island_discrete_dispatcher(memory,{first_world}))return true;
        const auto current_world=word(memory,listener+8u);
        const auto dispatcher=native_tu15_island_discrete_dispatcher(memory,{current_world});
        const auto table=word(memory,dispatcher),owner=word(memory,table+4u);
        if(owner!=0x281530u)missing(s,owner);
        const auto result=native_tu15_test_bound_discrete_pair(memory,dispatcher,scratch.result_byte,
            {first_root},{second_root},fp,{},&s.discrete);
        if(!word(memory,result,1u))return true;
        const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
        if(shape_kind(memory,m.word(event+8u),s)==23u){
            const auto body=m.word(first_root+0x20u),payload=m.word(event+8u),shape=m.word(payload);
            std::uint32_t radius,relative,other;
            {
                Environment phase(fp,depth);const auto motion=m.word(body+0x3cu);
                push(m.word(shape+0x24u));const auto velocity=m.vector(motion+0x40u);
                const auto difference=m.word(shape+0x28u);asm volatile("fsubs %0"::"m"(difference):"memory","st");
                relative=m.word(event+0x1cu);const auto squared=multiply(velocity,velocity);
                radius=m.word(shape+0x24u);m.pop_float(0x2e4c15u,scratch.radius_difference);
                m.word(0x2e4c30u,scratch.squared_speed,sum_three(squared));
                other=m.word(second_root+0x20u);
            }
            if(native_tu15_test_contact_bumper(memory,{event,body,other,radius,
                m.word(scratch.radius_difference),relative},scratch,fp,depth,services,&s)){
                {
                    Environment phase(fp,depth);push(m.word(scratch.squared_speed));asm volatile("fsqrt":::"memory","st");
                    const auto motion=m.word(body+0x3cu);
                    const auto velocity=m.vector(motion+0x40u);
                    const auto one=m.word(0x46a0d0u);asm volatile("fdivrs %0"::"m"(one):"memory","st");
                    push(m.word(shape+0x24u));const auto difference=m.word(shape+0x28u);asm volatile("fsubs %0;fmulp %%st,%%st(1)"::"m"(difference):"memory","st","st(1)");
                    m.pop_float(0x2e4c6eu,scratch.radius_difference);
                    m.vector(0x2e4c82u,scratch.scaled_velocity,multiply(lane<0u>(scalar(m.word(scratch.radius_difference))),velocity));
                }
                ++s.velocity_requests;if(!services.set_velocity)missing(s,0x1e10f0u);
                services.set_velocity(memory,{body},scratch.scaled_velocity,fp);
                const auto properties=m.word(event+0x18u);m.word(0x2e4c8fu,properties+0xcu,0u);
            }
        }
        if(shape_kind(memory,m.word(event+0xcu),s)==23u){
            const auto payload=m.word(event+0xcu),shape=m.word(payload);
            second_root=m.word(second_root+0x20u);std::uint32_t other,radius,relative;
            {
                Environment phase(fp,depth);push(m.word(shape+0x24u));const auto motion=m.word(second_root+0x3cu);
                const auto difference=m.word(shape+0x28u);asm volatile("fsubs %0"::"m"(difference):"memory","st");
                const auto velocity=m.vector(motion+0x40u);other=m.word(first_root+0x20u);
                radius=m.word(shape+0x24u);const auto squared=multiply(velocity,velocity);
                m.word(0x2e4ce8u,scratch.squared_speed,sum_three(squared));relative=m.word(event+0x1cu);
                m.pop_float(0x2e4cf1u,scratch.radius_difference);
            }
            if(native_tu15_test_contact_bumper(memory,{event,second_root,other,radius,
                m.word(scratch.radius_difference),relative},scratch,fp,depth,services,&s)){
                {
                    Environment phase(fp,depth);push(m.word(scratch.squared_speed));const auto motion=m.word(second_root+0x3cu);
                    asm volatile("fsqrt":::"memory","st");const auto velocity=m.vector(motion+0x40u);
                    const auto one=m.word(0x46a0d0u);asm volatile("fdivrs %0"::"m"(one):"memory","st");
                    push(m.word(shape+0x24u));const auto difference=m.word(shape+0x28u);asm volatile("fsubs %0;fmulp %%st,%%st(1)"::"m"(difference):"memory","st","st(1)");
                    m.pop_float(0x2e4d26u,scratch.radius_difference);
                    m.vector(0x2e4d3au,scratch.scaled_velocity,multiply(lane<0u>(scalar(m.word(scratch.radius_difference))),velocity));
                }
                ++s.velocity_requests;if(!services.set_velocity)missing(s,0x1e10f0u);
                services.set_velocity(memory,{second_root},scratch.scaled_velocity,fp);
                const auto properties=m.word(event+0x18u);m.word(0x2e4d47u,properties+0xcu,0u);
            }
        }
        return true;
#else
        (void)memory;(void)listener;(void)event;(void)scratch;(void)fp;(void)depth;(void)services;
        throw std::runtime_error("Exact native creation world listener backend unavailable");
#endif
    });
}

void native_tu15_limit_created_contact(SourcePhysicsContactMemory&memory,std::uint32_t listener,
    std::uint32_t event,NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15ContactCreationGameplayStats*activity){
    NativeTu15ContactCreationGameplayStats local;auto&s=activity?*activity:local;++s.limit_calls;
    call(s,[&](){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth)throw std::runtime_error("Native contact limit requires empty x87 DATA");
        const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);Environment phase(fp,depth);
        const auto point=m.word(event+0x14u);push(m.word(point+0xcu));const auto threshold=m.word(listener+8u);
        std::uint16_t relation;asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(threshold):"memory","st");
        if((relation&0x500u)==0x100u){m.word(0x316524u,listener+4u,0u);m.word(0x31652bu,event+0x20u,1u);++s.point_rejections;}
        return true;
#else
        (void)memory;(void)listener;(void)event;(void)fp;(void)depth;throw std::runtime_error("Exact native contact limit backend unavailable");
#endif
    });
}
void native_tu15_body_contact_creation_listener(SourcePhysicsContactMemory&memory,std::uint32_t listener,
    std::uint32_t event,NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15ContactCreationGameplayStats*activity){
    NativeTu15ContactCreationGameplayStats local;auto&s=activity?*activity:local;++s.body_calls;
    call(s,[&](){(void)listener;(void)fp;(void)depth;if(word(memory,event+0x20u)==1u){++s.body_rejected_early_outs;return true;}missing(s,0x281710u);});
}
void native_tu15_dispatch_contact_creation_gameplay_listener(SourcePhysicsContactMemory&memory,
    std::uint32_t listener,NativeTu15ContactVirtualOwner owner,std::uint32_t event,
    NativeTu15ContactCreationGameplayScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ContactCreationGameplayServices&services,NativeTu15ContactCreationGameplayStats*activity){
    if(owner.identity==0x2e4d60u)return;
    if(owner.identity==0x2e4b60u)return native_tu15_world_contact_creation_listener(memory,listener,event,scratch,fp,depth,services,activity);
    if(owner.identity==0x316510u)return native_tu15_limit_created_contact(memory,listener,event,fp,depth,activity);
    if(owner.identity==0x281710u)return native_tu15_body_contact_creation_listener(memory,listener,event,fp,depth,activity);
    NativeTu15ContactCreationGameplayStats local;auto&s=activity?*activity:local;
    ++s.refusals;missing(s,owner.identity);
}
} // namespace halo2
