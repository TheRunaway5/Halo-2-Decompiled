#pragma once
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
namespace halo2 {
// Dispatches a real native child over this call's retained memory and ABI.
// It must execute the child or refuse; supplying a result is not permitted.
using NativeCampaignAiCommandNativeChildTu15 =
    std::function<void(std::uint32_t site,std::uint32_t entry,
                       SourcePhysicsWorldRuntimeAbi &)>;
// Notifications bind the real retained identities to the native HS scheduler.
// They cannot allocate an identity, complete a command or supply an HS result.
struct NativeCampaignAiCommandLifecycleServicesTu15 {
    std::function<void(std::uint32_t actor,std::uint32_t command,
        std::uint32_t thread,std::int16_t script)> associated;
    std::function<void(std::uint32_t thread)> thread_retired;
    // Actual native children only; an absent child is a named refusal.
    NativeCampaignAiCommandNativeChildTu15 callee;
};
// 2801D0: ECX packed AI reference, EDI signed script ordinal, RET0.
// 280400: ECX packed AI reference, stack signed script ordinal, AL running, RET4.
// 261BE0: EAX existing HS thread (NONE creates one), stack actor/script, RET8.
// 261A90: EBX existing HS thread, stack signed script ordinal, EAX command, RET4.
// Also owns261D50/262250/262190,263B80,262870/2629D0 and HS construction
// 2117E0/211A80/211CB0, plus211840/262590/262710 scheduling. The actual HS
// evaluator211B10 and primitive/global roots retain actual child boundaries;
// ordinary command-script begin roots are wholly native.
// Defined return bits, saved registers, ESP, retained stores and FP are public;
// dead private stack and volatile registers are not a semantic API.
void native_campaign_ai_command_lifecycle_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const NativeCampaignAiCommandLifecycleServicesTu15 & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Shared263B80 command steering-cache reset, without an ABI frame.
void native_campaign_ai_command_reset_tu15(SourcePhysicsContactMemory &,
    std::uint32_t command,const SourcePhysicsContactRecordObserver & = {});
}
