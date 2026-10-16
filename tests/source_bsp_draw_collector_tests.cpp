#include "halo2/source_bsp_draw_collector.hpp"
#include "source_bsp_command_frame_fixture.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
struct Reader {
    std::span<const std::uint32_t> values;
    std::size_t at{};
    std::uint32_t word(){if(at==values.size())throw FormatError("BSP collector fixture truncated");return values[at++];}
    float real(){return std::bit_cast<float>(word());}
    Vec3 point(){return {real(),real(),real()};}
};
Words solve(std::span<const std::uint32_t> input,const Cache* cache,unsigned version,bool complete_owner=false) {
    Reader r{input};SourceBspAdmissionBsp bsp;
    const auto nc=r.word(),ni=r.word(),nd=r.word();
    if(nc>512 || ni>2048 || nd>2048)throw FormatError("BSP collector fixture owner bank");
    bsp.clusters.resize(nc);bsp.instances.resize(ni);bsp.definitions.resize(nd);
    std::vector<SourceBspDrawGeometry> clusters(nc),definitions(nd);
    std::vector<SourceBspDrawInstance> instances(ni);
    std::vector<Transform> transforms(ni);
    const auto geometry=[&](auto& g){g.geometry_class=std::uint16_t(r.word());g.mask_subparts=std::uint16_t(r.word());g.resident=r.word()!=0;};
    for(unsigned i=0;i<nc;++i){geometry(clusters[i]);for(auto& v:bsp.clusters[i].bounds)v=r.real();}
    for(auto& g:definitions)geometry(g);
    for(unsigned i=0;i<ni;++i) {
        auto& b=bsp.instances[i];auto& o=instances[i];b.definition=std::int16_t(r.word());
        b.center=r.point();b.radius=r.real();o.lighting_policy=std::uint16_t(r.word());
        o.lightmap_resident=r.word()!=0;o.transform_address=r.word();
        if(version>=2) {
            std::array<std::uint32_t,13> w;for(auto& v:w)v=r.word();
            transforms[i]=std::bit_cast<Transform>(w);
        }
    }
    SourceBspDrawContext context;context.mode=r.word();context.mask_arena_address=r.word();
    context.override_byte=std::bit_cast<std::int8_t>(std::uint8_t(r.word()));
    context.fog.classification=int(r.word());context.fog.planar_datum=r.word();
    context.fog.eye_fraction=r.real();context.fog.planar_flags=r.word();context.fog.planar_only=r.word()!=0;
    for(auto& v:context.camera.inverse_view_z)v=r.real();context.camera.viewport_height_metric=r.real();
    const bool lightmap_matches=r.word()!=0;
    SourceDrawUnitCullContext cull_context;
    if(version>=2){cull_context.query_mode=std::bit_cast<std::int32_t>(r.word());cull_context.center=r.point();cull_context.radius=r.real();}
    SourceBspAdmission admission;
    for(auto& entry:admission.cluster_entries)entry=std::uint8_t(r.word());
    const auto queue=[&](auto& q){const auto n=r.word();if(n>512)throw FormatError("BSP collector fixture queue extent");for(unsigned i=0;i<n;++i)q.push_back({r.word(),std::uint16_t(r.word()),std::uint16_t(r.word()),r.word()});};
    queue(admission.clusters);queue(admission.instances);
    SourceBspDrawState state;state.main_prefix=r.word();state.draw_count=r.word();
    const auto used=r.word();if(used>512)throw FormatError("BSP collector fixture arena extent");state.arena.used=std::uint16_t(used);
    for(auto& unit:state.units)for(auto& v:unit.words)v=r.word();
    for(auto& v:state.coverage)v=std::uint8_t(r.word());
    for(auto& v:state.arena.words)v=r.word();
    std::unique_ptr<halo2::test::BspCommandFrameFixture> frame;
    if(version>=3) {
        if(!cache)throw FormatError("joined geometry frame Cache producer missing");
        frame=std::make_unique<halo2::test::BspCommandFrameFixture>(r,*cache,bsp,transforms,clusters,definitions);
    }
    if(r.at!=input.size())throw FormatError("BSP collector fixture trailing words");
    Words output;
    const auto append_state=[&] {
        output.insert(output.end(),{state.main_prefix,state.draw_count,state.arena.used});
        for(const auto& unit:state.units)output.insert(output.end(),unit.words.begin(),unit.words.end());
        for(const auto v:state.coverage)output.push_back(v);
        output.insert(output.end(),state.arena.words.begin(),state.arena.words.end());
        for(const auto& v:admission.instances)output.push_back(v.flags);
    };
    if(frame) {
        frame->prepare(admission);
        SourceEffectLightGeometryFrameWorld world{{bsp,clusters,definitions,instances,lightmap_matches},frame->commands,frame->metadata};
        // Original tool verifies133720 leaves this exact draw bank unchanged,
        // and41C80 has no queue entries in every joined case.
        world.dynamic=[](auto&,auto&,const auto&){};
        world.object_commands=[](auto&,auto){};
        bool command_owner_called=false,culled_observed=false;
        if(complete_owner) {
            // Exercise the full owner dispatch against the unchanged original
            // joined fixture. A second typed attachment/reset is a failure.
            world.object_commands=[](auto&,auto){throw FormatError("duplicate typed light command producer");};
            world.complete_commands=[&](auto& commands,const auto& collected,const auto& query,
                const auto& command_context,auto selector) {
                if(command_owner_called || !culled_observed || &collected!=&state || &query!=&admission)
                    throw FormatError("complete light command owner order/identity differs");
                command_owner_called=true;
                source_bsp_build_part_commands(commands,collected.units,collected.arena,world.commands,
                    command_context,selector,[](auto&,auto){});
            };
        }
        world.cull_bank=[&](auto,const auto& unit){return frame->cull(unit);};
        world.opaque_geometry=[&](auto index){return frame->select(state.units.at(index));};
        SourceEffectLightGeometryFrameContext current{context,cull_context,frame->context,0};
        const auto result=source_effect_light_geometry_frame(state,admission,frame->state,frame->opaque,world,current,
            [&](auto phase,const auto&,const auto&){
                if(phase==SourceEffectLightGeometryFramePhase::culled)culled_observed=true;
                append_state();
            });
        if(complete_owner && !command_owner_called)throw FormatError("complete light command owner absent");
        output.push_back(result.cull_calls.size());
        for(const auto& call:result.cull_calls) {
            output.push_back(call.unit);
            for(auto v:{call.center.x,call.center.y,call.center.z,call.radius})output.push_back(std::bit_cast<std::uint32_t>(v));
        }
        frame->append(output,result.opaque_count);return output;
    }
    source_bsp_collect_draw_units(state,admission,{bsp,clusters,definitions,instances,lightmap_matches},context);
    append_state();
    if(version>=2) {
        if(!cache)throw FormatError("BSP collector/cull Cache producer missing");
        std::map<std::pair<bool,unsigned>,SourceBspRangeGeometry> geometries;
        std::vector<SourceCullDrawBanks> banks(850);
        cull_context.main_prefix=state.main_prefix;cull_context.draw_count=state.draw_count;cull_context.mode=context.mode;
        const auto first=context.mode?state.main_prefix:0u;
        const auto end=context.mode?state.draw_count:state.main_prefix;
        for(unsigned i=first;i<end;++i) {
            const auto& unit=state.units[i].words;
            if((unit[0]&1) || !(unit[0]&0x40))continue;
            if(unit[1]!=0xffffffffu)throw FormatError("BSP collector/cull fixture consumes dynamic owner");
            const bool instance=unit[0]&0x1000;const auto owner=(unit[3]>>(instance?18:9))&(instance?2047:511);
            const auto section=instance?unsigned(bsp.instances.at(owner).definition):owner;
            auto [it,inserted]=geometries.try_emplace({instance,section});
            if(inserted)it->second=load_source_bsp_range_geometry(*cache,0,
                instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,section);
            const auto& g=it->second;auto& bank=banks[i];bank.geometry_resolved=true;
            if(instance)bank.transform=transforms.at(owner);
            bank.geometry=source_bsp_sphere_cull_geometry(g);
        }
        const auto calls=source_draw_unit_sphere_cull(state.units,banks,state.arena,context.mask_arena_address,cull_context);
        append_state();output.push_back(std::uint32_t(calls.size()));
        for(const auto& call:calls){output.push_back(call.unit);for(auto v:{call.center.x,call.center.y,call.center.z,call.radius})output.push_back(std::bit_cast<std::uint32_t>(v));}
    }
    return output;
}
}
int main(int argc,char** argv) {try {
    const auto path=argc>=2?argv[1]:std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/effect_light_bsp_draw_collector.original.bin";
    std::optional<Cache> cache;if(argc==3)cache.emplace(argv[2]);
    else if(argc==1)cache.emplace(std::string(HALO2_SOURCE_DIR)+"/assets/disc/maps/lockout.map");
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)throw FormatError("BSP collector fixture missing");
    const auto size=file.tellg();if(size<12 || size%4)throw FormatError("BSP collector fixture byte extent");
    Words values(std::size_t(size)/4);file.seekg(0);file.read(reinterpret_cast<char*>(values.data()),size);
    if(!file)throw FormatError("BSP collector fixture unreadable");Reader r{values};
    if(r.word()!=0x43504442)throw FormatError("BSP collector fixture magic");const auto version=r.word();
    if(version<1||version>3)throw FormatError("BSP collector fixture version");
    const auto cases=r.word();std::size_t checked=0;
    for(unsigned i=0;i<cases;++i) {
        const auto ni=r.word(),no=r.word();if(ni>values.size()-r.at)throw FormatError("BSP collector input extent");
        const auto inputs=std::span(values).subspan(r.at,ni);
        const auto out=solve(inputs,cache?&*cache:nullptr,version);
        if(version>=3 && solve(inputs,cache?&*cache:nullptr,version,true)!=out)
            throw FormatError("complete light command owner differs from original joined fixture");
        r.at+=ni;
        if(no>values.size()-r.at || no!=out.size())throw FormatError("BSP collector output extent");
        for(unsigned j=0;j<no;++j)if(out[j]!=r.word())throw FormatError("BSP collector differs at case"+std::to_string(i)+" word"+std::to_string(j)+" native"+std::to_string(out[j])+" source"+std::to_string(values[r.at-1]));
        checked+=no;
    }
    if(r.at!=values.size() || cases<32 || checked<100000)throw FormatError("BSP collector fixture coverage");
    std::cout<<cases<<" unchanged BSP collector cases, "<<checked<<" exact retained unit/coverage/arena/queue words\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
