#include "helpers/native_tu15_startup_test.hpp"
#include "halo2/renderer.hpp"
#include "helpers/native_tu15_coordinate_gpu.hpp"
#include "helpers/native_tu15_texture_pair_gpu.hpp"
#include "helpers/native_tu15_renderer_aux_gpu.hpp"
#include "helpers/native_tu15_debug_texture_gpu.hpp"
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
int main(int argc,char** argv)try {
    if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
    auto* window=SDL_CreateWindow("Native TU1.5 graphics startup",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    if(!window)throw std::runtime_error(SDL_GetError());
    int result;bool palette_probe=false,palette_temporary=false,depth_probe=false,coordinate_probe=false,texture_pair_probe=false,aux_texture_probe=false,debug_texture_probe=false;unsigned coordinate_base=0;
    nlohmann::json depth_samples=nlohmann::json::array(),coordinate_samples=nlohmann::json::array();
    nlohmann::json texture_pair_samples;
    nlohmann::json aux_texture_samples;
    nlohmann::json debug_texture_samples;
    std::optional<halo2::RendererTexturePairs> retained_texture_pairs;
    std::optional<halo2::RendererAuxTextures> retained_aux_textures;
    std::optional<halo2::RendererDebugTexture> retained_debug_texture;
    std::filesystem::path capture_path,palette_output;
    for(int i=1;i+1<argc;i+=2) {
        if(std::string_view(argv[i])=="--frontier" && std::string_view(argv[i+1])=="graphics-aux-textures")aux_texture_probe=true;
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-debug-storage" || std::string_view(argv[i+1])=="graphics-renderer-complete" || std::string_view(argv[i+1])=="render-object-storage" || std::string_view(argv[i+1])=="sound-storage" || std::string_view(argv[i+1])=="sound-sdk"))debug_texture_probe=true;
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-texture-pairs" || std::string_view(argv[i+1])=="graphics-target-storage"))texture_pair_probe=true;
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-palette" || std::string_view(argv[i+1])=="graphics-constants" || std::string_view(argv[i+1])=="graphics-depth" || std::string_view(argv[i+1])=="graphics-depth-state" || std::string_view(argv[i+1])=="graphics-output" || (std::string_view(argv[i+1])=="graphics-coordinates" || (std::string_view(argv[i+1])=="graphics-surfaces" || (std::string_view(argv[i+1])=="graphics-texture-pairs" || std::string_view(argv[i+1])=="graphics-target-storage")))))palette_probe=true;
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-depth" || std::string_view(argv[i+1])=="graphics-depth-state" || std::string_view(argv[i+1])=="graphics-output" || (std::string_view(argv[i+1])=="graphics-coordinates" || (std::string_view(argv[i+1])=="graphics-surfaces" || (std::string_view(argv[i+1])=="graphics-texture-pairs" || std::string_view(argv[i+1])=="graphics-target-storage")))))depth_probe=true;
        if(std::string_view(argv[i])=="--frontier" && (std::string_view(argv[i+1])=="graphics-coordinates" || (std::string_view(argv[i+1])=="graphics-surfaces" || (std::string_view(argv[i+1])=="graphics-texture-pairs" || std::string_view(argv[i+1])=="graphics-target-storage"))))coordinate_probe=true;
        if(std::string_view(argv[i])=="--capture")capture_path=argv[i+1];
    }
    if(palette_probe) {
        palette_temporary=capture_path.empty();
        palette_output=palette_temporary?std::filesystem::temp_directory_path()/("halo2-palette-gpu-"+
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())):
            capture_path.parent_path()/(capture_path.stem().string()+"-palette-gpu");
        std::filesystem::create_directories(palette_output);
    }
    if(depth_probe && SDL_setenv("HALO2_DEPTH_DIAGNOSTICS","1",1))throw std::runtime_error("Cannot enable actual depth readback");
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    std::string gpu;halo2::RendererDeviceLease observed;halo2::RendererPaletteFactory observed_palettes;halo2::RendererConstantMode observed_constants;halo2::RendererDepthMode observed_depth;halo2::RendererOutputState observed_output;halo2::RendererTextureCoordinates observed_coordinates;halo2::RendererTexturePairFactory observed_texture_pairs;halo2::RendererAuxTextureFactory observed_aux_textures;halo2::RendererDebugTextureFactory observed_debug_texture;
    try {
        halo2::Mesh mesh;halo2::Material material;
        halo2::BspAtmosphere depth_atmosphere;
        depth_atmosphere.gradient={1,1,11,{std::byte{0},std::byte{0},std::byte{0},std::byte{0}}};
        material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);
        if(depth_probe) {
            // Saturated red remains an unambiguous owner color through the
            // existing world bloom pass; this fixture tests depth admission.
            auto second=material;second.diffuse.pixels={std::byte{255},std::byte{0},std::byte{0},std::byte{255}};
            mesh.materials.push_back(second);
        }
        if(palette_probe)mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},
            {{4,2,-1},{1,1,1},{0,0},0},{{4,0,2},{1,1,1},{0,0},0}};
        if(coordinate_probe)coordinate_base=halo2_test::append_coordinate_probe(mesh);
        const auto* selected=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        halo2::Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,
            selected?std::filesystem::path(selected):std::filesystem::path{});
        gpu=renderer.device_name();
        auto lease=renderer.device_lease();
        observed=lease;
        if(lease.acquire().native_address().value<=0xffffffffu)
            throw std::runtime_error("Native graphics device validation requires an actual address above 4GiB");
        auto palettes=renderer.palette_factory();observed_palettes=palettes;
        auto constants=renderer.constant_mode_control();observed_constants=constants;
        auto depth=renderer.depth_mode_control();observed_depth=depth;
        auto output=renderer.output_state_control();observed_output=output;
        auto coordinates=renderer.texture_coordinate_control();observed_coordinates=coordinates;
        observed_texture_pairs=renderer.texture_pair_factory();
        observed_aux_textures=renderer.aux_texture_factory();
        observed_debug_texture=renderer.debug_texture_factory();
        halo2::Camera camera;camera.field_of_view=60;
        if(palette_probe && !renderer.draw(camera,palette_output/"before.ppm"))
            throw std::runtime_error("Native palette baseline draw failed");
        result=halo2::run_native_tu15_startup_test(argc,argv,std::move(lease),palettes,constants,depth,output,coordinates,renderer.target_resources(),renderer.texture_pair_factory(),renderer.aux_texture_factory(),renderer.debug_texture_factory());
        if(!result && debug_texture_probe) {
            debug_texture_samples=halo2_test::debug_texture_gpu_probe(observed_debug_texture);
            retained_debug_texture.emplace(observed_debug_texture.create());
        }
        if(!result && aux_texture_probe) {
            const auto* fixture=std::getenv("HALO2_NATIVE_AUX_TEXTURE_FIXTURE");
            aux_texture_samples=halo2_test::renderer_aux_gpu_probe(observed_aux_textures,fixture?std::filesystem::path(fixture):std::filesystem::path(HALO2_NATIVE_AUX_TEXTURE_FIXTURE));
            retained_aux_textures.emplace(observed_aux_textures.create());
        }
        if(!result && palette_probe) {
            for(unsigned stage=0;stage<4;++stage)if(!palettes.bound(stage).valid())
                throw std::runtime_error("Native palette stage has no live renderer color resource");
            const std::array<std::byte,1> index{std::byte{0}};
            renderer.update_palettized_image(0,0,index,1,1,false);
            if(!renderer.draw(camera,palette_output/"after.ppm"))throw std::runtime_error("Native palette draw failed");
            std::ifstream before(palette_output/"before.ppm",std::ios::binary),after(palette_output/"after.ppm",std::ios::binary);
            const std::vector<char> a{std::istreambuf_iterator<char>(before),{}},b{std::istreambuf_iterator<char>(after),{}};
            if(a.empty() || a==b)throw std::runtime_error("Native palette color resource did not reach the GPU image");
        }
        if(!result && depth_probe) {
            renderer.scene_bsp_atmosphere(depth_atmosphere);
            // Two overlapping triangles, near then far. A constant source c3
            // makes their integer W equal (LEQUAL lets the second win). A
            // position-dependent c3 makes the far triangle fail the same test.
            std::vector<halo2::Vertex> overlap=mesh.vertices;
            for(auto vertex:mesh.vertices) {
                vertex.position=vertex.position*2.f;vertex.material=1;overlap.push_back(vertex);
            }
            halo2::ModelView view;view.camera=camera;view.width=640;view.height=480;
            view.full_surface=true;view.vertices=overlap;view.atmosphere=halo2::BspFogSample{};
            view.source_view_shader_constants=halo2::SourceViewShaderConstants{};
            for(unsigned n=0;n<2;++n) {
                const auto expected=n?65536u:123456u;
                view.source_view_shader_constants->world[3]=n?halo2::ShaderVector{16384,0,0,0}:halo2::ShaderVector{0,0,0,123456.5f};
                const auto path=palette_output/("depth-"+std::to_string(n)+".ppm");
                if(!renderer.draw(camera,path,{},false,{}, {},{},std::span(&view,1)))
                    throw std::runtime_error("Native W-depth probe draw failed");
                std::ifstream input(path.string()+".depth-d24-low24.bin",std::ios::binary|std::ios::ate);
                if(input.tellg()!=640*480*4)throw std::runtime_error("Native W-depth readback has a different extent");
                input.seekg((240*640+320)*4);std::array<unsigned char,4> bytes{};
                input.read(reinterpret_cast<char*>(bytes.data()),4);
                const auto actual=std::uint32_t(bytes[0])|(std::uint32_t(bytes[1])<<8)|(std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<24);
                if(!input || actual!=expected)throw std::runtime_error("Actual native integer W depth differs: "+std::to_string(actual));
                std::ifstream ppm(path,std::ios::binary);std::string magic;unsigned width,height,maximum;
                ppm>>magic>>width>>height>>maximum;ppm.get();
                if(magic!="P6" || width!=640 || height!=480 || maximum!=255)throw std::runtime_error("Native W-depth color readback has a different format");
                ppm.seekg((240*640+320)*3,std::ios::cur);std::array<unsigned char,3> rgb{};
                ppm.read(reinterpret_cast<char*>(rgb.data()),3);
                const auto color=n?std::array<unsigned char,3>{126,126,255}:std::array<unsigned char,3>{255,0,0};
                if(!ppm || rgb!=color)throw std::runtime_error("Actual native W-depth write/LEQUAL occlusion differs");
                depth_samples.push_back({{"raw_d24",actual},{"expected_d24",expected},{"rgb",rgb},{"expected_rgb",color},
                    {"case",n?"far W rejected":"equal W accepted"},{"pixel",{320,240}}});
            }
        }
        if(!result && coordinate_probe)coordinate_samples=halo2_test::coordinate_gpu_probe(renderer,coordinate_base,palette_output);
        if(!result && texture_pair_probe) {
            texture_pair_samples=halo2_test::texture_pair_gpu_probe(observed_texture_pairs);
            retained_texture_pairs.emplace(observed_texture_pairs.create());
            const auto identity=retained_texture_pairs->image(0).image;
            SDL_SetWindowSize(window,320,240);renderer.resized();
            if(!renderer.draw(camera) || retained_texture_pairs->image(0).image!=identity ||
                retained_texture_pairs->image(0).width!=64)
                throw std::runtime_error("Independent texture pair changed during presentation recreation");
            texture_pair_samples["survives_presentation_recreation"]=true;
        }
    }catch(...) {retained_texture_pairs.reset();std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();
    if(retained_texture_pairs) {
        bool rejected=false;try{retained_texture_pairs->image(0);}catch(const std::runtime_error&){rejected=true;}
        if(retained_texture_pairs->valid()||!rejected)throw std::runtime_error("Retired renderer retained accessible texture-pair images");
        // Release the retained native implementation while its SDL window
        // still exists, then destroy that window below.
        retained_texture_pairs.reset();texture_pair_samples["retained_device_cleanup_after_retirement"]=true;
    }
    if(retained_aux_textures) {
        bool rejected=false;try{retained_aux_textures->image(0);}catch(const std::runtime_error&){rejected=true;}
        if(retained_aux_textures->valid()||!rejected)throw std::runtime_error("Retired renderer retained accessible auxiliary textures");
        retained_aux_textures.reset();aux_texture_samples["retained_device_cleanup_after_retirement"]=true;
    }
    if(retained_debug_texture) {
        bool rejected=false;try{retained_debug_texture->image();}catch(const std::runtime_error&){rejected=true;}
        if(retained_debug_texture->valid()||!rejected)throw std::runtime_error("Retired renderer retained an accessible debugger texture");
        retained_debug_texture.reset();debug_texture_samples["retained_device_cleanup_after_retirement"]=true;
    }
    SDL_DestroyWindow(window);SDL_Quit();
    if(observed_debug_texture.valid())throw std::runtime_error("Departed renderer retained a debugger texture factory");
    if(observed_aux_textures.valid())throw std::runtime_error("Departed renderer retained an auxiliary texture factory");
    if(aux_texture_probe) {
        bool retired=false;try{observed_aux_textures.create();}catch(const std::runtime_error&){retired=true;}
        if(!retired)throw std::runtime_error("Retired renderer created auxiliary textures");
    }
    if(observed_texture_pairs.valid())throw std::runtime_error("Departed renderer retained a live texture-pair factory");
    if(texture_pair_probe) {
        bool retired=false;try{observed_texture_pairs.create();}catch(const std::runtime_error&){retired=true;}
        if(!retired)throw std::runtime_error("Retired renderer created another texture pair");
    }
    if(observed_coordinates.valid())throw std::runtime_error("Departed renderer retained a live coordinate control");
    if(observed_output.valid())throw std::runtime_error("Departed renderer retained a live output-state control");
    if(observed_depth.valid())throw std::runtime_error("Departed renderer retained a live depth-mode control");
    if(observed_constants.valid())throw std::runtime_error("Departed renderer retained a live constant-mode control");
    if(observed_palettes.valid())throw std::runtime_error("Departed renderer retained a live palette factory");
    if(palette_temporary)std::filesystem::remove_all(palette_output);
    if(observed.valid())throw std::runtime_error("Departed renderer retained its source device lease");
    if(diagnostics.str().find("Vulkan validation:")!=std::string::npos)
        throw std::runtime_error("Native graphics startup emitted Vulkan validation messages");
    if(!result)for(int i=1;i+1<argc;i+=2)if(std::string_view(argv[i])=="--capture") {
        std::ifstream input(argv[i+1]);auto report=nlohmann::json::parse(input);input.close();
        report["native_vulkan_validation_enabled"]=true;report["native_vulkan_validation_messages"]=0;
        if(aux_texture_probe){report["native_aux_texture_gpu"]=aux_texture_samples;report["native_aux_texture_factory_retirement_verified"]=true;}
        if(debug_texture_probe){report["native_debug_texture_gpu"]=debug_texture_samples;report["native_debug_texture_factory_retirement_verified"]=true;}
        if(texture_pair_probe){report["native_texture_pair_gpu"]=texture_pair_samples;report["native_texture_pair_factory_retirement_verified"]=true;}
        if(palette_probe) {
            report["native_palette_gpu_upload_draw_verified"]=true;
            report["native_palette_factory_retirement_verified"]=true;
            report["native_palette_gpu_images"]=palette_output.string();
        }
        if(depth_probe) {report["native_depth_gpu_samples"]=depth_samples;report["native_depth_gpu_attachment_and_occlusion_verified"]=true;report["native_depth_control_retirement_verified"]=true;}
        if(coordinate_probe) {report["native_coordinate_gpu_samples"]=coordinate_samples;report["native_coordinate_gpu_stage_identity_verified"]=true;report["native_coordinate_control_retirement_verified"]=true;}
        report["native_output_control_retirement_verified"]=true;
        report["native_gpu_name"]=gpu;report["native_renderer_retirement_verified"]=true;report["native_constant_control_retirement_verified"]=true;
        std::ofstream output(argv[i+1]);output<<report.dump(2)<<'\n';
        if(!output)throw std::runtime_error("Cannot retain native Vulkan validation evidence");
    }
    return result;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
