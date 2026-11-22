#pragma once
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/scripts.hpp"
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

namespace halo2 {
// Live services of the campaign world for the native builtin dispatcher.
// floating_point is queried only by kernels with original SSE/x87 operations.
struct NativeCampaignScriptHostServices {
  SourcePhysicsContactMemory* memory{};
  NativeCampaignScriptWriter writer;
  std::function<NativeCampaignScriptFloatingPoint()> floating_point;
  // HS object-list members in original reference order (VM list bank).
  std::function<std::vector<std::uint32_t>(std::uint32_t)> list_members;
  // Native VM HS list bank: allocation (with its root refresh) and the
  // original prepend-style append, for builtins that return lists.
  std::function<std::uint32_t()> list_allocate;
  std::function<void(std::uint32_t,std::uint32_t)> list_append;
  // The native VM's local-player alias (object handle 0) resolves here.
  std::uint32_t player{0xffffffffu};
  // The retained World admits the authored line, playback and source FP.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> play_line_on_object;
};

// Returns nullopt when the name is not one of the native semantic builtins.
// Arity and compiled argument types follow the TU1.5 descriptors; arguments
// are converted to their declared scalar type exactly like 212010.
std::optional<ScriptValue> native_campaign_script_builtin_tu15(std::string_view name,
    const std::vector<ScriptValue>& arguments,const NativeCampaignScriptHostServices&);
bool native_campaign_script_builtin_admitted_tu15(std::string_view name);
}
