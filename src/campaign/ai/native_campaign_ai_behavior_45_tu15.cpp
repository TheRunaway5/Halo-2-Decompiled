#include "halo2/native_campaign_ai_behavior_45_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2 {

bool native_campaign_ai_behavior_select_45_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    (void)observe;
    constexpr std::uint32_t entry = 0x1b1fa0u, descriptor = 0x48a1f8u;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1afce4u ||
        call.target != entry || call.tail || eax != entry || ebx != descriptor)
        return false;

    using physics_step_detail::r;
    physics_contact_runtime_detail::Flags flags;
    flags.load(abi.eflags);
    const auto at = [&](std::uint32_t pc) {
        if (services.instruction) services.instruction(pc);
    };
    const auto publish_flags = [&] { abi.eflags = (abi.eflags & ~0x8d5u) | flags.bits(); };

    at(entry); ecx = r(memory, esp + 4u);
    at(0x1b1fa4u); eax = r(memory, 0x5020c8u);
    at(0x1b1fa9u); edx = r(memory, eax + 0x44u);
    at(0x1b1facu); eax = r(memory, 0x47c1a8u);
    at(0x1b1fb1u); ecx = flags.logic(ecx & 0xffffu, 4); publish_flags();
    at(0x1b1fb7u); ecx = flags.product(ecx, 0x888u); publish_flags();
    at(0x1b1fbdu); ecx = flags.plus(ecx, edx, 4); publish_flags();
    at(0x1b1fbfu); flags.minus(r(memory, ecx + 0x358u, 2), 0u, 2); publish_flags();
    at(0x1b1fc7u);
    bool cancel = flags.zf;
    if (!cancel) {
        at(0x1b1fc9u); edx = (edx & ~0xffu) | r(memory, ecx + 0x35eu, 1);
        at(0x1b1fcfu); flags.logic(edx & 0xffu, 1); publish_flags();
        at(0x1b1fd1u);
        cancel = !flags.zf;
    }
    if (cancel) {
        at(0x1b1fd3u); eax = (eax & ~0xffffu) | r(memory, 0x47c1a4u, 2);
    }
    at(0x1b1fd9u);
    if (r(memory, esp) != 0x1afce6u)
        throw std::runtime_error("Native behavior select 1B1FA0 has a different actual return word");
    esp += 16u;
    return true;
}

