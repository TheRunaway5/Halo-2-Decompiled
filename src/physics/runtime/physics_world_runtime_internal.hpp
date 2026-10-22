#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2::physics_world_runtime_detail {
using namespace physics_contact_runtime_detail;
struct WorldState : physics_contact_runtime_detail::State {
    std::optional<SourcePhysicsMmxAlias> active_mmx;
    WorldState(M &m,const O &o,const SourcePhysicsWorldRuntimeServices &s,
        SourcePhysicsContactImage image)
        : physics_contact_runtime_detail::State{m,o,s,image} {}
    void load(const SourcePhysicsContactRuntimeAbi &a) {
        if(a.active_mmx&&(a.x87_depth||(a.x87_status&0x3800u)))
            throw std::runtime_error("Invalid original active MMX register-file state");
        auto scalar=a;scalar.active_mmx.reset();
        physics_contact_runtime_detail::State::load(scalar);active_mmx=a.active_mmx;
    }
    SourcePhysicsContactRuntimeAbi save()const {
        auto a=physics_contact_runtime_detail::State::save();a.active_mmx=active_mmx;return a;
    }
    void begin_mmx() {
        if(active_mmx)return;
        SourcePhysicsMmxAlias alias{};
        for(unsigned i=0;i<fp.count;++i) {
            const auto physical=(fp.top+i)&7u;
            std::memcpy(alias.physical[physical].data(),&fp.lanes[i],10);
            alias.known_mask|=std::uint8_t(1u<<physical);
        }
        active_mmx=alias;fp.count=0;fp.top=0;
    }
    std::uint64_t mmx(unsigned index) {
        begin_mmx();
        if(!(active_mmx->known_mask&(1u<<index)))
            throw std::runtime_error("Required original dead physical MMX register owner unavailable");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i)value|=std::uint64_t(std::to_integer<unsigned>(active_mmx->physical[index][i]))<<(8u*i);
        return value;
    }
    void set_mmx(unsigned index,std::uint64_t value) {
        begin_mmx();auto &raw=active_mmx->physical[index];
        for(unsigned i=0;i<8;++i)raw[i]=std::byte(value>>(8u*i));
        raw[8]=raw[9]=std::byte{255};active_mmx->known_mask|=std::uint8_t(1u<<index);
    }
    void emms() {active_mmx.reset();fp.count=0;}
    void require_x87()const {
        if(active_mmx)throw std::runtime_error("Required original mixed MMX/x87 operation owner unavailable");
    }
    void required(std::uint32_t site,std::uint32_t target,bool tail) {
        if(active_mmx)throw std::runtime_error("Required original active MMX callee owner unavailable");
        physics_contact_runtime_detail::State::required(site,target,tail);
    }
    void compare_and_pop(long double other) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto value=fp.st(0);unsigned char carry,zero,parity;
        // Retain the original extended operands through FCOMIP. Converting to
        // a scalar SSE value here would discard source comparison precision.
        asm volatile("fldt %4\n\tfldt %3\n\tfcomip %%st(1),%%st\n\tsetb %0\n\tsete %1\n\tsetp %2\n\tfstp %%st(0)"
            :"=qm"(carry),"=qm"(zero),"=qm"(parity):"m"(value),"m"(other):"cc","st");
        flags.cf=carry;flags.zf=zero;flags.pf=parity;flags.af=flags.sf=flags.of=false;fp.pop();
#else
        throw std::runtime_error("Original world extended comparison backend unavailable");
#endif
    }
    void quotient(std::uint32_t denominator,bool signed_division) {
        const auto numerator=(std::uint64_t(gp[2])<<32u)|gp[0];
        if(!denominator)throw std::runtime_error("Required original world integer division-fault owner unavailable");
        if(signed_division) {
            const auto a=std::bit_cast<std::int64_t>(numerator),b=std::int64_t(si(denominator));
            if(a==std::numeric_limits<std::int64_t>::min()&&b==-1)
                throw std::runtime_error("Required original world integer division-fault owner unavailable");
            const auto q=a/b;if(q<std::numeric_limits<std::int32_t>::min()||q>std::numeric_limits<std::int32_t>::max())
                throw std::runtime_error("Required original world integer division-fault owner unavailable");
            gp[0]=std::uint32_t(q);gp[2]=std::uint32_t(a%b);
        } else {
            const auto q=numerator/denominator;if(q>0xffffffffu)
                throw std::runtime_error("Required original world integer division-fault owner unavailable");
            gp[0]=std::uint32_t(q);gp[2]=std::uint32_t(numerator%denominator);
        }
    }
};
} // namespace halo2::physics_world_runtime_detail
