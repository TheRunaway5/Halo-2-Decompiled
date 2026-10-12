#pragma once
#include "halo2/object_construction.hpp"
#include <array>

namespace halo2 {
struct SourceObjectPreparationState {std::uint32_t tag_table_slot;};
struct SourceObjectPressureState {
  std::uint32_t busy_byte,collecting_byte,last_collection_tick;
  std::uint32_t first_overflow_slot,clock_slot;
};
struct SourceHeapPressureState {
  std::uint32_t main_pool_slot,first_overflow_slot,second_overflow_slot;
};
struct SourceObjectPressureOperands {
  std::uint32_t main_usage_scale_bits,top_limit_bits,high_limit_bits;
  std::uint32_t medium_limit_bits,low_limit_bits,statistic_usage_scale_bits;
};
struct SourceObjectPressureCall {
  std::uint32_t instruction,entry,ecx{};
  bool ecx_input{},al_input{};
  std::uint8_t al{};
  std::array<std::uint32_t,3> arguments{};
  // Some original stack arguments have only a defined low byte. Masks retain
  // that ABI without inventing their uninitialized upper stack bytes.
  std::array<std::uint32_t,3> argument_masks{};
  unsigned count{};
};
struct SourceObjectPreparationPressureServices {
  std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Complete required pressure/reclaim/world owners. No default quiet
  // pressure, successful collection, constructor or physical state.
  std::function<std::uint32_t(const SourceObjectPressureCall &,
                             ObjectConstructionFloatingPointContext &)> invoke;
};
// Complete DDAC0. Actual definition fields gate the original separate SSE
// multiply/add operations on the retained request. No authored-root override.
void prepare_source_biped_request(const SourceObjectPreparationState &,
    std::uint32_t request,const SourceObjectPreparationPressureServices &,
    ObjectConstructionFloatingPointContext &,const ObjectConstructionObserver & = {});
// Complete22ECB0:16 wrapped statistic products followed by original signed
// CVTSI2SS/MULSS; returns actual XMM0 bits, not an invented heap classification.
std::uint32_t source_object_pressure_statistics(std::uint32_t actual_main_pool,
    const SourceObjectPressureOperands &,const SourceObjectPreparationPressureServices &,
    ObjectConstructionFloatingPointContext &);
// Complete148740, including its actual22ECB0 coupling and ordered thresholds.
std::uint32_t source_object_heap_pressure(const SourceHeapPressureState &,
    const SourceObjectPressureOperands &,const SourceObjectPreparationPressureServices &,
    ObjectConstructionFloatingPointContext &);
// Complete148510 and148360 direct owners. Required lower callbacks retain
// actual world rebuild/reclamation/diagnostic dependencies and outcomes.
void collect_source_object_memory(const SourceObjectPressureState &,
    const SourceObjectPreparationPressureServices &,ObjectConstructionFloatingPointContext &,
    const ObjectConstructionObserver & = {});
void make_source_object_memory_available(const SourceObjectPressureState &,
    const SourceObjectPreparationPressureServices &,ObjectConstructionFloatingPointContext &,
    const ObjectConstructionObserver & = {});
} // namespace halo2
