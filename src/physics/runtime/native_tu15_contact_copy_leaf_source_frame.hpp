#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_contact_agent_dispatch.hpp"
#include "halo2/native_tu15_contact_copy_leaf_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"

namespace halo2::native_contact_copy_leaf_detail {
// Private transport for the complete retained source. Unlike the raw CPU
// frame, this delegates v/vw to their actual provider and observer protocol.
// The exact owning body selects the format; a callback's mere presence does
// not imply that its direct delivery can be replaced with a cursor write.
enum class SourceFormat { direct, literal };

template<SourceFormat Format, class State>
void source_instruction(State& s, std::uint32_t pc) {
    if constexpr (Format == SourceFormat::literal) {
        s.instruction_literal(pc);
    } else if (s.services.instruction) {
        s.services.instruction(pc);
    }
}

template<class State>
std::uint32_t source_pc(const State& s, std::uint32_t canonical, std::uint32_t actual) {
    if constexpr (requires { s.selected_observation_pcs; }) {
        if (s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15)
            return actual;
    }
    return canonical;
}

struct CopySourceSites { unsigned field; std::uint32_t read, write; };
inline constexpr std::array copy_source_sites{
    CopySourceSites{0x00,0x300106u,0x300109u}, CopySourceSites{0x10,0x30010cu,0x300110u},
    CopySourceSites{0x20,0x300114u,0x300117u}, CopySourceSites{0x24,0x30011au,0x30011du},
    CopySourceSites{0x30,0x300120u,0x300124u}, CopySourceSites{0x40,0x300128u,0x30012cu},
    CopySourceSites{0x50,0x300130u,0x300134u}, CopySourceSites{0x60,0x300138u,0x30013cu},
    CopySourceSites{0x70,0x300140u,0x300143u}, CopySourceSites{0x74,0x300146u,0x300149u},
    CopySourceSites{0x78,0x30014cu,0x30014fu}, CopySourceSites{0x7c,0x300152u,0x300155u},
    CopySourceSites{0x7d,0x300158u,0x30015bu},
};

template<SourceFormat Format, class State> class CopySourceFrame {
    State& s_;
    std::uint32_t pc(std::uint32_t canonical) const {
        return source_pc(s_, canonical, canonical + 0x3aa0u);
    }
    void at(std::uint32_t canonical) { source_instruction<Format>(s_, pc(canonical)); }
    static const CopySourceSites& sites(unsigned field) {
        for (const auto& entry : copy_source_sites) if (entry.field == field) return entry;
        throw std::logic_error("Foreign contact-copy source field");
    }
    void scalar_store(unsigned field, unsigned width, unsigned reg) {
        const auto canonical = sites(field).write;
        at(canonical);
        // Acquire store inputs after the actual instruction callback. The
        // observer fast path and the post-write observer are owned by w.
        physics_step_detail::w(s_.memory, s_.observe, pc(canonical), s_.gp[0] + field,
            width, s_.reg(reg, width, 0));
    }
public:
    explicit CopySourceFrame(State& s) : s_(s) {}
    void begin() {
        at(0x300100u); s_.gp[0] = s_.gp[1];
        at(0x300102u); s_.gp[1] = physics_step_detail::r(s_.memory, s_.gp[4] + 4u);
    }
    NativeTu15ContactRecordVector vector(NativeTu15ContactRecord, unsigned field) {
        at(sites(field).read);
        s_.xmm[0] = physics_step_detail::v(s_.memory, s_.gp[1] + field);
        return std::bit_cast<NativeTu15ContactRecordVector>(s_.xmm[0]);
    }
    std::uint32_t word(NativeTu15ContactRecord, unsigned field) {
        at(sites(field).read);
        return s_.gp[2] = physics_step_detail::r(s_.memory, s_.gp[1] + field);
    }
    std::uint8_t byte(NativeTu15ContactRecord, unsigned field) {
        at(sites(field).read);
        const auto value = physics_step_detail::r(s_.memory, s_.gp[1] + field, 1);
        s_.set(field == 0x7c ? 2u : 1u, value, 1, 0);
        return static_cast<std::uint8_t>(value);
    }
    void vector(NativeTu15ContactRecord, unsigned field, const NativeTu15ContactRecordVector&,
        NativeTu15ContactRecordPublication) {
        const auto canonical = sites(field).write;
        at(canonical);
        // vw snapshots both halves and their address before the first write;
        // its two w calls each choose the live observer fast path.
        physics_step_detail::vw(s_.memory, s_.observe, pc(canonical), s_.gp[0] + field, s_.xmm[0]);
    }
    void word(NativeTu15ContactRecord, unsigned field, std::uint32_t, NativeTu15ContactRecordPublication) {
        scalar_store(field, 4, 2);
    }
    void byte(NativeTu15ContactRecord, unsigned field, std::uint8_t, NativeTu15ContactRecordPublication) {
        scalar_store(field, 1, field == 0x7c ? 2u : 1u);
    }
    void finish() { at(0x30015eu); s_.pop(); s_.gp[4] += 4u; }
};

template<SourceFormat Format, class State> void run_copy_source_private(State& s) {
    CopySourceFrame<Format, State> frame{s};
    frame.begin();
    copy(frame, NativeTu15ContactRecord{s.gp[0]}, NativeTu15ContactRecord{s.gp[1]});
    frame.finish();
}

template<SourceFormat Format, class State, class Resolve> void run_invalid_source_private(State& s, Resolve resolve) {
    source_instruction<Format>(s, source_pc(s, 0x307fe0u, 0x30bad0u));
    s.gp[0] = s.image == SourcePhysicsContactImage::TitleUpdate15 ?
        native_tu15_invalid_contact_agent().identity : resolve(0x487ac4u, s.image);
    source_instruction<Format>(s, source_pc(s, 0x307fe5u, 0x30bad5u));
    s.pop();
}
} // namespace halo2::native_contact_copy_leaf_detail
#endif
