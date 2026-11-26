#pragma once
#include "halo2/object_lighting.hpp"
#include "halo2/path_tracing/material_alias.hpp"
#include "halo2/world_material.hpp"
#include "../scalar_combiners.hpp"

namespace {
ShaderVector armory_source_sample(const PanelSourceTexture& image,const ShaderTextureState& sampler,Vec2 uv) {
    const auto address=[](int texel,int extent,unsigned mode) {
        if(mode==3)return std::clamp(texel,0,extent-1);
        const int period=extent*(mode==2?2:1);
        const int wrapped=(texel%period+period)%period;
        return wrapped>=extent?period-1-wrapped:wrapped;
    };
    const auto texel=[&](int x,int y) {
        const auto column=std::size_t(address(x,int(image.width),sampler.address_u));
        const auto row=std::size_t(address(y,int(image.height),sampler.address_v));
        const auto* pixel=static_cast<const std::uint8_t*>(image.upload.mapped)+(row*image.width+column)*4;
        return ShaderVector{pixel[0]/255.f,pixel[1]/255.f,pixel[2]/255.f,pixel[3]/255.f};
    };
    const float u=uv.x*image.width,v=uv.y*image.height;
    if(source_texture_filtering(sampler).mag_filter==1)
        return texel(int(std::floor(u)),int(std::floor(v)));
    const float x=u-.5f,y=v-.5f;
    const int left=int(std::floor(x)),top=int(std::floor(y));
    const ShaderVector tl=texel(left,top),tr=texel(left+1,top),bl=texel(left,top+1),br=texel(left+1,top+1);
    ShaderVector sample{};
    for(unsigned c=0;c<sample.size();++c)
        sample[c]=std::lerp(std::lerp(tl[c],tr[c],x-left),std::lerp(bl[c],br[c],x-left),y-top);
    return sample;
}
}

