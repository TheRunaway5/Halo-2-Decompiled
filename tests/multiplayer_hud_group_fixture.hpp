#pragma once
#include "halo2/multiplayer_hud_continuity.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/hud.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/material_presentation_continuity.hpp"
#include "halo2/native_weapon_hud.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/action_prompt_bindings.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <new>
#include <set>
#include <thread>
#include <tuple>
#include <type_traits>

// Driver: construct beside NativeSceneContinuityFixture before its early
// receipt capture, then call capture(early). At the existing genuine later
// source tick call restore(early, early_proof, alternate_early_proof, later,
// later_proof). Every receipt/proof is supplied by the existing LanClient/native
// checkpoint path; this fixture creates neither receipts nor proof seals.
// Its registry preparation follows multiplayer_runtime.cpp's authored census.
// Legitimately absent vehicle/parent roles stay absent and are reported, never
// filled with renamed weapon HUDs. No rendered role/gameplay coverage is implied.
struct NativeHudGroupFixture {
    using Group=halo2::MultiplayerHudContinuity;
    using Hud=halo2::MultiplayerHud;
    using Board=halo2::MultiplayerScoreboard;
    halo2::MultiplayerPreparedWorld &world;
    halo2::Mesh mesh;
    Group::DefinitionRegistry primary,offhand,vehicle,parent;
    Group::UnitRegistry unit;
    std::unique_ptr<Board> scoreboard;
    std::unique_ptr<halo2::MaterialPresentationContinuity> materials;
    std::unique_ptr<Group> continuity;
    Group::Snapshot early;
    halo2::MaterialPresentationContinuity::Checkpoint early_materials;
    struct Draw {Hud *owner{};unsigned view{};std::vector<halo2::Vertex> vertices;};
    std::vector<Draw> early_draws;
    std::vector<Board::Draw> early_boards;
    std::size_t material_count{};
    const halo2::Material *material_roots{};
    static void require(bool yes,const char *why){if(!yes)throw halo2::FormatError(why);}
    template<class F>static void reject(F &&fn,const char *why){bool rejected=false;
        try{fn();}catch(const halo2::FormatError &){rejected=true;}require(rejected,why);}
    template<class F>void each(F &&fn){
        for(auto &[key,hud]:primary)fn(*hud);for(auto &[key,hud]:offhand)fn(*hud);
        for(auto &[key,hud]:vehicle)fn(*hud);for(auto &[key,hud]:unit)fn(*hud);
        for(auto &[key,hud]:parent)fn(*hud);
    }
    template<class F>void registries(F &&fn){fn(primary);fn(offhand);fn(vehicle);fn(unit);fn(parent);}
    Group::Targets targets(){return {primary,offhand,vehicle,unit,parent,scoreboard};}
    explicit NativeHudGroupFixture(halo2::MultiplayerPreparedWorld &w):world(w){
        const auto &cache=*w.cache;const auto fonts=halo2::gameplay_font_directory(cache);
        const auto representations=halo2::source_session_player_representations(w.content,w.match.source_game_type());
        require(!representations.empty(),"Native HUD group has no actual player representation");
        const auto make=[&](halo2::HudContent content,unsigned hand=0){return std::make_unique<Hud>(std::move(content),fonts,mesh,4,
            w.rules.name,cache.original_data()->hud_bitmap_geometry_constants,std::filesystem::path{},hand);};
        for(unsigned representation:representations){
            const auto biped=halo2::multiplayer_biped(w.content,representation).datum;
            auto authored=cache.load_hud(biped,halo2::null_datum),empty=authored;
            std::erase_if(authored.widgets,[](const auto &widget){return widget.weapon_type==1;});
            unit.emplace(representation,make(std::move(authored)));
            std::erase_if(empty.widgets,[](const auto &widget){return widget.weapon_type!=1;});empty.action_messages.enabled=false;
            primary.emplace(std::pair{representation,halo2::null_datum},make(std::move(empty)));
            for(const auto &binding:halo2::handheld_weapon_hud_bindings(cache,representation)){
                auto weapon=cache.load_hud(biped,binding.definition);
                std::erase_if(weapon.widgets,[](const auto &widget){return widget.weapon_type!=1;});weapon.action_messages.enabled=false;
                primary.emplace(std::pair{representation,binding.definition},make(weapon));
                if(binding.dual_wieldable)offhand.emplace(std::pair{representation,binding.definition},make(std::move(weapon),1));
            }
        }
        std::set<std::uint32_t> vehicle_definitions,parent_definitions;
        if(w.content.unit_interactions)for(const auto &[id,definition]:w.content.unit_interactions->units){
            const auto tag=cache.query_tag({.handle=halo2::TagHandle{id},.require_address=true});
            require(tag&&tag->size>=0x1c8,"Native HUD group lost an actual resident unit definition");
            const auto metadata=cache.load_source_tag_metadata(id);
            const halo2::Bytes descriptor(metadata.bytes_at(tag->address+0x1c0u,8));const auto count=descriptor.u32(0);
            require(count<=4096,"Native HUD group unit weapon list exceeds authored bounds");
            if(count){const halo2::Bytes rows(metadata.bytes_at(descriptor.u32(4),count*8u));
                for(unsigned n=0;n<count;++n)if(const auto weapon=rows.u32(n*8u+4u);weapon!=halo2::null_datum)vehicle_definitions.insert(weapon);}
            for(auto hud:cache.load_unit_seat_huds(id))parent_definitions.insert(hud);
        }
        for(unsigned representation:representations){const auto biped=halo2::multiplayer_biped(w.content,representation).datum;
            for(auto id:vehicle_definitions){auto authored=cache.load_hud(biped,id);
                std::erase_if(authored.widgets,[](const auto &widget){return widget.weapon_type!=1;});
                for(auto &widget:authored.widgets)widget.weapon_type=4;authored.action_messages.enabled=false;
                vehicle.emplace(std::pair{representation,id},make(std::move(authored),2));}
            for(auto id:parent_definitions)parent.emplace(std::pair{representation,id},make(cache.load_hud_definition(biped,id)));
        }
        halo2::Cache menu(cache.shared_directory()/"mainmenu.map",cache.shared_directory(),cache.original_data());
        const auto frontend=menu.load_frontend();const auto variant=std::find_if(frontend.variants.begin(),frontend.variants.end(),[](const auto &v){
            return v.engine==0&&halo2::source_game_variant_get(halo2::source_game_variant(v),91)==1;
        });require(variant!=frontend.variants.end(),"Native HUD group lacks the actual authored team Slayer variant");
        scoreboard=std::make_unique<Board>(cache,fonts,mesh,*variant);require(scoreboard->available(),"Native HUD group lacks the actual scoreboard painter/font");
        const halo2::PcKeyboardSettings keys;const halo2::ControllerSettings pad;
        each([&](Hud &hud){for(unsigned view=0;view<4;++view){hud.prompt_gamepad(view,view&1);
            hud.prompt_gamepad_buttons(view,halo2::native_action_prompt_controller_buttons(pad));
            hud.prompt_keyboard_buttons(view,halo2::native_action_prompt_keyboard_buttons(keys));hud.prompt_boarding_buttons(view,keys,pad);}});
        std::vector<unsigned> radar;each([&](Hud &hud){for(unsigned view=0;view<4;++view)if(const auto slot=hud.motion_sensor_material(view))radar.push_back(*slot);});
        std::sort(radar.begin(),radar.end());radar.erase(std::unique(radar.begin(),radar.end()),radar.end());
        materials=std::make_unique<halo2::MaterialPresentationContinuity>(mesh,radar);
        continuity=std::make_unique<Group>(targets());material_count=mesh.materials.size();material_roots=mesh.materials.data();
    }
    void populate(const halo2::MultiplayerSnapshotOrigin &origin,bool first){
        const auto &state=origin.accepted_state();const auto slots=origin.local_slots();
        require(slots.size()==4,"Native HUD group fixture requires the genuine four-seat receipt");
        continuity->mutating();materials->mutating();
        each([&](Hud &hud){hud.voice_text(first?"Earlier speaker":"Later speaker");
            for(unsigned view=0;view<slots.size();++view){hud.prompt_gamepad(view,first?bool(view&1):!bool(view&1));
                hud.update_motion_sensor(state,slots[view],view);hud.animate(state,slots[view],view,25,14400,double(state.tick)*halo2::tick_seconds);}});
        for(unsigned n=0;n<8;++n)for(unsigned view=0;view<slots.size();++view)scoreboard->step(view,(view&1)==unsigned(!first),state,slots[view]);
    }
    void capture(const halo2::MultiplayerSnapshotOrigin &origin){
        populate(origin,true);const auto &state=origin.accepted_state();const auto slots=origin.local_slots();
        each([&](Hud &hud){for(unsigned view=0;view<slots.size();++view)
            early_draws.push_back({&hud,view,hud.draw(state,slots[view],view,4,1280,720,25,14400)});});
        for(unsigned view=0;view<slots.size();++view)early_boards.push_back(scoreboard->draw(state,slots,view,1280,720));
        require(std::any_of(early_draws.begin(),early_draws.end(),[](const auto &d){return !d.vertices.empty();}),"Native HUD group has no real authored draw stream");
        require(!early_boards[0].paint.draw.vertices.empty(),"Native HUD group never drew a real held scoreboard");
        early=continuity->capture(origin);early_materials=materials->checkpoint();
        const auto stats=early.statistics();require(early.matches(origin)&&continuity->owns_snapshot(early)&&stats&&stats->scoreboard&&
            stats->huds==std::array{primary.size(),offhand.size(),vehicle.size(),unit.size(),parent.size()},"Native HUD group lost its complete actual registry census");
        reject([&]{(void)continuity->capture(halo2::MultiplayerSnapshotOrigin{});},"HUD group fabricated an admitted receipt");
    }
    static auto fields(const halo2::Vertex &v){return std::array{v.position.x,v.position.y,v.position.z,v.color.x,v.color.y,v.color.z,v.uv.x,v.uv.y,
        v.lightmap_uv.x,v.lightmap_uv.y,v.normal.x,v.normal.y,v.normal.z,v.tangent.x,v.tangent.y,v.tangent.z,v.binormal.x,v.binormal.y,v.binormal.z,
        v.incident.x,v.incident.y,v.incident.z,v.prt_cluster,v.prt_weights[0],v.prt_weights[1],v.prt_weights[2],v.prt_weights[3],
        v.source_local_position.x,v.source_local_position.y,v.source_local_position.z,v.source_color_alpha};}
    static bool same_vertices(std::span<const halo2::Vertex> a,std::span<const halo2::Vertex> b){
        return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),[](const auto &x,const auto &y){const auto p=fields(x),q=fields(y);
            for(unsigned n=0;n<p.size();++n)if(std::bit_cast<std::uint32_t>(p[n])!=std::bit_cast<std::uint32_t>(q[n]))return false;
            return std::tie(x.material,x.lightmap,x.bsp_cluster,x.source_section,x.source_part,x.source_stipple_level,x.source_part_type,x.source_part_flags)==
                std::tie(y.material,y.lightmap,y.bsp_cluster,y.source_section,y.source_part,y.source_stipple_level,y.source_part_type,y.source_part_flags);});
    }
    void restore(const halo2::MultiplayerSnapshotOrigin &origin,const halo2::MultiplayerSnapshotProof &proof,
        const halo2::MultiplayerSnapshotProof &alternate,const halo2::MultiplayerSnapshotOrigin &later,const halo2::MultiplayerSnapshotProof &later_proof){
        require(later.tick()>origin.tick(),"HUD group lacks a genuinely later source receipt");populate(later,false);
        const auto native_tick=world.match.simulation().ticks();const auto native_mutations=world.source_objects->execution_mutations();
        const auto unchanged=[&]{return world.match.simulation().ticks()==native_tick&&world.source_objects->execution_mutations()==native_mutations&&
            mesh.materials.size()==material_count&&mesh.materials.data()==material_roots;};
        {auto cancelled=continuity->prepare_restore(early,proof);require(continuity->ready(cancelled,proof),"HUD group exact historical receipt did not prepare");}
        require(unchanged(),"HUD group cancellation changed native world or resources");
        reject([&]{(void)continuity->prepare_restore(early,halo2::MultiplayerSnapshotProof{});},"HUD group admitted a missing native proof");
        reject([&]{(void)continuity->prepare_restore(early,later_proof);},"HUD group admitted reordered real receipt/proof");
        Group foreign(targets());require(!foreign.owns_snapshot(early),"HUD group crossed aggregate ownership");
        reject([&]{(void)foreign.prepare_restore(early,proof);},"Foreign HUD group admitted another group's snapshot");
        auto stale=continuity->prepare_restore(early,proof);continuity->mutating();
        require(!continuity->ready(stale,proof)&&!continuity->commit(stale),"HUD group mutation kept a stale token eligible");
        // Every populated role is independently checked, including currently
        // dormant authored weapon/seat owners. Empty roles remain truthful.
        registries([&](auto &registry){if(registry.empty())return;auto prepared=continuity->prepare_restore(early,proof);
            registry.begin()->second->voice_text("Later speaker");
            require(!continuity->ready(prepared,proof)&&!continuity->commit(prepared),"HUD group skipped stale dormant child readiness");});
        {auto prepared=continuity->prepare_restore(early,proof);scoreboard->step(1,false);
            require(!continuity->ready(prepared,proof)&&!continuity->commit(prepared),"HUD group skipped stale scoreboard readiness");}
        // Test each of the five table boundaries even if its authored census
        // is empty: an extra alias is a rejection input, never a valid role.
        auto *borrowed=primary.begin()->second.get();
        registries([&](auto &registry){auto prepared=continuity->prepare_restore(early,proof);
            using Key=typename std::remove_reference_t<decltype(registry)>::key_type;
            const auto key=[]()->Key{if constexpr(std::is_same_v<Key,unsigned>)return 1000u;else return {1000u,halo2::null_datum};}();
            auto [at,inserted]=registry.emplace(key,nullptr);require(inserted,"HUD group extra-key control collided with authored census");
            at->second.reset(borrowed);
            const bool rejected=!continuity->owns_snapshot(early)&&!continuity->ready(prepared,proof)&&!continuity->commit(prepared);
            at->second.release();registry.erase(at);require(rejected&&continuity->ready(prepared,proof),"HUD group failed complete five-table census preflight");
        });
        // Actual key removal, changed key, null owner and replaced object all
        // leave the original resident HUD alive and are repaired before use.
        registries([&](auto &registry){if(registry.empty())return;auto prepared=continuity->prepare_restore(early,proof);
            auto node=registry.extract(registry.begin());const bool missing=!continuity->ready(prepared,proof);
            registry.insert(std::move(node));require(missing&&continuity->ready(prepared,proof),"HUD group accepted a missing key");
            node=registry.extract(registry.begin());const auto old_key=node.key();
            if constexpr(std::is_same_v<typename std::remove_reference_t<decltype(registry)>::key_type,unsigned>)node.key()+=1000u;else node.key().first+=1000u;
            const auto changed_key=node.key();registry.insert(std::move(node));const bool renamed=!continuity->ready(prepared,proof);
            node=registry.extract(changed_key);node.key()=old_key;registry.insert(std::move(node));require(renamed&&continuity->ready(prepared,proof),"HUD group relabeled a resident owner under a new key");
            auto owner=std::move(registry.begin()->second);const bool null_owner=!continuity->ready(prepared,proof);
            registry.begin()->second=std::move(owner);require(null_owner&&continuity->ready(prepared,proof),"HUD group admitted a null resident child");
        });
        // Optional presence is fixed by the referenced unique_ptr variable.
        std::unique_ptr<Board> absent_board;Group absent({primary,offhand,vehicle,unit,parent,absent_board});
        const auto absent_snapshot=absent.capture(later);require(!absent_snapshot.statistics()->scoreboard,"Absent scoreboard census invented an owner");
        auto absent_token=absent.prepare_restore(absent_snapshot,later_proof);auto present_token=continuity->prepare_restore(early,proof);
        absent_board=std::move(scoreboard);const bool changed_presence=!absent.ready(absent_token,later_proof)&&!absent.commit(absent_token)&&
            !continuity->ready(present_token,proof)&&!continuity->commit(present_token);
        scoreboard=std::move(absent_board);require(changed_presence&&absent.ready(absent_token,later_proof)&&continuity->ready(present_token,proof),"HUD group ignored optional scoreboard presence");
        require(absent.commit(absent_token)&&!absent.commit(absent_token),"Absent-scoreboard HUD group did not publish exactly once");
        // Rejected constructor inputs do not borrow aliased/null ownership.
        Group::DefinitionRegistry invalid;invalid.emplace(std::pair{1000u,halo2::null_datum},nullptr);
        reject([&]{Group nulls({invalid,offhand,vehicle,unit,parent,scoreboard});},"HUD group constructor admitted a null owner");
        invalid.begin()->second.reset(borrowed);bool alias_rejected=false;
        try{Group aliases({primary,invalid,vehicle,unit,parent,scoreboard});}catch(const halo2::FormatError &){alias_rejected=true;}
        catch(...){invalid.begin()->second.release();throw;}
        invalid.begin()->second.release();require(alias_rejected,"HUD group constructor admitted aliased child identities");
        auto prepared=continuity->prepare_restore(early,proof),competing=continuity->prepare_restore(early,proof);
        require(!continuity->ready(prepared,alternate)&&!continuity->ready(prepared,later_proof),"HUD group readiness ignored full peer/context/seal identity");
        bool foreign_ok=false;std::thread other([&]{foreign_ok=!continuity->owns_snapshot(early)&&!continuity->ready(prepared,proof)&&!continuity->commit(prepared);});other.join();
        require(foreign_ok&&continuity->ready(prepared,proof)&&unchanged(),"HUD group crossed executor or mutated native source state");
        auto material_restore=materials->prepare_restore(early_materials);
        require(materials->restore_ready(material_restore)&&continuity->ready(prepared,proof),"HUD/material CPU composition did not preflight before mutation");
        require(materials->commit_restore(material_restore)&&continuity->commit(prepared)&&!continuity->commit(prepared)&&!continuity->commit(competing),"HUD group publication was partial or reusable");
        const auto &state=origin.accepted_state();const auto slots=origin.local_slots();
        for(const auto &draw:early_draws)require(same_vertices(draw.vertices,draw.owner->draw(state,slots[draw.view],draw.view,4,1280,720,25,14400)),
            "Proof-bound grouped HUD restore reconstructed different authored CPU vertices");
        for(unsigned view=0;view<slots.size();++view){const auto result=scoreboard->draw(state,slots,view,1280,720);const auto &old=early_boards[view];
            require(result.normal_hud==old.normal_hud&&result.layout.rows.size()==old.layout.rows.size()&&result.layout.team_rows.size()==old.layout.team_rows.size()&&
                std::bit_cast<std::uint32_t>(result.layout.input_alpha)==std::bit_cast<std::uint32_t>(old.layout.input_alpha)&&
                same_vertices(result.paint.draw.vertices,old.paint.draw.vertices),"Proof-bound grouped scoreboard restore differs from exact historical paint");}
        require(unchanged(),"Grouped HUD/material restore changed native simulation or resident roots");
        // Child lifetime identity, rather than address equality, is part of
        // the fixed census. Reuse one actual HUD's allocation deliberately.
        lifetime_control(origin,proof);
        const auto stats=early.statistics();std::cout<<"PASS proof-bound complete HUD registry restore: primary="<<stats->huds[0]<<" offhand="<<stats->huds[1]
            <<" vehicle="<<stats->huds[2]<<" unit="<<stats->huds[3]<<" parent="<<stats->huds[4]
            <<"; genuine two-tick/four-seat receipt, exact native proof, dormant children, five-table census, optional scoreboard, cancellation/stale/foreign/full-seal/once controls\n";
        std::cout<<"LIMIT authored roles absent from this cache remain absent; CPU component/material composition only, no GPU/rendered-role/device/audio/native activation or full-ready claim\n";
    }
    void lifetime_control(const halo2::MultiplayerSnapshotOrigin &origin,const halo2::MultiplayerSnapshotProof &proof){
        const auto &cache=*world.cache;const auto representation=unit.begin()->first;
        const auto content=cache.load_hud(halo2::multiplayer_biped(world.content,representation).datum,halo2::null_datum);
        halo2::Mesh first_mesh,second_mesh;Group::DefinitionRegistry p,o,v,parents;Group::UnitRegistry units;std::unique_ptr<Board> board;
        const auto construct=[&](void *at,halo2::Mesh &resident){return ::new(at) Hud(content,halo2::gameplay_font_directory(cache),resident,4,
            world.rules.name,cache.original_data()->hud_bitmap_geometry_constants);};
        void *storage=::operator new(sizeof(Hud));Hud *first{};
        try{first=construct(storage,first_mesh);}catch(...){::operator delete(storage);throw;}
        units.emplace(representation,std::unique_ptr<Hud>(first));
        Group fixed({p,o,v,units,parents,board});const auto snapshot=fixed.capture(origin);auto prepared=fixed.prepare_restore(snapshot,proof);
        auto *retired=units.begin()->second.release();retired->~MultiplayerHud();Hud *replacement{};
        try{replacement=construct(storage,second_mesh);}catch(...){::operator delete(storage);throw;}
        units.begin()->second.reset(replacement);
        require(replacement==first&&!fixed.owns_snapshot(snapshot)&&!fixed.ready(prepared,proof)&&!fixed.commit(prepared),"HUD group admitted a replacement lifetime at the original address");
        reject([&]{(void)fixed.prepare_restore(snapshot,proof);},"HUD group relabeled an old weak child lifetime");
        Group::Snapshot expired;std::optional<Group::PreparedRestore> retired_group;
        {Group local({p,o,v,units,parents,board});expired=local.capture(origin);retired_group.emplace(local.prepare_restore(expired,proof));}
        require(!expired.valid(),"Prepared token pinned a destroyed group identity");
    }
};