bool native_campaign_ai_behavior_perform_45_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    constexpr std::uint32_t entry = 0x1b1fe0u, descriptor = 0x48a1f8u;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1b0ad9u ||
        call.target != entry || call.tail || eax != entry || esi != descriptor)
        return false;

    using physics_step_detail::r;
    using physics_step_detail::w;
    physics_contact_runtime_detail::Flags flags;
    flags.load(abi.eflags);
    const auto at = [&](std::uint32_t pc) {
        if (services.instruction) services.instruction(pc);
    };
    const auto publish_flags = [&] { abi.eflags = (abi.eflags & ~0x8d5u) | flags.bits(); };
    const auto push = [&](std::uint32_t pc, std::uint32_t value) {
        at(pc);
        const auto target = esp - 4u;
        if (observe) {
            const SourcePhysicsContactRecordObserver completed = [&](const auto &store) {
                esp = target;
                observe(store);
            };
            w(memory, completed, pc, target, 4, value);
        } else {
            w(memory, observe, pc, target, 4, value);
            esp = target;
        }
    };
    const auto pop = [&](std::uint32_t pc) {
        at(pc);
        const auto value = r(memory, esp);
        esp += 4u;
        return value;
    };
    const auto call_child = [&](std::uint32_t site, std::uint32_t ret_pc, std::uint32_t target,
                                std::uint32_t expected_sp, const char *missing_msg,
                                const char *unbalanced_msg) {
        push(site, ret_pc);
        if (!services.required_owner)
            throw std::runtime_error(missing_msg);
        services.required_owner(memory, {site, target, false}, abi, observe);
        if (esp != expected_sp)
            throw std::runtime_error(unbalanced_msg);
        flags.load(abi.eflags);
    };

    push(entry, ebp);
    at(0x1b1fe1u); ebp = esp;
    at(0x1b1fe3u); esp = flags.logic(esp & 0xfffffff8u, 4); publish_flags();
    at(0x1b1fe6u); esp = flags.minus(esp, 0x764u, 4); publish_flags();
    at(0x1b1fecu); ecx = r(memory, 0x5020c8u);
    at(0x1b1ff2u); edx = r(memory, ecx + 0x44u);
    push(0x1b1ff5u, ebx);
    at(0x1b1ff6u); ebx = r(memory, ebp + 8u);
    at(0x1b1ff9u); eax = ebx;
    at(0x1b1ffbu); eax = flags.logic(eax & 0xffffu, 4); publish_flags();
    at(0x1b2000u); eax = flags.product(eax, 0x888u); publish_flags();
    at(0x1b2006u); ecx = (ecx & ~0xffu) | r(memory, eax + edx + 0x40u, 1);
    at(0x1b200au); eax = flags.plus(eax, edx, 4); publish_flags();
    at(0x1b200cu); flags.logic(ecx & 0xffu, 1); publish_flags();
    push(0x1b200eu, esi);
    push(0x1b200fu, edi);
    at(0x1b2010u);
    if (!flags.zf) {
        at(0x1b2012u); flags.minus(r(memory, eax + 0x358u, 2), 0u, 2); publish_flags();
        at(0x1b201au);
        if (!flags.zf) {
            call_child(0x1b201cu, 0x1b2021u, 0x1d2d90u, esp,
                "Native behavior perform 1B1FE0 requires its actual 1D2D90 child",
                "Native behavior perform 1B1FE0 child 1D2D90 returned an unbalanced frame");
            at(0x1b2021u); esi = eax;
            at(0x1b2023u); eax = flags.logic(0u, 4); publish_flags();
            at(0x1b2025u); edx = esp + 0x14u;
            const auto frame_sp = esp;
            push(0x1b2029u, edx);
            at(0x1b202au); ecx = 0x1d6u;
            at(0x1b202fu); edi = esp + 0x1cu;
            at(0x1b2033u);
            while (ecx) {
                w(memory, observe, 0x1b2033u, edi, 4, eax);
                edi += (abi.eflags & 0x400u) ? 0xfffffffcu : 4u;
                --ecx;
                at(0x1b2033u);
            }
            push(0x1b2035u, esi);
            at(0x1b2036u); eax = esp + 0x18u;
            push(0x1b203au, eax);
            push(0x1b203bu, 0u);
            push(0x1b203du, ebx);
            at(0x1b203eu); eax = esp + 0x2cu;
            at(0x1b2042u); w(memory, observe, 0x1b2042u, esp + 0x2cu, 2, 6u);
            at(0x1b2049u); w(memory, observe, 0x1b2049u, esp + 0x40u, 1, 1u);
            at(0x1b204eu); w(memory, observe, 0x1b204eu, esp + 0x85u, 1, 1u);
            call_child(0x1b2056u, 0x1b205bu, 0x26af90u, frame_sp,
                "Native behavior perform 1B1FE0 requires its actual 26AF90 child",
                "Native behavior perform 1B1FE0 child 26AF90 returned an unbalanced frame");
            at(0x1b205bu); ecx = r(memory, esp + 0x14u);
            at(0x1b205fu); edx = r(memory, esp + 0x10u);
            push(0x1b2063u, 1u);
            push(0x1b2065u, ecx);
            push(0x1b2066u, esi);
            push(0x1b2067u, edx);
            push(0x1b2068u, eax);
            push(0x1b2069u, ebx);
            call_child(0x1b206au, 0x1b206fu, 0x26c3c0u, frame_sp,
                "Native behavior perform 1B1FE0 requires its actual 26C3C0 child",
                "Native behavior perform 1B1FE0 child 26C3C0 returned an unbalanced frame");
            at(0x1b206fu); edx = esi;
            call_child(0x1b2071u, 0x1b2076u, 0x1d2e30u, frame_sp,
                "Native behavior perform 1B1FE0 requires its actual 1D2E30 child",
                "Native behavior perform 1B1FE0 child 1D2E30 returned an unbalanced frame");
        }
    }

    edi = pop(0x1b2076u);
    esi = pop(0x1b2077u);
    at(0x1b2078u); eax = (eax & ~0xffu) | 1u;
    ebx = pop(0x1b207au);
    at(0x1b207bu); esp = ebp;
    ebp = pop(0x1b207du);
    at(0x1b207eu);
    if (r(memory, esp) != 0x1b0adbu)
        throw std::runtime_error("Native behavior perform 1B1FE0 has a different actual return word");
    esp += 12u;
    return true;
}

