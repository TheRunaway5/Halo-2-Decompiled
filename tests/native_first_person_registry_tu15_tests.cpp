#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_first_person_context_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <x86intrin.h>

using namespace halo2;
namespace {
void need(bool value,const char *why){if(!value)throw FormatError(why);}
std::vector<std::byte> encoded(const MultiplayerState::Player &player) {
    need(player.first_person_pose.has_value(),"Replay boundary lost its actual native FP packet");
    const auto &packet=*player.first_person_pose;
    return source_encode_first_person_packet_tu15(packet,packet.temporal.identity);
}
constexpr SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15},std::byte{0xf1}};
}
int main(int argc,char **argv) {
    nlohmann::json report{{"passed",false},{"rows",nlohmann::json::array()}};
    const auto save=[&]{if(argc>=4)std::ofstream(argv[3])<<report.dump(2)<<'\n';};
    try {
        need(argc==6 || argc==7,"Supply prepared MCC MAP SHARED REPORT REPRESENTATION local|remote [birth-only|caller-abi-only]");
        const std::string mode=argc==7?argv[6]:"ordinary";
        const bool birth_only=mode=="birth-only",caller_abi_only=mode=="caller-abi-only";
        need(mode=="ordinary" || birth_only || caller_abi_only,"Unknown registry fixture mode");
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run through tools/run_test.py");
        const auto representation=unsigned(std::stoul(argv[4]));const std::string route=argv[5];
        need((representation==2 || representation==3) && (route=="local" || route=="remote"),"Unknown registry fixture route");
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.mute=true;
        options.starting_weapon=5;options.source_object_world=true;
        options.dedicated=route=="remote";options.local_players=route=="local"?1u:0u;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};
        options.source_object_savegame_directory=std::filesystem::path(scratch)/"registry-world";
        auto profile=source_default_player_configuration();profile[0x44]=representation==3;
        if(options.local_players)options.selected_profiles={profile};
        MultiplayerPreparedWorld world(options);auto &match=world.match;
        need(world.cache->is_mcc() && world.source_objects && world.source_objects->owns_physics(),"Registry test requires native MCC authority");
        unsigned slot{};
        if(route=="local") {
            if(!birth_only)world.bind_source_local_views(world.local_slots);
            slot=world.local_slots.at(0);
        }
        else slot=match.join("Native remote FP registry",-1,profile);
        if(birth_only || caller_abi_only) {
            const auto &memory=world.source_objects->memory();
            const auto word=[&](std::uint32_t at) {
                std::array<std::byte,4> bytes;memory.read(at,bytes);return Bytes(bytes).u32(0);
            };
            std::optional<std::array<std::byte,0x94>> constructor_record;
            unsigned constructor_units{},constructor_stores{};
            std::array<std::byte,0x94> actual_local_input;
            memory.read(word(0x4f9b54u)+0x14u,actual_local_input);
            world.source_objects->observe_physics_stores([&](std::uint32_t,const auto &store) {
                if(store.instruction<0x18a0c0u || store.instruction>=0x18a210u)return;
                ++constructor_stores;
                if(store.instruction==0x18a132u)++constructor_units;
                if(store.instruction==0x18a209u) {
                    constructor_record.emplace();
                    memory.read(word(0x4f9b54u)+0x14u,*constructor_record);
                }
            });
            struct Unobserve {
                MultiplayerObjectRuntime &owner;
                ~Unobserve(){owner.observe_physics_stores({});}
            } unobserve{*world.source_objects};
            auto born=match.state();unsigned steps{};
            while(born.players[slot].source_unit==null_datum && steps++<3u) {
                match.step({});born=match.state();
            }
            const auto &player=born.players[slot];
            need(player.source_unit!=null_datum && !player.dead,"Original engine did not publish the first live unit");
            need(player.first_person_pose.has_value(),"First native unit phase omitted its genuine FP publication");
            const auto retained=0x60070000u+(player.source_player&65535u)*0xa0u;
            std::array<std::byte,0x94> birth_look;memory.read(retained,birth_look);
            need(Bytes(birth_look).u32(0)==player.source_unit,"First unit has no matching retained source look constructor");
            need(word(retained+0x94u)==null_datum,"Birth without a delivered command invented a raw input baseline");
            if(route=="remote") {
                need(constructor_units==1 && constructor_record && *constructor_record==birth_look,
                    "Remote birth input differs from its actual original18A0C0 constructor output");
                std::array<std::byte,0x94> after_birth;
                memory.read(word(0x4f9b54u)+0x14u,after_birth);
                need(after_birth==actual_local_input,"Private birth construction changed an actual local input record");
            } else {
                const auto row=word(word(0x4f54f4u)+0x44u)+(player.source_player&65535u)*0x21cu;
                const auto input_user=word(row+0x28u)&65535u;
                need(input_user<4u,"First local unit lost its actual original input-user binding");
                std::array<std::byte,0x94> original_local;
                memory.read(word(0x4f9b54u)+0x14u+input_user*0x94u,original_local);
                need(original_local==birth_look,"Retained local birth input differs from its genuine original producer record");
            }
            const auto birth_constructor_stores=constructor_stores;
            const auto generation=player.first_person_pose->temporal.generation;
            const auto source_tick=player.first_person_pose->temporal.source_tick;
            need(source_tick+1u==born.tick,"Birth FP publication lost the actual completed source clock");
            if(caller_abi_only) {
                auto &native=world.source_objects->tu15_world();
                SourceFirstPersonContextInputTu15 in;
                in.identity=player.first_person_pose->temporal.identity;in.look=birth_look;
                memory.read(word(0x4f3cd8u),std::as_writable_bytes(std::span(in.initial_rng)));
                const auto actual_caller=native.prepare_source_call();
                need(actual_caller.floating_point.has_value(),"Actual native caller omitted its FP environment");
                std::vector<std::uint32_t> observed;SourcePhysicsWorldRuntimeServices cpu;
                cpu.timestamp_counter=[&] {observed.push_back(_mm_getcsr());return __rdtsc();};
                cpu.reciprocal_sqrt=[&](auto bits) {
                    observed.push_back(_mm_getcsr());
                    return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));
                };
                report["caller_fp_cases"]=nlohmann::json::array();
                for(const auto mxcsr:{0x1f80u,0x1fa0u,0x1fa1u}) {
                    auto caller=actual_caller;caller.floating_point->mxcsr=mxcsr;
                    observed.clear();const auto outer=_mm_getcsr();
                    SourceRuntimeWriteSerial unchanged(memory);
                    SourceFirstPersonContextTu15 context(*world.cache,{},cpu);
                    context.reset(memory,caller,in);context.prepare_identity(memory,caller,in);
                    context.apply_action(memory,caller,{1,born.tick,player.source_unit,player.source_weapon,1,0});
                    context.advance_tick(memory,caller,in);
                    need(context.state().accepted_action_sequence==1 && !unchanged.load(),
                        "Actual native FP action failed or changed borrowed gameplay bytes");
                    need(caller.floating_point->mxcsr==mxcsr && _mm_getcsr()==outer,
                        "Private FP action changed its actual caller or enclosing FP environment");
                    need(std::all_of(observed.begin(),observed.end(),[&](auto value) {
                        return (value&(mxcsr&0x3fu))==(mxcsr&0x3fu);
                    }),"Actual nested CPU owner lost incoming sticky status bits");
                    report["caller_fp_cases"].push_back({{"mxcsr",mxcsr},{"nested_cpu_observations",observed.size()},
                        {"observed_nested_status_preserved",observed.empty()?nlohmann::json(nullptr):nlohmann::json(true)},
                        {"caller_mxcsr_unchanged",true},{"enclosing_fp_environment_unchanged",true}});
                }
                unsigned rejected{};
                for(unsigned invalid=0;invalid<9u;++invalid) {
                    auto bad=actual_caller;
                    if(invalid==0)bad.floating_point.reset();
                    if(invalid==1)bad.floating_point->x87_control_word^=0x100u;
                    if(invalid==2)bad.floating_point->mxcsr^=0x40u;
                    if(invalid==3)bad.floating_point->mxcsr^=0x80u;
                    if(invalid==4)bad.floating_point->mxcsr^=0x2000u;
                    if(invalid==5)bad.floating_point->mxcsr^=0x8000u;
                    if(invalid==6)bad.floating_point->mxcsr^=0x10000u;
                    if(invalid==7)bad.gp[4]=0;
                    if(invalid==8)bad.gp[4]=0xffffffe0u;
                    bool failed{};
                    try{SourceFirstPersonContextTu15 context(*world.cache,{},cpu);context.reset(memory,bad,in);}
                    catch(const FormatError &error){failed=std::string(error.what())=="Native FP context lacks the retained native caller ABI";}
                    need(failed,"Native FP caller admission accepted missing state or unsupported controls/stack");++rejected;
                }
                report.update({{"passed",true},{"representation",representation},{"route",route},
                    {"captured_1fa0_action_accepted",true},{"invalid_caller_cases_rejected",rejected},
                    {"scope","Real native context constructor, identity bind and declared action from actual born-unit inputs; live F0DF1 sequence qualification is separate"}});
                save();std::cout<<"PASS actual native FP caller status and controls "<<route<<'\n';return 0;
            }
            constructor_units=0;constructor_stores=0;
            std::array<Controls,multiplayer_capacity> input{};
            input[slot].yaw=.375f;input[slot].pitch=-.125f;
            match.step(input);const auto delivered=match.state();const auto &next=delivered.players[slot];
            need(next.source_unit==player.source_unit && next.first_person_pose &&
                next.first_person_pose->temporal.generation==generation && delivered.tick==born.tick+1u,
                "First genuine input replaced or failed to advance the birth FP owner");
            need(word(retained)==player.source_unit && word(retained+0x94u)==player.source_player &&
                word(retained+0x98u)==std::bit_cast<std::uint32_t>(input[slot].yaw) &&
                word(retained+0x9cu)==std::bit_cast<std::uint32_t>(input[slot].pitch),
                "First genuine command did not retain its full player and raw look baseline");
            need(constructor_units==0,"First genuine input reconstructed an already retained birth look owner");
            report.update({{"passed",true},{"representation",representation},{"route",route},
                {"first_birth_packet",true},{"birth_raw_baseline_none",true},
                {"remote_original_constructor_compared",route=="remote"},
                {"local_original_producer_record_compared",route=="local"},
                {"birth_constructor_observed_stores",birth_constructor_stores},
                {"remote_preserves_actual_local_input",route=="remote"},
                {"first_genuine_input_keeps_generation",true},{"birth_tick",born.tick},
                {"first_input_tick",delivered.tick}});
            save();std::cout<<"PASS first native FP birth and genuine input "<<route<<'\n';return 0;
        }
        const auto weapon=world.cache->slayer_starting_weapon(5);
        SourceMatchAuthority authority(match,*world.source_objects);
        std::array<Controls,multiplayer_capacity> controls{};unsigned dual_frames{};
        std::uint64_t generation{},last_actions{};
        const auto initial_packet=match.state().players[slot].first_person_pose;
        const auto initial_actions=initial_packet?initial_packet->temporal.accepted_action_sequence:0u;
        std::array<bool,2> actual_shots{};
        std::map<std::uint32_t,unsigned> action_store_counts;
        report["native_action_stores"]=nlohmann::json::array();unsigned observed_step{};
        const auto &action_memory=world.source_objects->memory();
        world.source_objects->observe_physics_stores_read_only([&](std::uint32_t entry,const auto &store) {
            if(store.instruction!=0xf0d50u && store.instruction!=0xf0df1u && store.instruction!=0x16b736u)return;
            ++action_store_counts[store.instruction];
            if(report["native_action_stores"].size()<128) {
                nlohmann::json row{{"step",observed_step},{"source_observer_entry",entry},
                    {"pc",store.instruction},{"address",store.address},
                    {"word",store.written.size()==4?Bytes(store.written).u32(0):0u}};
                if(store.instruction==0x16b736u) {
                    const auto word=[&](std::uint32_t offset){return Bytes(action_memory.view(store.address+offset,4)).u32(0);};
                    row.update({{"native_return",word(4)}, {"view",word(8)},
                        {"weapon",word(12)}, {"hand",word(16)}});
                }
                report["native_action_stores"].push_back(std::move(row));
            }
        });
        struct ActionUnobserve {
            MultiplayerObjectRuntime &owner;bool active=true;
            void clear(){if(active){owner.observe_physics_stores_read_only({});active=false;}}
            ~ActionUnobserve(){clear();}
        } action_unobserve{*world.source_objects};
        for(unsigned tick=0;tick<150 && dual_frames<48;++tick) {
            observed_step=tick;
            const auto before=match.state();const auto &player=before.players[slot];controls={};
            if(player.source_unit!=null_datum && player.secondary_weapon.datum==null_datum) {
                const auto item=std::find_if(before.world_weapons.begin(),before.world_weapons.end(),
                    [&](const auto &entry){return entry.definition==weapon;});
                if(item!=before.world_weapons.end()) {
                    world.source_objects->tu15_world().set_object_position(player.source_unit,item->position+Vec3{-.2f,0,0});
                    controls[slot].dual_wield=true;controls[slot].dual_wield_milliseconds=1000;
                }
            }
            if(dual_frames>=8 && dual_frames<20)controls[slot].fire=controls[slot].fire_secondary=true;
            if(dual_frames>=20 && dual_frames<32)controls[slot].fire_secondary=true;
            if(dual_frames==36)controls[slot].reload=true;
            match.step(controls);const auto state=match.state();const auto &current=state.players[slot];
            if(current.source_unit==null_datum || current.dead)continue;
            need(current.first_person_pose.has_value(),"Actual native Match phase did not publish its player FP input packet");
            const auto &packet=*current.first_person_pose;const auto &temporal=packet.temporal;
            need(temporal.source_tick+1u==state.tick && temporal.identity.player==current.source_player &&
                temporal.identity.unit==current.source_unit && temporal.identity.weapons[0]==current.source_weapon &&
                temporal.identity.weapons[1]==current.secondary_weapon.datum,
                "Match FP publication lost exact source tick/full player/unit/hand ownership");
            need(!generation || temporal.generation==generation,"Same unit unexpectedly changed FP lifetime generation");
            need(temporal.accepted_action_sequence>=last_actions,"Native action prefix sequence regressed");
            generation=temporal.generation;last_actions=temporal.accepted_action_sequence;
            if(current.secondary_weapon.datum!=null_datum)++dual_frames;
            const auto &unit=match.unit(slot);const auto &memory=world.source_objects->memory();
            const auto body=world.source_objects->source_object_body(current.source_unit);
            const auto read=[&](std::uint32_t offset){return Bytes(memory.view(body+offset,4)).u32(0);};
            nlohmann::json hands=nlohmann::json::array();
            for(unsigned hand=0;hand<2;++hand)if(const auto *weapon=native_hand_weapon(unit,hand)) {
                actual_shots[hand]=actual_shots[hand] || weapon->last_shot!=0;
                hands.push_back({{"hand",hand},{"object",weapon->source_datum},{"loaded",weapon->loaded},
                    {"reserve",weapon->reserve},{"last_shot",weapon->last_shot},{"magazine_mode",weapon->magazine.mode},
                    {"magazine_ticks",weapon->magazine.ticks},{"trigger_state",weapon->smg.trigger_state},
                    {"trigger_ticks",weapon->smg.trigger_ticks},{"input_flags",weapon->smg.input_flags},
                    {"weapon_flags",weapon->smg.weapon_flags},
                    {"hand_timers",{weapon->smg.hand_timers[0],weapon->smg.hand_timers[1]}}});
            }
            report["rows"].push_back({{"tick",state.tick},{"generation",generation},{"actions",last_actions},
                {"dual_frames",dual_frames},{"fire",controls[slot].fire},{"fire_secondary",controls[slot].fire_secondary},
                {"reload",controls[slot].reload},{"actual_unit_148",read(0x148u)},
                {"actual_primary_trigger_bits",read(0x1c0u)},{"actual_secondary_trigger_bits",read(0x1c4u)},
                {"hands",std::move(hands)}});
        }
        action_unobserve.clear();report["native_action_store_counts"]=nlohmann::json::array();
        for(const auto &[pc,count]:action_store_counts)report["native_action_store_counts"].push_back({{"pc",pc},{"count",count}});
        need(dual_frames==48,"Native registry fixture did not complete 48 actual dual ticks");
        const auto final_packet=match.state().players[slot].first_person_pose;
        need(final_packet.has_value(),"Final native action witness lost its published FP owner");
        std::uint64_t actual_requests{};unsigned weapon_requests{};
        std::array<unsigned,2> per_hand_requests{};
        for(const auto &event:report["native_action_stores"])if(event["pc"]==0x16b736u) {
            for(unsigned hand=0;hand<2;++hand)if(event["weapon"]==final_packet->temporal.identity.weapons[hand]) {
                ++actual_requests;++per_hand_requests[hand];
                if(event["native_return"]!=0xf0df6u)++weapon_requests;
            }
        }
        report.update({{"initial_action_sequence",initial_actions},{"accepted_action_delta",last_actions-initial_actions},
            {"actual_owned_native_requests",actual_requests},{"actual_owned_weapon_requests",weapon_requests},
            {"actual_owned_requests_by_hand",per_hand_requests},{"actual_native_shots_by_hand",actual_shots}});
        need(actual_requests>1u && weapon_requests>0u && last_actions-initial_actions==actual_requests,
            "Native action sequence does not match every actual owned pickup/fire/reload dispatch");
        need(actual_shots[0] && actual_shots[1] && per_hand_requests[0] && per_hand_requests[1],
            "Actual native fire requests did not exercise both dual-wielded hands");
        const auto cut=authority.checkpoint(binding);
        need(cut.match.native_first_person && cut.match.native_first_person->players[slot].context,
            "Authority continuation omitted the full private FP owner");
        const auto before_rejection=encoded(match.state().players[slot]);
        // Record the actual private130860 geometry writes against the fresh
        // native tag table. These bytes must survive a genuine restore while
        // adjacent asset metadata and unrelated model owners remain borrowed.
        const auto &resource_memory=world.source_objects->memory();
        const auto word=[&](std::uint32_t at) {
            std::array<std::byte,4> bytes;resource_memory.read(at,bytes);return Bytes(bytes).u32(0);
        };
        const auto private_byte=[](const auto &context,std::uint32_t at)->std::optional<std::byte> {
            for(const auto &range:context.private_ranges)
                if(range.address<=at && std::uint64_t(at)-range.address<range.bytes.size())
                    return range.bytes[at-range.address];
            return {};
        };
        const auto replace_byte=[](auto &context,std::uint32_t at,std::byte value) {
            for(auto &range:context.private_ranges)
                if(range.address<=at && std::uint64_t(at)-range.address<range.bytes.size()) {
                    range.bytes[at-range.address]=value;return;
                }
            context.private_ranges.push_back({at,{value}});
            std::sort(context.private_ranges.begin(),context.private_ranges.end(),
                [](const auto &a,const auto &b){return a.address<b.address;});
        };
        std::vector<std::pair<std::uint32_t,std::byte>> queued_flags;
        std::uint32_t foreign_geometry{};
        report["private_geometry_queued_flags"]=nlohmann::json::array();
        for(const auto &tag:world.cache->select_tags({.group=TagGroup::from_string("mode"),.require_address=true})) {
            const auto row=word(0x4f0414u)+(tag.datum&65535u)*16u,root=word(row+8u),bytes=word(row+12u);
            if(bytes<0x2cu)continue;
            const auto count=word(root+0x24u),sections=word(root+0x28u);
            if(!count || sections<root || std::uint64_t(sections)-root+std::uint64_t(count)*0x5cu>bytes)continue;
            if(tag.name=="objects\\weapons\\rifle\\smg\\smg")foreign_geometry=sections+0x38u;
            for(unsigned section=0;section<count;++section) {
                const auto at=sections+section*0x5cu+0x57u;
                if(const auto value=private_byte(*cut.match.native_first_person->players[slot].context,at)) {
                    std::array<std::byte,1> fresh;resource_memory.read(at,fresh);
                    need(((*value^fresh[0])&std::byte{0xfb})==std::byte{},
                        "Actual native resource write changed bits outside the queued flag");
                    queued_flags.emplace_back(at,*value);
                    report["private_geometry_queued_flags"].push_back({{"tag",tag.datum},{"name",tag.name},
                        {"section",section},{"address",at},{"fresh",std::to_integer<unsigned>(fresh[0])},
                        {"private",std::to_integer<unsigned>(*value)}});
                }
            }
        }
        if(route=="remote")need(std::any_of(report["private_geometry_queued_flags"].begin(),
            report["private_geometry_queued_flags"].end(),[](const auto &flag){return flag["fresh"]==1 && flag["private"]==5;}),
            "Remote native fixture did not retain the genuine resource byte1-to5 write");
        {
            auto staged_world=world.source_objects->prepare_authority(cut.world);
            const auto caller=staged_world.replacement_caller();
            const auto &replacement=staged_world.replacement_memory();
            bool actual_stack{};
            replacement.visit_committed([&](std::uint32_t at,std::span<const std::byte> bytes) {
                if(std::uint64_t(at)<=caller.gp[4] && caller.gp[4]<std::uint64_t(at)+bytes.size()) {
                    const auto saved_stack=cut.match.native_first_person->players[slot].context->caller_stack;
                    actual_stack=std::uint64_t(at)<=saved_stack && saved_stack<std::uint64_t(at)+bytes.size();
                }
            });
            need(caller.gp[4]!=0 && caller.floating_point.has_value() && actual_stack,
                "Staged native caller differs from the retained genuine phase stack owner");
            bool missing_caller_rejected{};
            try{auto invalid=match.prepare_authority(cut.match,binding,&replacement);(void)invalid;}
            catch(const FormatError &error) {
                missing_caller_rejected=std::string_view(error.what())==
                    "Native FP continuation requires its staged native caller";
            }
            need(missing_caller_rejected,"Native FP continuation accepted an absent staged caller ABI");
            report.update({{"staged_native_caller_esp",caller.gp[4]},
                {"staged_native_caller_matches_saved_stack_owner",actual_stack},
                {"missing_staged_native_caller_rejected",missing_caller_rejected}});
        }
        need(encoded(match.state().players[slot])==before_rejection,
            "Staged caller validation changed the live FP publication");
        for(unsigned invalid=0;invalid<9;++invalid) {
            auto bad=cut;auto &registry=*bad.match.native_first_person;auto &entry=registry.players[slot];
            if(invalid==0)entry.context->initialized=false;
            if(invalid==1)entry.packet.reset();
            if(invalid==2)++*entry.context->last_advanced_tick;
            if(invalid==3)++entry.context->temporal.source_tick;
            if(invalid==4)registry.resources.resident[0]^=std::byte{1};
            if(invalid==5)bad.match.native_first_person.reset();
            if(invalid==6)++entry.context->temporal.generation;
            if(invalid==7)++entry.context->temporal.accepted_action_sequence;
            if(invalid==8)entry.packet->temporal.clock_controls[0]^=std::byte{1};
            bool rejected{};
            try{auto staged_bad=authority.prepare(bad,binding);(void)staged_bad;}
            catch(const FormatError&){rejected=true;}
            need(rejected && encoded(match.state().players[slot])==before_rejection,
                "Invalid FP continuation was admitted or modified the live owner while preparing");
        }
        unsigned geometry_attacks{};
        if(!queued_flags.empty()) {
            need(foreign_geometry!=0,"Native geometry attack lacks the actual third-person SMG model");
            const auto reject_geometry=[&](auto bad) {
                bool rejected{};
                try{auto staged_bad=authority.prepare(bad,binding);(void)staged_bad;}
                catch(const FormatError&){rejected=true;}
                need(rejected && encoded(match.state().players[slot])==before_rejection,
                    "Native geometry continuation admitted unrelated asset bytes or changed its live owner");
                ++geometry_attacks;
            };
            const auto [flag,value]=queued_flags.front();
            const auto tag=report["private_geometry_queued_flags"].front()["tag"].get<std::uint32_t>();
            const auto tag_row=word(0x4f0414u)+(tag&65535u)*16u;
            for(const auto address:{flag-1u,flag+1u,foreign_geometry+0x1fu,word(tag_row+8u)+0x28u,
                tag_row+8u,0x4ecc1cu}) {
                auto bad=cut;std::array<std::byte,1> actual;resource_memory.read(address,actual);
                replace_byte(*bad.match.native_first_person->players[slot].context,address,actual[0]|std::byte{4});
                reject_geometry(std::move(bad));
            }
            auto other_bit=cut;replace_byte(*other_bit.match.native_first_person->players[slot].context,
                flag,value^std::byte{2});reject_geometry(std::move(other_bit));
            auto missing_flag=cut;replace_byte(*missing_flag.match.native_first_person->players[slot].context,
                flag,value&std::byte{0xfb});reject_geometry(std::move(missing_flag));
            const auto queue=word(0x4f2d60u),rows=word(queue+0x44u),bitmap=word(queue+0x48u);
            const auto &context=*cut.match.native_first_person->players[slot].context;
            const auto retained_word=[&](std::uint32_t at) {
                std::array<std::byte,4> bytes;resource_memory.read(at,bytes);
                for(unsigned n=0;n<4u;++n)if(const auto value=private_byte(context,at+n))bytes[n]=*value;
                return Bytes(bytes).u32(0);
            };
            bool paired{};
            for(unsigned index=0;index<0x15eu;++index)if((retained_word(bitmap+(index/32u)*4u)&(1u<<(index%32u))) &&
                retained_word(rows+index*8u+4u)==flag-0x1fu) {
                auto bad=cut;auto &private_context=*bad.match.native_first_person->players[slot].context;
                for(unsigned n=0;n<4u;++n)replace_byte(private_context,rows+index*8u+4u+n,
                    std::byte(foreign_geometry>>(n*8u)));
                reject_geometry(std::move(bad));paired=true;break;
            }
            need(paired && geometry_attacks==9u,"Private queued geometry flag has no actual native request row or complete attack coverage");
        }
        report["geometry_owner_attacks_rejected"]=geometry_attacks;
        controls={};controls[slot].fire=controls[slot].fire_secondary=true;
        const auto phase=authority.capture_execution_phase([&]{for(unsigned n=0;n<3;++n)match.step(controls);});
        const auto expected=encoded(match.state().players[slot]);
        auto staged=authority.prepare(cut,binding);need(authority.commit(std::move(staged)),"Native FP owner continuation failed publication");
        const auto restored_resources=authority.checkpoint(binding);
        for(const auto &[address,value]:queued_flags)
            need(private_byte(*restored_resources.match.native_first_person->players[slot].context,address)==value,
                "Genuine native restore lost its privately queued resource flag");
        authority.replay_execution_phase(phase);
        const auto actual=encoded(match.state().players[slot]);
        need(actual==expected,"Restored native FP registry diverged under actual recorded MatchStep replay");
        const auto original_fp=world.cache->load_first_person_binding(weapon,representation);
        if(route=="remote") {
            // Genuine inventory transitions must retire an old private SMG
            // request before its model leaves every fresh binding. No physical
            // SMG view may accidentally authorize the historical asset byte.
            const auto original_history=authority.checkpoint(binding);
            std::vector<std::uint32_t> old_smg_flags;
            for(const auto &flag:report["private_geometry_queued_flags"])
                if(flag["name"]=="objects\\weapons\\rifle\\smg\\fp_smg\\fp_smg") {
                    const auto address=flag["address"].template get<std::uint32_t>();
                    const auto value=private_byte(*original_history.match.native_first_person->players[slot].context,address);
                    need(value && (*value&std::byte{4})!=std::byte{},
                        "Historical fixture lost the genuine privately queued SMG request before transition");
                    old_smg_flags.push_back(address);
                }
            need(!old_smg_flags.empty(),"Historical fixture did not begin with a genuine private SMG resource request");
            unsigned replacements{};
            report["geometry_retirement_history"]=nlohmann::json::array();
            const auto history_sample=[&](const char *stage,const MultiplayerState &state) {
                const auto &player=state.players[slot];
                const auto &memory=world.source_objects->memory();
                const auto read=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
                const auto body=world.source_objects->source_object_body(player.source_unit);
                nlohmann::json slots=nlohmann::json::array();
                for(unsigned index=0;index<4u;++index) {
                    const auto item=read(body+0x218u+index*4u);
                    slots.push_back({{"index",index},{"object",item},{"definition",item==null_datum?null_datum:
                        read(world.source_objects->source_object_body(item))},{"ready_tick",read(body+0x228u+index*4u)}});
                }
                report["geometry_retirement_history"].push_back({{"stage",stage},{"replacement",replacements},
                    {"tick",state.tick},{"weapon",player.weapon},{"source_weapon",player.source_weapon},
                    {"secondary_weapon",player.secondary_weapon.definition},{"secondary_object",player.secondary_weapon.datum},
                    {"source_action_word",player.source_action_word},{"current_primary",read(body+0x212u)&255u},
                    {"current_secondary",read(body+0x213u)&255u},{"selection_214",read(body+0x214u)},
                    {"switch_weapon",controls[slot].switch_weapon},{"pickup",controls[slot].pickup},
                    {"pickup_milliseconds",controls[slot].pickup_milliseconds},{"dual_wield",controls[slot].dual_wield},
                    {"dual_wield_milliseconds",controls[slot].dual_wield_milliseconds},
                    {"fire",controls[slot].fire},{"fire_secondary",controls[slot].fire_secondary},{"reload",controls[slot].reload},
                    {"slots",std::move(slots)}});
            };
            auto history_state=match.state();
            const auto inventory_has_smg=[&] {
                const auto &m=world.source_objects->memory();
                const auto read=[&](std::uint32_t at){return Bytes(m.view(at,4)).u32(0);};
                const auto rows=read(read(0x4ecbd0u)+0x44u);
                const auto unit=history_state.players[slot].source_unit,body=read(rows+(unit&65535u)*12u+8u);
                for(unsigned n=0;n<4u;++n) {
                    const auto item=read(body+0x218u+n*4u);
                    if(item!=null_datum && read(read(rows+(item&65535u)*12u+8u))==weapon)return true;
                }
                return false;
            };
            while(inventory_has_smg() && replacements<3u) {
                history_sample("before-select",history_state);
                for(unsigned tick=0;tick<45u && history_state.players[slot].weapon!=weapon;++tick) {
                    controls={};controls[slot].switch_weapon=tick==0;match.step(controls);
                    history_state=match.state();history_sample("after-select-step",history_state);
                }
                need(history_state.players[slot].weapon==weapon,
                    "Native history fixture could not select the retained SMG to replace");
                const auto before=history_state;
                const auto &pickup_memory=world.source_objects->memory();
                const auto pickup_word=[&](std::uint32_t at){return Bytes(pickup_memory.view(at,4)).u32(0);};
                const auto model_candidate=[&](const auto &entry) {
                    if(entry.definition==weapon || entry.definition==null_datum)return false;
                    const auto root=pickup_word(pickup_word(0x4f0414u)+(entry.definition&65535u)*16u+8u);
                    const auto count=pickup_word(root+0x2a8u),records=pickup_word(root+0x2acu);
                    if(!count)return false;
                    const unsigned preferred=representation==3u?1u:0u;
                    const auto model=pickup_word(records+(count>preferred?preferred:0u)*0x10u+4u);
                    return model!=null_datum && model!=original_fp.weapon_model;
                };
                auto item=std::find_if(before.world_weapons.begin(),before.world_weapons.end(),model_candidate);
                need(item!=before.world_weapons.end(),"Native history fixture lacks a different authored FP pickup");
                const auto body=world.source_objects->source_object_body(before.players[slot].source_unit);
                std::array<std::uint32_t,4> inventory_objects{},inventory_definitions{};
                nlohmann::json inventory=nlohmann::json::array();
                for(unsigned index=0;index<4u;++index) {
                    const auto object=pickup_word(body+0x218u+index*4u);
                    inventory_objects[index]=object;inventory_definitions[index]=object==null_datum?null_datum:
                        pickup_word(world.source_objects->source_object_body(object));
                    inventory.push_back({{"index",index},{"object",object},{"definition",inventory_definitions[index]}});
                }
                const auto owned_definition=[&](std::uint32_t definition) {
                    return std::ranges::find(inventory_definitions,definition)!=inventory_definitions.end();
                };
                const auto candidate_identity=[&](const auto &entry) {
                    const auto target_body=world.source_objects->source_object_body(entry.handle);
                    return nlohmann::json{{"object",entry.handle},{"snapshot_definition",entry.definition},
                        {"native_definition",pickup_word(target_body)},{"native_parent",pickup_word(target_body+0x14u)},
                        {"owned_definition",owned_definition(entry.definition)},
                        {"owned_object",std::ranges::find(inventory_objects,entry.handle)!=inventory_objects.end()}};
                };
                const auto old_item=item;
                const bool exclude_owned=owned_definition(old_item->definition);
                report["geometry_retirement_pickup_selection"].push_back({{"replacement",replacements},{"tick",before.tick},
                    {"old_candidate",candidate_identity(*old_item)},{"inventory",inventory},
                    {"conditional_owned_definition_filter",exclude_owned},{"excluded_candidates",nlohmann::json::array()}});
                auto &selection=report["geometry_retirement_pickup_selection"].back();
                if(exclude_owned) {
                    for(const auto &entry:before.world_weapons)
                        if(model_candidate(entry) && owned_definition(entry.definition))
                            selection["excluded_candidates"].push_back(candidate_identity(entry));
                    item=std::find_if(before.world_weapons.begin(),before.world_weapons.end(),[&](const auto &entry) {
                        return model_candidate(entry) && !owned_definition(entry.definition);
                    });
                }
                need(item!=before.world_weapons.end(),"Native history fixture lacks an unowned different authored FP pickup");
                selection["chosen_candidate"]=candidate_identity(*item);
                selection["old_candidate_preserved_when_unowned"]=exclude_owned || item==old_item;
                const auto target=item->definition;
                world.source_objects->tu15_world().set_object_position(before.players[slot].source_unit,
                    item->position+Vec3{-.2f,0,0});
                for(unsigned tick=0;tick<45u && history_state.players[slot].weapon!=target;++tick) {
                    controls={};controls[slot].pickup=tick%2u==0;controls[slot].pickup_milliseconds=1000;
                    match.step(controls);
                    history_state=match.state();history_sample("after-pickup-step",history_state);
                }
                need(history_state.players[slot].weapon==target,
                    "Native history fixture did not equip its different authored FP model");
                ++replacements;
            }
            need(!inventory_has_smg(),"Native history fixture retained its old SMG inventory binding");
            const auto &history_memory=world.source_objects->memory();
            const auto history_word=[&](std::uint32_t at){return Bytes(history_memory.view(at,4)).u32(0);};
            const auto fp_views=history_word(0x4f6498u),object_rows=history_word(history_word(0x4ecbd0u)+0x44u);
            for(unsigned view=0;view<4u;++view) {
                const auto unit=history_word(fp_views+view*0x20ccu+4u);if(unit==null_datum)continue;
                const auto body=history_word(object_rows+(unit&65535u)*12u+8u);
                for(unsigned n=0;n<4u;++n) {
                    const auto item=history_word(body+0x218u+n*4u);
                    need(item==null_datum || history_word(history_word(object_rows+(item&65535u)*12u+8u))!=weapon,
                        "Physical SMG view masked the obsolete private request retirement");
                }
            }
            const auto history_cut=authority.checkpoint(binding);
            const auto before_packet=encoded(match.state().players[slot]);
            const auto before_context=history_cut.match.native_first_person->players[slot].context->temporal;
            const auto &history_context=*history_cut.match.native_first_person->players[slot].context;
            const auto history_retained_word=[&](std::uint32_t at) {
                std::array<std::byte,4> bytes;history_memory.read(at,bytes);
                for(unsigned n=0;n<4u;++n)if(const auto value=private_byte(history_context,at+n))bytes[n]=*value;
                return Bytes(bytes).u32(0);
            };
            const auto history_queue=history_word(0x4f2d60u),history_rows=history_word(history_queue+0x44u);
            const auto history_bitmap=history_word(history_queue+0x48u);
            for(const auto address:old_smg_flags) {
                need(!private_byte(history_context,address),
                    "Obsolete native SMG queued byte retained private ownership after drop");
                for(unsigned index=0;index<0x15eu;++index)
                    need(!(history_retained_word(history_bitmap+(index/32u)*4u)&(1u<<(index%32u))) ||
                        history_retained_word(history_rows+index*8u+4u)!=address-0x1fu,
                        "Obsolete native SMG request retained its active private queue row after drop");
            }
            std::optional<SourceMatchAuthority::Prepared> prepared;
            std::uint64_t source_writes{},source_topology{};
            const auto prepare_history=[&](std::uint64_t &writes,std::uint64_t &topology) {
                SourceRuntimeWriteSerial serial(history_memory);
                source_runtime_register_topology_observer(history_memory,&topology,
                    [&](std::uint32_t,std::size_t){++topology;});
                struct TopologyUnobserve {
                    const SourceRuntimeMemory &memory;const void *owner;
                    ~TopologyUnobserve(){source_runtime_unregister_topology_observer(memory,owner);}
                } unobserve{history_memory,&topology};
                prepared.emplace(authority.prepare(history_cut,binding));writes=serial.load();
            };
            report["history_restore_stage"]="preparing-cancellation";
            prepare_history(source_writes,source_topology);
            need(!source_writes && !source_topology,
                "Historical private resource restore changed World bytes or topology");
            // A Prepared owns the native staging barrier. Cancel it before
            // querying the live Match, then prepare the actual publication
            // separately under the same zero-World-write/topology monitors.
            prepared.reset();report["history_restore_stage"]="cancelled";
            need(encoded(match.state().players[slot])==before_packet,
                "Preparing the historical resource restore changed the live native packet");
            std::uint64_t publication_writes{},publication_topology{};
            report["history_restore_stage"]="preparing-publication";
            prepare_history(publication_writes,publication_topology);
            need(!publication_writes && !publication_topology,
                "Historical publication preparation changed World bytes or topology");
            need(authority.commit(std::move(*prepared)),"Native historical resource continuation failed publication");
            report["history_restore_stage"]="committed";
            const auto after_cut=authority.checkpoint(binding);
            const auto &before_ranges=history_cut.match.native_first_person->players[slot].context->private_ranges;
            const auto &after_ranges=after_cut.match.native_first_person->players[slot].context->private_ranges;
            const bool same_private=before_ranges.size()==after_ranges.size() &&
                std::equal(before_ranges.begin(),before_ranges.end(),after_ranges.begin(),
                    [](const auto &a,const auto &b){return a.address==b.address && a.bytes==b.bytes;});
            need(same_private && after_cut.match.native_first_person->players[slot].context->temporal==before_context &&
                encoded(match.state().players[slot])==before_packet,
                "Historical graphics retirement changed POST clocks, actions, RNG or the native packet");
            report.update({{"obsolete_smg_request_retired",true},{"history_pickup_replacements",replacements},
                {"history_restore_preserves_post_state",true},{"history_has_no_physical_smg_owner",true},
                {"history_cancelled_preparations",1},{"history_published_preparations",1},
                {"history_cancel_preserves_live_packet",true},
                {"history_cancel_prepare_world_writes",source_writes},{"history_cancel_prepare_world_topology",source_topology},
                {"history_prepare_world_writes",publication_writes},{"history_prepare_world_topology",publication_topology}});
        }
        const auto fp=world.cache->load_first_person_binding(match.state().players[slot].weapon,representation);
        bool local=route=="local";
        for(unsigned change=0;change<2;++change) {
            const auto before=match.state();const auto bytes=encoded(before.players[slot]);local=!local;
            const auto role_cut=authority.checkpoint(binding);
            const auto &continued=role_cut.match.native_first_person->players[slot].context->temporal;
            const std::array selected{slot};
            match.configure_local_views(local?std::span<const unsigned>(selected):std::span<const unsigned>{},fp);
            const auto rebound=match.state();
            need(rebound.tick==before.tick && encoded(rebound.players[slot])==bytes,
                "Physical view rebind advanced or replaced genuine completed FP temporal inputs");
            if(local) {
                auto installed=SourceFirstPersonContextTu15::capture_local(*world.cache,
                    world.source_objects->memory(),0,continued.identity,continued.generation,
                    continued.accepted_action_sequence);
                need(installed.source_tick==before.tick && continued.source_tick+1u==before.tick,
                    "Native FP role rebind changed the actual completed source clock");
                // The portable packet owns PREcomposition inputs. The actual
                // view must continue from the later POSTcomposition clocks.
                installed.source_tick=continued.source_tick;
                need(installed==continued,"Native FP role rebind installed display inputs over its later native clocks");
            }
            for(unsigned n=0;n<3;++n)match.step({});
            const auto advanced=match.state();
            need(advanced.players[slot].first_person_pose && advanced.tick==before.tick+3u &&
                advanced.players[slot].first_person_pose->temporal.generation==generation,
                "Native FP owner failed the first genuine phase after a physical view role change");
        }
        need(match.depart(slot),"Native registry departure fixture did not depart its actual player");
        const auto departed=authority.checkpoint(binding);
        need(!match.state().players[slot].first_person_pose && departed.match.native_first_person &&
            !departed.match.native_first_person->players[slot].context &&
            !departed.match.native_first_person->players[slot].packet,
            "Departed native player retained an active FP producer owner");
        report.update({{"passed",true},{"representation",representation},{"route",route},{"dual_frames",dual_frames},
            {"actions",last_actions},{"recorded_replay_packet_identical",true},
            {"malformed_continuations_rejected",9},{"both_physical_role_transitions",true},
            {"role_preserves_post_composition_clocks",true},
            {"retirement_clears_owner",true}});
        save();std::cout<<"PASS actual native FP Match registry, private continuation and replay "<<route<<'\n';return 0;
    }catch(const std::exception &error){report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;}
}
