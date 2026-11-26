#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_axis_byte_nodes_detail {
enum class Continuation {
    retained, write_first, adjust_second, load_first, write_second, spill_first, load_second,
    spill_second, load_low, combine_low, load_high, combine_high, offsets
};

struct AxisNode {
    bool decreasing;
    std::array<unsigned,4> axis_offsets;
    std::array<std::uint32_t,5> sites; // two integer staging stores, two spills, and transfer
};
inline constexpr std::array nodes{
    AxisNode{false,{8,4,24,20},{0x30aab8u,0x30aac2u,0x30aac6u,0x30aaceu,0x30aadeu}},
    AxisNode{true,{4,8,20,24},{0x30aaf2u,0x30ab01u,0x30ab05u,0x30ab0du,0x30ab1du}},
    AxisNode{false,{8,0,24,16},{0x30ab2cu,0x30ab36u,0x30ab3au,0x30ab42u,0x30ab51u}},
    AxisNode{true,{0,8,16,24},{0x30ab65u,0x30ab74u,0x30ab78u,0x30ab80u,0x30ab8fu}},
    AxisNode{false,{4,0,20,16},{0x30ab9eu,0x30aba8u,0x30abacu,0x30abb4u,0x30abc3u}},
    AxisNode{true,{0,4,16,20},{0x30abd7u,0x30abe6u,0x30abeau,0x30abf2u,0x30ac01u}}
};

