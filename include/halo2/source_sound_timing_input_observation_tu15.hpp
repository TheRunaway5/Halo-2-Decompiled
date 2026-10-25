#pragma once
#include "halo2/mcc_sound_timing_reference.hpp"
#include <functional>
#include <map>
#include <stdexcept>

namespace halo2 {
// Read-only observation inside the authenticated metadata/resource lease.
// These are original arithmetic operands, never a duration or gameplay result.
using SourceSoundTimingInputObserverTu15=std::function<void(const MccSoundTimingEntry &,
    const MccSoundOriginalTimingOperands &,const MccSoundTimingBinding &)>;
inline thread_local std::map<const SourcePhysicsContactMemory *,SourceSoundTimingInputObserverTu15>
    source_sound_timing_input_observers_tu15;
class SourceSoundTimingInputObserverBindingTu15 {
    const SourcePhysicsContactMemory *memory_;
public:
    SourceSoundTimingInputObserverBindingTu15(const SourcePhysicsContactMemory &memory,
        SourceSoundTimingInputObserverTu15 observer):memory_(&memory) {
        if(!source_sound_timing_input_observers_tu15.emplace(memory_,std::move(observer)).second)
            throw std::runtime_error("Sound timing input diagnostics already bound");
    }
    ~SourceSoundTimingInputObserverBindingTu15(){source_sound_timing_input_observers_tu15.erase(memory_);}
    SourceSoundTimingInputObserverBindingTu15(const SourceSoundTimingInputObserverBindingTu15 &)=delete;
    SourceSoundTimingInputObserverBindingTu15 &operator=(const SourceSoundTimingInputObserverBindingTu15 &)=delete;
};
inline void observe_source_sound_timing_input_tu15(const SourcePhysicsContactMemory &memory,
    const MccSoundTimingEntry &entry,const MccSoundOriginalTimingOperands &original,
    const MccSoundTimingBinding &binding) {
    const auto at=source_sound_timing_input_observers_tu15.find(&memory);
    if(at!=source_sound_timing_input_observers_tu15.end()) {
        const auto callback=at->second;callback(entry,original,binding);
    }
}
}
