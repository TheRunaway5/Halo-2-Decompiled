#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/source_world_depth_input.hpp"
#include "halo2/source_world_depth_position_contract.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
template<class T>T read(std::ifstream& input) {
    T value;need(bool(input.read(reinterpret_cast<char*>(&value),sizeof(value))),"Truncated actual camera bank");return value;
}
SourceRuntimeMemory snapshot(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);need(bool(input),"Actual retained camera bank is absent");SourceRuntimeMemory memory;
    while(input.peek()!=std::ifstream::traits_type::eof()) {
        const auto at=read<std::uint32_t>(input),size=read<std::uint32_t>(input);
        std::vector<std::byte> bytes(size);need(bool(input.read(reinterpret_cast<char*>(bytes.data()),size)),"Truncated actual bank segment");
        memory.mount(at,bytes,SourceRuntimeMemory::Access::ReadWrite,"Actual retained source camera reference");
    }
    return memory;
}
template<class T>T get(const SourceRuntimeMemory& memory,std::uint32_t at) {
    T value;std::memcpy(&value,memory.view(at,sizeof(value)).data(),sizeof(value));return value;
}
std::array<std::uint32_t,60> controls(const Material& material) {
    auto result=material.forward_program.words;std::fill(result.begin()+10,result.begin()+26,0);
    result[43]=result[44]=0;return result;
}
Json rejected=Json::array();
template<class F>void reject(const char* name,F fn) {
    try{fn();}catch(const FormatError& error){rejected.push_back({{"name",name},{"reason",error.what()}});return;}
    throw FormatError(std::string("Invalid bound-page control was admitted: ")+name);
}
int run(int argc,char**argv) {
    need(argc==5,"Supply original Lockout, actual640 camera bank, compiled shaders and JSON report");
    for(unsigned n=1;n<=2;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
    auto memory=snapshot(argv[2]);Cache cache(argv[1]);auto mesh=cache.load_bsp(0);
    SourceBspResidentPlan resident(cache,mesh,0,SourceBspResidentLighting::classification3);
    const auto geometry=cache.load_source_bsp_admission(0);SourceBspAdmission admission;
    for(unsigned n=0;n<geometry.clusters.size();++n)admission.clusters.push_back({n});
    for(unsigned n=0;n<geometry.instances.size();++n)admission.instances.push_back({n});
    const auto camera_bank=get<SourceViewProjectionBank>(memory,0x4c6634u);
    auto selection=resident.select({camera_bank,admission,0,480,1,false,{}});
    const auto chosen=std::find_if(selection.selected_parts.begin(),selection.selected_parts.end(),[&](const auto& part) {
        const auto &material=mesh.materials.at(part.material);
        return material.forward_pass && material.forward_raster.cull==0x900 &&
            source_world_depth_private::position_program_admitted(material.vertex_program) &&
            std::any_of(material.animated_textures.begin(),material.animated_textures.end(),[](const auto& pages){return !pages.empty();});
    });
    need(chosen!=selection.selected_parts.end(),"Actual Lockout has no admitted bitmap-page part");
    const auto selected=*chosen;selection.selected_parts={selected};
    for(unsigned n=0;n<selection.bindings.size();++n) {
        const auto &key=resident.parts()[n].original;
        if(key.geometry!=selected.geometry || key.section!=selected.section || key.part!=selected.part)
            selection.bindings[n].material.reset();
    }
    SDL_SetMainReady();need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    struct Sdl{~Sdl(){SDL_Quit();}} sdl;
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("Native bound bitmap page controls",0,0,640,480,
        SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    need(bool(window),SDL_GetError());
    Renderer renderer(window.get(),mesh,true,{},nullptr,{},nullptr,argv[3]);
    renderer.scene_bsp_atmosphere(cache.load_bsp_atmosphere(0));
    auto targets=renderer.target_resources();auto use=targets.acquire();
    auto owner=source_world_depth_input_tu15(memory,use,cache,mesh,resident,selection);
    auto foreign_owner=owner;
    auto foreign_window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("Independent native attachment lease",0,0,640,480,
        SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    need(bool(foreign_window),SDL_GetError());
    Mesh simple;simple.materials.resize(1);simple.vertices.resize(3);
    Renderer foreign_renderer(foreign_window.get(),simple,true,{},nullptr,{},nullptr,argv[3]);
    auto foreign_targets=foreign_renderer.target_resources();auto foreign_use=foreign_targets.acquire();
    struct RetirePresentations {
        Renderer &first,&second;bool active=true;
        void release(){first.release_presentation();second.release_presentation();active=false;}
        ~RetirePresentations(){if(active){try{first.release_presentation();}catch(...){}try{second.release_presentation();}catch(...){}}}
    } retire{renderer,foreign_renderer};
    ModelView view;view.camera.position=get<Vec3>(memory,0x4c65c0u);view.camera.forward=get<Vec3>(memory,0x4c65ccu);
    view.camera.up=get<Vec3>(memory,0x4c65d8u);view.camera.oriented=true;view.camera.field_of_view=90;
    view.width=640;view.height=480;view.full_surface=true;view.vertices=owner.selected_geometry();view.atmosphere=BspFogSample{};
    SourceViewShaderConstants bank;std::copy_n(owner.raster(use).camera.begin(),7,bank.world.begin());view.source_view_shader_constants=bank;
    view.source_world_depth=&owner;view.source_world_depth_targets=&use;
    std::optional<SourceWorldDepthFrameReceipt> retained;unsigned observed{};
    view.completed_material_upload_observer=[&](const RendererCompletedMaterialUpload& upload) {
        const auto* receipt=upload.source_depth_frame_receipt;if(!receipt)return;
        const auto material=receipt->source_material();const auto identity=receipt->identity();
        const auto images=receipt->images();const auto frames=receipt->frames();const auto samplers=receipt->samplers();const auto pipeline=controls(*material);
        const auto validate=[&](const auto& image,const auto& frame,const auto& sampler,const SourceWorldDepthFrameReceipt* token,unsigned slot) {
            owner.validate_uploaded_material(use,identity,material,pipeline,image,sampler,frame,token,slot);
        };
        validate(images,frames,samplers,receipt,receipt->expanded_slot());++observed;retained=*receipt;
        reject("missing current producer receipt",[&]{validate(images,frames,samplers,nullptr,receipt->expanded_slot());});
        reject("foreign expanded material slot",[&]{validate(images,frames,samplers,receipt,receipt->expanded_slot()+1);});
        reject("foreign SourceDepth owner",[&]{foreign_owner.validate_uploaded_material(use,identity,material,pipeline,images,samplers,frames,receipt,receipt->expanded_slot());});
        reject("foreign genuine attachment lease",[&]{owner.validate_uploaded_material(foreign_use,identity,material,pipeline,images,samplers,frames,receipt,receipt->expanded_slot());});
        auto wrong_frames=frames;++wrong_frames[1];
        reject("frame differs from completed CPU/GPU owner",[&]{validate(images,wrong_frames,samplers,receipt,receipt->expanded_slot());});
        auto wrong_sampler=samplers;wrong_sampler[1].address_u^=1u;
        reject("foreign stage sampler",[&]{validate(images,frames,wrong_sampler,receipt,receipt->expanded_slot());});
        auto pages=images;std::swap(pages[1],pages[2]);
        reject("foreign stage page",[&]{validate(pages,frames,samplers,receipt,receipt->expanded_slot());});
        pages=images;pages[1].pixel_fingerprint^=1u;
        reject("changed page pixels",[&]{validate(pages,frames,samplers,receipt,receipt->expanded_slot());});
        pages=images;++pages[1].mip_count;
        reject("changed page mip count",[&]{validate(pages,frames,samplers,receipt,receipt->expanded_slot());});
        pages=images;pages[1].source_image=!pages[1].source_image;
        reject("changed page source ownership",[&]{validate(pages,frames,samplers,receipt,receipt->expanded_slot());});
        pages=images;pages[1].mutated=true;
        reject("mutated selected page",[&]{validate(pages,frames,samplers,receipt,receipt->expanded_slot());});
        auto shape=*images[1].shape;pages=images;pages[1].shape=&shape;
        reject("substituted page shape owner",[&]{validate(pages,frames,samplers,receipt,receipt->expanded_slot());});
    };
    bool submitted{};for(unsigned n=0;n<5 && !submitted;++n)submitted=renderer.draw(view.camera,{}, {},false,{},{},{},std::span(&view,1));
    need(submitted && observed && retained.has_value(),"Actual native material upload emitted no current frame receipt");
    const auto validate_retained=[&] {
        owner.validate_uploaded_material(use,retained->identity(),retained->source_material(),controls(*retained->source_material()),
            retained->images(),retained->samplers(),retained->frames(),&*retained,retained->expanded_slot());
    };
    reject("receipt expired after synchronous upload",validate_retained);
    retire.release();
    reject("retired genuine attachment lease",validate_retained);
    Json report{{"passed",true},{"observed_uploads",observed},{"source_shader",mesh.materials.at(selected.material).name},
        {"root_slot",selected.material},{"controls",rejected},
        {"scope","Actual authored Lockout single-page bitmap chains, retained real camera and genuine current Vulkan page/lease/epoch; explicit selected-part admission, no live visibility, nonzero animation-frame or reference framebuffer parity claim"}};
    const auto text=report.dump(2)+"\n";need(text.size()<1024*1024,"Page receipt report exceeds its bound");
    std::ofstream output(argv[4]);output<<text;need(bool(output),"Could not write page receipt report");
    std::cout<<"PASS actual native bound bitmap pages: "<<observed<<" uploads, "<<rejected.size()<<" owner/frame/page/epoch/lease controls\n";return 0;
}
}
int main(int argc,char**argv)try {
    std::ostringstream validation;auto* previous=std::cerr.rdbuf(validation.rdbuf());int result;
    try{result=run(argc,argv);}catch(...){std::cerr.rdbuf(previous);std::cerr<<validation.str();throw;}
    std::cerr.rdbuf(previous);need(validation.str().find("Vulkan validation:")==std::string::npos,"Native page receipt Vulkan validation diagnostics");return result;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