inline bool reference_axis_byte_nodes() noexcept {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_MOPP_RAY_AXIS_BYTE_NODES");
        return (value && *value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_axis_byte_provider() noexcept {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_MOPP_RAY_AXIS_BYTE_PROVIDER");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
class AxisByteNodes {
    State &state_;
    const typename State::InstructionCursor *cursor_;
    bool provider_store_{};

    void publish(std::uint32_t pc) const {
        if (cursor_)
            *cursor_->position = pc;
    }
    template<class Value>
    bool read(std::uint32_t address, Value &value) const {
        if (const auto *page = state_.memory.direct_page(address,sizeof(Value))) {
            std::memcpy(&value,page->bytes+(address&4095u),sizeof(Value));
            return true;
        }
        const auto view = state_.memory.try_read_view(address,sizeof(Value));
        if (!view)
            return false;
        std::memcpy(&value,view->data(),sizeof(Value));
        return true;
    }
    bool store(std::uint32_t pc,unsigned offset,std::uint32_t bits) {
        publish(pc);
        return state_.memory.try_callback_free_write(state_.gp[4]+offset,std::as_bytes(std::span{&bits,1}));
    }
    void provider_write(std::uint32_t pc,unsigned offset,std::uint32_t bits) {
        provider_store_ = true;
        physics_step_detail::w(state_.memory,state_.observe,pc,state_.gp[4]+offset,4u,bits);
    }
    bool resume_after_store(unsigned minimum_depth,unsigned maximum_depth) const {
        return native_mopp_ray_scale_tail_detail::provider_continuation(state_,cursor_) &&
            state_.fp.count >= minimum_depth && state_.fp.count <= maximum_depth &&
            native_raw_x87_detail::supported(state_);
    }
    std::optional<Continuation> stage_integer(std::uint32_t pc,std::uint32_t bits,
        Continuation before,Continuation after,unsigned minimum_depth,unsigned maximum_depth) {
        if (store(pc,0x14u,bits))
            return {};
        if (reference_axis_byte_provider())
            return before;
        provider_write(pc,0x14u,bits);
        if (!resume_after_store(minimum_depth,maximum_depth))
            return after;
        return {};
    }
    template<class Spill>
    std::optional<Continuation> spill(std::uint32_t pc,unsigned offset,std::uint32_t bits,
        Continuation before,Continuation after,Spill spill_word) {
        const auto integer = std::bit_cast<std::int32_t>(bits);
        // These bytes encode twice an unsigned octet, optionally biased by
        // -255. Every such FILD/FSTP is exact, independent of x87 rounding.
        if (!provider_store_ && (integer < -255 || integer > 510))
            return before;
        // A prior provider may have replaced the live lane or its controls.
        // Keep that actual lane through the store and its possible fault.
        publish(pc);
        auto word = provider_store_ ? spill_word(state_.fp.st(0),state_.fp.status) :
            std::bit_cast<std::uint32_t>(static_cast<float>(integer));
        if (store(pc,offset,word)) {
            state_.fp.pop();
            return {};
        }
        if (reference_axis_byte_provider())
            return before;
        if (!provider_store_)
            word = spill_word(state_.fp.st(0),state_.fp.status);
        provider_write(pc,offset,word);
        state_.fp.pop();
        if (!resume_after_store(0u,6u))
            return after;
        return {};
    }

public:
    AxisByteNodes(State &state,const typename State::InstructionCursor *cursor)
        : state_(state), cursor_(cursor) {}

    template<unsigned Index,class Spill>
    Continuation run(Spill spill_word) {
        using namespace physics_step_detail;
        constexpr auto node = nodes[Index];
        auto &g = state_.gp;
        std::array<std::uint8_t,2> encoded;
        if (!read(g[6]+1u,encoded))
            return Continuation::retained;

        g[0] = encoded[1];
        if constexpr (node.decreasing) {
            g[1] = encoded[0];
            g[2] = g[1]*2u - 255u;
        } else {
            g[2] = state_.flags.shift(encoded[0],1u,4);
        }
        if (const auto next = stage_integer(node.sites[0],g[2],
            Continuation::write_first,Continuation::adjust_second,0u,6u))return *next;

        if constexpr (node.decreasing)
            g[1] = g[0]*2u - 255u;
        else
            g[0] = state_.flags.shift(g[0],1u,4);

        std::uint32_t first{}, second{};
        if (!read(g[4]+0x14u,first))
            return Continuation::load_first;
        state_.fp.push(static_cast<long double>(si(first)));
        if (const auto next = stage_integer(node.sites[1],g[node.decreasing?1:0],
            Continuation::write_second,Continuation::spill_first,1u,7u))return *next;
        if (const auto next = spill(node.sites[2],0x20u,first,
            Continuation::spill_first,Continuation::load_second,spill_word))return *next;

        if (!read(g[4]+0x14u,second))
            return Continuation::load_second;
        state_.fp.push(static_cast<long double>(si(second)));
        if (const auto next = spill(node.sites[3],0x1cu,second,
            Continuation::spill_second,Continuation::load_low,spill_word))return *next;

        std::uint32_t value{};
        if (!read(g[7]+node.axis_offsets[0],value))
            return Continuation::load_low;
        state_.fp.push(ext(f(value)));
        if (!read(g[7]+node.axis_offsets[1],value))
            return Continuation::combine_low;
        if constexpr (node.decreasing)
            state_.fp.st(0) = sub(state_.fp.st(0),ext(f(value)));
        else
            state_.fp.st(0) = add(state_.fp.st(0),ext(f(value)));
        if (!read(g[7]+node.axis_offsets[2],value))
            return Continuation::load_high;
        state_.fp.push(ext(f(value)));
        if (!read(g[7]+node.axis_offsets[3],value))
            return Continuation::combine_high;
        if constexpr (node.decreasing)
            state_.fp.st(0) = sub(state_.fp.st(0),ext(f(value)));
        else
            state_.fp.st(0) = add(state_.fp.st(0),ext(f(value)));
        publish(node.sites[4]);
        return Continuation::offsets;
    }
};
#endif

template<unsigned Index,class State,class Spill>
Continuation prepare_axis_byte_node(State &state,Spill spill_word) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr (std::endian::native != std::endian::little)
        return Continuation::retained;
    if (reference_axis_byte_nodes() || state.image != SourcePhysicsContactImage::TitleUpdate15 ||
        state.observe || state.active_mmx || state.fp.count > 6u)
        return Continuation::retained;
    const auto *cursor = native_mopp_ray_scale_tail_detail::admitted_cursor(state);
    if (state.services.instruction ? !cursor : state.instruction_cursor != nullptr)
        return Continuation::retained;
    return AxisByteNodes<State>{state,cursor}.template run<Index>(spill_word);
#else
    (void)state;(void)spill_word;
    return Continuation::retained;
#endif
}
}
