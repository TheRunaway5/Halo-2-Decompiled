#include "halo2/native_campaign_ai_path_distance_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include <bit>
#include <cstring>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiPathDistancePendingErrorTu15::NativeCampaignAiPathDistancePendingErrorTu15(
    NativeCampaignAiPathDistancePendingTu15 pending,std::exception_ptr cause):
    std::runtime_error("Path distance retained previous-Z/Y-X differences"),pending_(pending),cause_(std::move(cause)){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
std::int16_t index_byte(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    return std::bit_cast<std::int8_t>(std::uint8_t(read(memory,address,1)));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class ArithmeticEnvironment {
    alignas(16)std::array<std::byte,108> host_{};
    NativeTu15PhysicsFrameFp &fp_;unsigned mxcsr_;
public:
    explicit ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),mxcsr_(_mm_getcsr()) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native path distance requires its masked FP policy");
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
NativeCampaignAiPathDistancePendingTu15 pending_differences() {
    alignas(16)std::array<std::byte,108> saved{};NativeCampaignAiPathDistancePendingTu15 pending{};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(saved)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    for(auto &quantity:pending.previous_z_dy_dx)asm volatile("fstpt %0":"=m"(quantity)::"memory","st");
    asm volatile("frstor %0"::"m"(saved):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");return pending;
}
#endif
}
std::uint32_t native_campaign_ai_path_distance_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t actor,NativeTu15PhysicsFrameFp &fp,unsigned occupied) {
    const auto pool=read(memory,0x5020c8u),rows=read(memory,pool+0x44u),record=rows+(actor&0xffffu)*0x888u;
    return native_campaign_ai_actor_path_distance_tu15(memory,record,fp,occupied);
}
std::uint32_t native_campaign_ai_actor_path_distance_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t record,NativeTu15PhysicsFrameFp &fp,unsigned occupied,
    const NativeCampaignAiProjectionCoordinateObserverTu15 &coordinate) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!read(memory,record+0x50cu,1) || read(memory,record+0x504u,2)!=1)return 0;
    std::array<std::uint32_t,3> previous{read(memory,record+0x238u),read(memory,record+0x23cu),read(memory,record+0x240u)};
    auto index=index_byte(memory,record+0x53au);const auto initial_end=index_byte(memory,record+0x539u);
    std::uint32_t accumulated{};if(index>=initial_end)return accumulated;
    for(;;) {
        const auto point=record+0x548u+std::uint32_t(std::int32_t(index))*0x1cu;
        const auto current=native_campaign_ai_project_point_value_tu15(memory,point,fp,coordinate);
        if(occupied>3u)throw std::invalid_argument("Path distance requires five free original x87 temporaries");
        bool another{};
        {
            ArithmeticEnvironment environment(fp);
            asm volatile("flds %0\n\tfsubs %1\n\tflds %2\n\tfsubs %3\n\tflds %4"::
                "m"(previous[0]),"m"(current[0]),"m"(previous[1]),"m"(current[1]),"m"(previous[2]):"memory","st","st(1)","st(2)");
            std::int16_t end;
            try {end=index_byte(memory,record+0x539u);}
            catch(...){throw NativeCampaignAiPathDistancePendingErrorTu15(pending_differences(),std::current_exception());}
            index=std::bit_cast<std::int16_t>(std::uint16_t(std::int32_t(index)+1));another=index<end;
            // Source association sqrt((dz*dz + dx*dx) + dy*dy) remains
            // extended through the addition, then spills once per segment.
            asm volatile("fsubs %2\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\tfld %%st(3)\n\tfmul %%st(4),%%st\n\tfaddp %%st,%%st(1)\n\tfld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfadds %1\n\tfstps %0"
                :"=m"(accumulated):"m"(accumulated),"m"(current[2]):"memory","st","st(1)","st(2)","st(3)","st(4)");
        }
        previous=current;if(!another)return accumulated;
    }
#else
    (void)memory;(void)record;(void)fp;(void)occupied;(void)coordinate;
    throw std::runtime_error("Exact native campaign path distance backend unavailable");
#endif
}
}
