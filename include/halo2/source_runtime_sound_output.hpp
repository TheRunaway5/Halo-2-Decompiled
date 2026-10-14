#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/sdl_sound_platform.hpp"
namespace halo2 {
// Native output-resource service for the reached 387877 creation request.
// Its identity is the actual SDL device ID, consumed only by native adapters;
// it is never represented as an Xbox COM object/vtable. Xbox device/voice
// constructors, capabilities, DSP and saved settings remain separate owners.
class SourceRuntimeSoundOutput {
public:
    SourceRuntimeSoundOutput(SourceRuntimeMemory &, SdlSoundPlatformConfiguration);
    ~SourceRuntimeSoundOutput();
    SourceRuntimeSoundOutput(const SourceRuntimeSoundOutput &)=delete;
    SourceRuntimeSoundOutput &operator=(const SourceRuntimeSoundOutput &)=delete;
    bool dispatch(SourcePhysicsContactMemory &,const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &);
    std::uint32_t device_id() const noexcept;
    bool quiescent() const noexcept;
    void require_quiescent() const;
    // Native exception cleanup only. This does not synthesize a source release
    // return or implement the unrecovered Xbox singleton/AddRef domain.
    void close();
    class CallScope {
    public:
        explicit CallScope(SourceRuntimeSoundOutput *);
        ~CallScope();
        CallScope(const CallScope &)=delete;
        CallScope &operator=(const CallScope &)=delete;
    private:
        SourceRuntimeSoundOutput *owner_;
        bool existing_;
        int exceptions_;
    };
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void abort_new_device() noexcept;
};
}
