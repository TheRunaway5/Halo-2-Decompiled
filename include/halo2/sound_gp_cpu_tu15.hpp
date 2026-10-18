#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace halo2 {
enum class SoundGpSpaceTu15 {x,y,program};
struct SoundGpCpuInputTu15 {
  // Explicit caller state. The DSP56300 architectural reset and MCPX reset
  // are not interchangeable; this API supplies no guessed MCPX registers.
  std::array<std::uint32_t,64> registers;
  std::function<std::uint32_t(std::uint32_t)> read_peripheral;
  std::function<void(std::uint32_t,std::uint32_t)> write_peripheral;
};
// Read-only instruction-core state. The 64-slot bank retains the pinned
// core's internal bookkeeping slots too; it is not a physical MCPX register
// dump. This omits host scheduling/JIT fields and external peripherals and
// is not a restorable whole-device checkpoint.
struct SoundGpExecutionStateTu15 {
  std::uint32_t pc{},cycles{};
  std::array<std::uint32_t,64> registers{};
  std::array<std::array<std::uint32_t,16>,2> stack{};
  bool repeating{},pc_on_repeat{};
  bool operator==(const SoundGpExecutionStateTu15&)const=default;
};
// Persistent native instruction execution with the GP's actual X/mix aliases,
// Y and P extents. Peripheral callbacks must have a real owner; this class
// supplies no DMA, timer, IRQ, idle/readiness, FIFO or source-call result.
// Single-thread ownership. Callbacks may access these RAM methods but must
// not bootstrap/step/re-enter execution. A callback failure faults the owner.
class SoundGpCpuTu15 {
public:
  explicit SoundGpCpuTu15(SoundGpCpuInputTu15);
  ~SoundGpCpuTu15();
  SoundGpCpuTu15(const SoundGpCpuTu15&)=delete;
  SoundGpCpuTu15&operator=(const SoundGpCpuTu15&)=delete;
  // Actual boot-ROM transfer input: 0x800 little-endian decoded words. No
  // effects or state are preloaded independently of those supplied bytes.
  void bootstrap(std::span<const std::uint32_t>);
  void step();
  struct RunResult {std::uint32_t cycles;bool stopped;};
  // Compiled blocks require the complete authenticated original GP program.
  // max_cycles is a host scheduling slice, checked at core block boundaries;
  // the actual cycle counter is retained. stopped is a host yield, not a
  // device-ready result. The peripheral owner must check its real condition.
  RunResult run_original_gp(std::uint32_t max_cycles);
  // For executing callbacks. The original idle MOVEP yields at its next PC.
  // Single stepping already returns after that instruction. No guest flag,
  // register, stack, source return or device status is written by this API.
  void request_stop();
  std::uint32_t pc() const;
  std::uint32_t cycles() const; // Actual core counter, not hardware wall time.
  std::array<std::uint32_t,64> registers() const;
  SoundGpExecutionStateTu15 execution_state() const;
  std::uint32_t read(SoundGpSpaceTu15,std::uint32_t) const;
  void write(SoundGpSpaceTu15,std::uint32_t,std::uint32_t);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
