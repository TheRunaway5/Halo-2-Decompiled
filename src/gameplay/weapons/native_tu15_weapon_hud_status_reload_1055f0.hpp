#pragma once

#include "gameplay/weapons/native_tu15_weapon_hud_status_1055f0.hpp"

namespace halo2::native_weapon_hud_status_detail {
constexpr std::uint32_t final_status = 0x1058e6u;

template<class State>
void spill_reload_duration(State &s) {
    using namespace physics_contact_runtime_detail;
    const auto initial = source_x87_exception_flags();
    auto status = s.fp.status;
    const auto value = integer_spill(s.fp.st(0), status);
    const auto result = source_x87_exception_flags();
    if (result != initial)
        source_load_x87_exception_flags(initial);
    native_two_bone_solve::store_and_commit(s, 0x1058b2u, s.gp[esp] + 0x20u, value, [&] {
        if (result != initial)
            source_load_x87_exception_flags(source_x87_exception_flags() | result);
        s.fp.status = status;
        s.fp.pop();
    });
}

template<class State>
class ReloadFrame : public StatusFrame<State> {
    using Base = StatusFrame<State>;
    using Base::s;
    using Base::gp;
    using Base::flags;
    using Base::at;
    using Base::recheck;
    using Base::read;
    using Base::load;
    using Base::store;
    using Base::push;
    using Base::call;
    using Base::load_signed_halfword;

