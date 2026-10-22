#include "halo2/action_prompt_bindings.hpp"
#include "halo2/hud.hpp"
#include "halo2/material_presentation_continuity.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <new>
#include <optional>
#include <thread>
#include <tuple>
#include <utility>

// Observe allocations made by real draw/preparation calls. The one-shot hook
// exercises reentry while the actual owner is busy, without a test-only owner
// API. The watched voice buffer also proves deferred consumed-token retirement.
namespace allocation_watch {
thread_local void (*hook)()=nullptr;
thread_local const void *watched{};
thread_local bool freed{},counting{};
thread_local std::size_t allocations{},deletions{};
void *allocate(std::size_t bytes) {
    auto *p=std::malloc(bytes?bytes:1);if(!p)throw std::bad_alloc();
    if(counting)++allocations;
    if(auto callback=std::exchange(hook,nullptr))try{callback();}catch(...){std::free(p);throw;}
    return p;
}
void release(void *p)noexcept {
    if(p&&counting)++deletions;if(p&&p==watched)freed=true;std::free(p);
}
}
void *operator new(std::size_t n){return allocation_watch::allocate(n);}
void *operator new[](std::size_t n){return allocation_watch::allocate(n);}
void operator delete(void *p)noexcept{allocation_watch::release(p);}
void operator delete[](void *p)noexcept{allocation_watch::release(p);}
void operator delete(void *p,std::size_t)noexcept{allocation_watch::release(p);}
void operator delete[](void *p,std::size_t)noexcept{allocation_watch::release(p);}

