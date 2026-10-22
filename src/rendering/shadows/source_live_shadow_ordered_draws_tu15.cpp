#include "halo2/source_live_shadow_ordered_draws_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <cstring>

namespace halo2 {
SourceLiveShadowOrderedDrawsTu15 source_live_shadow_ordered_draws_tu15(
    PhysicsLiveWorldTu15& world,SourceShadowPhase phase,std::uint32_t bank,
    std::uint32_t category,std::uint32_t layer,std::uint32_t owner,
    const SourceShadowAdmissionObserverTu15& observe) {
    constexpr std::uint32_t entry=0x3baa0u,capacity=2560u,record_bytes=24u;
    const bool generate=phase==SourceShadowPhase::Generate;
    const bool apply=phase==SourceShadowPhase::Apply;
    if((!generate && !apply) || bank!=(generate?0x4ce298u:0x4ce280u) ||
       category!=(generate?2u:1u) || layer!=(generate?10u:11u))
        throw FormatError("TU1.5 shadow ordered draws require the actual layer10/11 command route");
    auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    std::array<std::uint32_t,6> header;
    std::memcpy(header.data(),memory.view(bank,sizeof(header)).data(),sizeof(header));
    const auto bank_capacity=generate?512u:1024u;
    if(header[0]!=bank_capacity || header[1]>header[0] || header[3]>header[1])
        throw FormatError("TU1.5 shadow ordered draws consume an incomplete command bank");
    if(header[1]) {
        if(!header[5])throw FormatError("TU1.5 shadow ordered draws have no original command storage");
        (void)memory.view(header[5],std::size_t(header[1])*record_bytes);
    }

    // Authenticated3BC20 at3BC20..3BC3F reservesF004 bytes through32A660,
    // saves EBP, then passes ESP+8 to3BAA0. For its entry stack S:
    //   output=S-F000, saved EBP=S-F008, 3BAA0 entry ESP=S-F01C;
    //   [ESP+4..16]={bank,category,layer,output}.3BAA0 returns with RET16.
    // The native ABI owner already supplies this stack allocation; no heap
    // bank is mounted and no uninitialized descriptor byte is cleared.
    // Evidence: shadow-caller-policy-audit-v1/generation-sorted-descriptors.json
    // and generation-sort-instructions.json (1730 unchanged original events).
    auto abi=world.prepare_source_call(std::array{category});
    const auto caller_stack=abi.gp[4];
    if((caller_stack&3u) || caller_stack<0xf01cu || caller_stack>0xfffffff7u)
        throw FormatError("TU1.5 shadow ordered draw caller stack cannot contain the original frame");
    const auto output=caller_stack-0xf000u,callee_stack=caller_stack-0xf01cu;
    static_assert(capacity*record_bytes==0xf000u);
    (void)memory.view(callee_stack,0xf024u);
    (void)memory.view(output,std::size_t(capacity)*record_bytes);
    const auto returned=word(caller_stack),saved_ebp=abi.gp[5];
    const auto put=[&](std::uint32_t p,std::uint32_t v) {
        world.write_source_call_input(p,std::as_bytes(std::span(&v,1)));
    };
    put(caller_stack-0xf008u,saved_ebp);
    abi.gp[0]=output;abi.gp[3]=layer;abi.gp[5]=category;abi.gp[7]=bank;
    abi.gp[4]=callee_stack;
    put(callee_stack,returned);
    const std::array arguments{bank,category,layer,output};
    for(unsigned n=0;n<arguments.size();++n)put(callee_stack+4u+n*4u,arguments[n]);
    if(observe)observe(true,entry,owner,abi,memory);
    world.run_actual(entry,abi);
    if(observe)observe(false,entry,owner,abi,memory);
    const auto count=abi.gp[0];
    if(abi.gp[4]!=caller_stack-0xf008u || count>capacity || count>header[1]-header[3])
        throw FormatError("TU1.5 shadow ordered draw result escaped its original caller stack/bank");

    SourceLiveShadowOrderedDrawsTu15 result;
    result.phase=phase;result.owner=owner;result.bank=bank;
    result.category=category;result.layer=layer;
    result.caller_stack=caller_stack;result.source_address=output;result.capacity=capacity;
    result.records.resize(count);
    if(count)std::memcpy(result.records.data(),memory.view(output,std::size_t(count)*record_bytes).data(),
        std::size_t(count)*record_bytes);
    for(const auto& record:result.records)if((record[0]&65535u)<header[3] || (record[0]&65535u)>=header[1])
        throw FormatError("TU1.5 shadow ordered draw descriptor lost its original command index");
    return result;
}
}
