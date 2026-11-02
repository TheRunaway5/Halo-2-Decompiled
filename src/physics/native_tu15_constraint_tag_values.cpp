#include "halo2/native_tu15_constraint_tag_values.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 tagged constraint value requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(event);
}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
bool greater(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
}
void native_tu15_publish_constraint_tag_value(SourcePhysicsContactMemory&m,std::uint32_t reference,
    std::uint32_t context,std::uint32_t value,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintTagValueStats*stats){
    if(stats)++stats->calls;const auto pool=read(m,0x4ecbecu);
    if(!pool||!read(m,pool+0x29u,1)){if(stats)++stats->disabled_skips;return;}
    if(reference==0xffffffffu||context==0xffffffffu){if(stats)++stats->invalid_inputs;return;}
    const auto definitions=read(m,0x4f0414u),definition=read(m,definitions+(reference&0xffffu)*16u+8u);
    const auto next=[&](std::int32_t first){
        if(stats)++stats->iterator_queries;const auto index=native_tu15_data_array_next(m,pool,first);
        if(index==0xffffffffu)return index;
        const auto stride=read(m,pool+0x24u),rows=read(m,pool+0x44u),salt=read(m,rows+index*stride,2);
        return (salt<<16u)|index;
    };
    auto datum=next(0);if(datum==0xffffffffu)return;
    Environment environment(fp);const auto one=read(m,0x46a0d0u);
    do{const auto rows=read(m,pool+0x44u),index=datum&0xffffu,record=rows+index*0x110u;
        if(stats)++stats->candidates;
        if(read(m,record+0xcu)==context){if(stats)++stats->context_matches;const auto candidate_reference=read(m,record+8u);
            if(candidate_reference==reference){
                if(stats)++stats->reference_matches;
                const auto candidate_definition=read(m,definitions+(candidate_reference&0xffffu)*16u+8u);
                if(read(m,candidate_definition+2u,2)==0&&read(m,definition+2u,2)==0){
                    if(stats)++stats->class_matches;std::uint32_t clamped;
                    if(greater(_mm_setzero_ps(),scalar(value)))clamped=0;
                    else if(greater(scalar(value),scalar(one)))clamped=one;else clamped=value;
                    store(m,observer,0xc61f7u,record+0x10u,clamped);if(stats)++stats->publications;
                }
            }
        }
        datum=next(std::bit_cast<std::int32_t>(index+1u));
    }while(datum!=0xffffffffu);
}
} // namespace halo2
