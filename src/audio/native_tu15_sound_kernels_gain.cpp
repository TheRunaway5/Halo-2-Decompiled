// Native kernels: gain routines (TU1.5 sound manager). A sound's gain range
// comes from its sound class and scale (21F780, 21F7F0); 127CF0 interpolates
// that range as an amplitude (220000 dB to amplitude, 220200 amplitude to dB)
// and adds its class's ducked gain (129B20).
//
// None of these routines stores live memory: every store the original makes
// is stack scratch below its caller's stack pointer after the return. What
// they owe their callers is registers, x87 state and MXCSR, all reproduced
// here exactly (scratch registers included).
#include "audio/native_tu15_sound_kernel.hpp"

namespace halo2::native_tu15_sound {
namespace {
// Retail .rdata constants (TU1.5 XBE).
constexpr std::uint32_t silence_db=0xc2800000u;                   // 0x455B0C: -64.0 dB, the gain floor
constexpr float ln_10=std::bit_cast<float>(0x40135d8eu);          // 0x455B14: ln 10
constexpr std::uint32_t unit=0x3f800000u;                         // 0x46A0D0: 1.0
constexpr std::uint32_t full_db=0u;                               // 0x46A0E8: 0.0 dB, the gain ceiling
constexpr float twenty=std::bit_cast<float>(0x41a00000u);         // 0x46A118: 20.0
constexpr float one_twentieth=std::bit_cast<float>(0x3d4ccccdu);  // 0x46A12C: 0.05
constexpr std::uint32_t shortest_fade=0x3a83126fu;                // 0x46A180: 0.001 (seconds)
// Retail globals.
constexpr std::uint32_t class_runtime=0x50ebf0u;    // 16-byte rows per sound class: +4 gain (dB), +0xC flags
constexpr std::uint32_t ducking_globals=0x4f2c50u;  // +0x1F8 active ducker type, +0x1FC its clock;
                                                    // +0x200 ducker type fading out, +0x204 its clock

// MOVSS from memory: the float in lane 0, upper lanes zero.
__m128 lane(std::uint32_t value){return _mm_set_ss(std::bit_cast<float>(value));}
std::uint32_t lane_bits(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
// COMISS x,y then JA: x > y, ordered.
bool above(__m128 x,__m128 y){return _mm_comigt_ss(x,y);}
// COMISS x,y then JB: x < y, or unordered (CF set).
bool below(__m128 x,__m128 y){return !_mm_comige_ss(x,y);}

// x87 instructions the original executes, under the caller's control word.
long double load_log2_e(){long double v;asm volatile("fldl2e":"=t"(v));return v;}
long double load_log10_2(){long double v;asm volatile("fldlg2":"=t"(v));return v;}
long double round_to_integer(long double x){long double v;asm volatile("frndint":"=t"(v):"0"(x));return v;}
long double exp2_minus_1(long double x){long double v;asm volatile("f2xm1":"=t"(v):"0"(x));return v;}
long double scale_by_power_of_2(long double x,long double exponent) {
    long double v;
    asm volatile("fldt %2\n\tfldt %1\n\tfscale\n\tfstpt %0\n\tfstp %%st(0)":"=m"(v):"m"(x),"m"(exponent):"st","st(1)");
    return v;
}
long double y_log2_x(long double y,long double x){long double v;asm volatile("fyl2x":"=t"(v):"0"(x),"u"(y):"st(1)");return v;}
// FLD x; FCOMIP ST(0),ST(1) against y; FSTP ST(0): x > y, ordered (any NaN signals).
bool extended_above(long double x,long double y) {
    unsigned char result;
    asm volatile("fldt %2\n\tfldt %1\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)"
        :"=qm"(result):"m"(x),"m"(y):"cc","st");
    return result!=0;
}
// An x87 m32 operand loaded as its own FLD before the arithmetic, as the
// reference routines do: a denormal or signalling single signals at the load
// whatever the other operand is (a fused FMUL m32 skips that against a NaN).
long double load_single(float x){volatile long double v=static_cast<long double>(x);return v;}
// The original's x87 loads need `slots` free registers. A caller stack that
// would overflow is refused exactly where the reference routine refuses it
// (no game caller has one).
void require_x87_room(const Abi &a,unsigned slots) {
    if(a.x87_depth+slots>8u)throw FormatError("Required original solver x87 stack-fault owner unavailable");
}

// 21F780 (lower, bound 0) / 21F7F0 (upper, bound 4): a sound's gain bound in
// dB, its sound class's bound (class row +0x20/+0x24) plus its scale's gain
// modifier bound (gestalt +0xC: 20-byte scale rows, +0/+4; definition +2
// class index, +0xB scale index). EDX: sound definition. Returns the float in
// EAX and XMM0; ECX and EDX keep the class and scale bounds.
std::uint32_t gain_bound(M &m,Abi &a,std::uint32_t definition,std::uint32_t bound) {
    const auto globals=u32(m,sound_globals);
    const auto reference=u32(m,globals+0xc0u)?u32(m,globals+0xc4u):0u;
    const auto data=tag_data(m,u32(m,reference+4u));
    const auto sound_class=s8(m,definition+2u);
    const auto classes=u32(m,data+4u);
    const auto class_bound=u32(m,classes+sound_class*0x5cu+0x20u+bound);
    const auto scale=s8(m,definition+0xbu);
    const auto scales=u32(m,u32(m,sound_gestalt)+0xcu);
    const auto scale_bound=u32(m,scales+scale*20u+bound);
    const auto gain=_mm_add_ss(lane(scale_bound),lane(class_bound));
    a.gp[0]=lane_bits(gain);a.gp[1]=class_bound;a.gp[2]=scale_bound;set_xmm(a,0,gain);
    return a.gp[0];
}
void lower_gain_bound(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);gain_bound(m,a,a.gp[2],0u);pop_return(m,a);
}
void upper_gain_bound(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);gain_bound(m,a,a.gp[2],4u);pop_return(m,a);
}

// 220000: a gain in dB as an amplitude, 10^(dB/20) with the gain clamped to
// [-64, 0], computed as 2^(dB * 0.05 * ln 10 * log2 e) through FRNDINT, F2XM1
// and FSCALE. Returns the amplitude for the x87 stack; EAX holds the clamped
// gain, XMM0 the gain and XMM1 the floor.
long double db_to_amplitude(Abi &a,std::uint32_t db) {
    const auto gain=lane(db),floor=lane(silence_db);
    set_xmm(a,0,gain);set_xmm(a,1,floor);
    auto clamped=db;
    if(above(floor,gain))clamped=silence_db;
    else if(above(gain,lane(full_db)))clamped=full_db;
    a.gp[0]=clamped;
    require_x87_room(a,3);
    volatile long double exponent=static_cast<long double>(std::bit_cast<float>(clamped));
    exponent=exponent*static_cast<long double>(one_twentieth);
    exponent=exponent*static_cast<long double>(ln_10);
    exponent=exponent*load_log2_e();
    const auto whole=round_to_integer(exponent);
    volatile long double power=exponent-whole;
    power=exp2_minus_1(power);
    power=power+1.0L;
    return scale_by_power_of_2(power,whole);
}
// Stack: the gain in dB (ret 4). Returns the amplitude in ST(0).
void db_to_amplitude_kernel(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);
    const auto amplitude=db_to_amplitude(a,argument(m,a,0));
    floating.push(amplitude);
    pop_return(m,a,4);
}

