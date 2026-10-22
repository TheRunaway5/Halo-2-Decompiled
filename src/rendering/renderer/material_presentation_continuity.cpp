#include "halo2/material_presentation_continuity.hpp"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <functional>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace halo2 {
namespace {
constexpr std::size_t absent=std::numeric_limits<std::size_t>::max();
void need(bool value){if(!value)throw FormatError("Material presentation lost its owner, completed batch, resident schema or storage budget");}
template<class T>bool bits(const T &a,const T &b)noexcept {
    static_assert(std::is_trivially_copyable_v<T>);return std::memcmp(&a,&b,sizeof(T))==0;
}
struct SchemaMismatch {
    const char *member="none";std::size_t offset=absent,field_offset=absent,pass=absent;
};
template<class T>struct ScalarArray :std::bool_constant<std::is_arithmetic_v<T>||std::is_enum_v<T>||std::is_same_v<T,std::byte>>{};
template<class T,std::size_t N>struct ScalarArray<std::array<T,N>>
    :std::bool_constant<N!=0&&ScalarArray<T>::value&&sizeof(std::array<T,N>)==N*sizeof(T)>{};
// Only these proved, padding-free records have object bytes identical to the
// named-field stream below. Other layouts keep the field-by-field fallback;
// trivially copyable or sizeof alone does not establish serialization order.
template<class T>struct ContiguousSchemaRecord:std::false_type{};
template<>struct ContiguousSchemaRecord<ShaderRange>:std::bool_constant<
    std::is_standard_layout_v<ShaderRange>&&std::is_trivially_copyable_v<ShaderRange>&&
    sizeof(ShaderRange)==sizeof(std::uint16_t)&&offsetof(ShaderRange,packed)==0>{};
template<>struct ContiguousSchemaRecord<ShaderTextureBinding>:std::bool_constant<
    std::is_standard_layout_v<ShaderTextureBinding>&&std::is_trivially_copyable_v<ShaderTextureBinding>&&
    sizeof(ShaderTextureBinding)==4*sizeof(std::uint8_t)&&
    offsetof(ShaderTextureBinding,parameter)==0&&offsetof(ShaderTextureBinding,external)==1&&
    offsetof(ShaderTextureBinding,stage)==2&&offsetof(ShaderTextureBinding,flags)==3>{};
template<>struct ContiguousSchemaRecord<ShaderTextureState>:std::bool_constant<
    std::is_standard_layout_v<ShaderTextureState>&&std::is_trivially_copyable_v<ShaderTextureState>&&
    sizeof(ShaderTextureState)==14*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,address_u)==0*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,address_v)==1*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,address_w)==2*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,mag_filter)==3*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,min_filter)==4*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,mip_filter)==5*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,lod_bias)==6*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,max_mip)==7*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,anisotropy)==8*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,color_key_op)==9*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,color_sign)==10*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,alpha_kill)==11*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,border_color)==12*sizeof(std::uint32_t)&&
    offsetof(ShaderTextureState,key_color)==13*sizeof(std::uint32_t)>{};
template<>struct ContiguousSchemaRecord<ShaderRemap>:std::bool_constant<
    std::is_standard_layout_v<ShaderRemap>&&std::is_trivially_copyable_v<ShaderRemap>&&
    sizeof(ShaderRemap)==sizeof(std::uint16_t)+2*sizeof(std::uint8_t)&&
    offsetof(ShaderRemap,destination)==0&&offsetof(ShaderRemap,component)==sizeof(std::uint16_t)&&
    offsetof(ShaderRemap,source)==sizeof(std::uint16_t)+sizeof(std::uint8_t)>{};
template<>struct ContiguousSchemaRecord<ShaderFragment>:std::bool_constant<
    std::is_standard_layout_v<ShaderFragment>&&std::is_trivially_copyable_v<ShaderFragment>&&
    ContiguousSchemaRecord<ShaderRange>::value&&sizeof(ShaderFragment)==2*sizeof(std::uint16_t)&&
    offsetof(ShaderFragment,switch_index)==0&&offsetof(ShaderFragment,permutations)==sizeof(std::int16_t)>{};
template<>struct ContiguousSchemaRecord<ShaderPermutation>:std::bool_constant<
    std::is_standard_layout_v<ShaderPermutation>&&std::is_trivially_copyable_v<ShaderPermutation>&&
    ContiguousSchemaRecord<ShaderRange>::value&&sizeof(ShaderPermutation)==3*sizeof(std::uint16_t)&&
    offsetof(ShaderPermutation,value)==0&&offsetof(ShaderPermutation,flags)==sizeof(std::int16_t)&&
    offsetof(ShaderPermutation,combiners)==sizeof(std::int16_t)+sizeof(std::uint16_t)>{};
template<>struct ContiguousSchemaRecord<ShaderCombiner>:std::bool_constant<
    std::is_standard_layout_v<ShaderCombiner>&&std::is_trivially_copyable_v<ShaderCombiner>&&
    ScalarArray<decltype(ShaderCombiner::words)>::value&&
    sizeof(ShaderCombiner)==8*sizeof(std::uint32_t)&&offsetof(ShaderCombiner,words)==0>{};
