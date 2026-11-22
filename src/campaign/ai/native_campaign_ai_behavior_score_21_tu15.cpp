#include "halo2/native_campaign_ai_behavior_score_21_tu15.hpp"
#include "halo2/detail/physics_scalar_memory.hpp"

namespace halo2 {
namespace {
constexpr std::uint32_t entry = 0x1ca160u;
constexpr std::uint32_t descriptor = 0x48b7c0u;

void logic_flags(SourcePhysicsWorldRuntimeAbi &abi, std::uint32_t value, std::uint32_t sign_bit) {
    // Logic instructions clear CF/OF and define SF/ZF/PF. Match the retained
    // World flag convention for undefined AF, leaving all other flags alone.
    const auto parity = (std::popcount(value & 0xffu) & 1) == 0;
    abi.eflags = (abi.eflags & ~0x8d5u) | (std::uint32_t(parity) << 2u) |
        (std::uint32_t(value == 0) << 6u) | (std::uint32_t((value & sign_bit) != 0) << 7u);
}
}

bool native_campaign_ai_behavior_score_21_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1b0195u ||
        call.target != entry || call.tail || abi.gp[0] != entry || abi.gp[7] != descriptor)
        return false;

    using physics_step_detail::r;
    using physics_step_detail::w;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    const auto at = [&](std::uint32_t pc) {
        if (services.instruction) services.instruction(pc);
    };
    at(entry);
    ecx = r(memory, esp + 4u);
    at(0x1ca164u);
    edx = r(memory, 0x5020c8u);
    at(0x1ca16au);
    edx = r(memory, edx + 0x44u);
    at(0x1ca16du);
    ecx &= 0xffffu;
    logic_flags(abi, ecx, 0x80000000u);
    at(0x1ca173u);
    // This product fits signed32 for every 16-bit index. CF/OF stay clear,
    // and the retained World keeps the other (undefined) IMUL flags.
    ecx *= 0x888u;
    at(0x1ca179u);
    const auto pushed_esp = esp - 4u;
    if (observe) {
        // A guard can refuse the store before PUSH commits. A post-store
        // observer sees the completed write and stack pointer even if it stops.
        const SourcePhysicsContactRecordObserver completed = [&](const auto &store) {
            esp = pushed_esp;
            observe(store);
        };
        w(memory, completed, 0x1ca179u, pushed_esp, 4, ebx);
    } else {
        w(memory, observe, 0x1ca179u, pushed_esp, 4, ebx);
        esp = pushed_esp;
    }
    at(0x1ca17au);
    ebx = (ebx & 0xffffff00u) | r(memory, ecx + edx + 0x223u, 1);
    at(0x1ca181u);
    eax = 0;
    logic_flags(abi, eax, 0x80000000u);
    at(0x1ca183u);
    const auto score_byte = ebx & 0xffu;
    logic_flags(abi, score_byte, 0x80u);
    at(0x1ca185u);
    ebx = r(memory, esp);
    esp += 4u;
    at(0x1ca186u);
    if (score_byte != 0) {
        at(0x1ca188u);
        eax = 3;
    }
    at(0x1ca18du);
    if (r(memory, esp) != 0x1b0197u)
        throw std::runtime_error("Native behavior score 1CA160 has a different actual return word");
    esp += 8u;
    return true;
}
}