// 220200: an amplitude (XMM0) in dB, 20 log10 of the amplitude clamped to
// [0, 1] (FYL2X with log10 2), then clamped to [-64, 0]; amplitudes that are
// not positive are -64. Returns the float in EAX; ECX and XMM0 hold the gain
// before its final clamp, XMM1 zero, XMM2 the floor.
std::uint32_t amplitude_to_db(Abi &a,Floating &floating) {
    const auto amplitude=xmm(a,0),zero=_mm_setzero_ps(),floor=lane(silence_db);
    auto level=lane_bits(amplitude);
    if(above(zero,amplitude))level=0u;
    else if(above(amplitude,lane(unit)))level=unit;
    auto db=floor;
    if(above(amplitude,zero)) {
        require_x87_room(a,2);
        volatile long double x=static_cast<long double>(std::bit_cast<float>(level));
        volatile long double decibels=y_log2_x(load_log10_2(),x);
        decibels=decibels*static_cast<long double>(twenty);
        const auto rounded=floating.store_single(decibels);   // FST m32
        const bool silent=extended_above(static_cast<long double>(std::bit_cast<float>(silence_db)),decibels);
        floating.clear_c1();                                   // FSTP ST(0)
        if(!silent) {
            db=_mm_set_ss(rounded);
            if(above(db,zero))db=zero;
        }
    }
    a.gp[1]=lane_bits(db);
    a.gp[0]=above(floor,db)?silence_db:above(db,zero)?0u:lane_bits(db);
    set_xmm(a,0,db);set_xmm(a,1,zero);set_xmm(a,2,floor);
    return a.gp[0];
}
void amplitude_to_db_kernel(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);amplitude_to_db(a,floating);pop_return(m,a);
}

