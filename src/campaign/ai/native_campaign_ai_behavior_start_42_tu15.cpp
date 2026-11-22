#include "halo2/native_campaign_ai_behavior_start_42_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2 {

bool native_campaign_ai_behavior_select_42_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    constexpr std::uint32_t entry = 0x1c0190u, descriptor = 0x48afc0u;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1afce4u ||
        call.target != entry || call.tail || eax != entry || ebx != descriptor)
        return false;

    using physics_step_detail::r;
    using physics_step_detail::w;
    physics_contact_runtime_detail::Flags flags;
    flags.load(abi.eflags);
    const auto at = [&](std::uint32_t pc) {
        if (services.instruction) services.instruction(pc);
    };
    const auto read = [&](std::uint32_t pc, std::uint32_t address, unsigned width = 4u) {
        at(pc);
        return r(memory, address, width);
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
        const auto value = read(pc, esp);
        esp += 4u;
        return value;
    };
    const auto finish = [&](std::uint32_t pop_esi, std::uint32_t ret) {
        esi = pop(pop_esi);
        if (read(ret, esp) != 0x1afce6u)
            throw std::runtime_error("Native behavior select 1C0190 has a different actual return word");
        esp += 16u;
        return true;
    };
    const auto context_selects_alternate = [&] {
        ecx = read(0x1c01d3u, 0x47b550u);
        edx = read(0x1c01d9u, ecx + 8u);
        flags.minus(edx, read(0x1c01dcu, 0x47b908u), 4); publish_flags();
        at(0x1c01e2u);
        if (flags.zf) return false;
        edx = read(0x1c01e4u, ecx + 4u);
        ecx = read(0x1c01e7u, 0x4fafc0u);
        at(0x1c01edu); edx = flags.logic(edx & ecx, 4); publish_flags();
        at(0x1c01efu); flags.minus(edx, ecx, 4); publish_flags();
        at(0x1c01f1u);
        if (!flags.zf) return false;
        ecx = read(0x1c01f3u, 0x56ae88u);
        at(0x1c01f9u); ecx = flags.right(ecx, 22u, 4, false); publish_flags();
        at(0x1c01fcu); flags.logic(ecx & 1u, 1); publish_flags();
        at(0x1c01ffu);
        return !flags.zf;
    };

    ecx = read(entry, esp + 4u);
    edx = read(0x1c0194u, 0x5020c8u);
    eax = read(0x1c019au, 0x47c1a4u);
    at(0x1c019fu); ecx = flags.logic(ecx & 0xffffu, 4); publish_flags();
    at(0x1c01a5u); ecx = flags.product(ecx, 0x888u); publish_flags();
    push(0x1c01abu, esi);
    esi = read(0x1c01acu, edx + 0x44u);
    edx = read(0x1c01afu, ecx + esi + 0x338u);
    at(0x1c01b6u); ecx = flags.plus(ecx, esi, 4); publish_flags();
    at(0x1c01b8u); flags.minus(edx, 0xffffffffu, 4); publish_flags();
    at(0x1c01bbu);
    if (flags.zf) return finish(0x1c022cu, 0x1c022du);

    push(0x1c01bdu, ebx);
    ebx = (ebx & ~0xffu) | read(0x1c01beu, ecx + 0x225u, 1);
    at(0x1c01c4u); flags.logic(ebx, 1); publish_flags();
    at(0x1c01c6u);
    if (flags.zf) {
        ecx = read(0x1c01c8u, esp + 0x10u);
        ebx = (ebx & ~0xffu) | read(0x1c01ccu, ecx + 0xdu, 1);
        at(0x1c01cfu); flags.logic(ebx, 1); publish_flags();
        at(0x1c01d1u);
        if (!flags.zf) {
            if (context_selects_alternate()) {
                ebx = pop(0x1c0201u);
                at(0x1c0202u); eax = 0x36u;
                return finish(0x1c0207u, 0x1c0208u);
            }
        } else {
            ecx = read(0x1c020bu, 0x50eef0u);
            ecx = read(0x1c0211u, ecx + 0x44u);
            at(0x1c0214u); edx = flags.logic(edx & 0xffffu, 4); publish_flags();
            at(0x1c021au); edx = flags.product(edx, 0x3cu); publish_flags();
            flags.minus(read(0x1c021du, edx + ecx + 0x18u, 2), 1u, 2); publish_flags();
            at(0x1c0223u);
            if (flags.zf) eax = (eax & ~0xffffu) | read(0x1c0225u, 0x47c1a8u, 2);
        }
    }
    ebx = pop(0x1c022bu);
    return finish(0x1c022cu, 0x1c022du);
}

