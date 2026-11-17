#pragma once

int capital_ship_gpu(const std::filesystem::path& shaders,const std::filesystem::path& map,
                     const std::filesystem::path& report_path) {
    if(!std::filesystem::is_regular_file(map))return 77;
    Cache cache(map);Fixture gpu;gpu.initialize(shaders);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    using Json=nlohmann::json;
    Json report{{"map",map.string()},{"materials",Json::array()},
        {"scope","Actual introduction capital_ship_a/b complete shader and vertex identities with controlled source texture samples. Raw, source-depth and filtered roles retain native detail RGB and animated DST_ALPHA reflection mask. Native scalar tests separately prove the exact source expression."}};
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    for(auto& vertex:primary)vertex.color={0,0,0}; // Native model radiance is not unlit albedo.
    unsigned found{};
    for(const auto& tag:cache.tags()) {
        if(tag.group!="shad" || !tag.address ||
           (!tag.name.ends_with("\\capital_ship_a") && !tag.name.ends_with("\\capital_ship_b")))continue;
        ++found;auto material=cache.load_material(tag.datum);material.selected_shader_lod=0;
        require(prepare_dynamic_material(cache,material,{},true,0.f),"Capital ship source material did not prepare");
        const auto source=pt::qualified_surface(material);
        require(source && source->family==pt::SurfaceFamily::DynamicDetail,
            "Actual capital ship lost its complete native dynamic-detail surface");
        // This is the actual prepared constant, not a synthetic mask hole.
        const auto authored_mask_word=material.forward_program.words[24];
        require((authored_mask_word>>24)==0,"Capital ship authored zero-mask control changed");
        material.diffuse=solid_texture({80,120,160,255});material.detail=solid_texture({160,128,96,255});
        material.animated_textures[0].clear();material.animated_textures[1].clear();
        const ShaderVector first{80.f/255,120.f/255,160.f/255,1},second{160.f/255,128.f/255,96.f/255,1};
        gpu.primary_color(first);gpu.primary_color(second,1);native_uniform(gpu,material);
        auto scene=pt::build_scene(primary,std::span(&material,1));
        require((scene.materials[0].texture[3]&2048u)!=0,"Capital ship native DST_ALPHA mask was not admitted");
        const auto atlas=pt::material_surface(scene,0,{.31f,.63f});
        require(atlas && (*atlas)[3]==0,"Capital ship secondary surface lost its authored zero reflection mask");
        ShaderVector expected{};
        for(unsigned c=0;c<3;++c)expected[c]=std::clamp(2.f*first[c]*second[c],0.f,1.f);
        for(unsigned c=0;c<4;++c)require(std::abs((*atlas)[c]-expected[c])<2.e-6f,
            "Capital ship atlas dropped the exact detail color or destination alpha");
        // Isolate source albedo from optional reflection sampling noise.
        scene.materials[0].response[1]=scene.materials[0].response[2]=0;
        float maximum_error{};
        for(unsigned mode=0;mode<3;++mode) {
            if(mode==2)for(unsigned frame=0;frame<15;++frame)
                gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,false,true);
            const auto frame=gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,mode==1,mode==2);
            for(unsigned c=0;c<3;++c) {
                const float error=std::abs(mean(frame,c)-expected[c]*63.75f);
                maximum_error=std::max(maximum_error,error);
                require(error<1.2f,"Capital ship raw/depth/filtered color lost native detail");
            }
        }
        // Keep actual .4/.8 authored reflection strengths and roughness. A
        // black albedo, black sky and one light isolate the masked GGX lobe.
        material.diffuse=solid_texture({0,0,0,255});gpu.primary_color({0,0,0,1});
        material.forward_program.words[24]=(authored_mask_word&0xffffffu)|(128u<<24);
        native_uniform(gpu,material);
        const auto indirect_scene=pt::build_scene(primary,std::span(&material,1));
        const auto indirect_frame=gpu.draw(indirect_scene,primary,{.6f,.3f,.15f});
        std::uint64_t indirect_checksum{};for(const auto value:indirect_frame)indirect_checksum+=value;
        const float indirect_red=mean(indirect_frame,0,8,56,8,56);
        require(indirect_red>20.f && indirect_red<100.f,
            "Capital ship bounded direct policy lost its independent GGX reflection continuation");
        pt::Light unit_sun;unit_sun.direction=Vec3{0,0,-1};unit_sun.color={1,1,1};
        auto unit_scene=pt::build_scene(primary,std::span(&material,1),std::span(&unit_sun,1));
        require(material.specular_brightness==1.f,"Capital ship native direct-specular source gain changed");
        std::array<float,3> bounded_highlights{};std::array<unsigned,3> bounded_peaks{};
        for(unsigned mode=0;mode<3;++mode) {
            if(mode==2)for(unsigned frame=0;frame<15;++frame)
                gpu.draw(unit_scene,primary,{},Camera{},~0u,false,true);
            const auto frame=gpu.draw(unit_scene,primary,{},Camera{},~0u,mode==1,mode==2);
            bounded_highlights[mode]=mean(frame,0,28,36,28,36);
            for(unsigned y=28;y<36;++y)for(unsigned x=28;x<36;++x)for(unsigned channel=0;channel<3;++channel)
                bounded_peaks[mode]=std::max(bounded_peaks[mode],unsigned(frame[(y*width+x)*4+channel]));
        }
        std::cout<<"Capital ship direct envelope "<<tag.name<<" raw/depth/filtered="
            <<bounded_highlights[0]<<','<<bounded_highlights[1]<<','<<bounded_highlights[2]
            <<" indirect_checksum="<<indirect_checksum<<" indirect_red="<<indirect_red<<'\n';
        // At this near-normal receiver the actual authored F is .4. The
        // conservative legacy direct lobe may not amplify a unit source into
        // an area-normalized white cap (the previous GGX peak was about97).
        for(const auto value:bounded_highlights)require(value>80.f && value<104.f,
            "Capital ship direct GGX exceeds its authored highlight envelope");
        for(const auto value:bounded_peaks)require(value<104u,
            "Capital ship direct GGX peak exceeds its authored highlight envelope");
        auto colored_sun=unit_sun;colored_sun.color={1,.25f,.0625f};
        auto colored_scene=pt::build_scene(primary,std::span(&material,1),std::span(&colored_sun,1));
        const auto colored_frame=gpu.draw(colored_scene,primary);
        const std::array<float,3> colored_highlight{mean(colored_frame,0,28,36,28,36),
            mean(colored_frame,1,28,36,28,36),mean(colored_frame,2,28,36,28,36)};
        require(colored_highlight[0]>80.f && std::abs(colored_highlight[1]-colored_highlight[0]*.25f)<1.f &&
            std::abs(colored_highlight[2]-colored_highlight[0]*.0625f)<1.f,
            "Capital ship bounded highlight lost authored light color");
        auto blocked_geometry=primary;quad(blocked_geometry,-1,-1,1,1,-.5f,{0,0,1},0);
        auto blocked=pt::build_scene(blocked_geometry,std::span(&material,1),std::span(&unit_sun,1));
        const auto blocked_frame=gpu.draw(blocked,primary);
        const float blocked_highlight=mean(blocked_frame,0,28,36,28,36);
        require(blocked_highlight<.1f,"Capital ship bounded highlight ignored the traced blocker");
        const pt::Light light{{0,0,-2},10,{1,1,1},.1f};
        std::array<float,3> highlights{};
        for(unsigned mask=0;mask<3;++mask) {
            const unsigned alpha=mask==0?0u:mask==1?64u:128u;
            material.forward_program.words[24]=(authored_mask_word&0xffffffu)|(alpha<<24);
            native_uniform(gpu,material);
            auto reflected=pt::build_scene(primary,std::span(&material,1),std::span(&light,1));
            const auto surface=pt::material_surface(reflected,0,{.31f,.63f});
            const float expected_mask=std::min(2.f*float(alpha)/255.f,1.f);
            require(surface && std::abs((*surface)[3]-expected_mask)<2.e-6f,
                "Capital ship secondary mask did not follow authored C24 animation");
            for(unsigned mode=0;mode<3;++mode) {
                const auto frame=gpu.draw(reflected,primary,{},Camera{},~0u,mode==1,mode==2);
                const float value=mean(frame,0,28,36,28,36);
                if(mode==0)highlights[mask]=value;
                require(std::abs(value-highlights[mask])<1.f,
                    "Capital ship filtered/depth role changed its authored reflection mask");
                if(mask==0)require(value<.1f,"Capital ship zero native mask leaked a white specular highlight");
            }
        }
        require(highlights[2]>1.f,"Capital ship full-mask control did not produce a real specular highlight");
        require(std::abs(highlights[1]-highlights[2]*(128.f/255.f))<1.f,
            "Capital ship partial native mask lost its multiplicative specular response");
        // An ideal primary mirror sees the ship through an actual secondary
        // hit. Its zero-roughness direct lobe is absent, so this measures the
        // ship atlas mask in traceLighting rather than primary resolve.
        Material mirror;mirror.diffuse=solid_texture({0,0,0,255});
        std::array<Material,2> reflected_materials{mirror,material};
        auto secondary_geometry=primary;
        quad(secondary_geometry,-8,-8,8,8,-1.f,{0,0,1},1);
        auto secondary_light=light;secondary_light.position={0,0,-.5f};secondary_light.intensity=.1f;
        std::array<float,2> secondary_highlights{};
        for(unsigned mask=0;mask<2;++mask) {
            reflected_materials[1].forward_program.words[24]=(authored_mask_word&0xffffffu)|(mask?128u<<24:0u);
            auto reflected=pt::build_scene(secondary_geometry,reflected_materials,std::span(&secondary_light,1));
            reflected.materials[0].response={0,1,1,0};
            const auto frame=gpu.draw(reflected,primary);
            secondary_highlights[mask]=mean(frame,0,28,36,28,36);
        }
        std::cout<<"Capital ship mask controls primary="<<highlights[0]<<','<<highlights[1]<<','<<highlights[2]
            <<" secondary="<<secondary_highlights[0]<<','<<secondary_highlights[1]<<'\n';
        require(secondary_highlights[0]<.1f && secondary_highlights[1]>1.f,
            "Capital ship secondary hit ignored its native DST_ALPHA mask or lost the positive control");
        // Zero mask must restore diffuse energy as well as suppressing specular.
        material.forward_program.words[24]=authored_mask_word;
        material.diffuse=solid_texture({80,120,160,255});gpu.primary_color(first);native_uniform(gpu,material);
        auto bright_light=light;bright_light.intensity=1.f;
        auto masked=pt::build_scene(primary,std::span(&material,1),std::span(&bright_light,1));
        auto matte=masked;matte.materials[0].response[1]=matte.materials[0].response[2]=0;
        const auto masked_frame=gpu.draw(masked,primary),matte_frame=gpu.draw(matte,primary);
        for(unsigned c=0;c<3;++c)require(std::abs(mean(masked_frame,c)-mean(matte_frame,c))<.6f,
            "Capital ship zero reflection mask still depleted its diffuse color");
        report["materials"].push_back({{"name",tag.name},{"authored_mask_word",authored_mask_word},
            {"native_family",unsigned(source->family)},{"albedo_maximum_byte_error",maximum_error},
            {"zero_half_full_highlights",highlights},{"secondary_zero_full_highlights",secondary_highlights},
            {"bounded_direct_highlights",bounded_highlights},{"colored_direct_highlight",colored_highlight},
            {"bounded_direct_peak_bytes",bounded_peaks},
            {"occluded_direct_highlight",blocked_highlight},{"indirect_checksum",indirect_checksum},
            {"indirect_red",indirect_red},
            {"roughness",masked.materials[0].response[0]}});
    }
    require(found==2,"Capital ship regression did not find both actual introduction hull materials");
    require(gpu.validation_errors.load()==0,"Vulkan validation reported errors in capital ship material draws");
    report["passed"]=true;report["validation_errors"]=gpu.validation_errors.load();
    std::ofstream output(report_path);require(bool(output),"Cannot write capital ship GPU report");output<<report.dump(2)<<'\n';
    std::cout<<"PASS capital ship native detail/mask GPU: "<<report.dump()<<'\n';return 0;
}