// 228220 (registered in native_tu15_sound_manager.cpp): the sound-class row
// for a sign-extended 16-bit class index.
std::uint32_t sound_class_row(const M &m,std::uint32_t index) {
    const auto globals=u32(m,sound_globals);
    const auto reference=u32(m,globals+0xc0u)?u32(m,globals+0xc4u):0u;
    const auto data=tag_data(m,u32(m,reference+4u));
    return index*0x5cu+u32(m,data+4u);
}
// (gain - 0) * t + 0: a ducking gain scaled by its progress, in the
// original's order (the zero is a stored local).
__m128 scaled_gain(std::uint32_t gain,__m128 t) {
    const auto zero=_mm_setzero_ps();
    return _mm_add_ss(_mm_mul_ss(_mm_sub_ss(lane(gain),zero),t),zero);
}

// 129B20: a sound class's gain in dB with ducking applied. CX: class index.
// The base gain is the class's runtime row +4; classes with runtime flag bit
// 0 are never ducked. Ducker types select the class's 16-byte ducking records
// at class row +0x28 (+0 gain in dB, +4 fade-in, +8 sustain, +0xC fade-out).
// A ducker restarted while it fades out ramps from its clocks' difference;
// otherwise the ducker fading out and the active one each add their gain
// scaled by their progress. Returns the float in EAX; ESI and EDI are kept.
std::uint32_t ducked_class_gain(M &m,Abi &a) {
    const auto index=std::uint32_t(std::int32_t(std::int16_t(a.gp[1])));
    const auto runtime=u32(m,class_runtime);
    a.gp[2]=runtime;
    const auto row=(index<<4)+runtime;
    const auto base=u32(m,row+4u);
    const auto flags=u8(m,row+0xcu);
    a.gp[0]=base;
    if(flags&1u)return base;
    const auto classes=sound_class_row(m,index);
    const auto ducking=u32(m,ducking_globals);
    a.gp[2]=ducking;
    const auto fading=u32(m,ducking+0x200u);
    a.gp[1]=fading;
    const auto record=[&](std::uint32_t type){return (type<<4)+classes+0x28u;};
    const auto zero=_mm_setzero_ps(),one=lane(unit),shortest=lane(shortest_fade);
    const auto finish=[&](__m128 gain){a.gp[0]=lane_bits(gain);return a.gp[0];};
    if(fading!=0xffffffffu && u32(m,ducking+0x1f8u)==fading) {
        // The ducker fading out was restarted.
        const auto clock=u32(m,ducking+0x1fcu),since=u32(m,ducking+0x204u);
        const auto elapsed=_mm_sub_ss(lane(clock),lane(since));
        set_xmm(a,3,elapsed);
        const auto duck=record(fading);
        const auto sustain=u32(m,duck+8u);
        if(!above(elapsed,lane(sustain))) {
            const auto gain=_mm_add_ss(lane(u32(m,duck)),lane(base));
            set_xmm(a,0,gain);return finish(gain);
        }
        const auto fade_in=lane(u32(m,duck+4u));
        set_xmm(a,0,shortest);set_xmm(a,1,fade_in);
        if(below(fade_in,shortest))return base;
        const auto fade_out=lane(u32(m,duck+0xcu));
        set_xmm(a,1,fade_out);
        bool ramping=false;
        if(!below(fade_out,shortest)) {
            const auto end=_mm_add_ss(fade_out,lane(sustain));
            set_xmm(a,0,end);
            ramping=above(end,elapsed);
        }
        if(ramping) {
            const auto reciprocal=_mm_div_ss(one,fade_in);
            const auto ramp=_mm_mul_ss(_mm_mul_ss(_mm_sub_ss(elapsed,lane(sustain)),reciprocal),fade_out);
            const auto position=_mm_add_ss(_mm_sub_ss(lane(clock),elapsed),ramp);
            const auto fraction=_mm_div_ss(position,fade_in);
            const auto t=above(one,_mm_mul_ss(position,reciprocal))?fraction:one;
            const auto gain_db=u32(m,duck);
            a.gp[1]=gain_db;
            const auto scaled=scaled_gain(gain_db,t);
            const auto gain=_mm_add_ss(lane(base),scaled);
            set_xmm(a,0,scaled);set_xmm(a,1,gain);set_xmm(a,2,zero);set_xmm(a,4,one);
            return finish(gain);
        }
        const auto progress=_mm_div_ss(_mm_sub_ss(lane(clock),elapsed),fade_in);
        const auto t=above(one,progress)?progress:one;
        const auto gain_db=u32(m,duck);
        a.gp[2]=gain_db;
        const auto gain=_mm_add_ss(scaled_gain(gain_db,t),lane(base));
        set_xmm(a,0,gain);set_xmm(a,1,t);set_xmm(a,2,zero);
        return finish(gain);
    }
    set_xmm(a,2,one);set_xmm(a,4,shortest);
    auto gain=base;
    if(fading!=0xffffffffu) {
        // The ducker fading out: its gain fades from full to none.
        const auto duck=record(fading);
        const auto fade_out=lane(u32(m,duck+0xcu));
        set_xmm(a,0,fade_out);
        if(!below(fade_out,shortest)) {
            const auto ratio=_mm_div_ss(lane(u32(m,duck+8u)),fade_out);
            const auto x=_mm_sub_ss(lane(u32(m,ducking+0x204u)),ratio);
            const auto t=above(zero,x)?zero:above(x,one)?one:x;
            const auto gain_db=lane(u32(m,duck));
            const auto ducked=_mm_add_ss(_mm_add_ss(_mm_mul_ss(_mm_sub_ss(zero,gain_db),t),gain_db),lane(gain));
            set_xmm(a,0,ducked);set_xmm(a,1,t);set_xmm(a,3,gain_db);
            gain=lane_bits(ducked);
        }
    }
    // The active ducker: its gain fades in.
    const auto active=u32(m,ducking+0x1f8u);
    a.gp[1]=active;a.gp[0]=gain;
    if(active==0xffffffffu)return gain;
    a.gp[1]=active<<4;
    const auto duck=record(active);
    const auto fade_in=lane(u32(m,duck+4u));
    set_xmm(a,0,fade_in);
    if(below(fade_in,shortest))return gain;
    const auto progress=_mm_div_ss(lane(u32(m,ducking+0x1fcu)),fade_in);
    const auto t=above(one,progress)?progress:one;
    const auto gain_db=u32(m,duck);
    a.gp[2]=gain_db;
    const auto ducked=_mm_add_ss(scaled_gain(gain_db,t),lane(gain));
    set_xmm(a,0,ducked);set_xmm(a,1,zero);set_xmm(a,2,t);
    return finish(ducked);
}
void ducked_class_gain_kernel(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);ducked_class_gain(m,a);pop_return(m,a);
}

