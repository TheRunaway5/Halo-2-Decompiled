#include "halo2/multiplayer_world.hpp"
#include "halo2/source_decal_draw.hpp"
#include "halo2/source_decal_frame_tu15.hpp"
#include "halo2/source_effect_tag_layout_tu15.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const char* message) {if(!value)throw FormatError(message);}
std::size_t quad_count(std::span<const SourceDecalCacheGeometry> records) {
    std::size_t count{};for(const auto& record:records)count+=record.quads.size();return count;
}
void same_draws(std::span<const SourceDecalDraw> expected,std::span<const SourceDecalDraw> actual) {
    check(expected.size()==actual.size(),"Detached frame lost or duplicated a retained source draw");
    for(unsigned index=0;index<expected.size();++index) {
        const auto& a=expected[index];const auto& b=actual[index];
        check(a.datum==b.datum && a.primary==b.primary && a.cache_datum==b.cache_datum &&
            a.layer==b.layer && a.texture==b.texture && a.color==b.color &&
            a.state.pixel.words==b.state.pixel.words && a.state.source_blend==b.state.source_blend &&
            a.state.destination_blend==b.state.destination_blend && a.state.blend_equation==b.state.blend_equation &&
            a.quads.size()==b.quads.size(),"Detached frame changed actual source draw order, identity, texture or state");
        for(unsigned q=0;q<a.quads.size();++q)for(unsigned v=0;v<4;++v) {
            const auto& x=a.quads[q][v];const auto& y=b.quads[q][v];
            check(x.packed_projection==y.packed_projection,"Detached frame changed actual packed projection");
            const std::array ax{x.position.x,x.position.y,x.position.z,x.projection[0],x.projection[1],x.projection[2]};
            const std::array by{y.position.x,y.position.y,y.position.z,y.projection[0],y.projection[1],y.projection[2]};
            for(unsigned component=0;component<ax.size();++component)
                check(std::bit_cast<std::uint32_t>(ax[component])==std::bit_cast<std::uint32_t>(by[component]),
                    "Detached frame changed actual source position or decoded projection bits");
        }
    }
}
}
int main(int argc,char** argv) {
    nlohmann::json report{{"passed",false},{"scope","Actual prepared MCC source world, retained decal cache and CPU draw binding; no framebuffer or network parity claim"}};
    const auto save=[&] {
        if(argc>2 && *argv[2]) {
            const std::filesystem::path path(argv[2]);
            if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path);file<<report.dump(2)<<'\n';
            if(!file)throw FormatError("Cannot save MCC decal integration report");
        }
    };
    try {
        check(argc>=2 && argc<=4,"Supply prepared MCC MAP [REPORT_JSON] [PLAYERS]");
        if(!std::filesystem::is_regular_file(argv[1]))return 77;
        MultiplayerOptions options;options.map=argv[1];
        options.shared_maps=options.map.parent_path();options.starting_weapon=5;
        options.session_random_seed=0x12345678u;
        options.local_players=argc>3?unsigned(std::stoul(argv[3])):1;
        check(options.local_players>=1 && options.local_players<=4,"Requires one to four actual local players");
        MultiplayerPreparedWorld world(options);
        check(world.cache->is_mcc(),"Integration acceptance requires the prepared imported MCC cache");
        check(world.source_objects && world.source_objects->owns_physics(),"Ordinary MCC world has no actual retained source authority");
        check(world.local_slots.size()==options.local_players,"Actual local source roster was not born");
        world.bind_source_local_views(world.local_slots);
        const auto& cache=*world.cache;
        report["map"]=cache.name();report["map_path"]=std::filesystem::absolute(options.map).string();
        report["local_players"]=options.local_players;

        std::array<unsigned,6> layers{};unsigned images{},decals{};
        for(const auto& tag:cache.select_tags({.group=tag_groups::decal,.require_address=true})) {
            const auto payload=cache.tag_payload(tag.handle());const Bytes root(payload);
            const auto* bitmap=cache.find_tag({root.u32(0x8c)},tag_groups::bitmap);
            check(bitmap,"Actual decal bitmap lost its full typed identity");
            const auto layout=qualify_source_decal_tag_layout_tu15(
                {tag,payload,*bitmap,cache.tag_payload(bitmap->handle())});
            ++layers.at(layout.layer);++decals;
            for(unsigned image=0;image<layout.bitmap_images;++image) {
                const auto texture=cache.load_bitmap(bitmap->datum,image);
                check(texture.layers==1 && !texture.depth && !texture.pixels.empty() && !texture.mips.empty(),
                    "Actual MCC decal image has no complete decoded two-dimensional resource");
                ++images;
            }
        }
        check(decals>0 && images>0,"Actual map has no decal resources to qualify");
        report["asset_layers"]=layers;report["asset_decals"]=decals;report["asset_images"]=images;
        const auto* globals=cache.query_tag({.group=tag_groups::globals,.require_address=true});
        check(globals,"Actual MCC rasterizer globals are absent");
        const Bytes globals_root(cache.tag_payload(globals->handle()));
        const Bytes raster(cache.read_tag_bytes(globals_root.u32(0x10c),0x108));
        const Bytes shaders(cache.read_tag_bytes(raster.u32(0x5c),std::size_t(raster.u32(0x58))*8u));
        const auto shader=shaders.u32(3*8+4);
        const auto code=cache.load_vertex_shader_code(shader,0);
        const auto declaration=cache.load_source_vertex_declaration(shader,0);
        qualify_source_effect_vertex_program_tu15(tag_groups::decal,declaration.class_index,code,declaration.semantics);
        report["original_vertex_program_bytes"]=code.size();
        for(unsigned layer=0;layer<6;++layer) {
            const auto state=source_decal_layer_state_tu15(layer);
            check(state.color_mask==0x10101 && state.cull==0x901 &&
                state.depth_function==0x203 && state.depth_bias==-200.f,
                "One of six ordinary source decal layer states lost its raster contract");
        }
        report["ordinary_layer_states"]=6;

        auto& memory=world.source_objects->memory();
        const auto initial=source_decal_vertex_cache_tu15(memory);
        report["initial_records"]=initial.size();report["initial_quads"]=quad_count(initial);
        for(unsigned tick=0;tick<60;++tick) {
            const auto before=world.match.simulation().ticks();world.match.step({});
            world.source_objects->advance_effect_frame(world.match.simulation().ticks()==before+1?tick_seconds:0.f);
        }
        const auto records=source_decal_vertex_cache_tu15(memory);
        report["ticks"]=world.match.simulation().ticks();
        report["records"]=records.size();report["quads"]=quad_count(records);
        report["records_by_layer"]=std::array<unsigned,source_decal_head_layers_tu15>{};
        for(const auto& record:records) {
            check(record.layer<source_decal_head_layers_tu15,"Live original decal has no original head bank");
            report["records_by_layer"][record.layer]=report["records_by_layer"][record.layer].get<unsigned>()+1;
        }
        SourceDecalScene scene(cache);report["scene_active"]=scene.active();
        // Declared complete cluster admission isolates the consumer from camera/PVS.
        // It is not a replacement for the ordinary rendered view's visibility owner.
        SourceBspAdmission view;
        for(unsigned cluster=0;cluster<512;++cluster)view.clusters.push_back({cluster,0,0xffff,0});
        std::array<ShaderVector,192> constants{};
        const auto draws=scene.draw(memory,view,constants,{0,0,640,480},{0,16777215,16,1048576});
        std::size_t drawn_quads{};std::set<std::uint32_t> primaries;
        for(const auto& draw:draws) {
            check(draw.texture && !draw.texture->pixels.empty(),"Actual retained decal draw has no decoded texture");
            check(primaries.insert(draw.primary).second,"One source decal primary was submitted more than once in a view");
            drawn_quads+=draw.quads.size();
        }
        report["draws"]=draws.size();report["drawn_quads"]=drawn_quads;
        check(!records.empty() && quad_count(records)>0,
            "Ordinary MCC source inputs did not populate a real decal vertex cache; consumer failure is not established");
        check(scene.active(),"Real MCC source decal cache exists but SourceDecalScene disables its consumer");
        check(!draws.empty() && drawn_quads>0,"Enabled MCC decal consumer omitted the real retained cache");
        const auto frame=source_decal_frame_tu15(memory);
        report["readback_tick"]=frame.tick;report["readback_heads"]=frame.heads.size();
        report["readback_allocations"]=frame.allocations.size();
        const auto serial=Bytes(memory.view(0x4c6858u,4)).u32(0);
        const auto pool=Bytes(memory.view(0x4f7220u,4)).u32(0);
        const auto data=Bytes(memory.view(pool+0x44u,4)).u32(0);
        const auto count=Bytes(memory.view(pool+0x38u,4)).u32(0);
        const auto raw=memory.view(data,std::size_t(count)*64u);
        const std::vector<std::byte> original_records(raw.begin(),raw.end());
        const auto detached=scene.draw(frame,view,constants,{0,0,640,480},{0,16777215,16,1048576});
        same_draws(draws,detached);
        same_draws(detached,scene.draw(frame,view,constants,{0,0,640,480},{0,16777215,16,1048576}));
        check(Bytes(memory.view(0x4c6858u,4)).u32(0)==serial &&
            std::ranges::equal(original_records,memory.view(data,std::size_t(count)*64u)),
            "Detached frame drawing changed original source serial or record stamps");
        report["detached_draws"]=detached.size();report["detached_repeat_exact"]=true;
        report["detached_source_stamps_unchanged"]=true;
        report["passed"]=true;save();
        std::cout<<"PASS actual MCC retained decals: "<<records.size()<<" records, "<<quad_count(records)
            <<" cache quads, "<<draws.size()<<" draws, "<<images<<" images, six ordinary layer states\n";
        return 0;
    } catch(const std::exception& error) {
        report["error"]=error.what();
        try {save();}catch(const std::exception& saving) {std::cerr<<saving.what()<<'\n';}
        std::cerr<<error.what()<<'\n';return 1;
    }
}
