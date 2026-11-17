#pragma once
#include "../scalar_combiners.hpp"

int vanilla_surface_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders);gpu.renderer->set_quality(1,1);
    Material material;material.forward_pass=true;material.forward_vertex=3;material.forward_pass_index=0;
    material.passes.resize(1);material.passes[0].vertex_shader="rasterizer\\vertex_shaders_xbox\\stencil_texture";
    for(const auto word:pt::unlit_uv_detail::texture_words)for(unsigned byte=0;byte<4;++byte)
        material.vertex_program.push_back(std::byte(word>>(byte*8)));
    material.passes[0].vertex_code=material.vertex_program;
    material.forward_transforms[0]={1,1,0,0};material.forward_transforms[1]={1,1,0,0};
    material.forward_transforms[2]={1,1,1,1};
    std::array<ShaderVector,4> samples{{{80.f/255,120.f/255,160.f/255,1},
        {160.f/255,128.f/255,96.f/255,1},{0,0,0,0},{.2f,.4f,.6f,.8f}}};
    material.diffuse=solid_texture({80,120,160,255});material.detail=solid_texture({160,128,96,255});
    material.auxiliary=solid_texture({51,102,153,204});
    for(unsigned stage:{0u,1u,3u})gpu.primary_color(samples[stage],stage);
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    for(auto& vertex:primary)vertex.color={0,0,0}; // Source model radiance is not albedo.
    float maximum{};
    for(unsigned family=0;family<3;++family) {
        material.passes[0].name=std::string("shaders\\shader_passes\\texture_mapping\\")+
            (family==0?"default_texture_zwrite":family==1?"default_texture_two_change_color_zwrite":"default_texture_four_change_color_zwrite");
        material.forward_program.words=pt::surface_detail::texture_controls(family==0?1:family==1?4:7);
        auto& words=material.forward_program.words;words[10]=0xff0000;words[18]=0xff00;
        words[11]=0x00c04020;words[12]=0x0020a0e0;words[13]=0x0080c060;words[14]=0x00d09030;
        words[43]=0x00a0c0e0;
        native_uniform(gpu,material);
        auto scene=pt::build_scene(primary,std::span(&material,1));
        const auto expected=halo2_test::scalar_combiners(words,samples);
        const auto atlas=pt::material_surface(scene,0,{.3f,.6f});
        require(atlas.has_value(),"Vanilla armor source atlas rejected an opaque sample");
        for(unsigned c=0;c<4;++c)require(std::abs((*atlas)[c]-expected[c])<2.e-6f,
            "Vanilla armor secondary atlas dropped native detail or change colors");
        for(unsigned mode=0;mode<3;++mode) {
            const auto frame=gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,mode==1,mode==2);
            for(unsigned c=0;c<3;++c) {
                const float error=std::abs(mean(frame,c)-expected[c]*63.75f);maximum=std::max(maximum,error);
                require(error<1.2f,"Raw/visibility/filtered vanilla armor lost full-resolution native source color");
            }
        }
        if(family==2) {
            const auto old_key=scene.material_image_keys[0];words[13]^=0x00700000u;
            pt::refresh_materials(scene,std::span(&material,1));
            require(scene.material_image_keys[0]!=old_key,"Third change-color animation reused a stale secondary surface atlas");
        }
    }
    // Isolate a current authored red reflection lobe against a black scene.
    // With no diffuse color, sky or mesh emission, only the shadow-tested
    // point-light GGX term can make this receiver nonzero.
    material.passes[0].name="shaders\\shader_passes\\texture_mapping\\default_texture_zwrite";
    material.forward_program.words=pt::surface_detail::texture_controls(1);material.forward_program.words[43]=0xffffff;
    material.diffuse=solid_texture({0,0,0,255});material.detail=solid_texture({128,128,128,255});
    material.environment_pass=std::make_shared<Material>();material.environment_pass->forward_vertex=4;
    material.environment_pass->forward_blend=8;
    material.environment_pass->forward_diffuse={1,.25f,.125f,.3f};
    material.environment_pass->forward_specular={1,.25f,.125f,.3f};
    gpu.primary_color({0,0,0,1});gpu.primary_color({128.f/255,128.f/255,128.f/255,1},1);native_uniform(gpu,material);
    // Unit source radiance keeps the authored .3 direct envelope measurable
    // without depending on the former area-normalized GGX peak amplification.
    const pt::Light light{{0,0,-2},10,{1,1,1},1.f};
    auto scene=pt::build_scene(primary,std::span(&material,1),std::span(&light,1));
    require((scene.materials[0].texture[3]&2048u)!=0,"Vanilla DST_ALPHA reflection mask was not retained");
    const auto lit=gpu.draw(scene,primary);const auto filtered=gpu.draw(scene,primary,{},Camera{},~0u,false,true);
    require(mean(lit,0)>5 && mean(lit,0)>mean(lit,1)*3,
        "Authored colored point-light highlight was absent or gained an invented white coat");
    require(std::abs(mean(lit,0)-mean(filtered,0))<3,"Direct specular vanished in the denoised lighting resolve");
    auto geometry=primary;quad(geometry,-2,-2,2,2,-.5f,{0,0,-1},0);
    auto blocked=pt::build_scene(geometry,std::span(&material,1),std::span(&light,1));
    require(mean(gpu.draw(blocked,primary),0)<1,"Occluded point light retained an unshadowed specular highlight");
    gpu.primary_color({0,0,0,0});
    require(mean(gpu.draw(scene,primary,{},Camera{},~0u,false,true),0)<1,
        "Denoised specular filled a full-resolution authored reflection-mask hole");
    // A reflection-mask hole must restore the diffuse lobe too. Filtering
    // unmasked (1-F) diffuse would leave a dark spot after specular is masked.
    material.diffuse=solid_texture({64,64,64,0});gpu.primary_color({64.f/255,64.f/255,64.f/255,0});
    auto strong_light=light;strong_light.intensity=1;
    const auto masked_scene=pt::build_scene(primary,std::span(&material,1),std::span(&strong_light,1));
    const auto masked_diffuse=gpu.draw(masked_scene,primary);
    auto matte=material;matte.environment_pass.reset();
    const auto matte_scene=pt::build_scene(primary,std::span(&matte,1),std::span(&strong_light,1));
    const auto matte_diffuse=gpu.draw(matte_scene,primary);
    require(std::abs(mean(masked_diffuse,0)-mean(matte_diffuse,0))<.6f && mean(matte_diffuse,0)>10,
        "Full-resolution reflection mask left diffuse darker than the equivalent matte source");
    require(gpu.validation_errors.load()==0,"Vulkan validation reported errors in vanilla material/highlight draws");
    std::cout<<"PASS vanilla GPU albedo/detail/colors max_error="<<maximum<<" colored_highlight="<<mean(lit,0)
        <<" filtered_highlight="<<mean(filtered,0)<<" shadow/mask=black\n";
    return 0;
}