// 127CF0: a sound's playback gain in dB. ESI: sound definition; stack: a gain
// offset in dB, then an interpolation fraction (ret 8). Interpolates between
// the amplitudes of the sound's lower and upper gain bounds, converts the
// result back to dB, and adds the class's ducked gain and the offset. Returns
// the float in EAX and XMM1; ECX holds the interpolated gain in dB, XMM0 the
// ducked gain plus the offset. EDX and XMM2-4 are left as the callees leave them.
void gain_interpolation(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);
    const auto definition=a.gp[6];
    const auto upper=gain_bound(m,a,definition,4u);
    const auto lower=gain_bound(m,a,definition,0u);
    const auto quiet=floating.store_single(db_to_amplitude(a,lower));   // FSTP m32
    const auto loud=db_to_amplitude(a,upper);
    volatile long double level=loud-load_single(quiet);                   // FSUB m32
    level=level*load_single(f32(m,a.gp[4]+8u));                          // FMUL m32, the fraction
    level=level+load_single(quiet);                                      // FADD m32
    set_xmm(a,0,_mm_set_ss(floating.store_single(level)));               // FSTP m32; MOVSS
    const auto db=amplitude_to_db(a,floating);
    a.gp[1]=(a.gp[1]&0xffff0000u)|(s8(m,definition+2u)&0xffffu);       // MOVSX CX, class index
    const auto class_gain=ducked_class_gain(m,a);
    const auto offset=_mm_add_ss(lane(class_gain),lane(u32(m,a.gp[4]+4u)));
    const auto gain=_mm_add_ss(lane(db),offset);
    set_xmm(a,0,offset);set_xmm(a,1,gain);
    a.gp[0]=lane_bits(gain);a.gp[1]=db;
    pop_return(m,a,8);
}

// Every register is reproduced, so each kernel owes the full default contract.
constexpr Kernel gain_kernels_table[]{
    {0x127cf0u,gain_interpolation},         // EAX gain (dB)
    {0x129b20u,ducked_class_gain_kernel},   // EAX ducked class gain (dB)
    {0x21f780u,lower_gain_bound},           // EAX lower gain bound (dB)
    {0x21f7f0u,upper_gain_bound},           // EAX upper gain bound (dB)
    {0x220000u,db_to_amplitude_kernel},     // ST(0) amplitude
    {0x220200u,amplitude_to_db_kernel},     // EAX gain (dB)
};
} // namespace
std::span<const Kernel> gain_kernels(){return gain_kernels_table;}
} // namespace halo2::native_tu15_sound
