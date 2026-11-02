#include "halo2/native_tu15_moving_shape_process.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
template<class F>void require(const F&f,const char*why){if(!f)throw std::runtime_error(why);}
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned n=4)const {
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(n));
        std::uint32_t value{};for(unsigned k=0;k<n;++k)value|=std::to_integer<unsigned>(b[k])<<(8u*k);return value;
    }
    void store(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(b.size());memory_.read(p,event.before);}
        memory_.write(p,b);
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned n=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(8u*k));store(pc,p,std::span(b).first(n));
    }
    NativeTu15ContactVirtualOwner owner(std::uint32_t p,unsigned slot)const {const auto table=word(p);return{word(table+slot)};}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const {
        if(p&15u)throw std::runtime_error("Native TU1.5 moving shape requires aligned vector DATA");
        std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;
    }
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){
        if(p&15u)throw std::runtime_error("Native TU1.5 moving shape requires aligned vector DATA");
        std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);store(pc,p,b);
    }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};
    std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned mxcsr_;bool active_{};
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),mxcsr_(_mm_getcsr()){
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){
        publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto value=f();resume();return value;}}catch(...){resume();throw;}
    }
};
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
std::uint32_t scalar(__m128 value){std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
__m128 broadcast(std::uint32_t bits){__m128 value{};std::memcpy(&value,&bits,4);return _mm_shuffle_ps(value,value,0);}
std::uint32_t squared_length(__m128 value){
    const auto product=multiply(value,value);
    const auto yx=add_scalar(_mm_shuffle_ps(product,product,0x55),product);
    return scalar(add_scalar(_mm_shuffle_ps(product,product,0xaa),yx));
}
std::uint16_t compare_threshold(Memory&m,std::uint32_t measured,std::uint32_t threshold){
    // FLD precedes the potentially faulting source threshold read.
    asm volatile("flds %0"::"m"(measured):"memory","st");
    const auto bits=m.word(threshold);std::uint16_t status;
    asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;
}
__m128 direction(__m128 x,__m128 y,__m128 z,__m128 velocity){
    const auto xy0=_mm_shuffle_ps(x,y,0x44),xy1=_mm_shuffle_ps(x,y,0xee);
    const auto zz0=_mm_shuffle_ps(z,z,0x44),zz1=_mm_shuffle_ps(z,z,0xee);
    const auto ty=_mm_shuffle_ps(xy0,zz0,0xdd);
    const auto yterm=multiply(ty,_mm_shuffle_ps(velocity,velocity,0x55));
    const auto tx=_mm_shuffle_ps(xy0,zz0,0x88),tz=_mm_shuffle_ps(xy1,zz1,0x88);
    const auto xterm=multiply(tx,_mm_shuffle_ps(velocity,velocity,0));
    const auto zterm=multiply(tz,_mm_shuffle_ps(velocity,velocity,0xaa));
    return add(add(xterm,yterm),zterm);
}
#endif
} // namespace

