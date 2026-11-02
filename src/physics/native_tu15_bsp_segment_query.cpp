#include "halo2/native_tu15_bsp_segment_query.hpp"
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
    const auto zero=F{};
    while(!(node&0x800000u)){
        if(stats)++stats->nodes;
        const auto descriptor=m.word(context+4u),nodes=m.word(descriptor+4u),record=nodes+(node&0x7fffffu)*8u;
        const auto plane_index=sign16(m.word(record,2)),planes=m.word(descriptor+12u),origin=m.word(context+16u),plane=planes+plane_index*16u;
        auto distance=scalar(m.word(origin+8u));auto y=scalar(m.word(origin+4u));
        y=mul(y,scalar(m.word(plane+4u)));distance=mul(distance,scalar(m.word(plane+8u)));distance=add(distance,y);
        auto x=scalar(m.word(plane));x=mul(x,scalar(m.word(origin)));
        const auto displacement=m.word(context+20u);auto dy=scalar(m.word(displacement+4u));dy=mul(dy,scalar(m.word(plane+4u)));distance=add(distance,x);
        auto slope=scalar(m.word(displacement+8u));slope=mul(slope,scalar(m.word(plane+8u)));distance=sub(distance,scalar(m.word(plane+12u)));slope=add(slope,dy);
        auto dx=scalar(m.word(displacement));dx=mul(dx,scalar(m.word(plane)));slope=add(slope,dx);
        auto at_max=mul(slope,scalar(maximum));auto at_min=mul(slope,scalar(minimum));at_min=add(at_min,distance);at_max=add(at_max,distance);
        const bool min_side=!compare(at_min,zero).below,max_side=!compare(at_max,zero).below;
        if(min_side==max_side){node=std::uint32_t(std::int32_t(m.word(record+1u+unsigned(min_side)*3u))>>8);continue;}
        const bool slope_positive=compare(slope,zero).above();const auto cross=crossing(bits(distance),bits(slope));
        const auto near_node=std::uint32_t(std::int32_t(m.word(record+1u+unsigned(!slope_positive)*3u))>>8);
        if(traverse(memory,m,environment,context,near_node,minimum,cross,fp,depth,observe,stats))return true;
        const auto output=m.word(context+24u);if(!compare(scalar(cross),scalar(m.word(output))).below)return false;
        const auto current_plane=sign16(m.word(record,2));m.word(0x1e6ab0u,context+36u,current_plane);
        node=std::uint32_t(std::int32_t(m.word(record+1u+unsigned(slope_positive)*3u))>>8);minimum=cross;
    }
    if(stats)++stats->leaves;
    std::uint32_t leaf=0xffffffffu;std::uint8_t kind=3;
    if(node!=0xffffffffu){const auto descriptor=m.word(context+4u),leaves=m.word(descriptor+20u);leaf=node&0x7fffffu;kind=std::uint8_t((m.word(leaves+leaf*4u,1)&1u)+1u);}
    const auto flags=m.word(context);const auto first=flags&1u;std::uint32_t candidate=0xffffffffu;bool check_surface{},verify{};
    if(first){const auto previous=m.word(context+32u,1);if((previous==1u||previous==2u)&&kind==3u){candidate=m.word(context+28u);check_surface=true;}}
    if(!check_surface&&(flags&2u)){if(m.word(context+32u,1)==3u&&(kind==1u||kind==2u)){candidate=leaf;check_surface=true;}}
    if(!check_surface&&!(flags&4u)&&m.word(context+32u,1)==2u&&kind==2u){candidate=first?m.word(context+28u):leaf;verify=true;check_surface=true;}
    if(check_surface&&candidate!=0xffffffffu){
        const auto descriptor=m.word(context+4u),displacement=m.word(context+20u),origin=m.word(context+16u),plane_index=m.word(context+36u),bitmap=m.word(context+12u);const auto limit=std::uint16_t(m.word(context+8u,2));
        if(stats)++stats->surface_queries;
        const auto surface=environment.dependency([&]{return native_tu15_query_bsp_leaf_surface(memory,{descriptor,candidate,origin,displacement,limit,bitmap,plane_index,minimum,verify},fp,depth);});
        if(surface!=0xffffffffu){const auto surfaces=m.word(descriptor+44u);const auto surface_flags=m.word(surfaces+surface*8u+4u,1),row=surfaces+surface*8u;
            if(!((surface_flags&2u)&&(flags&8u))&&!((surface_flags&8u)&&(flags&16u))){
                auto output=m.word(context+24u);m.word(0x1e6c05u,output,minimum);
                const auto current_descriptor=m.word(context+4u),planes=m.word(current_descriptor+12u),plane=m.word(context+36u);output=m.word(context+24u);m.word(0x1e6c1au,output+4u,planes+plane*16u);
                output=m.word(context+24u);m.word(0x1e6c24u,output+8u,candidate);output=m.word(context+24u);m.word(0x1e6c2au,output+12u,surface);
                const auto material=sign16(m.word(row,2));output=m.word(context+24u);m.word(0x1e6c33u,output+16u,material);
                output=m.word(context+24u);const auto flags_byte=m.word(row+4u,1);m.word(0x1e6c3cu,output+20u,flags_byte,1);
                output=m.word(context+24u);const auto brush=m.word(row+5u,1);m.word(0x1e6c46u,output+21u,brush,1);
                output=m.word(context+24u);const auto partition=m.word(row+6u,2);m.word(0x1e6c52u,output+22u,partition,2);if(stats)++stats->hits;return true;
            }
        }
    }
    if(leaf!=0xffffffffu){const auto output=m.word(context+24u),count=m.word(output+24u);if(std::int32_t(count)<0x100){m.word(0x1e6c7au,output+28u+count*4u,leaf);const auto current_output=m.word(context+24u),current_count=m.word(current_output+24u);m.word(0x1e6c81u,current_output+24u,current_count+1u);m.word(0x1e6c85u,context+28u,leaf);m.word(0x1e6c89u,context+32u,kind,1);return false;}m.word(0x1e6c96u,output+0x418u,leaf);}
    m.word(0x1e6c9du,context+28u,leaf);m.word(0x1e6ca1u,context+32u,kind,1);return false;
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
    Environment environment(fp,depth);Memory m(memory,observe);const auto epsilon=scalar(q.epsilon_bits),zero=F{};const auto negative=compare(zero,epsilon).above();
    m.word(0x1e68e9u,context,q.flags);m.word(0x1e68f1u,context+4u,q.descriptor);m.word(0x1e68f9u,context+8u,q.bitmap_limit,2);m.word(0x1e6902u,context+12u,q.bitmap);m.word(0x1e690au,context+16u,q.origin);m.word(0x1e690eu,context+20u,q.displacement);m.word(0x1e6912u,context+24u,q.output);
    m.word(0x1e6920u,q.output,negative?0u:q.epsilon_bits);m.word(0x1e6924u,q.output+24u,0u);const auto negative_again=compare(zero,epsilon).above();m.word(0x1e692du,context+28u,0xffffffffu);m.word(0x1e6931u,context+32u,0u,1);m.word(0x1e6935u,context+36u,0xffffffffu);
    std::uint32_t maximum{};if(!negative_again){const auto one=m.word(0x46a0d0u);maximum=compare(epsilon,scalar(one)).above()?one:q.epsilon_bits;}
    const auto result=traverse(memory,m,environment,context,0,0,maximum,fp,depth,observe,stats);if(stats)++stats->completed;return result;
#else
    (void)memory;(void)q;(void)context;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Native BSP segment backend unavailable");
#endif
}
} // namespace halo2
