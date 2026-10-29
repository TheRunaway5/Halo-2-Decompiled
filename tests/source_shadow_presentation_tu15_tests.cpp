#include "source_match_rebind_fixture.hpp"
#include "halo2/source_presented_model_pose_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_multiplayer_render_history_tu15.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include <cstring>

namespace {
template<class T>void put(SourceRuntimeMemory &m,std::uint32_t p,const T &v){m.write(p,std::as_bytes(std::span(&v,1)));}
std::vector<std::byte> bytes(const SourceRuntimeMemory &m,std::uint32_t p,std::size_t n){const auto b=m.view(p,n);return {b.begin(),b.end()};}
struct History {
    std::uint32_t frame{};std::vector<std::byte> pool,bits,records,arena,shadows;
    bool operator==(const History &)const=default;
};
History history(const SourceRuntimeMemory &m) {
    const auto p=word(m,0x515f14u);
    auto arena=bytes(m,0x494338u,4);const auto bank=bytes(m,0x49433cu,0x27000u);
    arena.insert(arena.end(),bank.begin(),bank.end());
    return {word(m,0x4c6854u),bytes(m,p+0x34u,14),bytes(m,word(m,p+0x48u),32),
        bytes(m,word(m,p+0x44u),65536),std::move(arena),bytes(m,0x5332c8u,4*0x484)};
}
SourceRuntimeMemory::Checkpoint split_model_arena(SourceRuntimeMemory::Checkpoint saved) {
    using Region=SourceRuntimeMemory::CheckpointRegion;
    constexpr std::uint32_t cursor=0x494338u,bank=0x49433cu,end=bank+0x27000u;
    std::vector<Region> result;bool split{};
    for(auto &region:saved.regions) {
        const auto limit=std::uint64_t(region.address)+region.bytes.size();
        if(region.address>cursor || limit<end) {result.push_back(std::move(region));continue;}
        check(region.access==SourceRuntimeMemory::Access::ReadWrite&&!region.page_bytes&&region.committed.empty(),
            "Native arena topology control did not select the actual writable source section");
        const auto data=std::span<const std::byte>(region.bytes);
        const auto add=[&](std::uint32_t address,std::size_t size,std::string owner) {
            if(size)result.push_back({address,region.access,std::move(owner),0,
                SourceRuntimeSnapshotBytes(data.subspan(address-region.address,size)),{}});
        };
        add(region.address,cursor-region.address,region.owner);
        add(cursor,4,"source1D640 first resource-storage counter");
        add(bank,0x27000,"source1D640 first resource-storage bank");
        add(end,std::size_t(limit-end),region.owner);split=true;
    }
    check(split,"Native completed source bytes lack their model arena");saved.regions=std::move(result);return saved;
}
void split_owner_history(const SourceRuntimeMemory &displayed,const SourceRuntimeMemory &incoming) {
    // Use genuine completed native pools, object salts and palette/history
    // bytes with the exact separate counter/bank declarations used by MCC's
    // mount_native_tu15_renderer_resource_storage_data. This is a controlled
    // topology seam; the live MCC runtime supplies its independent acceptance.
    const auto source=split_model_arena(displayed.checkpoint()),target=split_model_arena(incoming.checkpoint());
    SourceRuntimeMemory current,replacement;current.restore(source);replacement.restore(target);
    rejects([&]{current.view(0x494338u,5);});rejects([&]{replacement.view(0x494338u,5);});
    const auto before=current.checkpoint();
    check(history(current)!=history(replacement),"Split-owner control omitted distinct completed native histories");
    source_stage_multiplayer_render_history_tu15(current,replacement);
    check(history(replacement)==history(current)&&current.checkpoint()==before,
        "Native split cursor/bank import lost bytes or changed the displayed source");
    auto short_target=target;
    const auto bank=std::ranges::find(short_target.regions,0x49433cu,&SourceRuntimeMemory::CheckpointRegion::address);
    check(bank!=short_target.regions.end(),"Split-owner control lost its actual bank declaration");bank->bytes.pop_back();
    SourceRuntimeMemory short_bank;short_bank.restore(short_target);const auto unchanged=short_bank.checkpoint();
    rejects([&]{source_stage_multiplayer_render_history_tu15(current,short_bank);});
    check(short_bank.checkpoint()==unchanged,"Rejected split bank wrote an earlier native history patch");
}
void select(PhysicsLiveWorldTu15 &w,std::uint32_t owner) {
    // Declared candidates for original4A7B0/4AC20. The owner is a genuinely
    // allocated source unit; these controlled coverage inputs test retention
    // of real native selection output, not camera/visibility production.
    constexpr std::uint32_t candidates=0x60020000u;
    const SourceShadowCandidate candidate{1u,owner,std::int32_t(owner),1.f,128.f};
    put(w.memory(),candidates,candidate);
    auto a=w.prepare_source_call(std::array{candidates,1u});a.gp[0]=0x5332c8u;w.run_actual(0x4a7b0u,a);
    a=w.prepare_source_call(std::array{0x5332c8u,candidates,4u,std::bit_cast<std::uint32_t>(.005f)});
    w.run_actual(0x4ac20u,a);
    check(word(w.memory(),0x5332c8u+0x480u)>0,"Original shadow selection did not publish actual history");
}
void build(Owner &owner,std::uint32_t object,unsigned frames) {
    auto &w=owner.world->tu15_world();
    for(unsigned frame=0;frame<frames;++frame) {
        w.begin_model_frame();const auto c=w.build_world_model(object,0,1,false,0.f,1.f);
        check(Bytes(c.bytes).u16(0x166)>0,"Actual native render-cache fixture omitted the selected body");select(w,object);
    }
}
void pose(Owner &owner,std::uint32_t object) {
    auto &w=owner.world->tu15_world();auto &m=w.memory();
    SourcePresentedModelPoseTu15 shown;
    shown.object=object;shown.definition=owner.world->object_definition(object);
    shown.render_model=owner.cache.load_initial_model_definition(shown.definition).render_model;
    shown.root=owner.world->object_transform(object);shown.sphere=owner.world->object_sphere(object);
    shown.nodes=owner.world->completed_body_world_nodes(object);
    // Pure value-codec bounds: native main admission is256, while this input
    // bank legitimately covers the whole2048-object lifetime pool before it.
    std::vector<SourcePresentedModelPoseTu15> complete_pool(source_presented_model_owner_capacity_tu15,shown);
    for(unsigned i=0;i<complete_pool.size();++i){complete_pool[i].object=0x10000u|i;complete_pool[i].nodes.resize(1);}
    const auto complete_wire=source_execution_arguments(complete_pool);
    check(!complete_wire.empty(),"Whole native object-pool presentation bank was limited to main admission capacity");
    complete_pool.push_back(shown);rejects([&]{validate_source_presented_model_poses_tu15(complete_pool);});
    complete_pool.resize(400);for(auto &entry:complete_pool)entry.nodes.resize(source_presented_model_node_capacity_tu15);
    rejects([&]{validate_source_presented_model_poses_tu15(complete_pool);});
    const auto original_root=shown.root;
    const auto original_sphere=shown.sphere;
    const Vec3 offset{.125f,.25f,.0625f};shown.root.position=shown.root.position+offset;
    shown.sphere.center=shown.sphere.center+offset;for(auto &node:shown.nodes)node.position=node.position+offset;
    w.begin_model_frame();
    {
        SourcePresentedModelPoseScopeTu15 input(w,std::span(&shown,1));
        const auto sphere=owner.world->object_sphere(object);
        check(bits(sphere.center,shown.sphere.center),"Current presented native sphere was not published");
        const auto c=w.build_world_model(object,0,1,false,0.f,1.f);const Bytes collector(c.bytes);
        const auto used=w.rendered_world_nodes(object);
        check(used && used->size()==shown.nodes.size() && !std::memcmp(used->data(),shown.nodes.data(),shown.nodes.size()*sizeof(Transform)),
            "Native C++137800 palette discarded the renderer's sampled WORLD input");
        // Execute the authored native13A2E0 ABI too: its internal BF640 must
        // complete before the same temporary WORLD input reaches skinning.
        const auto body=w.source_object_body(object);
        const auto node_bank=body+word(m,body+0x116u,2);
        constexpr std::uint32_t out=0x60030000u;
        auto a=w.prepare_source_call(std::array{node_bank,c.address+0x75u,std::uint32_t(collector.u16(0x168)),0u,out});
        a.gp[1]=object;a.gp[2]=shown.render_model;w.run_actual(0x13a2e0u,a);
        const auto expected=0x49433cu+(collector.u32(4)&0x0fffffffu);
        check(word(m,out,2)==word(m,expected,2) && word(m,out+2u,2)==word(m,expected+2u,2),
            "Native13A2E0 and current collector have different palette extents");
        auto matrices=word(m,out,2);const auto regions=word(m,out+2u,2);
        check(regions<=16 && bytes(m,out+4u,regions*4u)==bytes(m,expected+4u,regions*4u),
            "Native sampled WORLD palette changed selected region maps");
        for(unsigned region=0;region<regions;++region)matrices=std::max(matrices,
            word(m,out+4u+region*4u,2)+word(m,out+6u+region*4u,2));
        check(matrices && matrices<=256 && bytes(m,out+0x44u,matrices*48u)==bytes(m,expected+0x44u,matrices*48u),
            "Native13A2E0 skin/compound/mapped rows differ from the current sampled WORLD collector");
        put(m,0x60060000u,std::uint32_t(0x15cab1e));
        rejects([&]{SourcePresentedModelPoseScopeTu15 nested(w,std::span(&shown,1));});
    }
    check(bits(owner.world->object_transform(object).position,original_root.position) &&
        bits(owner.world->object_sphere(object).center,original_sphere.center),
        "Presented root/spatial adoption escaped its synchronous native scope");
    check(word(m,0x60060000u)==0x15cab1eu,"Presented-pose restoration overwrote later native caller scratch residue");
    // This frame already owns a real palette from the previous scope. A new
    // sampled input cannot reuse it before the genuine next frame entry.
    const auto cache_before=owner.authority->checkpoint(binding);
    const auto repair=[&]{auto prepared=owner.authority->prepare(cache_before,binding);
        check(owner.authority->commit(std::move(prepared)),"Late-pose cache control could not restore the actual source owner");};
    rejects([&]{SourcePresentedModelPoseScopeTu15 late(w,std::span(&shown,1));
        w.build_world_model(object,0,1,false,0.f,1.f);});
    check(!source_has_presented_model_pose_tu15(m),"Rejected cached C++ palette retained its sampled pose owner");repair();
    rejects([&]{SourcePresentedModelPoseScopeTu15 late(w,std::span(&shown,1));
        auto a=w.prepare_source_call(std::array{object});w.run_actual(0x3ddc0u,a);});
    check(!source_has_presented_model_pose_tu15(m),"Rejected cached native palette retained its sampled pose owner");repair();
    {
        SourcePresentedModelPoseScopeTu15 early(w,std::span(&shown,1));w.begin_model_frame();
        const auto rebuilt=w.build_world_model(object,0,1,false,0.f,1.f);
        check(Bytes(rebuilt.bytes).u16(0x166)>0,"Actual frame advance did not admit the sampled palette");
        // Repeated collection in this same immutable scope remains native.
        const auto reused=w.build_world_model(object,0,1,false,0.f,1.f);
        check(Bytes(reused.bytes).u32(4)==Bytes(rebuilt.bytes).u32(4),
            "Fresh-scope repeated collection lost authentic native geometry reuse");
    }
    auto bad=shown;bad.object^=0x10000u;rejects([&]{SourcePresentedModelPoseScopeTu15 input(w,std::span(&bad,1));});
    bad=shown;bad.definition^=0x10000u;rejects([&]{SourcePresentedModelPoseScopeTu15 input(w,std::span(&bad,1));});
    bad=shown;bad.render_model^=0x10000u;rejects([&]{SourcePresentedModelPoseScopeTu15 input(w,std::span(&bad,1));});
    bad=shown;bad.nodes.pop_back();rejects([&]{SourcePresentedModelPoseScopeTu15 input(w,std::span(&bad,1));});
}
void replay(Owner &producer,Owner &receiver,std::uint32_t object) {
    auto original=producer.authority->checkpoint(binding);
    auto prepared=receiver.authority->prepare(original,binding);
    check(receiver.authority->commit(std::move(prepared)),"Presented-pose replay setup failed");
    SourcePresentedModelPoseTu15 shown;
    shown.object=object;shown.definition=producer.world->object_definition(object);
    shown.render_model=producer.cache.load_initial_model_definition(shown.definition).render_model;
    shown.root=producer.world->object_transform(object);shown.sphere=producer.world->object_sphere(object);
    shown.nodes=producer.world->completed_body_world_nodes(object);
    const Vec3 offset{.125f,-.125f,0};shown.root.position=shown.root.position+offset;
    shown.sphere.center=shown.sphere.center+offset;for(auto &node:shown.nodes)node.position=node.position+offset;
    auto phase=producer.authority->capture_execution_phase([&]{
        producer.world->begin_model_frame();
        SourcePresentedModelPoseScopeTu15 input(producer.world->tu15_world(),std::span(&shown,1));
        producer.world->build_world_model(object,0,1,false,0.f,1.f);
    });
    check(phase.calls.size()==4 && phase.calls[1].kind==SourceExecutionCallKind::BeginPresentedModelPoses &&
        phase.calls.back().kind==SourceExecutionCallKind::EndPresentedModelPoses,
        "Presented native model work did not retain its balanced typed journal owners");
    receiver.authority->replay_execution_phase(phase);
    check(encode_source_authority(producer.authority->checkpoint(binding))==
        encode_source_authority(receiver.authority->checkpoint(binding)),
        "Native presented-pose replay changed the complete coupled source world");
    const auto before=history(receiver.world->memory());
    auto unmatched=phase;unmatched.calls.pop_back();rejects([&]{receiver.authority->replay_execution_phase(unmatched);});
    auto nested=phase;nested.calls.insert(nested.calls.begin()+2,nested.calls[1]);rejects([&]{receiver.authority->replay_execution_phase(nested);});
    auto early=phase;std::swap(early.calls[1],early.calls.back());rejects([&]{receiver.authority->replay_execution_phase(early);});
    auto gameplay=phase;SourceExecutionCall step;step.kind=SourceExecutionCallKind::MatchStep;
    step.arguments=source_execution_arguments(std::array<Controls,16>{});gameplay.calls.insert(gameplay.calls.begin()+2,std::move(step));
    rejects([&]{validate_source_execution_calls(gameplay.calls);});
    check(history(receiver.world->memory())==before,"Malformed pose scope mutated native state before admission");
    // Real failure after Begin must retire the scope and restore every
    // temporary input even though this replay target now needs full repair.
    original=producer.authority->checkpoint(binding);prepared=receiver.authority->prepare(original,binding);
    check(receiver.authority->commit(std::move(prepared)),"Replay failure-control setup failed");
    auto failed=phase;failed.calls[2].arguments[2]^=std::byte{1};
    const auto body=receiver.world->tu15_world().source_object_body(object);
    const auto root_before=bytes(receiver.world->memory(),body+0x64u,60);
    rejects([&]{receiver.authority->replay_execution_phase(failed);});
    check(!source_has_presented_model_pose_tu15(receiver.world->memory()) &&
        bytes(receiver.world->memory(),body+0x64u,60)==root_before,
        "Rejected native replay retained presented-pose ownership or root bytes");
    prepared=receiver.authority->prepare(original,binding);check(receiver.authority->commit(std::move(prepared)),"Failed pose replay did not permit full native repair");
}
void recipient_only_resources(Owner &host,Owner &recipient,Owner &staging) {
    check(host.content.player_representations.size()>3,"Resource lifetime control lacks the authored Elite representation");
    const auto definition=host.content.player_representations[3].biped.datum;
    const auto model=host.cache.load_initial_model_definition(definition).render_model;
    const auto unit=host.world->create_biped(definition,host.cache.spawns().at(0).position,{1,0,0});
    auto &native=host.world->tu15_world();
    const auto metadata=word(native.memory(),word(native.memory(),0x4f0414u)+(model&65535u)*16u+8u);
    const auto sections_at=word(native.memory(),metadata+0x28u);
    const auto cold=host.authority->checkpoint(binding);
    check(std::ranges::none_of(cold.world.world.model_collectors,[&](const auto &entry){return entry.first.second==unit;}),
        "Host unexpectedly built the recipient-only native unit");
    auto prepared=recipient.authority->prepare(cold,binding);prepared.retain_live_multiplayer_render_history();
    check(recipient.authority->commit(std::move(prepared)),"Recipient-only model setup could not import the actual new unit");
    auto &w=recipient.world->tu15_world();w.begin_model_frame();
    const auto first=w.build_world_model(unit,0,1,false,0.f,1.f);const Bytes c(first.bytes);
    check(c.u16(0x166) && c.u32(0x5c)==model,"Recipient-only Elite model did not actually build");
    std::vector<unsigned> selected;unsigned cold_sections{},static_sections{};
    const auto prove_resident=[&](const SourceRuntimeMemory &m,unsigned section,std::uint32_t address) {
        const auto raw=load_menu_world_source_model_section(host.cache,model,section);const Bytes header(raw.raw_model_header());
        for(const auto &resource:raw.resources()) {
            if(resource.type!=0 && resource.type!=8 && resource.type!=16 && resource.type!=32 && resource.type!=100)continue;
            check(word(m,address+resource.type)==header.u32(resource.type),"Actual native resident resource count lost its authored owner");
            const auto bank=m.view(word(m,address+resource.type+4u),resource.bytes.size());
            check(std::ranges::equal(bank,resource.bytes),"Actual native resident resource bytes are stale, foreign or unowned");
        }
    };
    for(unsigned region=0;region<c.u32(0x18);++region) {
        const auto section=std::to_integer<unsigned>(c.slice(0x75u+region,1).front());
        if(section!=255) {
            const auto resident=word(native.memory(),sections_at+section*0x5cu+0x34u);
            if(resident){prove_resident(native.memory(),section,resident);++static_sections;}
            else ++cold_sections;
            selected.push_back(section);
        }
    }
    check(!selected.empty(),"Recipient-only model selected no authored sections");
    const auto geometry=recipient.cache.load_model_sections(model,false,0,c.u16(0x168),selected);
    const auto before=source_live_model_collector_vertices_tu15(w,first,0,geometry);
    check(!before.empty(),"Recipient-only native geometry produced no real vertices");
    const auto frame=word(w.memory(),0x4c6854u);
    const auto displayed_history=history(w.memory());
    {
        auto cancelled=staging.authority->prepare(cold,binding);
        cancelled.retain_live_multiplayer_render_history_from(*recipient.authority);
    }
    check(history(w.memory())==displayed_history,"Cancelled cross-world import changed displayed native history");
    rejects([&]{auto invalid=staging.authority->prepare(cold,binding);
        invalid.retain_live_multiplayer_render_history_from(*staging.authority);});
    auto invalid=cold;invalid.version=1;
    rejects([&]{staging.authority->prepare(invalid,binding);});
    check(history(w.memory())==displayed_history,"Failed cross-world preparation changed displayed native history");
    prepared=staging.authority->prepare(cold,binding);
    prepared.retain_live_multiplayer_render_history_from(*recipient.authority);
    check(staging.authority->commit(std::move(prepared)),"Recipient-only resource import into distinct staging world failed");
    auto &next=staging.world->tu15_world();
    check(history(next.memory())==displayed_history,"Distinct staging world lost the displayed complete native history");
    check(word(next.memory(),0x4c6854u)==frame,"Resource lifetime control advanced away from actual same-frame reuse");
    const auto again=next.build_world_model(unit,0,1,false,0.f,1.f);const Bytes d(again.bytes);
    check(d.u16(0x166) && d.u16(0x168)==c.u16(0x168) && d.u32(4)==c.u32(4),
        "Post-import model lost actual same-frame native geometry/palette reuse");
    const auto after=source_live_model_collector_vertices_tu15(next,again,0,geometry);
    check(after.size()==before.size() && !std::memcmp(after.data(),before.data(),before.size()*sizeof(Vertex)),
        "Recipient-only native resource geometry changed or escaped current backing after cold-host import");
    const auto current_model=word(next.memory(),word(next.memory(),0x4f0414u)+(model&65535u)*16u+8u);
    const auto current_sections=word(next.memory(),current_model+0x28u);
    for(auto section:selected) {
        const auto resident=word(next.memory(),current_sections+section*0x5cu+0x34u);
        check(resident!=0,"Post-import actual native model has no resident resource owner");prove_resident(next.memory(),section,resident);
    }
    std::cout<<"Recipient-only actual Elite model resources: "<<selected.size()<<" sections / "<<after.size()
        <<" vertices; hostcold="<<cold_sections<<" static-owned="<<static_sections<<"; distinct-world post-import native reuse\n";
}
void single_import_differential(Owner &host,Owner &displayed,Owner &staging,
    const char *xbe,const char *map) {
    const std::array<unsigned,4> producer{0,1,2,3},locals{4,5,6,7};
    Owner legacy_workspace(xbe,map,producer,700000);
    displayed.local_views(locals);staging.local_views(producer);
    const auto saved=host.authority->checkpoint(binding);
    const std::array<unsigned,8> slots{0,1,2,3,4,5,6,7};
    const auto roster=host.authority->player_admissions(slots),admitted=admissions(host,locals);
    const auto before=displayed.authority->checkpoint(binding);
    const auto before_history=history(displayed.world->memory());
    auto prepared=staging.authority->prepare(saved,binding,roster);
    prepared.retain_live_multiplayer_render_history_from(*displayed.authority);
    check(staging.authority->commit(std::move(prepared)),"Single complete staging import failed");
    staging.local_views(locals);
    check(displayed.world->memory().checkpoint()==before.world.world.bank &&
        history(displayed.world->memory())==before_history,
        "Single staging import/rebind changed any displayed source byte");
    // The established two-import transaction remains an independent native
    // reference for the new one-import+actual151C90/151D30 presentation route.
    prepared=displayed.authority->prepare_with_local_views(saved,binding,admitted,
        *legacy_workspace.authority,roster,producer);
    prepared.retain_live_multiplayer_render_history();
    check(displayed.authority->commit(std::move(prepared)),"Independent two-import native reference failed");
    check(history(staging.world->memory())==history(displayed.world->memory()),
        "Single import changed complete native render-cache/arena/shadow history");
    same_gameplay(staging,displayed);
    for(unsigned tick=0;tick<8;++tick) {
        std::array<Controls,16> controls{};
        for(unsigned slot=0;slot<player_count;++slot) {
            controls[slot].yaw=displayed.match->unit(slot).yaw;
            controls[slot].forward=(slot&1u)?0.f:.1f;
        }
        staging.step(controls);displayed.step(controls);same_gameplay(staging,displayed);
    }
    const auto object=staging.match->player(locals.front()).source_current_object;
    check(object!=null_datum,"Single-import native future control lost its actual local unit");
    replay(staging,displayed,object);
    std::cout<<"PASS single complete native import and actual recipient rebind match independent two-import reference, eight complete future ticks and typed native pose replay\n";
}
}
int main(int argc,char **argv)try {
    if(argc!=3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    const std::array<unsigned,4> locals{0,1,2,3};Owner host(argv[1],argv[2],locals,1000),recipient(argv[1],argv[2],locals,900000),
        staging(argv[1],argv[2],locals,500000);
    for(unsigned tick=0;tick<8;++tick)host.step({});
    auto captured=host.authority->checkpoint(binding);
    auto prepared=recipient.authority->prepare(captured,binding);check(recipient.authority->commit(std::move(prepared)),"Initial genuine native import failed");
    const auto object=host.match->player(4).source_current_object;check(object!=null_datum,"Actual native player has no body");
    build(host,object,1);build(recipient,object,5);
    const auto before=history(recipient.world->memory());
    split_owner_history(recipient.world->memory(),host.world->memory());
    const auto native_before=recipient.authority->checkpoint(binding);
    captured=host.authority->checkpoint(binding);
    prepared=recipient.authority->prepare(captured,binding);
    check(recipient.authority->commit(std::move(prepared)),"Cold native import control failed");
    check(history(recipient.world->memory())!=before,"Cold host import did not exercise lost recipient cache/history");
    prepared=recipient.authority->prepare(native_before,binding);check(recipient.authority->commit(std::move(prepared)),"Native recipient control restoration failed");
    {
        auto cancelled=recipient.authority->prepare(captured,binding);cancelled.retain_live_multiplayer_render_history();
    }
    check(history(recipient.world->memory())==before,"Cancelled render-history preparation changed the live recipient");
    prepared=recipient.authority->prepare(captured,binding);prepared.retain_live_multiplayer_render_history();
    check(recipient.authority->commit(std::move(prepared)),"Complete native cut plus recipient render history did not commit");
    check(history(recipient.world->memory())==before,"Native import lost actual allocator/cache/arena/frame/shadow bytes");
    check(recipient.authority->execution_tick()==host.authority->execution_tick(),"Render history replaced authoritative simulation time");
    pose(recipient,object);
    replay(recipient,host,object);
    recipient_only_resources(host,recipient,staging);
    const auto definition=host.world->object_definition(object);const auto spawn=host.cache.spawns().at(0).position;
    const auto extra=host.world->create_biped(definition,spawn,{1,0,0});
    captured=host.authority->checkpoint(binding);prepared=recipient.authority->prepare(captured,binding);prepared.retain_live_multiplayer_render_history();
    check(recipient.authority->commit(std::move(prepared)),"Fresh genuine object cut failed");build(recipient,extra,1);
    const auto old_token=word(recipient.world->memory(),recipient.world->tu15_world().source_object_body(extra)+0xccu);
    host.world->destroy_object(extra);const auto replacement=host.world->create_biped(definition,spawn,{1,0,0});
    check(replacement!=extra,"Native retirement did not change the complete object generation");
    captured=host.authority->checkpoint(binding);prepared=recipient.authority->prepare(captured,binding);prepared.retain_live_multiplayer_render_history();
    check(recipient.authority->commit(std::move(prepared)),"Retired native object cut failed");
    const auto body=recipient.world->tu15_world().source_object_body(replacement);
    check(word(recipient.world->memory(),body+0xccu)==null_datum,"New object generation inherited a predecessor render cache");
    build(recipient,replacement,1);
    check(word(recipient.world->memory(),body+0xccu)!=old_token,"Native cache allocation reused the retired complete token");
    rejects([&]{recipient.world->tu15_world().source_object_body(extra);});
    single_import_differential(host,recipient,staging,argv[1],argv[2]);
    std::cout<<"PASS actual native recipient cache/history import, cold/cancelled controls, presented native/C++ palette equality, full-salt retirement\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
