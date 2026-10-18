#pragma once
#include "halo2/source_script_sound_admission_tu15.hpp"
#include "halo2/source_script_sound_clock_tu15.hpp"
#include <memory>

namespace halo2 {
class Cache;
class PhysicsLiveWorldTu15;
// Producer-owned queries/metadata only. The supplied shared_ptr must retain
// the actual World (an alias from its real campaign/prepared owner is valid).
// No Audio, admission, source voice, RNG or muted-playback policy is supplied.
// Cache must outlive World. Quiescent refresh/close/destruction use the creating
// executor. Bindings stay outside this state to avoid a callback owner cycle.
class MccScriptSoundQueryServiceTu15 final {
public:
    static MccScriptSoundQueryServiceTu15 create(const Cache&,
        std::shared_ptr<PhysicsLiveWorldTu15> actual_world);
    MccScriptSoundQueryServiceTu15(const MccScriptSoundQueryServiceTu15&)=default;
    MccScriptSoundQueryServiceTu15&operator=(const MccScriptSoundQueryServiceTu15&)=default;
    void require_current(const Cache&)const;
    SourceScriptSoundClockRowsTu15 rows(const Cache&)const;
    // Copies retain state, and every callback uses its current generation.
    // Bound errors propagate; no PCM or unbound fallback is introduced here.
    SourceScriptSoundClockQueriesTu15 queries(const Cache&)const;
    // Fresh capabilities and checked current rows from the same retained
    // actual World replace the generation only after complete admission.
    // This imports actual restore/reset metadata, never guessed empty rows.
    void refresh(const Cache&)const;
    // Retire callbacks before releasing the retained World on its executor.
    // Subsequent operations reject, including copies of prior query bundles.
    void close()const;
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit MccScriptSoundQueryServiceTu15(std::shared_ptr<Impl>);
};
} // namespace halo2