bool native_campaign_ai_behavior_start_42_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    constexpr std::uint32_t entry = 0x1c0230u, descriptor = 0x48afc0u;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1b0d1cu ||
        call.target != entry || call.tail || eax != entry || ecx != descriptor)
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

    at(entry); ecx = r(memory, esp + 4u);
    at(0x1c0234u); ecx = flags.logic(ecx & 0xffffu, 4); publish_flags();
    at(0x1c023au); ecx = flags.product(ecx, 0x888u); publish_flags();
    push(0x1c0240u, ebx);
    push(0x1c0241u, esi);
    at(0x1c0242u); esi = r(memory, 0x5020c8u);
    at(0x1c0248u); edx = r(memory, esi + 0x44u);
    at(0x1c024bu); ebx = (ebx & ~0xffu) | r(memory, edx + ecx + 0x225u, 1);
    at(0x1c0252u); edx = flags.plus(edx, ecx, 4); publish_flags();
    at(0x1c0254u); eax &= ~0xffu; flags.logic(0, 1); publish_flags();
    at(0x1c0256u); flags.minus(ebx, 0, 1); publish_flags();
    at(0x1c0258u);
    if (flags.zf) {
        at(0x1c025au); w(memory, observe, 0x1c025au, edx + 0x220u, 1, 0);
        at(0x1c0260u); eax = r(memory, esp + 0x10u);
        at(0x1c0264u); w(memory, observe, 0x1c0264u, edx + 0x84u, 2, 4);
        at(0x1c026du); w(memory, observe, 0x1c026du, eax + 0x0cu, 1, 1);
        // The request store can alias or observe the actor pool. Reread its
        // row base, and reload the sentinel separately for each six-byte row.
        at(0x1c0271u); eax = r(memory, esi + 0x44u);
        at(0x1c0274u); eax = flags.plus(eax, ecx, 4); publish_flags();
        at(0x1c0276u); w(memory, observe, 0x1c0276u, eax + 0x3feu, 2, 3);
        at(0x1c027fu); eax = flags.plus(eax, 0x402u, 4); publish_flags();
        at(0x1c0284u); ecx = 4;
        at(0x1c0289u);
        do {
            at(0x1c0290u); edx = r(memory, 0x47d6c0u);
            at(0x1c0296u); w(memory, observe, 0x1c0296u, eax, 4, edx);
            at(0x1c0298u); eax = flags.plus(eax, 6u, 4); publish_flags();
            at(0x1c029bu); ecx = flags.increment(ecx, 4, true); publish_flags();
            at(0x1c029cu);
        } while (!flags.zf);
        at(0x1c029eu); eax = (eax & ~0xffu) | 1u;
    }

    at(0x1c02a0u); esi = r(memory, esp); esp += 4u;
    at(0x1c02a1u); ebx = r(memory, esp); esp += 4u;
    at(0x1c02a2u);
    if (r(memory, esp) != 0x1b0d1eu)
        throw std::runtime_error("Native behavior start 1C0230 has a different actual return word");
    esp += 12u;
    return true;
}

bool native_campaign_ai_behavior_update_42_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    constexpr std::uint32_t entry = 0x1c02b0u, descriptor = 0x48afc0u;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1b0b1au ||
        call.target != entry || call.tail || esi != descriptor)
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

    at(entry); eax = r(memory, esp + 4u);
    at(0x1c02b4u); ecx = r(memory, 0x5020c8u);
    at(0x1c02bau); edx = r(memory, ecx + 0x44u);
    push(0x1c02bdu, esi);
    at(0x1c02beu); esi = eax;
    at(0x1c02c0u); esi = flags.logic(esi & 0xffffu, 4); publish_flags();
    at(0x1c02c6u); esi = flags.product(esi, 0x888u); publish_flags();
    at(0x1c02ccu); ecx = (ecx & ~0xffu) | r(memory, esi + edx + 0x225u, 1);
    at(0x1c02d3u); esi = flags.plus(esi, edx, 4); publish_flags();
    at(0x1c02d5u); flags.logic(ecx, 1); publish_flags();
    at(0x1c02d7u);
    std::uint32_t ret = 0x1c02e0u;
    if (!flags.zf) {
        at(0x1c02d9u); eax = (eax & ~0xffffu) | r(memory, 0x47c1a4u, 2);
        at(0x1c02dfu); esi = r(memory, esp); esp += 4u;
    } else {
        at(0x1c02e3u); edx = r(memory, esp + 0x10u);
        push(0x1c02e7u, edi);
        at(0x1c02e8u); edi = r(memory, esp + 0x10u);
        const auto child_return_sp = esp;
        push(0x1c02ecu, edx);
        push(0x1c02edu, edi);
        push(0x1c02eeu, eax);
        push(0x1c02efu, 0x1c02f4u);
        if (!services.required_owner)
            throw std::runtime_error("Native behavior update 1C02B0 requires its actual 1B05D0 child");
        services.required_owner(memory, {0x1c02efu, 0x1b05d0u, false}, abi, observe);
        if (esp != child_return_sp)
            throw std::runtime_error("Native behavior update 1C02B0 child returned an unbalanced frame");

        flags.load(abi.eflags);
        at(0x1c02f4u); flags.minus(eax, r(memory, 0x47c1a4u, 2), 2); publish_flags();
        at(0x1c02fbu);
        if (flags.zf) {
            at(0x1c02fdu); eax = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(edi)));
            at(0x1c0300u); eax = flags.shift(eax, 6u, 4); publish_flags();
            at(0x1c0303u); w(memory, observe, 0x1c0303u, eax + esi + 0x9du, 1, 1);
            at(0x1c030bu); eax = 2;
        }
        at(0x1c0310u); edi = r(memory, esp); esp += 4u;
        at(0x1c0311u); esi = r(memory, esp); esp += 4u;
        ret = 0x1c0312u;
    }
    at(ret);
    if (r(memory, esp) != 0x1b0b1du)
        throw std::runtime_error("Native behavior update 1C02B0 has a different actual return word");
    esp += 16u;
    return true;
}

}
