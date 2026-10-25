#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_shadow_native_prefix_tu15.hpp"
#include "halo2/math.hpp"
#include "ordinary_multiplayer_trace.hpp"
#include <SDL.h>
#include <vulkan/vulkan.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <charconv>
#include <cfenv>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char *message){if(!value)throw FormatError(message);}
struct Environment {
    const char *name;std::optional<std::string> previous;
    explicit Environment(const char *key):name(key){if(const auto *p=std::getenv(key))previous=p;}
    void set(const char *value){if(value)setenv(name,value,1);else unsetenv(name);}
    ~Environment(){if(previous)setenv(name,previous->c_str(),1);else unsetenv(name);}
};
std::string sha256(std::span<const std::byte> bytes) {
    auto context=std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    need(bool(context),"Allocate SHA256 context");std::array<unsigned char,32> digest{};unsigned size{};
    need(EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)==1 &&
        EVP_DigestUpdate(context.get(),bytes.data(),bytes.size())==1 &&
        EVP_DigestFinal_ex(context.get(),digest.data(),&size)==1 && size==digest.size(),"Compute SHA256");
    std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto byte:digest)out<<std::setw(2)<<unsigned(byte);return out.str();
}
struct Words {
    std::vector<std::byte> bytes;
    void word(std::uint64_t value){for(unsigned i=0;i<8;++i)bytes.push_back(std::byte(value>>(i*8)));}
    void real(float value){word(std::bit_cast<std::uint32_t>(value));}
    void vector(Vec3 value){real(value.x);real(value.y);real(value.z);}
    void transform(const Transform &value){real(value.scale);for(auto basis:value.basis)vector(basis);vector(value.position);}
    void camera(const Camera &value){vector(value.position);real(value.yaw);real(value.pitch);real(value.field_of_view);word(value.horizontal_field_of_view);word(value.oriented);vector(value.forward);vector(value.up);}
    void controls(const Controls &c) {
        for(float value:{c.forward,c.right,c.yaw,c.pitch,c.look_stick})real(value);
        for(bool value:{c.jump,c.crouch,c.fire,c.reload,c.skip_cinematic,c.grenade,c.pickup,c.dual_wield,c.switch_weapon,
            c.fire_secondary,c.switch_grenade,c.accept,c.cancel,c.melee,c.flashlight,c.start,c.back})word(value);
        word(c.zoom);word(c.zoom_reset_serial);word(c.pickup_milliseconds);word(c.dual_wield_milliseconds);
        word(c.vehicle_interaction_target.has_value());if(c.vehicle_interaction_target) {
            word(c.vehicle_interaction_target->action);word(c.vehicle_interaction_target->index);word(c.vehicle_interaction_target->object);
        }
    }
    void state(const MultiplayerState &s) {
        word(s.tick);word(s.source_game_type);word(s.source_engine);word(s.finished);word(s.results_finalized);
        word(s.game_over_countdown);word(s.game_over_fade_started);word(s.winner);word(s.winner_team);
        word(s.round_mode);word(s.life_limit);word(s.time_ticks);word(s.clock_seconds);word(s.team_mode);word(s.retained_team_mask);
        for(auto value:s.team_scores)word(value);for(auto value:s.team_total_scores)word(value);for(auto value:s.team_wins)word(value);
        for(const auto &bank:s.team_statistics)for(auto value:bank)word(value);
        for(const auto &p:s.players) {
            word(p.connected);word(p.dead);vector(p.position);vector(p.velocity);
            for(float value:{p.yaw,p.pitch,p.crouch,p.health,p.shield,p.reload_seconds,p.native_forward,p.native_right})real(value);
            for(auto value:{p.weapon,p.source_player,p.source_unit,p.source_weapon,p.shot_tick,p.reload_tick,
                p.source_parent,p.source_action_word,p.source_previous_unit,p.source_death_tick,p.source_teleport_serial,
                p.source_teleport_sound,p.source_hud_target_flags,p.zoom_reset_serial})word(value);
            for(int value:{p.loaded,p.reserve,p.score,p.kills,p.deaths,p.assists,p.suicides,p.ammunition_available})word(value);
            word(p.reload_empty);word(p.native_grounded);word(p.native_crouching);word(p.native_support_material);
            word(p.source_flags);word(p.source_membership);word(p.source_ffa_identity);word(p.source_seat);
            word(p.source_lives_remaining);word(p.source_king_inside_ticks);word(p.source_respawn_countdown);
            for(auto value:p.grenades)word(value);word(p.grenade_current);
            word(p.source_zoom.current);word(p.source_zoom.requested);word(p.source_zoom.motion);word(p.source_zoom_magazine_mode);
            word(p.source_hud_zoom_mask);word(p.source_hud_autoaimed);word(p.source_unit_10a);word(p.motion_sensor_control);
            word(p.source_configuration.has_value());if(p.source_configuration)for(auto byte:*p.source_configuration)word(byte);
            const auto trigger=[&](const NativeWeaponTriggerPublication &t) {
                word(t.input_flags);word(t.ticks);word(t.mode);word(t.charge);word(t.target_acquired);
                word(t.target_position.has_value());if(t.target_position)for(auto value:*t.target_position)real(value);
            };
            trigger(p.weapon_trigger);trigger(p.secondary_weapon.trigger);
            const auto &secondary=p.secondary_weapon;
            word(secondary.definition);word(secondary.datum);word(secondary.shot_tick);word(secondary.reload_tick);
            word(secondary.loaded);word(secondary.reserve);real(secondary.reload_seconds);word(secondary.reload_empty);
            word(secondary.ammunition_available);word(secondary.magazine_mode);
            for(const auto *recent:{&p.damage_feedback.shield,&p.damage_feedback.body}) {
                real(recent->visible);real(recent->accumulated);word(recent->ticks);
            }
            word(p.shield_recharging);real(p.source_camouflage.primary);real(p.source_camouflage.secondary);
        }
        word(s.source_body_poses_authoritative);word(s.source_body_poses.size());
        for(const auto &pose:s.source_body_poses) {
            word(pose.object);word(pose.nodes.size());for(const auto &node:pose.nodes)transform(node);
            vector(pose.sphere.center);real(pose.sphere.radius);
        }
        word(s.world_weapons.size());for(const auto &w:s.world_weapons) {
            word(w.handle);word(w.definition);vector(w.position);vector(w.forward);vector(w.up);word(w.loaded);word(w.reserve);
        }
        word(s.world_vehicles.size());for(const auto &v:s.world_vehicles){word(v.handle);word(v.placement);transform(v.transform);}
        word(s.source_scene_roots_authoritative);word(s.source_scene_roots.size());
        for(const auto &r:s.source_scene_roots) {
            word(r.object);word(r.definition);word(r.placement);transform(r.transform);vector(r.velocity);
            word(r.visible);word(r.has_world_nodes);word(r.runtime_flags);word(r.frozen);word(r.created_tick);
            word(r.render_clusters.size());for(auto cluster:r.render_clusters)word(cluster);
        }
        word(s.bullet_impacts.size());for(const auto &e:s.bullet_impacts) {
            word(e.serial);word(e.tick);word(e.projectile);word(e.weapon);vector(e.position);vector(e.normal);vector(e.incoming);word(e.material);word(e.terrain);
        }
        word(s.weapon_effects.size());for(const auto &e:s.weapon_effects) {
            word(e.serial);word(e.tick);word(e.unit);word(e.weapon);word(e.effect);word(e.a_bits);word(e.b_bits);
            word(e.colors.a);word(e.colors.b);vector(e.velocity);word(e.source_instance);
            word(e.source_presentation);word(e.source_scheduled);word(e.source_locations_observed);word(e.source_locations.size());
            for(const auto &location:e.source_locations){word(location.name);transform(location.frame);}
        }
        word(s.unit_damage_effects.size());for(const auto &e:s.unit_damage_effects) {
            word(e.serial);word(e.tick);word(e.target);word(unsigned(e.effect.kind));word(e.effect.datum);
            word(e.owner.player);word(e.owner.unit);word(e.owner.designator);word(e.colors.a);word(e.colors.b);real(e.controls.a);real(e.controls.b);
        }
        word(s.unit_damage_voices.size());for(const auto &e:s.unit_damage_voices) {
            word(e.serial);word(e.tick);word(e.target);word(e.definition);word(e.report_flags);vector(e.position);
            real(e.recent_body);real(e.recent_shield);real(e.visible_body);
        }
        word(s.device_events.size());for(const auto &e:s.device_events){word(e.handle);word(e.tag);word(e.serial);word(e.tick);}
        word(s.rumble_events.size());for(const auto &e:s.rumble_events) {
            word(e.serial);word(e.tick);word(e.event.definition);word(e.event.shielded);real(e.event.scale);
            word(e.event.owner);real(e.event.flash_scale);word(e.event.seated);
        }
        word(s.corpses.size());for(const auto &c:s.corpses) {
            word(c.object);word(c.definition);word(c.player);word(c.death_tick);vector(c.velocity);word(c.physical_state);real(c.health);real(c.shield);
        }
    }
};
struct Trace {
    std::string kind,digest;unsigned tick{},view{};
    bool operator==(const Trace &)const=default;
};
struct Result {
    std::vector<SourceExecutionInput> external_inputs;
    std::vector<std::byte> passive_trace;
    ordinary_trace::Counts passive_counts;
    std::vector<Trace> trace;unsigned submitted{},inputs{},gpu_frames{},input_fire{},source_weapon_effects{};
    std::size_t copy_active_frames{},copy_worker_frames{},copy_jobs{},copy_bytes{};
    std::uint64_t copy_ns{},copy_overlap_ns{},copy_wait_ns{};
    std::size_t gpu_jobs{},gpu_vertices{},gpu_weighted{},gpu_verified{},gpu_post_identity{};
    std::size_t fallback_frames{},fallback_jobs{},fallback_vertices{},fallback_weighted{},fallback_post_identity{};
    std::size_t fallback_quota_jobs{},fallback_allocation_jobs{},fallback_mapping_jobs{};
    Json fallback_results=Json::object();
    std::size_t bodies_total{},bodies_min=std::numeric_limits<std::size_t>::max(),bodies_max{};
    std::vector<std::byte> ppm;std::string validation;Json evidence;
    void record(const char *kind,unsigned tick,unsigned view,const Words &words){trace.push_back({kind,sha256(words.bytes),tick,view});}
    Json fallback_metrics()const {
        return {{"frames",fallback_frames},{"jobs",fallback_jobs},{"vertices",fallback_vertices},{"weighted_vertices",fallback_weighted},
            {"post_identity_vertices",fallback_post_identity},{"quota_jobs",fallback_quota_jobs},
            {"allocation_jobs",fallback_allocation_jobs},{"mapping_jobs",fallback_mapping_jobs},{"vulkan_results",fallback_results}};
    }
};
bool fallback_reason(std::string_view reason) {
    for(const auto value:{"quota","packing","create","allocation","device-allocation","map","map-range"})if(reason==value)return true;
    return false;
}
VkResult fallback_result(std::string_view reason) {
    return reason=="quota" || reason=="packing"?VK_SUCCESS:reason=="device-allocation"?VK_ERROR_OUT_OF_DEVICE_MEMORY:
        reason=="map-range"?VK_ERROR_MEMORY_MAP_FAILED:VK_ERROR_OUT_OF_HOST_MEMORY;
}
struct DiagnosticEnvironment {
    ordinary_trace::Environment environment;
    DiagnosticEnvironment(){std::fesetround(FE_TONEAREST);}
};
// One local owning cut survives only the CPU/comparison pair. Its binding is
// a test-domain tag, not peer authentication or permission to bypass an owner.
struct CommonCut {
    SourceMatchAuthority::AuthorityDigest binding{std::byte{0x48},std::byte{0x32},std::byte{0x43},std::byte{0x43}};
    std::optional<SourceMatchAuthority::Checkpoint> saved;
    static constexpr std::uint64_t bank_limit=1024ull*1024u*1024u,file_limit=64ull*1024u*1024u;
    static constexpr std::size_t region_limit=8192;
};
Json common_cut_limits(const SourceMatchAuthority::Checkpoint &cut) {
    DiagnosticEnvironment preserved;
    const auto &world=cut.world.world;std::uint64_t bank_bytes{},file_bytes{};
    if(world.bank.regions.size()>CommonCut::region_limit)
        throw FormatError("Common cut exceeds its complete-region bound: actual="+std::to_string(world.bank.regions.size())+
            " limit="+std::to_string(CommonCut::region_limit));
    for(const auto &region:world.bank.regions) {
        need(region.bytes.size()<=CommonCut::bank_limit-bank_bytes,"Common cut exceeds its complete-bank byte bound");
        bank_bytes+=region.bytes.size();
    }
    need(world.arena.files.size()==2,"Common cut lacks its two genuine arena files");
    for(const auto &file:world.arena.files) {
        need(file.bytes.size()<=CommonCut::file_limit-file_bytes,"Common cut exceeds its arena-file byte bound");
        file_bytes+=file.bytes.size();
    }
    return {{"bank_bytes",bank_bytes},{"regions",world.bank.regions.size()},{"arena_file_bytes",file_bytes},
        {"arena_files",world.arena.files.size()},{"bank_byte_limit",CommonCut::bank_limit},
        {"arena_file_byte_limit",CommonCut::file_limit},{"region_limit",CommonCut::region_limit},
        {"scope","One owning in-process source/Match cut, released after this pair. Metadata/small vectors remain in process memory; large snapshot byte backing follows the existing owned-scratch storage policy. Local callbacks/executors/resources retain normal import guards."}};
}
void prove_common_cut_bank(const SourceRuntimeMemory &memory,const SourceRuntimeMemory::Checkpoint &saved,Json &proof) {
    DiagnosticEnvironment preserved;
    proof={{"passed",false},{"regions",saved.regions.size()},{"complete_bank_bytes",0},
        {"committed_ranges",0},{"committed_bytes",0},{"no_resource_exclusion",false}};
    SourceRuntimeMemory::validate_checkpoint(saved);
    std::uint64_t expected_total{},expected_ranges{},expected_bytes{};
    for(const auto &r:saved.regions) {
        need(r.bytes.size()<=CommonCut::bank_limit-expected_total,"Common cut initial proof exceeds its complete-bank bound");
        expected_total+=r.bytes.size();
        if(!r.page_bytes){++expected_ranges;expected_bytes+=r.bytes.size();continue;}
        for(std::size_t page=0;page<r.committed.size();) {
            if(!r.committed[page]){++page;continue;}
            const auto first=page;while(page<r.committed.size() && r.committed[page])++page;
            ++expected_ranges;expected_bytes+=(page-first)*r.page_bytes;
        }
    }
    proof.update({{"complete_bank_bytes",expected_total},{"expected_committed_ranges",expected_ranges},
        {"expected_committed_bytes",expected_bytes}});
    // The complete topology also detects wholly uncommitted extra owners.
    // Compare all region bytes, then independently stream actual committed
    // runs using exactly visit_committed's segmentation. No bytes are masked.
    const auto actual=memory.checkpoint();
    if(actual.version!=saved.version || actual.regions.size()!=saved.regions.size()) {
        proof["first_mismatch"]={{"kind","topology-count"},{"actual_regions",actual.regions.size()},{"expected_regions",saved.regions.size()}};
        throw FormatError("Common cut complete source topology/count differs");
    }
    std::uint64_t total{};
    for(std::size_t n=0;n<saved.regions.size();++n) {
        const auto &a=actual.regions[n],&b=saved.regions[n];
        if(a.address!=b.address || a.access!=b.access || a.owner!=b.owner || a.page_bytes!=b.page_bytes ||
            a.committed!=b.committed || a.bytes.size()!=b.bytes.size()) {
            proof["first_mismatch"]={{"kind","topology"},{"region",n},{"actual_address",a.address},{"expected_address",b.address}};
            throw FormatError("Common cut source region topology or policy differs");
        }
        need(b.bytes.size()<=CommonCut::bank_limit-total,"Common cut initial proof exceeds its bank bound");total+=b.bytes.size();
        const auto difference=std::mismatch(a.bytes.begin(),a.bytes.end(),b.bytes.begin());
        if(difference.first!=a.bytes.end()) {
            const auto offset=std::size_t(difference.first-a.bytes.begin());
            proof["first_mismatch"]={{"kind","byte"},{"guest_address",std::uint64_t(a.address)+offset},
                {"actual_byte",std::to_integer<unsigned>(*difference.first)},{"expected_byte",std::to_integer<unsigned>(*difference.second)}};
            throw FormatError("Common cut initial complete source byte differs");
        }
    }
    std::size_t region{},page{};std::uint64_t ranges{},bytes{};
    const auto next=[&](std::uint32_t &address,std::span<const std::byte> &value) {
        while(region<saved.regions.size()) {
            const auto &r=saved.regions[region];
            if(!r.page_bytes){++region;page=0;address=r.address;value={r.bytes.data(),r.bytes.size()};return true;}
            while(page<r.committed.size() && !r.committed[page])++page;
            if(page==r.committed.size()){++region;page=0;continue;}
            const auto first=page;while(page<r.committed.size() && r.committed[page])++page;
            const auto offset=first*r.page_bytes;address=r.address+std::uint32_t(offset);
            value={r.bytes.data()+offset,(page-first)*r.page_bytes};return true;
        }
        return false;
    };
    memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> value) {
        std::uint32_t expected_address{};std::span<const std::byte> expected;
        if(!next(expected_address,expected) || address!=expected_address || value.size()!=expected.size()) {
            proof["first_mismatch"]={{"kind","committed-run"},{"ordinal",ranges},{"actual_address",address},
                {"expected_address",expected_address},{"actual_bytes",value.size()},{"expected_bytes",expected.size()}};
            throw FormatError("Common cut committed source run segmentation differs");
        }
        const auto difference=std::mismatch(value.begin(),value.end(),expected.begin());
        if(difference.first!=value.end()) {
            proof["first_mismatch"]={{"kind","committed-byte"},{"guest_address",std::uint64_t(address)+std::size_t(difference.first-value.begin())},
                {"actual_byte",std::to_integer<unsigned>(*difference.first)},{"expected_byte",std::to_integer<unsigned>(*difference.second)}};
            throw FormatError("Common cut committed source byte differs");
        }
        ++ranges;bytes+=value.size();proof["committed_ranges"]=ranges;proof["committed_bytes"]=bytes;
    });
    std::uint32_t extra_address{};std::span<const std::byte> extra;
    need(!next(extra_address,extra),"Common cut omitted a saved committed source run");
    need(ranges && bytes,"Common cut has no actual committed source bytes");
    need(ranges==expected_ranges && bytes==expected_bytes,"Common cut committed source totals differ");
    proof.update({{"passed",true},{"complete_topology_identical",true},{"complete_region_bytes_identical",true},
        {"complete_bank_bytes",total},{"committed_ranges",ranges},{"committed_bytes",bytes},
        {"committed_run_segmentation_identical",true},{"committed_bytes_identical",true},{"no_resource_exclusion",true}});
}
Json vector_json(Vec3 value){return Json::array({value.x,value.y,value.z});}
Json camera_json(const Camera &camera) {
    return {{"position",vector_json(camera.position)},{"yaw",camera.yaw},{"pitch",camera.pitch},
        {"field_of_view",camera.field_of_view},{"horizontal_field_of_view",camera.horizontal_field_of_view},
        {"oriented",camera.oriented},{"forward",vector_json(camera.oriented?camera.forward:direction(camera.yaw,camera.pitch))},
        {"up",vector_json(camera.up)}};
}
Json player_evidence(const MultiplayerState &state,std::span<const unsigned> local_slots) {
    Json players=Json::array();
    for(unsigned slot=0;slot<state.players.size();++slot) {
        const auto &player=state.players[slot];if(!player.connected && player.source_unit==null_datum)continue;
        players.push_back({{"slot",slot},{"connected",player.connected},{"dead",player.dead},{"source_player",player.source_player},
            {"source_unit",player.source_unit},{"source_weapon",player.source_weapon},{"position",vector_json(player.position)},
            {"yaw",player.yaw},{"pitch",player.pitch},{"health",player.health},{"shield",player.shield},
            {"grounded",player.native_grounded},{"local_view_slot",std::find(local_slots.begin(),local_slots.end(),slot)!=local_slots.end()}});
    }
    return players;
}
Json camera_lines_of_sight(const MultiplayerPreparedWorld &world,const MultiplayerState &state,const Camera &camera,unsigned own_slot) {
    Json rays=Json::array();
    for(unsigned slot=0;slot<state.players.size();++slot) {
        const auto &player=state.players[slot];
        if(slot==own_slot || !player.connected || player.dead || player.source_unit==null_datum)continue;
        const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
            [&](const auto &value){return value.object==player.source_unit;});
        const bool source_sphere=pose!=state.source_body_poses.end();
        const auto target=source_sphere?pose->sphere.center:
            player.position+Vec3{0,0,world.match.unit(slot).definition.height*.5f};
        const auto delta=target-camera.position;const auto distance=length(delta);
        Json evidence={{"target_slot",slot},{"source_unit",player.source_unit},{"target",vector_json(target)},
            {"target_owner",source_sphere?"completed source body sphere":"fixture unit height midpoint"},{"distance",distance}};
        if(distance>0) {
            const auto forward=delta*(1.f/distance);const auto hit=world.match.simulation().collision().raycast(camera.position,forward,distance);
            evidence["host_triangle_ray_clear"]=!hit;evidence["host_triangle_count"]=world.match.simulation().collision().size();
            if(hit)evidence["first_hit"]={{"distance",hit->distance},{"triangle",hit->triangle},{"position",vector_json(hit->position)}};
        }
        rays.push_back(std::move(evidence));
    }
    return rays;
}
struct ValidationLog {
    std::ostringstream output;std::streambuf *previous=std::cerr.rdbuf(output.rdbuf());
    ~ValidationLog(){std::cerr.rdbuf(previous);const auto value=output.str();if(!value.empty())std::cerr<<value;}
};
std::vector<std::byte> read_file(const std::filesystem::path &path,std::uint64_t limit=64u*1024u*1024u) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);need(bool(in),"Open completed frame");
    const auto count=in.tellg();need(count>0 && std::uint64_t(count)<=limit,"Completed frame/trace has invalid bounded extent");
    std::vector<std::byte> out{std::size_t(count)};in.seekg(0);in.read(reinterpret_cast<char*>(out.data()),count);
    need(bool(in),"Read complete captured frame bytes");return out;
}
Json frame_extents(const std::filesystem::path &path,unsigned submitted,unsigned width,unsigned height,
                   unsigned present_width,unsigned present_height) {
    const auto bytes=read_file(path);std::istringstream stream(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
    const auto columns=[](const std::string &line) {
        std::vector<std::string> out;std::istringstream fields(line);std::string field;
        while(std::getline(fields,field,'\t'))out.push_back(field);return out;
    };
    std::string line;need(bool(std::getline(stream,line)),"Dual-extent renderer timings omit their header");
    const auto header=columns(line);std::array<std::size_t,5> positions{};unsigned selected{};
    for(const auto *name:{"frame","render_width","render_height","present_width","present_height"}) {
        const auto at=std::find(header.begin(),header.end(),name);
        need(at!=header.end() && std::find(at+1,header.end(),name)==header.end(),"Dual-extent renderer timings omit or duplicate integer columns");
        positions[selected++]=std::size_t(at-header.begin());
    }
    unsigned count{};
    while(std::getline(stream,line)) {
        const auto fields=columns(line);need(fields.size()==header.size(),"Dual-extent renderer timing row is incomplete");
        std::array<unsigned,5> values{};
        for(unsigned i=0;i<values.size();++i) {
            const auto &text=fields[positions[i]];
            const auto parsed=std::from_chars(text.data(),text.data()+text.size(),values[i]);
            need(!text.empty() && parsed.ec==std::errc{} && parsed.ptr==text.data()+text.size(),
                 "Dual-extent renderer timing column is not an unsigned integer");
        }
        need(values[0]==count,"Dual-extent renderer timing frames are not consecutive from zero");
        need(values[1]==width && values[2]==height && values[3]==present_width && values[4]==present_height,
             "Dual-extent renderer timing frame differs from requested internal or presented extent");++count;
    }
    need(count==submitted && count>=60,"Dual-extent renderer timings do not cover every submitted frame");
    return {{"frames",count},{"first_frame",0},{"last_frame",count-1},{"render_width",width},{"render_height",height},
        {"present_width",present_width},{"present_height",present_height},{"consecutive_frames",true},{"all_extents_match",true},
        {"tsv_bytes",bytes.size()},{"tsv_sha256",sha256(bytes)}};
}
Json frame_difference(std::span<const std::byte> cpu,std::span<const std::byte> gpu,unsigned width,unsigned height) {
    const auto header="P6\n"+std::to_string(width)+" "+std::to_string(height)+"\n255\n";
    const auto pixels=std::size_t(width)*height;
    need(cpu.size()==header.size()+pixels*3 && gpu.size()==cpu.size(),"Frame difference requires complete equal RGB extents");
    cpu=cpu.subspan(header.size());gpu=gpu.subspan(header.size());
    std::size_t changed_pixels{},changed_components{};std::array<std::size_t,3> channel_changes{};
    std::array<unsigned,3> channel_max{};unsigned left=width,top=height,right{},bottom{};
    Json first=nullptr,samples=Json::array(),channels=Json::array();
    for(std::size_t pixel=0;pixel<pixels;++pixel) {
        bool changed{};std::array<unsigned,3> before{},after{};
        for(unsigned channel=0;channel<3;++channel) {
            before[channel]=std::to_integer<unsigned>(cpu[pixel*3+channel]);after[channel]=std::to_integer<unsigned>(gpu[pixel*3+channel]);
            const auto error=before[channel]>after[channel]?before[channel]-after[channel]:after[channel]-before[channel];
            if(error){changed=true;++changed_components;++channel_changes[channel];channel_max[channel]=std::max(channel_max[channel],error);}
        }
        if(!changed)continue;
        ++changed_pixels;const auto x=unsigned(pixel%width),y=unsigned(pixel/width);
        left=std::min(left,x);top=std::min(top,y);right=std::max(right,x);bottom=std::max(bottom,y);
        Json sample={{"x",x},{"y",y},{"cpu_rgb",before},{"gpu_rgb",after}};
        if(first.is_null())first=sample;if(samples.size()<8)samples.push_back(std::move(sample));
    }
    std::vector<std::byte> plane(pixels);
    for(unsigned channel=0;channel<3;++channel) {
        for(std::size_t pixel=0;pixel<pixels;++pixel)plane[pixel]=cpu[pixel*3+channel];const auto cpu_hash=sha256(plane);
        for(std::size_t pixel=0;pixel<pixels;++pixel)plane[pixel]=gpu[pixel*3+channel];
        channels.push_back({{"channel",std::string(1,"RGB"[channel])},{"changed_components",channel_changes[channel]},
            {"maximum_absolute_error",channel_max[channel]},{"cpu_sha256",cpu_hash},{"gpu_sha256",sha256(plane)}});
    }
    return {{"width",width},{"height",height},{"rgb_payload_bytes",cpu.size()},{"changed_pixels",changed_pixels},
        {"changed_components",changed_components},{"maximum_absolute_error",*std::max_element(channel_max.begin(),channel_max.end())},
        {"first_changed_pixel",first},{"first_changed_samples",samples},{"channels",channels},
        {"changed_bounds_inclusive",changed_pixels?Json{{"left",left},{"top",top},{"right",right},{"bottom",bottom}}:Json(nullptr)}};
}
Json retain_frame_references(const std::filesystem::path &report,std::string_view cohort,
    std::span<const std::byte> cpu,std::span<const std::byte> gpu) {
    // Explicit diagnostic retention only: two required reference frames beside
    // the bounded report, never the worlds, savegames or disposable binaries.
    const auto write=[&](std::string_view suffix,std::span<const std::byte> bytes) {
        auto path=std::filesystem::absolute(report);path+=std::string(".")+std::string(cohort)+"."+std::string(suffix)+".ppm";
        std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));
        need(bool(out),"Write explicitly requested frame reference");return Json{{"path",path.string()},{"bytes",bytes.size()},{"sha256",sha256(bytes)}};
    };
    return {{"scope","One failed CPU/GPU final-frame pair retained as required diagnostic references; remove after investigation."},
        {"cpu",write("cpu",cpu)},{"gpu",write("gpu",gpu)}};
}
bool firing(unsigned command){return command>=12 && command<54 && command%6<4;}
Result run_case(const std::filesystem::path &map,const std::filesystem::path &output,unsigned players,unsigned species,bool fire,bool gpu,
                bool first_person,unsigned width,unsigned height,unsigned present_width,unsigned present_height,bool dual_extent,
                Json &live_evidence,std::string_view forced_fallback={},const std::vector<SourceExecutionInput> *replay_inputs=nullptr,
                CommonCut *common_cut=nullptr) {
    const auto prefix_before=source_shadow_native_prefix_tu15_stats();
    Environment enable("HALO2_ENABLE_GPU_SKINNING"),reference("HALO2_REFERENCE_CPU_SKINNING"),verify("HALO2_VERIFY_GPU_SKINNING");
    Environment prepare_failure("HALO2_TEST_GPU_SKIN_PREPARE_FAILURE");
    Environment render_timings("HALO2_RENDER_TIMINGS");
    const auto timing_path=output/"render-timings.tsv";
    if(dual_extent)render_timings.set(timing_path.c_str());
    const std::string forced_reason(forced_fallback);const bool fallback=!forced_reason.empty();
    need(!fallback || (gpu && fallback_reason(forced_reason)),"Unknown or noncandidate forced CPU fallback control");
    if(!gpu)prepare_failure.set(nullptr);else if(fallback)prepare_failure.set(forced_reason.c_str());
    enable.set("1");reference.set(gpu?nullptr:"1");verify.set(gpu?"1":nullptr);
    ValidationLog validation;Result result;
    std::shared_ptr<SourceExecutionInputs> inputs;
    {
        DiagnosticEnvironment saved;
        inputs=replay_inputs?std::make_shared<SourceExecutionInputs>(std::span(*replay_inputs)):
            std::make_shared<SourceExecutionInputs>();
        std::filesystem::create_directories(output);
    }
    const auto passive_path=output/"ordinary-source-trace.bin";
    ordinary_trace::Sink passive(passive_path);
    {
        MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();options.local_players=players;
        // Declared after options so exception unwinding releases this external
        // binding before its actual PreparedWorld/memory owner can disappear.
        std::optional<SourceExecutionInputBinding> common_binding;
        options.width=present_width;options.height=present_height;options.source_object_world=true;options.starting_weapon=0;
        options.variant.values.set(76,1);options.mute=true;options.validation=true;options.hidden=false;
        options.session_random_seed=0x1376c0;options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
        options.desktop=std::make_shared<DesktopSession>();options.desktop->auxiliary_random()=1;
        options.desktop->display_settings.internal_width=width;options.desktop->display_settings.internal_height=height;
        options.source_object_savegame_directory=output/"source";std::filesystem::create_directories(output);
        // Declare the finite capture before construction so the existing
        // reference RNG route can pin the source auxiliary word before any
        // scenario factory consumes it. The completed source tick below still
        // determines the actual final capture boundary.
        options.screenshot=output/"final.ppm";options.input_script=output/"inputs.tsv";
        options.capture_tick=120;options.reference_auxiliary_seed=1;
        unsigned construction_rng_receipts{};const SourceRuntimeMemory *source_rng_memory{};
        options.source_world_observer=[&](const SourceRuntimeMemory &memory) {
            DiagnosticEnvironment saved;
            if(!common_cut)passive.source(memory);
            const auto owner=Bytes(memory.view(0x4f3cd8u,4)).u32(0);
            need(owner!=0,"Constructed source RNG owner is absent");const auto bank=Bytes(memory.view(owner,8));
            const auto primary=bank.u32(0),auxiliary=bank.u32(4);
            ++construction_rng_receipts;source_rng_memory=&memory;
            result.evidence["source_rng_construction"]={{"receipts",construction_rng_receipts},{"owner",owner},
                {"primary",primary},{"auxiliary",auxiliary},{"requested_auxiliary",1},
                {"stage","After original map/session binding and reference input, before automatic object construction"}};
            if(common_cut)result.evidence["source_rng_construction"].update({{"pre_common_cut_observation",true},{"compared_in_ordered_trace",false}});
            else {Words receipt;receipt.word(primary);receipt.word(auxiliary);result.record("construction-source-rng",0,0,receipt);}
            live_evidence=result.evidence;
            need(auxiliary==1,"Source auxiliary RNG was not pinned before scenario construction");
        };
        const std::array<std::array<std::uint8_t,9>,2> appearance{{{8,12,9,1,0,26,9,0,0},{3,4,3,4,1,5,31,0,0}}};
        for(unsigned view=0;view<players;++view)
            options.selected_profiles.push_back(native_selected_player_configuration((species?"Elite":"Spartan")+std::to_string(view),appearance[species],-1));
        options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options,std::nullopt,
            std::shared_ptr<PhysicsLiveClockSource>{},common_cut?std::shared_ptr<SourceExecutionInputs>{}:inputs);
        need(construction_rng_receipts==1,"Prepared world omitted the pre-scenario source RNG receipt");
        auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
        if(common_cut) {
            DiagnosticEnvironment preserved;
            need(!fallback,"Common-cut qualification excludes forced fallback controls");
            need(world.source_objects && world.source_objects->owns_physics(),"Common cut requires its genuine owning native source world");
            need(!source_execution_inputs(world.source_objects->memory()),"Common cut requires its genuinely untaped constructor");
            SourceMatchAuthority authority(world.match,*world.source_objects);
            auto &cut=result.evidence["post_constructor_common_cut"];
            cut={{"enabled",true},{"role",replay_inputs?"import":"export"},{"completed_constructor_and_local_views",true},
                {"before_preparation_projection_and_runtime",true},{"local_test_binding_only",true},
                {"constructor_and_teardown_outside_tape",true},{"normal_coupled_publication",false},
                {"clock_contract","Unmodified coupled authority import rebases local external clock domains to its actual saved logical values; source clock/RNG/queue bytes are copied unchanged. Runtime operands are recorded/replayed afterward."}};
            live_evidence=result.evidence;
            if(replay_inputs) {
                need(common_cut->saved.has_value(),"Comparison lacks its CPU-owned complete common cut");
                auto prepared=authority.prepare(*common_cut->saved,common_cut->binding);
                need(authority.commit(std::move(prepared)),"Normal coupled common-cut publication failed");
                cut["normal_coupled_publication"]=true;
            } else {
                need(!common_cut->saved,"CPU common cut must be exported exactly once per cohort");
                common_cut->saved.emplace(authority.checkpoint(common_cut->binding));
            }
            cut["actual_complete_regions"]=common_cut->saved->world.world.bank.regions.size();
            cut["complete_region_limit"]=CommonCut::region_limit;
            live_evidence=result.evidence;
            cut["resource_limits"]=common_cut_limits(*common_cut->saved);
            cut["tick"]=common_cut->saved->match.simulation.ticks;
            try{prove_common_cut_bank(world.source_objects->memory(),common_cut->saved->world.world.bank,cut["initial_bank_proof"]);}
            catch(...){live_evidence=result.evidence;throw;}
            // No pre-cut bank receipt enters the shared passive stream.
            // This is the actual complete state after the ordinary import.
            passive.source(world.source_objects->memory());
            source_rng_memory=&world.source_objects->memory();
            common_binding.emplace(*source_rng_memory,*inputs);
            cut["tape_bound_after_initial_proof"]=true;live_evidence=result.evidence;
        }
        const auto initial=world.match.state();Vec3 center{};std::array<Controls,multiplayer_capacity> controls{};
        for(auto slot:world.local_slots){center.x+=initial.players[slot].position.x/players;center.y+=initial.players[slot].position.y/players;}
        for(auto slot:world.local_slots)controls[slot].yaw=std::atan2(center.y-initial.players[slot].position.y,center.x-initial.players[slot].position.x);
        for(unsigned tick=0;tick<60;++tick) {
            world.match.step(controls);
            passive.checkpoint(ordinary_trace::BankCheckpoint::preparation,tick,world.match.state().tick);
        }
        const auto prepared=world.match.state();const auto start_tick=prepared.tick;options.capture_tick=start_tick+60;
        // Source unit birth/placement can complete during the preparation
        // ticks. Aim the real finite commands from their completed positions,
        // rather than the earlier roster's unpublished position bank.
        Vec3 requested_center{};
        for(auto slot:world.local_slots) {
            const auto &player=prepared.players[slot];
            need(player.connected && !player.dead && player.source_unit!=null_datum,
                "Prepared source player has no actual live unit for declared aim");
            requested_center.x+=player.position.x/players;requested_center.y+=player.position.y/players;
        }
        for(auto slot:world.local_slots) {
            const auto &position=prepared.players[slot].position;
            controls[slot].yaw=std::atan2(requested_center.y-position.y,requested_center.x-position.x);
        }
        {
            DiagnosticEnvironment saved;
            result.evidence["fixture"]={{"prepared_tick",start_tick},{"players",player_evidence(prepared,world.local_slots)},
                {"local_slots",world.local_slots},{"requested_aim_center",vector_json(requested_center)},
                {"requested_aim_source","Completed source player positions after sixty real preparation ticks; delivered finite controls only, no placement or source-state writes."},
                {"requested_yaws",Json::array()},
                {"visibility_scope","Recorded cameras and host-triangle rays only; neither proves source PVS/frustum/model admission or surviving pixels."}};
            for(unsigned view=0;view<players;++view) {
                const auto slot=world.local_slots.at(view);const auto yaw=controls[slot].yaw;
                result.evidence["fixture"]["requested_yaws"].push_back({{"view",view},{"slot",slot},{"yaw",yaw},{"yaw_bits",std::bit_cast<std::uint32_t>(yaw)}});
            }
            result.evidence["view_mode"]=first_person?"ordinary-first-person":"world-body cohort: one-player inspection camera, ordinary split-screen cameras";
            result.evidence["particle_effect_observer"]=!first_person;
            result.evidence["forced_prepare_failure"]=forced_reason;
            result.evidence["capture_inputs"]={{"declared_preconstruction_capture_tick",120},{"final_requested_capture_tick",options.capture_tick},
                {"preparation_source_ticks",60},{"finite_script_commands_per_view",60},{"session_random_seed",0x1376c0},
                {"desktop_auxiliary_seed",1},{"source_auxiliary_seed",1},
                {"seed_route","Existing reference_auxiliary_seed for an isolated finite local input-script screenshot capture; source_world_observer only reads the constructed primary/auxiliary words."}};
            result.evidence["first_person_source_bank_check_requested"]=first_person && std::getenv("HALO2_VERIFY_GPU_FIRST_PERSON_INPUTS");
            result.evidence["view_cameras"]=Json::array();
            for(unsigned view=0;view<players;++view)result.evidence["view_cameras"].push_back({{"view",view},{"callbacks",0}});
        }
        {
            std::ofstream input(options.input_script);input<<std::setprecision(9);
            for(unsigned tick=0;tick<60;++tick)for(unsigned view=0;view<players;++view)
                input<<tick<<' '<<view<<' '<<(fire&&firing(tick))<<" 0 0 "<<controls[world.local_slots[view]].yaw<<" 0\n";
            need(bool(input),"Write actual finite command script");
        }
        if(players==1 && !first_person) {
            const auto slot=world.local_slots.front();const auto center=prepared.players[slot].position+Vec3{0,0,world.match.unit(slot).definition.height*.5f};
            float distance{};Vec3 outward{};
            for(auto direction:std::array{Vec3{1,0,0},Vec3{-1,0,0},Vec3{0,1,0},Vec3{0,-1,0}}) {
                const auto hit=world.match.simulation().collision().raycast(center,direction,3.2f);
                const auto clear=hit?std::max(0.f,hit->distance-.1f):3.2f;if(clear>distance){distance=clear;outward=direction;}
            }
            need(distance>.5f,"Actual player has no collision-safe inspection camera");Camera camera;
            camera.position=center+outward*distance;camera.yaw=std::atan2(-outward.y,-outward.x);
            options.capture_camera=camera;options.capture_third_person=true;
        }
        Words ready;ready.state(prepared);result.record("prepared",start_tick,0,ready);
        options.input_observer=[&](unsigned step,std::span<const Controls> input,const MultiplayerState &state) {
            ordinary_trace::Environment saved;
            passive.input(step,input,state);
            need(step<60 && input.size()==players,"Actual command delivery escaped its declared finite script");
            Words words;words.word(step);words.word(input.size());for(unsigned view=0;view<input.size();++view) {
                const auto &control=input[view];const auto requested_yaw=controls[world.local_slots.at(view)].yaw;
                need(control.forward==0 && control.right==0 && !control.jump && !control.crouch && !control.reload && control.fire==(fire&&firing(step)),
                    "Actual delivered command differs from the declared neutral/firing script");
                need(std::bit_cast<std::uint32_t>(control.yaw)==std::bit_cast<std::uint32_t>(requested_yaw) && control.pitch==0,
                    "Actual delivered yaw/pitch differs from the completed-position aim request");
                words.controls(control);result.input_fire+=control.fire;
            }
            words.state(state);result.record("input",state.tick,0,words);++result.inputs;
            result.source_weapon_effects=std::max(result.source_weapon_effects,unsigned(state.weapon_effects.size()));
        };
        options.world_portal_observer=[&](unsigned tick,const SourcePortalMaskState &mask) {
            Words words;for(auto value:mask.words)words.word(value);for(auto value:mask.acoustic)words.word(value);result.record("portal",tick,0,words);
        };
        options.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera) {
            ordinary_trace::Environment preserved;
            passive.view(view,state,camera);
            Words words;words.state(state);words.camera(camera);result.record("view",state.tick,view,words);
            DiagnosticEnvironment saved;auto &evidence=result.evidence["view_cameras"].at(view);
            auto camera_record=camera_json(camera);camera_record["tick"]=state.tick;
            if(evidence["callbacks"]==0) {
                evidence["first"]=camera_record;
                evidence["first"]["other_player_lines_of_sight"]=camera_lines_of_sight(world,state,camera,world.local_slots.at(view));
            }
            evidence["last"]=std::move(camera_record);evidence["callbacks"]=evidence["callbacks"].get<unsigned>()+1;
        };
        options.view_rectangle_observer=[&](unsigned view,const UiRect &rectangle){
            ordinary_trace::Environment preserved;
            if(players==4) {
                // Independent expected layout, rather than merely requiring
                // CPU/GPU routes to agree on the same submitted rectangles.
                constexpr std::array<std::array<float,4>,4> expected{{
                    {0,0,240,320},{0,320,240,640},
                    {240,0,480,320},{240,320,480,640}}};
                need(view<expected.size(),"Four-player rectangle has an invalid view index");
                const auto &r=expected[view];
                need(rectangle.top==r[0] && rectangle.left==r[1] &&
                    rectangle.bottom==r[2] && rectangle.right==r[3],
                    "Four-player submitted view differs from the required 2x2 layout");
            }
            passive.rectangle(view,rectangle);
        };
        options.node_observer=[&](unsigned view,std::uint32_t tick,const MultiplayerNodeObservation &nodes){passive.nodes(view,tick,nodes);};
        if(!first_person)options.effect_observer=[&](unsigned view,unsigned tick,std::span<const ParticleDraw> draws) {
            Words words;words.word(draws.size());for(const auto &draw:draws) {
                words.word(draw.effect_owner);words.word(draw.effect_definition);words.word(draw.before_light_volume);
                words.word(draw.model_first);words.word(draw.model_count);words.word(draw.scene_sort_depth.has_value());if(draw.scene_sort_depth)words.real(*draw.scene_sort_depth);
                for(const auto &attribute:draw.vertex.attributes)for(auto value:attribute)words.real(value);
                for(const auto &constant:draw.vertex.constants)for(auto value:constant)words.real(value);
                for(auto value:draw.vertex.clip_range)words.real(value);for(auto value:draw.vertex.surface_fog)words.real(value);
            }
            result.record("effects",tick,view,words);
        };
        options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
            need(frame.submitted && frame.views==players,"Declared ordinary frame failed to submit all views");
            const auto &r=frame.renderer;need(r.measured && r.width==width && r.height==height,"Ordinary renderer used another frame extent");
            if(r.presentation_copy_jobs) {
                ++result.copy_active_frames;
                need(r.presentation_copy_separate_thread,"Ordinary presentation copy work did not execute on its worker");
                ++result.copy_worker_frames;
            }
            result.copy_jobs+=r.presentation_copy_jobs;result.copy_bytes+=r.presentation_copy_bytes;
            result.copy_ns+=r.presentation_copy_ns;result.copy_overlap_ns+=r.presentation_copy_overlap_ns;
            result.copy_wait_ns+=r.presentation_copy_wait_ns;
            Words words;words.word(frame.frame);words.word(frame.views);words.word(frame.submitted);words.word(frame.bodies_drawn);
            {
                // The prepared source world's memory owner outlives every
                // synchronous frame callback. Re-read its actual owner word;
                // never retain borrowed source-bank bytes or write its stream.
                DiagnosticEnvironment saved;need(source_rng_memory!=nullptr,"Frame source RNG memory owner is absent");
                const auto owner=Bytes(source_rng_memory->view(0x4f3cd8u,4)).u32(0);
                need(owner!=0,"Frame source RNG bank owner is absent");const auto bank=Bytes(source_rng_memory->view(owner,8));
                const auto primary=bank.u32(0),auxiliary=bank.u32(4);words.word(primary);words.word(auxiliary);
                Json receipt={{"frame",frame.frame},{"tick",world.match.state().tick},{"owner",owner},{"primary",primary},{"auxiliary",auxiliary}};
                auto &observed=result.evidence["source_rng_frames"];
                if(!result.submitted)observed["first"]=receipt;
                observed["last"]=std::move(receipt);observed["callbacks"]=result.submitted+1;
                observed["scope"]="Actual source primary and auxiliary RNG words after each submitted frame; integer words included in ordered frame trace.";
            }
            result.record("frame",world.match.state().tick,0,words);++result.submitted;
            result.gpu_frames+=r.model_skin_gpu_jobs!=0;result.gpu_jobs+=r.model_skin_gpu_jobs;result.gpu_vertices+=r.model_skin_gpu_vertices;
            result.gpu_weighted+=r.model_skin_gpu_deformed_vertices;result.gpu_verified+=r.model_skin_gpu_verified_vertices;
            result.gpu_post_identity+=r.model_skin_gpu_post_identity_vertices;
            result.fallback_frames+=r.model_skin_cpu_fallback_jobs!=0;result.fallback_jobs+=r.model_skin_cpu_fallback_jobs;
            result.fallback_vertices+=r.model_skin_cpu_fallback_vertices;result.fallback_weighted+=r.model_skin_cpu_fallback_deformed_vertices;
            result.fallback_post_identity+=r.model_skin_cpu_fallback_post_identity_vertices;
            result.fallback_quota_jobs+=r.model_skin_cpu_fallback_quota_jobs;
            result.fallback_allocation_jobs+=r.model_skin_cpu_fallback_allocation_jobs;
            result.fallback_mapping_jobs+=r.model_skin_cpu_fallback_mapping_jobs;
            if(r.model_skin_cpu_fallback_jobs) {
                auto &count=result.fallback_results[std::to_string(r.model_skin_cpu_fallback_vulkan_result)];
                count=count.is_null()?1:count.get<unsigned>()+1;
            }
            result.evidence["cpu_fallback"]=result.fallback_metrics();
            result.bodies_total+=frame.bodies_drawn;result.bodies_min=std::min(result.bodies_min,std::size_t(frame.bodies_drawn));
            result.bodies_max=std::max(result.bodies_max,std::size_t(frame.bodies_drawn));
            live_evidence=result.evidence;live_evidence.update({{"submitted_frames",result.submitted},{"input_callbacks",result.inputs},{"input_fire",result.input_fire},
                {"source_weapon_effects",result.source_weapon_effects},{"gpu_frames",result.gpu_frames},{"gpu_jobs",result.gpu_jobs},
                {"gpu_vertices",result.gpu_vertices},{"gpu_weighted_vertices",result.gpu_weighted},{"gpu_verified_vertices",result.gpu_verified},
                {"gpu_post_identity_vertices",result.gpu_post_identity},
                {"bodies_drawn",{{"total",result.bodies_total},{"minimum",result.bodies_min},{"maximum",result.bodies_max},{"last",frame.bodies_drawn}}}});
            const bool gpu_zero=!r.model_skin_gpu_jobs && !r.model_skin_gpu_vertices && !r.model_skin_gpu_deformed_vertices &&
                !r.model_skin_gpu_verified_vertices && !r.model_skin_gpu_post_identity_vertices;
            const bool fallback_zero=!r.model_skin_cpu_fallback_jobs && !r.model_skin_cpu_fallback_vertices &&
                !r.model_skin_cpu_fallback_deformed_vertices && !r.model_skin_cpu_fallback_post_identity_vertices &&
                !r.model_skin_cpu_fallback_quota_jobs && !r.model_skin_cpu_fallback_allocation_jobs &&
                !r.model_skin_cpu_fallback_mapping_jobs && r.model_skin_cpu_fallback_vulkan_result==VK_SUCCESS;
            if(fallback) {
                need(gpu_zero,"Forced CPU materialization dispatched or verified GPU skin work");
                if(r.model_skin_cpu_fallback_jobs) {
                    const bool quota=forced_reason=="quota",mapping=forced_reason=="map" || forced_reason=="map-range";
                    need(r.model_skin_cpu_fallback_jobs==1 && r.model_skin_cpu_fallback_vertices>0 &&
                        r.model_skin_cpu_fallback_quota_jobs==unsigned(quota) && r.model_skin_cpu_fallback_mapping_jobs==unsigned(mapping) &&
                        r.model_skin_cpu_fallback_allocation_jobs==unsigned(!quota && !mapping) &&
                        r.model_skin_cpu_fallback_vulkan_result==fallback_result(forced_reason),
                        "Forced CPU materialization reported another actual resource-refusal reason");
                } else need(fallback_zero,"Empty forced fallback frame retained partial counters");
            } else {
                need(fallback_zero,"Ordinary CPU/GPU qualification unexpectedly used late CPU materialization");
                if(gpu)need(r.model_skin_gpu_verified_vertices==r.model_skin_gpu_vertices,"Actual GPU dispatch omitted complete CPU-byte verification");
                else need(gpu_zero,"CPU counterfactual unexpectedly dispatched or verified GPU skin work");
            }
            if(gpu && !first_person)need(!r.model_skin_gpu_post_identity_vertices,
                "World-body cohort unexpectedly dispatched first-person identity-placement skins");
            if(fallback && !first_person)need(!r.model_skin_cpu_fallback_post_identity_vertices,
                "World-body fallback cohort unexpectedly materialized first-person identity-placement skins");
        };
        need(run_multiplayer(options)==0,"Ordinary local runtime failed");
        if(dual_extent)result.evidence["frame_extent_evidence"]=frame_extents(timing_path,result.submitted,width,height,present_width,present_height);
        result.evidence["first_person_source_bank_check_completed"]=first_person && gpu &&
            std::getenv("HALO2_VERIFY_GPU_FIRST_PERSON_INPUTS");
        need(result.inputs==60 && result.submitted>=60,"Ordinary paired workload omitted declared source ticks or frames");
        need(result.copy_jobs>0 && result.copy_active_frames==result.copy_worker_frames,
            "Ordinary source trace performed no actual presentation-worker copy work");
        if(players==4)result.evidence["four_player_layout"]={
            {"expected_submitted_rectangles_checked",true},{"grid_columns",2},{"grid_rows",2},
            {"nominal_internal_view_width",double(width)/2},{"nominal_internal_view_height",double(height)/2},
            {"scope","Canonical submitted rectangles plus actual renderer frame extent; no independent Vulkan viewport observation."}};
        if(gpu && !fallback)need(result.gpu_frames && result.gpu_jobs && result.gpu_weighted && result.gpu_vertices==result.gpu_verified,
            "Ordinary character path performed no actual weighted GPU skin/verified complete records");
        if(gpu && !fallback && first_person)need(result.gpu_post_identity>0,
            "Ordinary first-person path performed no actual GPU skin with original identity placement");
        if(fallback)need(result.fallback_frames && result.fallback_jobs && result.fallback_weighted &&
            (!first_person || result.fallback_post_identity>0),"Forced control performed no actual weighted CPU materialization/post-placement");
        if(fire)need(result.input_fire && result.source_weapon_effects,"Actual firing commands produced no observed original weapon effect events");
        result.ppm=read_file(options.screenshot);
        {
            const auto header="P6\n"+std::to_string(present_width)+" "+std::to_string(present_height)+"\n255\n";
            const auto payload=std::size_t(present_width)*present_height*3;
            need(result.ppm.size()==header.size()+payload &&
                !std::memcmp(result.ppm.data(),header.data(),header.size()),
                "Actual final framebuffer header or complete RGB payload differs from requested extent");
            result.evidence["ppm_header"]=header;
        }
        result.evidence.update({{"prepared_tick",start_tick},{"final_tick",world.match.state().tick},{"inspection_camera",players==1 && !first_person},
            {"actual_delivered_commands_per_view",result.inputs},{"final_requested_capture_tick",options.capture_tick},
            {"render_width",width},{"render_height",height},{"present_width",present_width},{"present_height",present_height},
            {"dual_extent_mode",dual_extent},{"framebuffer_extent_kind","presented"},
            {"complete_ppm_bytes",result.ppm.size()},{"rgb_payload_bytes",std::size_t(present_width)*present_height*3},
            {"submitted_frames",result.submitted},{"input_callbacks",result.inputs},{"input_fire",result.input_fire},
            {"source_weapon_effects",result.source_weapon_effects},{"gpu_frames",result.gpu_frames},{"gpu_jobs",result.gpu_jobs},
            {"gpu_vertices",result.gpu_vertices},{"gpu_weighted_vertices",result.gpu_weighted},{"gpu_verified_vertices",result.gpu_verified},
            {"gpu_post_identity_vertices",result.gpu_post_identity},
            {"bodies_drawn",{{"total",result.bodies_total},{"minimum",result.bodies_min},{"maximum",result.bodies_max}}}});
        result.evidence["presentation_worker"]={{"frames_with_copy_jobs",result.copy_active_frames},
            {"frames_with_actual_worker_jobs",result.copy_worker_frames},{"jobs",result.copy_jobs},{"bytes",result.copy_bytes},
            {"copy_ns",result.copy_ns},{"overlap_ns",result.copy_overlap_ns},{"wait_ns",result.copy_wait_ns},
            {"scope","Actual same-frame copy jobs require separate-thread execution; timers include diagnostic trace and GPU verification, not performance qualification."}};
        passive.checkpoint(ordinary_trace::BankCheckpoint::final,0,result.evidence.at("final_tick").get<std::uint32_t>());
        result.passive_counts=passive.counts();const auto &counts=result.passive_counts;
        need(counts.sources==1 && counts.preparations==60 && counts.finals==1 && counts.inputs==60 &&
            counts.views==std::uint64_t(result.submitted)*players && counts.rectangles==counts.views,
            "Passive trace omitted actual source/preparation/input/view/rectangle/final callbacks");
        need(counts.nodes[0]==counts.nodes[1] && counts.nodes[2]==counts.nodes[3] &&
            (first_person?counts.nodes[2]>0:counts.nodes[0]>0),
            "Passive trace omitted actual paired completed/interpolated WORLD or FP nodes");
        passive.finish();
        // No callback may outlive its borrowed world/node/source owner. The
        // default PhysicsImpl binding still covers native teardown. The new
        // control deliberately ends its external tape before native teardown.
        options.source_world_observer={};options.input_observer={};options.view_state_observer={};
        options.view_rectangle_observer={};options.node_observer={};options.world_portal_observer={};
        options.effect_observer={};options.frame_observer={};source_rng_memory=nullptr;
        if(common_cut) {
            common_binding.reset();
            need(!source_execution_inputs(world.source_objects->memory()),"Common-cut tape remained bound before native teardown");
            result.evidence["post_constructor_common_cut"]["tape_unbound_before_native_teardown"]=true;
        }
        options.prepared_world.reset();
    }
    {
        DiagnosticEnvironment saved;
        const auto completed=inputs->finish();need(!completed.empty(),"Ordinary source input tape recorded no actual scoped operands");
        result.external_inputs.assign(completed.begin(),completed.end());
        std::array<std::uint64_t,9> kinds{};Words words;
        for(const auto &input:completed){const auto kind=unsigned(input.kind);need(kind<kinds.size(),"Ordinary input tape kind is invalid");
            ++kinds[kind];words.word(kind);words.word(input.operand);words.word(input.value);}
        result.passive_trace=read_file(passive_path,8u*1024u*1024u);
        const auto &counts=result.passive_counts;
        result.evidence["external_source_inputs"]={{"mode",replay_inputs?"replay":"record"},{"entries",completed.size()},
            {"kind_counts",kinds},{"semantic_sha256",sha256(words.bytes)},{"complete_consumption",true},
            {"constructor_through_world_teardown",!common_cut},{"actual_values_preserved",true},{"complete_call_journal",false}};
        if(common_cut)result.evidence["external_source_inputs"].update({{"post_constructor_common_cut",true},
            {"constructor_outside_scope",true},{"native_teardown_outside_scope",true},
            {"scope","After the complete common cut through60real preparation ticks, runtime and final reads; unbound before native teardown."}});
        result.evidence["passive_source_trace"]={{"schema","H2OTR02"},{"bytes",result.passive_trace.size()},
            {"sha256",sha256(result.passive_trace)},{"source_callbacks",counts.sources},{"preparation_checkpoints",counts.preparations},
            {"final_checkpoints",counts.finals},{"input_callbacks",counts.inputs},{"view_callbacks",counts.views},
            {"rectangle_callbacks",counts.rectangles},{"node_stage_counts",counts.nodes},{"complete_storage",true},
            {"bank_comparison","Complete committed bytes streamed into SHA256 at actual declared boundaries; no normalization."},
            {"full_ordered_source_or_opaque_abi_trace",false}};
        if(common_cut)result.evidence["passive_source_trace"]["initial_source_boundary"]="post-constructor-common-cut";
    }
    result.validation=validation.output.str();need(result.validation.find("Vulkan validation:")==std::string::npos,"Actual ordinary run reported Vulkan validation warnings/errors");
    const auto prefix_after=source_shadow_native_prefix_tu15_stats();
    result.evidence["shadow_native_prefix"]={
        {"attempts",prefix_after.attempts-prefix_before.attempts},
        {"admitted",prefix_after.admitted-prefix_before.admitted},
        {"declined_reference",prefix_after.declined_reference-prefix_before.declined_reference},
        {"declined_instruction",prefix_after.declined_instruction-prefix_before.declined_instruction},
        {"declined_view",prefix_after.declined_view-prefix_before.declined_view},
        {"declined_numeric",prefix_after.declined_numeric-prefix_before.declined_numeric}};
    result.evidence["validation_messages"]=0;result.evidence["frame_sha256"]=sha256(result.ppm);
    Words ordered;for(const auto &entry:result.trace){ordered.word(entry.tick);ordered.word(entry.view);
        for(auto c:entry.kind)ordered.word(std::uint8_t(c));ordered.word(0);for(auto c:entry.digest)ordered.word(std::uint8_t(c));}
    result.evidence["ordered_callbacks"]=result.trace.size();result.evidence["ordered_callback_sha256"]=sha256(ordered.bytes);return result;
}
void save(const std::filesystem::path &path,const Json &report) {
    const auto value=report.dump(2)+"\n";need(value.size()<=1024*1024,"Bounded runtime report exceeded1MiB");
    if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);out<<value;need(bool(out),"Write bounded ordinary runtime report");
}
}
int main(int argc,char **argv) {
    const auto *control_mode=std::getenv("HALO2_MODEL_SKIN_RUNTIME_REFERENCE_ONLY");
    const bool reference_only=control_mode && std::string_view(control_mode)=="1";
    const auto *fallback_mode=std::getenv("HALO2_MODEL_SKIN_RUNTIME_FALLBACK_CONTROL");
    const std::string fallback_control=fallback_mode?fallback_mode:"";const bool forced_fallback=!fallback_control.empty();
    const auto *common_cut_mode=std::getenv("HALO2_MODEL_SKIN_RUNTIME_POST_CONSTRUCTOR_COMMON_CUT");
    const bool post_constructor_common_cut=common_cut_mode && std::string_view(common_cut_mode)=="1";
    Json report={{"passed",false},{"cases",Json::array()},
        {"scope","Paired fresh ordinary prepared-MCC source worlds, real command scripts, actual weighted GPU dispatch/readback, exact full PPM file bytes, ordered explicit input/state/WORLD-node/event/camera/particle/portal/frame callback hashes. This is bounded regression evidence, not all-memory, full-playthrough, physical latency or FPS acceptance."},
        {"state_fields","Every roster entry: identity/configuration, core health/shield/motion/weapon/ammo/scoring, trigger/secondary weapon, zoom/source flags, recent damage/camouflage; all completed WORLD nodes/spheres and scene roots; retained weapon/bullet/damage/device/rumble events. Other Player/engine/entity fields are outside this bounded trace."}};
    report["source_trace_scope"]="CPU records real constructor/preparation/runtime/final-read/teardown external operands; fresh comparison owner replays exact kind/operand/value order. Complete ordered passive node/input/camera records and committed-bank digests at construction, all60prep ticks, command tails and final frame are compared without normalization. This is not every transient callback or opaque host ABI.";
    report["control_only"]=reference_only || forced_fallback || post_constructor_common_cut;report["ordinary_gpu_acceptance"]=false;
    report["qualification_kind"]=post_constructor_common_cut?"post-constructor-common-cut":forced_fallback?"forced-cpu-materialization":reference_only?"cpu-reference-control":"ordinary-gpu";
    if(post_constructor_common_cut) {
        report["post_constructor_common_cut"]=true;report["post_constructor_qualification_passed"]=false;
        report["actual_gpu_acceptance_run"]=false;
        report["scope"]="Separate post-constructor common-cut CPU/GPU control: genuine untaped constructors, unchanged complete coupled source/resource/Match import, then actual prep/runtime/final-read operand replay and exact presented PPM/passive/typed trace gates. No cold-constructor, native teardown, ordinary GPU, full ABI or performance acceptance.";
        report["source_trace_scope"]="One CPU-owned complete SourceMatchAuthority cut at completed constructor plus actual local-view binding, before60prep ticks/projection/runtime. Both roles directly prove complete initial source topology/bytes and committed runs, then compare ordered passive traces from this cut. Constructors and native teardown are outside the new tape; pre-cut RNG receipts are retained but not compared.";
    }
    if(forced_fallback) {
        report["actual_gpu_acceptance_run"]=false;report["forced_prepare_failure"]=fallback_control;
        report["fallback_qualification_passed"]=false;
        report["reason"]="Explicit forced CPU materialization qualification; passed remains false and ordinary GPU/pin acceptance is not granted.";
        report["scope"]="Paired fresh prepared-MCC source worlds with identical finite commands and pinned RNG: original CPU skinning versus actual late CPU materialization after an explicitly forced GPU prepare resource refusal. Requires exact final PPM bytes and complete ordered source-RNG/input/state/presentation trace, weighted fallback work and the requested refusal reason, and zero GPU dispatch/readback counters. This separate qualification grants no ordinary GPU/pin acceptance.";
    }
    if(reference_only) {
        report["actual_gpu_acceptance_run"]=false;
        report["reason"]="CPU-to-CPU fresh-world determinism diagnostic only; actual GPU acceptance was not run. This mode always reports passed=false and exits nonzero.";
        report["scope"]="Two fresh ordinary prepared-MCC worlds with HALO2_REFERENCE_CPU_SKINNING=1 in both runs; compare real final frame bytes and the same explicit ordered callback trace. No animation/source-draw/effect observer is added to ordinary first-person cohorts. This is a nonaccepting CPU control diagnostic, not GPU quality or pin acceptance.";
    }
    try {
        need(!common_cut_mode || std::string_view(common_cut_mode)=="0" || post_constructor_common_cut,"Common-cut mode must be explicitly0or1");
        need(!post_constructor_common_cut || (!reference_only && !forced_fallback),"Common-cut qualification excludes reference-only and forced fallback controls");
        need(!forced_fallback || (!reference_only && fallback_reason(fallback_control)),"Fallback control requires exactly one admitted resource-refusal reason and excludes reference-only mode");
        need(argc==4 || argc==5 || argc==7 || argc==9,"Supply prepared MCC Coagulation MAP OWNED_OUTPUT BOUNDED_REPORT [cohort-filter [WIDTH HEIGHT [PRESENT_WIDTH PRESENT_HEIGHT]]]");
        unsigned width=640,height=480,present_width{},present_height{};const bool dual_extent=argc==9;
        if(argc>=7) {
            const auto dimension=[](const char *text) {
                unsigned value{};const std::string_view source=text;
                const auto parsed=std::from_chars(source.data(),source.data()+source.size(),value);
                need(parsed.ec==std::errc{} && parsed.ptr==source.data()+source.size() && value>=64 && value<=4096,
                    "Requested dimensions must each be an integer in64..4096");return value;
            };
            width=dimension(argv[5]);height=dimension(argv[6]);
            if(dual_extent){present_width=dimension(argv[7]);present_height=dimension(argv[8]);}
        }
        if(!dual_extent){present_width=width;present_height=height;}
        report["requested_width"]=width;report["requested_height"]=height;
        report["requested_present_width"]=present_width;report["requested_present_height"]=present_height;
        report["dual_extent_mode"]=dual_extent;
        report["framebuffer_extent_kind"]="presented";
        report["extent_scope"]="Complete presented framebuffer equivalence at the separately requested internal render extent; no physical display mode, latency or FPS qualification.";
        need(std::getenv("HALO2_TEST_SCRATCH"),"Ordinary skin runtime regression requires owned scratch");
        const auto scratch=std::filesystem::weakly_canonical(std::getenv("HALO2_TEST_SCRATCH"));
        const auto output=std::filesystem::weakly_canonical(argv[2]);const auto relative=output.lexically_relative(scratch);
        need(!relative.empty() && !relative.is_absolute() && *relative.begin()!="..","Raw runtime outputs must stay inside owned scratch");
        Cache cache(argv[1]);need(cache.is_mcc() && cache.name()=="coagulation","Runtime skin cohort requires explicit prepared MCC Coagulation");
        SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
        const std::string filter=argc>=5?argv[4]:"";unsigned cases{};
        for(bool first_person:{false,true})for(unsigned players:{1u,2u,4u})for(unsigned species=0;species<2;++species)for(bool fire:{false,true}) {
            const auto name=std::string(species?"elite":"spartan")+(first_person?"-fp-":"-")+std::to_string(players)+"p-"+(fire?"fire":"neutral");
            if(!filter.empty() && name.find(filter)==std::string::npos)continue;
            auto &entry=report["cases"].emplace_back(Json{{"name",name},{"players",players},{"species",species?"Elite":"Spartan"},
                {"fire",fire},{"ordinary_first_person",first_person},{"control_only",reference_only || forced_fallback || post_constructor_common_cut},{"passed",false}});
            std::optional<CommonCut> common_cut;
            if(post_constructor_common_cut) {
                common_cut.emplace();entry.update({{"post_constructor_common_cut",true},{"post_constructor_qualification_passed",false},
                    {"common_cut_released_after_pair",false}});
            }
            if(forced_fallback)entry.update({{"forced_prepare_failure",fallback_control},{"fallback_qualification_passed",false}});
            save(argv[3],report);const auto cpu=run_case(argv[1],output/(name+"-cpu"),players,species,fire,false,first_person,width,height,
                present_width,present_height,dual_extent,entry["cpu"],{},nullptr,common_cut?&*common_cut:nullptr);entry["cpu"]=cpu.evidence;save(argv[3],report);
            const auto comparison_key=forced_fallback?"forced_fallback":reference_only?"cpu_control":"gpu";
            const auto comparison=run_case(argv[1],output/(name+(forced_fallback?"-forced-fallback":reference_only?"-cpu-control":"-gpu")),players,species,fire,!reference_only,
                first_person,width,height,present_width,present_height,dual_extent,entry[comparison_key],fallback_control,&cpu.external_inputs,
                common_cut?&*common_cut:nullptr);entry[comparison_key]=comparison.evidence;
            if(common_cut){common_cut.reset();entry["common_cut_released_after_pair"]=true;}
            entry["ordered_trace_identical"]=cpu.trace==comparison.trace;entry["full_frame_identical"]=cpu.ppm==comparison.ppm;
            entry["passive_source_trace_identical"]=cpu.passive_trace==comparison.passive_trace && cpu.passive_counts==comparison.passive_counts;
            entry["external_input_tape_identical"]=cpu.external_inputs==comparison.external_inputs;
            if(reference_only || forced_fallback || cpu.ppm!=comparison.ppm)entry["pixel_difference"]=frame_difference(cpu.ppm,comparison.ppm,present_width,present_height);
            save(argv[3],report);
            if(cpu.passive_trace!=comparison.passive_trace || cpu.passive_counts!=comparison.passive_counts) {
                std::size_t first{};while(first<std::min(cpu.passive_trace.size(),comparison.passive_trace.size()) &&
                    cpu.passive_trace[first]==comparison.passive_trace[first])++first;
                entry["first_passive_trace_byte_mismatch"]=first;save(argv[3],report);
                throw FormatError("Actual ordered passive node/source/input/camera trace differs: "+name);
            }
            need(cpu.external_inputs==comparison.external_inputs,"Actual replayed source input tape differs from its declared CPU scope");
            // Preserve actual failed-pair inputs before either rejection can
            // unwind the owned raw directory. World-body mismatches need no
            // first-person source-bank diagnostic; FP keeps its existing gate.
            if(cpu.ppm!=comparison.ppm)
                if(const auto *keep=std::getenv("HALO2_MODEL_SKIN_RUNTIME_KEEP_MISMATCH_FRAMES");keep && std::string_view(keep)=="1" &&
                    !reference_only && !forced_fallback && (!first_person || comparison.evidence.value("first_person_source_bank_check_completed",false))) {
                    entry["retained_frame_references"]=retain_frame_references(argv[3],name,cpu.ppm,comparison.ppm);save(argv[3],report);
                }
            if(cpu.trace!=comparison.trace) {
                unsigned first{};while(first<std::min(cpu.trace.size(),comparison.trace.size()) && cpu.trace[first]==comparison.trace[first])++first;
                entry["first_callback_mismatch"]=first;save(argv[3],report);
                throw FormatError(std::string(forced_fallback?"Forced CPU materialization":reference_only?"CPU reference control":"GPU path")+" changed ordered explicit gameplay/presentation callback trace: "+name);
            }
            if(cpu.ppm!=comparison.ppm) {
                if(!reference_only && !forced_fallback)entry["selected_local_binding_metadata_scope"]="No diagnostic source-draw observer is installed. GPU readback compares complete Vertex records; the optional first-person source-bank check independently compares original skin/select/identity assembly, including selected material and all vertex metadata.";
                save(argv[3],report);
                throw FormatError(std::string(forced_fallback?"Forced CPU materialization":reference_only?"CPU reference control":"Ordinary GPU path")+" changed complete final PPM bytes: "+name);
            }
            entry["complete_ppm_bytes"]=cpu.ppm.size();entry["passed"]=!reference_only && !forced_fallback && !post_constructor_common_cut;
            if(post_constructor_common_cut)entry["post_constructor_qualification_passed"]=true;
            if(forced_fallback)entry["fallback_qualification_passed"]=true;
            if(reference_only)entry["control_comparison_identical"]=true;
            save(argv[3],report);++cases;
            if(post_constructor_common_cut)std::cout<<"POST_CONSTRUCTOR_QUALIFIED "<<name<<" complete common cut/frame/ordered trace; "<<comparison.gpu_weighted<<" weighted GPU vertices verified; ordinary GPU acceptance not granted\n";
            else if(forced_fallback)std::cout<<"FALLBACK_QUALIFIED "<<name<<" complete frame/source-RNG/ordered trace; "<<comparison.fallback_weighted<<" weighted CPU vertices; ordinary GPU acceptance not granted\n";
            else if(reference_only)std::cout<<"CONTROL_IDENTICAL "<<name<<" complete CPU/CPU frame and ordered trace; GPU acceptance not run\n";
            else std::cout<<"PASS "<<name<<" complete frame/ordered source trace; "<<comparison.gpu_weighted<<" weighted GPU vertices verified\n";
        }
        need(cases>0 && (!filter.empty() || cases==24),"Ordinary runtime cohort omitted requested cases");
        if(post_constructor_common_cut) {
            report["post_constructor_qualification_passed"]=true;report["diagnostic_completed"]=true;save(argv[3],report);return 0;
        }
        if(forced_fallback) {
            report["fallback_qualification_passed"]=true;report["diagnostic_completed"]=true;save(argv[3],report);return 0;
        }
        if(reference_only) {
            report["diagnostic_completed"]=true;report["control_comparisons_identical"]=true;save(argv[3],report);
            std::cerr<<report["reason"].get<std::string>()<<'\n';return 1;
        }
        report["passed"]=true;report["ordinary_gpu_acceptance"]=true;save(argv[3],report);return 0;
    }catch(const std::exception &e){report["error"]=e.what();if(argc>=4)try{save(argv[3],report);}catch(...){}std::cerr<<e.what()<<'\n';return 1;}
}
