#include "halo2/native_tu15_constraint_scalar_cache.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 constraint scalar cache requires its unsupported numerical-fault owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>native{};std::memcpy(native.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);std::memcpy(native.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t out{};for(unsigned n=0;n<width;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;}
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2,%1; seta %0":"=qm"(out):"x"(a),"x"(b):"cc","memory");return out;}
__m128 convert(std::int32_t v){auto out=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(out):"r"(v):"memory");return out;}
std::int32_t round_to_integer(__m128 v){const auto bits=std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));std::int32_t result;asm volatile("flds %1; fistpl %0":"=m"(result):"m"(bits):"memory","st");return result;}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned width){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(v>>(n*8u));SourcePhysicsContactRecordStore event{site,p,{},{}};if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}m.write(p,std::span(b).first(width));if(o)o(event);}
}
void native_tu15_update_constraint_cached_scalar(SourcePhysicsContactMemory&memory,std::uint32_t instance,
 std::uint32_t value,NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ConstraintScalarCacheStats*stats){
    if(stats)++stats->calls;Environment environment(fp,depth);const auto incoming=scalar(value);
    if(read(memory,instance+0x11u,1)&&!greater(incoming,scalar(read(memory,instance+0x18u)))){if(stats)++stats->already_retained;return;}
    if(read(memory,instance+0x94u)!=0xffffffffu){
        if(stats)++stats->cooldown_checks;const auto clock=read(memory,0x51df54u),ticks=read(memory,clock+8u);
        const auto frequency=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(memory,clock+2u,2))));
        const auto duration=mul(convert(frequency),scalar(read(memory,0x46a16cu)));const auto rounded=round_to_integer(duration);
        const auto elapsed=std::bit_cast<std::int32_t>(ticks-read(memory,instance+0x94u));
        if(elapsed<=rounded){const auto previous=scalar(read(memory,instance+0x98u)),threshold=mul(previous,scalar(read(memory,0x46a35cu)));
            if(!greater(incoming,threshold)){if(stats)++stats->cooldown_retained;return;}
        }
    }
    store(memory,observer,0x22fe8fu,instance+0x11u,1u,1);store(memory,observer,0x22fe93u,instance+0x18u,value,4);if(stats)++stats->published;
}
} // namespace halo2
