#pragma once
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <string_view>
namespace halo2::shadow_test {
// Hash only defined submitted fields, never padding, pointers or GPU handles.
// This is a comparison of observed inputs, not a whole-frame parity proof.
struct InputHash {
    std::uint64_t value=14695981039346656037ull;
    void byte(std::uint8_t b){value=(value^b)*1099511628211ull;}
    void u32(std::uint32_t v){for(unsigned n=0;n<4;++n)byte(std::uint8_t(v>>(n*8)));}
    void u64(std::uint64_t v){for(unsigned n=0;n<8;++n)byte(std::uint8_t(v>>(n*8)));}
    void real(float v){u32(std::bit_cast<std::uint32_t>(v));}
    void flag(bool v){byte(v?1:0);}
    void vec(Vec2 v){real(v.x);real(v.y);}
    void vec(Vec3 v){real(v.x);real(v.y);real(v.z);}
    void transform(const Transform &v){real(v.scale);for(const auto &c:v.basis)vec(c);vec(v.position);}
    void nodes(std::span<const Transform> values){u64(values.size());for(const auto &v:values)transform(v);}
    void vertices(std::span<const Vertex> values){u64(values.size());for(const auto &v:values){
        vec(v.position);vec(v.color);vec(v.uv);u32(v.material);vec(v.lightmap_uv);
        vec(v.normal);vec(v.tangent);vec(v.binormal);vec(v.incident);u32(v.lightmap);
        u32(std::bit_cast<std::uint32_t>(v.bsp_cluster));real(v.prt_cluster);
        for(float f:v.prt_weights)real(f);u32(v.source_section);u32(v.source_part);
        u32(v.source_stipple_level);vec(v.source_local_position);real(v.source_color_alpha);
        u32(v.source_part_type);u32(v.source_part_flags);
    }}
    void bytes(std::span<const std::byte> values){u64(values.size());for(auto v:values)byte(std::to_integer<std::uint8_t>(v));}
    void camera(const Camera &v){vec(v.position);real(v.yaw);real(v.pitch);real(v.field_of_view);
        flag(v.oriented);flag(v.horizontal_field_of_view);vec(v.forward);vec(v.up);real(v.roll);
        flag(v.projection_vertical_angle.has_value());if(v.projection_vertical_angle)real(*v.projection_vertical_angle);
        vec(v.projection_clip_scale);vec(v.projection_clip_offset);
    }
    void texture(const ShaderTextureState &v){
        for(auto f:{v.address_u,v.address_v,v.address_w,v.mag_filter,v.min_filter,v.mip_filter,
            v.lod_bias,v.max_mip,v.anisotropy,v.color_key_op,v.color_sign,v.alpha_kill,v.border_color,v.key_color})u32(f);
    }
};
struct SubmittedInputs {
    InputHash camera,animation,node,depth,material;
    nlohmann::json animation_details=nlohmann::json::array(),node_details=nlohmann::json::array();
    static nlohmann::json node_words(std::span<const Transform> values) {
        if(values.size()>128)throw FormatError("Submitted node diagnostic exceeds its storage bound");
        auto rows=nlohmann::json::array();
        for(const auto &v:values) {
            auto row=nlohmann::json::array();
            const auto real=[&](float f){row.push_back(std::bit_cast<std::uint32_t>(f));};
            real(v.scale);for(const auto &c:v.basis){real(c.x);real(c.y);real(c.z);}
            real(v.position.x);real(v.position.y);real(v.position.z);rows.push_back(std::move(row));
        }
        return rows;
    }
    static nlohmann::json vertex_fields(std::span<const Vertex> vertices) {
        InputHash positions,normals,materials,local;
        for(const auto &v:vertices){positions.vec(v.position);normals.vec(v.normal);
            materials.u32(v.material);local.vec(v.source_local_position);}
        return {{"count",vertices.size()},{"positions",std::to_string(positions.value)},
            {"normals",std::to_string(normals.value)},{"materials",std::to_string(materials.value)},
            {"source_local_positions",std::to_string(local.value)}};
    }
    std::uint64_t cameras{},animations{},animation_vertices{},animation_nodes{},node_observations{},node_transforms{},
        depth_observations{},depth_selections{},depth_ordinals{},depth_before_vertices{},depth_after_vertices{},
        material_uploads{},material_bytes{},vertex_bank_bytes{},page_fingerprints{};
    std::array<std::uint64_t,6> node_stages{};
    nlohmann::json json()const {
        return {{"scope","Observed submitted camera, body/FP geometry/nodes, native model-depth banks and completed material uploads; no whole-frame parity claim"},
            {"camera",{{"fnv1a64",std::to_string(camera.value)},{"observations",cameras}}},
            {"primary_geometry",{{"fnv1a64",std::to_string(animation.value)},{"observations",animations},
                {"vertices",animation_vertices},{"nodes",animation_nodes}}},
            {"node_palettes",{{"fnv1a64",std::to_string(node.value)},{"observations",node_observations},
                {"nodes",node_transforms},{"stage_counts",node_stages}}},
            {"animation_details",animation_details},{"node_details",node_details},
            {"model_depth",{{"fnv1a64",std::to_string(depth.value)},{"observations",depth_observations},
                {"selections",depth_selections},{"source_ordinals",depth_ordinals},
                {"before_vertices",depth_before_vertices},{"after_vertices",depth_after_vertices}}},
            {"material_uploads",{{"fnv1a64",std::to_string(material.value)},{"observations",material_uploads},
                {"material_uniform_bytes",material_bytes},{"vertex_bank_uniform_bytes",vertex_bank_bytes},
                {"source_page_fingerprints",page_fingerprints}}}};
    }
};
struct Capture {
    std::string mode;
    std::filesystem::path picture;
    unsigned tick=180;
    unsigned expected_players{};
    bool exact_tick=true,joining=true,gpu_disabled_control=false;
    std::optional<unsigned> local_slot;
    std::function<void()> advance,finish,after_capture;
    std::function<bool()> capture_ready;
};
inline void need(bool value,const char *why){if(!value)throw FormatError(why);}
inline nlohmann::json capture(MultiplayerOptions options,const Capture& test) {
    using Clock=std::chrono::steady_clock;
    const auto& mode=test.mode;nlohmann::json report={{"mode",mode}};
    const auto* disabled=std::getenv("HALO2_REFERENCE_DISABLE_PLAYER_SHADOW_GPU");
    need(bool(disabled&&std::string_view(disabled)=="1")==test.gpu_disabled_control,
        "Network shadow GPU control differs from its explicit fixture request");
    need(!options.capture_tick,"Network shadow helper requires actual frame_capture without a capture-tick override");
    need(options.local_players==1&&options.prepared_world,"Network GPU shadow proof requires one actual local view");
    need(bool(options.prepared_world->source_objects)!=test.joining,"Network shadow helper has the wrong gameplay authority owner");
        options.desktop=std::make_shared<DesktopSession>();
        options.desktop->display_settings.graphics_mode=mode=="Original"?pstencil::GraphicsMode::Original:
            mode=="pStencil"?pstencil::GraphicsMode::pStencil:pstencil::GraphicsMode::PathTracing;
        options.desktop->keyboard_settings.sensitivity=3;options.desktop->keyboard_settings.invert=false;
        auto *window=options.desktop->window(640,480,false);
        SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
        need(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Joined shadow check could not capture actual mouse input");
        report["video_driver"]=SDL_GetCurrentVideoDriver()?SDL_GetCurrentVideoDriver():"";
        need(report["video_driver"]=="x11","Joined shadow GPU check must use its private X11 server");
        unsigned frames{},captured_tick{},last_tick{},local_slot=~0u,root_tick{},last_progress_tick{};std::uint32_t local_unit=~0u,local_player=~0u;
        bool looked{},look_confirmed{},capture_requested{},captured{},selected_local{},current_selected{},living{};
        std::uint64_t generate{},apply{};
        const auto started=Clock::now();
        auto diagnostic_at=started+std::chrono::seconds(5);
        const auto picture=test.picture;
        std::uint64_t ordinary_receipts{};unsigned ordinary_first{},ordinary_last{};
        std::uint32_t ordinary_sequence{};bool ordinary_sequence_set{};
        auto ordinary_begin=started,ordinary_end=started;
        auto root_observed_at=started;unsigned stable_root_tick{};
        nlohmann::json native_progress=nlohmann::json::array();
        SubmittedInputs submitted_inputs;
        options.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera) {
            auto &h=submitted_inputs.camera;h.u32(view);h.u32(state.tick);h.camera(camera);++submitted_inputs.cameras;
        };
        options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation &observation) {
            auto &h=submitted_inputs.animation;h.u32(view);h.u32(player);h.u32(observation.tick);
            h.u32(observation.unit);h.u32(observation.weapon);h.flag(observation.first_person);
            h.vertices(observation.vertices);h.vertices(observation.weapon_vertices);
            h.nodes(observation.body_nodes);h.nodes(observation.weapon_nodes);
            ++submitted_inputs.animations;
            submitted_inputs.animation_vertices+=observation.vertices.size()+observation.weapon_vertices.size();
            submitted_inputs.animation_nodes+=observation.body_nodes.size()+observation.weapon_nodes.size();
            need(submitted_inputs.animation_details.size()<32,"Submitted animation diagnostic exceeds its frame bound");
            submitted_inputs.animation_details.push_back({{"view",view},{"player",player},{"tick",observation.tick},
                {"unit",observation.unit},{"weapon",observation.weapon},{"first_person",observation.first_person},
                {"fraction",observation.fraction},{"frame",observation.frame},{"graph",observation.graph},
                {"animation",observation.animation},{"interpolated",observation.interpolated},
                {"body_activity",observation.body_activity},{"body_facing",observation.body_facing},
                {"vertices",SubmittedInputs::vertex_fields(observation.vertices)},
                {"weapon_vertices",SubmittedInputs::vertex_fields(observation.weapon_vertices)},
                {"body_nodes",SubmittedInputs::node_words(observation.body_nodes)},
                {"weapon_nodes",SubmittedInputs::node_words(observation.weapon_nodes)}});
        };
        options.observe_submitted_nodes=true;
        options.node_observer=[&](unsigned view,std::uint32_t tick,const MultiplayerNodeObservation &observation) {
            const auto stage=std::uint32_t(observation.stage);
            need(stage>=1&&stage<=6,"Node hash received an unknown observation stage");
            // Only actual final-submission palettes are primary geometry
            // inputs; completed/interpolated endpoints remain diagnostics.
            if(stage<5)return;
            auto &h=submitted_inputs.node;h.u32(view);h.u32(tick);h.u32(stage);
            h.u32(observation.owner);h.u32(observation.render_model);h.u32(observation.mode);
            h.u32(observation.hand);h.flag(observation.parent_known);
            if(observation.parent_known){h.u32(observation.parent);h.u32(observation.parent_node);}
            h.nodes(observation.nodes);++submitted_inputs.node_observations;
            submitted_inputs.node_transforms+=observation.nodes.size();++submitted_inputs.node_stages[stage-1];
            need(submitted_inputs.node_details.size()<32,"Submitted palette diagnostic exceeds its frame bound");
            submitted_inputs.node_details.push_back({{"view",view},{"tick",tick},{"stage",stage},
                {"owner",observation.owner},{"render_model",observation.render_model},{"mode",observation.mode},
                {"hand",observation.hand},{"fraction",observation.fraction},
                {"nodes",SubmittedInputs::node_words(observation.nodes)}});
        };
        options.model_depth_vertices_observer=[&](unsigned view,std::uint32_t tick,
            std::span<const SourceModelDepthSelection> selections,std::span<const Vertex> before,std::span<const Vertex> after) {
            auto &h=submitted_inputs.depth;h.u32(view);h.u32(tick);h.u64(selections.size());
            for(const auto &selection:selections){h.u32(selection.object);h.u32(selection.definition);
                h.u32(selection.render_model);h.u32(selection.lod);h.u64(selection.first);
                h.u64(selection.source_vertices.size());for(auto ordinal:selection.source_vertices)h.u32(ordinal);
                submitted_inputs.depth_ordinals+=selection.source_vertices.size();}
            h.vertices(before);h.vertices(after);++submitted_inputs.depth_observations;
            submitted_inputs.depth_selections+=selections.size();
            submitted_inputs.depth_before_vertices+=before.size();submitted_inputs.depth_after_vertices+=after.size();
        };
        options.completed_material_upload_tick=test.exact_tick?test.tick:0;
        options.completed_material_upload_observer=[&](unsigned view,std::uint32_t tick,const RendererCompletedMaterialUpload &upload) {
            auto &h=submitted_inputs.material;h.u32(view);h.u32(tick);h.u64(upload.root_material_count);
            h.u64(upload.expanded_material_count);h.u64(upload.root_slot);h.u64(upload.material_slot);
            h.u32(upload.source_shader);h.u32(upload.selected_shader_lod);
            for(auto frame:upload.wanted_frames)h.u32(frame);for(auto frame:upload.bound_frames)h.u32(frame);
            for(const auto &state:upload.texture_states)h.texture(state);
            for(const auto &page:upload.cached_material_pages){h.u32(page.width);h.u32(page.height);h.u32(page.depth);
                h.u32(page.layers);h.u32(page.mip_count);h.u32(page.format);h.flag(page.source_image);
                h.flag(page.source_pixel_fingerprint.has_value());if(page.source_pixel_fingerprint){
                    h.u64(*page.source_pixel_fingerprint);++submitted_inputs.page_fingerprints;}
                h.flag(page.source_mutated.has_value());if(page.source_mutated)h.flag(*page.source_mutated);
                h.u64(page.source_mips.size());for(const auto &mip:page.source_mips){h.u32(mip.width);h.u32(mip.height);
                    h.u32(mip.depth);h.u64(mip.offset);h.u64(mip.size);}}
            h.flag(upload.native_world_kernel.has_value());if(upload.native_world_kernel)h.u32(*upload.native_world_kernel);
            h.u32(upload.uploaded_vertex_instruction_words);h.flag(upload.baked_lightmap);h.flag(upload.accumulation);h.flag(upload.scope);
            for(bool target:upload.water_targets)h.flag(target);
            h.bytes(upload.material_uniform);h.bytes(upload.vertex_bank_uniform);
            ++submitted_inputs.material_uploads;submitted_inputs.material_bytes+=upload.material_uniform.size();
            submitted_inputs.vertex_bank_bytes+=upload.vertex_bank_uniform.size();
            // Deliberately omit target generation, frame counters and all
            // Vulkan image/view/sampler identities from independent runs.
        };
        options.replica_observer=[&](const MultiplayerState& state,const MultiplayerSnapshotOrigin& origin,bool advanced) {
            need(origin.tick()&&*origin.tick()==state.tick&&origin.sequence(),
                "Gameplay cadence observation lost its actual accepted body origin");
            if(ordinary_sequence_set&&*origin.sequence()!=ordinary_sequence)
                need(std::int32_t(*origin.sequence()-ordinary_sequence)>0,
                    "Historical native delivery rolled ordinary gameplay sequence backwards");
            ordinary_sequence=*origin.sequence();ordinary_sequence_set=true;
            if(advanced) {
                if(!ordinary_receipts){ordinary_first=state.tick;ordinary_begin=Clock::now();}
                ++ordinary_receipts;ordinary_last=state.tick;ordinary_end=Clock::now();
            }
        };
        // The fixed maximum keeps this a finite capture. The frame callback
        // requests the actual settled tick instead of relabeling a snapshot.
        options.frames=1800;
        options.input_observer=[&](auto,std::span<const Controls> input,const MultiplayerState& state) {
            submitted_inputs={};
            if(test.advance)test.advance();last_tick=state.tick;
            living=false;
            for(unsigned slot=0;slot<state.players.size();++slot) {
                const auto& player=state.players[slot];
                if((test.local_slot?slot==*test.local_slot:bool(player.source_player_camera))&&
                    player.connected&&!player.dead&&player.source_player!=~0u&&player.source_unit!=~0u&&
                    (player.source_player&65535u)==slot) {
                    local_slot=slot;local_player=player.source_player;local_unit=player.source_unit;living=true;
                }
            }
            if(!looked&&!input.empty()&&local_unit!=~0u) {
                SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=SDL_GetWindowID(window);
                e.motion.yrel=-int(std::lround((-1.5f-input[0].pitch)/.0025f));
                need(SDL_PushEvent(&e)==1,"Could not deliver actual joined down-look input");looked=true;
            }
            if(looked&&!input.empty()&&std::abs(input[0].pitch+1.5f)<=.01f)look_confirmed=true;
            if(Clock::now()>=diagnostic_at) {
                const auto connected=std::count_if(state.players.begin(),state.players.end(),[](const auto& p){return p.connected;});
                std::cerr<<"Shadow GPU client progress tick="<<state.tick<<" submitted_frames="<<frames
                    <<" connected="<<connected<<" local_slot="<<local_slot<<" player="<<local_player
                    <<" unit="<<local_unit<<" living="<<living<<" look_queued="<<looked
                    <<" look_observed="<<look_confirmed<<" input_pitch="<<(input.empty()?0.f:input[0].pitch)<<std::endl;
                diagnostic_at=Clock::now()+std::chrono::seconds(5);
            }
            need(Clock::now()-started<std::chrono::seconds(150),"Joined player shadow never reached its finite capture boundary");
        };
        options.shadow_root_frame_observer=[&](auto,auto tick,std::span<const SourceLiveShadowRootTu15> roots) {
            if(stable_root_tick!=tick){stable_root_tick=tick;root_observed_at=Clock::now();}
            if(tick!=last_progress_tick) {
                last_progress_tick=tick;
                if(native_progress.size()==64)native_progress.erase(native_progress.begin());
                native_progress.push_back({{"tick",tick},{"elapsed_ms",std::chrono::duration<double,std::milli>(Clock::now()-started).count()}});
            }
            current_selected=false;root_tick=tick;
            for(const auto &root:roots)if(root.owner==local_unit&&root.commands&&root.uniforms)
                current_selected=true;

        };
        options.frame_capture=[&](const MultiplayerState& state)->std::filesystem::path {
            // A settled host has no successor native tick. Wait the complete
            // admitted sparse interpolation window before pixel controls.
            const bool settled=!test.exact_tick||Clock::now()-root_observed_at>=std::chrono::seconds(4);
            if(!capture_requested&&settled&&(test.exact_tick?state.tick==test.tick:state.tick>=test.tick)&&frames>=90&&look_confirmed&&living&&(test.exact_tick?root_tick==state.tick:root_tick>=test.tick&&root_tick<=state.tick)&&(!test.capture_ready||test.capture_ready())) {
                need(local_slot<16&&state.players[local_slot].source_player==local_player&&
                    state.players[local_slot].source_unit==local_unit&&!state.players[local_slot].dead,
                    "Network capture lost its actual living local full-salt identity");
                nlohmann::json players=nlohmann::json::array();
                for(unsigned slot=0;slot<state.players.size();++slot) {
                    const auto& player=state.players[slot];
                    if(player.connected&&!(player.source_flags&2u))players.push_back({{"slot",slot},
                        {"player",player.source_player},{"unit",player.source_unit},{"alive",!player.dead}});
                }
                if(test.expected_players)need(players.size()==test.expected_players,
                    "Network GPU capture lost a genuinely admitted active participant");
                report["players"]=std::move(players);
                capture_requested=true;captured_tick=state.tick;selected_local=root_tick<=state.tick&&current_selected;return picture;
            }
            return {};
        };
        unsigned timing_rows{};
        auto last_submitted_at=started;
        options.frame_observer=[&](const MultiplayerFrameTiming& frame) {
            const auto observed_at=Clock::now();
            if(timing_rows<16 && (frames<8 || observed_at-last_submitted_at>std::chrono::seconds(1))) {
                ++timing_rows;
                std::cerr<<"Shadow GPU timing: frame="<<frame.frame<<" submitted="<<frame.submitted
                    <<" elapsed_ms="<<std::chrono::duration<double,std::milli>(observed_at-started).count()
                    <<" gap_ms="<<std::chrono::duration<double,std::milli>(observed_at-last_submitted_at).count()
                    <<" wall_ms="<<frame.wall_ms<<" input_ms="<<frame.input_ms
                    <<" common_ms="<<frame.common_ms<<" renderer_ms="<<frame.renderer_ms
                    <<" send_calls="<<frame.schedule.client_send_calls<<'\n';
            }
            if(frame.submitted)last_submitted_at=observed_at;
            frames+=frame.submitted;
            if(!capture_requested){current_selected=false;root_tick=0;}
            if(capture_requested&&frame.submitted&&!captured) {
                generate=frame.renderer.world_native_shadow_generate_prepared_draws;
                apply=frame.renderer.world_native_shadow_apply_prepared_draws;captured=true;
                need(submitted_inputs.cameras&&submitted_inputs.animation_vertices&&submitted_inputs.node_transforms,
                    "Captured frame lacks its actual camera/primary geometry/palette hash observations");
                if(test.exact_tick)need(submitted_inputs.material_uploads&&submitted_inputs.material_bytes,
                    "Settled captured frame lacks completed material upload observations");
                report["submitted_inputs"]=submitted_inputs.json();
                if(test.after_capture)test.after_capture();
                SDL_Event quit{};quit.type=SDL_QUIT;need(SDL_PushEvent(&quit)==1,"Could not finish joined shadow capture");
            }
            // A render can occur without a new fixed input tick. Retire the
            // previous draw's hashes here as well as at actual input begin.
            submitted_inputs={};
        };
        const auto result=run_multiplayer(options);if(test.finish)test.finish();
        report["return_code"]=result;report["frames"]=frames;report["tick"]=captured_tick;
        report["native_root_tick"]=root_tick;report["local_unit"]=local_unit;report["local_selected"]=selected_local;
        report["generate_draws"]=generate;report["apply_draws"]=apply;
        report["local_slot"]=local_slot;report["local_player"]=local_player;report["local_unit_alive"]=living;report["last_received_tick"]=last_tick;
        report["down_look_observed"]=look_confirmed;
        report["ordinary_gameplay"]={{"advanced_receipts",ordinary_receipts},{"first_tick",ordinary_first},
            {"last_tick",ordinary_last},{"last_sequence",ordinary_sequence},
            {"elapsed_seconds",std::chrono::duration<double>(ordinary_end-ordinary_begin).count()}};
        report["native_tick_progress"]=std::move(native_progress);
        report["joining_world_has_no_gameplay_authority"]=!options.prepared_world->source_objects;
        need(result==0&&captured&&captured_tick>=test.tick&&(!test.exact_tick||captured_tick==test.tick)&&local_unit!=~0u&&local_player!=~0u,
            "Joined shadow check did not capture an actual living local player");
        need(std::filesystem::file_size(picture)>640u*480u*3u,"Joined shadow capture is missing its actual RGB pixels");
        if(test.gpu_disabled_control)need(selected_local&&!generate&&!apply,
            "GPU-off control lost native root selection or submitted shadow GPU draws");
        else if(mode!="PathTracing")need(selected_local&&generate&&apply,
            "Joining living player never reached native Generate and Apply GPU submission");
    report["gpu_disabled_control"]=test.gpu_disabled_control;
    report["picture"]=test.picture.string();report["passed"]=true;
    report["retained_large_artifacts"]=nlohmann::json::array();return report;
}
}
