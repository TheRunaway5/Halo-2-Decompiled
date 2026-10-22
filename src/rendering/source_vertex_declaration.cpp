#include "halo2/source_vertex_inputs.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
SourceVertexDeclaration decode_source_vertex_declaration(
    const Bytes&b,std::size_t root,std::int64_t magic,unsigned classification) {
    b.require(root,12);
    if(b.u16(root)!=1)throw FormatError("vertex declaration is not original Xbox format");
    const auto classes=b.u32(root+4);
    if(classes>65536 || classification>=classes)
        throw FormatError("source vertex declaration class outside tag");
    const TagAddressSpace memory(b,magic);
    const auto at=memory.offset(b.u32(root+8),std::size_t(classes)*28)+classification*28;
    const auto count=b.u32(at+4);
    if(count>16)throw FormatError("source vertex declaration exceeds NV2A registers");
    SourceVertexDeclaration result;result.class_index=classification;
    if(count) {
        const auto semantics=memory.offset(b.u32(at+8),count*2);
        for(unsigned i=0;i<count;++i)result.semantics.push_back(b.u16(semantics+i*2));
    }
    return result;
}
SourceVertexDeclaration Cache::load_source_vertex_declaration(std::uint32_t datum,unsigned classification) const {
    const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::vertex_shader, .require_address=true});
    if(!tag)throw FormatError("source vertex declaration tag missing");
    const Bytes b(cache_bytes());const auto root=metadata_offset(tag->address,12);
    return decode_source_vertex_declaration(b,root,metadata_magic_,classification);
}
bool source_world_vertex_declaration_supported(const SourceVertexDeclaration&d) {
    if(d.class_index!=0 || d.semantics.empty() || d.semantics.size()>16)return false;
    for(const auto id:d.semantics)switch(SourceVertexSemantic(id)) {
    case SourceVertexSemantic::Position:case SourceVertexSemantic::UV:
    case SourceVertexSemantic::Normal:case SourceVertexSemantic::Tangent:
    case SourceVertexSemantic::Binormal:case SourceVertexSemantic::Incident:
    case SourceVertexSemantic::LightmapUV:case SourceVertexSemantic::Radiance:
    case SourceVertexSemantic::PrtCluster:case SourceVertexSemantic::PrtWeights:break;
    default:return false;
    }
    return true;
}
PackedSourceVertexDeclaration pack_source_vertex_declaration(
    const std::optional<SourceVertexDeclaration>&d,SourceWorldVertexInputTransform transform) {
    if(transform!=SourceWorldVertexInputTransform::Preserve &&
       transform!=SourceWorldVertexInputTransform::NormalizeSkinNormal &&
       transform!=SourceWorldVertexInputTransform::OffsetLocalPosition)
        throw FormatError("unsupported source world input transform");
    if(!d) {
        if(transform!=SourceWorldVertexInputTransform::Preserve)
            throw FormatError("source skin normal transform requires an authored declaration");
        return legacy_source_vertex_declaration;
    }
    if(!source_world_vertex_declaration_supported(*d))
        throw FormatError("unsupported source world vertex declaration");
    if(transform==SourceWorldVertexInputTransform::NormalizeSkinNormal &&
       std::find(d->semantics.begin(),d->semantics.end(),std::uint16_t(SourceVertexSemantic::Normal))==d->semantics.end())
        throw FormatError("source skin normal transform has no normal register");
    if(transform==SourceWorldVertexInputTransform::OffsetLocalPosition &&
       d->semantics!=std::vector<std::uint16_t>({0,4,5,6}))
        throw FormatError("source offset transform requires its proved world declaration");
    auto result=legacy_source_vertex_declaration;
    for(unsigned i=0;i<d->semantics.size();++i) {
        const auto shift=(i&3u)*8u;
        const auto modifier=(transform==SourceWorldVertexInputTransform::NormalizeSkinNormal &&
            d->semantics[i]==std::uint16_t(SourceVertexSemantic::Normal)) ||
            (transform==SourceWorldVertexInputTransform::OffsetLocalPosition &&
             d->semantics[i]==std::uint16_t(SourceVertexSemantic::Position))?0x40u:0u;
        const auto id=std::uint32_t(d->semantics[i])|modifier|(i==0?0x80u:0u);
        result[i/4]=(result[i/4]&~(0xffu<<shift))|(id<<shift);
    }
    return result;
}
std::array<ShaderVector,16> source_world_vertex_inputs(
    const SourceVertexDeclaration&d,const Vertex&v,
    const std::array<ShaderVector,16>&attributes,SourceWorldVertexInputTransform transform) {
    (void)pack_source_vertex_declaration(d,transform);
    auto result=attributes;
    const auto xyz=[](Vec3 value){return ShaderVector{value.x,value.y,value.z,1};};
    for(unsigned i=0;i<d.semantics.size();++i)switch(SourceVertexSemantic(d.semantics[i])) {
    case SourceVertexSemantic::Position:result[i]=xyz(v.position);break;
    case SourceVertexSemantic::UV:result[i]={v.uv.x,v.uv.y,0,1};break;
    case SourceVertexSemantic::Normal:{
        auto normal=v.normal;
        if(transform==SourceWorldVertexInputTransform::NormalizeSkinNormal) {
            const volatile float x=normal.x*normal.x,y=normal.y*normal.y,z=normal.z*normal.z;
            const volatile float xy=x+y;const float length=xy+z;
            // Source DP3/RSQ/MUL. NV multiplication defines 0*infinity as0.
            const float inverse=1.f/std::sqrt(std::abs(length));
            normal={normal.x==0?0:normal.x*inverse,normal.y==0?0:normal.y*inverse,
                    normal.z==0?0:normal.z*inverse};
        }
        result[i]=xyz(normal);break;
    }
    case SourceVertexSemantic::Tangent:result[i]=xyz(v.tangent);break;
    case SourceVertexSemantic::Binormal:result[i]=xyz(v.binormal);break;
    case SourceVertexSemantic::Incident:result[i]=xyz(v.incident);break;
    case SourceVertexSemantic::LightmapUV:result[i]={v.lightmap_uv.x,v.lightmap_uv.y,0,1};break;
    case SourceVertexSemantic::Radiance:result[i]=xyz(v.color);break;
    case SourceVertexSemantic::PrtCluster:result[i]={v.prt_cluster,0,0,1};break;
    case SourceVertexSemantic::PrtWeights:result[i]=v.prt_weights;break;
    }
    return result;
}
std::array<ShaderVector,16> source_immediate_color11_vertex_inputs(
    const SourceVertexDeclaration& declaration,const Vertex& vertex,
    const std::array<ShaderVector,16>& attributes) {
    auto result=source_world_vertex_inputs(declaration,vertex,attributes);
    result[11]={vertex.color.x,vertex.color.y,vertex.color.z,vertex.source_color_alpha};
    return result;
}
} // namespace halo2
