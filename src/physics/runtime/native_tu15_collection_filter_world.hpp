#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_collection_filter_world_detail {
namespace world = native_world_leaves_detail;
using world::eax;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::ebp;
using world::esi;
using world::esp;

inline constexpr std::uint32_t image_offset = 0x5240u;
inline constexpr std::uint32_t filter_groups = 0x2e98d0u;
inline constexpr world::Function functions[]{{0x1cbe70u, 0x1cbfabu, image_offset, image_offset}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "World collection filter verify: %llu calls\n",
            static_cast<unsigned long long>(calls.load()));
    }
};

template<class State, class Dispatch, class GroupFilter>
struct Frame {
    world::Port<State> &port;
    Dispatch &dispatch;
    GroupFilter &group_filter;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    // Virtual calls retain their original canonical dispatch site. Stack
    // writes use the image-selected return addresses seen by the callee.
    void call(std::uint32_t site, std::uint32_t returned, std::uint32_t target) {
        port.push(site, returned);
        world::call_dispatch(port, target, site - image_offset, dispatch);
    }

    void walk_parents(unsigned root, unsigned parent, std::uint32_t load_site) {
        while (!flags.zf) {
            gp[root] = gp[parent];
            gp[parent] = port.read(load_site, gp[root] + 12u, 4);
            flags.logic(gp[parent], 4);
        }
    }

    void classify(unsigned table, std::uint32_t table_site, std::uint32_t call_site,
                  std::uint32_t returned) {
        gp[table] = port.read(table_site, gp[esi], 4);
        gp[ecx] = gp[esi];
        const auto target = port.read(call_site, gp[table] + 0x14u, 4);
        call(call_site, returned, target);
    }

    // The special collision body may veto one child before the group masks
    // are consulted. Its real class, flag and motion queries own their ABIs.
    bool special_child_rejected() {
        gp[eax] = port.read(0x1cbeaau, gp[ebp] + 0x1cu, 4);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf) {
            gp[eax] = port.read(0x1cbec2u, gp[ebx], 4);
        } else {
            gp[ecx] = port.read(0x1cbeb2u, gp[ebp] + 0x18u, 4);
            gp[edx] = port.read(0x1cbeb5u, gp[ecx], 4);
            gp[esi] = gp[esp] + 0x20u;
            port.push(0x1cbebbu, gp[esi]);
            port.push(0x1cbebcu, gp[eax]);
            const auto target = port.read(0x1cbebdu, gp[edx] + 0x34u, 4);
            call(0x1cbebdu, 0x1cbec0u, target);
        }

        gp[esi] = gp[eax];
        classify(eax, 0x1cbec6u, 0x1cbecau, 0x1cbecdu);
        flags.minus(gp[eax], 21u, 4);
        if (flags.zf) gp[esi] = port.read(0x1cbed2u, gp[esi] + 12u, 4);

        classify(edx, 0x1cbed5u, 0x1cbed9u, 0x1cbedcu);
        flags.minus(gp[eax], 23u, 4);
        if (flags.zf) gp[esi] = port.read(0x1cbee1u, gp[esi] + 0x30u, 4);

        classify(eax, 0x1cbee4u, 0x1cbee8u, 0x1cbeebu);
        flags.minus(gp[eax], 24u, 4);
        if (flags.zf) return false;

        gp[ecx] = gp[esi];
        call(0x1cbef2u, 0x1cbef7u, 0x185020u);
        flags.logic(gp[eax] & 0xffu, 1);
        if (flags.zf) return false;

        gp[esi] = port.read(0x1cbefbu, gp[esi] + 8u, 4);
        flags.logic(gp[esi], 4);
        if (flags.zf) return false;
        flags.minus(gp[esi], 1u, 4);
        if (flags.sf != flags.of) {
            gp[eax] = 1u;
        } else {
            flags.minus(gp[esi], 16u, 4);
            gp[eax] = !flags.zf && flags.sf == flags.of ? 16u : gp[esi];
        }
        flags.minus(gp[eax], gp[esi], 4);
        if (flags.zf) return false;

        const auto user_flag = flags.logic(port.read(0x1cbf1eu, gp[esi] + 31u, 1) & 1u, 1);
        gp[ecx] = (gp[ecx] & 0xffffff00u) | user_flag;
        port.store(0x1cbf24u, gp[esp] + 0x1bu, 1, user_flag);
        if (flags.zf) return false;

        gp[ecx] = port.read(0x1cbf2au, gp[edi] + 12u, 4);
        flags.logic(gp[ecx], 4);
        gp[eax] = gp[edi];
        walk_parents(eax, ecx, 0x1cbf35u);