int armory_dynamic_gpu(const std::filesystem::path& shaders,const std::filesystem::path& map,
                      const std::filesystem::path& report_path) {
    if(!std::filesystem::is_regular_file(map))return 77;
    Cache cache(map);
    Fixture gpu;gpu.initialize(shaders);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    const std::array<Vec3,6> black{};
    using Json=nlohmann::json;
    Json report{{"map",map.string()},{"materials",Json::array()},
        {"scope","Actual Armory material programs and authored textures on controlled receivers. The Original scalar combiner isolates current class-2 and fixed class-3 ambient with no scene lights. Gameplay, model pose and campaign camera are outside this fixture."}};
    for(const std::string_view suffix:{"\\marine_standard","\\equipment_cart"})for(bool live:{true,false}) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& entry) {
            return entry.group=="shad" && entry.address && entry.name.ends_with(suffix);
        });
        require(tag!=cache.tags().end(),"Prepared Armory cache lost the requested dynamic material");
        auto root=cache.load_material(tag->datum);root.selected_shader_lod=0;
        if(live) {
            require(prepare_dynamic_material(cache,root,{},true,0.f) && pt::source_ambient_owner(root)==&root,
                "Armory material did not retain an authenticated dynamic ambient pass");
        } else {
            require(prepare_default_object_material(cache,root,0.f),"Armory default object material did not prepare");
            const auto surface=pt::qualified_surface(root);
            require(surface && surface->default_object_ambient,
                "Armory material did not retain an authenticated default ambient pass");
        }
        std::array<PanelSourceTexture,3> images;
        for(unsigned stage=0;stage<images.size();++stage)images[stage].bind(gpu,root,stage);
        PrimaryAmbientOwners descriptors(gpu);
        const std::array sets{descriptors.sets[0],descriptors.sets[0],descriptors.sets[1],descriptors.sets[1]};
        std::vector<Vertex> primary;
        quad(primary,-1,-1,0,1,.5f,{0,0,-1},0);
        quad(primary,0,-1,1,1,.5f,{0,0,-1},0);
        for(auto& vertex:primary)vertex.color={0,0,0}; // Model radiance is separate from authored albedo.
        // Isolate the native ambient transfer from the optional finish response.
        root.source_surface_material=SourceSurfaceMaterial::Unknown;
        const auto roots=std::span(&root,1);
        auto scene=pt::build_scene(primary,roots);
        const std::array<std::array<Vec3,2>,2> owner_colors{{
            {{{.38f,.19f,.09f},{.13f,.31f,.27f}}},
            {{{.07f,.14f,.38f},{.13f,.31f,.27f}}}}};
        Json measurements=Json::array();
        std::vector<std::uint8_t> previous;
        for(const auto& colors:owner_colors) {
            std::array<WorldMaterialUniform,2> uniforms{};
            std::array<pt::MaterialAliasInput,2> inputs{};
            for(unsigned owner=0;owner<uniforms.size();++owner) {
                auto material=root;
                ObjectLightingState state{};state.ambient=colors[owner];
                apply_object_lighting(material,state);
                auto& uniform=uniforms[owner];
                uniform.program=material.forward_program.words;
                uniform.forward_transforms=material.forward_transforms;
                const auto completed=live?pt::source_ambient_uniform(uniform,uniform):uniform;
                std::memcpy(descriptors.buffers[owner].mapped,&completed,sizeof(completed));
                inputs[owner].key={0xe2000001u+owner,0,~0u};
                inputs[owner].primary=&uniform;
                if(live)inputs[owner].ambient=&uniform;
            }
            pt::clear_material_aliases(scene,roots.size());
            const auto aliases=pt::refresh_material_aliases(scene,roots,inputs);
            require(aliases.size()==2 && aliases[0].material!=aliases[1].material,
                "Armory lighting owners did not receive independent material aliases");
            for(unsigned v=0;v<primary.size();++v)primary[v].material=aliases[v/6].material;
            pt::update_geometry(scene,primary);
            float max_error{},max_changed{},max_unchanged{},peak{};
            double squared_error{};
            unsigned samples{};
            const auto frame=gpu.draw(scene,primary,{},Camera{},~0u,false,true,&black,true,false,true,nullptr,sets);
            for(unsigned owner=0;owner<uniforms.size();++owner) {
                auto program=uniforms[owner].program;
                // Suppress the independent diffuse and specular inputs in the native combiner.
                if(live)for(unsigned word:{12u,14u,20u})program[word]&=0xff000000u;
                else for(unsigned word:{11u,19u})program[word]&=0xff000000u;
                const auto& transforms=uniforms[owner].forward_transforms;
                const auto transform=[](Vec2 uv,const ShaderVector& t) {
                    return Vec2{uv.x*t[0]+t[2],uv.y*t[1]+t[3]};
                };
                for(unsigned y=4;y<height-4;++y)for(unsigned x=4;x<width/2-4;++x) {
                    const auto uv=transform({(x+.5f)/(width/2),(y+.5f)/height},transforms[0]);
                    const std::array<ShaderVector,4> texels{
                        armory_source_sample(images[0],root.texture_states[0],uv),
                        armory_source_sample(images[1],root.texture_states[1],transform(uv,transforms[1])),
                        armory_source_sample(images[2],root.texture_states[2],transform(uv,transforms[2])),{}};
                    const auto expected=halo2_test::scalar_combiners(program,texels);
                    const auto at=(y*width+x+owner*(width/2))*4;
                    for(unsigned c=0;c<3;++c) {
                        const float value=frame[at+c];
                        const float error=value-expected[c]*255.f;
                        max_error=std::max(max_error,std::abs(error));
                        squared_error+=double(error)*error;++samples;
                        peak=std::max(peak,value);
                        if(!previous.empty()) {
                            float& change=owner?max_unchanged:max_changed;
                            change=std::max(change,std::abs(value-previous[at+c]));
                        }
                    }
                }
            }
            const auto rms_error=std::sqrt(squared_error/samples);
            measurements.push_back({{"maximum_byte_error",max_error},{"rms_byte_error",rms_error},{"peak_byte",peak},
                {"changed_owner_maximum_delta",max_changed},{"unchanged_owner_maximum_delta",max_unchanged}});
            // Hardware can quantize bilinear interpolation weights. Bound
            // both peak and aggregate error independently of that rounding.
            if(max_error>=3.f || rms_error>=1.0 || peak<=(live?15.f:3.f))
                throw FormatError("Armory GPU "+root.name+" diverged from native ambient: "+measurements.dump());
            require(previous.empty() || (max_unchanged==0.f && (live?max_changed>10.f:max_changed==0.f)),
                "Changing Armory object lights did not preserve live and default owner behavior");
            previous=frame;
        }
        report["materials"].push_back({{"name",root.name},{"lighting_class",live?2:3},{"frames",measurements}});
    }
    require(gpu.validation_errors.load()==0,"Armory GPU dynamic-detail fixture reported Vulkan errors");
    report["passed"]=true;report["validation_errors"]=gpu.validation_errors.load();
    std::ofstream output(report_path);require(bool(output),"Cannot write Armory GPU report");
    output<<report.dump(2)<<'\n';
    std::cout<<"PASS Armory native ambient GPU "<<report.dump()<<'\n';
    return 0;
}
