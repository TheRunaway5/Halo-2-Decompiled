#pragma once
#include "halo2/native_tu15_sound_gp_output.hpp"
#include "halo2/source_sound_wet_returns_tu15.hpp"
#include "halo2/source_sound_wet_voice_layout_tu15.hpp"
namespace halo2::tu15_sound_wet_return {
struct View {
    SourceSoundWetReturnRouteTu15 route;
    std::span<const tu15_sound::Word24,32> samples;
};
// Retain a real completed logical GP copy's native resources. Views observe
// current retained storage, as the receipt does; they are not frame snapshots.
// This names original return sources/destinations without guessing VP sample
// conversion, gain, feedback latency, scheduling or a device-ready state.
// Returned spans borrow these resources; retain a Sources/receipt while used.
class Sources final {
public:
    explicit Sources(const tu15_gp_output::LogicalCopyReceipt& receipt):receipt_(receipt){}
    View view(unsigned)const;
    // Select from actual SDK-produced layout values. This is a logical value
    // view; the caller still owns source-address/resource association and time.
    View logical_view(const SourceSoundWetVoiceLayoutTu15&)const;
    std::uint64_t copy_ordinal()const noexcept{return receipt_.copy_ordinal();}
private:
    tu15_gp_output::LogicalCopyReceipt receipt_;
};
}
