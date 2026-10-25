#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/native_tu15_multiplayer_data.hpp"
#include "halo2/cache.hpp"
#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/native_tu15_physics_branch_data.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_cinematic_shadow_control_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_network_session_startup_tu15.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <ranges>
#include <tuple>
#include <string_view>

namespace {
using namespace halo2;
#include "fixtures/native_tu15_vehicle_render_operands.original.inc"
#include "fixtures/native_tu15_vehicle_read_operands.original.inc"
#include "fixtures/native_tu15_physical_methods.original.inc"
#include "fixtures/native_tu15_vehicle_closure_operands.original.inc"
#include "fixtures/native_tu15_game_engine_methods.original.inc"
#include "fixtures/native_tu15_multiplayer_data.original.inc"
#include "fixtures/native_tu15_melee_candidate_table.original.inc"
#include "fixtures/weapon_fallback_constructor_tu15.original.inc"
#include "fixtures/native_tu15_multiplayer_branch_data.original.inc"
void check(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    return Bytes(memory.view(address,4)).u32(0);
}
void check_portal_empty_bounds(const SourceRuntimeMemory& cold) {
    // Original21b207e7...176462 reads this writable .data pointer. Its
    // destination is shared with474D40, and complete380FA0 constructs it.
    check(word(cold,0x474d3cu)==0x55655cu && word(cold,0x474d40u)==0x55655cu,
        "Portal and BSP queries must bind the same original empty-bounds owner");
    cold.validate_mutable_view(0x474d3cu,4);
    check(std::ranges::all_of(cold.view(0x55655cu,24),[](auto b){return b==std::byte{};}),
        "Portal bounds must remain loader-zeroed until their native constructor");
    cold.validate_mutable_view(0x55655cu,24);
    const auto regions=cold.checkpoint().regions;
    const auto binding=std::ranges::find_if(regions,[](const auto& r){return r.address==0x474d3cu;});
    check(binding!=regions.end() && binding->bytes.size()==4,
        "Portal binding must not admit adjacent original mutable state");
    SourceRuntimeMemory memory;memory.restore(cold.checkpoint());
    constexpr unsigned stack=0x63000800u,stop=0x63002000u;
    memory.mount(stack-0x800,std::vector<std::byte>(0x3000),SourceRuntimeMemory::Access::ReadWrite,
        "Portal constructor caller stack");
    memory.write(stack,std::as_bytes(std::span(&stop,1)));
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp={0x11111111u,0x22222222u,0x33333333u,0x44444444u,
        stack,0x66666666u,0x77777777u,0x88888888u};abi.eflags=0x202;
    auto expected_gp=abi.gp;expected_gp[4]+=4;
    SourcePhysicsWorldRuntimeServices services;
    std::vector<unsigned> instructions;
    services.instruction=[&](unsigned pc){instructions.push_back(pc);};
    unsigned stores{};
    source_run_effect_particle_draw_tu15(memory,0x380fa0u,abi,{0x27f,0x1f80},services,
        [&](const SourcePhysicsContactRecordStore& store) {
            if(store.address<0x55655cu || store.address>=0x556574u)return;
            check(stores<6 && store.instruction==0x380fb0u+8*stores &&
                store.address==0x55655cu+4*stores && store.written.size()==4 &&
                Bytes(store.written).u32(0)==(stores%2?0xff7fffffu:0x7f7fffffu),
                "Native portal constructor differs from original ordered bounds publication");
            ++stores;
        });
    constexpr std::array original_pcs{0x380fa0u,0x380fa8u,0x380fb0u,0x380fb8u,
        0x380fc0u,0x380fc8u,0x380fd0u,0x380fd8u,0x380fe0u};
    check(stores==6 && std::ranges::equal(instructions,original_pcs) &&
        abi.gp==expected_gp && abi.eflags==0x202,
        "Complete native portal constructor changed its original path or caller state");
    for(unsigned i=0;i<6;++i)
        check(word(memory,word(memory,0x474d3cu)+4*i)==(i%2?0xff7fffffu:0x7f7fffffu),
            "Portal pointer does not observe the constructor's actual published bounds");
    memory.write(0x474d3cu,std::as_bytes(std::span(&stop,1)));
    SourceRuntimeMemory restored;restored.restore(memory.checkpoint());
    check(word(restored,0x474d3cu)==stop,
        "Checkpoint restoration replaced live writable portal binding with loader state");
}
void check_weapon_fallback_constructor(const SourceRuntimeMemory &cold) {
    const auto storage=cold.view(0x5564f0u,0x58);
    check(std::ranges::all_of(storage,[](auto b){return b==std::byte{};}),
        "Fallback effect storage must remain loader-zeroed until its original constructor");
    cold.validate_mutable_view(0x5564f0u,0x58);
    for(const auto &row:weapon_fallback_original_cases) {
        SourceRuntimeMemory memory;
        memory.mount(0x5564f0u,std::vector<std::byte>(0x58,std::byte(row.fill)),
            SourceRuntimeMemory::Access::ReadWrite,"Original constructor differential destination");
        memory.mount(0x55d728u,std::array{std::byte(row.input),std::byte(row.input>>8)},
            SourceRuntimeMemory::Access::ReadOnly,"Original constructor differential ushort input");
        memory.mount(0x63000000u,std::vector<std::byte>(0x3000),
            SourceRuntimeMemory::Access::ReadWrite,"Original constructor differential caller stack");
        constexpr std::uint32_t sp=0x63000800u,stop=0x63002000u;
        memory.write(sp,std::as_bytes(std::span(&stop,1)));
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp={0x11111111u,0x22222222u,0x33333333u,0x44444444u,
            sp,0x66666666u,0x77777777u,0x88888888u};abi.eflags=2;
        SourcePhysicsWorldRuntimeServices services;
        std::vector<unsigned> instructions;
        services.instruction=[&](unsigned pc){instructions.push_back(pc);};
        unsigned stores{};
        source_run_effect_particle_frame_tu15(memory,0x380e70u,abi,{0x27f,0x1f80},services,
            [&](const SourcePhysicsContactRecordStore &store) {
                if(store.address<0x5564f0u || store.address>=0x556548u)return;
                check(stores<weapon_fallback_original_store_sites.size(),"Fallback constructor added a store");
                const auto site=weapon_fallback_original_store_sites[stores++];
                check(store.instruction==site[0] && store.address==site[1] && store.written.size()==site[2],
                    "Fallback constructor store order/width differs from original execution");
                for(unsigned i=0;i<site[2];++i)
                    check(std::to_integer<unsigned>(store.written[i])==row.bytes[store.address-0x5564f0u+i],
                        "Fallback constructor store value differs from original execution");
            });
        check(stores==26 && std::ranges::equal(instructions,weapon_fallback_original_pcs),
            "Fallback constructor omitted original instructions/stores");
        check(abi.gp==row.gp && abi.eflags==row.eflags,
            "Fallback constructor changed the original caller registers/flags/stack retirement");
        for(unsigned i=0;i<0x58;++i)
            check(std::to_integer<unsigned>(memory.view(0x5564f0u+i,1)[0])==row.bytes[i],
                "Fallback effect records differ from unchanged original execution");
    }
}
void check_object_update_policy(const SourceRuntimeMemory& cold) {
    // Pinned TU1.5 21b207e7...: E35D5 reads exactly one .data BSS byte.
    // The loader zeroes it; a reference executable is never a runtime input.
    constexpr std::uint32_t address=0x51daedu;
    // Execute the complete actual E3440 consumer, including its real E65A0
    // callee, with explicit caller-owned object/tag tables. Both zero/nonzero
    // policy branches then return normally; no external owner is fabricated.
    struct PrefixComplete {};
    for(const bool prefix:{false,true})for(const auto value:{0u,1u,0x80u,0xffu}) {
        const auto policy=prefix?0x51daecu:address;
        const auto input_checkpoint=cold.checkpoint();
        SourceRuntimeMemory memory;memory.restore(input_checkpoint);
        constexpr unsigned context=0x63000000u,objects=0x64000000u,
            object=0x66000000u,tags=0x67000000u,definition=0x68000000u,
            model=0x69000000u,stack=0x65000800u;
        const auto mount=[&](unsigned p,unsigned bytes,const char* owner) {
            memory.mount(p,std::vector<std::byte>(bytes),SourceRuntimeMemory::Access::ReadWrite,owner);
        };
        mount(context,0x48,"E3440 test caller object context");
        mount(objects,0x20,"E3440 test caller object table");
        mount(object,0x500,"E3440 test caller object");
        mount(tags,0x30,"E3440 test caller tag table");
        mount(definition,0x200,"E3440 test caller object definition");
        mount(model,0x30,"E3440 test caller model definition");
        mount(stack-0x800,0x1000,"E3440 test caller stack");
        const auto put=[&](unsigned p,unsigned bits,unsigned bytes=4) {
            std::array<std::byte,4> raw{};
            for(unsigned n=0;n<bytes;++n)raw[n]=std::byte(bits>>(8*n));
            memory.write(p,std::span(raw).first(bytes));
        };
        // The world normally publishes these live context pointers after
        // cold construction. Here their caller-owned tables are explicit.
        for(const auto p:{0x4ecbd0u,0x4f0414u}) {
            try{(void)memory.view(p,4);}catch(const std::out_of_range&){
                mount(p,4,"E3440 test caller context binding");
            }
        }
        put(0x4ecbd0u,context);put(context+0x44,objects);put(objects+8,object);
        put(0x4f0414u,tags);put(tags+8,definition);put(tags+0x18,model);
        put(object,0x23450000u);put(definition+0x38,0x34560001u);
        put(definition+0x1f0,0x800);put(object+0x3fd,1,1);
        put(stack,0x70000000u);put(stack+4,0x12340000u);
        if(prefix){
            // Real13BA80/1A8E50/16EA60 iterate an explicitly empty player
            // datum array. These are caller-owned runtime tables, not a
            // substituted callee result or a retail runtime asset.
            constexpr unsigned player_pool=0x6a000000u,game_options=0x6b000000u;
            mount(player_pool,0x4c,"E3440 test caller empty player datum array");
            mount(game_options,0x1134,"E3440 test caller game options");
            try{(void)memory.view(0x4f54f4u,4);}catch(const std::out_of_range&){
                mount(0x4f54f4u,4,"E3440 test caller player pool binding");
            }
            put(0x4f54f4u,player_pool);put(0x4f3218u,game_options);
            put(definition+0x1f0,0);put(object+0x134,1u<<27);
            //111E00 returns its real absent-equipment state; BBBC0 invokes
            // real B9390 on the already-marked caller object table entry.
            put(object+0x68,0xffffffffu);put(objects+2,2,1);
        }
        if(value)put(policy,value,1);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=0x202;
        unsigned instructions{},last{};bool read=false,after_read=false,entered_callee=false;
        bool late_read=false,after_late_read=false,entered_bbbc0=false,entered_iterator=false,completed_prefix=false;
        SourcePhysicsWorldRuntimeServices services{};
        services.instruction=[&](unsigned pc) {
            last=pc;read|=pc==0xe35d5u;after_read|=pc==0xe35dau;entered_callee|=pc==0xe65a0u;
            late_read|=pc==0xe366eu;after_late_read|=pc==0xe3673u;
            entered_bbbc0|=pc==0xbbbc0u;entered_iterator|=pc==0x16ea60u;
            if(prefix&&pc==0xe3683u)throw PrefixComplete{};
            check(++instructions<(prefix?512u:256u),"Actual E3440 reader escaped its bounded caller state");
        };
        if(prefix)services.required_owner=[&](SourcePhysicsContactMemory &bank,
            const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &state,
            const SourcePhysicsContactRecordObserver &observe){
            // The public runner exports these exact complete closure cases;
            // owns_entry() lists only its top-level coordination roles.
            check(call.target==0x16ea60u||call.target==0xbbbc0u,
                "E3440 prefix reached an unprovided actual callee owner");
            source_run_ai_frame_tu15(bank,call.target,state,{0x27f,0x1f80},services,observe);
        };
        try{source_run_physics_live_support_tu15(memory,0xe3440u,abi,{0x27f,0x1f80},services);}
        catch(const PrefixComplete&){completed_prefix=true;}
        catch(const std::exception&){std::cerr<<"Actual E3440 reader at0x"<<std::hex<<last<<std::dec<<": ";throw;}
        if(prefix){
            check(completed_prefix&&read&&after_read&&entered_callee&&late_read&&after_late_read&&
                entered_bbbc0&&entered_iterator,
                "Actual E3440 prefix omitted a real callee or its late policy reader/branch");
            check(std::to_integer<unsigned>(memory.view(object+0x34b,1).front())==1&&
                (word(memory,object+0xc0)&0x100u)==(value?0x100u:0u),
                "Actual E366E zero/nonzero branch differs from its caller-owned object state");
        }else{
            check(read&&after_read&&entered_callee&&abi.gp[4]==stack+8&&(abi.gp[0]&0xffu)==1,
                "Actual E3440 reader did not finish its genuine E65A0 callee and policy branch/return");
            check(memory.view(object+0x349,1).front()==std::byte(value?1:0),
                "Actual E3440 zero/nonzero policy branch differs from the source");
        }
        check(memory.view(policy,1).front()==std::byte(value),
            "Actual E3440 consumer changed its input policy byte");
        const auto consumed=memory.checkpoint();
        const auto input_owner=std::ranges::find(input_checkpoint.regions,policy,
            &SourceRuntimeMemory::CheckpointRegion::address);
        const auto consumed_owner=std::ranges::find(consumed.regions,policy,
            &SourceRuntimeMemory::CheckpointRegion::address);
        check(input_owner!=input_checkpoint.regions.end()&&consumed_owner!=consumed.regions.end()&&
            consumed_owner->owner==input_owner->owner&&consumed_owner->access==input_owner->access&&
            consumed_owner->page_bytes==input_owner->page_bytes&&consumed_owner->committed==input_owner->committed&&
            consumed_owner->bytes.size()==1&&consumed_owner->bytes.size()==input_owner->bytes.size()&&
            consumed_owner->bytes[0]==std::byte(value),
            "Actual E3440 consumer changed the exact policy checkpoint owner");
        SourceRuntimeMemory retained;retained.restore(consumed);
        const auto restored=retained.checkpoint();
        const auto retained_owner=std::ranges::find(restored.regions,policy,
            &SourceRuntimeMemory::CheckpointRegion::address);
        check(retained_owner!=restored.regions.end()&&*retained_owner==*consumed_owner&&
            retained.view(policy,1).front()==std::byte(value)&&!retained.is_read_only(policy,1),
            "Consumed E3440 policy owner did not survive exact checkpoint/restore");
    }
    check(cold.view(address,1).front()==std::byte{},
        "E35D5 object update lost its exact loader-zero policy byte");
    const auto checkpoint=cold.checkpoint();
    const auto owner=std::ranges::find(checkpoint.regions,address,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(owner!=checkpoint.regions.end()&&owner->bytes.size()==1&&
        owner->access==SourceRuntimeMemory::Access::ReadWrite&&
        !owner->page_bytes&&owner->committed.empty(),
        "E35D5 object update policy lacks its exact committed writable owner");
    // This is live mutable state in the authority bank, not an immutable
    // literal or an untracked default returned by the missing-backing path.
    SourceRuntimeMemory replacement;replacement.restore(checkpoint);
    for(const auto value:{std::byte{1},std::byte{0x80},std::byte{0xff}}) {
        replacement.write(address,std::span(&value,1));
        const auto changed=replacement.checkpoint();
        SourceRuntimeMemory retained;retained.restore(changed);
        check(retained.view(address,1).front()==value&&
            !retained.is_read_only(address,1),
            "Authority checkpoint dropped the mutable E35D5 policy byte");
        replacement.restore(checkpoint);
        check(replacement.view(address,1).front()==std::byte{},
            "Authority restore failed to restore the cold E35D5 policy byte");
    }
    check(cold.checkpoint()==checkpoint,"Replacement policy writes changed the cold source bank");
}
void check_ai_priority_readers(const SourceRuntimeMemory& cold) {
    // Independent TU1.5 4568C4..4568DD fixture SHA256 d770ce78698bcf11...
    // is bounded by270350's guarded270E90 selector, not adjacent padding.
    constexpr std::array<unsigned,13> expected{1,1,2,3,4,6,7,8,8,9,9,9,9};
    check(cold.is_read_only(0x4568c4u,26),"AI contact priorities lost immutable ownership");
    const auto regions=cold.checkpoint().regions;
    const auto owner=std::ranges::find(regions,0x4568c4u,&SourceRuntimeMemory::CheckpointRegion::address);
    check(owner!=regions.end()&&owner->bytes.size()==26,
        "AI contact priority owner extends beyond its authenticated reachable range");
    check(!cold.is_read_only(0x47d6c4u,8),"AI target priorities lost authored writable access");
    const auto target_owner=std::ranges::find(regions,0x47d6c4u,&SourceRuntimeMemory::CheckpointRegion::address);
    check(target_owner!=regions.end()&&target_owner->bytes.size()==8,
        "AI target priority owner extends beyond its original four-state domain");
    constexpr std::array<unsigned,4> target_expected{0,0,2,3};
    SourceRuntimeMemory memory;
    memory.mount(0x5020c8u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test actor-array binding");
    memory.mount(0x63000000u,std::array<std::byte,0x48>{},SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test actor-array header");
    memory.mount(0x64000000u,std::array<std::byte,0x888>{},SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test actor");
    memory.mount(0x65000000u,std::array<std::byte,4096>{},SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test stack");
    memory.mount(0x4568c4u,cold.view(0x4568c4u,26),SourceRuntimeMemory::Access::ReadOnly,
        "AI priority test original contact inputs");
    memory.mount(0x50eef8u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test target-array binding");
    memory.mount(0x66000000u,std::array<std::byte,0x48>{},SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test target-array header");
    memory.mount(0x67000000u,std::array<std::byte,0x50>{},SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test target");
    memory.mount(0x47d6c4u,cold.view(0x47d6c4u,8),SourceRuntimeMemory::Access::ReadWrite,
        "AI priority test original target-state inputs");
    const auto put=[&](unsigned address,unsigned value,unsigned width=4) {
        std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8));
        memory.write(address,std::span(bytes).first(width));
    };
    put(0x5020c8u,0x63000000u);
    put(0x63000044u,0x64000000u);
    put(0x64000858u,0xffffffffu);
    put(0x50eef8u,0x66000000u);
    put(0x66000044u,0x67000000u);
    for(const bool target:{false,true})for(int index=-1;index<=(target?4:13);++index) {
        put(0x64000328u,target?0u:unsigned(index),2);
        put(0x67000026u,target?unsigned(index):0u,2);
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp[4]=0x65000800u;abi.eflags=0x202;
        struct PrefixComplete {};
        bool read_done=false,rejected=false;
        unsigned instructions{};
        SourcePhysicsWorldRuntimeServices services{};
        services.instruction=[&](unsigned pc) {
            check(++instructions<=64,"AI contact reader exceeded its bounded original prefix");
            if(pc==(target?0x2700d9u:0x2700a9u)){read_done=true;throw PrefixComplete{};}
        };
        try{source_run_ai_frame_tu15(memory,0x270000u,abi,{0x27f,0x1f80},services);}
        catch(const PrefixComplete&){}
        catch(const std::exception&){rejected=true;}
        if(index<0||index>=(target?4:13))
            check(!read_done&&rejected,"AI contact reader admitted an outside-domain table index");
        else
            check(read_done&&!rejected&&(abi.gp[target?1:7]&0xffffu)==
                (target?target_expected[index]:expected[index]),
                "Actual2700A1/2700D1 read differs from original bounded priority inputs");
    }
}
}
int main(int argc,char** argv)try {
    constexpr std::uint32_t tick=0x60212000u;
    SourceRuntimeKernelTickCount input{tick,{std::byte{0x78},std::byte{0x56},std::byte{0x34},std::byte{0x12}},
        "world memory test retained clock"};
    for(bool writable:{false,true}) {
        SourceRuntimeMemory partial;const auto& row=original_multiplayer_branch_data.front();
        const std::array bytes{writable?row.bytes.front():row.bytes.front()^std::byte{0x80}};
        partial.mount(row.address,bytes,writable?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,
            "existing multiplayer branch owner control");
        const auto prior=partial.checkpoint();bool rejected=false;
        try{mount_native_tu15_multiplayer_branch_data(partial);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&partial.checkpoint()==prior,"Multiplayer branch admission changed or accepted a conflicting owner");
    }
    auto memory=native_tu15_world_memory(input);
    check_portal_empty_bounds(memory);
    // Pinned21b207e7... original167CAB/16821D and151570 operands.
    // The high-bounce Dire Valley map exercises the formerly missing first
    // owner; approaching a native portal exercises the second. The full
    // application replay remains the regression for those complete paths.
    check(std::ranges::equal(memory.view(0x46a5a0u,4),
        std::array{std::byte{6},std::byte{0},std::byte{0x80},std::byte{0x37}}) &&
        memory.is_read_only(0x46a5a0u,4),
        "Large-bound light query operand differs from its original read-only declaration");
    check(std::ranges::equal(memory.view(0x46a340u,4),
        std::array{std::byte{6},std::byte{0},std::byte{0x80},std::byte{0xb7}}) &&
        memory.is_read_only(0x46a340u,4),
        "Light shadow query operand differs from its original read-only declaration");
    check(std::ranges::equal(memory.view(0x48c78cu,4),
        std::array{std::byte{0},std::byte{0},std::byte{0},std::byte{0x40}}) &&
        !memory.is_read_only(0x48c78cu,4),
        "Portal approach policy differs from its original writable initialization");
    for(const auto [address,size]:std::array{std::pair{0x56e20cu,8u},std::pair{0x56e218u,8u},
            std::pair{0x56e214u,2u},std::pair{0x56e220u,2u}}) {
        check(std::ranges::all_of(memory.view(address,size),[](auto v){return v==std::byte{};}),
            "151540/151630 policy must retain original loader-zero bytes");
        memory.validate_mutable_view(address,size);
    }
    // Whole53240 owns this complete saved-session destination;165860 reads
    // its initialized byte only after a source player loses the current unit.
    // Cold storage is not a completed network/session initialization result.
    const auto saved_session=memory.view(0x4d6098u,0x144u);
    check(std::ranges::all_of(saved_session,[](auto value){return value==std::byte{};}),
        "Saved-session storage must retain original loader bytes before startup");
    memory.validate_mutable_view(0x4d6098u,0x144u);
    {
        // Reuse preserves arbitrary actual live bytes and native backing.
        SourceRuntimeMemory live;mount_native_tu15_network_saved_session_storage(live);
        const auto native=live.native_address(0x4d6098u,0x144u);
        live.write(0x4d61d8u,std::array{std::byte{0xa7}});
        const auto before=live.checkpoint();mount_native_tu15_network_saved_session_storage(live);
        check(live.checkpoint()==before&&live.native_address(0x4d6098u,0x144u)==native,
            "Repeated saved-session admission changed live bytes or backing");
        mount_native_tu15_network_session_data(live);
        check(live.native_address(0x4d6098u,0x144u)==native&&
              live.view(0x4d61d8u,1)[0]==std::byte{0xa7},
            "Full network data mounting replaced the shared saved-session owner");
        for(const bool readonly:{false,true}) {
            SourceRuntimeMemory foreign;
            foreign.mount(0x4d6098u,std::vector<std::byte>(0x144),
                readonly?SourceRuntimeMemory::Access::ReadOnly:SourceRuntimeMemory::Access::ReadWrite,
                "foreign saved-session owner");
            const auto prior=foreign.checkpoint();bool rejected{};
            try{mount_native_tu15_network_saved_session_storage(foreign);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected&&foreign.checkpoint()==prior,"Saved-session admission accepted or changed a foreign owner");
        }
    }
    const auto branch_checkpoint=memory.checkpoint();mount_native_tu15_multiplayer_branch_data(memory);
    check(memory.checkpoint()==branch_checkpoint,"Repeated multiplayer branch admission changed existing ownership");
    check_weapon_fallback_constructor(memory);
    for(const auto [address,bytes]:std::array{
        std::pair{0x4d2b18u,9u*0x408u},std::pair{0x4d4f60u,0x40u},std::pair{0x515f1cu,8u}}) {
        const auto cold=memory.view(address,bytes);
        check(std::ranges::all_of(cold,[](auto value){return value==std::byte{};}),
            "Cached special storage must retain its original loader bytes before source frame reset");
        memory.validate_mutable_view(address,bytes);
    }
    check_object_update_policy(memory);
    check_ai_priority_readers(memory);
    {
        // The complete HS startup provider also supplies the two bindings
        // formerly declared separately by the world constructor. Preserve
        // the real registry access/owner and the exact authored descriptors.
        const auto regions=memory.checkpoint().regions;
        for(const auto&[cell,target]:std::array{
            std::pair{0x480c24u,0x456bb0u},std::pair{0x481634u,0x459d58u}}) {
            check(word(memory,cell)==target,"Cold HS builtin registry binding differs");
            memory.validate_mutable_view(cell,4);
            const auto owner=std::ranges::find_if(regions,[&](const auto& region) {
                return region.address<=cell &&
                    std::uint64_t(region.address)+region.bytes.size()>=std::uint64_t(cell)+4;
            });
            check(owner!=regions.end() && owner->access==SourceRuntimeMemory::Access::ReadWrite &&
                owner->owner=="Original TU1.5 authored HS function pointer registry",
                "Cold HS binding lost its complete writable registry owner");
        }
        const auto descriptor=[&]<std::size_t N>(std::uint32_t address,
            const std::array<std::uint32_t,N>& words) {
            check(memory.is_read_only(address,N*4),"Cold HS builtin descriptor is not immutable");
            for(std::size_t i=0;i<N;++i)
                check(word(memory,address+std::uint32_t(i*4))==words[i],
                    "Cold HS builtin descriptor differs from its authored words");
        };
        descriptor(0x456bb0u,std::array{0x00020006u,0x2105d0u,0x4670d4u,0u});
        descriptor(0x459d58u,std::array{0x00000004u,0x002b4a40u,0u,0x00060002u,0x00000006u});
    }
    // Independent original image read at450212:05 00, readonly .rdata.
    // No neighbouring selector is admitted by this reached-word owner.
    check(memory.is_read_only(0x450212u,2) && Bytes(memory.view(0x450212u,2)).u16(0)==5,
        "Actual1F1A3C index1 alternative is absent, writable or changed");
    // Independently authenticated original45F290..45F2BF, covering all24
    // bitmap formats admitted by138780's original selector bound.
    std::uint64_t bitmap_depth_hash=14695981039346656037ull;
    check(memory.is_read_only(0x45f290u,48),"Complete bitmap bit-depth table is not immutable");
    for(const auto byte:memory.view(0x45f290u,48))
        bitmap_depth_hash=(bitmap_depth_hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
    check(bitmap_depth_hash==0x30844cf11e590c09ull,"Bitmap bit-depth inputs differ from all24 original formats");
    for(const auto offset:{0u,22u,47u}) {
        SourceRuntimeMemory foreign;
        foreign.mount(0x45f290u+offset,std::array<std::byte,1>{},
            SourceRuntimeMemory::Access::ReadWrite,"foreign bitmap table overlap");
        const auto before=foreign.checkpoint();bool rejected=false;
        try{mount_native_tu15_renderer_startup_data(foreign);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&foreign.checkpoint()==before,"Bitmap table overlap caused partial startup publication");
    }
    // Independent original fixture e926ad8121e9e0f0... authenticates these
    // 132 bytes; test actual cold composition, not only the standalone mount.
    // Table order is the existing draw reader's blend_operands[0..2].
    std::uint64_t contrail_hash=14695981039346656037ull;
    for(const auto address:{0x45c438u,0x45c408u,0x45c3d8u}) {
        check(memory.is_read_only(address,44),"Cold contrail blend data lacks its exact immutable owner");
        for(const auto byte:memory.view(address,44))
            contrail_hash=(contrail_hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
    }
    check(contrail_hash==0xcaf5ef873c341710ull,"Cold contrail blend data differs from33 original words");
    // Independent original geometry-dispatch fixture/proof, not blend state.
    constexpr std::array geometry_targets{0x430ceu,0x431c8u,0x431c8u,0x436dfu,0x432bdu};
    check(memory.is_read_only(0x436f0u,20),"Cold contrail geometry dispatch lacks immutable coverage");
    for(unsigned n=0;n<geometry_targets.size();++n)
        check(word(memory,0x436f0u+n*4u)==geometry_targets[n],"Cold contrail geometry dispatch differs from five original words");
    for(const auto&[address,bytes,writable]:vehicle_closure_operands) {
        std::vector<std::byte> actual(bytes.size());memory.read(address,actual);
        check(std::ranges::equal(actual,std::as_bytes(bytes)),"Original vehicle closure input bytes differ");
        for(unsigned n=0;n<bytes.size();++n)check(memory.is_read_only(address+n,1)!=writable,
            "Original vehicle closure input lost section access");
    }
    check(word(memory,0x1d574u)==0x1d4fdu && word(memory,0x1d578u)==0x1d4f6u &&
        word(memory,0x138be4u)==0x13887cu,"Original raw CPU bitmap dispatch operands differ");
    check(memory.is_read_only(0x1d574u,8)&&memory.is_read_only(0x1d59eu,2)&&
        memory.is_read_only(0x45f2a4u,2)&&memory.is_read_only(0x138be4u,4)&&
        !memory.is_read_only(0x40db36u,2),"Original raw CPU bitmap section access differs");
    check(memory.view(0x1d59eu,2)[0]==std::byte{5}&&memory.view(0x1d59eu,2)[1]==std::byte{6}&&
        memory.view(0x40db36u,2)[0]==std::byte{0xa1}&&memory.view(0x40db36u,2)[1]==std::byte{0xa1}&&
        memory.view(0x45f2a4u,2)[0]==std::byte{32}&&memory.view(0x45f2a4u,2)[1]==std::byte{},
        "Original raw CPU bitmap literal selector/flags/bit depth differs");
    check(std::ranges::all_of(memory.view(0x505e84u,8),[](auto b){return b==std::byte{};})&&
        std::ranges::all_of(memory.view(0x505e90u,0xd0c),[](auto b){return b==std::byte{};}),
        "Cold contrail destinations contain a fabricated completed value");
    check(!memory.is_read_only(0x505e84u,8)&&!memory.is_read_only(0x505e90u,0xd0c)&&
        memory.is_read_only(0x46a2fcu,4)&&word(memory,0x46a2fcu)==0x47e7aaacu,
        "Original contrail startup storage/scale operand differs");
    check(word(memory,0x46a168u)==0x3d8f5c29u&&word(memory,0x46a170u)==0x4b000000u&&
        memory.is_read_only(0x46a168u,4)&&memory.is_read_only(0x46a170u,4),
        "Original contrail interpolation/quantization operands differ");
    check(word(memory,0x4f2d74u)==0&&word(memory,0x4f2d78u)==0&&
        !memory.is_read_only(0x4f2d74u,8),"Original model-cache policy/counter cold fields differ");
    check(memory.view(0x48c754u,1)[0]==std::byte{1}&&!memory.is_read_only(0x48c754u,1),
        "Original130F90 geometry-cache pressure-warning setting lost its authored writable byte");
    for(const auto [address,target]:std::array<std::pair<std::uint32_t,std::uint32_t>,4>{{
        {0x474cc0u,0x44abecu},{0x474cd4u,0x44abecu},
        {0x474cd8u,0x44ac28u},{0x474cdcu,0x44ac34u}}}) {
        check(word(memory,address)==target&&!memory.is_read_only(address,4),
            "Original direct-consumer vector alias lost its authored value or writable section access");
        check(memory.is_read_only(target,12),"Vector alias lost its existing authored constant backing");
    }
    check(word(memory,0x411568u)==0&&!memory.is_read_only(0x411568u,4),
        "Original405801 cold SDK device cell lost its loader-zero writable owner");
    check(word(memory,0x52be40u)==0&&!memory.is_read_only(0x52be40u,4),
        "Original targeting sort lost its loader-zero writable parameter binding");
    check(memory.view(0x51daeeu,1)[0]==std::byte{}&&!memory.is_read_only(0x51daeeu,1),
        "Original optional vehicle query lost its loader-zero writable policy byte");
    check(memory.view(0x56e198u,1)[0]==std::byte{}&&!memory.is_read_only(0x56e198u,1),
        "Original receiver request warning lost its loader-zero writable flag");
    for(const auto address:std::array{0x56e6c0u,0x56e6c4u,0x56e6c8u,0x56e6ccu,0x56e6d0u})
        check(word(memory,address)==0 && !memory.is_read_only(address,4),
            "Original282170 conveyor vector/guard lost its loader-zero writable owner");
    check(word(memory,0x56ecccu)==0 && word(memory,0x56ecd0u)==0 &&
        !memory.is_read_only(0x56ecccu,8),
        "Original F4D1A lazy scalar/flags lost their loader-zero writable owner");
    check(word(memory,0x46a178u)==0x3dcccccdu && memory.is_read_only(0x46a178u,4),
        "Original F4D70 lazy initializer lost its exact tick scaling operand");
    check(memory.is_read_only(0x45efacu,12)&&word(memory,0x45efacu)==0&&
        word(memory,0x45efb0u)==0&&word(memory,0x45efb4u)==0xbf800000u,
        "Original object effect direction lost its exact immutable vector");
    constexpr std::array damage_dispatch{0x16098au,0x160968u,0x160972u,0x16097cu};
    for(unsigned n=0;n<damage_dispatch.size();++n)
        check(memory.is_read_only(0x160a4cu+n*4,4)&&word(memory,0x160a4cu+n*4)==damage_dispatch[n],
            "Original damage-response dispatch lost its immutable branch identities");
    constexpr std::array object_damage_targets{0xdbfadu,0xdbf53u,0xdbf98u,0xdbf89u,0xdbfbeu,0xdbfccu};
    for(unsigned n=0;n<object_damage_targets.size();++n)
        check(memory.is_read_only(0xdc084u+n*4,4)&&word(memory,0xdc084u+n*4)==object_damage_targets[n],
            "Original object damage dispatch lost its immutable branch identities");
    constexpr std::array<unsigned,13> object_damage_selectors{0,1,2,2,2,3,5,1,5,5,5,1,4};
    const auto selectors=memory.view(0xdc09cu,object_damage_selectors.size());
    check(memory.is_read_only(0xdc09cu,selectors.size()),"Original object damage selectors lost immutable ownership");
    for(unsigned n=0;n<selectors.size();++n)
        check(std::to_integer<unsigned>(selectors[n])==object_damage_selectors[n],"Original object damage byte selector differs");
    check(memory.is_read_only(0x40db3eu,1)&&memory.view(0x40db3eu,1)[0]==std::byte{8},
        "Original SDK DXT3 format entry lost its immutable literal backing");
    check(memory.is_read_only(0x46ab2cu,4)&&word(memory,0x46ab2cu)==0x400a3ae7u,
        "Original object-effect comparison lost its immutable authored float");
    check(memory.is_read_only(0x46a7f0u,4)&&word(memory,0x46a7f0u)==0x3da3d70au,
        "Original vehicle-look input scalar lost its immutable authored bits");
    constexpr std::array physical_directions{
        0x0u,0x0u,0x0u,0x3f800000u,0x0u,0x0u,0x0u,0x0u,0x3f800000u,
        0x3f3504f3u,0x0u,0x3f3504f3u,0x3f13cd3au,0x3f13cd3au,0x3f13cd3au,0x3f13cd3au,0xbf13cd3au,0x3f13cd3au,
        0x3f3504f3u,0x3f3504f3u,0x0u,0x3f3504f3u,0xbf3504f3u,0x0u,0x0u,0x3f3504f3u,0x3f3504f3u,
        0x0u,0xbf3504f3u,0x3f3504f3u,0xbf800000u,0x0u,0x0u,0x0u,0x3f800000u,0x0u,
        0x0u,0xbf800000u,0x0u,0xbf3504f3u,0xbf3504f3u,0x0u,0xbf3504f3u,0x3f3504f3u,0x0u,
        0xbf3504f3u,0x0u,0x3f3504f3u,0xbf13cd3au,0xbf13cd3au,0x3f13cd3au,0xbf13cd3au,0x3f13cd3au,0x3f13cd3au,
        0x0u,0x0u,0xbf800000u,0x3f3504f3u,0x0u,0xbf3504f3u,0xbf3504f3u,0x0u,0xbf3504f3u,
        0x0u,0x3f3504f3u,0xbf3504f3u,0x0u,0xbf3504f3u,0xbf3504f3u,0x3f13cd3au,0x3f13cd3au,0xbf13cd3au,
        0x3f13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0x3f13cd3au,0xbf13cd3au};
    check(memory.is_read_only(0x461998u,27*12),"Original physical query directions lost immutable ownership");
    for(unsigned n=0;n<physical_directions.size();++n)
        check(word(memory,0x461998u+n*4)==physical_directions[n],
            "Original18/27 physical query direction bits differ");
    check(!memory.is_read_only(0x473360u,8)&&word(memory,0x473360u)==0&&word(memory,0x473364u)==0x3fe00000u,
        "Original CRT double half lost its file-initialized writable owner");
    for(const auto address:{0x44aa04u,0x44aa24u})
        check(memory.is_read_only(address,4)&&word(memory,address)==0xffffffffu,
            "Original mode1 object effect event lost its immutable authored sentinel");
    check(memory.is_read_only(0x44a9e4u,4)&&word(memory,0x44a9e4u)==0u,
        "Original mode1 object effect lost its exact immutable event selector");
    check(memory.is_read_only(0x44aa48u,4)&&word(memory,0x44aa48u)==0x3f2aacdau,
        "Original damage event scalar differs from authenticated original bits");
    for(const auto& record:original_physical_methods)
        for(unsigned offset=0;offset<record.bytes.size();offset+=4) {
            check(memory.is_read_only(record.base+offset,4),
                "Original physical method record lost readonly ownership");
            check(std::ranges::equal(memory.view(record.base+offset,4),std::span(record.bytes).subspan(offset,4)),
                "Physical method record differs from independently captured original bytes");
        }
    check(!memory.is_read_only(0x47c1f0u,4)&&word(memory,0x47c1f0u)==0x0600008bu,
        "Original object effect event name lost its file-initialized writable owner");
    // Independent TU1.5 image audit v1356: the complete16-byte .data table
    // is writable, including selector2 shared with111786. Cold composition
    // must borrow the existing owner, never remount or freeze either word.
    for(unsigned i=0;i<4;++i)
        check(!memory.is_read_only(0x47c1f0u+i*4u,4) && word(memory,0x47c1f0u+i*4u)==0x0600008bu+i,
            "Complete authored reaction-name table lost its writable identity");
    check(!memory.is_read_only(0x48c770u,4)&&word(memory,0x48c770u)==0x46800000u,
        "Original packed color interpolation scale lost its initialized writable owner");
    check(memory.is_read_only(0x44a8dcu,original_melee_candidate_bytes.size()) &&
        std::ranges::equal(memory.view(0x44a8dcu,original_melee_candidate_bytes.size()),original_melee_candidate_bytes),
        "Original melee candidate table lost its complete authenticated immutable bytes");
    for(const auto& row:original_multiplayer_branch_data)
        check(memory.is_read_only(row.address,row.bytes.size()) &&
            std::ranges::equal(memory.view(row.address,row.bytes.size()),row.bytes),
            "Multiplayer branch data differs from its full authenticated immutable table");
    for(const auto& region:original_multiplayer_data_regions) {
        check(std::ranges::equal(memory.view(region.address,region.bytes.size()),region.bytes),
            "Cold world lacks authenticated multiplayer class or state publication storage");
        check(memory.is_read_only(region.address,region.bytes.size())==region.read_only,
            "Cold world multiplayer data access differs from the original owner");
    }
    for(const auto &record:original_game_engine_methods)
        for(unsigned offset=0;offset<record.bytes.size();offset+=4) {
            check(memory.is_read_only(record.base+offset,4),"Original registered game-engine method lost readonly ownership");
            check(std::ranges::equal(memory.view(record.base+offset,4),std::span(record.bytes).subspan(offset,4)),
                "Registered game-engine method differs from authenticated original bytes");
        }
    for(const auto address:{0x468c04u,0x468cd4u,0x468da4u,0x468e74u})
        check(word(memory,address)==0x2e4d60u&&memory.is_read_only(address,4),
            "Original game-engine player retirement slot lost its literal immutable method");
    for(const auto address:{0x468bf8u,0x468cc8u,0x468d98u,0x468e68u})
        check(memory.is_read_only(address,4)&&word(memory,address)==0x25bb50u,
            "Original game-engine player event policy lost its immutable method identity");
    for(const auto address:{0x468c5cu,0x468d2cu,0x468dfcu,0x468eccu})
        check(memory.is_read_only(address,4)&&word(memory,address)==0x2e4ea0u,
            "Original game-engine damage event lost its immutable method identity");
    check(!memory.is_read_only(0x50ef10u,255*32)&&
        std::ranges::all_of(memory.view(0x50ef10u,255*32),[](auto b){return b==std::byte{};}),
        "Original animation transform array prefix lacks its loader-zero writable owner");
    check(!memory.is_read_only(0x510ef0u,32)&&memory.view(0x510ef0u,32).size()==32&&
        !memory.is_read_only(0x510f10u,32)&&memory.view(0x510f10u,32).size()==32,
        "Animation array admission displaced its retained final node or fallback transform");
    constexpr std::array lens_dispatch{0x2f5bau,0x2f2ccu,0x2f3e2u,0x2f3feu,0x2f50au};
    for(unsigned i=0;i<lens_dispatch.size();++i)
        check(word(memory,0x2f5c8u+i*4)==lens_dispatch[i]&&memory.is_read_only(0x2f5c8u+i*4,4),
            "Original2F2C5 lens draw dispatch table lacks its exact immutable targets");
    check(memory.view(0x4c6894u,16u*12u).size()==192&&!memory.is_read_only(0x4c6894u,192)&&
        std::ranges::all_of(memory.view(0x4c6894u,192),[](auto b){return b==std::byte{};}),
        "Original sixteen-entry cinematic shadow destination differs");
    {
        auto shadow=native_tu15_world_memory(input);
        shadow.mount(0x64000000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"declared shadow-control test stack");
        const auto put=[&](unsigned at,unsigned value){std::array<std::byte,4>bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));shadow.write(at,bytes);};
        const auto call=[&](unsigned object,bool enabled,unsigned marker){
            put(0x64000800u,0x65000000u);put(0x64000804u,0x3ecccccdu);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[3]=enabled;abi.gp[6]=object;abi.gp[7]=marker;abi.gp[4]=0x64000800u;abi.eflags=0x202;
            source_run_cinematic_shadow_control_tu15(shadow,0x30e10u,abi,{0x27f,0x1f80},{});
            check(abi.gp[4]==0x64000808u,"Original shadow caller did not return");
        };
        for(unsigned n=0;n<16;++n){call(n+1,true,n+100);const auto at=0x4c6894u+n*12u;
            check(word(shadow,0x4c6954u)==n+1&&word(shadow,at)==n+1&&word(shadow,at+4)==n+100&&word(shadow,at+8)==0x3ecccccdu,
                "Original shadow insertion escaped or changed the cold owner");}
        call(17,true,999);check(word(shadow,0x4c6954u)==16&&word(shadow,0x4c6894u+15*12u)==16,
            "Original shadow capacity guard changed");
        call(1,false,0);check(word(shadow,0x4c6954u)==15&&word(shadow,0x4c6894u)==16,
            "Original shadow removal did not move its actual last entry");
    }
    constexpr std::array vehicle_dispatch{0xf25dfu,0xf25f0u,0xf2649u,0xf2600u,0xf2610u,0xf2625u,0xf2635u};
    check(memory.is_read_only(0xf2bfcu,28),"Original vehicle dispatch table lacks immutable backing");
    for(unsigned i=0;i<vehicle_dispatch.size();++i)check(word(memory,0xf2bfcu+i*4)==vehicle_dispatch[i],
        "Original vehicle dispatch target differs");
    for(const auto&[address,bytes]:vehicle_read_operands) {
        std::vector<std::byte> actual(bytes.size());memory.read(address,actual);
        check(std::ranges::equal(actual,std::as_bytes(bytes)),"Original vehicle literal read bytes differ");
        for(unsigned n=0;n<bytes.size();++n)check(memory.is_read_only(address+n,1),
            "Original vehicle literal read lost immutable ownership");
    }
    for(const auto&[address,bits]:vehicle_render_operands) {
        check(word(memory,address)==bits,"Original vehicle render scalar cold input differs");
        const bool writable=address==0x473e1cu||address==0x473e20u||address==0x473e34u;
        check(memory.is_read_only(address,4)!=writable,"Vehicle operand lost its original section access");
    }

    // Actual180C77/180C82 select row1 in the MCC Cyclotron world. All four
    // fields are offline-verified TU1.5 .rdata, also consumed by182BB0.
    constexpr std::array placement_row1{0x42200000u,0x42dc0000u,0x3fc00000u,1u};
    for(unsigned i=0;i<placement_row1.size();++i)
        check(word(memory,0x44eb40u+i*4)==placement_row1[i],
            "Native original placement selector-one parameters differ");
    check(memory.is_read_only(0x44eb40u,16),"Placement selector-one row became writable");
    const auto dispatch=[&](std::uint32_t address,const auto& expected) {
        for(unsigned i=0;i<expected.size();++i) {
            check(memory.is_read_only(address+i*4,4),
                "Original dispatch table lacks complete immutable ownership");
            check(word(memory,address+i*4)==expected[i],
                "Native dispatch identity differs from the offline TU1.5 table");
        }
    };
    // Actual MCC player/vehicle/lift runs reached these original data tables;
    // constructors, consumers and extents are pinned in v779/v780 audits.
    dispatch(0x461420u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x2e9250u,0x2e4d60u,0x31ede0u,
        0x2e8120u,0x2e90e0u,0x2e81e0u,0x2e9140u,0x2e7d10u,0x306bb0u,
        0x2e7d90u,0x3070f0u,0x2e7e10u,0x31d520u});
    dispatch(0x41ca9cu,std::array{
        0x2fc540u,0x2f16f0u,0x2f13f0u,0x2f10b0u,
        0x2f0f10u,0x2f0da0u,0x2e38c0u,0x2f1850u});
    dispatch(0x15cfa8u,std::array{
        0x15cefeu,0x15cf16u,0x15cf2eu,0x15cf46u,0x15cf5eu,0x15cf76u,0x15cf8eu});
    dispatch(0x1e0184u,std::array{
        0x1dfc35u,0x1dfc9cu,0x1dfd9eu,0x1dff3eu,0x1dffb4u,0x1e001cu});
    dispatch(0x41ca8cu,std::array{0x2f0a50u,0x30ba80u});
    dispatch(0x41ca94u,std::array{0x2f0a20u,0x30ba90u});
    dispatch(0x462060u,std::array{
        0x1f2c10u,0x2e1270u,0x2e12a0u,0x306a80u,0x2e4d60u,0x31cae0u,
        0x306de0u,0x2e9400u,0x306bc0u,0x306ed0u,0x306b90u,0x306bb0u});
    dispatch(0x41ea3cu,std::array{
        0x31cea0u,0x2e1270u,0x2e12a0u,0x31e1e0u,0x2e4d60u,0x31e2e0u,
        0x31e2f0u,0x325d80u,0xa3850u,0x31e360u,0x31e330u});
    dispatch(0x41e79cu,std::array{0x31d700u,0x2e4d60u,0x325aa0u,0x2e4d60u});
    dispatch(0x45f8c0u,std::array{0x1848a0u,0x185190u});
    dispatch(0x461f64u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x308530u,0x2e4d60u,0x308540u,
        0x308550u,0x2e9400u,0x1f7da0u,0x2e4ea0u,0x1f34d0u,0x1f3550u});
    dispatch(0x41e458u,std::array{
        0x3235c0u,0x3240d0u,0x323e30u,0x323f50u,0x324230u,0x323420u,
        0x31d520u,0x323860u,0x3238a0u,0x323b80u,0x323b80u,0x323bb0u,
        0x323c10u,0x323c50u,0x323c50u,0x3238b0u,0x323900u,0x323950u,
        0x3239a0u,0x323b20u,0x3239f0u,0x323a00u,0x323a10u,0x323a70u,
        0x323c80u,0x323d10u,0x323d80u,0x323d50u,0x323e00u,0x324410u,0x323ab0u});
    // Actual MCC v820 reached31D5A4's slot7 after original31D68A installed
    // this interface. Offline v826 bounds its eleven methods before null.
    dispatch(0x41e910u,std::array{
        0x31cea0u,0x2e1270u,0x2e12a0u,0x31d410u,0x2e4d60u,0x31d510u,
        0x31ede0u,0x31d710u,0x31d520u,0x31d930u,0x31d630u});
    check(word(memory,0x46aae8u)==0x3fd55556u && memory.is_read_only(0x46aae8u,4) &&
        word(memory,0x46a1f4u)==0x3f555555u && memory.is_read_only(0x46a1f4u,4),
        "Original F9083/E892C fixed arithmetic inputs lack their exact readonly bytes");
    check(word(memory,0x46a584u)==0x46a2f8e0u && memory.is_read_only(0x46a584u,4),
        "Original18B580 sound-scenery angle scale lacks its exact readonly bytes");
    check(word(memory,0xe985cu)==0 && memory.is_read_only(0xe985cu,4),
        "Authored null branch identity was replaced or lacks readonly ownership");
    // The live movement failure read byte10 of original115BCF's selector,
    // after real native frames and ticks. Check its actual factory ownership.
    constexpr std::array<unsigned,12> movement_selector{0,0,2,2,2,2,2,2,2,2,1,1};
    for(unsigned i=0;i<movement_selector.size();++i) {
        check(memory.is_read_only(0x115e10u+i,1),"Original movement selector is not immutable");
        check(std::to_integer<unsigned>(memory.view(0x115e10u+i,1).front())==movement_selector[i],
            "Original movement selector byte differs");
    }
    // Partial shared owners must survive, and neither writable bytes nor
    // differing readonly bytes may be accepted or overwritten by the profile.
    for(unsigned fault=0;fault<2;++fault) {
        SourceRuntimeMemory conflicting;
        const std::array value{std::byte(fault?1:0)};
        conflicting.mount(0x115e1au,value,fault?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"isolated conflicting selector owner");
        const auto before=conflicting.checkpoint();bool rejected=false;
        try{mount_native_tu15_physics_branch_data(conflicting);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected && conflicting.checkpoint().regions==before.regions,
            "Physics branch profile changed or admitted a conflicting owner");
    }
    SourceRuntimeMemory partial;
    partial.mount(0x115e1au,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadOnly,
        "isolated compatible selector owner");
    mount_native_tu15_physics_branch_data(partial);
    const auto retained=partial.checkpoint();
    const auto at=std::ranges::find(retained.regions,0x115e1au,&SourceRuntimeMemory::CheckpointRegion::address);
    check(at!=retained.regions.end() && at->owner=="isolated compatible selector owner" && at->bytes.size()==1,
        "Physics branch profile replaced a compatible partial owner");
    std::array<std::byte,12> selector_read{};partial.read(0x115e10u,selector_read);
    for(unsigned i=0;i<selector_read.size();++i)check(std::to_integer<unsigned>(selector_read[i])==movement_selector[i],
        "Selector read across shared owners differs");
    mount_native_tu15_physics_branch_data(partial);
    check(partial.checkpoint().regions==retained.regions,"Physics branch profile is not idempotent");
    // Exact TU1.5 C3300 class tables, including the transparent opening's
    // class-zero path into C2A80. These are authored operands, not fade results.
    constexpr std::array<std::uint32_t,25> light_fade_tables{
        0x41a00000u,0x41700000u,0x41200000u,0x40a00000u,0u,
        0x41700000u,0x41200000u,0x40e00000u,0x40400000u,0u,
        0x40200000u,0x40000000u,0x3fc00000u,0x3f800000u,0x3f000000u,
        0x40200000u,0x40000000u,0x3fc00000u,0x3f800000u,0x3f000000u,
        0u,0u,0u,0u,0u};
    for(unsigned i=0;i<light_fade_tables.size();++i)
        check(word(memory,0x44a6d8u+i*4)==light_fade_tables[i],
            "Native original effect-light class table differs");
    check(memory.is_read_only(0x44a6d8u,100),"Effect-light authored tables became writable");
    check(word(memory,0x474c80u)==0x44ab1cu&&word(memory,0x44ab1cu)==0u&&
        word(memory,0x44ab20u)==0x3f800000u&&word(memory,0x44ab24u)==0u,
        "Original1FEB9 default screen-vector owner differs");
    check(memory.is_read_only(0x474c80u,4)&&memory.is_read_only(0x44ab1cu,12),
        "Original screen-vector operands became writable");
    constexpr std::array camera_tables{
        std::pair{0x16fd30u,std::array{0x16f6f4u,0x16fa29u,0x16fbf2u,0x16fc79u}},
        std::pair{0x16f33cu,std::array{0x16f247u,0x16f21bu,0x16f29cu,0x16f265u}}};
    for(const auto&[address,words]:camera_tables) {
        check(memory.is_read_only(address,16),"Original camera dispatch table lacks its immutable owner");
        for(unsigned i=0;i<words.size();++i)check(word(memory,address+i*4)==words[i],
            "Original bounded camera dispatch identity differs");
    }
    check(word(memory,0x474c2cu)==0x44aad8u,"Original17C84C lens color record binding differs");
    for(const auto&[address,bits]:std::array{
        std::pair{0x46a250u,0x4b7fffffu},std::pair{0x46a268u,0x3c008081u},
        std::pair{0x46a384u,0x3c800000u},std::pair{0x46a51cu,0x3fb504f3u},
        std::pair{0x46a9b4u,0x4f000000u}})
        check(word(memory,address)==bits&&memory.is_read_only(address,4),
            "Original literal lens-query/draw operand lacks immutable exact-byte backing");
    for(unsigned n=0;n<4;++n)check(word(memory,0x44aad8u+n*4)==0x3f800000u,
        "Original lens RGBA default operand differs");
    check(memory.is_read_only(0x474c2cu,4)&&memory.is_read_only(0x44aad8u,4)&&memory.is_read_only(0x44aadcu,12),
        "Original lens default record lacks immutable ownership");
    check(word(memory,0x41b774)==tick && word(memory,tick)==0x12345678u,
        "Native tick-count data was not bound to its actual owner");
    check(memory.is_read_only(0x41b774,4) && !memory.is_read_only(tick,4),
        "Tick-count binding or volatile data access differs");
    check(memory.view(word(memory,0x474c70u),12).size()==12,
        "Original scale/color vector cannot be read by its complete native owner");
    input.bytes.fill(std::byte{});
    check(word(memory,tick)==0x12345678u,"World memory borrowed expired caller bytes");
    for(const auto cell:{0x56ee28u,0x4f3218u,0x4f3cd8u,0x51df54u,
                         0x5020c4u,0x4f54f0u,0x51df44u,0x515ec0u,
                         0x56ebacu,0x56e684u,0x502088u,0x56b170u})
        check(!word(memory,cell),
            "Cold composition fabricated a completed constructor result");
    for(const auto cell:{0x4f0430u,0x4922a8u,0x48d480u})
        check(memory.view(cell,1).front()==std::byte{},
            "Cold composition fabricated successful initialization");
    bool missing=false;
    try{(void)memory.view(0x10000,4);}catch(const std::out_of_range&){missing=true;}
    check(missing,"Cold composition included an executable header");
    missing=false;
    try{(void)memory.view(0x13a700,4);}catch(const std::out_of_range&){missing=true;}
    check(missing,"Cold composition included executable instructions");
    std::size_t regions=0,bytes=0;
    const auto checkpoint=memory.checkpoint();
    const auto type_owner=std::ranges::find(checkpoint.regions,0x5020b4u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(type_owner!=checkpoint.regions.end() && type_owner->bytes.size()==15u &&
        type_owner->access==SourceRuntimeMemory::Access::ReadWrite &&
        type_owner->owner=="native TU1.5 cold complete 1E7690 object-type runtime flags" &&
        !type_owner->page_bytes && type_owner->committed.empty(),
        "Complete native object-type runtime owner is absent or has an invalid schema");
    check(std::ranges::all_of(type_owner->bytes,[](auto value){return value==std::byte{};}),
        "Native object-type runtime flags were warmed before the source writer");
    check(std::ranges::none_of(checkpoint.regions,[](const auto&region){return region.address==0x5020b8u;}),
        "Old one-byte member remained separately mounted inside the complete array");
    native_tu15_validate_type_runtime_restore(memory,checkpoint);
    unsigned rejected_type_schemas{};
    for(unsigned fault=0;fault<9;++fault) {
        auto invalid=checkpoint;
        const auto owner=std::ranges::find(invalid.regions,0x5020b4u,
            &SourceRuntimeMemory::CheckpointRegion::address);
        switch(fault) {
        case 0:invalid.regions.erase(owner);break;
        case 1:owner->bytes.pop_back();break;
        case 2:owner->bytes.push_back(std::byte{});break;
        case 3:owner->access=SourceRuntimeMemory::Access::ReadOnly;break;
        case 4:owner->owner+=" invalid";break;
        case 5:owner->page_bytes=4096;break;
        case 6:owner->committed={1};break;
        case 7:owner->address=0x5020b8u;owner->bytes={std::byte{}};
            owner->owner="native TU1.5 cold 102A54 scene placement policy byte";break;
        case 8:owner->address+=1;break;
        }
        bool rejected=false;
        try {native_tu15_validate_type_runtime_restore(memory,invalid);}
        catch(const std::exception&error) {
            rejected=std::string(error.what())=="TU1.5 authority lacks its complete object-type runtime owner";
        }
        check(rejected,"Native restore admitted a missing, malformed or old partial type-runtime owner");
        check(memory.checkpoint()==checkpoint,"Rejected type-runtime schema mutated the current factory bank");
        ++rejected_type_schemas;
    }
    // This isolated whole-section topology has no native15-byte declaration;
    // the surrounding authority keeps its existing offline-image admission.
    SourceRuntimeMemory whole_section;
    whole_section.mount(0x502000u,std::vector<std::byte>(4096),
        SourceRuntimeMemory::Access::ReadWrite,"isolated whole-section schema control");
    native_tu15_validate_type_runtime_restore(whole_section,whole_section.checkpoint());
    std::cout<<"Native object-type runtime restore schemas: "<<rejected_type_schemas
        <<" rejected, complete factory owner accepted; offline whole-section route preserved\n";

    for(const auto& region:checkpoint.regions) {
        check(!region.owner.empty(),"World declaration has no retained owner identity");
        check(memory.native_address(region.address,region.bytes.size()).value>UINT32_MAX,
            "World data was narrowed to a 32-bit host address");
        ++regions;bytes+=region.bytes.size();
    }
    // Optional offline audit compares individually declared data against the
    // authenticated TU1.5 reference. The production factory accepts no path.
    if(argc==3 && std::string_view(argv[1])=="--light-query-data-reference") {
        auto original=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
        for(const auto [address,size]:std::array{std::pair{0x46a5a0u,4u},std::pair{0x46a340u,4u},
                std::pair{0x48c78cu,16u},std::pair{0x48c79cu,2u},std::pair{0x56e20cu,8u},
                std::pair{0x56e218u,8u},std::pair{0x56e214u,2u},std::pair{0x56e220u,2u}}) {
            check(std::ranges::equal(memory.view(address,size),original.view(address,size)),
                "Light/portal query declarations differ from original executable loader bytes");
        }
        std::cout<<"Original TU1.5 light/portal query data:8 exact declarations matched\n";
    }else if(argc==2) {
        auto original=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);
        for(const auto& region:checkpoint.regions) {
            if(region.address==tick || region.address==0x41b774)continue;
            if(!std::ranges::equal(memory.view(region.address,region.bytes.size()),
                original.view(region.address,region.bytes.size()))) {
                std::cerr<<"Reference differs at 0x"<<std::hex<<region.address<<" "<<region.owner<<'\n';
                return 1;
            }
        }
    }else check(argc==1,"Expected optional offline reference path or --light-query-data-reference PATH");
    const auto level_owner=std::ranges::find(checkpoint.regions,0x4fadb8u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(level_owner!=checkpoint.regions.end() && level_owner->bytes.size()==0x208u &&
        level_owner->access==SourceRuntimeMemory::Access::ReadWrite &&
        level_owner->owner=="source 64 level-handle pairs and two pool constructor bindings",
        "Complete level-handle source owner is absent or has a partial schema");
    check(std::ranges::all_of(level_owner->bytes,[](auto value){return value==std::byte{};}),
        "Cold level-handle backing fabricated a source constructor result");
    unsigned bounded_name_cases{};
    for(const auto& [address,name,extent]:std::array{
        std::tuple{0x4607ecu,std::string_view{"level handles"},16u},
        std::tuple{0x4607d0u,std::string_view{"multiplayer level handles"},28u}}) {
        check(memory.is_read_only(address,extent),"Level-handle name extent is not immutable");
        const auto owner=std::ranges::find(checkpoint.regions,address,
            &SourceRuntimeMemory::CheckpointRegion::address);
        check(owner!=checkpoint.regions.end() && owner->bytes.size()==extent,
            "Level-handle pool name has an inexact owned extent");
        check(memory.view(0x4607ecu+15u,1).front()==std::byte{},
            "Level-handle original post-NUL DWORD byte differs");
        bool read_only{};try{memory.mutable_view(address,1);}catch(const std::exception&){read_only=true;}
        check(read_only,"Authored pool name accepted a write");
        constexpr std::uint32_t stack=0x61008000u,destination=0x61010000u,stop=0x70000000u;
        for(const auto count:std::array{0u,1u,unsigned(name.size()+1u),32u}) {
            SourceRuntimeMemory call_bank;call_bank.restore(checkpoint);
            call_bank.mount(stack-4096u,std::vector<std::byte>(8192),SourceRuntimeMemory::Access::ReadWrite,
                "isolated bounded pool-name actual caller stack");
            call_bank.mount(destination,std::vector<std::byte>(64,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
                "isolated bounded pool-name caller destination");
            const auto put=[&](auto p,auto value){std::array<std::byte,4> raw{};
                for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));call_bank.write(p,raw);};
            put(stack,stop);put(stack+4,destination);put(stack+8,address);put(stack+12,count);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=0x202;
            source_run_physics_live_support_tu15(call_bank,0x32a4e0u,abi,{0x27f,0x1f80},{});
            check(abi.gp[0]==destination && abi.gp[4]==stack+4,
                "Actual bounded pool-name copy returned a different caller frame");
            const auto output=call_bank.view(destination,64);
            for(unsigned i=0;i<64;++i) {
                const auto expected=i>=count?std::byte{0xa5}:i<name.size()?std::byte(name[i]):std::byte{};
                check(output[i]==expected,"Actual bounded pool-name copy differs from source string/zero padding");
            }
            ++bounded_name_cases;
        }
    }
    std::cout<<"Native bounded level-handle name controls: "<<bounded_name_cases<<" cases\n";
    for(const auto& bad:std::array{
        SourceRuntimeKernelTickCount{0x1000,{},"invalid low clock"},
        SourceRuntimeKernelTickCount{0xffffffffu,{},"invalid wrapped clock"},
        SourceRuntimeKernelTickCount{tick,{},""}}) {
        bool rejected=false;
        try{(void)native_tu15_world_memory(bad);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Invalid native tick-count owner was admitted");
    }
    std::cout<<"Native TU1.5 world cold declarations: "<<regions<<" regions, "<<bytes<<" bytes\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