        flags.minus(port.read(0x1cbf3cu, gp[eax] + 24u, 4), 1u, 4);
        if (!flags.zf) return false;
        gp[eax] = port.read(0x1cbf42u, gp[eax] + 32u, 4);
        flags.logic(gp[eax], 4);
        if (flags.zf) return false;

        gp[eax] = port.read(0x1cbf49u, gp[eax] + 0x3cu, 4);
        gp[edx] = port.read(0x1cbf4cu, gp[eax], 4);
        gp[ecx] = gp[eax];
        const auto target = port.read(0x1cbf50u, gp[edx] + 0x18u, 4);
        call(0x1cbf50u, 0x1cbf53u, target);
        flags.minus(gp[eax], 7u, 4);
        return flags.zf;
    }

    void apply_group_filter() {
        gp[ecx] = port.read(0x1cbf67u, gp[ebx] + 12u, 4);
        flags.logic(gp[ecx], 4);
        gp[eax] = gp[ebx];
        walk_parents(eax, ecx, 0x1cbf72u);

        // The second parent is prefetched before the first group's read.
        gp[ecx] = port.read(0x1cbf79u, gp[edi] + 12u, 4);
        flags.logic(gp[ecx], 4);
        gp[edx] = port.read(0x1cbf7eu, gp[eax] + 28u, 4);
        gp[eax] = gp[edi];
        walk_parents(eax, ecx, 0x1cbf87u);

        gp[eax] = port.read(0x1cbf8eu, gp[eax] + 28u, 4);
        gp[esi] = port.read(0x1cbf91u, gp[ebp] + 8u, 4);
        gp[ecx] = port.read(0x1cbf94u, gp[esp] + 0x1cu, 4);
        port.push(0x1cbf98u, gp[edx]);
        port.push(0x1cbf99u, gp[eax]);
        port.push(0x1cbf9au, gp[esi]);
        gp[ecx] = flags.plus(gp[ecx], 0xfffffff4u, 4);
        port.push(0x1cbf9eu, 0x1cbfa3u);
        port.call_child(filter_groups, group_filter);
    }

    void finish(std::uint32_t first_pop, std::uint32_t returned, bool group_path) {
        gp[edi] = port.pop(first_pop);
        if (group_path) gp[eax] = gp[esi];
        gp[esi] = port.pop(returned - 5u);
        gp[ebx] = port.pop(returned - 4u);
        gp[esp] = gp[ebp];
        gp[ebp] = port.pop(returned - 1u);
        port.ret(returned, 24u);
        port.publish(returned);
    }

    void run() {
        port.push(0x1cbe70u, gp[ebp]);
        gp[ebp] = gp[esp];
        gp[esp] = flags.logic(gp[esp] & 0xfffffff0u, 4);
        gp[esp] = flags.minus(gp[esp], 0x114u, 4);
        port.push(0x1cbe7cu, gp[ebx]);
        port.push(0x1cbe7du, gp[esi]);
        port.push(0x1cbe7eu, gp[edi]);

        gp[edi] = port.read(0x1cbe7fu, gp[ebp] + 0x10u, 4);
        gp[eax] = port.read(0x1cbe82u, gp[edi] + 12u, 4);
        flags.logic(gp[eax], 4);
        port.store(0x1cbe87u, gp[esp] + 0x1cu, 4, gp[ecx]);
        gp[ecx] = gp[edi];
        walk_parents(ecx, eax, 0x1cbe92u);

        gp[eax] = port.read(0x1cbe99u, 0x48b860u, 4);
        flags.minus(gp[eax], port.read(0x1cbe9eu, gp[ecx] + 32u, 4), 4);
        gp[ebx] = port.read(0x1cbea1u, gp[ebp] + 0x14u, 4);

        if (flags.zf && special_child_rejected()) {
            gp[eax] = port.read(0x1cbf58u, gp[ebp] + 8u, 4);
            port.store(0x1cbf5bu, gp[eax], 1, 0u);
            finish(0x1cbf5eu, 0x1cbf64u, false);
            return;
        }

        apply_group_filter();
        finish(0x1cbfa3u, 0x1cbfabu, true);
    }
};

template<class State, class Reference, class Dispatch, class GroupFilter>
bool bridge(State &state, Reference reference, Dispatch dispatch, GroupFilter group_filter) {
    static const bool reference_mode = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_COLLECTION_FILTER_WORLD");
        return value && *value;
    }();
    if (reference_mode || state.active_mmx) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication)) return false;
    static VerifySummary summary;
    world::execute(state, publication, summary.calls, reference,
        [&](world::Port<State> &port) { Frame{port, dispatch, group_filter}.run(); });
    return true;
}
} // namespace halo2::native_collection_filter_world_detail
