#include "halo2/native_campaign_camera_control_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
    memory.write(address,event.written);if(observe)observe(event);
}
std::uint32_t animation_identity(const SourcePhysicsContactMemory &memory,std::uint32_t unit) {
    const auto pool=read(memory,0x4ecbd0u),rows=read(memory,pool+0x44u);
    const auto body=read(memory,rows+(unit&0xffffu)*12u+8u);
    const auto offset=signed_word(read(memory,body+0x12au,2));
    const auto animation=body+std::uint32_t(offset);
    if(read(memory,animation+0x68u)==none || read(memory,animation)==none || read(memory,animation+6u,2)==0xffffu)return none;
    return read(memory,animation+0x7cu);
}
bool whitelisted_animation(const SourcePhysicsContactMemory &memory,std::uint32_t unit) {
    const auto pool=read(memory,0x4ecbd0u),rows=read(memory,pool+0x44u);
    const auto body=read(memory,rows+(unit&0xffffu)*12u+8u);
    const auto offset=signed_word(read(memory,body+0x12au,2));
    const auto identity=read(memory,body+std::uint32_t(offset)+0x7cu);
    for(std::uint32_t entry=0x473990u;entry<0x4739a4u;entry+=4u)if(read(memory,entry)==identity)return true;
    return false;
}
template<class PublishMode>bool unit_mode(const SourcePhysicsContactMemory &memory,std::uint32_t unit,PublishMode mode) {
    const auto body=native_campaign_camera_validated_unit_tu15(memory,unit);
    mode(0x15835a,0);
    if(!body)return false;
    const auto parent=read(memory,body+0x14u);if(parent==none)return false;
    const auto objects=read(memory,0x4ecbd0u),rows=read(memory,objects+0x44u);
    const auto parent_body=read(memory,rows+(parent&0xffffu)*12u+8u);
    const auto action=animation_identity(memory,unit);
    const auto kind=read(memory,parent_body+0xaau,1);
    bool seat{};
    if((1u<<(kind&31u))&3u) {
        const auto definition_datum=read(memory,parent_body),tags=read(memory,0x4f0414u);
        const auto definition=read(memory,tags+(definition_datum&0xffffu)*16u+8u);
        const auto index=signed_word(read(memory,body+0x1fcu,2));
        const auto seats=read(memory,definition+0x1ccu);
        const auto flags=read(memory,seats+std::uint32_t(index)*0xb0u);
        seat=(flags&0x10u)!=0;
        if(flags&0x40u) {
            if(whitelisted_animation(memory,unit)){mode(0x1583fb,1);return true;}
            if(action==0x400004au){mode(0x158418,3);return true;}
        }
    }
    mode(0x158432,2);return seat;
}
class SimdEnvironment {
    NativeCampaignCameraCommandFpTu15 &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeCampaignCameraCommandFpTu15 &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::invalid_argument("Native camera mode needs masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
bool ordered_equal(std::uint32_t a,std::uint32_t b) {
    const auto left=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(a)));
    const auto right=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(b)));
    unsigned char equal,ordered;
    asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(left),"x"(right):"cc","memory");return equal&&ordered;
}
class NoTransition final:public NativeCampaignCameraBooleanArgumentTu15 {public:std::uint8_t value()const override{return 0;}};
}
std::uint32_t native_campaign_camera_validated_unit_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t unit) {
    if(unit==none)return 0;
    const auto objects=read(memory,0x4ecbd0u),index=unit&0xffffu;
    if(std::int32_t(index)>=std::bit_cast<std::int32_t>(read(memory,objects+0x38u)))return 0;
    const auto stride=read(memory,objects+0x24u),rows=read(memory,objects+0x44u),header=rows+index*stride;
    const auto salt=signed_word(read(memory,header,2));
    if(!salt || salt!=(std::bit_cast<std::int32_t>(unit)>>16))return 0;
    const auto type=read(memory,header+3u,1);
    return ((1u<<(type&31u))&3u)?read(memory,header+8u):0u;
}
NativeCampaignCameraUnitModeTu15 native_campaign_camera_unit_mode_value_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t unit) {
    NativeCampaignCameraUnitModeTu15 result;
    result.seat_camera=unit_mode(memory,unit,[&](std::uint32_t,std::uint16_t mode){result.mode=mode;});return result;
}
bool native_campaign_camera_unit_mode_tu15(SourcePhysicsContactMemory &memory,std::uint32_t unit,
    std::uint32_t destination,const SourcePhysicsContactRecordObserver &observe) {
    return unit_mode(memory,unit,[&](std::uint32_t site,std::uint16_t mode){publish(memory,observe,site,destination,mode,2);});
}
void native_campaign_camera_set_mode_tu15(SourcePhysicsContactMemory &memory,std::int32_t view,
    std::uint32_t callback,const NativeCampaignCameraBooleanArgumentTu15 &transition,
    NativeCampaignCameraCommandFpTu15 &fp,const SourcePhysicsContactRecordObserver &observe) {
    const auto one=read(memory,0x46a0d0u),base=0x4f5514u+std::uint32_t(view)*0x140u;
    const auto enabled=transition.value();
    publish(memory,observe,0x15891a,base+0xcu,callback);publish(memory,observe,0x15891d,base+0x10cu,one);
    publish(memory,observe,0x158925,base+0x108u,0,1);
    if(!enabled || !read(memory,0x4ecc20u))return;
    const auto scenario=read(memory,0x4ecc1cu),durations=read(memory,scenario+0xecu);
    const auto duration=read(memory,durations+(callback==0x245ea0u?0x10u:callback==0x245400u?0xcu:8u));
    const auto reference=read(memory,0x46a0e8u);
    SimdEnvironment environment(fp);
    if(!ordered_equal(duration,reference))publish(memory,observe,0x158972,base+4u,duration);
}
void native_campaign_camera_control_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignCameraBooleanArgumentTu15 &argument,NativeCampaignCameraCommandFpTu15 &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto control=read(memory,0x4f5504u);auto active=argument.value();const auto one=read(memory,0x46a0d0u);
    const auto users=read(memory,0x4f54f0u);publish(memory,observe,0x158584,control,active,1);
    for(unsigned view=0;view<4u;++view) {
        const auto at=0x4f5526u+view*0x140u;
        if(active) {
            publish(memory,observe,0x158596,at-6u,0x16f6a0u);publish(memory,observe,0x15859d,at+0xfau,one);
            publish(memory,observe,0x1585a5,at+0xf6u,0,1);
        } else {
            auto unit=none;const auto player=read(memory,users+0xcu+view*4u);
            if(player!=none) {
                const auto players=read(memory,0x4f54f4u),rows=read(memory,players+0x44u);
                unit=read(memory,rows+(player&0xffffu)*0x21cu+0x2cu);
            }
            const auto selected=native_campaign_camera_unit_mode_value_tu15(memory,unit);
            if(selected.seat_camera) {
                publish(memory,observe,0x1585fc,at-2u,0,1);publish(memory,observe,0x1585ff,at-1u,0,1);
                publish(memory,observe,0x158602,at,0,1);publish(memory,observe,0x158604,at+1u,0,1);
                publish(memory,observe,0x158607,at+4u,0,2);publish(memory,observe,0x15860b,at+6u,none);
                publish(memory,observe,0x15860e,at+0xau,0xffffu,2);publish(memory,observe,0x158612,at+0x12u,0);
                publish(memory,observe,0x158617,at+0xeu,0);publish(memory,observe,0x15861c,at+0x16u,one);
                publish(memory,observe,0x158621,at+2u,0,1);
            } else {publish(memory,observe,0x15862c,at-2u,0);publish(memory,observe,0x158631,at+2u,0);}
            NoTransition transition;native_campaign_camera_set_mode_tu15(memory,std::int32_t(view),selected.seat_camera?0x245ea0u:0x245400u,transition,fp,observe);
            publish(memory,observe,0x158649,at+0x46u,selected.mode,2);
            active=argument.value();
        }
        const auto director=read(memory,0x51df6cu);
        publish(memory,observe,0x158657,director,active,1);publish(memory,observe,0x158659,director+1u,1,1);
    }
}
}
