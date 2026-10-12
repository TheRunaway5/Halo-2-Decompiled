#include "halo2/source_prop_scene.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
std::optional<SourcePropLightingSphere> source_prop_physics_sphere(
    std::span<const SourcePropBodyBounds> bodies){
    if(bodies.empty())return {};
    Vec3 minimum{},maximum{};
    for(unsigned i=0;i<bodies.size();++i){
        auto low=bodies[i].minimum,high=bodies[i].maximum;
        if(bodies[i].margin!=0){
            low.x-=bodies[i].margin;low.y-=bodies[i].margin;low.z-=bodies[i].margin;
            high.x+=bodies[i].margin;high.y+=bodies[i].margin;high.z+=bodies[i].margin;
        }
        if(i==0){minimum=low;maximum=high;}
        else {
            //MINPS/MAXPS choose operand2 for equality/unordered operands.
            minimum={minimum.x<low.x?minimum.x:low.x,minimum.y<low.y?minimum.y:low.y,
                     minimum.z<low.z?minimum.z:low.z};
            maximum={maximum.x>high.x?maximum.x:high.x,maximum.y>high.y?maximum.y:high.y,
                     maximum.z>high.z?maximum.z:high.z};
        }
    }
    const Vec3 half{.5f*(maximum.x-minimum.x),.5f*(maximum.y-minimum.y),.5f*(maximum.z-minimum.z)};
    const Vec3 center{.5f*(minimum.x+maximum.x),.5f*(minimum.y+maximum.y),.5f*(minimum.z+maximum.z)};
    const float radius=std::sqrt(half.z*half.z+(half.y*half.y+half.x*half.x))+.001f;
    return SourcePropLightingSphere{center,radius};
}
SourcePropDefinition Cache::load_source_prop_definition(std::uint32_t datum)const {
    const Bytes data(storage_);
    const auto root=[&](std::uint32_t id,const char *group,std::size_t size){
        const auto tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
        if(!tag)throw FormatError("source prop tag missing");
        return metadata_offset(tag->address,size);
    };
    const auto p=root(datum,"bloc",0xc0);
    if(data.u16(p)!=11)throw FormatError("source prop object type mismatch");
    SourcePropDefinition result;
    result.name=query_tag({.handle=TagHandle{datum}})->name;
    result.object_flags=data.u16(p+2);result.model=data.u32(p+0x38);
    result.initial_prop_flags=(data.u32(p+0xbc)&1u)?2u:0u;
    if(result.model==0xffffffffu)return result;
    const auto model=root(result.model,"hlmt",0x28);
    result.render_model=data.u32(model+4);result.animation_graph=data.u32(model+0x14);
    result.physics_model=data.u32(model+0x24);
    if(result.physics_model!=0xffffffffu){
        const auto physics=root(result.physics_model,"phmo",0x40);
        result.physics_bodies=data.u32(physics+0x38);
        if(result.physics_bodies>65536)throw FormatError("source prop body count exceeds limit");
        bool all_fixed=true;
        if(result.physics_bodies){
            const auto bodies=metadata_offset(data.u32(physics+0x3c),std::size_t(result.physics_bodies)*0x90);
            for(unsigned i=0;i<result.physics_bodies;++i){
                const auto type=data.u16(bodies+i*0x90+0x1e);
                all_fixed&=type==1 || type==2;
            }
        }
        if(all_fixed)result.initial_prop_flags|=2u;
        result.constructor_admitted=result.physics_bodies!=0;
    }
    return result;
}
}
