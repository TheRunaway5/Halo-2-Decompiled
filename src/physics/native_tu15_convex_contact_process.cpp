#include "halo2/native_tu15_convex_contact_process.hpp"
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
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observe_)observe_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned size=4){std::array<std::byte,4>b{};for(unsigned k=0;k<size;++k)b[k]=std::byte(n>>(k*8u));bytes(pc,p,std::span(b).first(size));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 convex query requires aligned vector DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 convex query requires aligned vector DATA");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);bytes(pc,p,b);}
    void scalar_store(std::uint32_t pc,std::uint32_t p,bool pop){std::uint32_t bits;if(pop)asm volatile("fstps %0":"=m"(bits)::"memory","st");else asm volatile("fsts %0":"=m"(bits)::"memory");word(pc,p,bits);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 convex query requires its unsupported FP exception owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 convex query requires its overfull x87 owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto result=f();resume();return result;}}catch(...){resume();throw;}}
};
__m128 scalar(std::uint32_t n){__m128 value{};std::memcpy(&value,&n,4);return value;}
std::uint32_t low(__m128 value){std::uint32_t n;std::memcpy(&n,&value,4);return n;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 broadcast(std::uint32_t n){const auto value=scalar(n);return _mm_shuffle_ps(value,value,0);}
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void plus(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
void minus(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
void reversed_minus(std::uint32_t bits){asm volatile("fsubrs %0"::"m"(bits):"memory","st");}
void scalar_multiply(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
void negate(){asm volatile("fchs":::"memory","st");}
std::uint16_t compare_pop(std::uint32_t bits){std::uint16_t status;asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
__m128 transpose_direction(__m128 x,__m128 y,__m128 z,__m128 value){const auto xy0=_mm_shuffle_ps(x,y,0x44),xy1=_mm_shuffle_ps(x,y,0xee),zz0=_mm_shuffle_ps(z,z,0x44),zz1=_mm_shuffle_ps(z,z,0xee);const auto tx=_mm_shuffle_ps(xy0,zz0,0x88),ty=_mm_shuffle_ps(xy0,zz0,0xdd),tz=_mm_shuffle_ps(xy1,zz1,0x88);const auto yterm=multiply(ty,_mm_shuffle_ps(value,value,0x55)),xterm=multiply(tx,_mm_shuffle_ps(value,value,0)),zterm=multiply(tz,_mm_shuffle_ps(value,value,0xaa));return add(add(xterm,yterm),zterm);}
#endif
}
void native_tu15_process_convex_contact_agent(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15ContactContext context,
    NativeTu15ContactOutput output,NativeTu15ConvexProcessScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConvexProcessServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ConvexProcessStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if((scratch.identity&15u)||!scratch.result_scalar)
            throw std::runtime_error("Native TU1.5 convex process requires owned aligned DATA and retained scalar");
        Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity,a=agent.identity;
        const auto profile=[&](std::uint32_t cursor,std::uint32_t limit,std::uint32_t label,
            std::uint32_t scalar,std::uint32_t label_pc,std::uint32_t scalar_pc,
            std::uint32_t stamp_pc,std::uint32_t cursor_pc){
            if(cursor>=limit)return cursor;
            m.word(label_pc,cursor,label);
            require(services.geometry.timestamp_counter,"Native TU1.5 convex process requires actual timestamp owner");
            m.word(scalar_pc,scalar,std::uint32_t(services.geometry.timestamp_counter()));
            const auto stamp=m.word(scalar);m.word(stamp_pc,cursor+4u,stamp);
            m.word(cursor_pc,0x48c974u,cursor+12u);if(stats)++stats->profile_entries;return cursor+12u;
        };
        const auto limit=m.word(0x48c978u);auto cursor=m.word(0x48c974u);
        cursor=profile(cursor,limit,0x41cd44u,p+0x1cu,0x2f8b80u,0x2f8b88u,0x2f8b90u,0x2f8b96u);
        push(m.word(a+0x2cu));const auto initial=compare_pop(m.word(0x46a0e8u));
        const auto second_shape=m.word(second.identity),first_shape=m.word(first.identity);
        m.word(0x2f8bafu,p+0x1cu,second_shape);m.word(0x2f8bb5u,p+0x18u,first_shape);
        bool remove=false;
        if(!(initial&0x4100u)){
            if(stats)++stats->fast_projections;
            cursor=profile(cursor,m.word(0x48c978u),0x41cd3cu,p+0x14u,0x2f8bcau,0x2f8bd2u,0x2f8bdau,0x2f8be0u);
            const auto am=m.word(first.identity+8u);const auto start_a=m.vector(am);
            const auto bm=m.word(second.identity+8u);const auto end_a=m.vector(am+0x10u);
            push(m.word(bm+0xcu));const auto end_b=m.vector(bm+0x10u),normal=m.vector(a+0x20u);
            const auto am_now=m.word(first.identity+8u);plus(m.word(am_now+0xcu));
            const auto delta_a=subtract(end_a,start_a),start_b=m.vector(bm),delta_b=subtract(end_b,start_b);
            const auto product=multiply(subtract(delta_b,delta_a),normal);
            const auto xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product);
            const auto dot=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);
            m.word(0x2f8c30u,p+0x14u,low(dot));plus(m.word(p+0x14u));
            reversed_minus(m.word(a+0x2cu));m.scalar_store(0x2f8c3bu,a+0x2cu,false);
            if(!(compare_pop(m.word(0x46a0e8u))&0x4100u)){
                remove=true;if(stats)++stats->fast_separations;
            }else cursor=m.word(0x48c974u);
        }
        if(!remove){
            cursor=profile(cursor,m.word(0x48c978u),0x41cd34u,p+0x14u,0x2f8c5du,0x2f8c65u,0x2f8c6du,0x2f8c73u);
            const auto bm=m.word(second.identity+8u),am=m.word(first.identity+8u);
            environment.dependency([&]{NativeTu15InverseGeometryMemory data(memory,p+0x80u,am+0x20u,bm+0x20u,observe);native_tu15_compose_inverse_affine_geometry(data,fp);});
            const auto first_now=m.word(p+0x18u),second_now=m.word(p+0x1cu);
            m.word(0x2f8cb5u,p+0xccu,0,1);if(stats)++stats->simplex_calls;
            const auto solved=environment.dependency([&]{require(services.geometry.solve_simplex,"Native TU1.5 convex process requires actual30E930 simplex owner");return services.geometry.solve_simplex(p+0xc0u,{first_now},{second_now},p+0x80u,a+0xcu,p+0x70u,fp);});
            if(solved)remove=true;
            else{
                environment.dependency([&]{native_tu15_build_convex_contact_result(memory,p+0xc0u,p+0x70u,p+0x20u,scratch.result_scalar,fp,observe);});
                const auto transform=m.word(first.identity+8u);
                environment.dependency([&]{native_tu15_rotate_contact_direction(memory,p+0x60u,transform+0x20u,p+0x20u,fp,observe);});
                push(m.word(p+0x40u));const auto radius_shape=m.word(p+0x18u);
                minus(m.word(radius_shape+0xcu));const auto other_radius_shape=m.word(p+0x1cu);
                minus(m.word(other_radius_shape+0xcu));m.scalar_store(0x2f8d0bu,p+0x18u,true);
                const auto distance=m.word(p+0x18u);push(m.word(p+0x18u));
                m.word(0x2f8d17u,p+0x40u,distance);const auto relation=compare_pop(m.word(context.identity+8u));
                if((relation&0x0500u)==0x0100u){
                    push(m.word(radius_shape+0xcu));const auto normal=m.vector(p+0x20u),translation=m.vector(p+0xb0u);
                    negate();const auto y=m.vector(p+0x90u);m.scalar_store(0x2f8d46u,p+0x14u,true);
                    const auto radius=m.word(p+0x14u);push(m.word(p+0x18u));negate();m.scalar_store(0x2f8d5du,p+0x14u,true);
                    const auto point=m.vector(p+0x30u),shifted=add(point,multiply(broadcast(radius),normal));
                    const auto negative_distance=m.word(p+0x14u);const auto correction=multiply(broadcast(negative_distance),normal);
                    const auto count=a+0x18u,records=a+0x30u;m.vector(0x2f8d80u,p+0x30u,shifted);
                    const auto combined=add(shifted,correction),z=m.vector(p+0xa0u),relative=subtract(combined,translation),x=m.vector(p+0x80u);
                    const auto other_point=transpose_direction(x,y,z,relative);
                    m.word(0x2f8dbbu,p+0x14u,count);const auto manager=m.word(a+8u);
                    m.word(0x2f8dc3u,p+0x1cu,records);m.vector(0x2f8dffu,p+0x50u,other_point);
                    if(stats)++stats->contacts;
                    environment.dependency([&]{require(services.publish_contact,"Native TU1.5 convex process requires actual30CD00 contact publication owner");services.publish_contact(first,second,context,{p+0x20u},manager,records,count,fp);});
                    push(m.word(p+0x40u));const auto current_manager=m.word(a+8u);minus(m.word(0x46a180u));
                    const auto current_count=m.word(p+0x14u),current_records=m.word(p+0x1cu);
                    m.scalar_store(0x2f8e2cu,scratch.result_scalar,true);const auto published_distance=m.word(scratch.result_scalar);
                    if(stats)++stats->distance_publications;
                    environment.dependency([&]{require(services.publish_distance,"Native TU1.5 convex process requires actual30CF40 distance publication owner");services.publish_distance(first,second,context,published_distance,current_records,current_count,output,current_manager,fp);});
                }else{
                    if(!m.word(a+0x18u)){
                        const auto settings=m.word(context.identity+0xcu);
                        if(m.word(settings,1)){
                            m.vector(0x2f8e50u,a+0x20u,m.vector(p+0x60u));push(m.word(context.identity+8u));
                            scalar_multiply(m.word(0x46a190u));reversed_minus(m.word(p+0x18u));m.scalar_store(0x2f8e61u,a+0x2cu,true);
                            if(stats)++stats->direction_updates;
                        }
                    }
                    remove=true;
                }
            }
        }
        if(remove){
            const auto manager=m.word(a+8u);if(stats)++stats->cache_removals;
            environment.dependency([&]{native_tu15_remove_cached_contact_points(memory,a+0x30u,a+0x18u,manager,fp,services.cleanup,observe);});
        }
        cursor=m.word(0x48c974u);profile(cursor,m.word(0x48c978u),0x41cabcu,p+0x14u,0x2f8e86u,0x2f8e8eu,0x2f8e96u,0x2f8e9cu);
#else
        (void)memory;(void)agent;(void)first;(void)second;(void)context;(void)output;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native TU1.5 convex process unavailable on this architecture");
#endif
        if(stats)++stats->completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
