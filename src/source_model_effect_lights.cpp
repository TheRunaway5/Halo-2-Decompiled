#include "halo2/source_model_effect_lights.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/world_material.hpp"
#include <map>
#include <set>
#include <tuple>
namespace halo2 {
struct SourceModelEffectLights::Impl {
    const Cache& cache;
    std::shared_ptr<const MenuWorldSourceSection> geometry;
    std::map<std::uint32_t,EffectLightDefinition> lights;
    using Key=std::tuple<std::uint32_t,std::uint32_t,unsigned,unsigned>;
    struct Slot {unsigned material{};Material authored;MenuWorldSourcePass pass;};
    std::map<Key,Slot> slots;
    std::vector<std::string> unavailable;
    Impl(const Cache& c,Mesh& mesh,std::uint32_t model,unsigned section,
        std::span<const std::uint32_t> ids):cache(c),
        geometry(std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(c,model,section))){
        for(auto id:ids)lights.emplace(id,load_effect_light(cache,id));
        std::set<std::uint32_t> shaders;
        for(const auto& part:geometry->parts())if(part.shader!=0xffffffffu)shaders.insert(part.shader);
        const auto read=[&](std::uint32_t at,std::size_t n){return Bytes(cache.read_tag_bytes(at,n));};
        for(auto shader:shaders){
            const auto owned=cache.load_source_tag_metadata(shader);
            if(owned.group()!="shad")throw FormatError("source model light material requires its shad owner");
            const auto root=read(owned.source_address(),0x28);
            const auto stem=cache.load_source_tag_metadata(read(root.u32(0x24),4).u32(0));
            const auto post=read(read(stem.source_address(),0x60).u32(0x5c),8);
            auto authored=cache.load_material(shader);
            for(unsigned lod=0;lod<post.u32(0);++lod)for(const auto& [id,definition]:lights)try{
                authored.selected_shader_lod=lod;
                EffectLightState state;state.size=state.brightness=1;
                const auto uniforms=source_effect_light_view_uniforms(definition,state,{}, {1,0,0},{0,0,1},{1,1,1,1,1},1,{});
                auto prepared=prepare_effect_light_diffuse_material(cache,authored,definition,uniforms);
                for(unsigned pass=0;pass<prepared.size();++pass){
                    const auto slot=unsigned(mesh.materials.size());mesh.materials.push_back(std::move(prepared[pass]));
                    slots.emplace(Key{id,shader,lod,pass},Slot{slot,authored,
                        load_menu_world_source_pass(cache,shader,{lod,12,pass,1})});
                }
            }catch(const FormatError& e){unavailable.push_back(std::to_string(id)+"/"+std::to_string(shader)+": "+e.what());}
        }
    }
};
SourceModelEffectLights::SourceModelEffectLights(const Cache& cache,Mesh& mesh,std::uint32_t model,unsigned section,
    std::span<const std::uint32_t> lights):impl_(std::make_unique<Impl>(cache,mesh,model,section,lights)){}
SourceModelEffectLights::~SourceModelEffectLights()=default;
const std::vector<std::string>& SourceModelEffectLights::unavailable()const{return impl_->unavailable;}
EffectLightRawDraw SourceModelEffectLights::draw(const SourceModelEffectLightDrawInput& in)const{
    const auto& i=*impl_;
    if(in.object==0xffffffffu)throw FormatError("source model light binding requires its actual object identity");
    const auto found=i.slots.find({in.light,in.shader,in.shader_lod,in.pass});
    if(found==i.slots.end())throw FormatError("selected model light pass was not admitted during preload");
    const auto& slot=found->second;
    auto completed=prepare_effect_light_diffuse_material(i.cache,slot.authored,i.lights.at(in.light),in.uniforms);
    if(in.pass>=completed.size())throw FormatError("selected model light pass disappeared");
    const auto constants=source_effect_light_raw_constants(in.constants,completed[in.pass]);
    const MenuWorldSourceOwner owner{MenuWorldSourceOwnerKind::dynamic_object,i.geometry->owner_datum(),
        in.object&65535u,in.object};
    auto draw=menu_world_source_draw(i.geometry,in.part,in.topology,in.palette,in.range);
    auto binding=menu_world_source_binding_class(i.cache,owner,std::move(draw),slot.pass.vertex_shader(),in.vertex_class,constants);
    return source_effect_light_raw_draw(slot.material,completed[in.pass],slot.pass,std::move(binding),in.raster,
        in.clip_range,in.fog_parameters);
}
}
