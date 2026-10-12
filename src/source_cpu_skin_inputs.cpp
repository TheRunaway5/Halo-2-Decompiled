#include "halo2/source_cpu_skin_inputs.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::uint64_t fingerprint(std::span<const std::byte> bytes) {
    std::uint64_t hash=14695981039346656037ull;
    for(auto byte:bytes)hash=(hash^std::to_integer<unsigned>(byte))*1099511628211ull;
    return hash;
}
}
SourceWorldVertexInputTransform source_cpu_skin_input_transform(
    std::span<const std::byte> world,std::span<const std::byte> source,unsigned classification) {
    if(classification>6)throw FormatError("source CPU skin compiled class is outside0..6");
    // Literal original transparent_generic_offset family. The posed position
    // and raw normal feed geometry; local decompressed position feeds only
    // texture outputs o9/o10.xyz. Class6 has its own four-influence program.
    if(world.size()==612 && fingerprint(world)==0xa08e72e4be20d7d3ull) {
        constexpr std::array<std::size_t,7> sizes{612,756,836,836,916,996,1076};
        constexpr std::array<std::uint64_t,7> hashes{
            0xa08e72e4be20d7d3ull,0xb5d7cb08eb54ec5full,0xb4689760c7bc2720ull,
            0xb4689760c7bc2720ull,0xd9a272d9725c770cull,0xa38932acd8deca86ull,
            0x35c56a0b5fa2f524ull};
        if(source.size()!=sizes[classification] || fingerprint(source)!=hashes[classification])
            throw FormatError("source CPU skin offset compiled variant is not proved");
        return classification?SourceWorldVertexInputTransform::OffsetLocalPosition:
            SourceWorldVertexInputTransform::Preserve;
    }
    // Literal original transparent-reflection payloads. Class0 lacks the
    // normal DP3/RSQ/MUL present in classes3..5. Identity is code-based;
    // shader names and the native draw category do not select this operation.
    if(world.size()!=676 || fingerprint(world)!=0x2a8e7898c5cfcc35ull)
        return SourceWorldVertexInputTransform::Preserve;
    constexpr std::array<std::size_t,7> sizes{676,804,884,916,996,1076,1076};
    constexpr std::array<std::uint64_t,7> hashes{
        0x2a8e7898c5cfcc35ull,0xb47680bc67be672dull,0xcc1cde1f22db7f16ull,
        0x1efa15f1fb49a95full,0x115d8f3760211a8bull,0x06d79a0a81d6ac27ull,
        0x06d79a0a81d6ac27ull};
    if(source.size()!=sizes[classification] || fingerprint(source)!=hashes[classification])
        throw FormatError("source CPU skin reflection compiled variant is not proved");
    if(classification==6)
        throw FormatError("source CPU skin reflection class6 aliases the three-influence program; four-influence adapter is unresolved");
    return classification>=3?SourceWorldVertexInputTransform::NormalizeSkinNormal:
        SourceWorldVertexInputTransform::Preserve;
}
bool prepare_cpu_skin_material_inputs(const Cache&cache,Material&material,unsigned classification) {
    if(classification>6)throw FormatError("source CPU skin material class is outside0..6");
    bool changed=false;
    if(material.forward_pass &&
       ((material.vertex_program.size()==676 && fingerprint(material.vertex_program)==0x2a8e7898c5cfcc35ull) ||
        (material.vertex_program.size()==612 && fingerprint(material.vertex_program)==0xa08e72e4be20d7d3ull))) {
        if(material.forward_pass_index>=material.passes.size())
            throw FormatError("source CPU skin material lost its selected pass");
        const auto&pass=material.passes[material.forward_pass_index];
        const auto tag=cache.query_tag({.group=tag_groups::vertex_shader, .name=pass.vertex_shader, .require_address=true});
        if(!tag || cache.load_vertex_shader_code(tag->datum,0)!=material.vertex_program)
            throw FormatError("source CPU skin material differs from its authored world program");
        material.world_vertex_input_transform=source_cpu_skin_input_transform(
            material.vertex_program,cache.load_vertex_shader_code(tag->datum,classification),classification);
        (void)pack_source_vertex_declaration(material.vertex_declaration,material.world_vertex_input_transform);
        changed=material.world_vertex_input_transform!=SourceWorldVertexInputTransform::Preserve;
    }
    const auto child=[&](std::shared_ptr<Material>&value){
        if(value){value=std::make_shared<Material>(*value);
            changed=prepare_cpu_skin_material_inputs(cache,*value,classification)||changed;}
    };
    child(material.environment_pass);
    for(auto&value:material.world_overlays_before)child(value);
    for(auto&value:material.world_overlays_after)child(value);
    return changed;
}
void apply_cpu_skin_texture_coordinates(SourceWorldVertexInputTransform transform,
    Vec3 position,std::span<const ShaderVector> constants,std::array<ShaderVector,16> &outputs) {
    if(transform!=SourceWorldVertexInputTransform::OffsetLocalPosition)return;
    if(constants.size()<24)throw FormatError("source offset noise constants missing");
    for(unsigned stage=0;stage<2;++stage)for(unsigned axis=0;axis<3;++axis) {
        const auto &row=constants[18+stage*3+axis];
        const volatile float x=position.x*row[0],y=position.y*row[1],z=position.z*row[2];
        const volatile float xy=x+y,xyz=xy+z;
        outputs[9+stage][axis]=xyz+row[3];
    }
}
}
