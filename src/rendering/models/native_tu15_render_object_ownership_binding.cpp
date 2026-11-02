#include "halo2/native_tu15_render_object_ownership_binding.hpp"
#include "rendering/models/native_tu15_render_ownership_transport.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using Abi = SourcePhysicsWorldRuntimeAbi;
using namespace render_ownership_detail;
std::array<std::atomic<std::uint64_t>, 4> calls{};
unsigned ordinal(std::uint32_t entry) {
    switch (entry) {
        case 0xbcfb0u: return 0; case 0xbac70u: return 1;
        case 0x158290u: return 2; case 0x3e9b0u: return 3;
        default: throw std::invalid_argument("Unowned native TU1.5 render ownership entry");
    }
}
std::uint32_t read(const SourcePhysicsContactMemory& memory, std::uint32_t address) {
    std::array<std::byte, 4> bytes{}; memory.read(address, bytes);
    std::uint32_t result{};
    for (unsigned n = 0; n < 4; ++n) result |= std::to_integer<unsigned>(bytes[n]) << (n * 8);
    return result;
}
void push(SourcePhysicsContactMemory& memory, Registers& abi, std::uint32_t site,
    std::uint32_t value, const SourcePhysicsContactRecordObserver& observe) {
    abi.gp[4] -= 4;
    std::array<std::byte, 4> bytes{};
    for (unsigned n = 0; n < 4; ++n) bytes[n] = std::byte(value >> (n * 8));
    SourcePhysicsContactRecordStore event{site, abi.gp[4], {}, {}};
    if (observe) {
        event.before.resize(4); memory.read(abi.gp[4], event.before);
        event.written.assign(bytes.begin(), bytes.end());
    }
    memory.write(abi.gp[4], bytes);
    if (observe) observe(event);
}
std::uint32_t pop(const SourcePhysicsContactMemory& memory, Registers& abi) {
    const auto value = read(memory, abi.gp[4]); abi.gp[4] += 4; return value;
}
// Condition codes are compatibility outputs only. The semantic algorithms
// above have no register/flags state and no instruction dispatch.
std::uint32_t logical_flags(std::uint32_t value, unsigned width = 4) {
    const auto mask = width == 4 ? 0xffffffffu : (1u << (width * 8)) - 1;
    value &= mask;
    return ((std::popcount(value & 255u) & 1) ? 0u : 4u) |
        (value ? 0u : 64u) | ((value & (1u << (width * 8 - 1))) ? 128u : 0u);
}
std::uint32_t comparison_flags(std::uint32_t first, std::uint32_t second) {
    const auto result = first - second;
    return logical_flags(result) | (first < second ? 1u : 0u) |
        (((first ^ second ^ result) & 16u) ? 16u : 0u) |
        (((first ^ second) & (first ^ result) & 0x80000000u) ? 2048u : 0u);
}
void flags(Registers& abi, std::uint32_t bits) {abi.eflags = (abi.eflags & ~0x8d5u) | bits;}
void low_result(Registers& abi, unsigned value) {abi.gp[0] = (abi.gp[0] & 0xffffff00u) | value;}
void root_result(Registers& abi, const Root& result, std::uint32_t object) {
    abi.gp[0] = result.identity;
    if (object != 0xffffffffu) {abi.gp[1] = 0xffffffffu; abi.gp[2] = result.headers;}
    flags(abi, 0x44u);
}
void category_result(Registers& abi, const ViewCategory& result) {
    abi.gp[0] = result.category; abi.gp[1] = result.callback; abi.gp[2] = 3;
    if (result.compared) {
        abi.publish_blend(abi.blend_context, result.blend);
        flags(abi, result.category == 0 ? 0x44u : logical_flags(result.unordered ? 0x44u : 0u, 1));
    } else if (result.callback == 0x245ea0u || result.callback == 0x16f6a0u) flags(abi, 0x44u);
    else if (result.callback == 0x246380u) flags(abi, logical_flags(result.first_person_gate, 1));
    else flags(abi, comparison_flags(result.callback, 0x246380u));
}
}
bool native_tu15_render_object_ownership_owns_entry(std::uint32_t entry) {
    return entry == 0xbcfb0u || entry == 0xbac70u || entry == 0x158290u || entry == 0x3e9b0u;
}
void native_tu15_render_object_ownership_run(SourcePhysicsContactMemory& memory, std::uint32_t entry,
    Abi& abi, const SourcePhysicsContactRecordObserver& observe) {
    const auto context = abi.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{0x27fu, 0x1f80u});
    if ((abi.active_mmx && (abi.x87_depth || (abi.x87_status & 0x3800u))) || abi.x87_depth > 8 ||
        (context.x87_control_word & 0x3fu) != 0x3fu ||
        (context.mxcsr & 0x1f80u) != 0x1f80u || (context.mxcsr & 0xffff0000u))
        throw std::invalid_argument("Native render ownership requires a masked scalar FP environment");
    NativeTu15PhysicsFrameFp fp{context.x87_control_word, abi.x87_status, context.mxcsr};
    // Even a later memory/observer rejection retains the comparison's real
    // sticky SSE status; the selected host environment itself is restored.
    struct CommitFp {Abi& abi; NativeTu15PhysicsFrameFp& fp;
        ~CommitFp() {abi.floating_point = SourcePhysicsFactoryFloatingPoint{fp.control_word, fp.mxcsr};}
    } commit{abi, fp};
    Registers registers{abi.gp, abi.eflags, &abi, [](void* context, std::uint32_t blend) {
        static_cast<Abi*>(context)->xmm[0] = {blend, 0, 0, 0};
    }};
    run_transport(memory, entry, registers, fp, observe);
}
void render_ownership_detail::run_transport(SourcePhysicsContactMemory& memory, std::uint32_t entry,
    Registers abi, NativeTu15PhysicsFrameFp& fp, const SourcePhysicsContactRecordObserver& observe, bool live_sse) {
    calls[ordinal(entry)].fetch_add(1, std::memory_order_relaxed);
    if (entry == 0xbcfb0u) {
        const auto object = abi.gp[1];
        root_result(abi, root(memory, object), object);
    } else if (entry == 0xbac70u) {
        const auto index = std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[0]));
        const auto result = attachment(memory, abi.gp[1], index,
            [&] {push(memory, abi, 0xbac94u, abi.gp[6], observe);});
        abi.gp[0] = result.marker; abi.gp[2] = result.definition;
        abi.gp[1] = index < 0 ? result.directory : result.found ? result.rows : std::uint32_t(index);
        flags(abi, index < 0 ? logical_flags(std::uint16_t(index), 2) :
            comparison_flags(std::uint32_t(index), result.count));
        abi.gp[6] = pop(memory, abi);
    } else if (entry == 0x158290u) {
        category_result(abi, category(memory, abi.gp[0], fp, live_sse));
    } else {
        const auto object = abi.gp[6];
        push(memory, abi, 0x3e9b0u, abi.gp[3], observe);
        PriorityTransport transport;
        transport.object_present = [&] {push(memory, abi, 0x3e9b8u, abi.gp[7], observe);};
        transport.enter_camera = [&](auto view) {
            abi.gp[7] = view;
            push(memory, abi, 0x3e9c6u, 0x3e9cbu, observe);
        };
        transport.leave_camera = [&](const auto& result) {
            category_result(abi, result); pop(memory, abi);
            flags(abi, logical_flags(result.category));
        };
        transport.enter_root = [&] {
            abi.gp[1] = object; push(memory, abi, 0x3e9d1u, 0x3e9d6u, observe);
        };
        transport.leave_root = [&](const auto& result) {
            root_result(abi, result, object); pop(memory, abi);
        };
        const auto result = priority(memory, object, fp, transport, live_sse);
        switch (result.path) {
            case PriorityPath::no_object: case PriorityPath::no_view:
                flags(abi, 0x44u); break;
            case PriorityPath::other_camera: break;
            case PriorityPath::nonunit_root:
                abi.gp[0] = result.header;
                abi.gp[1] = (result.pool & 0xffffff00u) | result.root_kind;
                abi.gp[2] = result.headers;
                flags(abi, logical_flags(result.root_kind, 1)); break;
            case PriorityPath::different_unit: case PriorityPath::matching_unit:
                abi.gp[0] = result.body; abi.gp[1] = result.view_unit; abi.gp[2] = result.headers;
                flags(abi, comparison_flags(result.view_unit, result.root_unit)); break;
        }
        if (object != 0xffffffffu) abi.gp[7] = pop(memory, abi);
        low_result(abi, result.path == PriorityPath::matching_unit);
        abi.gp[3] = pop(memory, abi);
    }
    pop(memory, abi);
}
std::uint64_t native_tu15_render_object_ownership_calls(std::uint32_t entry) {
    return calls[ordinal(entry)].load(std::memory_order_relaxed);
}
}
