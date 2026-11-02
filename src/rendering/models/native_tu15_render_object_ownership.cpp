#include "halo2/native_tu15_render_object_ownership.hpp"
#include "rendering/models/native_tu15_render_ownership_detail.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& memory, std::uint32_t at, unsigned width = 4) {
    std::array<std::byte, 4> bytes{};
    memory.read(at, std::span(bytes).first(width));
    std::uint32_t value{};
    for (unsigned n = 0; n < width; ++n) value |= std::to_integer<unsigned>(bytes[n]) << (n * 8);
    return value;
}
bool nan(std::uint32_t bits) {return (bits & 0x7fffffffu) > 0x7f800000u;}
bool ordered_equal(std::uint32_t first, std::uint32_t second, NativeTu15PhysicsFrameFp& fp, bool live_sse) {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    if (live_sse) fp.mxcsr = _mm_getcsr();
    if ((fp.mxcsr & 0x1f80u) != 0x1f80u || (fp.mxcsr & 0xffff0000u))
        throw std::invalid_argument("Native render ownership requires a masked SSE environment");
    struct Scope {
        NativeTu15PhysicsFrameFp& state; unsigned host; bool restore;
        Scope(NativeTu15PhysicsFrameFp& f, bool live):state(f),host(live ? f.mxcsr : _mm_getcsr()),restore(!live) {
            if (restore) _mm_setcsr(f.mxcsr);
        }
        ~Scope() {state.mxcsr = _mm_getcsr(); if (restore) _mm_setcsr(host);}
    } environment(fp, live_sse);
    const auto a = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(first)));
    const auto b = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(second)));
    // UCOMISS must stay between the selected MXCSR install and its save.
    // The intrinsic is a pure compiler builtin and can move across LDMXCSR.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char equal;
    asm volatile("ucomiss %2,%1; sete %0" : "=qm"(equal) : "x"(a), "x"(b) : "cc", "memory");
#else
    volatile int equal = _mm_ucomieq_ss(a, b);
#endif
    // UCOMISS reports ZF for unordered operands too. Keep its hardware
    // exceptions and DAZ behavior, then exclude unordered through bit tests.
    return equal != 0 && !nan(first) && !nan(second);
#else
    (void)first; (void)second; (void)fp; (void)live_sse;
    throw std::runtime_error("Native TU1.5 render ownership requires its SSE comparison backend");
#endif
}
}
namespace render_ownership_detail {
Root root(const SourcePhysicsContactMemory& memory, std::uint32_t object) {
    Root result;
    if (object == 0xffffffffu) return result;
    const auto pool = read(memory, 0x4ecbd0u);
    result.headers = read(memory, pool + 0x44u);
    unsigned depth{};
    do {
        if (depth++ == 65536u) throw std::runtime_error("Native render owner has a cyclic object-parent chain");
        result.identity = object;
        const auto body = read(memory, result.headers + (object & 65535u) * 12u + 8u);
        object = read(memory, body + 0x14u);
    } while (object != 0xffffffffu);
    return result;
}
Attachment attachment(const SourcePhysicsContactMemory& memory, std::uint32_t object,
    std::int16_t index, const std::function<void()>& before_definition) {
    Attachment result; result.index = index;
    const auto pool = read(memory, 0x4ecbd0u);
    const auto headers = read(memory, pool + 0x44u);
    const auto body = read(memory, headers + (object & 65535u) * 12u + 8u);
    const auto definition_id = read(memory, body);
    result.directory = read(memory, 0x4f0414u);
    if (before_definition) before_definition();
    result.definition = read(memory, result.directory + (definition_id & 65535u) * 16u + 8u);
    if (index < 0) return result;
    result.count = read(memory, result.definition + 0x94u); result.count_read = true;
    if (std::int32_t(index) >= std::bit_cast<std::int32_t>(result.count)) return result;
    result.rows = read(memory, result.definition + 0x98u);
    result.marker = read(memory, result.rows + std::uint32_t(index) * 24u + 8u);
    result.found = true;
    return result;
}
ViewCategory category(const SourcePhysicsContactMemory& memory, std::uint32_t view,
    NativeTu15PhysicsFrameFp& fp, bool live_sse) {
    ViewCategory result;
    result.row = 0x4f5514u + view * 320u;
    result.callback = read(memory, result.row + 12u);
    if (result.callback == 0x245400u) {
        result.blend = read(memory, result.row + 4u);
        result.zero = read(memory, 0x46a0e8u);
        result.compared = true; result.unordered = nan(result.blend) || nan(result.zero);
        if (ordered_equal(result.blend, result.zero, fp, live_sse)) result.category = 0;
    } else if (result.callback == 0x245ea0u) result.category = 1;
    else if (result.callback == 0x16f6a0u) result.category = 2;
    else if (result.callback == 0x246380u) {
        result.first_person_gate = std::uint8_t(read(memory, 0x52d6a0u, 1));
        if (result.first_person_gate) result.category = 2;
    }
    return result;
}
Priority priority(const SourcePhysicsContactMemory& memory, std::uint32_t object,
    NativeTu15PhysicsFrameFp& fp, const PriorityTransport& transport, bool live_sse) {
    Priority result;
    if (object == 0xffffffffu) return result;
    if (transport.object_present) transport.object_present();
    result.view = read(memory, 0x4c66f8u); result.path = PriorityPath::no_view;
    if (result.view == 0xffffffffu) return result;
    if (transport.enter_camera) transport.enter_camera(result.view);
    result.camera = category(memory, result.view, fp, live_sse);
    if (transport.leave_camera) transport.leave_camera(result.camera);
    result.path = PriorityPath::other_camera;
    if (result.camera.category) return result;
    if (transport.enter_root) transport.enter_root();
    result.root = root(memory, object);
    if (transport.leave_root) transport.leave_root(result.root);
    result.pool = read(memory, 0x4ecbd0u);
    result.headers = read(memory, result.pool + 0x44u);
    result.header = result.headers + (result.root.identity & 65535u) * 12u;
    result.root_kind = std::uint8_t(read(memory, result.header + 3u, 1));
    result.path = PriorityPath::nonunit_root;
    if (result.root_kind) return result;
    result.body = read(memory, result.header + 8u);
    const auto views = read(memory, 0x4f54f0u);
    result.view_unit = read(memory, views + 12u + result.view * 4u);
    result.root_unit = read(memory, result.body + 0x13cu);
    result.path = result.view_unit == result.root_unit ? PriorityPath::matching_unit : PriorityPath::different_unit;
    return result;
}
}
std::uint32_t native_tu15_render_root_owner(const SourcePhysicsContactMemory& memory, std::uint32_t object) {
    return render_ownership_detail::root(memory, object).identity;
}
std::uint32_t native_tu15_render_attachment_marker(const SourcePhysicsContactMemory& memory,
    std::uint32_t object, std::int16_t index) {
    return render_ownership_detail::attachment(memory, object, index).marker;
}
std::uint32_t native_tu15_render_view_category(const SourcePhysicsContactMemory& memory,
    std::uint32_t view, NativeTu15PhysicsFrameFp& fp) {
    return render_ownership_detail::category(memory, view, fp).category;
}
bool native_tu15_render_owner_has_view_priority(const SourcePhysicsContactMemory& memory,
    std::uint32_t object, NativeTu15PhysicsFrameFp& fp) {
    return render_ownership_detail::priority(memory, object, fp).path == render_ownership_detail::PriorityPath::matching_unit;
}
}