void native_tu15_process_moving_shape_contact(SourcePhysicsContactMemory&memory,
    NativeTu15ContactChild child,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15ContactContext context,
    std::uint32_t output,NativeTu15MovingShapeProcessScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MovingShapeProcessServices&services,
    const SourcePhysicsContactRecordObserver&observe,
    NativeTu15MovingShapeProcessStats*stats){
    if(stats)++stats->calls;
    try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
            if(stats)++stats->unsupported_fp;
            throw std::runtime_error("Native TU1.5 moving shape requires its qualified FP/depth owner");
        }
        if(scratch.relative_transform&15u)throw std::runtime_error("Native TU1.5 moving shape requires aligned relative transform DATA");
        Memory m(memory,observe);Environment environment(fp);
        const auto profile=[&](std::uint32_t cursor,std::uint32_t limit,bool ending){
            if(cursor>=limit)return;
            m.word(ending?0x2e567au:0x2e533fu,cursor,ending?0x41c394u:0x41c398u);
            require(services.timestamp_counter,"Native TU1.5 moving shape requires its actual profiling clock");
            const auto timestamp=std::uint32_t(environment.dependency(services.timestamp_counter));
            m.word(ending?0x2e568au:0x2e534fu,cursor+4u,timestamp);
            m.word(ending?0x2e5690u:0x2e5355u,0x48c974u,cursor+12u);
            if(stats)++stats->profile_entries;
        };
        const auto limit=m.word(0x48c978u),cursor=m.word(0x48c974u);profile(cursor,limit,false);
        const auto settings=m.word(context.identity+0xcu),old_setting=m.word(settings,1);
        const auto first_shape=m.word(first.identity),first_table=m.word(first_shape);
        const auto classify=[&](std::uint32_t shape,NativeTu15ContactVirtualOwner owner){
            if(stats)++stats->classifications;
            return environment.dependency([&]{require(services.classify_shape,"Native TU1.5 moving shape requires actual shape class owner");return services.classify_shape({shape},owner,fp);});
        };
        const auto eligible=[&](std::uint32_t body){
            if(stats)++stats->eligibility_tests;
            return environment.dependency([&]{return native_tu15_body_contact_eligible(memory,{body});});
        };
        std::uint32_t motion_state{};bool changed{};
        if(classify(first_shape,{m.word(first_table+0x14u)})==23u){
            const auto parent=m.word(first.identity+0xcu),shape=m.word(first.identity);
            auto root=first.identity,next=parent;while(next){root=next;next=m.word(root+0xcu);}
            const auto body=m.word(root+0x20u),motion=m.word(body+0x3cu);
            const auto measured=squared_length(m.vector(motion+0x40u));
            if(!(compare_threshold(m,measured,shape+0x20u)&0x4100u)&&eligible(body)){
                const auto current_motion=m.word(body+0x3cu);
                const auto velocity=m.vector(current_motion+0x40u);
                const auto transform=m.word(first.identity+8u);
                const auto z=m.vector(transform+0x40u),x=m.vector(transform+0x20u),y=m.vector(transform+0x30u);
                m.vector(0x2e5457u,shape+0x10u,direction(x,y,z,velocity));
                const auto scale=m.word(context.identity+0x10u);const auto prior=m.vector(shape+0x10u);
                m.vector(0x2e5476u,shape+0x10u,multiply(broadcast(scale),prior));
                motion_state=1;changed=true;if(stats)++stats->motion_vectors;
            }else{m.vector(0x2e548cu,shape+0x10u,_mm_setzero_ps());if(stats)++stats->cleared_vectors;}
        }
        const auto second_shape=m.word(second.identity);
        if(classify(second_shape,m.owner(second_shape,0x14u))==23u){
            const auto parent=m.word(second.identity+0xcu),shape=m.word(second.identity);
            auto root=second.identity,next=parent;while(next){root=next;next=m.word(root+0xcu);}
            const auto body=m.word(root+0x20u),motion=m.word(body+0x3cu);
            const auto measured=squared_length(m.vector(motion+0x40u));
            if(!(compare_threshold(m,measured,shape+0x20u)&0x4100u)&&eligible(body)){
                const auto current_motion=m.word(body+0x3cu);
                const auto velocity=m.vector(current_motion+0x40u);
                const auto transform=m.word(second.identity+8u);
                const auto z=m.vector(transform+0x40u),x=m.vector(transform+0x20u),y=m.vector(transform+0x30u);
                m.vector(0x2e5581u,shape+0x10u,direction(x,y,z,velocity));
                const auto scale=m.word(context.identity+0x10u);const auto prior=m.vector(shape+0x10u);
                m.vector(0x2e55a3u,shape+0x10u,multiply(broadcast(scale),prior));
                motion_state+=2u;changed=true;if(stats)++stats->motion_vectors;
            }else{m.vector(0x2e55b5u,shape+0x10u,_mm_setzero_ps());if(stats)++stats->cleared_vectors;}
        }
        const bool different=motion_state!=m.word(child.identity+0xf0u);
        if(different||changed){
            if(different)m.word(0x2e55cfu,m.word(context.identity+0xcu),0,1);
            const auto second_transform=m.word(second.identity+8u),first_transform=m.word(first.identity+8u);
            const auto cache_first=m.word(first.identity),cache_second=m.word(second.identity);
            environment.dependency([&]{NativeTu15InverseGeometryMemory storage(memory,scratch.relative_transform,first_transform+0x20u,second_transform+0x20u,observe);native_tu15_compose_inverse_affine_geometry(storage,fp);});
            const auto cache_class=classify(cache_second,m.owner(cache_second,0x14u));
            environment.dependency([&]{require(services.initialize_cache,"Native TU1.5 moving shape requires actual cache initializer");services.initialize_cache(cache_class==5u?0x309150u:0x309060u,child.identity+0xcu,{cache_first},{cache_second},scratch.relative_transform,fp);});
            m.word(0x2e563bu,child.identity+0x2cu,0xbf800000u);if(stats)++stats->cache_refreshes;
        }
        if(stats)++stats->convex_processes;
        environment.dependency([&]{require(services.process_convex,"Native TU1.5 moving shape requires actual2F8B60 processing");services.process_convex(child,first,second,context,output,fp);});
        m.word(0x2e565cu,child.identity+0xf0u,motion_state);
        m.word(0x2e5669u,m.word(context.identity+0xcu),old_setting,1);
        const auto final_cursor=m.word(0x48c974u),final_limit=m.word(0x48c978u);profile(final_cursor,final_limit,true);
        if(stats)++stats->completed;
#else
        (void)memory;(void)child;(void)first;(void)second;(void)context;(void)output;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native TU1.5 moving shape backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