    static std::uint32_t sign_byte(std::uint32_t value) {
        return std::uint32_t(std::bit_cast<std::int8_t>(std::uint8_t(value)));
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc, address))));
    }

    std::uint32_t scalar_bits(unsigned lane) const {
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(s.xmm[lane])));
    }

    template<class Target>
    void query_target(Target target) {
        load(0x1057abu, 0x1057b2u, eax, gp[esp] + 0x98u);
        gp[ecx] = gp[esp] + 0x13u;
        push(0x1057b6u, 0x1057b7u, gp[ecx]);
        gp[edx] = gp[esp] + 0x20u;
        push(0x1057bbu, 0x1057bcu, gp[edx]);
        call(0x1057bcu, 0x1057c1u, target);
        flags.logic(s.reg(eax, 1), 1);
        if (!flags.zf)
            store(0x1057c9u, 0x1057cdu, gp[ebp] + 0x11u, 1, 1);
    }

    template<class Object, class Matrix>
    void query_attachment(Object object, Matrix matrix) {
        load(0x1057deu, 0x1057e4u, edi, gp[ebx] + 0x194u);
        flags.minus(gp[edi], 0xffffffffu, 4);
        if (flags.zf)
            return;
        push(0x1057edu, 0x1057efu, 0xffffffffu);
        gp[eax] = gp[edi];
        call(0x1057f1u, 0x1057f6u, object);
        flags.logic(gp[eax], 4);
        if (flags.zf)
            return;
        load(0x1057feu, 0x105804u, ecx, gp[ebx] + 0x177u, 1, 1);
        flags.minus(s.reg(ecx, 1), 0xffu, 1);
        if (flags.zf)
            return;
        load(0x10580du, 0x10580fu, eax, gp[eax]);
        load(0x10580fu, 0x105815u, edx, 0x4f0414u);
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[eax] = flags.shift(gp[eax], 4, 4);
        load(0x10581du, 0x105821u, eax, gp[eax] + gp[edx] + 8u);
        load(0x105821u, 0x105824u, eax, gp[eax] + 0x38u);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf)
            return;
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[eax] = flags.shift(gp[eax], 4, 4);
        flags.logic(s.reg(ecx, 1), 1);
        load(0x105833u, 0x105837u, eax, gp[eax] + gp[edx] + 8u);
        if (flags.sf != flags.of)
            return;
        load(0x105839u, 0x10583cu, edx, gp[eax] + 0x68u);
        gp[ecx] = sign_byte(gp[ecx]);
        flags.minus(gp[ecx], gp[edx], 4);
        if (flags.sf == flags.of)
            return;
        load(0x105843u, 0x105846u, edx, gp[eax] + 0x6cu);
        gp[ecx] = flags.product(gp[ecx], 0x1cu);
        load(0x105849u, 0x10584cu, ecx, gp[ecx] + gp[edx]);
        push(0x10584cu, 0x10584eu, 1);
        push(0x10584eu, 0x10584fu, gp[ecx]);
        push(0x10584fu, 0x105850u, gp[edi]);
        s.set(ecx, flags.logic(0, 1), 1);
        gp[ebx] = gp[esp] + 0x30u;
        call(0x105856u, 0x10585bu, matrix);
        flags.minus(s.reg(eax, 2), 1, 2);
        load(0x10585fu, 0x105863u, esi, gp[esp] + 0x14u);
        load(0x105863u, 0x105867u, ebx, gp[esp] + 0x18u);
        if (!flags.zf)
            return;
        load(0x105869u, 0x105870u, ecx, gp[esp] + 0x84u);
        load(0x105870u, 0x105877u, edx, gp[esp] + 0x88u);
        gp[eax] = gp[ebp] + 0x18u;
        store(0x10587au, 0x10587cu, gp[eax], gp[ecx]);
        load(0x10587cu, 0x105883u, ecx, gp[esp] + 0x8cu);
        store(0x105883u, 0x105886u, gp[eax] + 4u, gp[edx]);
        store(0x105886u, 0x105889u, gp[eax] + 8u, gp[ecx]);
    }

    void publish_reload() {
        flags.minus(read(0x105889u, gp[ebx] + 0x20cu, 1), 5, 1);
        recheck(0x105890u);
        if (!flags.zf) {
            store(0x1058e2u, final_status, gp[ebp] + 0x13u, 1, 1);
            return;
        }
        load(0x105892u, 0x105898u, edx, 0x51df54u);
        load_signed_halfword(0x105898u, 0x10589cu, eax, gp[edx] + 2u);
        s.xmm[0] = _mm_cvtsi32_ss(s.xmm[0], std::bit_cast<std::int32_t>(gp[eax]));
        s.xmm[0] = native_tu15_leaves::ordered_mul_ss(s.xmm[0], scalar(0x1058a0u, 0x46a0ccu));
        recheck(0x1058a8u);
        store(0x1058a8u, 0x1058aeu, gp[esp] + 0x1cu, scalar_bits(0));
        s.fp.push(native_live_support::x87::load_single(read(0x1058aeu, gp[esp] + 0x1cu)));
        s.fp.status &= ~0x200u;
        recheck(0x1058b2u);
        at(0x1058b2u);
        spill_reload_duration(s);
        recheck(0x1058b6u);
        s.xmm[1] = _mm_cvtsi32_ss(s.xmm[1], std::bit_cast<std::int32_t>(read(0x1058b6u, gp[esp] + 0x20u)));
        recheck(0x1058bcu);
        store(0x1058bcu, 0x1058c0u, gp[ebp] + 0x12u, 1, 1);
        load_signed_halfword(0x1058c0u, 0x1058c7u, ecx, gp[ebx] + 0x20eu);
        s.xmm[0] = _mm_cvtsi32_ss(s.xmm[0], std::bit_cast<std::int32_t>(gp[ecx]));
        s.xmm[0] = _mm_div_ss(s.xmm[0], s.xmm[1]);
        s.xmm[1] = scalar(0x1058cfu, 0x46a0d0u);
        recheck(0x1058d7u);
        s.xmm[1] = _mm_sub_ss(s.xmm[1], s.xmm[0]);
        store(0x1058dbu, 0x1058e0u, gp[ebp] + 0x14u, scalar_bits(1));
    }

public:
    using Base::Base;

    template<class Target, class Object, class Matrix>
    std::uint32_t reload(Target target, Object object, Matrix matrix) {
        if (s.active_mmx || s.fp.count >= 8)
            return 0;
        load(0x105774u, 0x10577au, eax, gp[edx] + 0x2c8u);
        flags.logic(gp[eax], 4);
        if (flags.zf || flags.sf != flags.of)
            return final_status;
        load(0x105782u, 0x105788u, eax, gp[edx] + 0x2ccu);
        flags.minus(read(0x105788u, gp[eax] + 6u, 2), 5, 2);
        recheck(0x10578du);
        if (!flags.zf)
            return final_status;
        flags.minus(read(0x105793u, gp[ebx] + 0x22cu, 2), 0, 2);
        recheck(0x10579bu);
        if (flags.zf || flags.sf != flags.of)
            return final_status;
        load(0x1057a1u, 0x1057a7u, eax, gp[ebx] + 0x20cu, 1, 1);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.zf) {
            query_target(target);
        } else {
            flags.minus(s.reg(eax, 1), 5, 1);
            if (!flags.zf)
                flags.minus(s.reg(eax, 1), 6, 1);
            if (flags.zf) {
                query_attachment(object, matrix);
                publish_reload();
            }
        }
        return final_status;
    }
};
} // namespace halo2::native_weapon_hud_status_detail
