#include "halo2/model.hpp"
#include "halo2/appearance.hpp"
#include "halo2/math.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/unit_animation.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include <algorithm>
#include <map>
#include <set>
namespace halo2 {
namespace {

int i16(const Bytes &b, std::size_t p) { return std::int16_t(b.u16(p)); }
unsigned u8(const Bytes &b, std::size_t p) {
  return std::to_integer<unsigned>(b.slice(p, 1)[0]);
}
float real(const Bytes &b, std::size_t p) {
  const auto f = b.f32(p);
  if (!std::isfinite(f) || std::abs(f) > 1e6f)
    throw FormatError("invalid model float");
  return f;
}
Vec3 point(const Bytes &b, std::size_t p) {
  return {real(b, p), real(b, p + 4), real(b, p + 8)};
}
std::array<float, 4> rotation(const Bytes &b, std::size_t p,
                              bool packed = false, bool normalize = true) {
  std::array<float, 4> q;
  float norm = 0;
  for (unsigned i = 0; i < 4; ++i) {
    q[i] = packed ? float(i16(b, p + i * 2)) / 32767.f : real(b, p + i * 4);
    norm += q[i] * q[i];
  }
  if (norm < 1e-12f)
    throw FormatError("zero model quaternion");
  if (normalize)
    for (auto &v : q)
      v /= std::sqrt(norm);
  return q;
}

} // namespace
std::array<float, 4> source_model_skin_weights(
    std::span<const std::uint8_t> stored, unsigned active) {
  if (active < 1 || active > 4 || active > stored.size())
    throw FormatError("invalid authored skin influence count");
  std::array<float, 4> weights{1, 0, 0, 0};
  for (unsigned j = 1; j < active; ++j)
    weights[j] = float(stored[j]) / 255.f;
  for (unsigned j = active - 1; j; --j)
    weights[0] -= weights[j];
  return weights;
}
unsigned source_model_vertex_class(unsigned mode,unsigned opaque,unsigned transparent,unsigned type) {
  if(mode>3 || opaque>4 || transparent>4)
    throw FormatError("source model vertex class metadata is invalid");
  return mode<3?mode:((type==4 || type==5)?transparent:opaque)+2;
}
std::array<Vec3,4> Cache::load_object_colors(std::uint32_t datum,
                                           std::uint32_t variant) const {
  const Bytes data(cache_bytes());
  const auto* t = find_tag({datum});
  if (!t || !t->address || t->group == "mode" || t->group == "hlmt" || t->size < 0xb4)
    throw FormatError("object change-color definition missing");
  const auto root = metadata_offset(t->address,t->size);
  if (!variant) variant = data.u32(root + 0x30);
  std::array<Vec3,4> colors{{{1,1,1},{1,1,1},{1,1,1},{1,1,1}}};
  const auto records = TagAddressSpace(data, metadata_magic_).block(root + 0xac, 16, 4);
  for (std::size_t i=0;i<records.count;++i) {
    const auto permutations = TagAddressSpace(data, metadata_magic_).block(records.offset+i*16, 32, 32);
    for (std::size_t j=0;j<permutations.count;++j) {
      const auto p = permutations.offset+j*32;
      const auto name = data.u32(p+28);
      if (real(data,p) <= 0 || (name && name != variant)) continue;
      // Asset previews use the first eligible lower bound. Live constructor
      // colors use source_object_initial_colors with the actual position and
      // selected hlmt variant; BE240 does not consume the primary RNG.
      colors[i] = point(data,p+4);
      break;
    }
  }
  return colors;
}
RenderModel Cache::load_model(std::uint32_t datum, bool textures,
                              std::uint32_t variant) const {
  return load_model_with_initial_selection(datum,textures,variant,nullptr);
}
RenderModel Cache::load_model_hierarchy(std::uint32_t datum) const {
  const Bytes data(cache_bytes());
  const auto find=[&](std::uint32_t id)->const Tag& {
    const auto *tag=find_tag({id});
    if(!tag || !tag->address)throw FormatError("Cinematic model hierarchy tag is unavailable");
    return *tag;
  };
  const Tag *tag=&find(datum);RenderModel out;
  if(tag->group!="mode" && tag->group!="hlmt")
    tag=&find(data.u32(metadata_offset(tag->address,0x3c)+0x38));
  if(tag->group=="hlmt") {
    const auto root=metadata_offset(tag->address,0x18);
    out.animation=data.u32(root+0x14);tag=&find(data.u32(root+4));
  }
  if(tag->group!="mode")throw FormatError("Cinematic object has no render-model hierarchy");
  const auto root=metadata_offset(tag->address,0x50);
  out.name=tag->name;out.checksum=data.u32(root+8);
  out.compound_nodes=load_source_model_palette_definition(tag->datum).compounds[5];
  const auto nodes=TagAddressSpace(data,metadata_magic_).block(root+0x48,0x60,255);
  for(std::size_t i=0;i<nodes.count;++i) {
    const auto at=nodes.offset+i*0x60;ModelBone bone;
    bone.name=data.u32(at);bone.parent=i16(data,at+4);
    if(bone.parent>=int(nodes.count) || bone.parent==int(i))throw FormatError("Invalid cinematic model parent");
    bone.bind.translation=point(data,at+12);
    bone.bind.rotation=rotation(data,at+24,false,false);
    bone.inverse_scale=real(data,at+40);
    for(unsigned n=0;n<12;++n)bone.inverse[n]=real(data,at+44+n*4);
    out.bones.push_back(bone);
  }
  return out;
}
RenderModel Cache::load_model(std::uint32_t datum, bool textures,
                              std::uint32_t variant,
                              const SourceInitialModelSelection& selected) const {
  if (!selected.resolved())
    throw FormatError("model geometry requires resolved source active regions");
  return load_model_with_initial_selection(datum,textures,variant,&selected);
}
RenderModel Cache::load_model(std::uint32_t datum, bool textures,
                              std::uint32_t variant,
                              const SourceInitialModelSelection& selected,
                              unsigned geometry_lod) const {
  if (!selected.resolved() || geometry_lod > 5)
    throw FormatError("model geometry requires resolved source regions and LOD0..5");
  return load_model_with_initial_selection(datum,textures,variant,&selected,geometry_lod);
}
RenderModel Cache::load_model_sections(std::uint32_t datum,bool textures,std::uint32_t variant,
    unsigned geometry_lod,std::span<const unsigned> sections)const{
  if(geometry_lod>5)throw FormatError("resident model section bank requires LOD0..5");
  return load_model_with_initial_selection(datum,textures,variant,nullptr,geometry_lod,sections);
}
std::uint32_t Cache::load_model_animation_graph(std::uint32_t datum) const {
  const Bytes data(cache_bytes());
  const auto find=[&](std::uint32_t id) {
    const auto *tag=find_tag({id});
    if(!tag||!tag->address)throw FormatError("missing render model tag");
    return tag;
  };
  const auto *tag=find(datum);
  if(tag->group!="mode"&&tag->group!="hlmt")
    tag=find(data.u32(metadata_offset(tag->address,0x3c)+0x38));
  return tag->group=="hlmt"?data.u32(metadata_offset(tag->address,0x58)+0x14):0xffffffffu;
}
RenderModel Cache::load_model_with_initial_selection(std::uint32_t datum, bool textures,
                              std::uint32_t variant,
                              const SourceInitialModelSelection* supplied_selection,
                              std::optional<unsigned> geometry_lod,
                              std::optional<std::span<const unsigned>> resident_sections) const {
  const Bytes data(cache_bytes());
  auto find = [&](std::uint32_t id) -> const Tag & {
    const auto* t = find_tag({id});
    if (!t || !t->address)
      throw FormatError("missing render model tag");
    return *t;
  };
  const Tag *tag = &find(datum);
  RenderModel out;
  const bool object = tag->group != "mode" && tag->group != "hlmt";
  std::uint32_t variant_name = variant;
  std::map<std::uint32_t, int> overrides;
  std::optional<SourceInitialModelSelection> initial_selection;
  std::vector<unsigned> selected_sections;
  if (supplied_selection && tag->group == "mode")
    throw FormatError("render model has no source object active-region definition");
  if (tag->group != "mode" && tag->group != "hlmt") {
    const auto p = metadata_offset(tag->address, 0x3c);
    if (!variant_name)
      variant_name = data.u32(p + 0x30);
    tag = &find(data.u32(p + 0x38));
  }
  if (tag->group == "hlmt") {
    const auto p = metadata_offset(tag->address, 0x58);
    out.animation = data.u32(p + 0x14);
    const auto definition = load_initial_model_definition(tag->datum);
    if (supplied_selection) {
      if (supplied_selection->variant_index != source_initial_model_variant(definition,variant_name) ||
          supplied_selection->permutations.size() != definition.regions.size() ||
          supplied_selection->states.size() != definition.regions.size())
        throw FormatError("source active regions do not match the model definition");
      // Validate the authored region/permutation bounds before decoding streams.
      const auto resident = source_initial_model_sections(
          definition,*supplied_selection,geometry_lod.value_or(5));
      if (geometry_lod) selected_sections = resident;
      initial_selection = *supplied_selection;
    } else {
      initial_selection = source_deterministic_initial_model_selection(definition,variant_name);
    }
    const auto variants = TagAddressSpace(data, metadata_magic_).block(p + 0x50, 0x38);
    for (std::size_t i = 0; i < variants.count; ++i) {
      const auto v = variants.offset + i * 0x38;
      if (int(i) != initial_selection->variant_index)
        continue;
      const auto regions = TagAddressSpace(data, metadata_magic_).block(v + 0x14, 0x14);
      for (std::size_t j = 0; j < regions.count; ++j) {
        const auto r = regions.offset + j * 0x14;
        const auto permutations = TagAddressSpace(data, metadata_magic_).block(r + 8, 0x20);
        // Only unresolved RNG regions retain the asset preview's first
        // positive-weight alternative below. Deterministic source16D280
        // choices, including absent regions, override this preview later.
        for (std::size_t k = 0; k < permutations.count; ++k) {
          const auto q = permutations.offset + k * 0x20;
          if (real(data, q + 8) > 0 || permutations.count == 1) {
            overrides[data.u32(r)] = std::int8_t(u8(data, q + 4));
            break;
          }
        }
      }
      break;
    }
    for(const auto region:initial_selection->unresolved_rng_regions)
      out.mesh.material_warnings.push_back("initial model region "+std::to_string(region)+
          " requires the original object RNG stream; geometry is an unresolved asset preview");
    tag = &find(data.u32(p + 4));
  }
  if (tag->group != "mode")
    throw FormatError("object does not reference a render model");
  out.name = tag->name;
  const auto root = metadata_offset(tag->address, 0x68);
  out.checksum = data.u32(root + 8);
  out.compound_nodes=load_source_model_palette_definition(tag->datum).compounds[geometry_lod.value_or(5)];
  const auto bounds = TagAddressSpace(data, metadata_magic_).block(root + 0x14, 0x38, 64),
             regions = TagAddressSpace(data, metadata_magic_).block(root + 0x1c, 16),
             sections = TagAddressSpace(data, metadata_magic_).block(root + 0x24, 0x5c),
             nodes = TagAddressSpace(data, metadata_magic_).block(root + 0x48, 0x60, 255),
             materials = TagAddressSpace(data, metadata_magic_).block(root + 0x60, 32);
  if (!bounds.count)
    throw FormatError("model has no compression bounds");
  std::array<float, 10> limits{};
  for (unsigned i = 0; i < 10; ++i)
    limits[i] = real(data, bounds.offset + i * 4);
  auto expand_normalized = [&](float normalized, unsigned axis) {
    // Xbox1CDD0 and Vista66F551 upload c170..172 as half range and
    // midpoint; both original vertex programs multiply before adding.
    const float low = limits[axis * 2], high = limits[axis * 2 + 1];
    const float scale = (high - low) * .5f;
    const float offset = (low + high) * .5f;
    // Preserve the separately rounded MUL/ADD even on hosts with FMA.
    const volatile float product = normalized * scale;
    return product + offset;
  };
  auto decompress = [&](int value, unsigned axis) {
    // Xbox1C330/35790 select normalized SHORT inputs; both negative
    // endpoints read -1. Vista's declared FLOAT inputs need no such clamp.
    return expand_normalized(std::max(-1.f, float(value) / 32767.f),axis);
  };
  for (std::size_t i = 0; i < nodes.count; ++i) {
    const auto p = nodes.offset + i * 0x60;
    ModelBone bone;
    bone.name = data.u32(p);
    bone.parent = i16(data, p + 4);
    if (bone.parent >= int(nodes.count) || bone.parent == int(i))
      throw FormatError("invalid model bone parent");
    bone.bind.translation = point(data, p + 12);
    //16D940 copies authored bind quaternion words without a prior normalize.
    bone.bind.rotation = rotation(data, p + 24, false, false);
    bone.inverse_scale = real(data, p + 40);
    for (unsigned j = 0; j < 12; ++j)
      bone.inverse[j] = real(data, p + 44 + j * 4);
    out.bones.push_back(bone);
  }
  out.mesh.materials.resize(materials.count + 1);
  if (textures)
    for (std::size_t i = 0; i < materials.count; ++i) {
      try {
        out.mesh.materials[i] =
            load_material(data.u32(materials.offset + i * 32 + 12));
        prepare_model_environment(*this,out.mesh.materials[i]);
      } catch (const FormatError &e) {
        out.mesh.material_warnings.push_back(std::to_string(i) + ": " +
                                             e.what());
      }
    }
  if (textures && object) {
    const auto colors = load_object_colors(datum,variant);
    for (auto &material : out.mesh.materials) apply_object_colors(material,colors);
  }
  // The preview path historically collapses duplicate sections and falls
  // through authored LODs. An explicit resident source collector does neither.
  std::set<unsigned> preview_sections;
  if (!geometry_lod) for (std::size_t i = 0; i < regions.count; ++i) {
    const auto permutations =
        TagAddressSpace(data, metadata_magic_).block(regions.offset + i * 16 + 8, 16);
    if (!permutations.count)
      continue;
    int permutation = 0;
    if (const auto it = overrides.find(data.u32(regions.offset + i * 16));
        it != overrides.end())
      permutation = it->second;
    //4C2B0 indexes object active bytes and render regions by the same ordinal.
    //A proven NONE here means absent geometry, not the preview permutation0.
    if(initial_selection && i<initial_selection->permutations.size() &&
       std::find(initial_selection->unresolved_rng_regions.begin(),
                 initial_selection->unresolved_rng_regions.end(),unsigned(i))==
                 initial_selection->unresolved_rng_regions.end())
      permutation=initial_selection->permutations[i]==255?-1:
          int(initial_selection->permutations[i]);
    if (permutation < 0)
      continue;
    if (std::size_t(permutation) >= permutations.count)
      throw FormatError("variant permutation outside model region");
    // Highest authored LOD, falling back through the remaining authored LODs.
    for (int lod = 14; lod >= 4; lod -= 2) {
      const auto section =
          i16(data, permutations.offset + std::size_t(permutation) * 16 + lod);
      if (section < 0)
        continue;
      if (std::size_t(section) >= sections.count)
        throw FormatError("model permutation section outside block");
      preview_sections.insert(unsigned(section));
      break;
    }
  }
  if (!geometry_lod)
    selected_sections.assign(preview_sections.begin(),preview_sections.end());
  if(resident_sections)selected_sections.assign(resident_sections->begin(),resident_sections->end());
  for (auto section : selected_sections) {
    if (section >= sections.count)
      throw FormatError("model permutation section outside block");
    const auto s = sections.offset + section * 0x5c;
    const auto classification = i16(data, s);
    const auto count = data.u16(s + 4), faces = data.u16(s + 6);
    const auto opaque_influences = u8(data, s + 0x14);
    const auto transparent_influences = u8(data, s + 0x15);
    // The packed stream retains its maximum-sized bone/weight fields. 1CAA0
    // independently chooses the shader's active count for each part below.
    const auto influences = std::max(opaque_influences, transparent_influences);
    if (!count)
      continue;
    if (classification < 0 || classification > 3 || influences > 4)
      throw FormatError("unsupported model vertex classification");
    const auto raw = raw_data(data.u32(s + 0x38), data.u32(s + 0x3c));
    const Bytes b(raw);
    const auto header = data.u32(s + 0x44);
    if (std::uint64_t(header) + 4 > raw.size())
      throw FormatError("invalid model raw header size");
    const auto base = raw.size() - header - 4;
    const auto resources = TagAddressSpace(data, metadata_magic_).block(s + 0x48, 16, 128);
    struct Resource {
      std::size_t offset{}, size{};
      bool found{};
    } parts, indices, positions, uvs, basis, incident, radiance, buffers, nodemap;
    for (std::size_t i = 0; i < resources.count; ++i) {
      const auto p = resources.offset + i * 16;
      Resource r{base + data.u32(p + 12), data.u32(p + 8), true};
      b.require(r.offset, r.size);
      const auto type = data.u16(p + 4), channel = data.u16(p + 6);
      if (i == 0)
        parts = r;
      if (type == 32)
        indices = r;
      if (type == 56 && channel == 0)
        positions = r;
      if (type == 56 && channel == 1)
        uvs = r;
      if (type == 56 && channel == 2)
        basis = r;
      if (type == 56 && channel == 3)
        incident = r;
      if (type == 56 && channel == 4)
        radiance = r;
      if (type == 56 && channel == 32)
        buffers = r;
      if (type == (mcc_?104:100))
        nodemap = r;
    }
    const auto index_count = b.u16(0x28), map_count = b.u16(0x6c);
    if (!parts.found || parts.size % 72 || !indices.found ||
        indices.size < std::size_t(index_count) * 2 || !positions.found ||
        positions.size % count || !uvs.found ||
        uvs.size < std::size_t(count) * (pc_geometry()?8:4) ||
        (map_count && (!nodemap.found || nodemap.size < map_count)))
      throw FormatError("invalid model resource layout");
    const auto stride = positions.size / count;
    if (stride < 6 || stride > 64)
      throw FormatError("invalid model vertex stride");
    if (!basis.found || basis.size != std::size_t(count) * (pc_geometry()?36:12))
      throw FormatError("invalid model tangent basis stream");
    if(pc_geometry()) {
      auto declared=[&](unsigned channel,unsigned type,unsigned bytes) {
        if(!buffers.found || buffers.size<(channel+1)*32 ||
           u8(b,buffers.offset+channel*32)!=type || u8(b,buffers.offset+channel*32+1)!=bytes)
          throw FormatError("unsupported Vista model vertex declaration");
      };
      if(!buffers.found || buffers.size<96)
        throw FormatError("missing Vista model stream descriptors");
      const auto format=u8(b,buffers.offset);
      // Original Vista 66E828 emits FLOAT3 at0, UBYTE4 bone indices at12,
      // and UBYTE4 weights at16. Formats4..9 retain all four slots even
      // when the authored part selects fewer active influences.
      if(((classification==0 || classification==1) && (format!=1 || stride!=12)) ||
         (classification==2 && ((format!=2 && format!=3) || stride!=16)) ||
         (classification==3 && !((format>=4 && format<=9 && stride==20) ||
                                 ((format==2 || format==3) && stride==16 && influences==1))))
        throw FormatError("unsupported Vista model position declaration");
      declared(0,format,unsigned(stride));declared(1,23,8);declared(2,25,36);
      if(incident.found)declared(3,47,12);
      if(radiance.found)declared(4,46,4);
    }
    // 1C8C0 reads these literal 32-byte stream descriptors and supplies
    // 43F408 + type*21 to 1C330. Type48 is semantic8 / NORMPACKED3;
    // type47 is semantic14 / PBYTE3. PRT-lightmap's class0 declaration
    // places radiance before its cluster/weights, unlike plain PRT.
    if (!pc_geometry() && incident.found &&
        (!buffers.found || buffers.size < 4 * 32 ||
         u8(b, buffers.offset + 3 * 32) != 48 ||
         u8(b, buffers.offset + 3 * 32 + 1) != 4 ||
         incident.size != std::size_t(count) * 4))
      throw FormatError("unsupported model incident stream declaration");
    if (!pc_geometry() && radiance.found &&
        (!buffers.found || buffers.size < 5 * 32 ||
         u8(b, buffers.offset + 4 * 32) != 47 ||
         u8(b, buffers.offset + 4 * 32 + 1) != 3 ||
         radiance.size != std::size_t(count) * 3))
      throw FormatError("unsupported model radiance stream declaration");
    if(pc_geometry() && ((incident.found && incident.size!=std::size_t(count)*12) ||
                  (radiance.found && radiance.size!=std::size_t(count)*4)))
      throw FormatError("invalid Vista model auxiliary lighting stream size");
    // TU1.5 type1 at4494DD binds POSITION format2 (FLOAT3/0x32),
    // size12 via35780/35840. Reach's authored XYZ compression bounds are
    // zero: these literal float positions must not use the SHORT expansion.
    const bool xbox_float_positions = !pc_geometry() && buffers.found &&
        buffers.size >= 32 && u8(b, buffers.offset) == 1;
    if (xbox_float_positions &&
        (u8(b, buffers.offset + 1) != 12 || stride != 12 || classification > 1))
      throw FormatError("unsupported Xbox FLOAT3 model position declaration");
    std::vector<Vertex> vertices(count);
    std::vector<SkinBinding> bindings(count);
    for (std::size_t i = 0; i < count; ++i) {
      const auto p = positions.offset + i * stride;
      vertices[i].color={1,1,1};
      if(pc_geometry()) {
        vertices[i].position=point(b,p);
        vertices[i].uv={real(b,uvs.offset+i*8),real(b,uvs.offset+i*8+4)};
        // Vista6809C4 passes section+1A to66F551; its optional OR1 cannot
        // change an authored3. The actual class2 progress-symbol shader
        // expands these FLOAT inputs using c170..172, just like the packed
        // Xbox input. They are normalized values, not final coordinates.
        // Other classes/flag combinations retain their independent boundary.
        if(classification==2 && (data.u16(s+0x1a)&3)==3) {
          const auto input=vertices[i].position;
          vertices[i].position={expand_normalized(input.x,0),
              expand_normalized(input.y,1),expand_normalized(input.z,2)};
          vertices[i].uv={expand_normalized(vertices[i].uv.x,3),
              expand_normalized(vertices[i].uv.y,4)};
        }
        vertices[i].normal=point(b,basis.offset+i*36);
        vertices[i].binormal=point(b,basis.offset+i*36+12);
        vertices[i].tangent=point(b,basis.offset+i*36+24);
      } else {
        vertices[i].position=xbox_float_positions?point(b,p):
            Vec3{decompress(i16(b,p),0),decompress(i16(b,p+2),1),decompress(i16(b,p+4),2)};
        // 1CDB0 leaves XYZ identity when bit1 is clear (Reach flags2).
        // FLOAT3 normalized inputs still use the authored expansion if set.
        if(xbox_float_positions && (data.u16(s+0x1a)&1)) {
          const auto input=vertices[i].position;
          vertices[i].position={expand_normalized(input.x,0),
              expand_normalized(input.y,1),expand_normalized(input.z,2)};
        }
        vertices[i].uv={decompress(i16(b,uvs.offset+i*4),3),decompress(i16(b,uvs.offset+i*4+2),4)};
        vertices[i].normal = unpack_normal(b.u32(basis.offset + i * 12));
        vertices[i].tangent = unpack_normal(b.u32(basis.offset + i * 12 + 4));
        vertices[i].binormal = unpack_normal(b.u32(basis.offset + i * 12 + 8));
      }
      vertices[i].source_local_position=vertices[i].position;
      if (incident.found)
        vertices[i].incident = pc_geometry()?point(b,incident.offset+i*12):unpack_normal(b.u32(incident.offset + i * 4));
      // Original35790 format6 returns0x34; 3F7440 uploads it literally
      // as NV097 UB_OGL/count3 (0x334 with stride3). This is normalized
      // byte input, independent of CPU packed lightmap RGB/HDR conversions.
      if (radiance.found) {
        const auto at=radiance.offset+i*(pc_geometry()?4:3);
        vertices[i].color = {float(u8(b,at))/255.f,float(u8(b,at+1))/255.f,float(u8(b,at+2))/255.f};
      }
      auto &skin = bindings[i];
      skin.weights = {0, 0, 0, 0};
      auto mapped = [&](unsigned local) -> std::uint16_t {
        const auto bone =
            map_count
                ? (local < map_count ? u8(b, nodemap.offset + local) : 65535u)
                : local;
        if (bone >= nodes.count+out.compound_nodes.size())
          throw FormatError("model skin bone outside skeleton: "+out.name+" bone="+std::to_string(bone)+" count="+std::to_string(out.bones.size()));
        return std::uint16_t(bone);
      };
      if (classification == 0 || nodes.count == 0)
        skin.weights = {0, 0, 0, 0};
      else if (classification == 1) {
        skin.bones[0] = mapped(0);
        skin.weights[0] = 1;
      } else if (classification == 2) {
        if (stride < (pc_geometry()?16:8))
          throw FormatError("truncated rigid-boned vertex");
        skin.bones[0] = mapped(u8(b, p + (pc_geometry()?12:6)));
        skin.weights[0] = 1;
      }
    }
    bool list = std::size_t(faces) * 3 == index_count;
    if (list)
      for (std::size_t i = 0; i + 2 < std::min<std::size_t>(index_count, 30);
           i += 3) {
        const auto a = b.u16(indices.offset + i * 2),
                   c = b.u16(indices.offset + (i + 1) * 2),
                   d = b.u16(indices.offset + (i + 2) * 2);
        if (a == c || c == d || a == d) {
          list = false;
          break;
        }
      }
    for (std::size_t i = 0; i < parts.size / 72; ++i) {
      const auto p = parts.offset + i * 72;
      const auto part_type = b.u16(p);
      // Original 1CAA0 selects section+15 for types4/5, section+14 otherwise.
      // A mixed section (Elite transparent parts, FP beam rifle) must not
      // consume all the opaque influences just because the stream stores them.
      const auto active_influences = part_type == 4 || part_type == 5
                                         ? transparent_influences
                                         : opaque_influences;
      const auto shader = b.u16(p + 4), first = b.u16(p + 6),
                 length = b.u16(p + 8);
      if (first > index_count || length > index_count - first ||
          (list && length % 3))
        throw FormatError("model part index range outside buffer");
      for (std::size_t j = 0; j + 2 < length; j += list ? 3 : 1) {
        std::array<unsigned, 3> triangle{
            b.u16(indices.offset + (first + j) * 2),
            b.u16(indices.offset + (first + j + 1) * 2),
            b.u16(indices.offset + (first + j + 2) * 2)};
        if (triangle[0] >= count || triangle[1] >= count ||
            triangle[2] >= count)
          throw FormatError("model triangle outside vertices");
        if (triangle[0] == triangle[1] || triangle[0] == triangle[2] ||
            triangle[1] == triangle[2])
          continue;
        if (!list && j % 2)
          std::swap(triangle[0], triangle[1]);
        for (auto v : triangle) {
          auto vertex = vertices[v];
          auto skin = bindings[v];
          if (classification == 3 && nodes.count) {
            const auto start = pc_geometry()?12:6 + ((influences == 2 || influences == 4) ? 2u : 0u);
            if (active_influences == 0 || active_influences > influences ||
                start + 2 * influences > stride)
              throw FormatError("invalid authored skinned part influence count");
            const auto position = positions.offset + v * stride;
            std::array<std::uint8_t, 4> stored{};
            for (unsigned j = 0; j < active_influences; ++j) {
              const auto local = u8(b, position + start + j);
              const auto bone = map_count
                                    ? (local < map_count ? u8(b, nodemap.offset + local) : 65535u)
                                    : local;
              if (bone >= nodes.count+out.compound_nodes.size())
                throw FormatError("model skin bone outside skeleton: "+out.name+" bone="+std::to_string(bone)+" count="+std::to_string(out.bones.size()));
              skin.bones[j] = std::uint16_t(bone);
              stored[j] = pc_geometry() && stride==16?0:std::uint8_t(u8(b, position + start + (pc_geometry()?4:influences) + j));
            }
            // Compiled source classes3..6 derive x as 1-w, 1-z-y,
            // or 1-w-z-y. They ignore the stored x weight and do not normalize.
            skin.weights = source_model_skin_weights(stored, active_influences);
          }
          vertex.material = shader < materials.count
                                ? shader
                                : std::uint32_t(materials.count);
          vertex.source_section=std::uint32_t(section);
          vertex.source_part=std::uint32_t(i);
          vertex.source_part_type=part_type;vertex.source_part_flags=b.u16(p+2);
          out.mesh.vertices.push_back(vertex);
          out.skin.push_back(skin);
          out.vertex_origins.push_back({std::uint16_t(section),std::uint16_t(v),std::uint16_t(i),
              std::uint8_t(source_model_vertex_class(unsigned(classification),opaque_influences,
                                                     transparent_influences,part_type))});
          if (out.mesh.vertices.size() > 3000000)
            throw FormatError("model expansion exceeds limit");
        }
      }
    }
  }
  if (out.mesh.vertices.empty() && geometry_lod)
    return out;
  if (out.mesh.vertices.empty())
    throw FormatError("model has no selected render geometry");
  out.mesh.minimum = out.mesh.maximum = out.mesh.vertices.front().position;
  for (const auto &v : out.mesh.vertices) {
    out.mesh.minimum = minimum(out.mesh.minimum, v.position);
    out.mesh.maximum = maximum(out.mesh.maximum, v.position);
  }
  return out;
}
std::uint32_t Cache::animation_name(std::uint32_t datum,
                                    const std::string &name) const {
  const auto id = find_animation_name(datum, name);
  if (!id)
    throw FormatError("named animation missing: " + name);
  return *id;
}
std::optional<std::uint32_t> Cache::find_animation_name(
    std::uint32_t datum, const std::string &name) const {
  const auto* tag = find_tag({datum}, tag_groups::model_animation_graph);
  if (!tag || !tag->address)
    throw FormatError("named animation graph missing");
  const Bytes data(cache_bytes());
  const auto root = metadata_offset(tag->address, 0x34);
  const auto stride=vista_?0x60u:0x6cu;
  const auto animations = TagAddressSpace(data, metadata_magic_).block(root + 0x2c, stride);
  for (std::size_t i = 0; i < animations.count; ++i) {
    const auto id = data.u32(animations.offset + i * stride);
    if (string_id(id) == name)
      return id;
  }
  return {};
}
AnimationClip Cache::load_named_animation(std::uint32_t datum,
                                          std::uint32_t name) const {
  if(const auto index=find_local_animation_index(datum,name))return load_animation(datum,*index);
  throw FormatError("named animation missing: " + string_id(name));
}
std::optional<unsigned> Cache::find_local_animation_index(std::uint32_t datum,std::uint32_t name) const {
  const auto* tag = find_tag({datum}, tag_groups::model_animation_graph);
  if (!tag || !tag->address)
    throw FormatError("named animation graph missing");
  const Bytes data(cache_bytes());
  const auto root = metadata_offset(tag->address, 0x34);
  const auto stride=vista_?0x60u:0x6cu;
  const auto animations = TagAddressSpace(data, metadata_magic_).block(root + 0x2c, stride);
  for (std::size_t i = 0; i < animations.count; ++i)
    if (data.u32(animations.offset + i * stride) == name)
      return unsigned(i);
  return {};
}
UnitAnimationGraphMetadata Cache::load_source_unit_animation_graph_metadata(
    std::uint32_t graph) const {
  if(vista_)throw FormatError("Vista animation graph requires verified Xbox unit-animation metadata conversion");
  const auto* tag = find_tag({graph}, tag_groups::model_animation_graph);
  if (!tag || !tag->address || tag->size < 0xc4)
    throw FormatError("source unit animation graph metadata is unavailable");
  const auto root = metadata_offset(tag->address, tag->size);
  return {graph, tag->address, Bytes(cache_bytes()).slice(root, tag->size)};
}
UnitAnimationDefinition Cache::load_source_unit_animation_definition(
    std::uint32_t graph, std::uint32_t packed) const {
  const Bytes data(cache_bytes());
  auto root_for = [&](std::uint32_t datum) {
    const auto* tag = find_tag({datum}, tag_groups::model_animation_graph);
    if (!tag || !tag->address)
      throw FormatError("source unit animation graph is unavailable");
    return metadata_offset(tag->address, vista_?0xac:0xb4);
  };
  auto root = root_for(graph);
  // 1CA5A0 and 1DE890 resolve the signed low short once, through the
  // original graph+4C/+50 32-byte inherited records. It is not a clip ID.
  const auto inherited = std::int16_t(packed & 0xffffu);
  if (inherited != -1) {
    const auto parents = TagAddressSpace(data, metadata_magic_).block(root + 0x4c, 32);
    if (inherited < 0 || std::size_t(inherited) >= parents.count)
      throw FormatError("source inherited animation graph is unavailable");
    root = root_for(data.u32(parents.offset + std::size_t(inherited) * 32 + 4));
  }
  const auto index = std::int16_t(packed >> 16);
  const auto stride=vista_?0x60u:0x6cu,shift=vista_?12u:0u;
  const auto clips = TagAddressSpace(data, metadata_magic_).block(root + 0x2c, stride);
  if (index < 0 || std::size_t(index) >= clips.count)
    throw FormatError("source unit animation definition is unavailable");
  const auto p = clips.offset + std::size_t(index) * stride;
  UnitAnimationDefinition out;
  out.frame_count = std::int16_t(data.u16(p + 0x14));
  out.loop_frame = std::int16_t(data.u16(p + (vista_?0x20:0x2e)));
  out.authored_flags = std::uint8_t(u8(data, p + 0x18));
  const auto timing = TagAddressSpace(data, metadata_magic_).block(p + 0x4c-shift, 4);
  for (std::size_t i = 0; i < timing.count; ++i) {
    const auto e = timing.offset + i * 4;
    out.timing.push_back({std::int16_t(data.u16(e)),
                          std::int16_t(data.u16(e + 2))});
  }
  const auto sounds = TagAddressSpace(data, metadata_magic_).block(p + 0x54-shift, 8);
  for (std::size_t i = 0; i < sounds.count; ++i) {
    const auto e = sounds.offset + i * 8;
    out.sounds.push_back({std::int16_t(data.u16(e)),
                          std::int16_t(data.u16(e + 2)), data.u32(e + 4)});
  }
  const auto effects = TagAddressSpace(data, metadata_magic_).block(p + 0x5c-shift, 4);
  for (std::size_t i = 0; i < effects.count; ++i) {
    const auto e = effects.offset + i * 4;
    out.effects.push_back({std::int16_t(data.u16(e)),
                           std::int16_t(data.u16(e + 2))});
  }
  // 1CAE40 reads the reference arrays from the resolved graph, in authored
  // order, including NONE references and every suppression/direction flag.
  auto references = [&](std::size_t offset, auto &destination) {
    const auto records = TagAddressSpace(data, metadata_magic_).block(root + offset, 12);
    for (std::size_t i = 0; i < records.count; ++i) {
      const auto r = records.offset + i * 12;
      destination.push_back({data.u32(r + 4), data.u16(r + 8)});
    }
  };
  references(0x14, out.sound_references);
  references(0x1c, out.effect_references);
  return out;
}
AnimationClip Cache::load_animation_identifier(std::uint32_t graph,std::uint32_t packed) const {
  const auto index=std::bit_cast<std::int16_t>(std::uint16_t(packed>>16));
  if(index<0)throw FormatError("Packed animation has no valid original clip");
  const auto inheritance=std::bit_cast<std::int16_t>(std::uint16_t(packed));
  if(inheritance!=-1) {
    const auto *tag=find_tag({graph},tag_groups::model_animation_graph);
    if(!tag || !tag->address)throw FormatError("Packed animation manager graph is missing");
    const Bytes data(cache_bytes());const auto root=metadata_offset(tag->address,0x54);
    const auto graphs=TagAddressSpace(data,metadata_magic_).block(root+0x4c,32,64);
    if(inheritance<0 || std::size_t(inheritance)>=graphs.count)
      throw FormatError("Packed animation inherited graph outside list");
    graph=data.u32(graphs.offset+std::size_t(inheritance)*32+4);
  }
  auto clip=load_animation(graph,unsigned(index));
  clip.source_identifier=packed;
  return clip;
}
AnimationClip Cache::load_animation(std::uint32_t datum,
                                    unsigned animation) const {
  return load_animation(datum,animation,[this](auto pointer,auto bytes){return raw_data(pointer,bytes);});
}
AnimationClip Cache::load_animation(std::uint32_t datum,unsigned animation,
                                    const AnimationResourceReader &resources) const {
  const Bytes data(cache_bytes());
  const auto* tag = find_tag({datum}, tag_groups::model_animation_graph);
  if (!tag || !tag->address)
    throw FormatError("missing animation graph");
  const auto root = metadata_offset(tag->address, vista_?0xac:0xb4);
  const auto stride=vista_?0x60u:0x6cu,shift=vista_?12u:0u;
  const auto nodes = TagAddressSpace(data, metadata_magic_).block(root + 0xc, 32, 255),
             clips = TagAddressSpace(data, metadata_magic_).block(root + 0x2c, stride);
  if (animation >= clips.count)
    throw FormatError("animation index outside graph");
  const auto p = clips.offset + animation * stride;
  const int frames = i16(data, p + 0x14);
  const int static_size = i16(data, p + 0x42-shift),
            animated_size = std::int32_t(data.u32(p + 0x48-shift));
  if (frames <= 0 || frames > 10000 || static_size < 0 ||
      animated_size < 0 || u8(data, p + 0x13) != nodes.count)
    throw FormatError("invalid animation resource range");
  std::size_t offset=0;
  std::vector<std::byte> raw;
  if(vista_) {
    // Vista's 0x60-byte clip owns an inline metadata dataref at28/2C.
    // The supplied FP SMG's 33 codec payloads are byte-identical to Xbox;
    // their channel decoding remains the existing verified implementation.
    const auto size=data.u32(p+0x28);
    const auto bytes=data.slice(metadata_offset(data.u32(p+0x2c),size),size);
    raw.assign(bytes.begin(),bytes.end());
  } else {
    const auto groups=TagAddressSpace(data, metadata_magic_).block(root+0xac, 20);
    const auto group=std::int32_t(data.u32(p+0x24)),stored_offset=std::int32_t(data.u32(p+0x28));
    if(group<0 || std::size_t(group)>=groups.count || stored_offset<0)
      throw FormatError("invalid animation resource group range");
    offset=std::size_t(stored_offset);
    const auto g=groups.offset+std::size_t(group)*20;
    if(mcc_) {
      const auto begin=metadata_offset(tag->address,tag->size);
      const auto resource=data.u32(g+8),size=data.u32(g+4);
      if(size && (resource<begin || resource-begin>tag->size || size>tag->size-(resource-begin)))
        throw FormatError("Canonical MCC animation resource escapes its owned graph");
      if(size) {
        const auto bytes=data.slice(resource,size);raw.assign(bytes.begin(),bytes.end());
      }
    } else raw=resources(data.u32(g+8),data.u32(g+4));
  }
  const Bytes b(raw);
  const auto words = (nodes.count + 31) / 32, mask_size = words * 4,
             mask = std::size_t(offset) + static_size + animated_size;
  const auto static_flags = u8(data, p + 0x3c-shift),
             animated_flags = u8(data, p + 0x3d-shift);
  if ((static_size && static_flags != mask_size * 3) ||
      (animated_size && animated_flags != mask_size * 3))
    throw FormatError("invalid animation channel mask size");
  b.require(std::size_t(offset), std::size_t(static_size) + animated_size +
                                     static_flags + animated_flags);
  AnimationClip out;
  out.source_graph=datum;out.source_index=animation;
  out.source_identifier=(std::uint32_t(animation)<<16)|0xffffu;
  out.name = tag->name + ":" + std::to_string(animation);
  out.frame_count = unsigned(frames);
  out.frame_event_count = data.u32(p + 0x4c-shift);
  const auto frame_events = TagAddressSpace(data, metadata_magic_).block(p + 0x4c-shift, 4);
  for (std::size_t i = 0; i < frame_events.count; ++i) {
    const auto e = frame_events.offset + i * 4;
    out.frame_events.push_back({std::int16_t(i16(data, e)),
                                std::int16_t(i16(data, e + 2))});
  }
  out.animation_type = u8(data,p+0x10);
  out.authored_flags = data.u16(p + 0x18);
  out.loop_frame = unsigned(std::max(0, i16(data, p + (vista_?0x20:0x2e))));
  const unsigned movement_type=u8(data,p+0x11),movement_size=data.u16(p+0x3e - shift);
  if(movement_size) {
    const unsigned stride=movement_type==1?8:movement_type==2?12:movement_type==3?16:0;
    if(!stride || movement_size!=stride*unsigned(frames))
      throw FormatError("Invalid authored animation movement records");
    const auto start=mask+static_flags+animated_flags;
    b.require(start,movement_size);
    for(unsigned f=0;f<unsigned(frames);++f) {
      const auto at=start+f*stride;
      out.movement.push_back({b.f32(at),b.f32(at+4),movement_type==3?b.f32(at+8):0.f});
      out.movement_yaw.push_back(movement_type>=2?b.f32(at+stride-4):0.f);
    }
  }
  // Non-looping authored actions can retain a loop marker beyond the final
  // sample (e.g. Spartan hard landing139:29 frames, loop30). Decoding does not
  // request looping. The explicit seconds-loop sampler validates on use;
  // the source channel clock owns its signed authored loop field separately.
  for (std::size_t i = 0; i < nodes.count; ++i) {
    const auto n = nodes.offset + i * 32;
    out.nodes.push_back(data.u32(n));
    out.parents.push_back(i16(data, n + 8));
    out.node_flags.push_back(std::to_integer<std::uint8_t>(data.slice(n+10,1)[0]));
  }
  out.frames.resize(nodes.count * frames);
  out.components.resize(nodes.count);
  out.channels.resize(nodes.count);
  for (unsigned section = 0; section < 2; ++section) {
    const auto base = std::size_t(offset) + (section ? static_size : 0);
    const auto size = section ? animated_size : static_size;
    if (!size)
      continue;
    const Bytes codec(b.slice(base, std::size_t(size)));
    codec.require(0, 32);
    const auto id = u8(codec, 0);
    if ((!section && id != 1) || (section && id != 3 && (id < 4 || id > 8)))
      throw FormatError("unsupported animation codec " + std::to_string(id));
    std::array<unsigned, 3> ordinal{};
    for (std::size_t i = 0; i < nodes.count; ++i)
      for (unsigned channel = 0; channel < 3; ++channel) {
        if (!(b.u32(mask + (section ? static_flags : 0) + channel * mask_size +
                    (i / 32) * 4) &
              (1u << (i % 32))))
          continue;
        const auto index = ordinal[channel]++;
        if (index >= u8(codec, channel + 1))
          throw FormatError("animation mask exceeds channel count");
        out.components[i] |= 1u << channel;
        auto &track = out.channels[i][channel];
        track.times.clear();
        track.keys.clear();
        if (section && id >= 4 && id <= 7) {
          codec.require(0, 48);
          const auto descriptor_base =
              channel == 0 ? 48u : codec.u32(channel == 1 ? 12 : 16);
          const auto descriptor = codec.u32(descriptor_base + index * 4);
          const auto keys = descriptor & 0xfffu, start = descriptor >> 12;
          const auto width = (id == 5 || id == 7) ? 2u : 1u;
          const auto frame_base = codec.u32(20 + channel * 4),
                     key_base = codec.u32(32 + channel * 4);
          const auto element = channel == 0 ? 8u : channel == 1 ? 12u : 4u;
          if (!keys)
            throw FormatError("empty sparse animation channel");
          codec.require(std::size_t(frame_base) + std::size_t(start) * width,
                        std::size_t(keys) * width);
          codec.require(std::size_t(key_base) + std::size_t(start) * element,
                        std::size_t(keys) * element);
          std::vector<unsigned> times(keys);
          for (unsigned k = 0; k < keys; ++k) {
            const auto at = std::size_t(frame_base) + (start + k) * width;
            times[k] = width == 1 ? u8(codec, at) : codec.u16(at);
            if ((k && times[k] <= times[k - 1]) || times[k] > out.frame_count)
              throw FormatError("invalid sparse animation key times");
            const auto key_at = std::size_t(key_base) + (start + k) * element;
            std::array<float, 4> key{};
            if (channel == 0)
              key = rotation(codec, key_at, true, false);
            else if (channel == 1) {
              const auto v = point(codec, key_at);
              key = {v.x, v.y, v.z, 0};
            } else
              key[0] = real(codec, key_at);
            track.times.push_back(times[k]);
            track.keys.push_back(key);
          }
          for (unsigned f = 0; f < out.frame_count; ++f) {
            const auto upper = std::upper_bound(times.begin(), times.end(), f);
            const auto k = upper == times.begin()
                               ? 0u
                               : unsigned(upper - times.begin() - 1),
                       next = std::min(k + 1, keys - 1);
            const float blend =
                f <= times[k] || k == next
                    ? 0.f
                    : std::clamp(float(f - times[k]) /
                                     float(times[next] - times[k]),
                                 0.f, 1.f);
            const auto a = std::size_t(key_base) + (start + k) * element,
                       b = std::size_t(key_base) + (start + next) * element;
            auto &pose = out.frames[std::size_t(f) * nodes.count + i];
            if (channel == 0) {
              const auto qa = rotation(codec, a, true, false),
                         qb = rotation(codec, b, true, false);
              float norm = 0;
              for (unsigned j = 0; j < 4; ++j) {
                pose.rotation[j] = qa[j] * (1 - blend) + qb[j] * blend;
                norm += pose.rotation[j] * pose.rotation[j];
              }
              if (norm < 1e-12f)
                throw FormatError("degenerate sparse animation quaternion");
              for (auto &v : pose.rotation)
                v /= std::sqrt(norm);
            } else if (channel == 1)
              pose.translation =
                  point(codec, a) * (1 - blend) + point(codec, b) * blend;
            else
              pose.scale =
                  real(codec, a) * (1 - blend) + real(codec, b) * blend;
          }
          continue;
        }
        const auto channel_base =
            channel == 0 ? 32u : codec.u32(channel == 1 ? 12 : 16);
        // Original292970/2929B0/2CB1E0 copy codec8 float4/float3/float
        // channels. Quaternion keys use16 bytes and are not normalized.
        const bool float_rotation=id==8;
        const auto element = channel == 0 ? (float_rotation?16u:8u) : channel == 1 ? 12u : 4u;
        const auto stride = section ? codec.u32(20 + channel * 4) : element;
        for (unsigned f = 0; f < (section ? out.frame_count : 1u); ++f) {
          const auto at = std::size_t(channel_base) +
                          std::size_t(index) * stride +
                          (section ? std::size_t(f) * element : 0);
          std::array<float, 4> key{};
          if (channel == 0)
            key = rotation(codec, at, !float_rotation, false);
          else if (channel == 1) {
            const auto v = point(codec, at);
            key = {v.x, v.y, v.z, 0};
          } else
            key[0] = real(codec, at);
          track.times.push_back(f);
          track.keys.push_back(key);
        }
        for (unsigned f = 0; f < out.frame_count; ++f) {
          const auto at = std::size_t(channel_base) +
                          std::size_t(index) * stride +
                          (section ? std::size_t(f) * element : 0);
          auto &pose = out.frames[std::size_t(f) * nodes.count + i];
          if (channel == 0)
            pose.rotation = rotation(codec, at, !float_rotation,!float_rotation);
          else if (channel == 1)
            pose.translation = point(codec, at);
          else
            pose.scale = real(codec, at);
        }
      }
    for (unsigned channel = 0; channel < 3; ++channel)
      if (ordinal[channel] != u8(codec, channel + 1))
        throw FormatError("animation channel count disagrees with mask");
  }
  return out;
}
std::string Cache::string_id(std::uint32_t id) const {
  const Bytes data(cache_bytes());
  const auto shift=pc_geometry()?12u:0u;
  const auto index = StringId{id}.index(), count = data.u32(mcc_?48:356+shift), size = data.u32(mcc_?56:360+shift),
             indices = data.u32(mcc_?60:364+shift), strings = data.u32(mcc_?52:368+shift);
  if (index >= count || count > 1000000)
    throw FormatError("StringID index outside table");
  const auto offset = data.u32(std::size_t(indices) + index * 4);
  if (offset >= size)
    throw FormatError("StringID offset outside data");
  const Bytes table(data.slice(strings, size));
  return table.string(offset, size - offset);
}
AnimationClip Cache::load_action(std::uint32_t datum, const std::string &mode,
                                 const std::string &weapon_class,
                                 const std::string &weapon_type,
                                 const std::string &action) const {
  return load_action(datum,mode,weapon_class,weapon_type,action,
      [this](auto pointer,auto bytes){return raw_data(pointer,bytes);});
}
AnimationClip Cache::load_action(std::uint32_t datum,const std::string &mode,
                                 const std::string &weapon_class,const std::string &weapon_type,
                                 const std::string &action,const AnimationResourceReader &resources) const {
  const Bytes data(cache_bytes());
  const auto* tag = find_tag({datum}, tag_groups::model_animation_graph);
  if (!tag || !tag->address)
    throw FormatError("missing action animation graph");
  auto named = [&](TagBlockRange table, std::size_t stride, const std::string &name) {
    for (std::size_t i = 0; i < table.count; ++i)
      if (string_id(data.u32(table.offset + i * stride)) == name)
        return table.offset + i * stride;
    throw FormatError("animation action category missing: " + name);
  };
  const auto root = metadata_offset(tag->address, 0x54);
  const auto m = named(TagAddressSpace(data, metadata_magic_).block(root + 0x34, 20), 20, mode);
  const auto c =
      named(TagAddressSpace(data, metadata_magic_).block(m + 4, 20), 20, weapon_class);
  const auto t =
      named(TagAddressSpace(data, metadata_magic_).block(c + 4, 0x34), 0x34, weapon_type);
  const auto a = named(TagAddressSpace(data, metadata_magic_).block(t + 4, 8), 8, action);
  auto graph = datum;
  if (const auto inherited = i16(data, a + 4); inherited != -1) {
    const auto graphs = TagAddressSpace(data, metadata_magic_).block(root + 0x4c, 32, 64);
    if (inherited < 0 || std::size_t(inherited) >= graphs.count)
      throw FormatError("inherited action graph outside list");
    graph = data.u32(graphs.offset + std::size_t(inherited) * 32 + 4);
  }
  const auto animation = i16(data, a + 6);
  if (animation < 0)
    throw FormatError("action has no animation");
  auto clip = load_animation(graph, unsigned(animation),resources);
  clip.source_identifier=data.u32(a+4);
  clip.name = mode + ":" + weapon_class + ":" + weapon_type + ":" + action;
  return clip;
}
} // namespace halo2
