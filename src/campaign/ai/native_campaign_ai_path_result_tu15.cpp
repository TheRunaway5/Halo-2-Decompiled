#include "halo2/native_campaign_ai_path_result_tu15.hpp"
#include "halo2/native_campaign_ai_path_candidates_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_frame_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include <bit>
#include <cstring>
#include <optional>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiPathResultPendingErrorTu15::NativeCampaignAiPathResultPendingErrorTu15(
    NativeCampaignAiPathResultPendingTu15 pending,std::exception_ptr cause):
    std::runtime_error("Path result retained pending mathematical quantities"),pending_(pending),cause_(std::move(cause)){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
std::uint32_t node(std::uint32_t workspace,std::uint16_t index) {
    return workspace+0xb0u+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(index)))*0x44u;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class ArithmeticEnvironment {
    alignas(16)std::array<std::byte,108> host_{};
    NativeTu15PhysicsFrameFp &fp_;unsigned mxcsr_;
public:
    explicit ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),mxcsr_(_mm_getcsr()) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native path result requires its masked actual FP policy");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28> native{};std::memcpy(native.data(),host_.data(),native.size());const std::uint16_t empty=0xffffu;
        std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);std::memcpy(native.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~ArithmeticEnvironment() {
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(mxcsr_);
    }
};
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native path result requires its SIMD exception owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(Scalar a,Scalar b){unsigned char value{};asm volatile("comiss %2,%1\n\tseta %0":"=qm"(value):"x"(a),"x"(b):"memory","cc");return value;}
// The218C40 association differs from218D00. Keep X/Z/Y for equal frames,
// and Z/Y/X after the two real point projections for different frames.
std::uint32_t distance_squared(const SourcePhysicsContactMemory &memory,std::uint32_t first,
    std::uint32_t second,NativeTu15PhysicsFrameFp &fp) {
    const auto first_frame=read(memory,first+0xcu,2),second_frame=read(memory,second+0xcu,2);
    if(first_frame==second_frame) {
        SimdEnvironment environment(fp);
        auto x=scalar(read(memory,second));x=subtract(x,scalar(read(memory,first)));
        auto z=scalar(read(memory,second+8u));z=subtract(z,scalar(read(memory,first+8u)));
        auto y=scalar(read(memory,second+4u));y=subtract(y,scalar(read(memory,first+4u)));
        auto sum=multiply(x,x);const auto z_squared=multiply(z,z);sum=add(sum,z_squared);
        return bits(add(sum,multiply(y,y)));
    }
    const auto a=native_campaign_ai_project_point_value_tu15(memory,first,fp);
    const auto b=native_campaign_ai_project_point_value_tu15(memory,second,fp);
    SimdEnvironment environment(fp);
    const auto z=subtract(scalar(b[2]),scalar(a[2])),y=subtract(scalar(b[1]),scalar(a[1])),x=subtract(scalar(b[0]),scalar(a[0]));
    auto sum=multiply(z,z);sum=add(sum,multiply(y,y));return bits(add(sum,multiply(x,x)));
}
NativeCampaignAiPathResultPendingTu15 pending_quantity(NativeCampaignAiPathResultPendingTu15::Stage stage,unsigned count) {
    alignas(16)std::array<std::byte,108> saved{};NativeCampaignAiPathResultPendingTu15 value{stage,count,{}};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(saved)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    for(unsigned n=0;n<count;++n)asm volatile("fstpt %0":"=m"(value.quantities[n])::"memory","st");
    asm volatile("frstor %0"::"m"(saved):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");return value;
}
struct Cost {std::uint32_t value;bool clearance;};
Cost path_cost(const SourcePhysicsContactMemory &memory,std::uint32_t workspace,std::uint32_t record,
    std::uint32_t goal,NativeTu15PhysicsFrameFp &fp,unsigned occupied) {
    const auto squared=distance_squared(memory,record+0x18u,goal,fp);
    if(occupied>7u)throw std::invalid_argument("Path cost requires one free original x87 temporary");
    ArithmeticEnvironment environment(fp);Cost result{};
    asm volatile("flds %0\n\tfsqrt"::"m"(squared):"memory","st");
    try {
        result.clearance=read(memory,workspace+0x28u,1)!=0;
        const auto stored=read(memory,record+0x30u);
        asm volatile("fadds %1\n\tfstps %0":"=m"(result.value):"m"(stored):"memory","st");
    }catch(...){throw NativeCampaignAiPathResultPendingErrorTu15(pending_quantity(NativeCampaignAiPathResultPendingTu15::Stage::Distance,1),std::current_exception());}
    return result;
}
std::uint32_t path_clearance(const SourcePhysicsContactMemory &memory,std::uint32_t workspace,
    std::uint32_t record,std::uint32_t goal,NativeTu15PhysicsFrameFp &fp,unsigned occupied) {
    const auto start=native_campaign_ai_project_point_value_tu15(memory,record+0x18u,fp);
    const auto end=native_campaign_ai_project_point_value_tu15(memory,goal,fp);
    const auto closest=native_campaign_ai_path_segment_candidate_value_tu15(memory,start,end,workspace+0x2cu,fp);
    if(occupied>3u)throw std::invalid_argument("Path clearance requires five free original x87 temporaries");
    ArithmeticEnvironment environment(fp);using Stage=NativeCampaignAiPathResultPendingTu15::Stage;
    std::optional<Stage> pending;unsigned count{};std::uint32_t value;
    try {
        asm volatile("flds %0"::"m"(closest[0]):"memory","st");pending=Stage::ClosestX;count=1;
        auto coordinate=read(memory,workspace+0x2cu);
        asm volatile("fsubs %0\n\tflds %1"::"m"(coordinate),"m"(closest[1]):"memory","st","st(1)");pending=Stage::ClosestYAndX;count=2;
        coordinate=read(memory,workspace+0x30u);
        asm volatile("fsubs %0\n\tflds %1"::"m"(coordinate),"m"(closest[2]):"memory","st","st(1)","st(2)");pending=Stage::ClosestZAndYX;count=3;
        coordinate=read(memory,workspace+0x34u);
        asm volatile("fsubs %1\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\tfld %%st(3)\n\tfmul %%st(4),%%st\n\tfaddp %%st,%%st(1)\n\tfld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfstps %0"
            :"=m"(value):"m"(coordinate):"memory","st","st(1)","st(2)","st(3)","st(4)");
        pending.reset();const auto limit=read(memory,record+0x2cu);unsigned char capped{};
        asm volatile("flds %1\n\tflds %2\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=qm"(capped):"m"(limit),"m"(value):"memory","cc","st","st(1)");
        if(capped)return read(memory,record+0x2cu);
        return value;
    }catch(...){if(pending)throw NativeCampaignAiPathResultPendingErrorTu15(pending_quantity(*pending,count),std::current_exception());throw;}
}
std::uint32_t route_target(SourcePhysicsContactMemory &memory,std::uint32_t workspace,std::uint16_t found,
    std::uint32_t goal,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    auto current=found;std::uint16_t previous=0xffffu;std::uint32_t record{};
    do {
        record=node(workspace,current);publish(memory,observe,0x27a56du,record,previous,2);
        previous=current;current=std::uint16_t(read(memory,record+2u,2));
    }while(current!=0xffffu);
    if(previous==0xffffu)return goal;
    SimdEnvironment environment(fp);auto accumulated=scalar(0);
    const auto threshold=scalar(read(memory,0x456910u));current=previous;
    while(above(threshold,accumulated)) {
        record=node(workspace,current);const auto step=scalar(read(memory,record+0x28u));
        current=std::uint16_t(read(memory,record,2));accumulated=add(step,accumulated);
        if(current==0xffffu)return goal;
    }
    return current==0xffffu?goal:record+0x18u;
}
#endif
}
bool native_campaign_ai_path_result_tu15(SourcePhysicsContactMemory &memory,std::uint32_t workspace,
    std::uint32_t key,std::uint32_t goal,std::uint32_t cost_output,std::uint32_t clearance_output,
    std::uint32_t direction_output,NativeTu15PhysicsFrameFp &fp,unsigned occupied,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto found=native_campaign_ai_path_find_node_tu15(memory,workspace,key);
    if(found==-1) {
        const auto distance=read(memory,0x45695cu);
        if(clearance_output)publish(memory,observe,0x27a5feu,clearance_output,distance);
        if(direction_output) {
            const auto fallback=read(memory,0x474d04u);
            for(unsigned n=0;n<3;++n)publish(memory,observe,std::array{0x27a612u,0x27a617u,0x27a61du}[n],
                direction_output+n*4u,read(memory,fallback+n*4u));
        }
        publish(memory,observe,0x27a624u,cost_output,distance);return false;
    }
    const auto record=node(workspace,std::uint16_t(found));
    const auto cost=path_cost(memory,workspace,record,goal,fp,occupied);
    const auto clearance=cost.clearance?path_clearance(memory,workspace,record,goal,fp,occupied):0u;
    if(clearance_output)publish(memory,observe,0x27a527u,clearance_output,clearance);
    publish(memory,observe,0x27a53du,cost_output,cost.value);
    if(direction_output) {
        const auto target=route_target(memory,workspace,std::uint16_t(found),goal,fp,observe);
        native_campaign_ai_framed_difference_tu15(memory,workspace+0x14u,target,direction_output,fp,observe);
        if(occupied>5u)throw std::invalid_argument("Path direction requires three free original x87 temporaries");
        (void)native_campaign_ai_normalize_vector_tu15(memory,direction_output,fp,observe);
    }
    return true;
#else
    (void)memory;(void)workspace;(void)key;(void)goal;(void)cost_output;(void)clearance_output;(void)direction_output;(void)fp;(void)occupied;(void)observe;
    throw std::runtime_error("Exact native campaign path-result backend unavailable");
#endif
}
}
