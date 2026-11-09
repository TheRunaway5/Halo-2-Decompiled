#include "halo2/native_tu15_bsp_segment_query.hpp"
#include "halo2/native_tu15_bsp_segment_kernel.hpp"
#include "halo2/native_tu15_bsp_segment_setup_kernel.hpp"
#include "halo2/native_tu15_bsp_surface_query.hpp"
#include <bit>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <type_traits>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8*k);return n;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned width=4){std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(8*k));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(width);m_.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m_.write(p,std::span(b).first(width));if(o_)o_(e);}
};
std::uint32_t sign16(std::uint32_t n){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(n))));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;bool active_{};
    void validate(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native BSP segment requires its numerical-fault owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){if(depth>5)throw std::runtime_error("Native BSP segment requires x87 capacity owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();}
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>auto dependency(F&&f){publish();try{auto value=f();resume();return value;}catch(...){resume();throw;}}
};
using F=__m128;
F scalar(std::uint32_t n){F v{};std::memcpy(&v,&n,4);return v;}
std::uint32_t bits(F v){std::uint32_t n;std::memcpy(&n,&v,4);return n;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Compare{bool below,equal;bool above()const{return!below&&!equal;}};
Compare compare(F a,F b){unsigned char below,equal;asm volatile("comiss %3,%2;setb %0;sete %1":"=qm"(below),"=qm"(equal):"x"(a),"x"(b):"cc","memory");return{below!=0,equal!=0};}
std::uint32_t crossing(std::uint32_t distance,std::uint32_t slope){std::uint32_t out;asm volatile("flds %1;fdivs %2;fchs;fstps %0":"=m"(out):"m"(distance),"m"(slope):"memory","st");return out;}
bool traverse(SourcePhysicsContactMemory&memory,Memory&m,Environment&environment,
    std::uint32_t context,std::uint32_t node,std::uint32_t minimum,std::uint32_t maximum,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15BspSegmentQueryStats*stats){
    auto surface=[&](const native_bsp_segment_detail::Progress&data){
        return environment.dependency([&]{return native_tu15_query_bsp_leaf_surface(memory,{data.descriptor,data.candidate,data.origin,data.displacement,std::uint16_t(data.bitmap_limit),data.bitmap,data.plane_index,data.minimum,(data.verify&255u)!=0},fp,depth);});
    };
    return native_bsp_segment_detail::traverse_values(m,context,node,minimum,maximum,surface,stats);
}
#endif
}
bool native_tu15_traverse_bsp_segment(SourcePhysicsContactMemory&memory,std::uint32_t context,std::uint32_t node,std::uint32_t minimum,std::uint32_t maximum,NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,NativeTu15BspSegmentQueryStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);Memory m(memory,observe);return traverse(memory,m,environment,context,node,minimum,maximum,fp,depth,observe,stats);
#else
    (void)memory;(void)context;(void)node;(void)minimum;(void)maximum;(void)fp;(void)depth;(void)observe;(void)stats;throw std::runtime_error("Native BSP segment backend unavailable");
#endif
}
bool native_tu15_query_bsp_segment(SourcePhysicsContactMemory&memory,const NativeTu15BspSegmentQuery&q,std::uint32_t context,NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,NativeTu15BspSegmentQueryStats*stats){
    if(stats)++stats->calls;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);Memory m(memory,observe);
    const auto walk=[&](const native_bsp_segment_setup_detail::Progress&d){return traverse(memory,m,environment,context,0,0,d.maximum,fp,depth,observe,stats);};
    struct Values {
        const NativeTu15BspSegmentQuery&q;std::uint32_t address;Memory&m;const decltype(walk)&continue_walk;
        std::uint32_t context()const{return address;}
        std::uint32_t flags()const{return q.flags;}
        std::uint32_t descriptor()const{return q.descriptor;}
        std::uint32_t output()const{return q.output;}
        auto epsilon()const{return native_bsp_segment_setup_detail::scalar(q.epsilon_bits);}
        std::uint32_t limit()const{return q.bitmap_limit;}
        std::uint32_t bitmap()const{return q.bitmap;}
        std::uint32_t origin()const{return q.origin;}
        std::uint32_t displacement()const{return q.displacement;}
        std::uint32_t maximum(std::uint32_t value)const{return value;}
        bool traverse(const native_bsp_segment_setup_detail::Progress&d){return continue_walk(d);}
        void store(native_bsp_segment_setup_detail::Write kind,std::uint32_t at,std::uint32_t value,unsigned width){
            using W=native_bsp_segment_setup_detail::Write;std::uint32_t pc{};
            // The public value API has no exposed caller argument slot. Its
            // observable DATA stores retain their original field acquisition
            // points, including observers that hold another reference to q.
            switch(kind){
            case W::flags:pc=0x1e68e9u;value=q.flags;break;case W::descriptor:pc=0x1e68f1u;value=q.descriptor;break;
            case W::limit:pc=0x1e68f9u;value=q.bitmap_limit;break;case W::bitmap:pc=0x1e6902u;value=q.bitmap;break;
            case W::origin:pc=0x1e690au;value=q.origin;break;case W::displacement:pc=0x1e690eu;value=q.displacement;break;
            case W::output:pc=0x1e6912u;value=q.output;break;case W::fraction:pc=0x1e6920u;at=q.output;break;
            case W::count:pc=0x1e6924u;at=q.output+24u;break;case W::previous_leaf:pc=0x1e692du;break;
            case W::previous_kind:pc=0x1e6931u;break;case W::partition:pc=0x1e6935u;break;
            case W::maximum_zero:case W::maximum_one:case W::maximum_epsilon:return;
            }
            m.word(pc,at,value,width);
        }
    }input{q,context,m,walk};
    const auto result=native_bsp_segment_setup_detail::setup(m,input,[](auto,const auto&){});if(stats)++stats->completed;return result;
#else
    (void)memory;(void)q;(void)context;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Native BSP segment backend unavailable");
#endif
}
} // namespace halo2
