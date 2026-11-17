#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_render_root_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"

namespace halo2::native_vehicle_render_root_detail {
// Private VehicleUpdate BBE70 source transport. The owning complete body
// decides whether observations are direct or use the prepared literal helper.
// Either route uses the live callable, including the real Phantom PC remap.
template<bool Literal, class State> class SourceFrame {
    using Object = native_render_root_detail::Object;
    using Rows = native_render_root_detail::Rows;
    using Slots = native_render_root_detail::Slots;
    State& s_;
    void at(std::uint32_t pc) {
        if constexpr (Literal) s_.instruction_literal(pc);
        else if (s_.services.instruction) s_.services.instruction(pc);
    }
public:
    explicit SourceFrame(State& s) : s_(s) {}
    Object start() {
        at(0xbbe70u); s_.gp[0] = s_.flags.logic(native_render_root_detail::missing.identity, 4);
        at(0xbbe73u); s_.flags.minus(s_.gp[1], native_render_root_detail::missing.identity, 4);
        return {s_.gp[1]};
    }
    bool has_object(Object) { at(0xbbe76u); return !s_.flags.zf; }
    Rows acquire_rows() {
        at(0xbbe78u); s_.gp[0] = physics_step_detail::r(s_.memory, native_render_root_detail::object_pool_address);
        at(0xbbe7du); s_.gp[2] = physics_step_detail::r(s_.memory, s_.gp[0] + 0x44u);
        return {s_.gp[2]};
    }
    Slots select(Object) {
        at(0xbbe80u); s_.gp[0] = s_.gp[1];
        at(0xbbe82u); s_.gp[1] = s_.flags.logic(native_render_root_detail::object_index({s_.gp[1]}), 4);
        at(0xbbe88u); s_.gp[1] = native_render_root_detail::row_slots(s_.gp[1]).index;
        return {s_.gp[1]};
    }
    Object acquire_body(Rows, Slots) {
        at(0xbbe8bu);
        const auto address = native_render_root_detail::body_address({s_.gp[2]}, {s_.gp[1]});
        return {s_.gp[1] = physics_step_detail::r(s_.memory, address)};
    }
    Object acquire_parent(Object) {
        at(0xbbe8fu);
        return {s_.gp[1] = physics_step_detail::r(s_.memory, native_render_root_detail::parent_address({s_.gp[1]}))};
    }
    bool has_parent(Object) {
        at(0xbbe92u); s_.flags.minus(s_.gp[1], native_render_root_detail::missing.identity, 4);
        at(0xbbe95u); return !s_.flags.zf;
    }
    void finish() { at(0xbbe97u); s_.pop(); }
};

template<bool Literal, class State> void run_source_private(State& s) {
    SourceFrame<Literal,State> view{s};
    native_render_root_detail::find(view);
}
} // namespace halo2::native_vehicle_render_root_detail
#endif
