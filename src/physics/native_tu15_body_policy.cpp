#include "halo2/native_tu15_body_policy.hpp"
#include "halo2/native_tu15_gameplay_policy_destruction.hpp"
#include "halo2/native_tu15_island_activation.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    alignas(16)std::array<std::byte,108>host_{};unsigned host_mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 body policy requires its unsupported numerical-fault owner");
        host_mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>native{};std::memcpy(native.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);std::memcpy(native.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);
    }
};
class Policy {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
    const NativeTu15BodyPolicyServices&services_;NativeTu15PhysicsFrameFp&fp_;
    unsigned depth_;NativeTu15BodyPolicyScratch scratch_;NativeTu15BodyPolicyStats*stats_;
public:
    Policy(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
      const NativeTu15BodyPolicyServices&s,NativeTu15PhysicsFrameFp&fp,unsigned depth,
      NativeTu15BodyPolicyScratch scratch,NativeTu15BodyPolicyStats*stats)
      :memory_(m),observer_(o),services_(s),fp_(fp),depth_(depth),scratch_(scratch),stats_(stats){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>raw{};memory_.read(p,std::span(raw).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);return value;
    }
    void bytes(std::uint32_t site,std::uint32_t p,std::span<const std::byte>raw){
        SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(raw.size());memory_.read(p,event.before);event.written.assign(raw.begin(),raw.end());}
        memory_.write(p,raw);if(observer_)observer_(event);
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>raw{};for(unsigned n=0;n<width;++n)raw[n]=std::byte(value>>(n*8u));bytes(site,p,std::span(raw).first(width));
    }
    void vector(std::uint32_t site,std::uint32_t p,std::uint32_t lane){
        if(p&15u)throw std::runtime_error("Native TU1.5 body policy requires aligned vector storage");
        std::array<std::uint32_t,4>raw{lane,lane,lane,lane};bytes(site,p,{reinterpret_cast<const std::byte*>(raw.data()),16});
    }
    __m128 vector(std::uint32_t p){
        if(p&15u)throw std::runtime_error("Native TU1.5 body policy requires aligned bound storage");
        std::array<std::byte,16>raw{};memory_.read_vector(p,raw);__m128 value;std::memcpy(&value,raw.data(),16);return value;
    }
    void release(std::uint32_t policy,std::uint32_t memory_class,bool size_first=true){
        auto bytes=size_first?word(policy+4u,2):0u;
        const auto allocator=word(0x48c938u),table=word(allocator);
        if(!size_first)bytes=word(policy+4u,2);
        const auto owner=word(table+0x14u);
        if(!services_.allocation.release)throw std::runtime_error("Native TU1.5 body policy requires its actual release owner");
        if(stats_)++stats_->storage_releases;services_.allocation.release({allocator,owner,policy,bytes,memory_class},fp_);
    }
    void destroy_base(std::uint32_t p){ //B4150
        const auto flags=word(p+4u,2);word(0xb415bu,p,0x45eddcu);
        if(flags&0x8000u){
            word(0xb416au,p+4u,flags&0x7fffu,2);
            if(!services_.release_base_lock)throw std::runtime_error("Native TU1.5 body policy requires actual base-lock release ownership");
            if(stats_)++stats_->base_lock_releases;services_.release_base_lock(p,0u,fp_);
        }
        word(0xb4173u,p,0x45edf0u);
    }
    void destroy(std::uint32_t policy,std::uint32_t flags){
        const auto table=word(policy),owner=word(table);
        if(owner==0x1d9c80u){
            if(stats_){++stats_->native_destructions;++stats_->gameplay_destructions;}
            NativeTu15GameplayPolicyDestructionServices gameplay{services_.allocation,services_.release_base_lock};
            if(gameplay.allocation.release)gameplay.allocation.release=[&](const auto&request,auto&context){
                if(stats_)++stats_->storage_releases;services_.allocation.release(request,context);
            };
            if(gameplay.release_base_lock)gameplay.release_base_lock=[&](auto object,auto disposing,auto&context){
                if(stats_)++stats_->base_lock_releases;services_.release_base_lock(object,disposing,context);
            };
            (void)native_tu15_destroy_gameplay_body_policy(memory_,policy,flags,fp_,gameplay,observer_);return;
        }
        if(owner==0x31e7f0u){ //31E7F0
            if(stats_)++stats_->native_destructions;destroy_base(policy);
            if(flags&1u)release(policy,0x26u);return;
        }
        if(!services_.destroy_policy)throw std::runtime_error("Native TU1.5 body policy requires its actual policy destructor");
        if(stats_)++stats_->unconverted_destructions;services_.destroy_policy(policy,{owner},flags,fp_);
    }
    bool decrement(std::uint32_t p,std::uint32_t decrement_site,std::uint32_t protected_site){
        word(decrement_site,p+6u,word(p+6u,2)-1u,2);if(word(p+6u,2))return false;
        if(native_tu15_reserved_body_reference(memory_,{p})){
            if(stats_)++stats_->protected_references;word(protected_site,p+6u,1u,2);return true;
        }
        destroy(p,1u);return false;
    }
    void install(std::uint32_t body,std::uint32_t policy){ //1E10A0
        if(stats_)++stats_->installs;
        if(policy)word(0x1e10adu,policy+6u,word(policy+6u,2)+1u,2);
        const auto old=word(body+0x54u);const bool protected_reference=old&&decrement(old,0x1e10b8u,0x1e10d0u);
        word(protected_reference?0x1e10d6u:0x1e10e7u,body+0x54u,policy);
    }
    void extent(std::uint32_t p,std::uint32_t shape){ //31E760, result discarded at31E8B3.
        if(shape){
            const auto table=word(shape),owner=word(table+0x18u);
            if(!services_.bounds.child_bounds)throw std::runtime_error("Native TU1.5 body policy requires actual shape bounds ownership");
            if(stats_)++stats_->shape_bounds_calls;
            services_.bounds.child_bounds({shape},owner,{0x46e400u},0u,scratch_.bounds,fp_);
            Environment environment(fp_,depth_);auto lower=vector(scratch_.bounds.identity),upper=vector(scratch_.bounds.identity+16u);
            asm volatile("subps %1,%0":"+x"(upper):"x"(lower):"memory");
            const auto half=_mm_castsi128_ps(_mm_set1_epi32(0x3f000000));auto squared=half;
            asm volatile("mulps %1,%0":"+x"(squared):"x"(upper):"memory");
            asm volatile("mulps %0,%0":"+x"(squared)::"memory");
            auto y=_mm_shuffle_ps(squared,squared,0x55);asm volatile("addss %1,%0":"+x"(y):"x"(squared):"memory");
            auto z=_mm_shuffle_ps(squared,squared,0xaa);asm volatile("addss %1,%0":"+x"(z):"x"(y):"memory");
            const auto result=std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(z)));word(0x31e7d1u,p+8u,result);
            const auto returned=word(p+8u);asm volatile("flds %0\n\tfstp %%st(0)"::"m"(returned):"memory","st");
        }else{
            Environment environment(fp_,depth_);word(0x31e7deu,p+8u,0xbf800000u);const auto returned=word(p+8u);
            asm volatile("flds %0\n\tfstp %%st(0)"::"m"(returned):"memory","st");
        }
    }
    void construct(std::uint32_t p,std::uint32_t body){ //31E820
        if(stats_)++stats_->constructors;word(0x31e845u,p+6u,1u,2);word(0x31e84bu,p,0x41eb6cu);
        word(0x31e851u,p+0x50u,0x3c23d70au);word(0x31e858u,p+0x54u,0x3ba3d70au);
        word(0x31e85fu,p+0x58u,0x3dcccccdu);word(0x31e866u,p+0x5cu,0x3e4ccccdu);
        vector(0x31e87fu,p+0x10u,0x7f7fffffu);vector(0x31e883u,p+0x30u,0x7f7fffffu);
        vector(0x31e88au,p+0x20u,0);word(0x31e893u,p+0x2cu,0x3f800000u);
        vector(0x31e896u,p+0x40u,0);word(0x31e89au,p+0x4cu,0x3f800000u);
        const auto shape=word(body+0xcu);native_contact_detail::UnwindGuard base([&]{destroy_base(p);});base.arm();extent(p,shape);base.disarm();
    }
    void select(std::uint32_t body,std::uint32_t requested){ //31B730
        if(stats_)++stats_->calls;const auto current=word(body+0x54u);
        if(current){
            const auto table=word(current),owner=word(table+0x1cu);std::uint32_t kind{};
            if(const auto known=native_tu15_activation_type(owner)){kind=*known;if(stats_)++stats_->native_type_queries;}
            else{
                if(!services_.policy_kind)throw std::runtime_error("Native TU1.5 body policy requires actual policy type ownership");
                if(stats_)++stats_->unconverted_type_queries;kind=services_.policy_kind(current,{owner},fp_);
            }
            if(kind==requested){if(stats_)++stats_->same_kind;return;}
        }
        if(requested==2u){
            const auto allocator=word(0x48c938u),table=word(allocator),owner=word(table+0x10u);
            if(!services_.allocation.allocate)throw std::runtime_error("Native TU1.5 body policy requires actual allocation ownership");
            const auto policy=services_.allocation.allocate({allocator,owner,0x60u,0x26u},fp_);
            native_contact_detail::UnwindGuard storage([&]{release(policy,0x26u,false);});
            word(0x31b780u,policy+4u,0x60u,2);storage.arm();construct(policy,body);storage.disarm();
            install(body,policy);decrement(policy,0x31b7acu,0x31b7c4u);
        }else if(requested==1u){
            if(stats_)++stats_->default_installs;word(0x31b7f8u,0x48d3deu,word(0x48d3deu,2)+1u,2);
            const auto old=word(body+0x54u);if(old)decrement(old,0x31b806u,0x31b81eu);
            word(0x31b82eu,body+0x54u,0x48d3d8u);
        }
    }
};
}
void native_tu15_select_body_policy(SourcePhysicsContactMemory&memory,NativeTu15PhysicsBody body,
 std::uint32_t kind,NativeTu15BodyPolicyScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 const NativeTu15BodyPolicyServices&services,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15BodyPolicyStats*stats){Policy(memory,observer,services,fp,depth,scratch,stats).select(body.identity,kind);}
} // namespace halo2
