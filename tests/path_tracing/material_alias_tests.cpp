#include "halo2/path_tracing/material_alias.hpp"
#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/path_tracing/source_ambient.hpp"
#include "halo2/object_lighting.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
void require(bool value,const char* reason){if(!value)throw FormatError(reason);}
std::vector<Vertex> triangle(unsigned material=0) {
    std::vector<Vertex> result(3);result[0].position={-1,-1,0};result[1].position={1,-1,0};result[2].position={0,1,0};
    for(auto& v:result){v.material=material;v.normal={0,0,1};v.color={1,1,1};}
    return result;
}
WorldMaterialUniform uniform(const Material& material) {
    WorldMaterialUniform result;result.program=material.forward_program.words;
    result.forward_transforms=material.forward_transforms;result.diffuse=material.forward_diffuse;result.specular=material.forward_specular;
    return result;
}
}
int main(int argc,char** argv)try {
    if(argc!=3)throw FormatError("Supply prepared introduction cache and compact report path");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);unsigned tested{},samples{};float maximum{};
    nlohmann::json rows=nlohmann::json::array();
    for(const auto& tag:cache.tags()) {
        if(tag.group!="shad")continue;
        if(!tag.name.ends_with("\\capital_ship_a") && !tag.name.ends_with("\\capital_ship_b") &&
           !tag.name.ends_with("\\masterchief"))continue;
        auto root=cache.load_material(tag.datum);
        if(!prepare_dynamic_material(cache,root,{},true,0.f))continue;
        const auto family=pt::qualified_surface(root);
        if(!family || (family->family!=pt::SurfaceFamily::DynamicDetail && family->family!=pt::SurfaceFamily::TextureFourChangeColor))continue;
        if(tested>=8)break;
        auto base=triangle();auto scene=pt::build_scene(base,std::span(&root,1));
        if(root.texture_states[0].max_mip==0) {
            const auto& material=scene.materials[0];
            const auto record=std::bit_cast<std::uint32_t>(material.base[3])-1;
            require(record+scene.pixels.at(record+12)==material.texture[0],
                "Qualified native surface duplicated its base image in the ray atlas");
        }
        const auto root_key=scene.material_image_keys[0];const auto source_words=root.forward_program.words;
        auto a=uniform(root),b=a;
        a.program[24]=0x20000000;b.program[24]=0xb0000000;
        a.program[11]=0x00ef3820;b.program[11]=0x002038ef;
        a.program[12]=0x004f9fdf;b.program[12]=0x00df9f4f;
        a.forward_transforms[0]={1.3f,.8f,.11f,-.13f};b.forward_transforms[1]={.7f,1.7f,.23f,.09f};
        std::array inputs{pt::MaterialAliasInput{{0xe2000001u,0,~0u},&a},pt::MaterialAliasInput{{0xe3000001u,0,~0u},&b}};
        auto aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
        require(aliases.size()==2 && aliases[0].material!=aliases[1].material,"Shared material owners collapsed into one alias");
        const auto shared_size=scene.texture_texel_count;
        auto check=[&] {
            for(unsigned owner=0;owner<2;++owner) {
                const auto& completed=owner?b:a;
                auto expected=root;expected.forward_program.words=completed.program;expected.forward_transforms=completed.forward_transforms;
                auto oracle=pt::build_scene(base,std::span(&expected,1));
                for(unsigned n=0;n<32;++n) {
                    const Vec2 uv{float(n)*.137f-.8f,float(n)*.213f-.3f};
                    const auto actual=pt::material_surface(scene,aliases[owner].material,uv);
                    const auto reference=pt::material_surface(oracle,0,uv);
                    require(bool(actual)==bool(reference),"Owner surface alpha kill differed from its completed native source");
                    if(actual)for(unsigned c=0;c<4;++c) {
                        const auto error=std::abs((*actual)[c]-(*reference)[c]);maximum=std::max(maximum,error);
                        require(error<2.e-6f,"Owner alias C24/change colors/ordered UV differed from its completed native source");
                    }
                    ++samples;
                }
            }
        };
        check();const auto first_a=aliases[0].material,first_b=aliases[1].material;
        const auto key_a=scene.material_image_keys[first_a],key_b=scene.material_image_keys[first_b];
        // Source camera/fog bookkeeping does not change a surface or its history identity.
        a.source_camera_world[0][0]=17;a.fog_constants[1][2]=.3f;
        a.program[24]^=0x00010203u;a.program[11]^=0xff000000u; // Unconsumed color lanes.
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        std::swap(inputs[0],inputs[1]);aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
        require(aliases[0].material==first_a && aliases[1].material==first_b &&
            scene.material_image_keys[first_a]==key_a && scene.material_image_keys[first_b]==key_b,
            "Owner reorder or camera/fog state changed a stable live material identity");
        require(scene.texture_texel_count==shared_size,"Stable owners duplicated shared image storage");
        // Update one owner while preserving the other owner's exact completed shader state.
        a.program[24]^=0x60000000;a.program[11]^=0x00700000;a.forward_transforms[0][2]+=.5f;
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);check();
        require(scene.material_image_keys[first_a]!=key_a && scene.material_image_keys[first_b]==key_b,
            "One owner's new constants leaked into another shared-root owner");
        // Header growth moves roots and shared texels together, preserving positive
        // surface offsets and live IDs without copying every owner's source images.
        std::vector<pt::MaterialAliasInput> many(inputs.begin(),inputs.end());
        for(unsigned n=0;n<15;++n)many.push_back({{0xe4000000u+n,0,~0u},&b});
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        const auto before=scene.texture_texel_count,old_prefix=scene.material_alias_prefix_words;
        auto grown=pt::refresh_material_aliases(scene,std::span(&root,1),many);check();
        require(scene.texture_texel_count-before==scene.material_alias_prefix_words-old_prefix,
            "Owner growth duplicated shared source texture texels");
        require(grown[0].material==first_a && grown[1].material==first_b,"Header growth changed live owner material IDs");
        auto geometry=base;for(auto& v:geometry)v.material=first_a;pt::update_geometry(scene,geometry);
        require(pt::source_material(scene,scene.triangles.at(0).metadata[0])==0,"Alias geometry lost its native root provenance");
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        require(scene.triangles.at(0).metadata[0]==0,"Next-frame material refresh retained a stale alias geometry index");
        require(root.forward_program.words==source_words && scene.material_image_keys[0]==root_key,
            "Owner aliases mutated their shared authored source root");
        const auto* lighting_owner=pt::source_ambient_owner(root);
        require(lighting_owner,"Actual ship/armor lost its qualified dynamic lighting owner");
        auto lighting_a=*lighting_owner,lighting_b=*lighting_owner;
        ObjectLightingState state_a,state_b;state_a.ambient={.37f,.16f,.07f};state_b.ambient={.02f,.24f,.43f};
        apply_object_lighting(lighting_a,state_a);apply_object_lighting(lighting_b,state_b);
        auto ambient_a=uniform(lighting_a),ambient_b=uniform(lighting_b);
        inputs[0].ambient=&ambient_b;inputs[1].ambient=&ambient_a;
        aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
        require(scene.material_alias_images.size()==1,"Two ambient owners duplicated the same source bump image");
        for(unsigned n=0;n<2;++n) {
            const auto& material=scene.materials[aliases[n].material];
            const auto& ambient=n?ambient_b:ambient_a;
            require(material.color_guide[0]==3 && material.color_guide[2]==(ambient.program[15]&0xffffffu),
                "Secondary owner ambient changed the native packed RGB quantization");
            require(scene.pixels.at(material.color_guide[1])==scene.material_alias_images[0].image[0],
                "Secondary owner ambient did not share its source bump texels");
        }
        const auto image_size=scene.texture_texel_count;
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
        require(scene.texture_texel_count==image_size,"Next-frame ambient aliases appended duplicate source images");
        std::vector<pt::MaterialAliasInput> expanded(inputs.begin(),inputs.end());
        for(unsigned n=0;n<63;++n) {
            auto input=inputs[0];input.key.owner=0xe5000000u+n;expanded.push_back(input);
        }
        const auto prefix=scene.material_alias_prefix_words;
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        const auto relocated=pt::refresh_material_aliases(scene,std::span(&root,1),expanded);
        require(scene.material_alias_images.size()==1 && scene.texture_texel_count-image_size==scene.material_alias_prefix_words-prefix,
            "Header growth copied or lost the already-resident shared bump image");
        for(const auto& binding:relocated) {
            const auto& material=scene.materials[binding.material];
            require(scene.pixels.at(material.color_guide[1])==scene.material_alias_images[0].image[0],
                "Header growth left a stale absolute ambient image pointer");
        }
        // An actual completed descriptor may select another resident frame.
        // Both owners borrow that same image and retain independent metadata.
        for(auto& input:inputs)input.surface_images[1]=&root.diffuse;
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
        for(unsigned n=0;n<2;++n) {
            const auto& completed=n?b:a;auto expected=root;
            expected.forward_program.words=completed.program;expected.forward_transforms=completed.forward_transforms;
            expected.detail=root.diffuse;expected.animated_textures[1].clear();
            auto oracle=pt::build_scene(base,std::span(&expected,1));
            require(pt::material_surface(scene,aliases[n].material,{.31f,.72f})==pt::material_surface(oracle,0,{.31f,.72f}),
                "Completed owner image override retained the shared root's different frame");
        }
        inputs[1].key.owner=0xe6000001u; // Same datum index, a new native generation.
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        const auto replaced=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
        require(replaced.size()==2 && std::none_of(scene.material_alias_keys.begin(),scene.material_alias_keys.end(),
            [](const auto& key){return key.owner==0xe2000001u;}),
            "A retired native object generation retained an active material alias");
        if(!tested) {
            pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
            scene.pixels.resize(16'777'216);scene.texture_texel_count=std::uint32_t(scene.pixels.size());
            const Texture next{1,1,11,{std::byte{9},std::byte{27},std::byte{63},std::byte{255}}};
            auto animated=inputs;
            for(auto& input:animated)input.surface_images[1]=&next;
            const auto compacted=pt::refresh_material_aliases(scene,std::span(&root,1),animated);
            require(scene.texture_texel_count<16'777'216 && scene.material_alias_images.size()==2 &&
                compacted[0].material==replaced[0].material && compacted[1].material==replaced[1].material,
                "Alias image pressure left a retired animation frame or changed live owner IDs");
            for(const auto& binding:compacted) {
                const auto& material=scene.materials[binding.material];
                const auto record=std::bit_cast<std::uint32_t>(material.base[3])-1;
                require(scene.pixels.at(record+scene.pixels.at(record+22))==0xff3f1b09u &&
                    scene.pixels.at(material.color_guide[1])<scene.texture_texel_count,
                    "Compaction broke the new frame or surviving owner lighting map pointer");
            }
            pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
            (void)pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
        }
        const auto unchanged_policy=scene.material_image_keys[replaced[0].material];
        auto policy=root;policy.forward_raster.alpha_test=!root.forward_raster.alpha_test;
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&policy,1));
        const auto policy_aliases=pt::refresh_material_aliases(scene,std::span(&policy,1),inputs);
        require(scene.material_image_keys[policy_aliases[0].material]!=unchanged_policy,
            "An owner alpha visibility policy changed without changing its consumed-state fingerprint");
        pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
        auto bad=a;bad.program[34]^=1;auto invalid=inputs;invalid[1].primary=&bad;
        bool rejected=false;try{(void)pt::refresh_material_aliases(scene,std::span(&root,1),invalid);}catch(const FormatError&){rejected=true;}
        require(rejected,"Unsupported completed native controls reused an old surface graph");
        rows.push_back({{"name",tag.name},{"family",unsigned(family->family)},{"owners",many.size()},
            {"shared_texels",shared_size},{"alias_cpu_bytes",pt::material_alias_cpu_bytes(scene)}});++tested;
    }
    require(tested==3,"Actual capital_ship_a/b and Master Chief owner fixtures were not found");
    nlohmann::json report{{"passed",true},{"materials",rows},{"sample_count",samples},{"maximum_error",maximum}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
