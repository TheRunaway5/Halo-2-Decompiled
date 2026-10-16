#pragma once
#include "halo2/particle_draw.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <thread>

namespace halo2 {
// Native CPU emission storage for original4012B0/401410. This owns actual
// command/shadow allocations, not an Xbox device or a fabricated SDK return.
// The caller supplies its complete inherited native shader bank. Other source
// accesses retain their real world owner; exhaustion requires a separate
// command submission owner and must stop before404D00 is serviced.
class SourceParticleConstantStorageTu15 final : public SourcePhysicsContactMemory {
public:
    SourceParticleConstantStorageTu15();
    void begin(SourcePhysicsContactMemory &,std::span<const ShaderVector,192>);
    void read(std::uint32_t,std::span<std::byte>) const override;
    void write(std::uint32_t,std::span<const std::byte>) override;
    std::span<const std::byte> commands() const;
    // Decode the actual emitted methods into the native consumer's logical
    // c0..191 bank. Validate the whole stream before publishing any output.
    void consume(std::span<ShaderVector,192>) const;
    void visit_committed(const std::function<void(std::uint32_t,
        std::span<const std::byte>)> &) const;
private:
    friend class SourceParticleGraphicsBindingTu15;
    void consume_packet(std::span<const std::byte>,std::span<ShaderVector,192>) const;
    void check() const;
    bool owns(std::uint32_t,std::size_t) const;
    void reject_partial_ownership(std::uint32_t,std::size_t) const;
    SourceRuntimeMemory storage_;
    SourcePhysicsContactMemory *source_{};
    std::array<ShaderVector,192> inherited_{};
    const std::thread::id thread_=std::this_thread::get_id();
};
}