using namespace halo2;
namespace {
unsigned checks{},draws{},cases{},guard_controls{};
void check(bool value,const char *why){++checks;if(!value)throw FormatError(why);}
template<class F>void rejects(F &&call,const char *why){
    bool rejected=false;try{call();}catch(const FormatError &){rejected=true;}check(rejected,why);++guard_controls;
}
auto values(const Vertex &v){return std::array{
    v.position.x,v.position.y,v.position.z,v.color.x,v.color.y,v.color.z,v.uv.x,v.uv.y,
    v.lightmap_uv.x,v.lightmap_uv.y,v.normal.x,v.normal.y,v.normal.z,
    v.tangent.x,v.tangent.y,v.tangent.z,v.binormal.x,v.binormal.y,v.binormal.z,
    v.incident.x,v.incident.y,v.incident.z,v.prt_cluster,
    v.prt_weights[0],v.prt_weights[1],v.prt_weights[2],v.prt_weights[3],
    v.source_local_position.x,v.source_local_position.y,v.source_local_position.z,v.source_color_alpha};}
template<std::size_t N>bool same_bits(const std::array<float,N> &a,const std::array<float,N> &b){
    for(unsigned n=0;n<N;++n)if(std::bit_cast<std::uint32_t>(a[n])!=std::bit_cast<std::uint32_t>(b[n]))return false;return true;
}
bool same_vertices(std::span<const Vertex> a,std::span<const Vertex> b){
    return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),[](const auto &x,const auto &y){
        return same_bits(values(x),values(y))&&std::tie(x.material,x.lightmap,x.bsp_cluster,x.source_section,x.source_part,
            x.source_stipple_level,x.source_part_type,x.source_part_flags)==
            std::tie(y.material,y.lightmap,y.bsp_cluster,y.source_section,y.source_part,
            y.source_stipple_level,y.source_part_type,y.source_part_flags);
    });
}
bool same_programs(const Mesh &a,const Mesh &b){
    if(a.materials.size()!=b.materials.size())return false;
    for(unsigned n=0;n<a.materials.size();++n){
        const auto &x=a.materials[n],&y=b.materials[n];
        if(x.forward_program.words!=y.forward_program.words||x.program_constants!=y.program_constants||
            x.forward_transforms!=y.forward_transforms||x.forward_diffuse!=y.forward_diffuse||
            x.forward_specular!=y.forward_specular||x.texture_frames!=y.texture_frames||
            x.diffuse.pixels!=y.diffuse.pixels)return false;
    }
    return true;
}
auto rectangle(const FullScoreboardRect &r){return std::tuple{r.top,r.left,r.bottom,r.right};}
bool same_widget(const FullScoreboardWidget &a,const FullScoreboardWidget &b){
    return a.valid==b.valid&&a.right_justified==b.right_justified&&rectangle(a.rectangle)==rectangle(b.rectangle)&&
        rectangle(a.text_rectangle)==rectangle(b.text_rectangle)&&same_bits(a.background,b.background)&&
        same_bits(a.foreground,b.foreground)&&a.text==b.text;
}
template<std::size_t N>bool same_widgets(const std::array<FullScoreboardWidget,N> &a,const std::array<FullScoreboardWidget,N> &b){
    return std::equal(a.begin(),a.end(),b.begin(),same_widget);
}
bool same_board(const MultiplayerScoreboard::Draw &a,const MultiplayerScoreboard::Draw &b){
    const auto &x=a.layout,&y=b.layout;
    if(a.normal_hud!=b.normal_hud||a.paint.status!=b.paint.status||a.paint.emblems_unavailable!=b.paint.emblems_unavailable||
        a.paint.connection_quality_unavailable!=b.paint.connection_quality_unavailable||a.paint.pulse_unavailable!=b.paint.pulse_unavailable||
        a.paint.text_controls_unavailable!=b.paint.text_controls_unavailable||!same_vertices(a.paint.draw.vertices,b.paint.draw.vertices)||
        a.paint.draw.rendering_gaps!=b.paint.draw.rendering_gaps||a.paint.draw.full_surface_bounds!=b.paint.draw.full_surface_bounds||
        x.status!=y.status||x.columns!=y.columns||std::bit_cast<std::uint32_t>(x.scroll)!=std::bit_cast<std::uint32_t>(y.scroll)||
        std::bit_cast<std::uint32_t>(x.input_alpha)!=std::bit_cast<std::uint32_t>(y.input_alpha)||!same_widget(x.title,y.title)||
        !same_widgets(x.headers,y.headers)||x.rows.size()!=y.rows.size()||x.team_rows.size()!=y.team_rows.size())return false;
    for(unsigned n=0;n<x.rows.size();++n){const auto &p=x.rows[n],&q=y.rows[n];
        if(std::tie(p.datum,p.encoded_rank,p.current_player,p.icon_kind)!=std::tie(q.datum,q.encoded_rank,q.current_player,q.icon_kind)||
            !same_widgets(p.widgets,q.widgets))return false;
    }
    for(unsigned n=0;n<x.team_rows.size();++n){const auto &p=x.team_rows[n],&q=y.team_rows[n];
        if(std::tie(p.team,p.encoded_rank)!=std::tie(q.team,q.encoded_rank)||!same_widgets(p.widgets,q.widgets))return false;
    }
    return true;
}
void bindings(MultiplayerHud &hud,const PcKeyboardSettings &keys,const ControllerSettings &pad){
    for(unsigned view=0;view<4;++view){
        hud.prompt_gamepad(view,view&1);hud.prompt_gamepad_buttons(view,{0xe103,0xe101});
        hud.prompt_keyboard_buttons(view,{"F","Mouse 4"});hud.prompt_boarding_buttons(view,keys,pad);
    }
}
void observe(MultiplayerHud &hud,MultiplayerScoreboard &board,MaterialPresentationContinuity &materials,
    const MultiplayerState &state,bool future){
    materials.mutating();
    for(unsigned view=0;view<4;++view){
        hud.update_motion_sensor(state,view,view);hud.animate(state,view,view,25,14400,double(state.tick)*tick_seconds);
        board.step(view,(view&1)==unsigned(future),state,view);
    }
}
const std::array<unsigned,4> ids{0,1,2,3};
void compare_draws(MultiplayerHud &used,MultiplayerHud &reference,MultiplayerScoreboard &board,
    MultiplayerScoreboard &reference_board,MaterialPresentationContinuity &materials,MaterialPresentationContinuity &reference_materials,
    const MultiplayerState &state){
    materials.mutating();reference_materials.mutating();
    for(unsigned width:{640u,1280u})for(unsigned players=1;players<=4;++players)for(unsigned view=0;view<players;++view){
        const auto height=width==640?480:720;
        const auto a=used.draw(state,view,view,players,width,height,25,14400);
        const auto b=reference.draw(state,view,view,players,width,height,25,14400);
        check(!a.empty()&&same_vertices(a,b),"Restored HUD differs from independently advanced complete vertex stream");
        const auto p=board.draw(state,std::span(ids).first(players),view,width,height);
        const auto q=reference_board.draw(state,std::span(ids).first(players),view,width,height);
        check(same_board(p,q),"Restored scoreboard differs from independently advanced layout/paint stream");
        if(!(view&1))check(!p.paint.draw.vertices.empty(),"Authored held scoreboard never produced a real paint stream");
        ++draws;
    }
    for(unsigned view=0;view<4;++view)check(!used.action_text(state,view,view).empty()&&
        used.action_text(state,view,view)==reference.action_text(state,view,view),"Restored action mode/labels differ");
}
MultiplayerHud *busy_hud{};MultiplayerScoreboard *busy_board{};
MultiplayerHud::PreparedRestore *busy_hud_token{};MultiplayerScoreboard::PreparedRestore *busy_board_token{};
bool hook_hit{},hook_rejected{},hook_ready{},hook_committed{};
void hud_reentry(){
    hook_hit=true;try{(void)busy_hud->checkpoint();}catch(const FormatError &){hook_rejected=true;}
    hook_ready=busy_hud->restore_ready(*busy_hud_token);hook_committed=busy_hud->commit_restore(*busy_hud_token);
}
void board_reentry(){
    hook_hit=true;try{(void)busy_board->checkpoint();}catch(const FormatError &){hook_rejected=true;}
    hook_ready=busy_board->restore_ready(*busy_board_token);hook_committed=busy_board->commit_restore(*busy_board_token);
}
void reset_hook(){hook_hit=hook_rejected=hook_ready=hook_committed=false;}
}
int main(int argc,char **argv)try{
    check(argc==3||argc==4,"Supply TU1.5 XBE, original maps and optional imported MCC artwork");
    const auto maps=std::filesystem::path(argv[2]);const auto artwork=argc==4?std::filesystem::path(argv[3]):std::filesystem::path{};
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(maps/"lockout.map")||
        !std::filesystem::is_regular_file(maps/"mainmenu.map")||!std::filesystem::is_directory(maps/"fonts")||
        (!artwork.empty()&&!std::filesystem::is_regular_file(artwork/"hud.tsv")))return 77;
    const auto original=OriginalData::load_xbe(argv[1]);check(original->image==OriginalData::Image::TitleUpdate15,"HUD continuation requires TU1.5");
    Cache cache(maps/"lockout.map",maps,original),menu(maps/"mainmenu.map",maps,original);
    const auto level=cache.load_content(0,true);const auto frontend=menu.load_frontend();
    const auto variant=std::find_if(frontend.variants.begin(),frontend.variants.end(),[](const auto &v){
        return v.engine==0&&source_game_variant_get(source_game_variant(v),91)==1;
    });
    check(variant!=frontend.variants.end(),"Actual authored team Slayer scoreboard variant is absent");
    const PcKeyboardSettings keys;const ControllerSettings pad;
    for(unsigned representation:{2u,3u})for(bool teams:{false,true}){
        const auto authored=cache.load_hud(level.player_representations.at(representation).biped.datum,cache.slayer_starting_weapon());
        check(std::any_of(authored.widgets.begin(),authored.widgets.end(),[](const auto &w){return w.special==5;}),"Actual shield-history widget absent");
        const auto prompt=std::find_if(authored.action_messages.weapons.begin(),authored.action_messages.weapons.end(),
            [&](const auto &p){return p.second[0]&&authored.strings.contains(p.second[0]);});
        check(prompt!=authored.action_messages.weapons.end(),"Actual pickup prompt string absent");
        Mesh mesh,reference_mesh;
        MultiplayerHud hud(authored,maps/"fonts",mesh,4,"Slayer",original->hud_bitmap_geometry_constants,artwork),
            reference(authored,maps/"fonts",reference_mesh,4,"Slayer",original->hud_bitmap_geometry_constants,artwork);
        MultiplayerScoreboard board(cache,maps/"fonts",mesh,*variant,artwork),reference_board(cache,maps/"fonts",reference_mesh,*variant,artwork);
        check(board.available()&&reference_board.available(),"Actual imported scoreboard/font/painter is unavailable");
        bindings(hud,keys,pad);bindings(reference,keys,pad);hud.voice_text("Old speaker");reference.voice_text("Old speaker");
        std::vector<unsigned> radar;for(unsigned view=0;view<4;++view){
            const auto slot=hud.motion_sensor_material(view);check(bool(slot),"Actual four-view radar owner is absent");radar.push_back(*slot);
        }
        MaterialPresentationContinuity materials(mesh,radar),reference_materials(reference_mesh,radar);
        const auto *roots=mesh.materials.data();const auto material_count=mesh.materials.size();
        MultiplayerState old;old.team_mode=teams;old.team_scores={10,4};old.retained_team_mask=3;old.score_limit=25;old.clock_seconds=120;
        for(unsigned view=0;view<4;++view){
            auto &p=old.players[view];p.connected=true;p.name="Player"+std::to_string(view);p.shield=p.health=1;p.loaded=60;p.reserve=120;
            p.source_player=0x70000000+view;p.source_unit=0x20000+view;p.source_weapon=0x30000+view;
            p.source_flags=1;p.source_membership=view&1;p.position={float(view)*2,0,0};p.velocity={2,0,0};p.score=10-int(view);
            std::array<std::uint8_t,9> appearance{};appearance[4]=representation==3;
            p.source_configuration=native_selected_player_configuration(p.name,appearance,int(view&1));
            p.action_prompt.definition=prompt->first;p.action_prompt.target.action=1;p.action_prompt.target.index=1;p.action_prompt.target.object=0x90000+view;
        }
        for(unsigned tick=90;tick<=120;++tick){old.tick=tick;
            for(unsigned view=0;view<4;++view){old.players[view].position.y=float(tick-90)*.08f;old.players[view].shield=tick>=119?.7f:1.f;}
            observe(hud,board,materials,old,false);observe(reference,reference_board,reference_materials,old,false);
        }
        compare_draws(hud,reference,board,reference_board,materials,reference_materials,old);
        check(same_programs(mesh,reference_mesh),"Independent old CPU shader/image witnesses differ");
        const auto old_hud=hud.checkpoint();const auto old_board=board.checkpoint();const auto old_materials=materials.checkpoint();
        check(old_hud.valid()&&old_board.valid()&&hud.owns_checkpoint(old_hud)&&board.owns_checkpoint(old_board),"Actual checkpoint lost owner binding");
        auto future=old;future.team_scores={1,20};future.clock_seconds=240;
        std::string future_voice="Future speaker "+std::string(128,'A');const auto *voice_storage=future_voice.data();hud.voice_text(std::move(future_voice));
        for(unsigned view=0;view<4;++view)hud.prompt_gamepad(view,!(view&1));
        for(unsigned tick=121;tick<=146;++tick){future.tick=tick;
            for(unsigned view=0;view<4;++view){auto &p=future.players[view];p.position={float(view)*2,float(tick-120),0};
                p.velocity={0,2,0};p.shield=.2f;p.score=int(view)+17;if(tick>=132)p.source_unit=0x50000+view;}
            observe(hud,board,materials,future,true);
        }
        materials.mutating();const auto future_draw=hud.draw(future,0,0,4,1280,720,25,14400);
        const auto future_board=board.draw(future,ids,1,1280,720);
        check(!same_vertices(future_draw,reference.draw(future,0,0,4,1280,720,25,14400))&&
            !same_programs(mesh,reference_mesh),"Future negative control did not exercise real HUD clocks/images");
        {auto cancel_hud=hud.prepare_restore(old_hud);auto cancel_board=board.prepare_restore(old_board);
            check(hud.restore_ready(cancel_hud)&&board.restore_ready(cancel_board),"Valid cancellation controls were not prepared");}
        check(same_vertices(future_draw,hud.draw(future,0,0,4,1280,720,25,14400))&&
            same_board(future_board,board.draw(future,ids,1,1280,720)),"Canceled preparation changed actual draw state");
        rejects([&]{(void)reference.prepare_restore(old_hud);},"Foreign HUD checkpoint admitted");
        rejects([&]{(void)reference_board.prepare_restore(old_board);},"Foreign scoreboard checkpoint admitted");
        {auto stale=hud.prepare_restore(old_hud);hud.prompt_gamepad(0,true);
            check(!hud.restore_ready(stale)&&!hud.commit_restore(stale),"HUD mutation did not retire a prepared revision");++guard_controls;}
        {auto stale=board.prepare_restore(old_board);board.step(1,false);
            check(!board.restore_ready(stale)&&!board.commit_restore(stale),"Scoreboard step did not retire a prepared revision");++guard_controls;}
        hud.prompt_keyboard_buttons(3,{"Different","Mouse 4"});
        rejects([&]{(void)hud.prepare_restore(old_hud);},"Changed live pickup key was silently replaced by historic label");
        hud.prompt_keyboard_buttons(3,{"F","Mouse 4"});
        hud.prompt_gamepad_buttons(2,{0xe104,0xe101});
        rejects([&]{(void)hud.prepare_restore(old_hud);},"Changed live pickup button was silently replaced");
        hud.prompt_gamepad_buttons(2,{0xe103,0xe101});
        auto changed_keys=keys;changed_keys.bindings[unsigned(PcAction::Melee)]=pc_mouse_button(5);
        hud.prompt_boarding_buttons(3,changed_keys,pad);
        rejects([&]{(void)hud.prepare_restore(old_hud);},"Changed fourth resolved keyboard binding was silently replaced");
        hud.prompt_boarding_buttons(3,keys,pad);
        auto changed_pad=pad;changed_pad.buttons=3;
        check(native_boarding_prompt_controller_buttons(changed_pad)!=native_boarding_prompt_controller_buttons(pad),"Controller binding control has no actual change");
        hud.prompt_boarding_buttons(2,keys,changed_pad);
        rejects([&]{(void)hud.prepare_restore(old_hud);},"Changed fourth resolved controller binding was silently replaced");
        hud.prompt_boarding_buttons(2,keys,pad);
        auto hud_token=hud.prepare_restore(old_hud);auto board_token=board.prepare_restore(old_board);
        reset_hook();busy_hud=&hud;busy_hud_token=&hud_token;allocation_watch::hook=hud_reentry;
        (void)hud.draw(future,0,0,4,1280,720,25,14400);allocation_watch::hook=nullptr;
        check(hook_hit&&hook_rejected&&!hook_ready&&!hook_committed&&hud.restore_ready(hud_token),"Real HUD draw reentry bypassed its busy barrier");++guard_controls;
        reset_hook();busy_board=&board;busy_board_token=&board_token;allocation_watch::hook=board_reentry;
        (void)board.draw(future,ids,1,1280,720);allocation_watch::hook=nullptr;
        check(hook_hit&&hook_rejected&&!hook_ready&&!hook_committed&&board.restore_ready(board_token),"Real scoreboard paint reentry bypassed its busy barrier");++guard_controls;
        bool foreign_ok=false;
        std::thread foreign([&]{bool hud_rejected=false,board_rejected=false;
            try{(void)hud.checkpoint();}catch(const FormatError &){hud_rejected=true;}
            try{(void)board.checkpoint();}catch(const FormatError &){board_rejected=true;}
            foreign_ok=hud_rejected&&board_rejected&&!old_hud.valid()&&!old_board.valid()&&!hud.owns_checkpoint(old_hud)&&
                !board.owns_checkpoint(old_board)&&!hud.restore_ready(hud_token)&&!board.restore_ready(board_token)&&
                !hud.commit_restore(hud_token)&&!board.commit_restore(board_token);
        });foreign.join();check(foreign_ok,"Foreign executor admitted checkpoint/preflight/publication");++guard_controls;
        std::optional<MultiplayerHud::PreparedRestore> deferred;deferred.emplace(std::move(hud_token));
        auto material_token=materials.prepare_restore(old_materials);
        check(materials.restore_ready(material_token)&&hud.restore_ready(*deferred)&&board.restore_ready(board_token),"Whole CPU preparation failed before publication");
        allocation_watch::watched=voice_storage;allocation_watch::freed=false;
        allocation_watch::allocations=allocation_watch::deletions=0;allocation_watch::counting=true;
        const bool preflight=materials.restore_ready(material_token)&&hud.restore_ready(*deferred)&&board.restore_ready(board_token);
        const bool published=preflight&&materials.commit_restore(material_token)&&hud.commit_restore(*deferred)&&board.commit_restore(board_token);
        allocation_watch::counting=false;
        check(published&&!allocation_watch::allocations&&!allocation_watch::deletions&&!allocation_watch::freed,
            "CPU publication allocated or retired displaced owner storage");
        check(!hud.restore_ready(*deferred)&&!hud.commit_restore(*deferred)&&!board.restore_ready(board_token)&&!board.commit_restore(board_token),
            "Consumed HUD/scoreboard token was reused");++guard_controls;
        check(mesh.materials.data()==roots&&mesh.materials.size()==material_count,"CPU restoration replaced resident Mesh roots");
        check(same_programs(mesh,reference_mesh),"Composed CPU material/radar restore differs before any update/reset");
        compare_draws(hud,reference,board,reference_board,materials,reference_materials,old);
        check(!allocation_watch::freed,"Displaced voice storage retired before consumed token release");deferred.reset();
        check(allocation_watch::freed,"Consumed token did not retire its actual displaced voice allocation");allocation_watch::watched=nullptr;
        // This first continuation reuses the old salted units after a future
        // salt change and preserves the old sensor trail's exact sample phase.
        auto next=old;++next.tick;for(unsigned view=0;view<4;++view)next.players[view].shield=.6f;
        observe(hud,board,materials,next,false);observe(reference,reference_board,reference_materials,next,false);
        check(same_programs(mesh,reference_mesh),"Next accepted-tick radar/shield continuation differs after historical restore");
        compare_draws(hud,reference,board,reference_board,materials,reference_materials,next);
        // Exercise the retained inactive counter through the actual finalized
        // lifecycle path, not by writing private state or constructing a reset.
        auto finalized=next;finalized.finished=finalized.results_finalized=true;
        for(unsigned n=0;n<29;++n)for(unsigned view=0;view<4;++view){board.step(view,false,finalized,view);reference_board.step(view,false,finalized,view);}
        auto finalized_cp=board.checkpoint();for(unsigned view=0;view<4;++view)board.step(view,false,next,view);
        auto finalized_restore=board.prepare_restore(finalized_cp);check(board.commit_restore(finalized_restore),"Inactive lifecycle state did not restore");
        for(unsigned view=0;view<4;++view){board.step(view,false,finalized,view);reference_board.step(view,false,finalized,view);
            check(same_board(board.draw(finalized,ids,view,1280,720),reference_board.draw(finalized,ids,view,1280,720)),
                "Restored inactive scoreboard counter continuation differs");}
        ++cases;
    }
    // Expiration and changed resident vector storage are separate real owner
    // controls. A new owner at a reused address cannot inherit weak identity.
    const auto authored=cache.load_hud(level.player_representations.at(2).biped.datum,cache.slayer_starting_weapon());
    MultiplayerHud::Checkpoint expired_hud;MultiplayerScoreboard::Checkpoint expired_board;
    {Mesh mesh;MultiplayerHud hud(authored,maps/"fonts",mesh,4,"Slayer",original->hud_bitmap_geometry_constants,artwork);
        MultiplayerScoreboard board(cache,maps/"fonts",mesh,*variant,artwork);
        expired_hud=hud.checkpoint();expired_board=board.checkpoint();auto hp=hud.prepare_restore(expired_hud);auto bp=board.prepare_restore(expired_board);
        mesh.materials.reserve(mesh.materials.capacity()+1);
        check(!hud.restore_ready(hp)&&!hud.commit_restore(hp)&&!board.restore_ready(bp)&&!board.commit_restore(bp),"Changed resident storage admitted prepared owners");
        rejects([&]{(void)hud.prepare_restore(expired_hud);},"HUD historical resources relabeled after vector relocation");
        rejects([&]{(void)board.prepare_restore(expired_board);},"Scoreboard historical painter roots relabeled after vector relocation");}
    check(!expired_hud.valid()&&!expired_board.valid(),"Destroyed HUD/scoreboard identity remained live");
    std::cout<<"PASS exact HUD/scoreboard CPU continuation: "<<cases<<" authored Spartan/Elite and FFA/team cases, "<<draws
        <<" complete 1-4 seat 4:3/widescreen streams, "<<guard_controls<<" guard controls and "<<checks<<" checks; MCC="<<!artwork.empty()<<'\n';
    std::cout<<"PASS historical HUD radar/shield/winning/voice/mode and scoreboard held/inactive continuation; unchanged resolved four-key/four-button admission; no-allocation publication and deferred voice retirement\n";
    std::cout<<"LIMIT declared snapshot component inputs and CPU outputs only; scoreboard scroll_axis is not supplied by this adapter; no GPU upload, physical device/audio handoff, live migration or final-ready claim\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
