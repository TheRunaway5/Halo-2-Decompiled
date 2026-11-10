#include "halo2/native_tu15_camera_instance_query_value.hpp"
#include "halo2/native_tu15_camera_instance_bitmap_kernel.hpp"
#include "halo2/native_tu15_camera_instance_admission_value.hpp"
#include "halo2/native_tu15_camera_instanced_query.hpp"
#include "halo2/native_tu15_camera_query_leaves.hpp"
#include "halo2/native_tu15_camera_dynamic_query_leaves.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include "halo2/native_tu15_bsp_segment_query.hpp"
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
bool different_or_unordered(F a,F b){unsigned char different,unordered;asm volatile("ucomiss %3,%2;setne %0;setp %1":"=qm"(different),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");return different||unordered;}
#endif
}
bool native_tu15_admit_camera_query_instance(const SourcePhysicsContactMemory&memory,
 std::uint32_t instance,std::uint32_t definition,std::uint32_t mask){
 const SourcePhysicsContactRecordObserver none;Memory m(const_cast<SourcePhysicsContactMemory&>(memory),none);
 native_camera_instance_admission_detail::Values view{m,instance,definition,mask};
 native_camera_instance_admission_detail::Progress data;
 return native_camera_instance_admission_detail::admit(view,data);
}
std::uint32_t native_tu15_camera_instance_collision_bitmap(const SourcePhysicsContactMemory&memory,std::uint32_t instance){
 const SourcePhysicsContactRecordObserver none;Memory m(const_cast<SourcePhysicsContactMemory&>(memory),none);
 native_camera_instance_bitmap_detail::Values view{m,instance};native_camera_instance_bitmap_detail::Progress data;
 return native_camera_instance_bitmap_detail::locate(view,data);
}
bool native_tu15_query_camera_instance(SourcePhysicsContactMemory&memory,
 const NativeTu15CameraInstanceQuery&q,NativeTu15CameraInstanceQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CameraInstanceQueryStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 Memory m(memory,observe);
 const NativeTu15CameraInstanceQuery input{q.instance,q.mask,q.bsp_filter,q.origin,q.displacement,q.result};
 native_camera_instance_query_value_detail::View<Memory,Environment> view{memory,m,input,scratch,fp,depth,observe,stats};
 native_camera_instance_query_detail::Progress<__m128> data;return native_camera_instance_query_detail::query(view,data);
#else
 if(stats)++stats->calls;
 (void)memory;(void)q;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Native camera instance query backend unavailable");
#endif
}
} // namespace halo2
