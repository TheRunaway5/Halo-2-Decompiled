#include "halo2/native_campaign_ai_alert_permission_tu15.hpp"
#include "halo2/native_campaign_ai_character_tu15.hpp"
#include <array>
#include <cstring>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t value{};
    for(unsigned k=0;k<n;++k)value|=std::uint32_t(std::to_integer<unsigned>(b[k]))<<(k*8u);
    return value;
}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value,unsigned n=4){
    std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(k*8u));m.write(p,std::span(b).first(n));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class ExtendedEnvironment {
    alignas(16)std::array<std::byte,108>ambient_{};std::array<std::byte,28>source_{};NativeTu15PhysicsFrameFp&fp_;
public:
    explicit ExtendedEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp){
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u))throw std::runtime_error("Native alert probability requires its original FP trap owner");
        asm volatile("fnsave %0":"=m"(ambient_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(source_.data(),ambient_.data(),source_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(source_.data(),&fp.control_word,2);std::memcpy(source_.data()+4,&fp.status,2);std::memcpy(source_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(source_):"memory");
    }
    ~ExtendedEnvironment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));
        asm volatile("frstor %0"::"m"(ambient_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
};
// One native mathematical quantity at the composed25980 ->2656C5 boundary.
// It contains no virtual CPU, register file or reconstructed source stack.
struct Probability {std::array<std::byte,10> extended;};
Probability random_probability(SourcePhysicsContactMemory&m,std::uint32_t owner){
    const auto previous=read(m,owner);
    const auto next=previous*1664525u+1013904223u;
    write(m,owner,next);
    const std::int32_t sample=std::int32_t(next>>16u);
    asm volatile("fildl %0"::"m"(sample):"memory","st");
    const auto normalization=read(m,0x46a160u);Probability result;
    asm volatile("fmuls %1; fstpt %0":"=m"(result.extended):"m"(normalization):"memory","st");
    return result;
}
#endif
}
void native_campaign_ai_alert_permission_tu15(SourcePhysicsContactMemory&m,
    std::uint32_t actor,std::uint32_t perception,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    const auto actors=read(m,0x5020c8u),properties=read(m,0x50eef0u);
    const auto property_rows=read(m,properties+0x44u),actor_rows=read(m,actors+0x44u);
    const auto actor_offset=(actor&0xffffu)*0x888u;
    const auto object=read(m,property_rows+(perception&0xffffu)*0x3cu+0x20u);
    const auto selected=read(m,actor_rows+actor_offset+0x368u);
    const auto record=actor_rows+actor_offset;
    if(perception!=selected)return;
    if(read(m,record+0x35eu,1)){
        write(m,record+0x35cu,0,1);
        const auto rows=read(m,actors+0x44u),unit=read(m,rows+actor_offset+0x18u);
        if(unit==none)return;
        throw NativeCampaignAiAlertRequiredOwnerTu15(0x26568au,0x213d30u,unit,object);
    }
    if(!read(m,record+0x35au,2)){
        const auto kind=read(m,record+0x358u,2);
        if(kind!=1u&&kind!=4u){
            const auto character=read(m,record+0x54u);
            const auto policy=native_campaign_ai_character_navigation_tu15(m,character);
            const auto owner=read(m,0x4f3cd8u);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            if(occupied_x87>6u)throw std::runtime_error("Native alert probability requires its original x87 stack-fault owner");
            ExtendedEnvironment environment(fp);
            const auto probability=random_probability(m,owner);
            const auto threshold=read(m,policy+0xcu);unsigned char permitted;
            asm volatile("fldt %1; flds %2; fcomip %%st(1),%%st; fstp %%st(0); seta %0"
                :"=qm"(permitted):"m"(probability.extended),"m"(threshold):"memory","cc","st","st(1)");
            write(m,record+0x35cu,permitted,1);return;
#else
            (void)owner;(void)policy;(void)fp;(void)occupied_x87;
            throw std::runtime_error("Native alert probability arithmetic backend unavailable");
#endif
        }
    }
    write(m,record+0x35cu,1,1);
}
}
