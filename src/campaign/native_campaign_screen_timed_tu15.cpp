#include "halo2/native_campaign_screen_timed_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
class ScreenBank {
public:
    SourcePhysicsContactMemory &memory;
    const SourcePhysicsContactRecordObserver &observer;
    std::uint32_t base{};
    ScreenBank(SourcePhysicsContactMemory &m,const SourcePhysicsContactRecordObserver &o):memory(m),observer(o){}
    std::uint64_t absolute(std::uint32_t address,unsigned bytes)const {
        std::array<std::byte,8> data{};memory.read(address,std::span(data).first(bytes));
        std::uint64_t value{};
        for(unsigned i=0;i<bytes;++i)value|=std::uint64_t(std::to_integer<unsigned>(data[i]))<<(i*8u);
        return value;
    }
    bool bind(){base=std::uint32_t(absolute(0x515ec0,4));return base!=0;}
    std::uint32_t word(unsigned offset)const{return std::uint32_t(absolute(base+offset,4));}
    std::uint8_t flag(unsigned offset)const{return std::uint8_t(absolute(base+offset,1));}
    std::uint64_t clock()const{return absolute(0x4920c0,8);}
    std::int32_t views()const{return std::bit_cast<std::int32_t>(std::uint32_t(absolute(0x4c686c,4)));}
    void publish(std::uint32_t site,unsigned offset,std::uint32_t value,unsigned bytes=4) {
        SourcePhysicsContactRecordStore event{site,base+offset,{},std::vector<std::byte>(bytes)};
        if(observer){event.before.resize(bytes);memory.read(event.address,event.before);}
        for(unsigned i=0;i<bytes;++i)event.written[i]=std::byte(value>>(i*8u));
        memory.write(event.address,event.written);if(observer)observer(event);
    }
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// The algorithms use actual host scalar arithmetic. Hardware environment
// preservation is distinct from a virtual x87 register file or CPU executor.
class Environment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    unsigned host_mxcsr_{};
    NativeCampaignScreenTimedFpTu15 &fp_;
public:
    explicit Environment(NativeCampaignScreenTimedFpTu15 &fp):fp_(fp) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native screen timing requires masked original FP exceptions");
        host_mxcsr_=_mm_getcsr();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");
        _mm_setcsr(fp.mxcsr);
    }
    ~Environment() {
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
};
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
Scalar zero(){return _mm_setzero_ps();}
Scalar one(){return scalar(0x3f800000u);}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(Scalar a,Scalar b){unsigned char value;asm volatile("comiss %2,%1;seta %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value!=0;}
bool at_least(Scalar a,Scalar b){unsigned char value;asm volatile("comiss %2,%1;setae %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value!=0;}
bool ordered_equal(Scalar a,Scalar b){unsigned char equal,ordered;asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(b):"cc","memory");return equal&&ordered;}
Scalar saturate(Scalar value){if(above(zero(),value))return zero();if(above(value,one()))return one();return value;}
void load_single(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void load_clock(std::uint64_t value){asm volatile("fldl %0"::"m"(value):"memory","st");}
void add_single(std::uint32_t value){asm volatile("fadds %0"::"m"(value):"memory","st");}
void subtract_from_clock(std::uint64_t value){asm volatile("fsubrl %0"::"m"(value):"memory","st");}
void subtract_single(std::uint32_t value){asm volatile("fsubs %0"::"m"(value):"memory","st");}
std::uint32_t take_single(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
// Convert/store is one semantic publication. A rejected destination must not
// commit the hardware spill/pop; an observer rejecting a completed write does.
void publish_clock(ScreenBank &bank,std::uint32_t site,unsigned offset,bool pop) {
    SourcePhysicsContactRecordStore event{site,bank.base+offset,{},std::vector<std::byte>(4)};
    if(bank.observer){event.before.resize(4);bank.memory.read(event.address,event.before);}
    alignas(16)std::array<std::byte,108> prior{};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(prior)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::uint32_t value;
    if(pop)asm volatile("fstps %0":"=m"(value)::"memory","st");
    else asm volatile("fsts %0":"=m"(value)::"memory","st");
    for(unsigned i=0;i<4;++i)event.written[i]=std::byte(value>>(i*8u));
    try{bank.memory.write(event.address,event.written);}
    catch(...){asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
    if(bank.observer)bank.observer(event);
}
std::uint32_t clamped_elapsed(ScreenBank &bank,unsigned start,unsigned finish) {
    load_single(bank.word(start));
    subtract_from_clock(bank.clock());
    load_single(bank.word(finish));
    subtract_single(bank.word(start));
    asm volatile("fdivrp %%st,%%st(1)":::"memory","st","st(1)");
    const std::uint64_t lower=0,upper=0x3ff0000000000000ull;
    unsigned char below;
    asm volatile("fld %%st(0)\n\tfldl %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
        :"=qm"(below):"m"(lower):"cc","memory","st","st(1)","st(2)");
    if(below)asm volatile("fstp %%st(0)\n\tfldl %0"::"m"(lower):"memory","st");
    else {
        unsigned char over;
        asm volatile("fldl %1\n\tfld %%st(1)\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
            :"=qm"(over):"m"(upper):"cc","memory","st","st(1)","st(2)");
        if(over)asm volatile("fstp %%st(0)\n\tfldl %0"::"m"(upper):"memory","st");
    }
    return take_single();
}
Scalar elapsed(ScreenBank &bank,unsigned start,unsigned finish,Scalar fallback) {
    const auto end=scalar(bank.word(finish)),begin=scalar(bank.word(start));
    return ordered_equal(end,begin)?fallback:scalar(clamped_elapsed(bank,start,finish));
}
bool clock_finished(ScreenBank &bank,unsigned finish) {
    load_single(bank.word(finish));const auto now=bank.clock();unsigned char finished;
    asm volatile("fldl %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tsetae %0"
        :"=qm"(finished):"m"(now):"cc","memory","st","st(1)");
    return finished!=0;
}
void update_opacity_and_color(ScreenBank &bank) {
    const auto opacity_time=elapsed(bank,0xb8,0xbc,one());
    const auto color_time=elapsed(bank,0xd0,0xd4,one());
    const auto opacity_end=mul(scalar(bank.word(0xb4)),opacity_time);
    const auto opacity_complement=sub(one(),opacity_time);
    const auto opacity_start=mul(opacity_complement,scalar(bank.word(0xb0)));
    bank.publish(0x1fe28,4,bits(add(opacity_start,opacity_end)));
    const auto color_end=scalar(bank.word(0xc4)),color_start=scalar(bank.word(0xc0));
    const auto complement=sub(one(),color_time);
    const auto end=mul(color_end,color_time),begin=mul(complement,color_start);
    bank.publish(0x1fe68,0x10,bits(saturate(add(begin,end))));
    const auto second_end=scalar(bank.word(0xcc)),second_start=scalar(bank.word(0xc8));
    const auto second_begin=mul(complement,second_start),second_finish=mul(second_end,color_time);
    const auto second=saturate(add(second_begin,second_finish));
    const auto zero_vector=std::uint32_t(bank.absolute(0x474c78,4));
    bank.publish(0x1feb0,0x14,bits(second));
    // Raw vector comparison stops at the first mismatch. Pointer slots are
    // retained storage: load the first before publication14, and obtain the
    // replacement only after a complete match. Copy each word lazily so
    // legitimate aliasing keeps the original publication order.
    bool default_vector=true;
    for(unsigned component=0;component<3;++component) {
        const auto current=bank.word(0x18+component*4u);
        const auto expected=std::uint32_t(bank.absolute(zero_vector+component*4u,4));
        if(current!=expected){default_vector=false;break;}
    }
    if(default_vector){
        const auto replacement=std::uint32_t(bank.absolute(0x474c80,4));
        constexpr std::array sites{0x1fec1u,0x1fec6u,0x1feccu};
        for(unsigned component=0;component<3;++component) {
            const auto value=std::uint32_t(bank.absolute(replacement+component*4u,4));
            bank.publish(sites[component],0x18+component*4u,value);
        }
    }
    const auto epsilon=scalar(0x38d1b717u);
    const bool transparent=at_least(epsilon,scalar(bank.word(4)));
    if(bank.views()>1 || transparent){bank.publish(0x1fef3,4,0);bank.publish(0x1fef8,2,0,2);bank.publish(0x1fefe,0,0,2);}
    if(at_least(epsilon,scalar(bank.word(0x10))) && at_least(epsilon,scalar(bank.word(0x14))) && at_least(color_time,one())) {
        bank.publish(0x1ff14,0x10,0);bank.publish(0x1ff19,0x14,0);
    }
}
void update_dof(ScreenBank &bank) {
    bool no_ranges=true;
    for(const auto offset:{0xdcu,0xe0u,0xecu,0xf0u})if(above(scalar(bank.word(offset)),zero())){no_ranges=false;break;}
    if(!no_ranges) {
        const auto far_time=elapsed(bank,0xe4,0xe8,one()),near_time=elapsed(bank,0xf4,0xf8,one());
        const auto far_end=mul(scalar(bank.word(0xe0)),far_time);
        const auto far_complement=sub(one(),far_time);
        const auto far_begin=mul(far_complement,scalar(bank.word(0xdc)));
        const auto far=add(far_begin,far_end);const bool far_positive=above(far,zero());
        const auto near_end=scalar(bank.word(0xf0));
        const auto near_complement=sub(one(),near_time);
        const auto near_begin=mul(near_complement,scalar(bank.word(0xec)));
        const auto near=add(near_begin,mul(near_end,near_time));
        if(far_positive || above(near,zero())) {
            const auto focus=bank.word(0xd8);bank.publish(0x20079,0x3c,1,1);
            bank.publish(0x2007d,0x40,focus);bank.publish(0x20080,0x44,bits(far));bank.publish(0x20085,0x48,bits(near));
        }
    }
    bool completed=false;
    if(ordered_equal(scalar(bank.word(0xe0)),zero()) && ordered_equal(scalar(bank.word(0xf0)),zero()) &&
       clock_finished(bank,0xe8) && clock_finished(bank,0xf8))completed=true;
    if(completed || no_ranges) {
        bank.publish(0x200d4,0x3c,0,1);
        constexpr std::array fields{0x40u,0x44u,0x48u,0xd8u,0xdcu,0xe0u,0xe4u,0xe8u,0xecu,0xf0u,0xf4u,0xf8u};
        constexpr std::array sites{0x200d8u,0x200ddu,0x200e2u,0x200e7u,0x200efu,0x200f7u,0x200ffu,0x20107u,0x2010fu,0x20117u,0x2011fu,0x20127u};
        for(unsigned i=0;i<fields.size();++i)bank.publish(sites[i],fields[i],0);
    }
}
void update_effects(ScreenBank &bank) {
    const auto time=elapsed(bank,0x124,0x128,zero());Scalar complement=one();
    constexpr std::array sites{0x201bau,0x201dbu,0x201fcu,0x2021du,0x2023eu};
    for(unsigned channel=0;channel<5;++channel) {
        const auto from=scalar(bank.word(0xfc+channel*8u));Scalar begin,end;
        if(channel==0){const auto finish=scalar(bank.word(0x100));end=mul(finish,time);complement=sub(one(),time);begin=mul(from,complement);}
        else if(channel==4){begin=mul(from,complement);end=mul(scalar(bank.word(0x120)),time);}
        else {const auto finish=scalar(bank.word(0x100+channel*8u));begin=mul(from,complement);end=mul(finish,time);}
        bank.publish(sites[channel],0x4c+channel*4u,bits(add(begin,end)));
    }
    constexpr std::array<std::array<std::uint32_t,3>,4> writes{{{0x2025eu,0x2026fu,0x20280u},
        {0x202b1u,0x202c2u,0x202d3u},{0x20304u,0x20315u,0x20326u},{0x2035du,0x20371u,0x20385u}}};
    constexpr std::array<std::array<std::uint32_t,3>,4> copies{{{0x20287u,0x2028cu,0x20291u},
        {0x202dau,0x202dfu,0x202e4u},{0x20330u,0x20335u,0x2033au},{0x2038fu,0x20397u,0x2039fu}}};
    for(unsigned channel=0;channel<4;++channel) {
        const bool colored=bank.flag(0x15c+channel)!=0;
        const auto amount=scalar(bank.word(0x50+channel*4u));
        for(unsigned color=0;color<3;++color) {
            const auto output=0x60+channel*12u+color*4u;
            if(colored)bank.publish(writes[channel][color],output,bits(mul(scalar(bank.word(0x12c+channel*12u+color*4u)),amount)));
            else bank.publish(copies[channel][color],output,bits(amount));
        }
    }
}
void update_crossfade(ScreenBank &bank) {
    const auto first=bank.word(0x164),second=bank.word(0x160);
    const auto end=scalar(bank.word(0x17c)),start=scalar(bank.word(0x178));
    const bool same=ordered_equal(end,start);
    bank.publish(0x203c2,0x94,first);
    const auto third=bank.word(0x16c);bank.publish(0x203ce,0x9c,third);
    bank.publish(0x203d8,0x90,second);const auto fourth=bank.word(0x168);bank.publish(0x203e4,0x98,fourth);
    auto alpha=zero();
    if(!same) {
        alpha=sub(one(),scalar(clamped_elapsed(bank,0x178,0x17c)));
        if(above(zero(),alpha))alpha=zero();
        else {
            bool positive;
            if(above(alpha,one())){alpha=one();positive=true;}
            else positive=above(alpha,zero());
            if(positive && bank.flag(0x170)){bank.publish(0x20469,0xa0,1,1);bank.publish(0x20470,0x170,0,1);}
        }
    }
    const auto strength=bank.word(0x174);bank.publish(0x2047d,0xa4,bits(alpha));bank.publish(0x20485,0xa8,strength);
}
#endif
}

void native_campaign_screen_dof_setup_tu15(SourcePhysicsContactMemory &memory,std::uint32_t focus,
    const NativeCampaignScreenDofArgumentsTu15 &arguments,NativeCampaignScreenTimedFpTu15 &fp,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp);ScreenBank bank(memory,observe);if(!bank.bind())return;
    auto focus_value=scalar(focus);if(above(zero(),focus_value))focus_value=zero();
    bank.publish(0x1fc28,0xd8,bits(focus_value));
    for(unsigned range=0;range<2;++range) {
        const auto first=range*3u;const auto duration=scalar(arguments.word(first+2u));
        if(at_least(duration,zero())) {
            load_clock(bank.clock());const auto start=arguments.word(first);
            publish_clock(bank,range?0x1fc84u:0x1fc47u,range?0xf4u:0xe4u,false);
            bank.publish(range?0x1fc8au:0x1fc4du,range?0xecu:0xdcu,start);
            add_single(arguments.word(first+2u));const auto finish=arguments.word(first+1u);
            bank.publish(range?0x1fc9cu:0x1fc5fu,range?0xf0u:0xe0u,finish);
            publish_clock(bank,range?0x1fca4u:0x1fc67u,range?0xf8u:0xe8u,true);
        }
    }
#else
    (void)memory;(void)focus;(void)arguments;(void)fp;(void)observe;
    throw std::runtime_error("Native timed screen x87/SSE backend unavailable");
#endif
}
std::optional<std::uint32_t> native_campaign_screen_timed_update_tu15(SourcePhysicsContactMemory &memory,
    NativeCampaignScreenTimedFpTu15 &fp,const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp);ScreenBank bank(memory,observe);if(!bank.bind() || !bank.flag(0xac))return {};
    update_opacity_and_color(bank);update_dof(bank);update_effects(bank);update_crossfade(bank);return bank.base;
#else
    (void)memory;(void)fp;(void)observe;throw std::runtime_error("Native timed screen x87/SSE backend unavailable");
#endif
}
}
