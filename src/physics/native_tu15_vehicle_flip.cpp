#include "halo2/native_tu15_vehicle_flip.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned prior_;bool active_{};
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),prior_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 vehicle flip requires its x87 overflow owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);
        try{resume();}catch(...){asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);throw;}
    }
    ~Environment(){if(active_)suspend();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
    void suspend(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){
        if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 vehicle flip requires its unsupported numerical-fault owner");
        const std::uint16_t empty=0xffffu;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;
    }
};
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
std::uint32_t bits(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 divide(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 convert(std::int32_t v){auto out=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(out):"r"(v):"memory");return out;}
bool greater(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2,%1; seta %0":"=qm"(out):"x"(a),"x"(b):"cc","memory");return out;}
bool below(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2,%1; setb %0":"=qm"(out):"x"(a),"x"(b):"cc","memory");return out;}
std::int32_t rounded_integer(__m128 v){const auto value=bits(v);std::int32_t out;asm volatile("flds %1; fistpl %0":"=m"(out):"m"(value):"memory","st");return out;}
std::uint32_t rounded_double(std::uint64_t v){std::uint32_t out;asm volatile("fldl %1; fstps %0":"=m"(out):"m"(v):"memory","st");return out;}
class Flip {
    SourcePhysicsContactMemory&memory_;NativeTu15PhysicsFrameFp&fp_;unsigned depth_;
    const NativeTu15VehicleFlipServices&services_;const SourcePhysicsContactRecordObserver&observer_;NativeTu15VehicleFlipStats*stats_;
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t out{};for(unsigned n=0;n<width;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;}
    std::uint64_t wide(std::uint32_t p)const{std::array<std::byte,8>b{};memory_.read(p,b);std::uint64_t out{};for(unsigned n=0;n<8;++n)out|=std::uint64_t(std::to_integer<unsigned>(b[n]))<<(n*8u);return out;}
    __m128 value(std::uint32_t p)const{return scalar(word(p));}
    void byte(std::uint32_t site,std::uint32_t p,std::uint32_t v){const std::byte b{std::uint8_t(v)};SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(1);memory_.read(p,event.before);event.written.assign(1,b);}memory_.write(p,std::span(&b,1));if(observer_)observer_(event);}
    bool clear(std::uint32_t vehicle){byte(0xf20a5u,vehicle+0x34eu,0);byte(0xf20abu,vehicle+0x34du,0);byte(0xf20b1u,vehicle+0x348u,word(vehicle+0x348u,1)&0xbfu);return false;}
public:
    Flip(SourcePhysicsContactMemory&m,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15VehicleFlipServices&s,const SourcePhysicsContactRecordObserver&o,NativeTu15VehicleFlipStats*stats):memory_(m),fp_(fp),depth_(depth),services_(s),observer_(o),stats_(stats){}
    bool run(std::uint32_t object){
        if(stats_)++stats_->calls;const auto pool=word(0x4ecbd0u),headers=word(pool+0x44u),tags=word(0x4f0414u),vehicle=word(headers+(object&0xffffu)*12u+8u),definition_datum=word(vehicle),flags=word(vehicle+0x348u,1),definition=word(tags+(definition_datum&0xffffu)*16u+8u);
        if(!(flags&0x40u)){if(stats_)++stats_->flag_skips;return clear(vehicle);}
        if(!word(vehicle+0x34du,1)){if(stats_)++stats_->direction_skips;return clear(vehicle);}
        Environment environment(fp_,depth_);const auto clock=word(0x51df54u);const auto frequency=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word(clock+2u,2))));
        const auto maximum_ticks=rounded_integer(mul(convert(frequency),value(0x46a11cu)));
        if(std::int32_t(word(vehicle+0x34eu,1))>=maximum_ticks){if(stats_)++stats_->timeouts;return clear(vehicle);}
        const auto up_threshold=value(0x46a194u),up_z=value(vehicle+0x84u);if(below(up_threshold,up_z)){if(stats_)++stats_->orientation_skips;return clear(vehicle);}
        const auto sign_direction=word(vehicle+0x34du,1),signed_gain=rounded_double(wide(sign_direction==2u||sign_direction==4u?0x46a6d0u:0x46a8c0u));
        if(!services_.get_velocities){if(stats_)++stats_->unbound_getters;throw std::runtime_error("Native TU1.5 vehicle flip requires actual BC200 velocity ownership");}
        if(stats_)++stats_->get_calls;environment.suspend();auto velocities=services_.get_velocities(object,fp_);environment.resume();
        const auto direction=word(vehicle+0x34du,1);__m128 x,y,z;
        if(direction!=3u&&direction!=4u){const auto fx=word(vehicle+0x70u),fy=word(vehicle+0x74u),fz=word(vehicle+0x78u);x=scalar(fx);y=scalar(fy);z=scalar(fz);}
        else{
            x=value(vehicle+0x84u);auto temporary=value(vehicle+0x80u);temporary=mul(temporary,value(vehicle+0x78u));x=mul(x,value(vehicle+0x74u));
            y=value(vehicle+0x7cu);y=mul(y,value(vehicle+0x78u));z=value(vehicle+0x70u);z=mul(z,value(vehicle+0x80u));x=sub(x,temporary);
            temporary=value(vehicle+0x84u);temporary=mul(temporary,value(vehicle+0x70u));y=sub(y,temporary);temporary=value(vehicle+0x7cu);temporary=mul(temporary,value(vehicle+0x74u));z=sub(z,temporary);
        }
        const auto current_clock=word(0x51df54u);auto gain=value(vehicle+0x84u);gain=divide(gain,value(current_clock+4u));gain=mul(gain,value(0x46a448u));const auto minimum=value(definition+0x23cu);
        if(greater(minimum,gain))gain=minimum;else if(greater(gain,value(definition+0x240u)))gain=value(definition+0x240u);gain=mul(gain,scalar(signed_gain));
        if(direction==1u||direction==2u){
            auto cross_x=value(vehicle+0x84u),temporary=value(vehicle+0x78u);temporary=mul(temporary,value(vehicle+0x80u));cross_x=mul(cross_x,value(vehicle+0x74u));
            auto y_sub=value(vehicle+0x84u);y_sub=mul(y_sub,value(vehicle+0x70u));auto z_sub=value(vehicle+0x74u);z_sub=mul(z_sub,value(vehicle+0x7cu));cross_x=sub(cross_x,temporary);
            temporary=value(vehicle+0x78u);temporary=mul(temporary,value(vehicle+0x7cu));const auto cross_y=sub(temporary,y_sub);auto cross_z=value(vehicle+0x70u);cross_z=mul(cross_z,value(vehicle+0x80u));cross_z=sub(cross_z,z_sub);
            const auto negative_z=sub(_mm_setzero_ps(),value(vehicle+0x78u));const auto corrected_y=add(mul(cross_y,negative_z),y);auto corrected_x=mul(negative_z,cross_x),corrected_z=mul(cross_z,negative_z);corrected_x=add(corrected_x,x);corrected_z=add(corrected_z,z);x=corrected_x;y=corrected_y;z=corrected_z;
        }
        const auto vehicle_kind=word(definition+0x1f0u,2);x=mul(x,gain);y=mul(y,gain);z=mul(z,gain);
        const Vec3 angular{std::bit_cast<float>(bits(x)),std::bit_cast<float>(bits(y)),std::bit_cast<float>(bits(z))};
        if(!vehicle_kind){
            auto projected=value(vehicle+0x74u);projected=mul(projected,scalar(std::bit_cast<std::uint32_t>(velocities.linear.y)));auto other=value(vehicle+0x78u);other=mul(other,scalar(std::bit_cast<std::uint32_t>(velocities.linear.z)));projected=add(projected,other);
            other=value(vehicle+0x70u);other=mul(other,scalar(std::bit_cast<std::uint32_t>(velocities.linear.x)));projected=add(projected,other);
            const auto linear_x=mul(value(vehicle+0x70u),projected),linear_y=mul(value(vehicle+0x74u),projected),linear_z=mul(value(vehicle+0x78u),projected);
            velocities.linear={std::bit_cast<float>(bits(linear_x)),std::bit_cast<float>(bits(linear_y)),std::bit_cast<float>(bits(linear_z))};
        }
        if(!services_.set_velocities){if(stats_)++stats_->unbound_setters;throw std::runtime_error("Native TU1.5 vehicle flip requires actual B9800 native activation ownership");}
        if(stats_)++stats_->set_calls;environment.suspend();services_.set_velocities(object,velocities.linear,angular,fp_);
        byte(0xf2096u,vehicle+0x34eu,word(vehicle+0x34eu,1)+1u);if(stats_)++stats_->completed;return true;
    }
};
}
bool native_tu15_flip_vehicle(SourcePhysicsContactMemory&memory,std::uint32_t object,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15VehicleFlipServices&services,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15VehicleFlipStats*stats){return Flip(memory,fp,depth,services,observer,stats).run(object);}
} // namespace halo2