bool native_campaign_ai_behavior_end_45_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    constexpr std::uint32_t entry = 0x1b2090u;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1b0f56u ||
        call.target != entry || call.tail || ecx != entry)
        return false;

    using physics_step_detail::r;
    using physics_step_detail::w;
    physics_contact_runtime_detail::Flags flags;
    flags.load(abi.eflags);
    const auto at = [&](std::uint32_t pc) {
        if (services.instruction) services.instruction(pc);
    };
    const auto publish_flags = [&] { abi.eflags = (abi.eflags & ~0x8d5u) | flags.bits(); };
    const auto push = [&](std::uint32_t pc, std::uint32_t value) {
        at(pc);
        const auto target = esp - 4u;
        if (observe) {
            const SourcePhysicsContactRecordObserver completed = [&](const auto &store) {
                esp = target;
                observe(store);
            };
            w(memory, completed, pc, target, 4, value);
        } else {
            w(memory, observe, pc, target, 4, value);
            esp = target;
        }
    };
    const auto finish = [&](std::uint32_t pop_pc, std::uint32_t ret_pc) {
        at(pop_pc);
        esi = r(memory, esp);
        esp += 4u;
        at(ret_pc);
        if (r(memory, esp) != 0x1b0f58u)
            throw std::runtime_error("Native behavior end 1B2090 has a different actual return word");
        esp += 12u;
        return true;
    };

    at(entry); eax = r(memory, 0x5020c8u);
    at(0x1b2095u); ecx = r(memory, eax + 0x44u);
    push(0x1b2098u, esi);
    at(0x1b2099u); esi = r(memory, esp + 8u);
    at(0x1b209du); esi = flags.logic(esi & 0xffffu, 4); publish_flags();
    at(0x1b20a3u); esi = flags.product(esi, 0x888u); publish_flags();
    at(0x1b20a9u); esi = flags.plus(esi, ecx, 4); publish_flags();
    at(0x1b20abu); ecx = r(memory, esi + 0x338u);
    at(0x1b20b1u); flags.minus(ecx, 0xffffffffu, 4); publish_flags();
    at(0x1b20b4u);
    if (!flags.zf) {
        const auto child_sp = esp;
        push(0x1b20b6u, 0x1b20bbu);
        if (!services.required_owner)
            throw std::runtime_error("Native behavior end 1B2090 requires its actual 267410 child");
        services.required_owner(memory, {0x1b20b6u, 0x267410u, false}, abi, observe);
        if (esp != child_sp)
            throw std::runtime_error("Native behavior end 1B2090 child 267410 returned an unbalanced frame");
        flags.load(abi.eflags);
        at(0x1b20bbu); flags.logic(eax, 4); publish_flags();
        at(0x1b20bdu);
        if (!flags.zf) {
            at(0x1b20bfu); flags.minus(r(memory, eax, 2), 6u, 2); publish_flags();
            at(0x1b20c3u);
            if (flags.sf == flags.of) {
                at(0x1b20c5u); w(memory, observe, 0x1b20c5u, esi + 0x488u, 1, 1u);
                at(0x1b20ccu); w(memory, observe, 0x1b20ccu, esi + 0x41cu, 2, 4u);
                at(0x1b20d5u); w(memory, observe, 0x1b20d5u, esi + 0x420u, 2, 2u);
                return finish(0x1b20deu, 0x1b20dfu);
            }
        }
    }

    at(0x1b20e2u); flags.minus(r(memory, esi + 0x358u, 2), 0u, 2); publish_flags();
    at(0x1b20eau);
    if (!flags.zf && flags.sf == flags.of) {
        at(0x1b20ecu); w(memory, observe, 0x1b20ecu, esi + 0x41cu, 2, 2u);
        at(0x1b20f5u); w(memory, observe, 0x1b20f5u, esi + 0x420u, 2, 5u);
        return finish(0x1b20feu, 0x1b20ffu);
    }

    at(0x1b2102u); eax = 2u;
    at(0x1b2107u); w(memory, observe, 0x1b2107u, esi + 0x41cu, 2, eax & 0xffffu);
    at(0x1b210eu); w(memory, observe, 0x1b210eu, esi + 0x420u, 2, eax & 0xffffu);
    return finish(0x1b2115u, 0x1b2116u);
}

}
