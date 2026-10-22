#pragma once
#include <filesystem>

namespace halo2 {
class Cache;
class DesktopSession;
// Legacy V2/reference bridge. This does not satisfy MCC-only runtime assets.
void configure_mcc_loading_presentation(DesktopSession&,const Cache&,
    const std::filesystem::path& imported_cache,bool mute);
// Explicit V3 admission: genuine authenticated MCC PCM, finite native one-shot
// policy, and no V2/ADPCM fallback. Import/onboarding/package activation is a
// separate step; the current production caller has not been switched here.
void configure_mcc_loading_presentation_v3(DesktopSession&,const Cache&,
    const std::filesystem::path& imported_cache,bool mute);
}
