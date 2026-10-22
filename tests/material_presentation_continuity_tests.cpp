#include "halo2/material_presentation_continuity.hpp"
#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/appearance.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/motion_sensor.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string_view>
#include <thread>
#include <type_traits>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F run){bool rejected{};try{run();}catch(const FormatError &){rejected=true;}
    check(rejected,"Invalid material presentation operation was accepted");}
template<class F>std::string rejection(F run){try{run();}catch(const FormatError &e){return e.what();}
    throw FormatError("Invalid material presentation operation produced no diagnostic");}
Material authored(const nlohmann::json &row){
    // Same original material/animation input as camouflage_material_tests.
    // Expected program words come from that retained independent fixture.
    Material m;m.forward_pass=true;m.forward_pass_index=0;m.passes.emplace_back();
    auto &pass=m.passes[0];pass.program.words=row.at("initial").get<std::array<std::uint32_t,60>>();
    m.shader_switches[0]=row.at("selected");
    if(row.at("fragmented").get<bool>()){
        pass.fragments.push_back({0,{std::uint16_t(8<<9)}});pass.program.fragment_offsets[0]=row.at("offset");
    }
    const auto remaps=[](const auto &rows){std::vector<ShaderRemap> out;
        for(const auto &r:rows){const auto word=r.template get<std::uint32_t>();
            out.push_back({std::uint16_t(word),std::uint8_t(word>>16),std::uint8_t(word>>24)});}return out;};
    pass.pixel_externs=remaps(row.at("externs"));
    auto &a=m.animation;a.mappings=remaps(row.at("animated"));
    a.lods.push_back({1,{std::uint16_t(1<<9)}});a.layers.push_back({std::uint16_t(1<<9)});a.passes.push_back({std::uint16_t(1<<9)});
    a.implementations.push_back({{},{std::uint16_t(a.mappings.size()<<9)},{}});
    const auto function=row.at("function").get<std::array<std::uint32_t,6>>();ShaderOverlay overlay;
    overlay.period=std::bit_cast<float>(function[0]);
    for(unsigned i=1;i<function.size();++i)for(unsigned b=0;b<4;++b)overlay.curve.push_back(std::byte(function[i]>>(b*8)));
    a.parameters.push_back({std::uint16_t(1<<9)});a.references.push_back({0,0});a.overlays.push_back(overlay);
    a.tables=std::make_shared<ShaderFunctionTables>();m.forward_program=pass.program;
    m.diffuse={1,1,11,TexturePixels{std::byte{7},std::byte{8},std::byte{9},std::byte{255}},{}};
    return m;
}
void one(const nlohmann::json &row){
    Mesh mesh;mesh.materials.resize(3);mesh.materials[0]=authored(row);mesh.materials[1]=mesh.materials[0];
    auto pass=std::make_shared<Material>(mesh.materials[0]);pass->forward_vertex=9;
    auto bank=std::make_shared<VertexProgramBank>();bank->constants.push_back({1,2,3,4});bank->prt_inputs=true;pass->vertex_bank=bank;
    auto leaf=std::make_shared<Material>(mesh.materials[0]);pass->planar_fog_pass=leaf;
    mesh.materials[0].environment_pass=pass;mesh.materials[1].environment_pass=pass;
    mesh.materials[0].planar_fog_pass=leaf;mesh.materials[0].world_overlays_before={pass,{},leaf};
    mesh.materials[0].world_overlays_after={leaf};mesh.materials[0].world_bloom_passes={pass};mesh.materials[0].world_planar_passes={leaf,pass,{}};
    const auto uses=pass.use_count();auto *const roots=mesh.materials.data();
    const auto *const image=std::as_const(mesh.materials[0]).diffuse.pixels.data();
    MaterialPresentationContinuity owner(mesh);
    check(pass.use_count()==uses,"Idle material owner changed the original child COW sharing count");
    const auto saved=owner.checkpoint();const auto repeated=owner.checkpoint();const auto stats=repeated.stats();
    check(saved.valid()&&owner.owns_checkpoint(saved)&&stats.roots==3&&stats.nodes==5&&stats.shared_value_bytes&&
        !stats.new_value_bytes&&stats.unique_bytes<256ull*1024*1024,"Material capture lost deduplication or exact graph accounting");
    {auto cancelled=owner.prepare_restore(saved);check(owner.restore_ready(cancelled)&&cancelled.target_graph().ready(),"Material cancellation preparation failed");}
    const float camouflage=std::bit_cast<float>(row.at("camouflage").get<std::uint32_t>());
    const float seconds=std::bit_cast<float>(row.at("seconds").get<std::uint32_t>());
    const auto original=mesh.materials[0].forward_program.words;
    {auto mutation=owner.begin_mutation();rejects([&]{(void)owner.checkpoint();});rejects([&]{(void)owner.begin_mutation();});
        bind_object_camouflage(mesh.materials[0],camouflage);
        check(mesh.materials[0].forward_program.words==row.at("before_animation").get<std::array<std::uint32_t,60>>(),
            "Real original camouflage producer no longer matches the independent fixture");
        check(animate_forward_material(mesh.materials[0],seconds,{}),"Real original animated material produced no changed value");
        check(mesh.materials[0].forward_program.words==row.at("after_animation").get<std::array<std::uint32_t,60>>(),
            "Real original animation producer no longer matches the independent fixture");
        apply_object_lighting(mesh.materials[1],{});
        apply_object_render_colors(mesh.materials[1],{{{.2f,.3f,.4f},{.4f,.5f,.6f},{.1f,.2f,.3f},{.6f,.7f,.8f}}});
        mesh.materials[2].texture_frames={1,2,3,4};mesh.materials[2].program_constants[1][7]={5,6,7,8};
    }
    check(mesh.materials[0].environment_pass!=pass&&mesh.materials[1].environment_pass!=pass,
        "Ordinary real producers did not detach their separately mutated child graph");
    const auto *future=mesh.materials[0].environment_pass.get();const std::weak_ptr<Material> retired=mesh.materials[0].environment_pass;
    {auto stale=owner.prepare_restore(saved);owner.mutating();
        check(!owner.restore_ready(stale)&&!owner.commit_restore(stale),"Announced later material mutation kept prepared eligibility");}
    MaterialPresentationContinuity other(mesh);rejects([&]{(void)other.prepare_restore(saved);});
    MaterialPresentationContinuity::TargetGraph graph;
    {
        auto prepared=owner.prepare_restore(saved);graph=prepared.target_graph();
        check(graph.ready()&&!graph.applied()&&graph.matches_environment(mesh,0,pass.get(),bank)&&
            !graph.matches_environment(mesh,0,future,bank),"Prepared target proof relabeled its historical child identity");
        bool capture_rejected{},ready{},applied{},commit{};std::thread foreign([&]{
            try{(void)owner.checkpoint();}catch(const FormatError &){capture_rejected=true;}
            ready=graph.ready();applied=graph.applied();commit=owner.commit_restore(prepared);
        });foreign.join();check(capture_rejected&&!ready&&!applied&&!commit&&graph.ready(),"Foreign executor used or consumed material preparation");
        const auto before=mesh.materials[2].program_constants[1][7];mesh.materials[2].program_constants[1][7][0]+=1;
        check(!graph.ready()&&!owner.commit_restore(prepared),"Unannounced observed mutable write escaped exact preflight");
        mesh.materials[2].program_constants[1][7]=before;
        check(owner.commit_restore(prepared)&&!graph.ready()&&graph.applied()&&!owner.commit_restore(prepared),
            "Prepared material graph did not publish once with an exact applied proof");
        check(mesh.materials.data()==roots&&std::as_const(mesh.materials[0]).diffuse.pixels.data()==image&&
            mesh.materials[0].environment_pass==pass&&mesh.materials[1].environment_pass==pass&&
            mesh.materials[0].planar_fog_pass==leaf&&mesh.materials[0].world_overlays_before[0]==pass&&
            mesh.materials[0].world_overlays_before[2]==leaf&&mesh.materials[0].world_bloom_passes[0]==pass&&
            mesh.materials[0].world_planar_passes[0]==leaf&&mesh.materials[0].world_planar_passes[1]==pass&&
            pass->planar_fog_pass==leaf&&pass->vertex_bank==bank&&mesh.materials[0].forward_program.words==original&&
            mesh.materials[2].texture_frames==std::array<std::uint32_t,4>{}&&!retired.expired(),
            "Material restore lost values, alias topology, root/image residency or retired graph ownership");
        owner.mutating();check(!graph.applied(),"Later material batch retained applied target eligibility");
    }
    graph={};
    check(retired.expired(),"Consumed material token kept the displaced graph after all caller retirement");
    {owner.mutating();auto resident=std::move(mesh.materials[0].diffuse);
        mesh.materials[0].diffuse={1,1,11,TexturePixels{std::byte{1},std::byte{2},std::byte{3},std::byte{255}},{}};
        rejects([&]{(void)owner.checkpoint();});mesh.materials[0].diffuse=std::move(resident);}
    const auto changed=owner.checkpoint(),again=owner.checkpoint();
    check(again.stats().shared_value_bytes&&!again.stats().new_value_bytes,"Repeated exact material capture discarded unchanged value reuse");
    std::cout<<"MEASURE material graph roots="<<stats.roots<<" nodes="<<stats.nodes<<" links="<<stats.links
        <<" logical_bytes="<<stats.logical_bytes<<" shared_value_bytes="<<stats.shared_value_bytes
        <<" new_value_bytes="<<stats.new_value_bytes<<" unique_bytes="<<stats.unique_bytes
        <<" resident_schema_bytes="<<stats.resident_schema_bytes<<'\n';
}
void controls(){
    Mesh mesh;mesh.materials.resize(1);MaterialPresentationContinuity::Checkpoint old;
    MaterialPresentationContinuity::TargetGraph proof;
    {MaterialPresentationContinuity owner(mesh);old=owner.checkpoint();
        {auto mutation=owner.begin_mutation();mesh.materials[0].forward_program.words[0]=17;}
        auto prep=owner.prepare_restore(old);proof=prep.target_graph();
        check(owner.commit_restore(prep)&&proof.applied(),"Lifetime control never applied an owning material cut");}
    check(!old.valid()&&!proof.ready()&&!proof.applied(),"Expired material owner retained usable raw graph pointers");
    MaterialPresentationContinuity::Limits small;small.checkpoint_bytes=1;
    MaterialPresentationContinuity bounded(mesh,small);rejects([&]{(void)bounded.checkpoint();});
    Mesh cycle;cycle.materials.resize(1);auto loop=std::make_shared<Material>();loop->environment_pass=loop;cycle.materials[0].environment_pass=loop;
    rejects([&]{MaterialPresentationContinuity invalid(cycle);});loop->environment_pass.reset();
    Mesh moved;moved.materials.resize(1);MaterialPresentationContinuity stationary(moved);const auto cp=stationary.checkpoint();
    moved.materials.reserve(8);rejects([&]{(void)stationary.prepare_restore(cp);});
}
void generated_radar(){
    // Exercise the same source history/render/composite producers as the HUD.
    // The tiny explicit artwork is a controlled input, not a retail GPU oracle.
    Texture white{1,1,11,TexturePixels{std::byte{255},std::byte{255},std::byte{255},std::byte{255}},{}};
    const MotionSensorArtwork art{white,white,white,white};MotionSensorHistory history{};
    std::array<std::optional<MotionSensorSample>,4> samples;
    const std::array<MotionSensorCandidate,1> candidates{{{{2,3,0},true,true,3,1}}};
    for(unsigned view=0;view<4;++view)samples[view]=source_motion_sensor_sample(true,{},0,25,candidates);
    source_motion_sensor_store_samples(history,samples);
    const auto image=[&](unsigned view,unsigned tick,Vec3 color){return source_motion_sensor_composite(
        source_motion_sensor_render_plan(history,view,0,{0,0},{0,0},tick,1.f/30.f),art,color,64);};
    Mesh mesh;mesh.materials.resize(5);std::array<TexturePixels,4> expected;
    for(unsigned view=0;view<4;++view){mesh.materials[view].diffuse=image(view,1,{.2f,.4f,.6f});
        expected[view]=std::as_const(mesh.materials[view]).diffuse.pixels;}
    mesh.materials[4].diffuse=white;
    const std::array<unsigned,4> generated{0,1,2,3};auto *const roots=mesh.materials.data();
    MaterialPresentationContinuity owner(mesh,generated);
    auto *const exposed=mesh.materials[0].diffuse.pixels.data();const auto first=exposed[0];
    const auto cut=owner.checkpoint();
    check(cut.stats().generated_image_bytes==4*64*64*4&&
        std::any_of(expected[0].cbegin(),expected[0].cend(),[](auto v){return v!=std::byte{};}),
        "Generated image control had no real nonempty source compositor payload");
    {
        auto observed=owner.prepare_restore(cut);exposed[0]=std::byte(std::to_integer<unsigned>(first)^1u);
        check(!owner.restore_ready(observed)&&!owner.commit_restore(observed),
            "Unannounced generated image byte write escaped observed preflight");
        exposed[0]=first;check(owner.restore_ready(observed),"Restored observed CPU image was not byte-exact");
    }
    {
        auto mutation=owner.begin_mutation();
        for(unsigned view=0;view<4;++view)mesh.materials[view].diffuse=image(view,2,{.8f,.1f,.3f});
    }
    check(mesh.materials[0].diffuse.pixels!=expected[0],"Real source compositor did not change the admitted output");
    const auto later=owner.checkpoint();
    {auto cancelled=owner.prepare_restore(cut);check(owner.restore_ready(cancelled),"Generated image cancellation failed");}
    check(mesh.materials[0].diffuse.pixels!=expected[0],"Cancellation replaced live generated image bytes");
    {
        auto prepared=owner.prepare_restore(cut);const auto proof=prepared.target_graph();
        // The displaced actual page stays owned by this token until retirement.
        const auto *const old_page=std::as_const(mesh.materials[0]).diffuse.pixels.data();const auto old_byte=old_page[0];
        check(owner.commit_restore(prepared)&&proof.applied()&&mesh.materials.data()==roots&&old_page[0]==old_byte,
            "Generated image commit changed root addresses or retired its replaced page inside publication");
        for(unsigned view=0;view<4;++view)check(mesh.materials[view].diffuse.pixels==expected[view],
            "Generated image restore failed an exact four-seat CPU output");
    }
    // A later cut remains exact and independently eligible after restoring old.
    {auto prepared=owner.prepare_restore(later);check(owner.commit_restore(prepared),"Later generated image could not be restored");}
    {
        owner.mutating();const auto width=mesh.materials[0].diffuse.width;mesh.materials[0].diffuse.width+=64;
        rejects([&]{(void)owner.checkpoint();});mesh.materials[0].diffuse.width=width;
        auto &page=mesh.materials[0].diffuse;const auto format=page.format;page.format^=1;
        rejects([&]{(void)owner.checkpoint();});page.format=format;
        ++page.layers;rejects([&]{(void)owner.checkpoint();});--page.layers;
        ++page.depth;rejects([&]{(void)owner.checkpoint();});--page.depth;
        page.mips.push_back({64,64,0,64*64*4,1});rejects([&]{(void)owner.checkpoint();});page.mips.clear();
        page.pixels.push_back(std::byte{});rejects([&]{(void)owner.checkpoint();});page.pixels.pop_back();
        auto resident=std::move(mesh.materials[4].diffuse);mesh.materials[4].diffuse=image(0,3,{1,1,1});
        rejects([&]{(void)owner.checkpoint();});mesh.materials[4].diffuse=std::move(resident);
    }
    Mesh strict;strict.materials.resize(1);strict.materials[0].diffuse=image(0,1,{.2f,.4f,.6f});
    MaterialPresentationContinuity unadmitted(strict);strict.materials[0].diffuse=image(0,2,{.8f,.1f,.3f});
    rejects([&]{(void)unadmitted.checkpoint();});
    const std::array<unsigned,2> duplicate{0,0};const std::array<unsigned,1> outside{5};
    rejects([&]{MaterialPresentationContinuity invalid(mesh,duplicate);});
    rejects([&]{MaterialPresentationContinuity invalid(mesh,outside);});
    MaterialPresentationContinuity::Limits budget;budget.checkpoint_bytes=cut.stats().unique_bytes-1;
    MaterialPresentationContinuity bounded(mesh,generated,budget);rejects([&]{(void)bounded.checkpoint();});
    std::cout<<"PASS generated radar CPU material outputs: actual source history/render/composite, four-seat exact pixels, exposed-write protection, cancellation, displaced-page retention, historical/later restore, strict unadmitted image and fixed shape controls; image_bytes="
        <<cut.stats().generated_image_bytes<<"; renderer upload/descriptor adoption remains separate\n";
}
void capture_diagnostics(){
    const auto contains=[](const std::string &s,std::initializer_list<const char *> fields){
        for(const auto *field:fields)check(s.find(field)!=std::string::npos,"Material capture diagnostic lost its exact failing boundary");
        check(s.size()<1024&&s.find("0x")==std::string::npos,"Material capture diagnostic is unbounded or exposes addresses");
    };
    Mesh mesh;mesh.materials.resize(2);mesh.materials[0].diffuse={1,1,11,
        TexturePixels{std::byte{1},std::byte{2},std::byte{3},std::byte{255}},{}};
    mesh.materials[1].passes.emplace_back();MaterialPresentationContinuity owner(mesh);
    auto original=std::move(mesh.materials[0].diffuse);mesh.materials[0].diffuse={1,1,11,
        TexturePixels{std::byte{4},std::byte{5},std::byte{6},std::byte{255}},{}};
    contains(rejection([&]{(void)owner.checkpoint();}),{"reason=resident_schema","root=0 ","node=0 ",
        "logical_bytes=0 ","schema_member=diffuse.pixels.identity","schema_offset=","field_offset="});
    mesh.materials[0].diffuse=std::move(original);++mesh.materials[1].passes[0].template_pass;
    contains(rejection([&]{(void)owner.checkpoint();}),{"reason=resident_schema","root=1 ",
        "schema_member=passes.template_pass","pass=0"});
    --mesh.materials[1].passes[0].template_pass;
    {auto mutation=owner.begin_mutation();contains(rejection([&]{(void)owner.checkpoint();}),
        {"reason=incomplete_mutation_batch","root=none","nodes=0 "});}
    MaterialPresentationContinuity::Limits tiny;tiny.checkpoint_bytes=1;MaterialPresentationContinuity bounded(mesh,tiny);
    contains(rejection([&]{(void)bounded.checkpoint();}),{"reason=checkpoint_budget","root=0 ","nodes=0 ",
        "logical_bytes=0 ","new_value_bytes=0 ","shared_value_bytes=0 ","requested_bytes=","checkpoint_cap=1 ","schema_member=none"});
    std::string foreign;std::thread other([&]{foreign=rejection([&]{(void)owner.checkpoint();});});other.join();
    contains(foreign,{"reason=owner_or_executor","root=none"});
    mesh.materials.reserve(16);contains(rejection([&]{(void)owner.checkpoint();}),{"reason=root_storage","root=none"});
    std::cout<<"PASS bounded material capture diagnostics: exact budget/schema/root/pass/owner/batch boundaries, first differing byte offsets, no resident addresses; policy and admission unchanged\n";
}
void compact_authored_fanout(const nlohmann::json &rows){
    check(rows.size()>=96,"Repeated authored material control needs96 original pass cases");
    auto prototype=authored(rows.at(0));prototype.passes.clear();
    for(unsigned n=0;n<96;++n){const auto material=authored(rows.at(n));prototype.passes.push_back(material.passes[0]);}
    auto child=std::make_shared<Material>(prototype);prototype.environment_pass=child;
    Mesh mesh;mesh.materials.assign(4096,prototype);auto *const roots=mesh.materials.data();
    auto first_bank=std::make_shared<VertexProgramBank>(),second_bank=std::make_shared<VertexProgramBank>();
    first_bank->constants.push_back({1,2,3,4});second_bank->constants=first_bank->constants;
    mesh.materials[6].vertex_bank=first_bank;mesh.materials[7].vertex_bank=second_bank;
    MaterialPresentationContinuity owner(mesh);const MaterialPresentationContinuity::Limits policy;
    const auto old=owner.checkpoint();const auto before=old.stats();
    check(before.roots==4096&&before.nodes==4097&&before.logical_bytes>policy.checkpoint_bytes&&
        before.unique_bytes<policy.checkpoint_bytes/8&&before.new_value_bytes>0,
        "Complete repeated authored pass fanout did not distinguish expanded and unique storage under the unchanged cap");
    check(before.static_passes_checked==96&&before.static_passes_reused==(before.nodes-1)*96,
        "Repeated authored pass validation did not use exact same-capture pass/schema pairs");
    const auto repeated=owner.checkpoint();
    check(repeated.stats().unique_bytes==before.unique_bytes&&repeated.stats().shared_value_bytes>0&&
        !repeated.stats().new_value_bytes,"Prior shared immutable payload was omitted or copied in current graph accounting");
    for(const auto &material:mesh.materials)for(unsigned n=0;n<96;++n)
        check(&material.passes[n]==&std::as_const(*child).passes[n],
            "Authored repeated workload lost its actual shared const-pass identity");
    // The real authored rows share their const pass records across all roots
    // and the child. Retain every timed cut and validate complete accounting;
    // measure only checkpoint work, outside construction and restore paths.
    std::array<MaterialPresentationContinuity::Checkpoint,8> measured_cuts;
    std::size_t checked{},reused{};double capture_ms{};
    for(auto &cut:measured_cuts){
        const auto begin=std::chrono::steady_clock::now();cut=owner.checkpoint();
        capture_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
        const auto stats=cut.stats();checked+=stats.mutable_passes_checked;reused+=stats.mutable_passes_reused;
        check(cut.valid()&&owner.owns_checkpoint(cut)&&stats.nodes==before.nodes&&stats.links==before.links&&
            stats.unique_bytes==before.unique_bytes&&!stats.new_value_bytes&&stats.shared_value_bytes>0&&
            stats.mutable_passes_checked+stats.mutable_passes_reused==stats.nodes*96,
            "Repeated authored capture omitted a node/pass or changed complete inherited accounting");
    }
    std::cout<<"OBSERVED authored mutable-pass workload: captures="<<measured_cuts.size()<<" nodes="<<before.nodes
        <<" passes_per_node=96 checked="<<checked<<" reused="<<reused<<" capture_wall_ms="<<capture_ms
        <<" timing_scope=retained_checkpoints_only no_timing_gate=1\n";
    const auto original_pass=std::as_const(mesh.materials[4]).passes[93].constants;
    const auto original_program=mesh.materials[3].lightmap_program;
    const auto nan1=std::bit_cast<float>(0x7fc00011u),nan2=std::bit_cast<float>(0x7fc00022u);
    {
        auto mutation=owner.begin_mutation();
        // Real source producer performs its ordinary child COW. Immutable
        // checkpoint sharing must never become writable material sharing.
        apply_object_lighting(mesh.materials[0],{});
        mesh.materials[0].program_constants[0][0][0]=nan1;mesh.materials[1].program_constants[0][0][0]=nan2;
        mesh.materials[2].program_constants[0][0][0]=std::bit_cast<float>(0x80000000u);
        mesh.materials[3].lightmap_program.words[7]^=0x10204080u;
        mesh.materials[4].passes[93].constants[1][9][2]=nan1;
        mesh.materials[5].passes[93].constants[1][9][2]=nan2;
        mesh.materials[5].passes[4].program.words[3]^=0x08040201u;
        mesh.materials[7].vertex_bank=first_bank;
        mesh.materials[8].object_render_colors=std::array<Vec3,4>{};
        mesh.materials[8].object_render_colors->at(0).x=std::bit_cast<float>(0x80000000u);
    }
    const auto detached=mesh.materials[0].environment_pass;
    check(detached!=child&&mesh.materials[1].environment_pass==child&&
        std::as_const(mesh.materials[1]).passes[93].constants==original_pass,
        "Normal authored mutation changed an unrelated source pass/child owner");
    const auto changed=owner.checkpoint();const auto after=changed.stats();
    check(after.nodes==4098&&after.logical_bytes>policy.checkpoint_bytes&&after.unique_bytes>before.unique_bytes&&
        after.unique_bytes<policy.checkpoint_bytes/8&&after.new_value_bytes&&after.shared_value_bytes,
        "Distinct actual changed values or previous immutable components disappeared from compact accounting");
    {auto cancelled=owner.prepare_restore(old);check(owner.restore_ready(cancelled),"Compact authored cancellation failed");}
    check(std::bit_cast<std::uint32_t>(mesh.materials[0].program_constants[0][0][0])==0x7fc00011u,
        "Compact preparation changed a live NaN payload before commit");
    {
        auto prepared=owner.prepare_restore(old);const auto proof=prepared.target_graph();
        check(owner.commit_restore(prepared)&&proof.applied()&&mesh.materials.data()==roots,
            "Compact authored checkpoint failed exact historical publication");
        check(mesh.materials[0].environment_pass==child&&mesh.materials[7].vertex_bank==second_bank&&
            mesh.materials[6].vertex_bank==first_bank&&!mesh.materials[8].object_render_colors&&
            std::bit_cast<std::uint32_t>(mesh.materials[0].program_constants[0][0][0])==0&&
            std::as_const(mesh.materials[4]).passes[93].constants==original_pass&&
            mesh.materials[3].lightmap_program.words==original_program.words,
            "Compact authored restore lost optional, bank identity, aliases, programs or constants");
    }
    {
        auto prepared=owner.prepare_restore(changed);check(owner.commit_restore(prepared)&&prepared.target_graph().applied(),
            "Compact later cut was corrupted by restoring its predecessor");
        check(std::bit_cast<std::uint32_t>(mesh.materials[0].program_constants[0][0][0])==0x7fc00011u&&
            std::bit_cast<std::uint32_t>(mesh.materials[1].program_constants[0][0][0])==0x7fc00022u&&
            std::bit_cast<std::uint32_t>(mesh.materials[2].program_constants[0][0][0])==0x80000000u&&
            std::bit_cast<std::uint32_t>(std::as_const(mesh.materials[4]).passes[93].constants[1][9][2])==0x7fc00011u&&
            std::bit_cast<std::uint32_t>(std::as_const(mesh.materials[5]).passes[93].constants[1][9][2])==0x7fc00022u&&
            mesh.materials[0].environment_pass==detached&&mesh.materials[1].environment_pass==child&&
            mesh.materials[7].vertex_bank==first_bank&&mesh.materials[8].object_render_colors&&
            std::bit_cast<std::uint32_t>(mesh.materials[8].object_render_colors->at(0).x)==0x80000000u,
            "Immutable compact banks conflated distinct NaN/signzero, optional or per-node alias values");
    }
    MaterialPresentationContinuity::Limits too_small;too_small.checkpoint_bytes=before.unique_bytes-1;
    MaterialPresentationContinuity bounded(mesh,too_small);rejects([&]{(void)bounded.checkpoint();});
    std::cout<<"PASS compact original authored material fanout: every4096root/96pass and actual child alias retained; expanded payload exceeds unchanged256MiB while unique payload fits; prior reuse still charged; actual COW, divergent programs/constants, NaN payloads/signzero, optional and vertex-bank identity, old/later exact restores; expanded_bytes="
        <<before.logical_bytes<<" unique_bytes="<<before.unique_bytes<<" changed_unique_bytes="<<after.unique_bytes<<'\n';
}
void mutable_pass_pairs(const nlohmann::json &row){
    const auto material=[&](std::uint32_t constant,std::uint32_t program_xor=0){
        auto out=authored(row);auto pass=std::as_const(out).passes[0];
        pass.constants[0][0][0]=std::bit_cast<float>(constant);pass.program.words[7]^=program_xor;
        // Const insertion publishes an unexposed value copy. Rvalue insertion
        // preserves a possible mutable reference and would force later COW.
        out.passes.clear();out.passes.push_back(std::as_const(pass));return out;
    };
    const auto constant=[](const Material &m){return std::bit_cast<std::uint32_t>(m.passes[0].constants[0][0][0]);};
    const auto program=[](const Material &m){return m.passes[0].program.words[7];};
    // One actual const pass must be compared independently to BOTH prior
    // expectations. Vary constants and programs separately: neither expected
    // component may be omitted from a successful-pair identity.
    for(const bool different_program:{false,true}){
        Mesh mesh;mesh.materials.push_back(material(0x7fc00011u));
        mesh.materials.push_back(material(different_program?0x7fc00011u:0x7fc00022u,
            different_program?0x10204080u:0));
        auto *const roots=mesh.materials.data();
        const auto first_program=program(mesh.materials[0]),second_program=program(mesh.materials[1]);
        check(&std::as_const(mesh.materials[0]).passes[0]!=&std::as_const(mesh.materials[1]).passes[0],
            "Distinct authored predecessor expectations accidentally share one actual pass");
        MaterialPresentationContinuity owner(mesh);const auto old=owner.checkpoint();
        {auto mutation=owner.begin_mutation();
            mesh.materials[1].passes=std::as_const(mesh.materials[0]).passes;}
        check(&std::as_const(mesh.materials[0]).passes[0]==&std::as_const(mesh.materials[1]).passes[0],
            "Different-expected control did not merge the actual const pass record");
        const auto merged=owner.checkpoint();
        check(merged.stats().mutable_passes_checked==2&&!merged.stats().mutable_passes_reused,
            "One actual record was not compared to both distinct immutable expectations");
        {auto restore=owner.prepare_restore(old);check(owner.commit_restore(restore)&&
                constant(mesh.materials[0])==0x7fc00011u&&
                constant(mesh.materials[1])==(different_program?0x7fc00011u:0x7fc00022u)&&
                program(mesh.materials[0])==first_program&&program(mesh.materials[1])==second_program,
                "Different immutable predecessor constants/programs were conflated");}
        {auto restore=owner.prepare_restore(merged);check(owner.commit_restore(restore)&&
                restore.target_graph().applied()&&mesh.materials.data()==roots&&
                constant(mesh.materials[0])==0x7fc00011u&&constant(mesh.materials[1])==0x7fc00011u&&
                program(mesh.materials[0])==first_program&&program(mesh.materials[1])==first_program,
                "Capture reused a successful actual pass against a foreign expected component");}
    }
    {
        Mesh mesh;mesh.materials.push_back(material(0));MaterialPresentationContinuity owner(mesh);
        auto &exposed=mesh.materials[0].passes[0];const auto *const identity=&exposed;
        const auto first_program=exposed.program.words[7];
        const auto positive=owner.checkpoint(),seeded=owner.checkpoint();
        check(seeded.stats().mutable_passes_checked==1&&!seeded.stats().mutable_passes_reused,
            "Fresh checkpoint did not independently validate its first actual pass");
        owner.mutating();exposed.constants[0][0][0]=std::bit_cast<float>(0x80000000u);
        const auto negative=owner.checkpoint();
        owner.mutating();exposed.program.words[7]^=0x01020408u;
        const auto later=owner.checkpoint();
        check(&std::as_const(mesh.materials[0]).passes[0]==identity&&
            negative.stats().mutable_passes_checked==1&&later.stats().mutable_passes_checked==1,
            "Same-address later mutation escaped fresh per-capture validation");
        {auto ready=owner.prepare_restore(later);check(owner.restore_ready(ready),"Actual later mutable state was not ready");
            exposed.constants[0][0][0]=0.f;
            check(!owner.restore_ready(ready)&&!owner.commit_restore(ready),
                "Prepared readiness reused a capture memo after an unannounced same-address write");
            exposed.constants[0][0][0]=std::bit_cast<float>(0x80000000u);
            check(owner.restore_ready(ready),"Repaired mutable bits failed complete prepared readiness");}
        {auto restore=owner.prepare_restore(positive);check(owner.commit_restore(restore)&&
                constant(mesh.materials[0])==0&&program(mesh.materials[0])==first_program,
                "Historical mutable positive-zero/program bits changed");}
        {auto restore=owner.prepare_restore(negative);check(owner.commit_restore(restore)&&
                constant(mesh.materials[0])==0x80000000u&&program(mesh.materials[0])==first_program,
                "Later mutable signed-zero was compared as numeric equality");}
        {auto restore=owner.prepare_restore(later);check(owner.commit_restore(restore)&&
                constant(mesh.materials[0])==0x80000000u&&
                program(mesh.materials[0])==(first_program^0x01020408u),
                "Later same-address program mutation was lost to an earlier capture");}
    }
    {
        const auto prototype=material(0);Mesh mesh;mesh.materials.assign(2,prototype);
        const auto *const original=&std::as_const(mesh.materials[0]).passes[0];
        check(original==&std::as_const(mesh.materials[1]).passes[0],"Authored COW control lost its initially shared record");
        MaterialPresentationContinuity owner(mesh);const auto old=owner.checkpoint(),repeated=owner.checkpoint();
        check(repeated.stats().mutable_passes_checked+repeated.stats().mutable_passes_reused==2,
            "Shared authored COW control skipped a full predecessor pass occurrence");
        {auto mutation=owner.begin_mutation();mesh.materials[0].passes[0].constants[0][0][0]=std::bit_cast<float>(0x80000000u);}
        check(&std::as_const(mesh.materials[0]).passes[0]!=original&&
            &std::as_const(mesh.materials[1]).passes[0]==original,
            "Actual mutable MaterialPassBank access did not isolate its COW record");
        const auto split=owner.checkpoint();
        check(split.stats().mutable_passes_checked==2&&!split.stats().mutable_passes_reused,
            "New COW identity was treated as a previous successful actual-record pair");
        {auto restore=owner.prepare_restore(old);check(owner.commit_restore(restore)&&
                !constant(mesh.materials[0])&&!constant(mesh.materials[1]),
                "Old authored COW pass values were changed by a detached later record");}
        {auto restore=owner.prepare_restore(split);check(owner.commit_restore(restore)&&
                constant(mesh.materials[0])==0x80000000u&&!constant(mesh.materials[1]),
                "Restoring history corrupted the independent later COW pass value");}
    }
    std::cout<<"PASS exact mutable pass pairs: shared actual/different expected constants and programs, "
        "same-address later writes, fresh COW identities, NaN payloads/signedzero, nonmemo prepared checks and old/later restore\n";
}
void schema_validation_boundaries(const nlohmann::json &row){
    const auto metric=[](const std::string &message,const char *key){const auto start=message.find(key);
        check(start!=std::string::npos,"Immutable schema rejection omitted an original serialization offset");
        return std::stoull(message.substr(start+std::strlen(key)));};
    const auto field=[](const std::string &message,const char *name){
        check(message.find(name)!=std::string::npos,"Immutable schema cache admitted a different expected schema or lost its field/pass identity");
    };
    const auto original=authored(row);Mesh live;live.materials.resize(1);
    live.materials[0].passes.push_back(original.passes[0]);live.materials[0].vertex_constants.push_back({0,1,2,3});
    auto &record=live.materials[0].passes[0];record.states.push_back({7,0x11223344u});
    const auto *const identity=&record;MaterialPresentationContinuity owner(live);
    const auto first=owner.checkpoint(),next=owner.checkpoint();
    check(first.stats().static_passes_checked==1&&next.stats().static_passes_checked==1&&
        !first.stats().static_passes_reused&&!next.stats().static_passes_reused,
        "Immutable pass validation escaped its single completed capture");
    const auto word_error=[&](unsigned index){owner.mutating();record.definition.words[index]^=1;
        auto error=rejection([&]{(void)owner.checkpoint();});record.definition.words[index]^=1;
        field(error,"schema_member=passes.definition");field(error,"pass=0");return error;};
    const auto zero=word_error(0),seventeen=word_error(17);
    check(&std::as_const(live.materials[0]).passes[0]==identity&&metric(zero,"field_offset=")==0&&
        metric(seventeen,"field_offset=")==17*sizeof(std::uint32_t)&&
        metric(seventeen,"schema_offset=")==metric(zero,"schema_offset=")+17*sizeof(std::uint32_t),
        "Contiguous scalar-array schema comparison changed original word serialization or cached a prior identity");
    owner.mutating();record.definition.fragment_offsets[3]^=1;
    const auto fragment=rejection([&]{(void)owner.checkpoint();});record.definition.fragment_offsets[3]^=1;
    check(metric(fragment,"field_offset=")==60*sizeof(std::uint32_t)+3&&
        metric(fragment,"schema_offset=")==metric(zero,"schema_offset=")+60*sizeof(std::uint32_t)+3,
        "Nested original pixel-definition byte offsets changed after scalar-array batching");
    record.states[0].value^=1;const auto state=rejection([&]{(void)owner.checkpoint();});record.states[0].value^=1;
    field(state,"schema_member=passes.states");
    check(metric(state,"field_offset=")==sizeof(std::size_t)+sizeof(std::uint8_t),
        "Immutable padded ShaderState was serialized as raw aggregate storage");
    auto &constants=live.materials[0].vertex_constants[0];constants[0]=std::bit_cast<float>(1u);
    const auto x=rejection([&]{(void)owner.checkpoint();});constants[0]=0;
    constants[3]=std::bit_cast<float>(std::bit_cast<std::uint32_t>(3.f)^1u);
    const auto w=rejection([&]{(void)owner.checkpoint();});constants[3]=3;
    field(x,"schema_member=vertex_constants");field(w,"schema_member=vertex_constants");
    check(metric(w,"schema_offset=")==metric(x,"schema_offset=")+3*sizeof(float)&&
        metric(w,"field_offset=")==metric(x,"field_offset=")+3*sizeof(float),
        "Recursive scalar-array schema serialization changed an authored vector component offset");
    {auto prepared=owner.prepare_restore(first);record.definition.words[0]^=1;
        check(!owner.restore_ready(prepared)&&!owner.commit_restore(prepared),"Prepared readiness reused capture's transient schema memo");
        record.definition.words[0]^=1;check(owner.restore_ready(prepared),"Restored immutable schema failed full allocation-free readiness");}
    // Same actual pass address, two distinct expected canonical schemas. A
    // pointer-only cache would incorrectly accept root1 after checking root0.
    Mesh different;different.materials.resize(2);
    different.materials[0].passes.push_back(original.passes[0]);
    auto other=original.passes[0];other.definition.words[0]^=0x01020408u;different.materials[1].passes.push_back(other);
    const auto shared_bank=std::as_const(different.materials[0]).passes;
    MaterialPresentationContinuity distinct(different);(void)distinct.checkpoint();
    distinct.mutating();different.materials[1].passes=shared_bank;
    check(&std::as_const(different.materials[0]).passes[0]==&std::as_const(different.materials[1]).passes[0],
        "Different expected-schema control did not share the actual source pass");
    const auto conflict=rejection([&]{(void)distinct.checkpoint();});field(conflict,"root=1 ");
    field(conflict,"schema_member=passes.definition");field(conflict,"pass=0");
    // Normal source pass COW produces a new address even with the same schema.
    Mesh detached;detached.materials.resize(2);detached.materials[0].passes.push_back(original.passes[0]);
    detached.materials[1].passes=std::as_const(detached.materials[0]).passes;
    MaterialPresentationContinuity splitting(detached);const auto shared=splitting.checkpoint();
    check(shared.stats().static_passes_checked==1&&shared.stats().static_passes_reused==1,"Actual shared pass did not exercise capture memo reuse");
    const auto *const before=&std::as_const(detached.materials[1]).passes[0];auto &changed=detached.materials[1].passes[0];
    check(&changed!=before,"New identity control failed actual MaterialPassBank COW");splitting.mutating();changed.definition.words[0]^=1;
    const auto bad_detach=rejection([&]{(void)splitting.checkpoint();});field(bad_detach,"root=1 ");field(bad_detach,"schema_member=passes.definition");
    changed.definition.words[0]^=1;const auto accepted=splitting.checkpoint();
    check(accepted.stats().static_passes_checked==2&&!accepted.stats().static_passes_reused,
        "New COW identity bypassed its exact expected static schema");
    std::cout<<"PASS exact immutable pass-schema validation: canonical schemas, same actual pass/different expected refusal, same-identity next-capture mutation, new COW identity revalidation, full prepared checks, original scalar-array/vector/padded-field byte and pass offsets\n";
}
void schema_record_arrays(const nlohmann::json &row){
    auto decoded=authored(row);auto &source=decoded.passes[0];
    source.textures={{1,2,3,4},{5,6,7,8},{9,10,11,12}};
    source.texture_states.resize(3);
    for(unsigned n=0;n<3;++n){const std::uint32_t b=n*16;
        source.texture_states[n]={b+1,b+2,b+3,b+4,b+5,b+6,b+7,b+8,b+9,b+10,b+11,b+12,b+13,b+14};}
    const std::vector<ShaderRemap> mappings{{0x1100,1,2},{0x2200,3,4},{0x3300,5,6}};
    source.bitmap_remaps=source.pixel_remaps=source.vertex_remaps=mappings;
    source.pixel_externs=source.vertex_externs=source.state_externs=mappings;
    source.fragments={{-1,{0x0102}},{2,{0x0304}},{3,{0x0506}}};
    source.permutations={{-1,0x0102,{0x0304}},{2,0x0506,{0x0708}},{3,0x090a,{0x0b0c}}};
    source.combiners.resize(3);
    for(unsigned n=0;n<3;++n)for(unsigned w=0;w<8;++w)source.combiners[n].words[w]=0x10203040u+n*16+w;
    // These padded records deliberately stay on their named-field fallback.
    source.states={{1,0x11223344},{2,0x55667788},{3,0x99aabbcc}};
    decoded.layers={{0x0102},{0x0304},{0x0506}};
    decoded.vertex_constants={{0,1,2,3},{4,5,0,7},{8,std::bit_cast<float>(0x7fc12345u),10,11}};
    Mesh live;live.materials.resize(1);live.materials[0]=std::move(decoded);
    // Const insertion owns new records without an escaped mutable reference.
    const auto owned=std::as_const(live.materials[0]).passes;
    live.materials[0].passes.clear();live.materials[0].passes.push_back(owned[0]);
    const auto *const identity=&std::as_const(live.materials[0]).passes[0];
    const auto shared_before=std::as_const(live.materials[0]).passes;
    MaterialPresentationContinuity owner(live);const auto cut=owner.checkpoint();
    const auto repeated=owner.checkpoint();const auto shared_after=std::as_const(live.materials[0]).passes;
    check(&shared_before[0]==identity&&&shared_after[0]==identity&&
        cut.stats().static_passes_checked==1&&repeated.stats().static_passes_checked==1,
        "Schema array reads exposed, copied or cached a formerly sealed record across captures");
    const auto metric=[](const std::string &message,const char *key){const auto at=message.find(key);
        check(at!=std::string::npos,"Schema array rejection omitted its named byte offset");
        return std::stoull(message.substr(at+std::strlen(key)));};
    const auto field=[](const std::string &message,const char *name){
        check(message.find(std::string("schema_member=")+name+" ")!=std::string::npos,
            "Schema array rejection lost its original named field");};
    struct Byte {std::byte *at;std::size_t serialized;};
    const auto named=[](auto &scalar,unsigned byte,std::size_t offset){
        using T=std::remove_reference_t<decltype(scalar)>;
        static_assert(std::is_arithmetic_v<T>&&std::is_trivially_copyable_v<T>);
        check(byte<sizeof(T),"Schema array named scalar byte is outside its value");
        return Byte{reinterpret_cast<std::byte *>(&scalar)+byte,offset+byte};};
    // The actual allocation is nonconst even through its const bank view.
    // Deliberately bypass exposure/revision, then repair before normal use:
    // every new capture must still compare these bytes at the same identity.
    auto &untracked=const_cast<MaterialPass &>(*identity);untracked.texture_states[1].border_color^=1;
    const auto cast_error=rejection([&]{(void)owner.checkpoint();});untracked.texture_states[1].border_color^=1;
    field(cast_error,"passes.texture_states");
    const auto shared_repaired=std::as_const(live.materials[0]).passes;
    check(&shared_repaired[0]==identity,"Const-view schema tamper control changed record exposure or allocation identity");
    owner.mutating();auto &record=live.materials[0].passes[0];
    check(&record!=identity,"Schema array controls failed to exercise an actual selected-record COW");
    unsigned mutations{};
    const auto array=[&](auto &values,const char *name,std::size_t width,auto first,auto middle,auto last){
        check(values.size()==3,"Schema array control requires three distinct records");
        const std::array probes{first(values[0]),middle(values[1]),last(values[2])};std::uint64_t field_start{};
        for(unsigned n=0;n<probes.size();++n){const auto probe=probes[n];owner.mutating();*probe.at^=std::byte{1};
            const auto error=rejection([&]{(void)owner.checkpoint();});*probe.at^=std::byte{1};++mutations;
            field(error,name);const auto expected=sizeof(std::size_t)+n*width+probe.serialized;
            check(metric(error,"field_offset=")==expected,"Schema record array changed its original field serialization offset");
            if(!n)field_start=metric(error,"schema_offset=")-expected;
            check(metric(error,"schema_offset=")==field_start+expected,"Schema record array changed its absolute serialization offset");
            if(std::string_view(name).starts_with("passes."))
                check(metric(error,"pass=")==0,"Schema record array lost its selected pass ordinal");
        }
        owner.mutating();values.emplace_back();const auto extent=rejection([&]{(void)owner.checkpoint();});values.pop_back();
        field(extent,name);++mutations;
        const auto three=std::bit_cast<std::array<std::byte,sizeof(std::size_t)>>(std::size_t(3));
        const auto four=std::bit_cast<std::array<std::byte,sizeof(std::size_t)>>(std::size_t(4));
        const auto difference=std::mismatch(three.begin(),three.end(),four.begin()).first-three.begin();
        check(metric(extent,"field_offset=")==std::size_t(difference)&&
            metric(extent,"schema_offset=")==field_start+std::size_t(difference),
            "Schema record array lost its original element-count prefix");
    };
    array(record.textures,"passes.textures",4,
        [&](auto &p){return named(p.parameter,0,0);},[&](auto &p){return named(p.stage,0,2);},[&](auto &p){return named(p.flags,0,3);});
    array(record.texture_states,"passes.texture_states",56,
        [&](auto &p){return named(p.address_u,0,0);},[&](auto &p){return named(p.max_mip,0,28);},[&](auto &p){return named(p.key_color,3,52);});
    const auto remap_array=[&](auto &values,const char *name){array(values,name,4,
        [&](auto &p){return named(p.destination,0,0);},[&](auto &p){return named(p.component,0,2);},[&](auto &p){return named(p.source,0,3);});};
    remap_array(record.bitmap_remaps,"passes.bitmap_remaps");remap_array(record.pixel_remaps,"passes.pixel_remaps");
    remap_array(record.vertex_remaps,"passes.vertex_remaps");remap_array(record.pixel_externs,"passes.pixel_externs");
    remap_array(record.vertex_externs,"passes.vertex_externs");remap_array(record.state_externs,"passes.state_externs");
    array(record.fragments,"passes.fragments",4,
        [&](auto &p){return named(p.switch_index,0,0);},[&](auto &p){return named(p.permutations.packed,0,2);},[&](auto &p){return named(p.permutations.packed,1,2);});
    array(record.permutations,"passes.permutations",6,
        [&](auto &p){return named(p.value,0,0);},[&](auto &p){return named(p.flags,1,2);},[&](auto &p){return named(p.combiners.packed,1,4);});
    array(record.combiners,"passes.combiners",32,
        [&](auto &p){return named(p.words[0],0,0);},[&](auto &p){return named(p.words[4],1,16);},[&](auto &p){return named(p.words[7],3,28);});
    array(record.states,"passes.states",5,
        [&](auto &p){return named(p.index,0,0);},[&](auto &p){return named(p.value,1,1);},[&](auto &p){return named(p.value,3,1);});
    array(live.materials[0].layers,"layers",2,
        [&](auto &p){return named(p.packed,0,0);},[&](auto &p){return named(p.packed,0,0);},[&](auto &p){return named(p.packed,1,0);});
    array(live.materials[0].vertex_constants,"vertex_constants",16,
        [&](auto &p){return named(p[0],0,0);},[&](auto &p){return named(p[2],1,8);},[&](auto &p){return named(p[3],3,12);});
    const auto float_bits=[&](unsigned record_index,unsigned component,std::uint32_t changed){
        auto &value=live.materials[0].vertex_constants[record_index][component];const auto before=std::bit_cast<std::uint32_t>(value);
        owner.mutating();value=std::bit_cast<float>(changed);const auto error=rejection([&]{(void)owner.checkpoint();});value=std::bit_cast<float>(before);
        field(error,"vertex_constants");const auto a=std::bit_cast<std::array<std::byte,4>>(before),b=std::bit_cast<std::array<std::byte,4>>(changed);
        const auto at=std::mismatch(a.begin(),a.end(),b.begin()).first-a.begin();
        check(metric(error,"field_offset=")==sizeof(std::size_t)+record_index*16+component*4+std::size_t(at),
            "Schema float array compared numeric equality instead of exact signed-zero/NaN payload bits");++mutations;
    };
    float_bits(1,2,0x80000000u);float_bits(2,1,0x7fc12344u);
    {auto prepared=owner.prepare_restore(cut);check(owner.restore_ready(prepared),"Restored schema array controls did not recover exact original values");
        record.texture_states[1].border_color^=1;check(!owner.restore_ready(prepared)&&!owner.commit_restore(prepared),
            "Prepared readiness skipped a same-address static record-array write");record.texture_states[1].border_color^=1;
        check(owner.restore_ready(prepared),"Repaired static record array did not recover exact prepared readiness");}
    check(&shared_before[0]==identity&&&shared_after[0]==identity&&shared_before[0].texture_states[1].border_color==29,
        "Schema record-array control changed a shared pre-COW authored value or owner identity");
    std::cout<<"PASS exact named schema record arrays: first/middle/last payload bytes and size prefixes, padded fallback, signedzero/NaN bits, shared-record COW and full prepared tamper checks; mutations="<<mutations<<'\n';
}
void schema_named_scalars(const nlohmann::json &row){
    Mesh mesh;mesh.materials.resize(1);auto &m=mesh.materials[0];m=authored(row);
    m.pixel_constants={0x11223344u};m.vertex_declaration=SourceVertexDeclaration{0x10203040u,{1,2}};
    m.bitmaps={{7,9,std::bit_cast<float>(0x7fc12345u)}};
    m.lods={{{0x0102},0x11223344u,0.f}};
    auto &pass=m.passes[0];const auto *const pass_identity=&pass;
    MaterialPresentationContinuity owner(mesh);const auto cut=owner.checkpoint();unsigned mutations{};
    std::map<std::string,std::uint64_t> field_begins;
    const auto metric=[](const std::string &message,const char *key){const auto at=message.find(key);
        check(at!=std::string::npos,"Named scalar rejection omitted its exact byte offset");
        return std::stoull(message.substr(at+std::strlen(key)));};
    const auto first_byte=[](const auto &before,const auto &after){
        using T=std::remove_cvref_t<decltype(before)>;
        static_assert(std::is_trivially_copyable_v<T>);
        const auto a=std::as_bytes(std::span(&before,1)),b=std::as_bytes(std::span(&after,1));
        const auto at=std::mismatch(a.begin(),a.end(),b.begin()).first-a.begin();
        check(std::size_t(at)<sizeof(T),"Named scalar control did not change its representation");
        return std::size_t(at);
    };
    const auto diagnose=[&](const std::string &error,const char *name,std::size_t offset,bool selected_pass=false){
        check(error.find("reason=resident_schema")!=std::string::npos&&error.find("root=0 ")!=std::string::npos&&
            error.find(std::string("schema_member=")+name+" ")!=std::string::npos,
            "Named scalar rejection lost its field or resident boundary");
        const auto absolute=metric(error,"schema_offset=");
        check(metric(error,"field_offset=")==offset&&absolute>=offset,
            "Named scalar comparison changed the first differing serialized byte");
        const auto at=field_begins.emplace(name,absolute-offset).first;
        check(at->second==absolute-offset,"Named scalar mutations changed their absolute schema field origin");
        check(error.find(selected_pass?"pass=0":"pass=none")!=std::string::npos,
            "Named scalar rejection lost its selected pass boundary");
        check(owner.checkpoint().valid(),"Repairing a named scalar did not restore the original schema stream");
        ++mutations;
    };
    const auto scalar=[&](auto &live,const auto changed,const char *name,std::size_t prefix=0,bool selected_pass=false){
        const auto before=live;const auto offset=prefix+first_byte(before,changed);owner.mutating();live=changed;
        const auto error=rejection([&]{(void)owner.checkpoint();});live=before;diagnose(error,name,offset,selected_pass);
    };
    scalar(m.source_shader_datum,m.source_shader_datum^0x01000000u,"source_shader_datum");
    scalar(m.source_shader_datum,m.source_shader_datum^1u,"source_shader_datum");
    scalar(m.water_definition,-2,"water_definition");
    scalar(m.ui_full_surface,true,"ui_full_surface");
    scalar(m.ui_frame_extension,UiFrameExtension::Right,"ui_frame_extension");
    scalar(pass.template_pass,pass.template_pass^0x00010000u,"passes.template_pass",0,true);
    scalar(m.vertex_declaration->class_index,m.vertex_declaration->class_index^0x100u,
        "vertex_declaration",sizeof(bool));
    scalar(m.bitmaps[0].log_dimension,std::bit_cast<float>(0x7fc12344u),"bitmaps",
        sizeof(std::size_t)+2*sizeof(std::uint32_t));
    scalar(m.lods[0].layers.packed,std::uint16_t(m.lods[0].layers.packed^0x100u),"lods",sizeof(std::size_t));
    scalar(m.lods[0].projected_height,std::bit_cast<float>(0x80000000u),"lods",
        sizeof(std::size_t)+sizeof(std::uint16_t)+sizeof(std::uint32_t));
    {const auto saved=m.vertex_declaration;const bool present=true,missing=false;owner.mutating();m.vertex_declaration.reset();
        const auto error=rejection([&]{(void)owner.checkpoint();});m.vertex_declaration=saved;
        diagnose(error,"vertex_declaration",first_byte(present,missing));}
    {const auto before=m.pixel_constants.size(),after=before+1;owner.mutating();m.pixel_constants.push_back(0x55667788u);
        const auto error=rejection([&]{(void)owner.checkpoint();});m.pixel_constants.pop_back();
        diagnose(error,"pixel_constants",first_byte(before,after));}
    {const auto before=reinterpret_cast<std::uintptr_t>(std::as_const(m).diffuse.pixels.data());
        owner.mutating();auto original=std::move(m.diffuse.pixels);
        m.diffuse.pixels=TexturePixels(std::span<const std::byte>(std::as_const(original).data(),original.size()));
        const auto after=reinterpret_cast<std::uintptr_t>(std::as_const(m).diffuse.pixels.data());
        const auto error=rejection([&]{(void)owner.checkpoint();});m.diffuse.pixels=std::move(original);
        diagnose(error,"diffuse.pixels.identity",first_byte(before,after));}
    check(&std::as_const(m).passes[0]==pass_identity,"Named scalar comparison copied/exposed a different live pass");
    {auto prepared=owner.prepare_restore(cut);check(owner.restore_ready(prepared),"Named scalar preparation was not initially exact");
        m.ui_full_surface=true;check(!owner.restore_ready(prepared)&&!owner.commit_restore(prepared),
            "Prepared comparison skipped an unannounced named bool write");m.ui_full_surface=false;
        check(owner.restore_ready(prepared),"Repaired bool did not recover exact prepared readiness");
        m.vertex_declaration.reset();check(!owner.restore_ready(prepared)&&!owner.commit_restore(prepared),
            "Prepared comparison skipped an unannounced optional-presence write");
        m.vertex_declaration=SourceVertexDeclaration{0x10203040u,{1,2}};
        check(owner.restore_ready(prepared),"Repaired optional did not recover exact prepared readiness");}
    check(mutations==13,"Direct named schema scalar coverage changed");
    std::cout<<"PASS exact named schema scalars: integer/signed/enum/bool/optional/pointer/count bytes, NaN payload and signedzero, first-mismatch offsets and full prepared tamper checks; mutations="<<mutations<<'\n';
}
void retained_topology_history(const nlohmann::json &row){
    Mesh mesh;mesh.materials.assign(3,authored(row));
    auto child=std::make_shared<Material>(authored(row));
    mesh.materials[0].environment_pass=child;mesh.materials[1].environment_pass=child;
    const auto *const original_child=child.get();const std::weak_ptr<Material> original_lifetime=child;
    auto *const roots=mesh.materials.data();MaterialPresentationContinuity owner(mesh);
    std::array<MaterialPresentationContinuity::Checkpoint,128> cuts;
    const auto colors=[](unsigned n){const float v=float(n)/256;
        return ObjectColors{{{v,.25f,.5f},{.5f,v,.25f},{.25f,.5f,v},{v,v,v}}};};
    const auto exact_colors=[](const std::optional<ObjectColors> &actual,const ObjectColors &expected){
        if(!actual)return false;
        for(unsigned n=0;n<expected.size();++n){const auto &a=(*actual)[n],&b=expected[n];
            if(std::bit_cast<std::uint32_t>(a.x)!=std::bit_cast<std::uint32_t>(b.x)||
                std::bit_cast<std::uint32_t>(a.y)!=std::bit_cast<std::uint32_t>(b.y)||
                std::bit_cast<std::uint32_t>(a.z)!=std::bit_cast<std::uint32_t>(b.z))return false;}
        return true;
    };
    const double seconds=std::bit_cast<float>(row.at("seconds").get<std::uint32_t>());
    for(unsigned n=0;n<cuts.size();++n){
        {auto mutation=owner.begin_mutation();
            (void)animate_forward_material(mesh.materials[0],seconds+double(n)/30,{});
            apply_object_render_colors(mesh.materials[2],colors(n));
            mesh.materials[0].texture_frames={n+1,n+2,n+3,n+4};}
        cuts[n]=owner.checkpoint();
        check(cuts[n].valid()&&owner.owns_checkpoint(cuts[n])&&
            mesh.materials[0].environment_pass.get()==original_child&&
            mesh.materials[1].environment_pass.get()==original_child,
            "Retained authored updates changed their stable child topology");
        if(n)check(cuts[n].stats().nodes==cuts[0].stats().nodes&&
            cuts[n].stats().links==cuts[0].stats().links,
            "Stable authored history changed its complete topology census");
    }
    check(std::ranges::all_of(cuts,[](const auto &cut){return cut.valid();}),
        "Authored history failed to retain all128 simultaneous owning cuts");
    {const auto repeated=owner.checkpoint();const auto stats=repeated.stats();
        check(stats.unique_bytes==cuts.back().stats().unique_bytes&&stats.shared_value_bytes>0&&!stats.new_value_bytes,
            "Inherited authored payload became free or was copied in per-cut accounting");}
    // Every retained cut must retain its own changing value selection, even
    // though its actual child topology and most immutable banks are shared.
    for(unsigned n=0;n<cuts.size();++n){auto prepared=owner.prepare_restore(cuts[n]);
        check(owner.commit_restore(prepared)&&prepared.target_graph().applied()&&
            mesh.materials.data()==roots&&mesh.materials[0].environment_pass.get()==original_child&&
            mesh.materials[1].environment_pass.get()==original_child&&
            mesh.materials[0].texture_frames==std::array<std::uint32_t,4>{n+1,n+2,n+3,n+4}&&
            exact_colors(mesh.materials[2].object_render_colors,colors(n)),
            "One of128 retained cuts lost exact values, source aliases or root residency");
    }
    {auto mutation=owner.begin_mutation();apply_object_lighting(mesh.materials[0],{});}
    const auto *const split_child=mesh.materials[0].environment_pass.get();
    const std::weak_ptr<Material> split_lifetime=mesh.materials[0].environment_pass;
    check(split_child!=original_child&&mesh.materials[1].environment_pass.get()==original_child,
        "Real lighting did not detach only the actually mutated historical child");
    auto split=owner.checkpoint();
    check(split.stats().nodes==cuts.back().stats().nodes+1,
        "Actual child COW was collapsed into the previous topology");
    child.reset();
    {auto old=owner.prepare_restore(cuts.front());check(owner.commit_restore(old)&&
            mesh.materials[0].environment_pass.get()==original_child&&
            mesh.materials[0].texture_frames==std::array<std::uint32_t,4>{1,2,3,4}&&!split_lifetime.expired(),
            "Historical restore lost a detached child or its original values");
        {auto later=owner.prepare_restore(split);check(owner.commit_restore(later)&&
                mesh.materials[0].environment_pass.get()==split_child&&
                mesh.materials[1].environment_pass.get()==original_child&&
                mesh.materials[0].texture_frames==std::array<std::uint32_t,4>{128,129,130,131},
                "Restoring old history corrupted the later actual COW cut");}}
    {auto retire=owner.prepare_restore(cuts.front());check(owner.commit_restore(retire),
            "Final historical restore did not publish its actual child links");
        split={};check(!split_lifetime.expired(),
            "Final cut retirement destroyed a child still borrowed by the consumed token");}
    check(split_lifetime.expired(),"Consumed-token retirement left an unused detached child pinned");
    {auto mutation=owner.begin_mutation();mesh.materials[0].environment_pass.reset();mesh.materials[1].environment_pass.reset();}
    check(!original_lifetime.expired(),"Live128-cut history lost its detached original child");
    cuts={};check(original_lifetime.expired(),"Final128-cut retirement left the original child pinned");
    std::cout<<"PASS128-cut authored topology lifetime: changing values, stable aliases, real lighting COW, every historical cut, detached later restore, final cut and consumed-token retirement\n";
}
template<class T,class U>bool same_shared_owner(const std::shared_ptr<T> &a,const std::shared_ptr<U> &b){
    return !a.owner_before(b)&&!b.owner_before(a);
}
void topology_owner_aliases(const nlohmann::json &row){
    // Both leases retain the same actual Material, but their independent
    // control blocks have different lifetimes. get() equality cannot license
    // reusing an older link owner in a newly captured checkpoint.
    auto target=std::make_shared<Material>(authored(row));
    auto first_owner=std::make_shared<std::shared_ptr<Material>>(target);
    auto second_owner=std::make_shared<std::shared_ptr<Material>>(target);
    const std::weak_ptr<std::shared_ptr<Material>> first_lifetime=first_owner,second_lifetime=second_owner;
    std::shared_ptr<Material> first(first_owner,target.get()),second(second_owner,target.get());
    check(first.get()==second.get()&&!same_shared_owner(first,second),"Aliased-owner control did not have independent leases");
    Mesh mesh;mesh.materials.push_back(authored(row));mesh.materials[0].environment_pass=first;
    MaterialPresentationContinuity owner(mesh);auto early=owner.checkpoint();
    owner.mutating();mesh.materials[0].environment_pass=second;auto later=owner.checkpoint();
    {auto prepared=owner.prepare_restore(early);check(owner.commit_restore(prepared)&&
            same_shared_owner(mesh.materials[0].environment_pass,first),
            "Same-address historical restore installed a different control-block owner");}
    {auto prepared=owner.prepare_restore(later);check(owner.commit_restore(prepared)&&
            same_shared_owner(mesh.materials[0].environment_pass,second),
            "Same-address later checkpoint inherited an older owner lease");}
    owner.mutating();mesh.materials[0].environment_pass.reset();
    first.reset();second.reset();first_owner.reset();second_owner.reset();target.reset();
    check(!first_lifetime.expired()&&!second_lifetime.expired(),"Historical cuts did not retain both actual owner leases");
    early={};check(first_lifetime.expired()&&!second_lifetime.expired(),
        "Independent alias owners did not retire with their own final cut");
    later={};check(second_lifetime.expired(),"Later same-address owner survived its final cut retirement");
    {
        auto a=std::make_shared<unsigned>(1),b=std::make_shared<unsigned>(2);
        const std::weak_ptr<unsigned> weak_a=a,weak_b=b;
        std::shared_ptr<Material> null_a(a,nullptr),null_b(b,nullptr);
        check(!null_a&&!null_b&&!same_shared_owner(null_a,null_b),"Null alias controls lost their actual owners");
        Mesh empty;empty.materials.push_back(authored(row));empty.materials[0].environment_pass=null_a;
        MaterialPresentationContinuity null_owner(empty);auto old_null=null_owner.checkpoint();
        null_owner.mutating();empty.materials[0].environment_pass=null_b;auto new_null=null_owner.checkpoint();
        {auto prepared=null_owner.prepare_restore(old_null);check(null_owner.commit_restore(prepared)&&
                !empty.materials[0].environment_pass&&same_shared_owner(empty.materials[0].environment_pass,null_a),
                "Null historical link lost its nonempty ownership");}
        {auto prepared=null_owner.prepare_restore(new_null);check(null_owner.commit_restore(prepared)&&
                !empty.materials[0].environment_pass&&same_shared_owner(empty.materials[0].environment_pass,null_b),
                "Null later link reused the previous null-owner lease");}
        null_owner.mutating();empty.materials[0].environment_pass.reset();
        null_a.reset();null_b.reset();a.reset();b.reset();
        check(!weak_a.expired()&&!weak_b.expired(),"Null links did not retain both actual captured leases");
        old_null={};check(weak_a.expired()&&!weak_b.expired(),"Null-owner retirement merged independent histories");
        new_null={};check(weak_b.expired(),"Final null-alias cut left its owner pinned");
    }
    std::cout<<"PASS exact topology owner leases: same-get distinct control blocks and null aliases retain, restore and retire independent actual owners\n";
}
void prepared_topology_owners(const nlohmann::json &row){
    for(unsigned which=0;which<6;++which){
        const bool null_alias=which%2,detached=which>=2&&which<4,applied=which>=4;
        std::weak_ptr<std::shared_ptr<Material>> retired_first,retired_second;
        {
            auto target=std::make_shared<Material>(authored(row));
            auto first_owner=std::make_shared<std::shared_ptr<Material>>(target);
            auto second_owner=std::make_shared<std::shared_ptr<Material>>(target);
            retired_first=first_owner;retired_second=second_owner;
            std::shared_ptr<Material> first(first_owner,null_alias?nullptr:target.get());
            std::shared_ptr<Material> second(second_owner,null_alias?nullptr:target.get());
            check(first.get()==second.get()&&!same_shared_owner(first,second),
                "Prepared owner controls do not have independent same-address leases");
            Mesh mesh;mesh.materials.push_back(authored(row));auto *const roots=mesh.materials.data();
            auto historical=std::make_shared<Material>(authored(row));
            if(detached){historical->environment_pass=first;mesh.materials[0].environment_pass=historical;}
            else mesh.materials[0].environment_pass=first;
            MaterialPresentationContinuity owner(mesh);const auto cut=owner.checkpoint();
            std::shared_ptr<Material> live;
            {auto mutation=owner.begin_mutation();mesh.materials[0].texture_frames[0]=42;
                if(detached){live=std::make_shared<Material>(*historical);mesh.materials[0].environment_pass=live;}}
            auto prepared=owner.prepare_restore(cut);auto proof=prepared.target_graph();
            auto &link=detached?historical->environment_pass:mesh.materials[0].environment_pass;
            check(owner.restore_ready(prepared)&&proof.ready(),"Exact prepared owner was not initially ready");
            if(!applied){
                // Deliberately bypass mutating(): preflight must detect this
                // exact owner change independently of its revision guard.
                link=second;
                check(!owner.restore_ready(prepared)&&!proof.ready()&&!owner.commit_restore(prepared),
                    "Same-address owner substitution passed prepared publication preflight");
                check(same_shared_owner(link,second)&&mesh.materials.data()==roots&&
                    mesh.materials[0].texture_frames[0]==42&&
                    (!detached||mesh.materials[0].environment_pass==live),
                    "Rejected owner substitution partially published a historical restore");
                link=first;
                check(owner.restore_ready(prepared)&&proof.ready(),
                    "Repairing the exact owner did not recover prepared readiness");
            }
            check(owner.commit_restore(prepared)&&proof.applied()&&mesh.materials.data()==roots&&
                mesh.materials[0].texture_frames[0]==0&&same_shared_owner(link,first),
                "Exact owner publication lost root residency, values or its captured lease");
            if(applied){link=second;check(!proof.applied(),
                    "Applied proof accepted an unannounced same-address owner substitution");
                link=first;check(proof.applied(),"Repairing the exact owner did not recover the applied proof");}
            // Cancellation has the same owner guard and may not consume the
            // original token or publish values while its owner is changed.
            {auto cancelled=owner.prepare_restore(cut);link=second;
                check(!owner.restore_ready(cancelled)&&!owner.commit_restore(cancelled)&&
                    same_shared_owner(link,second),"Cancelled preparation accepted a changed owner");
                link=first;}
        }
        check(retired_first.expired()&&retired_second.expired(),
            "Prepared/applied/cancelled owner controls leaked an actual shared lease");
    }
    std::cout<<"PASS exact prepared child ownership: same-address and null aliases, detached historical children, rejection before publication, repaired readiness/applied proofs, cancellation and lease retirement\n";
}
void topology_fresh_validation(const nlohmann::json &row){
    Mesh mesh;mesh.materials.push_back(authored(row));auto &root=mesh.materials[0];
    auto a=std::make_shared<Material>(authored(row)),b=std::make_shared<Material>(authored(row));
    auto leaf=std::make_shared<Material>(authored(row));a->environment_pass=leaf;
    root.environment_pass=a;root.planar_fog_pass=b;root.world_overlays_before={a,b};
    MaterialPresentationContinuity owner(mesh);const auto valid=owner.checkpoint();
    const auto fail=[&](auto change,auto repair,const char *reason){
        owner.mutating();change();std::string error;
        try{error=rejection([&]{(void)owner.checkpoint();});}catch(...){repair();throw;}
        repair();check(error.find(reason)!=std::string::npos,"A prior valid topology hid a new graph violation");
        check(owner.checkpoint().valid(),"Repairing topology did not recover its exact resident walk");
    };
    fail([&]{a->environment_pass=a;},[&]{a->environment_pass=leaf;},"reason=graph_cycle");
    fail([&]{root.environment_pass.reset();},[&]{root.environment_pass=a;},"reason=child_presence");
    fail([&]{std::swap(root.world_overlays_before[0],root.world_overlays_before[1]);},
        [&]{std::swap(root.world_overlays_before[0],root.world_overlays_before[1]);},"reason=alias_resident");
    fail([&]{root.world_overlays_before[1]=a;},[&]{root.world_overlays_before[1]=b;},"reason=alias_resident");
    check(valid.valid(),"Rejected topology capture retired a previously completed checkpoint");
    // Derive the budget from this actual complete graph, without assuming a
    // compiler's object sizes. Reusing old payload still pays the full charge.
    Mesh budget_mesh;budget_mesh.materials.assign(2,authored(row));
    MaterialPresentationContinuity measure(budget_mesh);const auto measured=measure.checkpoint().stats();
    MaterialPresentationContinuity::Limits limit;limit.checkpoint_bytes=measured.unique_bytes;
    MaterialPresentationContinuity bounded(budget_mesh,limit);const auto first=bounded.checkpoint(),repeated=bounded.checkpoint();
    check(first.stats().unique_bytes==limit.checkpoint_bytes&&repeated.stats().unique_bytes==first.stats().unique_bytes&&
        repeated.stats().shared_value_bytes&&!repeated.stats().new_value_bytes,
        "Inherited payload was not fully charged against its exact per-cut budget");
    bounded.mutating();budget_mesh.materials[1].texture_frames[0]=1;
    const auto over=rejection([&]{(void)bounded.checkpoint();});
    check(over.find("reason=checkpoint_budget")!=std::string::npos&&first.valid()&&repeated.valid(),
        "A new value plus inherited payload escaped the same full per-cut cap");
    std::cout<<"PASS fresh topology validation: later cycle, presence, order and resident-alias violations rejected; inherited payload pays the complete unchanged per-cut budget\n";
}
}
int main(int argc,char **argv)try{
    check(argc==2,"Supply the retained original camouflage/animation fixture");std::ifstream in(argv[1]);check(bool(in),"Missing material reference input");
    nlohmann::json source;in>>source;unsigned count{};for(const auto &row:source.at("cases")){one(row);++count;}
    check(count!=0,"Material producer fixture has no actual source cases");controls();generated_radar();capture_diagnostics();
    compact_authored_fanout(source.at("cases"));schema_validation_boundaries(source.at("cases").at(0));
    mutable_pass_pairs(source.at("cases").at(0));
    schema_record_arrays(source.at("cases").at(0));
    schema_named_scalars(source.at("cases").at(0));
    retained_topology_history(source.at("cases").at(0));topology_owner_aliases(source.at("cases").at(0));
    prepared_topology_owners(source.at("cases").at(0));topology_fresh_validation(source.at("cases").at(0));
    std::cout<<"PASS exact material presentation: original camouflage/animation/lighting COW, all child aliases and mutable pass values, fixed roots/images/PRT handles, sharing/accounting, cancellation/stale/foreign/reentry/expired/budget/cycle controls; cases="<<count<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
