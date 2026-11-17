#pragma once
#include "halo2/native_campaign_hs_command_builtins_tu15.hpp"
#include "halo2/native_campaign_ai_command_lifecycle_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/native_campaign_ai_command_facing_tu15.hpp"

// Test-only strict composition. Every accepted child executes a compiled native
// owner against the same retained state; no evaluated arguments are supplied.
inline void native_hs_command_test_call(halo2::SourcePhysicsContactMemory &memory,
    std::uint32_t entry,halo2::SourcePhysicsWorldRuntimeAbi &abi,
    const halo2::SourcePhysicsContactRecordObserver &observe={}) {
    using namespace halo2;
    auto child=[&](std::uint32_t site,std::uint32_t target,SourcePhysicsWorldRuntimeAbi &frame){
        if(target==0x212130u)throw NativeCampaignAiPerformRequiredOwnerTu15(site,target);
        native_hs_command_test_call(memory,target,frame,observe);
    };
    switch(entry) {
    case 0x2b2470u:case 0x2b2670u:case 0x210a10u:case 0x2b1200u:
    case 0x2b0e30u:case 0x2b1000u:case 0x2b10b0u:case 0x2b11b0u:case 0x2b1300u:case 0x2b2410u:
        native_campaign_hs_command_builtins_call_tu15(memory,entry,abi,{child},observe);return;
    case 0x280c90u:case 0x280d70u:
        native_campaign_ai_command_facing_call_tu15(memory,entry,abi,observe);return;
    case 0x211cb0u:case 0x211840u:case 0x262590u:case 0x262710u: {
        NativeCampaignAiCommandLifecycleServicesTu15 services;services.callee=child;
        native_campaign_ai_command_lifecycle_call_tu15(memory,entry,abi,services,observe);return;
    }
    case 0x211b10u:case 0x212010u:case 0x211da0u:case 0x212560u:
    case 0x20fee0u:case 0x2b0bf0u: {
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &,const auto &call,auto &frame,const auto &){
            if(call.target==0x212130u)throw NativeCampaignAiPerformRequiredOwnerTu15(call.instruction,call.target);
            child(call.instruction,call.target,frame);
        };
        source_run_ai_frame_tu15(memory,entry,abi,*abi.floating_point,services,observe);return;
    }
    default:throw NativeCampaignAiPerformRequiredOwnerTu15(entry,entry);
    }
}
