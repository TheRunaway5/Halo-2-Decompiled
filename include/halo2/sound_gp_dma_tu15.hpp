#pragma once
#include "halo2/sound_gp_cpu_tu15.hpp"
#include <functional>
#include <memory>

namespace halo2 {
// Format2 has conflicting external evidence. No implicit layout is selected.
// Explicit layouts are declared analysis inputs until hardware/authenticated
// producer-consumer evidence establishes the real device contract.
enum class SoundGpDmaFormat2Tu15 {unverified,declared_low24,declared_high24};
struct SoundGpDmaResourcesTu15 {
    std::function<std::uint32_t(SoundGpSpaceTu15,std::uint32_t)> read_word;
    std::function<void(SoundGpSpaceTu15,std::uint32_t,std::uint32_t)> write_word;
    std::function<void(std::uint32_t,std::span<std::byte>)> read_scratch;
    std::function<void(std::uint32_t,std::span<const std::byte>)> write_scratch;
    SoundGpDmaFormat2Tu15 format2=SoundGpDmaFormat2Tu15::unverified;
};
// Explicit caller context, not an asserted MCPX reset. The verified domain is
// the original TU1.5 program's synchronous scratch DMA commands. Other config,
// formats, FIFO, interleave, interrupt/timer/device semantics remain absent.
// Format2 remains rejected unless the caller supplies an explicit declared
// analysis layout. No layout constitutes physical hardware certification.
struct SoundGpDmaStateTu15 {
    std::uint32_t configuration,control,start,next;
    bool eol,error;
    bool operator==(const SoundGpDmaStateTu15&)const=default;
};
class SoundGpDmaTu15 {
public:
    SoundGpDmaTu15(SoundGpDmaResourcesTu15,SoundGpDmaStateTu15);
    ~SoundGpDmaTu15();
    SoundGpDmaTu15(const SoundGpDmaTu15&)=delete;
    SoundGpDmaTu15&operator=(const SoundGpDmaTu15&)=delete;
    // Actual FFFFD4..D7 source register transactions. D6 status describes
    // this native transfer owner; it is not a physical MCPX MMIO model.
    std::uint32_t read(std::uint32_t address) const;
    void write(std::uint32_t address,std::uint32_t value);
    void acknowledge_eol();
    SoundGpDmaStateTu15 state() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
