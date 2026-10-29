#include "halo2/native_campaign_ai_perception_group_pressure_tu15.hpp"
#include "halo2/native_campaign_ai_clump_retirement_tu15.hpp"
#include "halo2/native_campaign_ai_clump_callbacks_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_initialize_tu15.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
#include <array>
#include <bit>
#include <optional>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(at,std::span(bytes).first(width));
}
std::int16_t signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native perception pressure requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
__m128 convert(std::int32_t value) {
    auto result=scalar(0);asm volatile("cvtsi2ss %1,%0":"+x"(result):"r"(value):"memory");return result;
}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
bool greater(__m128 left,__m128 right) {
    unsigned char result{};asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(left),"x"(right):"cc","memory");return result;
}
__m128 clamp_unit(const SourcePhysicsContactMemory &memory,__m128 value) {
    if(greater(scalar(0),value))return scalar(0);
    const auto one=scalar(read(memory,0x46a0d0u));
    return greater(value,one)?one:value;
}
#endif
struct RetainedRequest {
    const SourcePhysicsContactMemory &memory;std::uint32_t address;
    std::uint16_t priority()const{return std::uint16_t(read(memory,address+8u,2));}
    std::uint32_t object()const{return read(memory,address+4u);}
    std::int16_t kind()const{return signed16(read(memory,address,2));}
};
struct LocalRequest {
    const NativeCampaignAiPerceptionGroupRequestTu15 &value;
    std::uint16_t priority()const{return value.priority;}
    std::uint32_t object()const{return value.object;}
    std::int16_t kind()const{return value.kind;}
};
template<class Request> std::uint32_t construct(SourcePhysicsContactMemory &memory,
    std::uint32_t clump,const Request &request,NativeTu15PhysicsFrameFp &fp) {
    const auto group=native_campaign_ai_perception_group_allocate_tu15(memory,clump,signed16(request.priority()),fp);
    if(group==none)return none;
    const auto groups=read(memory,0x50eef4u);
    const auto rows=read(memory,groups+0x44u);
    const auto record=rows+(group&0xffffu)*0xc4u;
    const auto priority=request.priority();
    const auto object=request.object();
    const auto kind=request.kind();
    native_campaign_ai_perception_group_initialize_tu15(memory,record,object,clump,kind,priority);
    return group;
}
}
std::uint16_t native_campaign_ai_perception_group_prune_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t clump,std::int16_t minimum_priority) {
    const auto clumps=read(memory,0x50eef8u);
    const auto clump_rows=read(memory,clumps+0x44u);
    const auto record=clump_rows+(clump&0xffffu)*0x50u;
    auto group=read(memory,record+0x14u);
    std::uint16_t count{};
    while(group!=none) {
        const auto groups=read(memory,0x50eef4u);
        const auto rows=read(memory,groups+0x44u);
        const auto current=rows+(group&0xffffu)*0xc4u;
        const auto priority=signed16(read(memory,current+0xcu,2));
        const auto next=read(memory,current+0x14u);
        if(priority<minimum_priority) {
            native_campaign_ai_perception_group_retirement_tu15(memory,record,group,
                [&](const auto &request){native_campaign_ai_clump_reference_invalidation_tu15(memory,request);});
            count=std::uint16_t(count+1u);
        }
        group=next;
    }
    return count;
}
std::uint32_t native_campaign_ai_perception_group_allocate_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t clump,std::int16_t priority,
    NativeTu15PhysicsFrameFp &fp) {
    if(priority<signed16(read(memory,clump+0x24u,2)))return none;
    const auto pool=read(memory,0x50eef4u);
    const auto allocated=native_tu15_data_array_allocate(memory,pool);
    if(allocated!=none)return allocated;
    const auto clock=read(memory,0x51df54u);
    const auto now=read(memory,clock+8u);
    std::uint16_t retired{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // The original numeric owner is entered only for an eligible pressure
    // candidate. Disabled/empty/threshold-rejected scans perform no FP math.
    std::optional<SimdEnvironment> environment;
    const auto begin_arithmetic=[&]{if(!environment)environment.emplace(fp);};
    while(true) {
        const auto state=read(memory,0x5020a8u);
        const bool enabled=read(memory,state+1u,1)!=0;
        auto best=none;
        auto best_score=scalar(0);
        if(enabled) {
            const auto clump_pool=read(memory,0x50eef8u);
            NativeTu15DataArrayIterator iterator{0,clump_pool,none,none};
            while(native_tu15_data_array_advance(memory,iterator)) {
                const auto candidate=iterator.current;
                const auto candidate_priority=signed16(read(memory,candidate+0x24u,2));
                if(candidate_priority>=4)continue;
                if(candidate==clump && std::int32_t(priority)<=std::int32_t(candidate_priority)+1)continue;
                __m128 secondary;
                if(!read(memory,candidate+0x1cu,1)) {
                    const auto previous=read(memory,candidate+0x20u);
                    const auto current_clock=read(memory,0x51df54u);
                    begin_arithmetic();
                    secondary=convert(std::bit_cast<std::int32_t>(now-previous));
                    secondary=multiply(secondary,scalar(read(memory,current_clock+4u)));
                    secondary=multiply(secondary,scalar(read(memory,0x46a6acu)));
                    secondary=clamp_unit(memory,secondary);
                    secondary=multiply(secondary,scalar(read(memory,0x46a6a8u)));
                    secondary=add(secondary,scalar(read(memory,0x46a128u)));
                }else {
                    const auto members=signed16(read(memory,candidate+0x26u,2));
                    begin_arithmetic();
                    secondary=convert(3-std::int32_t(members));
                    secondary=multiply(secondary,scalar(read(memory,0x46a174u)));
                    secondary=clamp_unit(memory,secondary);
                    secondary=multiply(secondary,scalar(read(memory,0x46a11cu)));
                }
                auto score=convert(3-std::int32_t(candidate_priority));
                score=multiply(score,scalar(read(memory,0x46a100u)));
                score=add(score,secondary);
                if(greater(score,best_score)){best_score=score;best=iterator.datum;}
            }
        }
        if(best==none) {
            if(signed16(retired)<=0)return none;
            return native_tu15_data_array_allocate(memory,read(memory,0x50eef4u));
        }
        const auto clumps=read(memory,0x50eef8u);
        const auto rows=read(memory,clumps+0x44u);
        const auto candidate=rows+(best&0xffffu)*0x50u;
        write(memory,candidate+0x24u,std::uint16_t(read(memory,candidate+0x24u,2)+1u),2);
        const auto threshold=signed16(read(memory,candidate+0x24u,2));
        retired=native_campaign_ai_perception_group_prune_tu15(memory,best,threshold);
        if(signed16(retired)>0)return native_tu15_data_array_allocate(memory,read(memory,0x50eef4u));
    }
#else
    (void)now;(void)retired;(void)fp;
    throw std::runtime_error("Exact native perception pressure backend unavailable");
#endif
}
std::uint32_t native_campaign_ai_perception_group_construct_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t clump,
    const NativeCampaignAiPerceptionGroupRequestTu15 &request,NativeTu15PhysicsFrameFp &fp) {
    return construct(memory,clump,LocalRequest{request},fp);
}
std::uint32_t native_campaign_ai_perception_group_construct_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t clump,std::uint32_t request,
    NativeTu15PhysicsFrameFp &fp) {
    return construct(memory,clump,RetainedRequest{memory,request},fp);
}
}
