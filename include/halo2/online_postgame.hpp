#pragma once
#include "halo2/native_vista_pcr.hpp"
#include "halo2/online_match_session.hpp"
namespace halo2 {
class FrontendSession;
struct MultiplayerState;
// Host-owned PCR-consumed publication, with original player/team ranking and
// explicit native room identity/configuration adapters. No persistent rank
// update and no claim of full retail retained metadata publication.
VistaPcrConsumedSnapshot online_postgame_publish(const MultiplayerMatch&,
    const UiVariant&,std::span<const OnlineAdmission>);
// Check the admitted roster, source variant, scores, original placement and
// the four retained public statistics against the final native snapshot.
void online_postgame_validate(const MultiplayerState&,const UiVariant&,
    std::span<const OnlineAdmission>,std::span<const std::byte> consumed_report);
std::shared_ptr<const VistaPcrScreenOwner> online_postgame_screen(
    FrontendSession&,std::shared_ptr<OnlineMatchSession>,std::function<void()> done);
// Actual disconnected source ending, copied after ordinary original completion.
// No room, voice session, history/rank commit or departure consent is invented.
std::shared_ptr<const VistaPcrScreenOwner> online_terminal_postgame_screen(
    FrontendSession&,const MultiplayerState&,VistaPcrConsumedSnapshot,std::vector<OnlineAdmission>,std::function<void()> done);
} // namespace halo2
