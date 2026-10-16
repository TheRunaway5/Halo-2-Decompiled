#pragma once
#include "halo2/native_tu15_sound_gain_fanout.hpp"
#include "halo2/native_tu15_sound_mono_distortion.hpp"
#include "halo2/native_tu15_sound_two_product_accumulator.hpp"
#include "halo2/native_tu15_sound_chorus.hpp"
#include "halo2/native_tu15_sound_crosstalk.hpp"
#include "halo2/native_tu15_sound_controller.hpp"
#include "halo2/native_tu15_sound_ring_effect.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
namespace halo2::tu15_sound_graph_storage {
using Word24=tu15_sound::Word24;
using Block=std::span<Word24,32>;
struct Modules {
    tu15_sound_gain_fanout::Parameters fanout_parameters;
    tu15_sound_gain_fanout::State fanout_state;
    tu15_sound_mono_distortion::Parameters distortion_parameters;
    tu15_sound_mono_distortion::State distortion_state;
    std::array<tu15_sound_two_product_accumulator::Parameters,2> accumulator_parameters;
    std::array<tu15_sound_two_product_accumulator::State,2> accumulator_states;
    std::array<tu15_sound_chorus::Parameters,4> chorus_parameters;
    std::array<tu15_sound_chorus::State,4> chorus_states;
    tu15_sound_crosstalk::State crosstalk_state;
    std::array<tu15_sound_controller_arithmetic::Parameters,4> controller_parameters;
    std::array<tu15_sound_controller_arithmetic::State,4> controller_states;
};
struct ReverbAdapter {
    tu15_sound_ring_effect::State state;
    std::array<std::uint32_t,32> input{};
    std::array<std::array<std::uint32_t,32>,4> outputs{};
};
// Explicit named retained contents for the original no-allocation/reuse branch.
// The constructor copies these values; it never caches caller seed spans.
struct RetainedScratchSeed {
    std::array<std::array<std::span<const std::uint32_t>,14>,2> reverb_histories;
    std::array<std::span<const std::byte>,2> reverb_unused_tails;
    std::array<std::span<const Word24>,4> controller_sinks;
    std::span<const std::byte> final_unused_tail;
};
// Histories and sinks contain logical24-bit values, not a physical DMA byte
// image. Byte consumers need an explicit format adapter; they cannot borrow
// these arrays as packed DMA buffers. Installation and frame readiness are separate.
class GraphStorage final {
public:
    // Literal installed state before the first effect call. Successful fresh
    // 38DE81->38796C(flag1) zeros the full actual title scratch allocation.
    static std::unique_ptr<GraphStorage> fresh_literal();
    static std::unique_ptr<GraphStorage> retained_scratch_literal(const RetainedScratchSeed&);
    GraphStorage(const GraphStorage&)=delete;
    GraphStorage& operator=(const GraphStorage&)=delete;
    GraphStorage(GraphStorage&&)=delete;
    GraphStorage& operator=(GraphStorage&&)=delete;
    Modules& effects() noexcept{return modules_;}
    const Modules& effects()const noexcept{return modules_;}
    std::span<Word24,6> unresolved_prefix() noexcept;
    std::span<const Word24,32> fanout_input()const noexcept;
    Block mixbin(unsigned);
    Block fanout_destination(unsigned);
    std::array<Block,8> fanout_routes();
    std::span<Word24,544> workspace() noexcept{return workspace_;}
    std::span<Word24,401> chorus_history(unsigned);
    const tu15_sound_chorus::Bindings& chorus_bindings(unsigned)const;
    Block reverb_common_input(unsigned);
    Block reverb_common_output(unsigned,unsigned);
    ReverbAdapter& reverb(unsigned);
    const ReverbAdapter& reverb(unsigned)const;
    tu15_sound_ring_effect::Workspace& raw_reverb_workspace() noexcept{return raw_workspace_;}
    std::span<const std::byte,51008> reverb_unused_tail(unsigned)const;
    std::span<Word24,640> controller_sink(unsigned);
    tu15_sound_controller::Sink& controller_sink_owner(unsigned);
    const tu15_sound_controller::Sink& controller_sink_owner(unsigned)const;
    std::uint32_t controller_sink_byte_cursor(unsigned)const;
    std::span<const std::byte,3904> final_unused_tail()const noexcept{return final_tail_;}
    std::size_t actual_title_scratch_bytes()const noexcept;
    // Value adapters only. They certify no processing, status, ACK or receipt.
    // Both reverb States share raw_workspace_ without an intervening reload.
    void copy_common_to_reverb_buffers();
    void copy_reverb_buffers_to_common();
private:
    GraphStorage();
    // One array object makes the prefix+26mix-word input span valid C++.
    std::array<Word24,6+32*32> mix_with_prefix_{};
    std::array<Word24,4*32> fanout_four_{};
    std::array<Word24,544> workspace_{};
    std::array<Word24,1601> chorus_history_{};
    Modules modules_{};
    std::array<tu15_sound_chorus::Bindings,4> chorus_bindings_;
    std::array<ReverbAdapter,2> reverbs_{};
    tu15_sound_ring_effect::Workspace raw_workspace_{};
    std::array<std::array<std::byte,51008>,2> reverb_tails_{};
    std::array<tu15_sound_controller::Sink,4> controller_sinks_{};
    std::array<std::byte,3904> final_tail_{};
    static void validate_seed(const RetainedScratchSeed&);
};
}