template<bool Encode>struct Schema {
    std::vector<std::byte> *output{};std::span<const std::byte> expected;std::size_t cursor{};bool equal=true;
    const char *member="unknown";std::size_t field_begin{},pass_index=absent;SchemaMismatch mismatch;
    void bytes(const void *p,std::size_t n){
        if(!n)return;
        if constexpr(Encode){const auto *b=static_cast<const std::byte *>(p);output->insert(output->end(),b,b+n);}
        else if(equal){
            const auto available=cursor>expected.size()?0:std::min(n,expected.size()-cursor);
            if(available!=n||std::memcmp(expected.data()+cursor,p,n)){
                std::size_t at{};const auto *live=static_cast<const std::byte *>(p);
                while(at<available&&expected[cursor+at]==live[at])++at;
                equal=false;mismatch={member,cursor+at,cursor+at-field_begin,pass_index};
            }
        }
        cursor+=n;
    }
    template<class T>requires(std::is_arithmetic_v<T>||std::is_enum_v<T>||std::is_same_v<T,std::byte>)
    void add(const T &v){
        if constexpr(!Encode&&(sizeof(T)==1||sizeof(T)==2||sizeof(T)==4||sizeof(T)==8)){
            // Compare the serialized representation, never numeric floating
            // equality or an unaligned typed read from the canonical stream.
            // Failure keeps the original first-differing/missing-byte scanner.
            if(!equal){cursor+=sizeof(T);return;}
            if(cursor<=expected.size()&&sizeof(T)<=expected.size()-cursor){
                using Word=std::conditional_t<sizeof(T)==1,std::uint8_t,
                    std::conditional_t<sizeof(T)==2,std::uint16_t,
                    std::conditional_t<sizeof(T)==4,std::uint32_t,std::uint64_t>>>;
                Word actual{},canonical{};
                std::memcpy(&actual,&v,sizeof(T));
                std::memcpy(&canonical,expected.data()+cursor,sizeof(T));
                if(actual==canonical){cursor+=sizeof(T);return;}
            }
        }
        bytes(&v,sizeof(v));
    }
    template<class T,std::size_t N>void add(const std::array<T,N> &v){
        // The recursive scalar-only case is exactly the former concatenation
        // of element bytes. Aggregate/padded records keep their named fields.
        if constexpr(N==0)return;
        else if constexpr(ScalarArray<std::array<T,N>>::value)bytes(v.data(),sizeof(v));
        else for(const auto &x:v)add(x);
    }
    template<class T>void add(const std::vector<T> &v){add(v.size());
        // Preserve the size prefix and every payload byte. In particular,
        // padded ShaderState/ShaderLod still use their named-field encoders.
        if constexpr(ScalarArray<T>::value||ContiguousSchemaRecord<T>::value)bytes(v.data(),v.size()*sizeof(T));
        else for(const auto &x:v)add(x);}
    template<class T>void add(const std::optional<T> &v){add(bool(v));if(v)add(*v);}
    void add(const std::string &v){add(v.size());bytes(v.data(),v.size());}
    template<class... T>void fields(const T &...v){(add(v),...);}
    template<class T>void field(const char *name,const T &v){member=name;field_begin=cursor;add(v);}
    void add(const Vec3 &v){fields(v.x,v.y,v.z);}
    void add(const ShaderRange &v){add(v.packed);}
    void add(const ShaderBitmap &v){fields(v.datum,v.index,v.log_dimension);}
    void add(const ShaderLod &v){fields(v.layers,v.available_layers,v.projected_height);}
    void add(const ShaderTextureBinding &v){fields(v.parameter,v.external,v.stage,v.flags);}
    void add(const ShaderTextureState &v){fields(v.address_u,v.address_v,v.address_w,v.mag_filter,v.min_filter,v.mip_filter,
        v.lod_bias,v.max_mip,v.anisotropy,v.color_key_op,v.color_sign,v.alpha_kill,v.border_color,v.key_color);}
    void add(const ShaderState &v){fields(v.index,v.value);}
    void add(const ShaderRemap &v){fields(v.destination,v.component,v.source);}
    void add(const ShaderFragment &v){fields(v.switch_index,v.permutations);}
    void add(const ShaderPermutation &v){fields(v.value,v.flags,v.combiners);}
    void add(const ShaderCombiner &v){add(v.words);}
    void add(const PixelProgram &v){fields(v.words,v.fragment_offsets);}
    void add(const SourceVertexDeclaration &v){fields(v.class_index,v.semantics);}
    void add(const ShaderOverlay &v){fields(v.input,v.range,v.period,v.curve);}
    void add(const ShaderOverlayReference &v){fields(v.function,v.transform);}
    void add(const ShaderAnimationImplementation &v){fields(v.bitmap_transforms,v.pixel_constants,v.vertex_constants);}
    void add(const ShaderAnimationLod &v){fields(v.available_layers,v.layers);}
    void add(const ShaderAnimation &v){fields(v.lods,v.layers,v.passes,v.parameters,v.implementations,v.overlays,
        v.references,v.mappings,std::uintptr_t(v.tables.get()));}
    void add(const TextureMip &v){fields(v.width,v.height,v.offset,v.size,v.depth);}
    void add(const Texture &v){fields(v.width,v.height,v.format,std::uintptr_t(v.pixels.data()),v.pixels.size(),v.mips,v.layers,v.depth);}
    void pass(const MaterialPass &v){
#define HALO2_PASS_FIELD(name) field("passes." #name,v.name)
        HALO2_PASS_FIELD(name);HALO2_PASS_FIELD(vertex_shader);HALO2_PASS_FIELD(template_pass);HALO2_PASS_FIELD(implementation);
        HALO2_PASS_FIELD(textures);HALO2_PASS_FIELD(states);HALO2_PASS_FIELD(texture_states);HALO2_PASS_FIELD(bitmap_remaps);
        HALO2_PASS_FIELD(pixel_remaps);HALO2_PASS_FIELD(vertex_remaps);HALO2_PASS_FIELD(pixel_externs);HALO2_PASS_FIELD(vertex_externs);
        HALO2_PASS_FIELD(state_externs);HALO2_PASS_FIELD(definition);HALO2_PASS_FIELD(fragments);HALO2_PASS_FIELD(permutations);
        HALO2_PASS_FIELD(combiners);HALO2_PASS_FIELD(vertex_code);HALO2_PASS_FIELD(vertex_declaration);
#undef HALO2_PASS_FIELD
    }
    void header(const Material &v,bool generated_diffuse=false){
        field("name",v.name);field("diffuse.width",v.diffuse.width);field("diffuse.height",v.diffuse.height);
        field("diffuse.format",v.diffuse.format);
        if(!generated_diffuse)field("diffuse.pixels.identity",std::uintptr_t(v.diffuse.pixels.data()));
        field("diffuse.pixels.size",v.diffuse.pixels.size());field("diffuse.mips",v.diffuse.mips);
        field("diffuse.layers",v.diffuse.layers);field("diffuse.depth",v.diffuse.depth);
#define HALO2_MATERIAL_FIELD(name) field(#name,v.name)
        HALO2_MATERIAL_FIELD(template_name);HALO2_MATERIAL_FIELD(bitmaps);HALO2_MATERIAL_FIELD(pixel_constants);
        HALO2_MATERIAL_FIELD(vertex_constants);HALO2_MATERIAL_FIELD(lods);HALO2_MATERIAL_FIELD(layers);
        HALO2_MATERIAL_FIELD(pass_implementations);HALO2_MATERIAL_FIELD(animation);HALO2_MATERIAL_FIELD(detail);
        HALO2_MATERIAL_FIELD(bump);HALO2_MATERIAL_FIELD(vertex_program);HALO2_MATERIAL_FIELD(vertex_declaration);
        HALO2_MATERIAL_FIELD(world_vertex_input_transform);HALO2_MATERIAL_FIELD(static_lightmap_pass);HALO2_MATERIAL_FIELD(forward_pass);
        HALO2_MATERIAL_FIELD(hud_pass);HALO2_MATERIAL_FIELD(ui_full_surface);HALO2_MATERIAL_FIELD(light_volume_pass);
        HALO2_MATERIAL_FIELD(forward_pass_index);HALO2_MATERIAL_FIELD(forward_vertex);HALO2_MATERIAL_FIELD(forward_blend);
        HALO2_MATERIAL_FIELD(auxiliary);HALO2_MATERIAL_FIELD(animated_textures);HALO2_MATERIAL_FIELD(accumulation_texture);
        HALO2_MATERIAL_FIELD(world_pass);HALO2_MATERIAL_FIELD(per_vertex_lighting);HALO2_MATERIAL_FIELD(baked_lightmap_texture);
        HALO2_MATERIAL_FIELD(world_bloom_pass);HALO2_MATERIAL_FIELD(water_reflection_texture);HALO2_MATERIAL_FIELD(water_refraction_texture);
        HALO2_MATERIAL_FIELD(water_pass);HALO2_MATERIAL_FIELD(scope_texture);HALO2_MATERIAL_FIELD(water_definition);
        HALO2_MATERIAL_FIELD(rendering_gaps);HALO2_MATERIAL_FIELD(source_shader_datum);HALO2_MATERIAL_FIELD(selected_shader_lod);
        HALO2_MATERIAL_FIELD(ui_emblem_pass);HALO2_MATERIAL_FIELD(ui_widescreen_background);HALO2_MATERIAL_FIELD(ui_frame_center);
        HALO2_MATERIAL_FIELD(ui_frame_extension);
#undef HALO2_MATERIAL_FIELD
        field("passes.size",v.passes.size());
    }
};
template<class F>void children(const Material &v,F f){
    f(v.environment_pass);f(v.planar_fog_pass);
    for(const auto &p:v.world_overlays_before)f(p);for(const auto &p:v.world_overlays_after)f(p);
    for(const auto &p:v.world_bloom_passes)f(p);for(const auto &p:v.world_planar_passes)f(p);
}
struct Layout {
    std::size_t before{},after{},bloom{};
    bool operator==(const Layout &)const=default;
};
Layout layout(const Material &v){return {v.world_overlays_before.size(),v.world_overlays_after.size(),v.world_bloom_passes.size()};}
struct Links {
    std::shared_ptr<Material> environment,fog;
    std::vector<std::shared_ptr<Material>> before,after,bloom;
    std::array<std::shared_ptr<Material>,3> planar;
    explicit Links(const Material &v):environment(v.environment_pass),fog(v.planar_fog_pass),before(v.world_overlays_before),
        after(v.world_overlays_after),bloom(v.world_bloom_passes),planar(v.world_planar_passes){}
    bool same(const Material &v)const noexcept;
    void swap(Material &v)noexcept{environment.swap(v.environment_pass);fog.swap(v.planar_fog_pass);
        before.swap(v.world_overlays_before);after.swap(v.world_overlays_after);bloom.swap(v.world_bloom_passes);planar.swap(v.world_planar_passes);}
    std::size_t count()const noexcept{return 5+before.size()+after.size()+bloom.size();}
};
template<class T>bool same_link_owner(const std::shared_ptr<T> &a,const std::shared_ptr<T> &b)noexcept {
    return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
bool same_link_owners(const Links &saved,const Material &live)noexcept {
    const auto sequence=[](const auto &a,const auto &b)noexcept {
        if(a.size()!=b.size())return false;
        for(std::size_t n=0;n<a.size();++n)if(!same_link_owner(a[n],b[n]))return false;
        return true;
    };
    return same_link_owner(saved.environment,live.environment_pass)&&same_link_owner(saved.fog,live.planar_fog_pass)&&
        sequence(saved.before,live.world_overlays_before)&&sequence(saved.after,live.world_overlays_after)&&
        sequence(saved.bloom,live.world_bloom_passes)&&sequence(saved.planar,live.world_planar_passes);
}
bool Links::same(const Material &v)const noexcept{return same_link_owners(*this,v);}
struct PassValue {
    std::shared_ptr<const ShaderConstants> constants;
    std::shared_ptr<const PixelProgram> program;
    bool operator==(const PassValue &)const=default;
};
using PassValues=std::vector<PassValue>;
struct MutablePassComparisons {
    struct Key {
        const MaterialPass *actual;
        const ShaderConstants *constants;
        const PixelProgram *program;
        bool operator==(const Key &)const=default;
    };
    struct Hash {
        std::size_t operator()(const Key &key)const noexcept {
            auto hash=std::hash<const void *>{}(key.actual);
            hash^=std::hash<const void *>{}(key.constants)+0x9e3779b9u+(hash<<6)+(hash>>2);
            return hash^(std::hash<const void *>{}(key.program)+0x9e3779b9u+(hash<<6)+(hash>>2));
        }
    };
    std::size_t checked{},reused{};
    // Only this capture owns the set. The prior Graph keeps both expected
    // immutable banks alive; keys do not retain or expose live COW records.
    std::unordered_set<Key,Hash> successful;
    bool same(const PassValue &expected,const MaterialPass &actual) {
        const Key key{&actual,expected.constants.get(),expected.program.get()};
        if(successful.contains(key)){++reused;return true;}
        ++checked;
        if(!bits(*expected.constants,actual.constants)||!bits(*expected.program,actual.program))return false;
        successful.insert(key);return true;
    }
};
bool image_same(const Texture &a,const Texture &b)noexcept {
    if(a.width!=b.width||a.height!=b.height||a.format!=b.format||a.layers!=b.layers||a.depth!=b.depth||
        a.pixels.size()!=b.pixels.size()||a.mips.size()!=b.mips.size())return false;
    for(std::size_t n=0;n<a.mips.size();++n){const auto &x=a.mips[n],&y=b.mips[n];
        if(x.width!=y.width||x.height!=y.height||x.offset!=y.offset||x.size!=y.size||x.depth!=y.depth)return false;}
    return a.pixels.empty()||std::memcmp(a.pixels.data(),b.pixels.data(),a.pixels.size())==0;
}
// Actual post-upload producer writes. Everything omitted here is part of the
// independently validated immutable resident schema above.
struct Fields {
    std::array<std::int32_t,8> switches;
    float specular,ambient;
    std::array<ShaderVector,3> texture_transforms;
    std::shared_ptr<const VertexProgramBank> vertex_bank;
    std::array<ShaderTextureState,4> texture_states;
    std::optional<VistaLoadingMaterial> vista;
    std::uint32_t blend_color;
    ForwardRasterState raster;
    std::array<ShaderVector,8> transforms;
    ShaderVector diffuse,specular_color;
    std::array<std::uint32_t,4> frames;
    std::optional<std::array<Vec3,4>> colors;
    explicit Fields(const Material &m):switches(m.shader_switches),specular(m.specular_brightness),ambient(m.ambient_brightness),
        texture_transforms(m.texture_transforms),vertex_bank(m.vertex_bank),texture_states(m.texture_states),vista(m.vista_loading),
        blend_color(m.forward_blend_color),raster(m.forward_raster),transforms(m.forward_transforms),diffuse(m.forward_diffuse),
        specular_color(m.forward_specular),frames(m.texture_frames),colors(m.object_render_colors){}
    bool same(const Fields &b)const noexcept{
        const bool same_vista=bool(vista)==bool(b.vista)&&(!vista||(vista->shader==b.vista->shader&&
            vista->blend==b.vista->blend&&bits(vista->constant0,b.vista->constant0)&&
            bits(vista->raster_offset_pixels,b.vista->raster_offset_pixels)));
        return bits(switches,b.switches)&&bits(specular,b.specular)&&bits(ambient,b.ambient)&&
            bits(texture_transforms,b.texture_transforms)&&vertex_bank==b.vertex_bank&&bits(texture_states,b.texture_states)&&
            same_vista&&blend_color==b.blend_color&&raster==b.raster&&bits(transforms,b.transforms)&&
            bits(diffuse,b.diffuse)&&bits(specular_color,b.specular_color)&&frames==b.frames&&
            bool(colors)==bool(b.colors)&&(!colors||bits(*colors,*b.colors));
    }
    void swap(Material &m)noexcept{
        using std::swap;swap(switches,m.shader_switches);swap(specular,m.specular_brightness);swap(ambient,m.ambient_brightness);
        swap(texture_transforms,m.texture_transforms);vertex_bank.swap(m.vertex_bank);swap(texture_states,m.texture_states);swap(vista,m.vista_loading);
        swap(blend_color,m.forward_blend_color);swap(raster,m.forward_raster);swap(transforms,m.forward_transforms);
        swap(diffuse,m.forward_diffuse);swap(specular_color,m.forward_specular);swap(frames,m.texture_frames);swap(colors,m.object_render_colors);
    }
};
struct Values {
    Fields fields;
    std::shared_ptr<const PixelProgram> lightmap_program,forward_program;
    std::shared_ptr<const ShaderConstants> constants;
    std::shared_ptr<const PassValues> passes;
    std::shared_ptr<const Texture> generated_diffuse;
    bool same_fixed(const Material &m)const noexcept{
        return fields.same(Fields(m))&&bits(*lightmap_program,m.lightmap_program)&&bits(*forward_program,m.forward_program)&&
            bits(*constants,m.program_constants)&&passes->size()==m.passes.size()&&
            (!generated_diffuse||image_same(*generated_diffuse,m.diffuse));
    }
    bool same(const Material &m)const noexcept{
        if(!same_fixed(m))return false;
        for(std::size_t n=0;n<passes->size();++n)if(!bits(*(*passes)[n].constants,m.passes[n].constants)||
            !bits(*(*passes)[n].program,m.passes[n].program))return false;return true;
    }
    bool same_capture(const Material &m,MutablePassComparisons &comparisons)const{
        if(!same_fixed(m))return false;
        for(std::size_t n=0;n<passes->size();++n)
            if(!comparisons.same((*passes)[n],m.passes[n]))return false;
        return true;
    }
};
struct WritableValues {
    Fields fields;PixelProgram lightmap_program,forward_program;ShaderConstants constants;
    explicit WritableValues(const Values &v):fields(v.fields),lightmap_program(*v.lightmap_program),
        forward_program(*v.forward_program),constants(*v.constants){}
    void swap(Material &m)noexcept{fields.swap(m);using std::swap;swap(lightmap_program,m.lightmap_program);
        swap(forward_program,m.forward_program);swap(constants,m.program_constants);}
};
std::size_t image_bytes(const Texture &v)noexcept{return sizeof(Texture)+v.pixels.size()+v.mips.size()*sizeof(TextureMip);}
// The former expanded value layout, retained only as a truthful comparison
// measurement. The storage cap below applies to distinct reachable payloads.
struct ExpandedValues {
    std::array<std::int32_t,8> switches;float specular,ambient;
    std::array<ShaderVector,3> texture_transforms;PixelProgram lightmap_program,forward_program;ShaderConstants constants;
    std::shared_ptr<const VertexProgramBank> vertex_bank;std::array<ShaderTextureState,4> texture_states;
    std::optional<VistaLoadingMaterial> vista;std::uint32_t blend_color;ForwardRasterState raster;
    std::array<ShaderVector,8> transforms;ShaderVector diffuse,specular_color;std::array<std::uint32_t,4> frames;
    std::optional<std::array<Vec3,4>> colors;
    struct Pass {ShaderConstants constants;PixelProgram program;};
    std::vector<Pass> passes;std::shared_ptr<const Texture> generated_diffuse;
};
std::size_t expanded_bytes(const Material &m,bool generated)noexcept{
    return sizeof(ExpandedValues)+m.passes.size()*sizeof(ExpandedValues::Pass)+(generated?image_bytes(m.diffuse):0);
}
struct Fingerprint {
    std::size_t value=1469598103934665603ull;
    void bytes(const void *p,std::size_t n)noexcept{
        const auto h=std::hash<std::string_view>{}({static_cast<const char *>(p),n});
        value^=h+0x9e3779b97f4a7c15ull+(value<<6)+(value>>2);
    }
    template<class T>void add(const T &v)noexcept{static_assert(std::is_trivially_copyable_v<T>);bytes(&v,sizeof(v));}
    template<class T>void pointer(const std::shared_ptr<const T> &v)noexcept{add(std::uintptr_t(v.get()));}
};
template<class T>std::size_t fingerprint(const T &v)noexcept{Fingerprint h;h.add(v);return h.value;}
std::size_t fingerprint(const Fields &v)noexcept{
    Fingerprint h;h.add(v.switches);h.add(v.specular);h.add(v.ambient);h.add(v.texture_transforms);h.pointer(v.vertex_bank);
    h.add(v.texture_states);h.add(bool(v.vista));if(v.vista){h.add(v.vista->shader);h.add(v.vista->blend);
        h.add(v.vista->constant0);h.add(v.vista->raster_offset_pixels);}
    h.add(v.blend_color);h.add(v.raster);h.add(v.transforms);h.add(v.diffuse);h.add(v.specular_color);h.add(v.frames);
    h.add(bool(v.colors));if(v.colors)h.add(*v.colors);return h.value;
}
std::size_t fingerprint(const Texture &v)noexcept{
    Fingerprint h;h.add(v.width);h.add(v.height);h.add(v.format);h.add(v.layers);h.add(v.depth);h.add(v.mips.size());
    for(const auto &m:v.mips){h.add(m.width);h.add(m.height);h.add(m.offset);h.add(m.size);h.add(m.depth);}
    h.bytes(v.pixels.data(),v.pixels.size());return h.value;
}
std::size_t fingerprint(const PassValues &v)noexcept{
    Fingerprint h;h.add(v.size());for(const auto &p:v){h.pointer(p.constants);h.pointer(p.program);}return h.value;
}
std::size_t fingerprint(const Values &v)noexcept{
    Fingerprint h;h.add(fingerprint(v.fields));h.pointer(v.lightmap_program);h.pointer(v.forward_program);
    h.pointer(v.constants);h.pointer(v.passes);h.pointer(v.generated_diffuse);return h.value;
}
template<class T>class ValuePool {
    struct Entry {std::shared_ptr<const T> value;bool inherited{},counted{};};
    std::vector<Entry> entries_;
    std::unordered_map<std::size_t,std::vector<std::size_t>> buckets_;
    std::unordered_map<const T *,std::size_t> indices_;
public:
    bool contains(const std::shared_ptr<const T> &value)const{return indices_.contains(value.get());}
    bool counted(const std::shared_ptr<const T> &value)const{return entries_.at(indices_.at(value.get())).counted;}
    void seed(const std::shared_ptr<const T> &value){
        if(indices_.contains(value.get()))return;const auto index=entries_.size();
        indices_.emplace(value.get(),index);buckets_[fingerprint(*value)].push_back(index);entries_.push_back({value,true,false});
    }
    template<class Charge>void retain(const std::shared_ptr<const T> &value,std::size_t bytes,Charge &charge){
        auto &entry=entries_.at(indices_.at(value.get()));if(!entry.counted){charge(bytes,entry.inherited);entry.counted=true;}
    }
    template<class Same,class Factory,class Charge>std::shared_ptr<const T> get(std::size_t hash,std::size_t bytes,
        Same same,Factory factory,Charge &charge){
        auto &bucket=buckets_[hash];for(const auto index:bucket)if(same(*entries_[index].value)){
            const auto value=entries_[index].value;retain(value,bytes,charge);return value;}
        charge(bytes,false);auto value=factory();const auto index=entries_.size();bucket.push_back(index);
        indices_.emplace(value.get(),index);entries_.push_back({value,false,true});return value;
    }
};
class ValueCapture {
    ValuePool<PixelProgram> programs_;ValuePool<ShaderConstants> constants_;ValuePool<PassValues> passes_;
    ValuePool<Texture> images_;ValuePool<Values> values_;
    std::unordered_map<const MaterialPass *,PassValue> pass_cache_;
    PassValues scratch_;
public:
    void seed(const std::shared_ptr<const Values> &v){
        if(values_.contains(v))return;
        programs_.seed(v->lightmap_program);programs_.seed(v->forward_program);constants_.seed(v->constants);
        if(!passes_.contains(v->passes)){passes_.seed(v->passes);for(const auto &p:*v->passes){programs_.seed(p.program);constants_.seed(p.constants);}}
        if(v->generated_diffuse)images_.seed(v->generated_diffuse);values_.seed(v);
    }
    template<class Charge>void retain(const std::shared_ptr<const Values> &v,Charge &charge){
        if(values_.counted(v))return;
        programs_.retain(v->lightmap_program,sizeof(PixelProgram),charge);programs_.retain(v->forward_program,sizeof(PixelProgram),charge);
        constants_.retain(v->constants,sizeof(ShaderConstants),charge);
        if(!passes_.counted(v->passes)){
            passes_.retain(v->passes,sizeof(PassValues)+v->passes->size()*sizeof(PassValue),charge);
            for(const auto &p:*v->passes){programs_.retain(p.program,sizeof(PixelProgram),charge);constants_.retain(p.constants,sizeof(ShaderConstants),charge);}}
        if(v->generated_diffuse)images_.retain(v->generated_diffuse,image_bytes(*v->generated_diffuse),charge);
        values_.retain(v,sizeof(Values),charge);
    }
    template<class Charge>std::shared_ptr<const Values> capture(const Material &m,bool generated,Charge &charge){
        const auto program=[&](const PixelProgram &p){return programs_.get(fingerprint(p),sizeof(p),[&](const auto &v){return bits(v,p);},
            [&]{return std::make_shared<const PixelProgram>(p);},charge);};
        const auto constants=[&](const ShaderConstants &c){return constants_.get(fingerprint(c),sizeof(c),[&](const auto &v){return bits(v,c);},
            [&]{return std::make_shared<const ShaderConstants>(c);},charge);};
        auto lightmap=program(m.lightmap_program),forward=program(m.forward_program);auto bank=constants(m.program_constants);
        scratch_.clear();scratch_.reserve(m.passes.size());
        for(const auto &p:m.passes){const auto at=pass_cache_.find(&p);
            if(at!=pass_cache_.end())scratch_.push_back(at->second);
            else{PassValue value{constants(p.constants),program(p.program)};pass_cache_.emplace(&p,value);scratch_.push_back(std::move(value));}}
        auto passes=passes_.get(fingerprint(scratch_),sizeof(PassValues)+scratch_.size()*sizeof(PassValue),
            [&](const auto &v){return v==scratch_;},[&]{return std::make_shared<const PassValues>(scratch_);},charge);
        std::shared_ptr<const Texture> image;if(generated)image=images_.get(fingerprint(m.diffuse),image_bytes(m.diffuse),
            [&](const auto &v){return image_same(v,m.diffuse);},[&]{return std::make_shared<const Texture>(m.diffuse);},charge);
        Values proposed{Fields(m),std::move(lightmap),std::move(forward),std::move(bank),std::move(passes),std::move(image)};
        return values_.get(fingerprint(proposed),sizeof(Values),[&](const auto &v){return v.fields.same(proposed.fields)&&
            v.lightmap_program==proposed.lightmap_program&&v.forward_program==proposed.forward_program&&v.constants==proposed.constants&&
            v.passes==proposed.passes&&v.generated_diffuse==proposed.generated_diffuse;},
            [&]{return std::make_shared<const Values>(std::move(proposed));},charge);
    }
};
struct Control {
    Mesh *mesh{};Material *roots{};std::size_t count{};
    std::thread::id executor=std::this_thread::get_id();std::uint64_t revision{};unsigned depth{};bool alive=true;
    bool own()const noexcept{return alive&&std::this_thread::get_id()==executor;}
    bool stable()const noexcept{return own()&&mesh->materials.data()==roots&&mesh->materials.size()==count;}
    void quiet()const {need(stable()&&!depth);}
};
struct Resident {
    std::shared_ptr<const std::vector<std::byte>> header;
    std::vector<std::shared_ptr<const std::vector<std::byte>>> passes;
    Layout layout;std::vector<std::size_t> children;
    bool generated_diffuse{};
};
using SchemaBytes=std::shared_ptr<const std::vector<std::byte>>;
class SchemaBank {
    std::unordered_map<std::size_t,std::vector<SchemaBytes>> entries_;
    std::size_t bytes_{};
public:
    SchemaBytes intern(std::vector<std::byte> bytes){
        const auto hash=std::hash<std::string_view>{}({reinterpret_cast<const char *>(bytes.data()),bytes.size()});
        auto &bucket=entries_[hash];for(const auto &prior:bucket)if(*prior==bytes)return prior;
        bytes_+=bytes.size();auto value=std::make_shared<const std::vector<std::byte>>(std::move(bytes));bucket.push_back(value);return value;
    }
    std::size_t bytes()const noexcept{return bytes_;}
};
// This table exists for one completed, callback-free capture only. Neither a
// pass identity alone nor validation from a previous batch authorizes a skip.
class PassValidationMemo {
    struct Key {const MaterialPass *actual{};const std::vector<std::byte> *expected{};bool operator==(const Key &)const=default;};
    struct Hash {std::size_t operator()(const Key &v)const noexcept{
        const auto a=std::hash<const void *>{}(v.actual),b=std::hash<const void *>{}(v.expected);
        return a^(b+0x9e3779b97f4a7c15ull+(a<<6)+(a>>2));
    }};
    std::unordered_set<Key,Hash> validated_;
public:
    std::size_t checked{},reused{};
    bool contains(const MaterialPass &actual,const SchemaBytes &expected){
        if(!validated_.contains({&actual,expected.get()}))return false;++reused;return true;
    }
    void remember(const MaterialPass &actual,const SchemaBytes &expected){validated_.insert({&actual,expected.get()});++checked;}
};
// A retained cut selects Values independently of its immutable child owners.
// Reuse is only a storage choice after this capture's complete validation;
// these records never authorize skipping a schema, child or cycle check.
struct Topology {
    Material *target{};std::shared_ptr<Material> pin;
    std::size_t resident{};
    Links links;
    Topology(Material &m,std::shared_ptr<Material> p,std::size_t s)
        :target(&m),pin(std::move(p)),resident(s),links(m){}
    bool matches(Material &m,const std::shared_ptr<Material> &p,std::size_t s)const noexcept {
        return target==&m&&resident==s&&same_link_owner(pin,p)&&same_link_owners(links,m);
    }
};
// Keep logical_bytes' former expanded layout independent of this private
// representation change. This type is used for accounting only, never stored.
struct ExpandedNodeLayout {
    Material *target{};std::shared_ptr<Material> pin;
    std::size_t resident{};
    std::shared_ptr<const Values> values;
    Links links;
};
struct Node {
    std::shared_ptr<const Topology> topology;
    std::shared_ptr<const Values> values;
    Node(std::shared_ptr<const Topology> t,std::shared_ptr<const Values> v)
        :topology(std::move(t)),values(std::move(v)){}
    Node(Material &m,std::shared_ptr<Material> p,std::size_t s,std::shared_ptr<const Values> v)
        :Node(std::make_shared<const Topology>(m,std::move(p),s),std::move(v)){}
    Material *target()const noexcept{return topology->target;}
    const std::shared_ptr<Material> &pin()const noexcept{return topology->pin;}
    std::size_t resident()const noexcept{return topology->resident;}
    const Links &links()const noexcept{return topology->links;}
};
struct Graph {
    std::vector<Node> nodes;std::vector<std::size_t> roots;
    MaterialPresentationContinuity::Stats stats;
};
template<bool Memo>bool schema_compare(const Material &m,const Resident &r,SchemaMismatch *mismatch,
    PassValidationMemo *memo=nullptr)noexcept(!Memo){
    if(layout(m)!=r.layout){if(mismatch)*mismatch={"child_layout",absent,absent,absent};return false;}
    Schema<false> compare{nullptr,*r.header};compare.header(m,r.generated_diffuse);
    if(!compare.equal||compare.cursor!=r.header->size()){
        if(compare.equal)compare.mismatch={"schema_extent",compare.cursor,absent,absent};
        if(mismatch)*mismatch=compare.mismatch;return false;
    }
    if(m.passes.size()!=r.passes.size()){if(mismatch)*mismatch={"passes.size",absent,absent,absent};return false;}
    std::size_t offset=r.header->size();
    for(std::size_t n=0;n<r.passes.size();++n){const auto &actual=m.passes[n];const auto &expected=r.passes[n];
        bool reused=false;if constexpr(Memo)reused=memo->contains(actual,expected);
        if(!reused){
            Schema<false> pass{nullptr,*expected};pass.pass_index=n;pass.pass(actual);
            if(!pass.equal||pass.cursor!=expected->size()){
                if(pass.equal)pass.mismatch={"schema_extent",pass.cursor,absent,n};
                if(pass.mismatch.offset!=absent)pass.mismatch.offset+=offset;
                if(mismatch)*mismatch=pass.mismatch;return false;
            }
            if constexpr(Memo)memo->remember(actual,expected);
        }
        offset+=expected->size();
    }
    if(mismatch)*mismatch={};return true;
}
bool schema_same(const Material &m,const Resident &r,SchemaMismatch *mismatch=nullptr)noexcept{
    return schema_compare<false>(m,r,mismatch);
}
bool graph_same(const Graph &graph,std::span<const Resident> resident,const Control &owner)noexcept {
    if(!owner.stable()||graph.roots.size()!=owner.count)return false;
    for(std::size_t n=0;n<graph.roots.size();++n)if(graph.nodes[graph.roots[n]].target()!=owner.roots+n)return false;
    for(const auto &node:graph.nodes)if(!node.values->same(*node.target())||!node.links().same(*node.target())||
        !schema_same(*node.target(),resident[node.resident()]))return false;return true;
}
}
struct MaterialPresentationContinuity::Impl {
    std::shared_ptr<Control> control;
    Limits limits;
    std::vector<Resident> resident;
    std::vector<std::size_t> roots;
    std::size_t schema_bytes{};
    mutable std::weak_ptr<const Graph> latest;
    Impl(Mesh &mesh,Limits policy,std::span<const unsigned> generated_slots):control(std::make_shared<Control>()),limits(policy){
        need(limits.nodes&&limits.links&&limits.depth&&limits.checkpoint_bytes);
        control->mesh=&mesh;control->roots=mesh.materials.data();control->count=mesh.materials.size();
        std::unordered_set<const Material *> generated;
        for(const auto slot:generated_slots){need(slot<control->count&&generated.insert(control->roots+slot).second);
            const auto &image=control->roots[slot].diffuse;
            need(image.width&&image.height&&!image.pixels.empty()&&image.pixels.size()<=limits.checkpoint_bytes);}
        std::unordered_map<const Material *,std::size_t> seen;std::unordered_set<const Material *> active;
        seen.reserve(control->count);active.reserve(limits.depth);
        SchemaBank schemas;
        // No source Material/MaterialPass owner is copied or pinned here.
        // The constructor is itself quiescent; an observed const record is
        // encoded once, then only its exact owning byte schema is retained.
        std::unordered_map<const MaterialPass *,SchemaBytes> observed_passes;
        std::size_t links{};
        const auto visit=[&](auto &self,const Material &m,std::size_t depth)->std::size_t{
            need(depth<=limits.depth&&!active.contains(&m));
            if(const auto at=seen.find(&m);at!=seen.end())return at->second;
            need(resident.size()<limits.nodes);const auto index=resident.size();seen.emplace(&m,index);active.insert(&m);resident.emplace_back();
            const bool generated_diffuse=generated.contains(&m);
            std::vector<std::byte> bytes;Schema<true> encode{&bytes};encode.header(m,generated_diffuse);
            Resident r{schemas.intern(std::move(bytes)),{},layout(m),{},generated_diffuse};r.passes.reserve(m.passes.size());
            for(const auto &pass:m.passes){const auto at=observed_passes.find(&pass);
                if(at!=observed_passes.end())r.passes.push_back(at->second);
                else{std::vector<std::byte> pass_bytes;Schema<true> pass_encode{&pass_bytes};pass_encode.pass(pass);
                    auto schema=schemas.intern(std::move(pass_bytes));observed_passes.emplace(&pass,schema);r.passes.push_back(std::move(schema));}}
            children(m,[&](const auto &p){need(++links<=limits.links);r.children.push_back(p?self(self,*p,depth+1):absent);});
            resident[index]=std::move(r);active.erase(&m);return index;
        };
        for(const auto &m:mesh.materials)roots.push_back(visit(visit,m,1));
        schema_bytes=schemas.bytes();
    }
    std::shared_ptr<const Graph> capture()const {
        auto result=std::make_shared<Graph>();result->stats.roots=roots.size();result->stats.resident_schema_bytes=schema_bytes;
        std::size_t root=absent,node=absent,schema_index=absent,current_depth{};
        const auto reject=[&](const char *reason,std::size_t requested=0,SchemaMismatch mismatch={}){
            const auto ordinal=[](std::size_t n){return n==absent?std::string("none"):std::to_string(n);};
            throw FormatError(std::string("Material presentation capture rejected: reason=")+reason+
                " root="+ordinal(root)+" node="+ordinal(node)+" resident="+ordinal(schema_index)+
                " roots="+std::to_string(roots.size())+" nodes="+std::to_string(result->nodes.size())+
                " resident_nodes="+std::to_string(resident.size())+" depth="+std::to_string(current_depth)+
                " links="+std::to_string(result->stats.links)+" logical_bytes="+std::to_string(result->stats.logical_bytes)+
                " unique_bytes="+std::to_string(result->stats.unique_bytes)+
                " new_value_bytes="+std::to_string(result->stats.new_value_bytes)+
                " shared_value_bytes="+std::to_string(result->stats.shared_value_bytes)+
                " generated_image_bytes="+std::to_string(result->stats.generated_image_bytes)+
                " requested_bytes="+std::to_string(requested)+" checkpoint_cap="+std::to_string(limits.checkpoint_bytes)+
                " schema_member="+mismatch.member+" schema_offset="+ordinal(mismatch.offset)+
                " field_offset="+ordinal(mismatch.field_offset)+" pass="+ordinal(mismatch.pass));
        };
        if(!control->own())reject("owner_or_executor");
        if(!control->stable())reject("root_storage");
        if(control->depth)reject("incomplete_mutation_batch");
        const auto prior=latest.lock();std::unordered_map<const Material *,const Node *> previous;
        if(prior)previous.reserve(prior->nodes.size());
        if(prior)for(const auto &n:prior->nodes)previous.emplace(n.target(),&n);
        ValueCapture values_capture;
        if(prior)for(const auto &n:prior->nodes)values_capture.seed(n.values);
        PassValidationMemo pass_validation;
        MutablePassComparisons mutable_passes;
        std::unordered_map<const Material *,std::size_t> seen;std::unordered_set<const Material *> active;
        seen.reserve(resident.size());active.reserve(limits.depth);result->nodes.reserve(resident.size());result->roots.reserve(roots.size());
        const auto charge=[&](std::size_t bytes){
            if(bytes>limits.checkpoint_bytes-result->stats.unique_bytes)reject("checkpoint_budget",bytes);
            result->stats.unique_bytes+=bytes;
        };
        const auto payload=[&](std::size_t bytes,bool inherited){
            charge(bytes);
            if(inherited)result->stats.shared_value_bytes+=bytes;else result->stats.new_value_bytes+=bytes;
        };
        const auto visit=[&](auto &self,Material &m,std::shared_ptr<Material> pin,std::size_t schema,std::size_t depth)->std::size_t{
            node=result->nodes.size();schema_index=schema;current_depth=depth;
            if(depth>limits.depth)reject("depth_limit");
            if(active.contains(&m))reject("graph_cycle");
            if(schema>=resident.size())reject("resident_index");
            if(const auto at=seen.find(&m);at!=seen.end()){
                node=at->second;if(result->nodes[at->second].resident()!=schema)reject("alias_resident");return at->second;
            }
            SchemaMismatch mismatch;
            if(!schema_compare<true>(m,resident[schema],&mismatch,&pass_validation))reject("resident_schema",0,mismatch);
            if(result->nodes.size()>=limits.nodes)reject("node_limit");
            const auto index=result->nodes.size();seen.emplace(&m,index);active.insert(&m);
            // Every reachable record is charged in this cut, even when older
            // cuts share its allocation. Reuse never makes the cap cheaper.
            charge(sizeof(Node)+sizeof(Topology));
            result->stats.logical_bytes+=sizeof(ExpandedNodeLayout)+expanded_bytes(m,resident[schema].generated_diffuse);
            if(resident[schema].generated_diffuse)result->stats.generated_image_bytes+=m.diffuse.pixels.size();
            std::shared_ptr<const Values> values;
            const auto at=previous.find(&m);const auto *old=at==previous.end()?nullptr:at->second;
            if(old&&old->values->same_capture(m,mutable_passes))values=old->values;
            if(values)values_capture.retain(values,payload);
            else values=values_capture.capture(m,resident[schema].generated_diffuse,payload);
            // Only this unpublished draft may contain a placeholder. Active
            // nodes reject before the seen lookup; completed aliases already
            // have their topology. Never keep a vector reference across DFS.
            result->nodes.emplace_back(std::shared_ptr<const Topology>{},std::move(values));
            std::size_t child{};children(m,[&](const auto &p){
                node=index;schema_index=schema;current_depth=depth;
                if(++result->stats.links>limits.links)reject("link_limit");
                result->stats.logical_bytes+=sizeof(std::shared_ptr<Material>);
                // Five fixed links are already inside Topology::Links. Only its
                // variable child vectors allocate additional graph payload.
                if(child>=2&&child<2+resident[schema].layout.before+resident[schema].layout.after+resident[schema].layout.bloom)
                    charge(sizeof(std::shared_ptr<Material>));
                const auto expected=resident[schema].children.at(child++);
                if(bool(p)!=(expected!=absent))reject("child_presence");
                if(p)(void)self(self,*p,p,expected,depth+1);});
            if(old&&old->topology->matches(m,pin,schema))result->nodes[index].topology=old->topology;
            else result->nodes[index].topology=std::make_shared<const Topology>(m,std::move(pin),schema);
            active.erase(&m);return index;
        };
        for(std::size_t n=0;n<roots.size();++n){root=n;result->roots.push_back(visit(visit,control->roots[n],{},roots[n],1));}
        root=node=schema_index=absent;current_depth=0;
        result->stats.nodes=result->nodes.size();const auto root_bytes=result->roots.size()*sizeof(std::size_t);
        charge(root_bytes+sizeof(Graph));result->stats.logical_bytes+=root_bytes;
        result->stats.static_passes_checked=pass_validation.checked;result->stats.static_passes_reused=pass_validation.reused;
        result->stats.mutable_passes_checked=mutable_passes.checked;result->stats.mutable_passes_reused=mutable_passes.reused;
        latest=result;return result;
    }
};
struct MaterialPresentationContinuity::Checkpoint::Data {std::weak_ptr<Control> owner;std::shared_ptr<const Graph> graph;};
struct MaterialPresentationContinuity::TargetGraph::Data {
    std::weak_ptr<Control> owner;
    const std::vector<Resident> *resident{};
    std::uint64_t revision{};
    std::shared_ptr<const Graph> target,observed;
    std::vector<Node> target_observed;
    struct Replacement {WritableValues values;MaterialPassBank passes;Links links;std::optional<Texture> generated_diffuse;
        explicit Replacement(const Node &node):values(*node.values),passes(std::as_const(*node.target()).passes),links(node.links()){
            need(passes.size()==node.values->passes->size());
            for(std::size_t n=0;n<passes.size();++n){passes[n].program=*(*node.values->passes)[n].program;passes[n].constants=*(*node.values->passes)[n].constants;}
            if(node.values->generated_diffuse)generated_diffuse=*node.values->generated_diffuse;
        }
    };
    std::vector<Replacement> replacement;
    bool consumed{};
    bool ready()const noexcept {
        const auto s=owner.lock();if(!s||!s->own()||s->depth||consumed||s->revision!=revision||
            revision==UINT64_MAX||!graph_same(*observed,*resident,*s))return false;
        for(const auto &node:target_observed)if(!node.values->same(*node.target())||!node.links().same(*node.target())||
            !schema_same(*node.target(),(*resident)[node.resident()]))return false;return true;
    }
    bool applied()const noexcept {
        const auto s=owner.lock();return s&&s->own()&&!s->depth&&consumed&&revision!=UINT64_MAX&&s->revision==revision+1&&
            graph_same(*target,*resident,*s);
    }
};
struct MaterialPresentationContinuity::Mutation::Data {
    std::weak_ptr<Control> owner;
    ~Data(){if(const auto s=owner.lock())if(s->own()&&s->depth)--s->depth;}
};
MaterialPresentationContinuity::MaterialPresentationContinuity(Mesh &mesh):MaterialPresentationContinuity(mesh,Limits{}){}
MaterialPresentationContinuity::MaterialPresentationContinuity(Mesh &mesh,Limits limits):MaterialPresentationContinuity(mesh,std::span<const unsigned>{},limits){}
MaterialPresentationContinuity::MaterialPresentationContinuity(Mesh &mesh,std::span<const unsigned> generated):MaterialPresentationContinuity(mesh,generated,Limits{}){}
MaterialPresentationContinuity::MaterialPresentationContinuity(Mesh &mesh,std::span<const unsigned> generated,Limits limits):impl_(std::make_unique<Impl>(mesh,limits,generated)){}
MaterialPresentationContinuity::~MaterialPresentationContinuity(){impl_->control->alive=false;}
void MaterialPresentationContinuity::mutating(){auto &s=*impl_->control;s.quiet();need(s.revision!=UINT64_MAX);++s.revision;}
MaterialPresentationContinuity::Mutation::Mutation(MaterialPresentationContinuity &owner){
    auto data=std::make_unique<Data>();owner.mutating();data->owner=owner.impl_->control;++owner.impl_->control->depth;data_=std::move(data);
}
MaterialPresentationContinuity::Mutation::~Mutation()=default;
MaterialPresentationContinuity::Mutation::Mutation(Mutation &&)noexcept=default;
MaterialPresentationContinuity::Mutation &MaterialPresentationContinuity::Mutation::operator=(Mutation &&)noexcept=default;
MaterialPresentationContinuity::Mutation MaterialPresentationContinuity::begin_mutation(){return Mutation(*this);}
bool MaterialPresentationContinuity::Checkpoint::valid()const noexcept{const auto s=data_?data_->owner.lock():nullptr;return s&&s->own();}
MaterialPresentationContinuity::Stats MaterialPresentationContinuity::Checkpoint::stats()const {need(valid());return data_->graph->stats;}
MaterialPresentationContinuity::Checkpoint MaterialPresentationContinuity::checkpoint()const {
    Checkpoint out;out.data_=std::make_shared<const Checkpoint::Data>(Checkpoint::Data{impl_->control,impl_->capture()});return out;
}
bool MaterialPresentationContinuity::owns_checkpoint(const Checkpoint &checkpoint)const noexcept{
    return checkpoint.valid()&&checkpoint.data_->owner.lock()==impl_->control;
}
MaterialPresentationContinuity::PreparedRestore::PreparedRestore()=default;
MaterialPresentationContinuity::PreparedRestore::~PreparedRestore()=default;
MaterialPresentationContinuity::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
MaterialPresentationContinuity::PreparedRestore &MaterialPresentationContinuity::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
MaterialPresentationContinuity::TargetGraph MaterialPresentationContinuity::PreparedRestore::target_graph()const{
    need(bool(data_));TargetGraph out;out.data_=data_;return out;
}
MaterialPresentationContinuity::PreparedRestore MaterialPresentationContinuity::prepare_restore(const Checkpoint &checkpoint)const{
    impl_->control->quiet();need(owns_checkpoint(checkpoint)&&impl_->control->revision!=UINT64_MAX);
    auto data=std::make_shared<TargetGraph::Data>();data->owner=impl_->control;data->resident=&impl_->resident;
    data->revision=impl_->control->revision;data->target=checkpoint.data_->graph;data->observed=impl_->capture();
    // A historical object can be detached but is still pinned by the cut.
    // Its immutable schema must remain intact before private pass-bank copies.
    data->replacement.reserve(data->target->nodes.size());
    ValueCapture observed_values;
    const auto observed_charge=[](std::size_t,bool){};
    for(const auto &node:data->target->nodes){need(schema_same(*node.target(),impl_->resident[node.resident()]));
        data->target_observed.emplace_back(*node.target(),node.pin(),node.resident(),
            observed_values.capture(*node.target(),impl_->resident[node.resident()].generated_diffuse,observed_charge));
        data->replacement.emplace_back(node);}
    PreparedRestore out;out.data_=std::move(data);return out;
}
bool MaterialPresentationContinuity::restore_ready(const PreparedRestore &prepared)const noexcept{
    return prepared.data_&&prepared.data_->owner.lock()==impl_->control&&prepared.data_->ready();
}
bool MaterialPresentationContinuity::commit_restore(PreparedRestore &prepared)noexcept{
    if(!restore_ready(prepared))return false;auto &p=*prepared.data_;
    for(std::size_t n=0;n<p.target->nodes.size();++n){auto &m=*p.target->nodes[n].target();auto &r=p.replacement[n];
        r.values.swap(m);r.passes.swap(m.passes);r.links.swap(m);
        if(r.generated_diffuse){static_assert(std::is_nothrow_swappable_v<Texture>);using std::swap;swap(*r.generated_diffuse,m.diffuse);}}
    ++impl_->control->revision;p.consumed=true;return true;
}
bool MaterialPresentationContinuity::TargetGraph::ready()const noexcept{return data_&&data_->ready();}
bool MaterialPresentationContinuity::TargetGraph::applied()const noexcept{return data_&&data_->applied();}
bool MaterialPresentationContinuity::TargetGraph::matches_environment(const Mesh &mesh,unsigned slot,const Material *pass,
    const std::shared_ptr<const VertexProgramBank> &bank)const noexcept{
    const auto s=data_?data_->owner.lock():nullptr;
    if(!s||!s->stable()||s->depth||&mesh!=s->mesh||slot>=data_->target->roots.size()||
        s->revision!=(data_->revision+(data_->consumed?1u:0u)))return false;
    const auto &root=data_->target->nodes[data_->target->roots[slot]];
    if(root.links().environment.get()!=pass)return false;
    if(!pass)return !bank;
    const auto at=std::find_if(data_->target->nodes.begin(),data_->target->nodes.end(),[&](const auto &n){return n.target()==pass;});
    return at!=data_->target->nodes.end()&&at->values->fields.vertex_bank==bank;
}
}
