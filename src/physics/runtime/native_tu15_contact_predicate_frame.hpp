#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_contact_predicate_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_integer_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <emmintrin.h>

namespace halo2::native_contact_predicate_detail {
struct Noop { template<class State> void operator()(State&,std::uint32_t) const {} };
template<class Scope> struct Io {
    Scope* scope;
    template<class Callback> void operator()(Callback&& callback) const {
        scope->io(std::forward<Callback>(callback));
    }
};

template<class State, class Before> class Frame :
    public native_mopp_ray_caller_detail::IntegerFrame<State, Before, Io<native_raw_x87_detail::Scope<State>>> {
    using Scope = native_raw_x87_detail::Scope<State>;
    using Base = native_mopp_ray_caller_detail::IntegerFrame<State, Before, Io<Scope>>;
    using Base::s_; using Base::at; using Base::word; using Base::push; using Base::pop;
    using Base::vector; using Base::store; using Base::io;
    Scope& scope_;

    void acquire_float(std::uint32_t pc, std::uint32_t address) {
        at(pc);
        const auto bits = word(address);
        scope_.require_capacity(1);
        asm volatile("flds %0" :: "m"(bits) : "memory", "st");
    }
    std::uint16_t compare(std::uint32_t pc, std::uint32_t address, unsigned mask) {
        at(pc);
        const auto bits = word(address);
        std::uint16_t status;
        asm volatile("fcomps %1\n\tfnstsw %0" : "=a"(status) : "m"(bits) : "memory", "st");
        s_.gp[0] = (s_.gp[0] & 0xffff0000u) | status;
        s_.flags.logic((status >> 8u) & mask, 1);
        return status;
    }
    static std::uint32_t low_bits(__m128 value) {
        return static_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));
    }
    void output_byte(std::uint32_t pc, std::uint32_t address, bool result) {
        at(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if (s_.observe) {
            event.before.resize(1);
            io([&]{s_.memory.read(address,event.before);});
        }
        const std::array written{std::byte(result ? 1u : 0u)};
        io([&]{s_.memory.write(address,written);});
        if (s_.observe) {
            event.written.assign(written.begin(),written.end());
            io([&]{s_.observe(event);});
        }
    }
public:
    Frame(State& s, Before& before, Scope& scope) : Base{s,before,{&scope}}, scope_(scope) {}
    std::uint16_t distance_limit() {
        auto& g=s_.gp;
        push(0x303800u,g[5]);
        g[5]=g[4]; g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s_.flags.minus(g[4],12u,4);
        at(0x303809u); g[1]=word(g[5]+16u);
        acquire_float(0x30380cu,g[1]+12u);
        push(0x30380fu,g[6]);
        return compare(0x303810u,0x41d018u,0x05u);
    }
    Pair normals() {
        at(0x303821u); s_.gp[2]=word(s_.gp[5]+12u);
        s_.xmm[1]=vector(0x303824u,s_.gp[2]+16u);
        s_.xmm[2]=vector(0x303828u,s_.gp[1]+16u);
        at(0x30382cu); s_.gp[6]=word(s_.gp[5]+20u);
        return {s_.xmm[1],s_.xmm[2]};
    }
    std::uint16_t normal_threshold(const Dot& agreement) {
        s_.xmm[0]=agreement.product; s_.xmm[2]=agreement.y_sum; s_.xmm[3]=agreement.z_sum;
        s_.gp[0]=s_.gp[4]+12u;
        store(0x30384fu,s_.gp[0],low_bits(s_.xmm[3]),[]{});
        acquire_float(0x303853u,s_.gp[4]+12u);
        return compare(0x303857u,s_.gp[6]+16u,0x05u);
    }
    Pair positions() {
        s_.xmm[0]=vector(0x303861u,s_.gp[2]);
        s_.xmm[2]=vector(0x303864u,s_.gp[1]);
        return {s_.xmm[0],s_.xmm[2]};
    }
    std::uint16_t separation_threshold(const Dot& separation) {
        s_.xmm[0]=separation.product; s_.xmm[1]=separation.y_sum; s_.xmm[2]=separation.z_sum;
        s_.gp[1]=s_.gp[4]+12u;
        store(0x30388au,s_.gp[1],low_bits(s_.xmm[2]),[]{});
        acquire_float(0x30388eu,s_.gp[4]+12u);
        return compare(0x303892u,s_.gp[6]+12u,0x41u);
    }
    void finish(bool rejected) {
        at(rejected ? 0x3038a9u : 0x30389cu); s_.gp[0]=word(s_.gp[5]+8u);
        output_byte(rejected ? 0x3038acu : 0x30389fu,s_.gp[0],rejected);
        s_.gp[6]=pop(rejected ? 0x3038afu : 0x3038a2u);
        s_.gp[4]=s_.gp[5]; s_.gp[5]=pop(rejected ? 0x3038b2u : 0x3038a5u);
        pop(rejected ? 0x3038b3u : 0x3038a6u); s_.gp[4]+=16u;
    }
};

// Private original architectural transport. Pure-op Before, unsupported FP
// and the retained source's split/vector/provider semantics are separate.
template<class State, class Before=Noop> void run_private(State& s, Before before={}) {
    native_raw_x87_detail::Scope scope{s};
    Frame view{s,before,scope};
    const bool rejected=reject(view);
    view.finish(rejected);
}
} // namespace halo2::native_contact_predicate_detail
#endif
