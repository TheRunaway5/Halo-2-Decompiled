#pragma once
#include "halo2/display_settings.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <memory>
#include <thread>
namespace halo2 {
// Exactly386D97..386DA9's explicit speaker override admission. Source-defined
// preference bits:0 stereo,1 mono,2 surround;10000 AC3 and20000 DTS. This
// predicate alone never proves any native route/encoder is implemented.
constexpr bool native_tu15_valid_sdk_audio_preference(std::uint32_t flags) noexcept {
    return (flags&0xffffu)<=2u&&(flags&0xfffc0000u)==0;
}
struct NativeTu15PersistedAudioPreference {
    unsigned speakers{};
    std::filesystem::path configuration;
    bool application_default{};
};
// Same pc_speakers0..4 field and application default as MenuState's native
// DisplaySettings. No console EEPROM, AV pack or hardware value is created.
NativeTu15PersistedAudioPreference native_tu15_load_audio_preference(const std::filesystem::path&);

// Owns a native preference together with the real running callback output resource that
// validates it. The current native mixer supports a two-channel PCM route;
// unsupported discrete/compressed modes stop before SDK preference publication.
// Original386D4A consumes its explicit override and constructs all SDK fields;
// this owner supplies no SDK object, refcount, capability, DSP or ready result.
class NativeTu15AudioPreference final {
public:
    NativeTu15AudioPreference(SourceRuntimeMemory&,std::shared_ptr<SdlSoundPlatform>,
        NativeTu15PersistedAudioPreference);
    ~NativeTu15AudioPreference();
    NativeTu15AudioPreference(const NativeTu15AudioPreference&)=delete;
    NativeTu15AudioPreference& operator=(const NativeTu15AudioPreference&)=delete;
    std::uint32_t flags() const;
    std::uint32_t output_device() const;
    const NativeTu15PersistedAudioPreference& preference()const noexcept{return preference_;}
    // Real SDK input390BE8 startsFFFFFFFF. Apply once before construction;
    // after all SDK borrowers retire, withdraw the owned input explicitly.
    void publish();
    void withdraw();
    // Foreign diagnostics fail before reading mutable publication state.
    bool published()const noexcept{return std::this_thread::get_id()==thread_&&published_;}
    void retire(); // Requires all SDK users to have retired first.
    bool quiescent()const noexcept;
    void require_quiescent()const;
    SdlSoundPlatform& output()const;
private:
    void require_live()const;
    SourceRuntimeMemory& memory_;
    std::shared_ptr<SdlSoundPlatform> output_;
    NativeTu15PersistedAudioPreference preference_;
    std::thread::id thread_=std::this_thread::get_id();
    std::uint32_t device_{};
    bool published_{};
};
}
