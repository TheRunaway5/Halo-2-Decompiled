#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_object_motion_update_detail {
using namespace native_object_tick_detail::boundary;
using native_object_tick_detail::Trace;
using native_small_queries_detail::VerifySummary;

enum Branch : unsigned {
    disconnect, keep_links, no_position, validate_position, rejected_position, write_position,
    notify_position, no_orientation, write_orientation, notify_orientation,
    update_physics, rebuild_nodes, reconnect, branch_count
};

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

inline VerifySummary &summary() {
    static VerifySummary value{"Object motion update"};
    return value;
}

template<class State, class Disconnect, class EntityNotice, class PhysicsNotice, class Reconnect>
struct Adapter : Port<State> {
    Disconnect &disconnect_object;
    EntityNotice &notify_entity_change;
    PhysicsNotice &update_object_physics;
    Reconnect &reconnect_object;

    Adapter(State &state, std::uint32_t *cursor, Trace<State> *trace,
            Disconnect &disconnect, EntityNotice &entity_notice, PhysicsNotice &physics_notice, Reconnect &reconnect)
        : Port<State>(state, cursor, trace, {}), disconnect_object(disconnect), notify_entity_change(entity_notice),
          update_object_physics(physics_notice), reconnect_object(reconnect) {}

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(std::bit_cast<float>(this->read(pc, address, 4)));
    }

    bool position_in_bounds() {
        auto &gp = this->gp;
        auto &xmm = this->s.xmm;
        xmm[0] = scalar(0xb94afu, gp[ecx]);
        xmm[1] = scalar(0xb94b3u, 0x46aa04u);
        this->flags.scalar_compare(xmm[0], xmm[1], false);
        if (this->flags.cf) return false;
        xmm[0] = scalar(0xb94c0u, 0x46a530u);
        this->flags.scalar_compare(xmm[0], scalar(0xb94c8u, gp[ecx]), false);
        if (this->flags.cf) return false;
        xmm[2] = scalar(0xb94cdu, gp[ecx] + 4u);
        this->flags.scalar_compare(xmm[2], xmm[1], false);
        if (this->flags.cf) return false;
        this->flags.scalar_compare(xmm[0], xmm[2], false);
        if (this->flags.cf) return false;
        xmm[2] = scalar(0xb94dcu, gp[ecx] + 8u);
        this->flags.scalar_compare(xmm[2], xmm[1], false);
        if (this->flags.cf) return false;
        this->flags.scalar_compare(xmm[0], xmm[2], false);
        return !this->flags.cf;
    }

    void save_frame() {
        auto &gp = this->gp;
        this->push(0xb9460u, gp[ecx]);
        gp[ecx] = this->read(0xb9461u, gp[esp] + 8u, 4);
        this->push(0xb9465u, gp[ebx]);
        this->push(0xb9466u, gp[ebp]);
        gp[ebp] = this->read(0xb9467u, 0x4ecbd0u, 4);
        gp[eax] = this->flags.logic(gp[ecx] & 0xffffu, 4);
        this->push(0xb9474u, gp[esi]);
        gp[esi] = 3u * gp[eax];
        gp[eax] = this->read(0xb9478u, gp[ebp] + 0x44u, 4);
        gp[esi] = this->flags.shift(gp[esi], 2u, 4);
        this->push(0xb947eu, gp[edi]);
        gp[edi] = this->read(0xb947fu, gp[eax] + gp[esi] + 8u, 4);
        gp[ebx] = this->read(0xb9483u, gp[edi] + 4u, 4);
        gp[ebx] = this->flags.right(gp[ebx], 8u, 4, false);
        set_low_byte(gp[ebx], this->flags.logic(gp[ebx] & 1u, 1));
        this->store(0xb948cu, gp[esp] + 0x13u, 1, 1u);
    }

    void disconnect_if_requested() {
        auto &gp = this->gp;
        if (!this->flags.zf) {
            set_low_byte(gp[eax], this->read(0xb9493u, gp[esp] + 0x38u, 1));
            this->test8(gp[eax]);
            if (this->flags.zf) {
                this->reached(Branch::disconnect);
                this->push(0xb949bu, gp[ecx]);
                this->call(0xb949cu, 0xb94a1u, 0xba7e0u, disconnect_object);
                return;
            }
        }
        this->reached(Branch::keep_links);
    }

    void move_position() {
        auto &gp = this->gp;
        gp[ecx] = this->read(0xb94a1u, gp[esp] + 0x1cu, 4);
        this->test32(gp[ecx]);
        if (this->flags.zf) {
            this->reached(Branch::no_position);
            return;
        }
        this->compare32(this->read(0xb94a9u, gp[edi] + 0x14u, 4), 0xffffffffu);
        if (this->flags.zf) {
            this->reached(Branch::validate_position);
            if (!position_in_bounds()) {
                this->reached(Branch::rejected_position);
                this->store(0xb94ebu, gp[esp] + 0x13u, 1, 0u);
                return;
            }
        }

        this->reached(Branch::write_position);
        gp[eax] = this->read(0xb94f2u, gp[ecx], 4);
        gp[edx] = gp[edi] + 0x64u;
        this->store(0xb94f7u, gp[edx], 4, gp[eax]);
        gp[eax] = this->read(0xb94f9u, gp[ecx] + 4u, 4);
        this->store(0xb94fcu, gp[edx] + 4u, 4, gp[eax]);
        gp[ecx] = this->read(0xb94ffu, gp[ecx] + 8u, 4);
        this->store(0xb9502u, gp[edx] + 8u, 4, gp[ecx]);

        gp[edx] = this->read(0xb9505u, gp[ebp] + 0x44u, 4);
        gp[eax] = this->read(0xb9508u, gp[edx] + gp[esi] + 8u, 4);
        gp[eax] = this->read(0xb950cu, gp[eax] + 0xd4u, 4);
        this->compare32(gp[eax], 0xffffffffu);
        if (!this->flags.zf) {
            this->reached(Branch::notify_position);
            this->push(0xb9517u, 2u);
            this->push(0xb9519u, gp[eax]);
            this->call(0xb951au, 0xb951fu, 0xb7e50u, notify_entity_change);
        }
        gp[ecx] = this->read(0xb951fu, gp[esp] + 0x1cu, 4);
    }

    void update_orientation() {
        auto &gp = this->gp;
        gp[eax] = this->read(0xb9523u, gp[esp] + 0x20u, 4);
        this->test32(gp[eax]);
        if (this->flags.zf) {
            this->reached(Branch::no_orientation);
            return;
        }

        this->reached(Branch::write_orientation);
        gp[edx] = this->read(0xb952bu, gp[eax], 4);
        gp[ecx] = gp[edi] + 0x70u;
        this->store(0xb9530u, gp[ecx], 4, gp[edx]);
        gp[edx] = this->read(0xb9532u, gp[eax] + 4u, 4);
        this->store(0xb9535u, gp[ecx] + 4u, 4, gp[edx]);
        gp[eax] = this->read(0xb9538u, gp[eax] + 8u, 4);
        this->store(0xb953bu, gp[ecx] + 8u, 4, gp[eax]);

        gp[eax] = this->read(0xb953eu, gp[esp] + 0x24u, 4);
        gp[ecx] = this->read(0xb9542u, gp[eax], 4);
        gp[edi] = this->flags.plus(gp[edi], 0x7cu, 4);
        this->store(0xb9547u, gp[edi], 4, gp[ecx]);
        gp[edx] = this->read(0xb9549u, gp[eax] + 4u, 4);
        this->store(0xb954cu, gp[edi] + 4u, 4, gp[edx]);
        gp[eax] = this->read(0xb954fu, gp[eax] + 8u, 4);
        this->store(0xb9552u, gp[edi] + 8u, 4, gp[eax]);

        gp[ecx] = this->read(0xb9555u, gp[ebp] + 0x44u, 4);
        gp[esi] = this->read(0xb9558u, gp[ecx] + gp[esi] + 8u, 4);
        gp[eax] = this->read(0xb955cu, gp[esi] + 0xd4u, 4);
        this->compare32(gp[eax], 0xffffffffu);
        if (!this->flags.zf) {
            this->reached(Branch::notify_orientation);
            this->push(0xb9567u, 4u);
            this->push(0xb9569u, gp[eax]);
            this->call(0xb956au, 0xb956fu, 0xb7e50u, notify_entity_change);
        }
        gp[ecx] = this->read(0xb956fu, gp[esp] + 0x1cu, 4);
    }

    void finish_relocation() {
        auto &gp = this->gp;
        set_low_byte(gp[eax], this->read(0xb9573u, gp[esp] + 0x30u, 1));
        this->test8(gp[eax]);
        if (!this->flags.zf) {
            this->reached(Branch::update_physics);
            gp[edx] = this->read(0xb957bu, gp[esp] + 0x34u, 4);
            this->test32(gp[ecx]);
            set_low_byte(gp[eax], this->flags.zf);
            this->push(0xb9584u, gp[edx]);
            this->push(0xb9585u, gp[eax]);
            gp[eax] = this->read(0xb9586u, gp[esp] + 0x20u, 4);
            this->call(0xb958au, 0xb958fu, 0x1cd620u, update_object_physics);
        }
        set_low_byte(gp[eax], this->read(0xb958fu, gp[esp] + 0x2cu, 1));
        this->test8(gp[eax]);
        if (!this->flags.zf) {
            this->reached(Branch::rebuild_nodes);
            gp[ecx] = this->read(0xb9597u, gp[esp] + 0x18u, 4);
            this->push(0xb959bu, gp[ecx]);
            this->call(0xb959cu, 0xb95a1u, 0xbf050u,
                       [](State &s) { s.required(0xb959cu, 0xbf050u, false); });
        }

        gp[edi] = this->pop(0xb95a1u);
        gp[esi] = this->pop(0xb95a2u);
        gp[ebp] = this->pop(0xb95a3u);
        this->test8(gp[ebx]);
        gp[ebx] = this->pop(0xb95a6u);
        if (!this->flags.zf) {
            set_low_byte(gp[eax], this->read(0xb95a9u, gp[esp] + 0x28u, 1));
            this->test8(gp[eax]);
            if (this->flags.zf) {
                this->reached(Branch::reconnect);
                gp[edx] = this->read(0xb95b1u, gp[esp] + 0x18u, 4);
                gp[eax] = this->read(0xb95b5u, gp[esp] + 8u, 4);
                this->push(0xb95b9u, gp[edx]);
                this->push(0xb95bau, gp[eax]);
                this->call(0xb95bbu, 0xb95c0u, 0xba630u, reconnect_object);
            }
        }
        set_low_byte(gp[eax], this->read(0xb95c0u, gp[esp] + 3u, 1));
        gp[ecx] = this->pop(0xb95c4u);
        this->ret(0xb95c5u, 36u);
        this->publish(0xb95c5u);
    }

    void run() {
        save_frame();
        disconnect_if_requested();
        move_position();
        update_orientation();
        finish_relocation();
    }
};

template<class State, class Reference, class Disconnect, class EntityNotice, class PhysicsNotice, class Reconnect>
bool bridge(State &s, Reference reference, Disconnect disconnect, EntityNotice entity_notice,
            PhysicsNotice physics_notice, Reconnect reconnect) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_OBJECT_MOTION_UPDATE");
    static const bool verify = flag("HALO2_VERIFY_OBJECT_MOTION_UPDATE");
    return native_small_queries_detail::run_owner(s, 0xb9460u, 0xb95c5u, force_reference, verify,
        coverage(), summary(), reference, [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State, Disconnect, EntityNotice, PhysicsNotice, Reconnect>{
                value, cursor, trace, disconnect, entity_notice, physics_notice, reconnect};
        });
}

} // namespace halo2::native_object_motion_update_detail
