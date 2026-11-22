#include "halo2/native_campaign_ai_behavior_score_43_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2 {

bool native_campaign_ai_behavior_score_43_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    constexpr std::uint32_t entry = 0x1b5000u;
    constexpr std::uint32_t descriptor = 0x48a450u;
    constexpr std::uint32_t continuation = 0x1b0197u;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.tail ||
        call.instruction != 0x1b0195u || call.target != entry ||
        abi.gp[0] != entry || abi.gp[7] != descriptor)
        return false;

    using physics_step_detail::r;
    using physics_step_detail::w;
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = abi.gp;
    const auto at = [&](std::uint32_t pc) {
        if (services.instruction) services.instruction(pc);
    };
    constexpr std::uint32_t arithmetic_flags = 0x8d5u;
    physics_contact_runtime_detail::Flags flags;
    flags.load(abi.eflags);
    const auto publish_flags = [&] {
        abi.eflags = (abi.eflags & ~arithmetic_flags) | flags.bits();
    };

    at(entry);
    ecx = r(memory, esp + 4u);
    at(0x1b5004u);
    edx = r(memory, 0x5020c8u);
    at(0x1b500au);
    ecx = flags.logic(ecx & 0xffffu, 4);
    publish_flags();
    at(0x1b5010u);
    ecx = flags.product(ecx, 0x888u);
    publish_flags();
    at(0x1b5016u);
    const auto pushed_esp = esp - 4u;
    if (observe) {
        const SourcePhysicsContactRecordObserver completed = [&](const auto &store) {
            esp = pushed_esp;
            observe(store);
        };
        w(memory, completed, 0x1b5016u, pushed_esp, 4, esi);
    } else {
        w(memory, observe, 0x1b5016u, pushed_esp, 4, esi);
        esp = pushed_esp;
    }
    at(0x1b5017u);
    esi = r(memory, edx + 0x44u);
    at(0x1b501au);
    edx = r(memory, ecx + esi + 0x338u);
    at(0x1b5021u);
    ecx = flags.plus(ecx, esi, 4);
    publish_flags();
    at(0x1b5023u);
    eax = flags.logic(0, 4);
    publish_flags();
    at(0x1b5025u);
    flags.minus(edx, 0xffffffffu, 4);
    publish_flags();
    at(0x1b5028u);
    esi = r(memory, esp);
    esp += 4u;
    at(0x1b5029u);
    if (!flags.zf) {
        at(0x1b502bu);
        const auto actor_flag = r(memory, ecx + 7u, 1);
        edx = (edx & 0xffffff00u) | actor_flag;
        at(0x1b502eu);
        flags.logic(actor_flag, 1);
        publish_flags();
        at(0x1b5030u);
        if (flags.zf) {
            at(0x1b5032u);
            eax = 3;
        }
    }
    at(0x1b5037u);
    if (r(memory, esp) != continuation)
        throw std::runtime_error("Native behavior score 1B5000 has a different actual return word");
    esp += 8u;
    return true;
}

}
