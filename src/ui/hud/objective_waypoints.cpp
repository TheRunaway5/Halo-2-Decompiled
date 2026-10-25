#include "halo2/objective_waypoints.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/mcc_hud_reference.hpp"
#include "halo2/mcc_hud_bitmap_geometry.hpp"
#include "halo2/texture.hpp"
#include <set>
#include <algorithm>
#include <bit>
namespace halo2 {
namespace {
Material material_for(const Cache& cache,std::uint32_t shader,const Texture& image) {
    auto m=cache.load_material(shader,true);
    if(!m.passes.empty() && m.passes.front().name=="shaders\\shader_passes\\hud\\waypoint_emblem") {
        m=frontend_emblem_material(cache,shader);m.ui_full_surface=true;
    }
    if(cache.is_mcc())for(auto& overlay:m.animation.overlays) {
        overlay.input=normalize_mcc_hud_function_operand(overlay.input,cache.string_id(overlay.input));
        overlay.range=normalize_mcc_hud_function_operand(overlay.range,cache.string_id(overlay.range));
    }
    if(m.ui_emblem_pass)return m;
    if(m.passes.size()!=1)throw FormatError("Objective waypoint requires its actual single HUD pass");
    const auto& p=m.passes.front();unsigned src=0x302,dst=0x303;
    for(const auto& state:p.states){if(state.index==62)src=state.value;if(state.index==63)dst=state.value;}
    if(src==0x302 && dst==0x303)m.forward_blend=1;
    else if(src==1 && dst==0x303)m.forward_blend=2;
    else if(src==1 && dst==1)m.forward_blend=3;
    else if(src==1 && dst==0x302)m.forward_blend=4;
    else throw FormatError("Unsupported original objective waypoint blend");
    for(const auto& binding:p.textures)if(binding.stage!=0 || (binding.external!=15 && binding.external!=16))
        throw FormatError("Objective waypoint requires its authored texture binding");
    m.diffuse=image;m.hud_pass=m.forward_pass=true;m.static_lightmap_pass=false;
    m.forward_pass_index=0;m.forward_program=p.program;m.forward_vertex=0;m.vertex_program.clear();
    m.program_constants=p.constants;m.environment_pass.reset();m.accumulation_texture=false;m.ui_full_surface=true;
    if(!p.texture_states.empty())m.texture_states[0]=p.texture_states[0];
    return m;
}
}
ObjectiveWaypoints::ObjectiveWaypoints(const Cache& cache,Mesh& mesh,unsigned views):mesh_(mesh),views_(views) {
    if(!views || views>4)throw FormatError("Invalid objective waypoint view count");
    for(const auto& color:cache.load_full_scoreboard_assets().palette)palette_.push_back(std::bit_cast<Vec3>(color));
    const Tag* selected=nullptr;
    for(const auto& tag:cache.select_tags({.group=tag_groups::globals,.require_address=true})) {
        if(selected)throw FormatError("Objective waypoints require unique globals");selected=&tag;
    }
    if(!selected)throw FormatError("Objective waypoints have no globals");
    const auto globals=cache.load_source_tag_metadata(selected->datum);
    const auto matg=Bytes(globals.bytes_at(globals.source_address(),0x118));
    if(!matg.u32(0x110))throw FormatError("Objective waypoints require actual HUD globals");
    const auto interface=Bytes(globals.bytes_at(matg.u32(0x114),0x98));
    const auto metadata=cache.load_source_tag_metadata(interface.u32(0x34));
    if(metadata.group()!="hudg")throw FormatError("Objective HUD globals group differs");
    const auto hud=Bytes(metadata.bytes_at(metadata.source_address(),0x418));
    std::map<std::pair<std::uint32_t,std::uint32_t>,unsigned> resources;
    for(unsigned bank=0;bank<2;++bank) {
        const unsigned offset=bank?0x410:0x408,stride=bank?24:36;
        const auto count=hud.u32(offset);
        if(count>64)throw FormatError("Excessive authored waypoint styles");
        const auto rows=Bytes(metadata.bytes_at(hud.u32(offset+4),count*stride));
        for(unsigned n=0;n<count;++n) {
            const auto bitmap=rows.u32(n*stride+4),shader=rows.u32(n*stride+12);
            if(bitmap!=null_datum && shader!=null_datum)resources[{shader,bitmap}]=64;
            if(!bank) {
                const auto meter=rows.u32(n*stride+32);
                if(meter!=null_datum && shader!=null_datum)resources[{shader,meter}]=256;
            }
        }
    }
    for(const auto& [resource,capacity]:resources) {
        const auto [shader,bitmap]=resource;
        const auto count=cache.bitmap_count(bitmap);
        if(count>256)throw FormatError("Excessive objective bitmap frames");
        UiImage image;image.name=cache.query_tag({.handle=TagHandle{bitmap}})->name;
        for(unsigned frame=0;frame<count;++frame) {
            auto texture=cache.load_bitmap(bitmap,frame);
            if(texture.mips.empty() || texture.mips.front().offset || !texture.mips.front().size)
                throw FormatError("Objective UI bitmap lacks its actual highest-detail resource");
            texture.pixels.resize(texture.mips.front().size);texture.mips.resize(1);
            if(texture.format==16)texture=prepare_xemu_bc3_texture(texture);
            image.images.push_back(std::move(texture));
        }
        const auto bitmap_metadata=cache.load_source_tag_metadata(bitmap);
        const auto bitmap_root=Bytes(bitmap_metadata.bytes_at(bitmap_metadata.source_address(),0x4c));
        const auto sequences=Bytes(bitmap_root.u32(0x3c)?bitmap_metadata.bytes_at(bitmap_root.u32(0x40),bitmap_root.u32(0x3c)*0x3c):std::span<const std::byte>{});
        for(unsigned n=0;n<bitmap_root.u32(0x3c);++n) {
            const auto at=n*0x3c;std::vector<UiSprite> sequence;
            const auto sprites=Bytes(sequences.u32(at+0x34)?bitmap_metadata.bytes_at(sequences.u32(at+0x38),sequences.u32(at+0x34)*32):std::span<const std::byte>{});
            for(unsigned j=0;j<sequences.u32(at+0x34);++j) {
                const auto q=j*32;sequence.push_back({sprites.u16(q),sprites.f32(q+8),sprites.f32(q+12),sprites.f32(q+16),sprites.f32(q+20),{sprites.f32(q+24),sprites.f32(q+28)}});
            }
            if(sequence.empty())for(unsigned j=0;j<sequences.u16(at+0x22);++j)sequence.push_back({sequences.u16(at+0x20)+j,0,1,0,1,{}});
            image.sequences.push_back(std::move(sequence));
        }
        const std::map<std::string,std::vector<std::array<std::int16_t,4>>> original_layouts{
#include "ui/hud/objective_bitmap_layout_tu15.inc"
        };
        const auto layout=original_layouts.find(image.name);
        if(layout==original_layouts.end() || layout->second.size()!=count)throw FormatError("Objective bitmap has no original registered layout");
        for(unsigned frame=0;frame<count;++frame) {
            const auto logical=mcc_hud_bitmap_logical_extent(image,frame);
            if(!logical || (*logical)[0]!=unsigned(layout->second[frame][0]) || (*logical)[1]!=unsigned(layout->second[frame][1]))
                throw FormatError("Objective bitmap differs from its authenticated MCC-to-Xbox artwork mapping: "+image.name);
        }
        if(std::ranges::none_of(bitmaps_,[&](const auto& b){return b.datum==bitmap;}))bitmaps_.push_back({bitmap,layout->second});
        for(unsigned frame=0;frame<count;++frame) {
            const auto material=material_for(cache,shader,image.images[frame]);
            auto& slots=materials_[{shader,bitmap,frame}];slots.resize(views);
            for(auto& view:slots)for(unsigned n=0;n<capacity;++n) {
                view.push_back(unsigned(mesh_.materials.size()));mesh_.materials.push_back(material);
            }
        }
    }
}
std::vector<Vertex> ObjectiveWaypoints::draw(std::span<const SourceObjectiveQuadTu15> quads,
    const MultiplayerState& state,unsigned player,unsigned view,unsigned width,unsigned height) {
    if(view>=views_ || !width || !height)throw FormatError("Invalid objective waypoint viewport");
    std::map<Key,unsigned> used;std::vector<Vertex> vertices;vertices.reserve(quads.size()*6);
    for(const auto& q:quads) {
        const Key key{q.shader,q.bitmap,q.frame};const auto found=materials_.find(key);
        if(found==materials_.end())throw FormatError("Objective waypoint selected an unprepared authored resource");
        const auto instance=used[key]++;
        if(instance>=found->second[view].size())throw FormatError("Objective waypoint exceeds original resident caller capacity");
        const auto slot=found->second[view][instance];auto& m=mesh_.materials.at(slot);const auto& pass=m.passes.front();
        m.forward_program=pass.program;
        FrontendAppearance appearance;
        if(m.ui_emblem_pass) {
            const auto owner=std::ranges::find_if(state.players,[&](const auto& p){return p.source_player==q.emblem_player && p.connected;});
            if(owner==state.players.end() || !owner->source_configuration)
                throw FormatError("Objective emblem has no actual retained player appearance");
            const auto& c=*owner->source_configuration;
            for(unsigned i=0;i<4;++i)appearance.colors[i]=std::bit_cast<std::int8_t>(c[0x40+i]);
            appearance.species=std::bit_cast<std::int8_t>(c[0x44]);appearance.foreground=c[0x45];appearance.background=c[0x46];appearance.flags=c[0x47];
            apply_object_colors(m,frontend_appearance_colors(appearance,palette_));
        }
        const auto function=[&](std::uint32_t sid) {
            if(sid==0x0d0005ca)return q.inputs[0];
            if(sid==0x0c0005cb)return q.inputs[1];
            if(sid==0x0c0005cc)return q.inputs[2];
            if(sid==0x0e0005cd)return q.inputs[3];
            if(sid>=0x0a0005cf && sid<=0x0a0005d2)return q.inputs[6+sid-0x0a0005cf];
            if(sid==0x090005ce) {
                const auto& p=state.players.at(player);
                const auto species=p.source_configuration?(*p.source_configuration)[0x44]:0;
                return species==1 || species==3?1.f:0.f;
            }
            return m.ui_emblem_pass?frontend_emblem_input(appearance,sid,1):1.f;
        };
        animate_forward_material(m,float(state.tick)*tick_seconds,function);
        std::array<ShaderVector,256> colors{};
        for(unsigned c=0;c<2;++c)colors[33+c]={q.colors[c*3],q.colors[c*3+1],q.colors[c*3+2],1};
        if(m.ui_emblem_pass)animate_material_bitmaps(m,float(state.tick)*tick_seconds,function);
        else {std::array<float,256> scalars{};scalars[40]=1;remap_pixel_externs(m.forward_program,scalars,colors,pass.pixel_externs,pass.fragments,m.shader_switches);}
        std::array<Vertex,4> corners{};
        // Original231064 receives UV bounds in left/right/top/bottom order.
        const std::array<Vec2,4> uv{{{q.uv[0],q.uv[2]},{q.uv[1],q.uv[2]},{q.uv[1],q.uv[3]},{q.uv[0],q.uv[3]}}};
        for(unsigned i=0;i<4;++i) {
            corners[i].position={q.positions[i*2]*640.f/width,q.positions[i*2+1]*480.f/height,1};
            corners[i].uv=uv[i];corners[i].color={1,1,1};corners[i].material=slot;
        }
        for(unsigned i:{0u,1u,2u,0u,2u,3u})vertices.push_back(corners[i]);
    }
    return vertices;
}
}
