#include "halo2/native_tu15_contact_query.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
#include <cstring>
#include <bit>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
template<class F>void require(const F&f,const char*why){if(!f)throw std::runtime_error(why);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observer_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned size=4){std::array<std::byte,4>b{};for(unsigned k=0;k<size;++k)b[k]=std::byte(n>>(k*8u));bytes(pc,p,std::span(b).first(size));}
    NativeTu15ContactVirtualOwner owner(std::uint32_t object,unsigned slot)const{return{word(word(object)+slot)};}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 contact query requires aligned vector storage");std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 v;std::memcpy(&v,b.data(),16);return v;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 v){if(p&15u)throw std::runtime_error("Native TU1.5 contact query requires aligned vector storage");std::array<std::byte,16>b{};std::memcpy(b.data(),&v,16);bytes(pc,p,b);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned mxcsr_;bool active_{};
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),mxcsr_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 contact query requires its overfull x87 owner");
        validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 contact query requires its unsupported FP owner");}
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto result=f();resume();return result;}}catch(...){resume();throw;}}
};
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
std::uint32_t scalar(__m128 v){std::uint32_t n;std::memcpy(&n,&v,4);return n;}
__m128 broadcast(std::uint32_t n){__m128 value{};std::memcpy(&value,&n,4);return _mm_shuffle_ps(value,value,0);}
std::uint16_t compare(std::uint32_t left,std::uint32_t right){std::uint16_t status;asm volatile("flds %1\n\tfcomps %2\n\tfnstsw %0":"=a"(status):"m"(left),"m"(right):"memory","st");return status;}
__m128 direction(__m128 x,__m128 y,__m128 z,__m128 value){const auto xy0=_mm_shuffle_ps(x,y,0x44),xy1=_mm_shuffle_ps(x,y,0xee),zz0=_mm_shuffle_ps(z,z,0x44),zz1=_mm_shuffle_ps(z,z,0xee);const auto tx=_mm_shuffle_ps(xy0,zz0,0x88),ty=_mm_shuffle_ps(xy0,zz0,0xdd),tz=_mm_shuffle_ps(xy1,zz1,0x88);const auto yterm=multiply(ty,_mm_shuffle_ps(value,value,0x55));const auto xterm=multiply(tx,_mm_shuffle_ps(value,value,0));const auto zterm=multiply(tz,_mm_shuffle_ps(value,value,0xaa));return add(add(xterm,yterm),zterm);}
#endif
}
bool native_tu15_body_contact_eligible(SourcePhysicsContactMemory&memory,
    NativeTu15ContactQueryBody body){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
    const auto count=m.word(body.identity+0x34u);std::uint32_t property{};
    if(std::bit_cast<std::int32_t>(count)>0){
        const auto properties=m.word(body.identity+0x30u);auto cursor=properties;
        for(std::uint32_t index=0;index<count;++index,cursor+=8u)
            if(m.word(cursor)==0x2001u){property=m.word(properties+index*8u+4u);break;}
    }
    if(property==0xffffffffu)return true;
    const auto components=m.word(m.word(0x52be5cu)+0x44u);
    const auto unit_datum=m.word(components+(property&0xffffu)*160u+8u);
    const auto units=m.word(m.word(0x4ecbd0u)+0x44u);
    const auto unit=m.word(units+(unit_datum&0xffffu)*12u+8u);
    const auto flags=m.word(unit+0xc0u,2);
    if(!(flags&6u))return true;
    const auto parent=m.word(unit+0xb8u);if(parent==0xffffffffu)return true;
    const auto roster=m.word(0x51daf0u),members=m.word(roster+0x1d80u);
    if(std::bit_cast<std::int32_t>(members)<=0)return true;
    auto cursor=roster+0x1d00u;
    for(std::uint32_t index=0;index<members;++index,cursor+=4u)
        if(m.word(cursor)==parent)return false;
    return true;
}
std::uint32_t native_tu15_query_contact(SourcePhysicsContactMemory&memory,
    NativeTu15ContactChild child,std::uint32_t valid_result,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15ContactContext context,std::uint32_t contact_output,
    NativeTu15ContactQueryScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ContactQueryServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ContactQueryStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(scratch.identity&15u)throw std::runtime_error("Native TU1.5 contact query requires aligned retained data");
    Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity;
    if(stats)++stats->calls;
    m.word(0x2e4ff0u,p+0x1cu,child.identity);
    const auto settings=m.word(context.identity+0xcu),old_setting=m.word(settings,1);
    const auto first_shape=m.word(first.identity);
    const auto first_table=m.word(first_shape);
    m.word(0x2e4fffu,p+0x24u,0,1);m.word(0x2e5004u,p+0x1au,old_setting,1);m.word(0x2e5008u,p+0x20u,0);
    const auto classify=[&](std::uint32_t shape,NativeTu15ContactVirtualOwner owner){if(stats)++stats->classifications;return environment.dependency([&]{require(services.classify_shape,"Native TU1.5 contact query requires actual shape class owner");return services.classify_shape({shape},owner,fp);});};
    const auto eligible=[&](NativeTu15ContactQueryBody body){if(stats)++stats->eligibility_tests;return environment.dependency([&]{return native_tu15_body_contact_eligible(memory,body);});};
    const auto length=[&](std::uint32_t motion){const auto v=m.vector(motion+0x40u);const auto product=multiply(v,v),yx=add_scalar(_mm_shuffle_ps(product,product,0x55),product),sum=add_scalar(_mm_shuffle_ps(product,product,0xaa),yx);const auto bits=scalar(sum);m.word(0x2e505au,p+0x28u,bits);return bits;};
    if(classify(first_shape,{m.word(first_table+0x14u)})==23u){
        // Source rereads the shape after resolving the first parent pointer.
        const auto parent=m.word(first.identity+0xcu),shape=m.word(first.identity);auto root=first.identity,next=parent;while(next){root=next;next=m.word(root+0xcu);}const NativeTu15ContactQueryBody body{m.word(root+0x20u)};
        const auto motion=m.word(body.identity+0x3cu);length(motion);
        const auto measured=m.word(p+0x28u),threshold=m.word(shape+0x20u);
        if(!(compare(measured,threshold)&0x4100u)&&eligible(body)){
            const auto fresh_motion=m.word(body.identity+0x3cu),transform=m.word(first.identity+8u);
            const auto z=m.vector(transform+0x40u),v=m.vector(fresh_motion+0x40u),x=m.vector(transform+0x20u),y=m.vector(transform+0x30u);
            m.vector(0x2e50ecu,shape+0x10u,direction(x,y,z,v));
            const auto scale=m.word(context.identity+0x10u);const auto prior=m.vector(shape+0x10u);m.word(0x2e50f7u,p+0x28u,scale);m.vector(0x2e510bu,shape+0x10u,multiply(broadcast(m.word(p+0x28u)),prior));
            m.word(0x2e510fu,p+0x20u,1);m.word(0x2e5117u,p+0x24u,1,1);if(stats)++stats->motion_vectors;
        }else m.vector(0x2e5121u,shape+0x10u,_mm_setzero_ps());
    }
    const auto second_shape=m.word(second.identity);const auto second_method=m.owner(second_shape,0x14u);
    if(classify(second_shape,second_method)==23u){
        const auto parent=m.word(second.identity+0xcu),shape=m.word(second.identity);auto root=second.identity,next=parent;while(next){root=next;next=m.word(root+0xcu);}const NativeTu15ContactQueryBody body{m.word(root+0x20u)};
        const auto motion=m.word(body.identity+0x3cu);const auto v=m.vector(motion+0x40u),product=multiply(v,v),yx=add_scalar(_mm_shuffle_ps(product,product,0x55),product),sum=add_scalar(_mm_shuffle_ps(product,product,0xaa),yx);m.word(0x2e5176u,p+0x28u,scalar(sum));const auto measured=m.word(p+0x28u),threshold=m.word(shape+0x20u);
        const auto status=compare(measured,threshold);m.word(0x2e5181u,p+0x2cu,body.identity);
        if(!(status&0x4100u)&&eligible(body)){
            const auto transform=m.word(second.identity+8u);const auto z=m.vector(transform+0x40u),x=m.vector(transform+0x20u),y=m.vector(transform+0x30u);const auto saved_body=m.word(p+0x2cu),fresh_motion=m.word(saved_body+0x3cu);const auto velocity=m.vector(fresh_motion+0x40u);
            m.vector(0x2e5210u,shape+0x10u,direction(x,y,z,velocity));
            const auto scale=m.word(context.identity+0x10u);const auto prior=m.vector(shape+0x10u);m.word(0x2e521bu,p+0x2cu,scale);const auto multiplier=m.word(p+0x2cu),next_state=m.word(p+0x20u)+2u;m.vector(0x2e5236u,shape+0x10u,multiply(broadcast(multiplier),prior));m.word(0x2e523au,p+0x20u,next_state);m.word(0x2e523eu,p+0x24u,1,1);if(stats)++stats->motion_vectors;
        }else m.vector(0x2e5248u,shape+0x10u,_mm_setzero_ps());
    }
    const auto actual_child=m.word(p+0x1cu),motion_state=m.word(p+0x20u);
    const bool different=motion_state!=m.word(actual_child+0xf0u);auto query_child=actual_child;
    if(different||m.word(p+0x24u,1)){
        if(different)m.word(0x2e5262u,m.word(context.identity+0xcu),0,1);
        const auto second_transform=m.word(second.identity+8u),cache_first=m.word(first.identity),first_transform=m.word(first.identity+8u),cache_second=m.word(second.identity);
        m.word(0x2e5280u,p+0x24u,cache_first);
        environment.dependency([&]{NativeTu15InverseGeometryMemory storage(memory,p+0x30u,first_transform+0x20u,second_transform+0x20u,observe);native_tu15_compose_inverse_affine_geometry(storage,fp);});
        const auto cache_class=classify(cache_second,m.owner(cache_second,0x14u));const auto first_now=m.word(p+0x24u),cache_owner=m.word(p+0x1cu);
        environment.dependency([&]{require(services.initialize_cache,"Native TU1.5 contact query requires actual cache initializer");services.initialize_cache(cache_class==5u?0x309150u:0x309060u,cache_owner+0xcu,{first_now},{cache_second},p+0x30u,fp);});
        query_child=m.word(p+0x1cu);m.word(0x2e52ceu,query_child+0x2cu,0xbf800000u);if(stats)++stats->cache_refreshes;
    }
    if(stats)++stats->convex_queries;
    environment.dependency([&]{require(services.query_convex,"Native TU1.5 contact query requires actual2F8630 convex query owner");services.query_convex({query_child},p+0x1bu,first,second,context,contact_output,fp);});
    const auto final_state=m.word(p+0x20u);m.word(0x2e52f4u,query_child+0xf0u,final_state);
    const auto current_settings=m.word(context.identity+0xcu),restore=m.word(p+0x1au,1);m.word(0x2e5305u,current_settings,restore,1);
    const auto valid=m.word(p+0x1bu,1);m.word(0x2e530cu,valid_result,valid,1);return valid_result;
#else
    (void)memory;(void)child;(void)valid_result;(void)first;(void)second;(void)context;(void)contact_output;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;throw std::runtime_error("Exact native TU1.5 contact query backend unavailable");
#endif
}
} // namespace halo2
