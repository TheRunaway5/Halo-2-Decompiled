#include "halo2/native_tu15_contact_point_create.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class EventMemory final:public SourcePhysicsContactMemory {
    SourcePhysicsContactMemory&storage_;std::uint32_t event_;
    bool current_body_initialized_{};NativeTu15ContactPointCreateStats*stats_;
    static bool overlaps(std::uint32_t p,std::size_t n,std::uint32_t first,unsigned size){
        return std::uint64_t(p)<std::uint64_t(first)+size&&std::uint64_t(first)<std::uint64_t(p)+n;
    }
    void check(std::uint32_t p,std::size_t n)const {
        if(overlaps(p,n,event_+2u,2)||(!current_body_initialized_&&overlaps(p,n,event_+0x10u,4))){
            if(stats_)++stats_->unprovided_event_reads;
            throw std::runtime_error("Native TU1.5 contact event requires its unprovided DATA lifetime owner");
        }
    }
public:
    EventMemory(SourcePhysicsContactMemory&m,std::uint32_t event,NativeTu15ContactPointCreateStats*s)
        :storage_(m),event_(event),stats_(s){}
    void read(std::uint32_t p,std::span<std::byte>b)const override{check(p,b.size());storage_.read(p,b);}
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{check(p,b.size());storage_.read_vector(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        if(overlaps(p,b.size(),event_+2u,2))throw std::runtime_error("Native TU1.5 contact event padding has no native writer");
        storage_.write(p,b);
        if(p<=event_+0x10u&&std::uint64_t(p)+b.size()>=std::uint64_t(event_)+0x14u)current_body_initialized_=true;
    }
};
template<class F>void require(const F&f,const char*message){if(!f)throw std::runtime_error(message);}
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(8u*k);return v;
}
void store(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v,unsigned n=4){
    std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(8u*k));m.write(p,std::span(b).first(n));
}
std::uint32_t body(SourcePhysicsContactMemory&m,std::uint32_t payload){
    auto next=word(m,payload+0xcu);
    while(next){payload=next;next=word(m,payload+0xcu);}
    return word(m,payload+0x20u);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace joint_constraint_numeric;
Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
void keep_float(SourcePhysicsContactMemory&memory,std::uint32_t p){
    alignas(16)std::array<std::byte,108>before{};std::uint32_t bits;
    asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    asm volatile("fsts %0":"=m"(bits)::"memory");
    try{store(memory,p,bits);}catch(...){asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
}
#endif
} // namespace

std::uint16_t native_tu15_create_contact_point(SourcePhysicsContactMemory&storage,
    std::uint32_t manager,NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext context,std::uint32_t packet,NativeTu15ContactPointCreateScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ContactPointCreateServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactPointCreateStats*stats){
    if(stats)++stats->attempts;
    try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(observe)throw std::runtime_error("Native TU1.5 contact creation raw private store observer is unqualified");
        if(depth||(scratch.identity&15u))throw std::runtime_error("Native TU1.5 contact creation requires its aligned DATA/FP owner");
        const auto p=scratch.identity,event=p+0x30u;
        const SourcePhysicsContactRecordObserver no_observer;
        EventMemory memory(storage,event,stats);
        const auto initial_count=word(memory,manager+0xa4u);
        if(std::bit_cast<std::int32_t>(initial_count)>250){if(stats){++stats->capacity_rejections;++stats->completed;}return 0xffffu;}
        const auto first_body=body(memory,first.identity),second_body=body(memory,second.identity);
        if(!initial_count){
            const auto world=word(memory,manager+8u);
            if(stats)++stats->registrations;
            require(services.register_constraint,"Native TU1.5 contact creation requires native constraint registration");
            services.register_constraint(memory,world,manager+0x10u,p+0x60u,fp);
        }
        if(stats)++stats->allocations;
        require(services.allocate_point,"Native TU1.5 contact creation requires native point storage");
        const auto id=services.allocate_point(memory,manager+0x10u,p,p+4u,p+0x60u,fp);
        const auto velocity=[&](std::uint32_t current_body,std::uint32_t output){
            const auto motion=word(memory,current_body+0x3cu),table=word(memory,motion),owner=word(memory,table+0x58u);
            if(stats)++stats->velocity_queries;
            require(services.point_velocity,"Native TU1.5 contact creation requires actual motion velocity owner");
            services.point_velocity(memory,motion,{owner},packet,output,fp);
        };
        velocity(first_body,p+0x10u);velocity(second_body,p+0x20u);
        std::uint32_t relative_velocity{},world{};
        {
            Environment environment(fp,depth);Memory m(memory,no_observer);
            const auto first_velocity=m.vector(p+0x10u),second_velocity=m.vector(p+0x20u),normal=m.vector(packet+0x10u);
            relative_velocity=sum_three(multiply(subtract(second_velocity,first_velocity),normal));
            const auto first_material=body(memory,first.identity)+0x48u;
            const auto second_material=body(memory,second.identity)+0x48u;
            push(m.word(second_material+4u));multiply_scalar(m.word(first_material+4u));
            const auto properties=m.word(p+4u);
            store(memory,event,std::uint16_t(id),2);store(memory,event+0xcu,second.identity);
            asm volatile("fsqrt":::"memory","st");store(memory,event+0x20u,0u);
            m.pop_float(0x322e4cu,properties+8u);
            push(m.word(second_material+8u));const auto current_properties=m.word(p+4u);
            multiply_scalar(m.word(first_material+8u));store(memory,event+8u,first.identity);
            asm volatile("fsqrt":::"memory","st");m.pop_float(0x322e62u,current_properties+0xcu);
            const auto point=m.word(p);push(m.word(packet+0xcu));
            m.vector(0x322e6fu,point,m.vector(packet));m.vector(0x322e76u,point+0x10u,m.vector(packet+0x10u));
            m.pop_float(0x322e7au,point+0xcu);
            const auto event_point=m.word(p),event_properties=m.word(p+4u);
            store(memory,event+0x14u,event_point);store(memory,event+0x18u,event_properties);
            store(memory,event+0x1cu,relative_velocity);
            world=m.word(manager+8u);store(memory,event+4u,manager);
            // Source has no live x87 value across the notification boundary.
        }
        if(stats)++stats->world_notifications;
        require(services.notify_world,"Native TU1.5 contact creation requires native world notification");
        services.notify_world(memory,world,event,fp);
        if(word(memory,first_body+0x64u)){
            if(stats)++stats->body_notifications;
            require(services.notify_body,"Native TU1.5 contact creation requires native body notification");
            services.notify_body(memory,first_body,event,fp);
        }
        if(word(memory,second_body+0x64u)){
            if(stats)++stats->body_notifications;
            require(services.notify_body,"Native TU1.5 contact creation requires native body notification");
            services.notify_body(memory,second_body,event,fp);
        }
        if(word(memory,event+0x20u)==1u){
            const auto table=word(memory,manager),owner=word(memory,table+8u);
            if(stats)++stats->rejected_points;
            require(services.remove_point,"Native TU1.5 contact creation requires native rejected-point removal");
            services.remove_point(memory,manager,{owner},std::uint16_t(id),fp);
            if(stats)++stats->completed;return 0xffffu;
        }
        {
            Environment environment(fp,depth);Memory m(memory,no_observer);
            auto properties=m.word(p+4u);push(m.word(properties+0xcu));
            multiply_scalar(m.word(context.identity+0x30u));multiply_scalar(relative_velocity);multiply_scalar(m.word(0x46a35cu));
            keep_float(memory,properties+4u);
            properties=m.word(p+4u);add_scalar(m.word(packet+0xcu));m.pop_float(0x322f23u,properties+0x14u);
            const auto second_motion=m.word(second_body+0x3cu),first_motion=m.word(first_body+0x3cu);
            push(m.word(second_motion+0x2cu));add_scalar(m.word(first_motion+0x2cu));properties=m.word(p+4u);
            add_scalar(m.word(0x41f118u));const auto one=m.word(0x46a0d0u);asm volatile("fdivrs %0"::"m"(one):"memory","st");
            push(m.word(properties+0xcu));add_scalar(m.word(0x46a0d0u));
            asm volatile("fmulp %%st,%%st(1)":::"memory","st","st(1)");
            multiply_scalar(relative_velocity);multiply_scalar(m.word(0x46a16cu));m.pop_float(0x322f59u,properties);
        }
        if(stats)++stats->completed;return std::uint16_t(id);
#else
        (void)storage;(void)manager;(void)first;(void)second;(void)context;(void)packet;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native TU1.5 contact creation backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
