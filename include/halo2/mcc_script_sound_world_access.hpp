#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_script_sound_clock_tu15.hpp"
#include <memory>

namespace halo2 {
class PhysicsLiveWorldTu15;
class MccSoundVariationProvider;
struct MccScriptSoundWorldAccessFactory;
// Trusted prepared-Audio phase only: one successful non-looping gain draw
// and one pitch draw have completed. This is not a cull/allocation-failure
// request, an arbitrary RNG setter, or proof of a published platform voice.
struct MccScriptSoundFirstPermutationInput {
    TagHandle sound;
    std::uint16_t relative_pitch{};
    std::uint32_t expected_auxiliary_seed{},post_gain_pitch_seed{};
};
struct MccScriptSoundFirstPermutationResult {
    std::int16_t relative_permutation{};
    std::uint32_t auxiliary_seed_after{};
};
// Typed access to one actual native World's script-sound policy. Every
// operation admits its retained epoch and producer thread before touching the
// borrowed World or Cache. Restore/teardown permanently retire old access.
// Cache keeps DAT/PCM ownership and must outlive its World, as for the metadata
// provider. This object exposes no arbitrary source mutation or sound voice.
// World restore/destruction must quiesce its producer executor, as required
// by World itself. Epoch admission is not a held cross-thread lifetime lock
// across subsequent source execution or an asynchronous teardown guarantee.
class MccScriptSoundWorldAccess final {
public:
    MccScriptSoundWorldAccess(const MccScriptSoundWorldAccess&)=default;
    MccScriptSoundWorldAccess&operator=(const MccScriptSoundWorldAccess&)=default;
    void require_current(const Cache& expected_owner)const;
    // Pair two independently constructed capabilities only when both belong
    // to this same actual current native memory owner and epoch. Cache
    // address, logical roots, identical bytes and matching RNG are not owner
    // identity. No numeric token or mutable access escapes this operation.
    void require_metadata_owner(const Cache& expected_owner,
                                const MccSoundVariationProvider&)const;
    SourceScriptSoundClockTimeTu15 time(const Cache& expected_owner)const;
    // Runs the actual complete213410 against its genuine prefix/RNG owners.
    // The two selector bytes retain stores already made before a later fault;
    // this operation never creates or assigns a serialized sound voice.
    void select(const Cache& expected_owner,TagHandle sound,
                SourceScriptSoundClockSelectionTu15& selection)const;
    // Runs complete213690 with qualified21F6B0, separately admitted21F670
    // authored permutation-time, or original stored-word paths. Missing original
    // tuples reject before caller-input writes. Recheck all current immutable
    // owners after ordinary scratch guards and immediately before execution;
    // a guard-induced mismatch retains completed caller-input stores. This
    // trusted single-producer contract requires immutable metadata throughout
    // the genuine call. Results are original FLOAT bits, not PCM.
    std::uint32_t selected_seconds(const Cache& expected_owner,TagHandle sound,
                const SourceScriptSoundClockSelectionTu15& selection)const;
    // Requires finite authenticated/equivalent original stored-time identity
    // and its complete expected native20-byte root. Unknown/custom rows reject.
    std::int32_t stored_milliseconds(const Cache& expected_owner,TagHandle sound)const;
    SoundDriverFloatingPointContext floating_point_context(const Cache& expected_owner)const;
    // Initial FF packet only, after the two actual creation draws. Uses the
    // verified ordinary selector and fresh native range/packed-used state.
    // Compare the complete beforeimage after mutable guards, then publish
    // final Aux/range/used state through the ordinary World journal. This is
    // final-state CAS, not literal original store-event replay. A later store
    // failure retains completed writes and the World continuation fault.
    // The original post-pitch private voice seed is not advanced here.
    MccScriptSoundFirstPermutationResult first_permutation_after_pitch(
        const Cache& expected_owner,const MccScriptSoundFirstPermutationInput&)const;
    // Import the actual checkpointed16-row metadata. Every genuine source
    // voice word must be NONE; a source voice cannot be made into a host ID.
    SourceScriptSoundClockRowsTu15 clock_rows(const Cache& expected_owner)const;
    // Closed native64 adapter publication, after its ordinary policy phase.
    // Compare all current metadata and require all16 source voices NONE
    // before any write. Sound identities must belong to this native catalog.
    // Raw selector/predicate/trailing bytes retain their source byte domain:
    // a failed selection may leave new selector bytes beside the old sound.
    // This trusted producer contract does not authenticate arbitrary rows as
    // original instruction execution. It never writes the source voice words.
    // Publication keeps the ordinary World journal and write guards. A later
    // write failure retains completed stores and the World's continuation
    // fault; it requires authority restore, rather than an invented rollback.
    void publish_clock_rows(const Cache& expected_owner,
        const SourceScriptSoundClockRowsTu15& expected,
        const SourceScriptSoundClockRowsTu15& after)const;
    // Callbacks retain this access and revalidate on each source read phase.
    // The existing native64 clock/Audio adapter supplies request_stop separately.
    SourceScriptSoundClockPolicyTu15 policy(const Cache& expected_owner)const;
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccScriptSoundWorldAccess(std::shared_ptr<const Impl>);
    friend struct MccScriptSoundWorldAccessFactory;
};
} // namespace halo2
