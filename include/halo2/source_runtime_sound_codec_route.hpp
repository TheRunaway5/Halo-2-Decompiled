#pragma once
#include "halo2/source_runtime_sound_reset_resources.hpp"
#include <array>
#include <thread>

namespace halo2 {
// Native functional ABI adaptation of the hardware-only38ED2C/38ECFF
// leaves. This explicitly selected WM9709 reference profile owns real SDL
// S16 stereo48k -> float stereo48k converters on the retained control worker.
// It does not emulate physical AC-link/PCR/CAS or advertise a source codec,
// channel, interrupt, DSP program, producer or factory as ready.
class SourceRuntimeSoundCodecRoute final {
    struct CpuMemory;
    struct Pipe;
public:
    enum class Profile { XboxWm9709Reference };
    enum class Kind { Analog, Spdif };
    struct Command { std::uint32_t index{},value{}; bool ignored_by_reference_profile{}; };
    struct PipeState { Kind kind{}; NativeRuntimeAddress native_converter{}; unsigned queued_float_bytes{}; std::uint64_t completed_resets{}; };
    struct State {
        Profile profile{};
        std::uint32_t source_controller{},output_device{};
        NativeRuntimeAddress native_controller{};
        std::uint64_t control_generation{},completed_transactions{};
        std::vector<PipeState> pipes;
        std::vector<Command> commands;
        bool command_acquired{},faulted{},exclusive_controller_claimed{},source_codec_ready{},source_channel_ready{},producer_ready{};
    };
    SourceRuntimeSoundCodecRoute(SourceRuntimeMemory&,std::shared_ptr<SdlSoundPlatform>,
        SourceRuntimeSoundResetResources::ControlLease,Profile);
    ~SourceRuntimeSoundCodecRoute();
    SourceRuntimeSoundCodecRoute(const SourceRuntimeSoundCodecRoute&)=delete;
    SourceRuntimeSoundCodecRoute& operator=(const SourceRuntimeSoundCodecRoute&)=delete;
    void require_bank(const SourceRuntimeMemory&)const;
    void require_control(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&)const;
    // Whole unchanged source constructor. Only declared38E484 or38F45F
    // callers are admitted; unknown native requests remain explicit.
    void run_cpu(std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Bounded component proof/service of original38ED90 command semantics.
    // Only while the whole constructor is suspended at its unowned IRQ6
    // request, using the original38F503/38F519 command frames. This does not
    // resume/complete the constructor or supply the missing interrupt route.
    void run_command_component(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Creating-executor synthetic PCM probe, not a source DSP producer. The
    // real converter's queue must be empty before a bounded conversion.
    std::vector<float> convert_native(Kind,std::span<const std::int16_t>);
    void queue_native_probe(Kind,std::span<const std::int16_t>);
    State diagnostics();
private:
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SdlSoundPlatform> output_;
    SourceRuntimeSoundResetResources::ControlLease control_;
    const Profile profile_;
    const std::uint64_t generation_;
    const std::uint32_t device_,controller_;
    NativeRuntimeAddress native_controller_;
    std::array<std::unique_ptr<Pipe>,2> pipes_;
    std::unique_ptr<CpuMemory> cpu_memory_;
    std::mutex command_mutex_;
    std::unique_lock<std::mutex> command_lock_;
    std::vector<Command> commands_;
    std::uint64_t completed_transactions_{};
    std::uint32_t pc_{},command_index_{},command_value_{};
    bool running_{},component_{},at_irq_frontier_{},fault_{},claimed_{};
    void require_thread()const;
    void require_live()const;
    bool create_routes();
    void write_action(std::uint32_t,unsigned,std::uint32_t);
    bool try_call(const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    void release_claim()noexcept;
    void abort()noexcept;
};
}
