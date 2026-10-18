#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <memory>
namespace halo2 {
// Native ownership for the authenticated preloaded DSPImage resource. The
// admitted memory bank stays in place and outlives this owner. These are real
// native resource references, not Xbox kernel/header/page-counter emulation.
// Only the paired DSPImage request domain reached by the sound callers is
// supported. Other sections and an unmatched unload remain unavailable.
class SourceRuntimeDspSection {
public:
    enum class LoadPolicy { eager, on_demand };
    SourceRuntimeDspSection(SourceRuntimeMemory &,const std::filesystem::path &);
    // The normal native PCM path does not request a DSP program. Retain the
    // optional reference adapter without opening its image until a real
    // section request arrives. No DSP result or readiness is supplied here.
    SourceRuntimeDspSection(SourceRuntimeMemory &,const std::filesystem::path &,LoadPolicy);
    ~SourceRuntimeDspSection();
    SourceRuntimeDspSection(const SourceRuntimeDspSection &)=delete;
    SourceRuntimeDspSection &operator=(const SourceRuntimeDspSection &)=delete;
    std::uint32_t retain();
    void release();
    std::span<const std::byte> retained_bytes() const;
    // Zero before an on-demand owner receives its first completed request.
    std::uint32_t address() const noexcept;
    std::size_t active_references() const noexcept;
    std::size_t resident_references() const noexcept;
    void require_quiescent() const;
    // Release new native references if their enclosing source call throws.
    // A real source false/true return is never supplied by this cleanup.
    class CallScope {
    public:
        explicit CallScope(SourceRuntimeDspSection *);
        ~CallScope();
        CallScope(const CallScope &)=delete;
        CallScope &operator=(const CallScope &)=delete;
    private:
        SourceRuntimeDspSection *owner_;
        std::size_t references_;
        int exceptions_;
    };
    // Explicit native adaptation of2DF851/2DF86C. Returns whether this is the
    // supported request. Preserves ABI state except genuine EAX result and
    // stdcall stack consumption; original kernel volatile clobbers, last-error
    // state, shared-page counters and VM behavior are not asserted here.
    bool dispatch(SourcePhysicsContactMemory &,const SourcePhysicsContactRuntimeCall &,
        SourcePhysicsWorldRuntimeAbi &);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void discard_new_references(std::size_t) noexcept;
};
}
