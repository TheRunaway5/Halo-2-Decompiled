#include "halo2/source_live_shadow_phase_inputs_tu15.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_view_projection.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why) {if(!value)throw std::runtime_error(why);}
template<class T>T input(std::ifstream& file) {
    T value;check(bool(file.read(reinterpret_cast<char*>(&value),sizeof(value))),"truncated phase fixture");return value;
}
SourceRuntimeMemory snapshot(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);check(bool(file),"actual retained bank missing");SourceRuntimeMemory memory;
    while(file.peek()!=std::ifstream::traits_type::eof()) {
        const auto address=input<std::uint32_t>(file),size=input<std::uint32_t>(file);
        std::vector<std::byte> bytes(size);check(bool(file.read(reinterpret_cast<char*>(bytes.data()),size)),"truncated actual bank");
        memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadWrite,"actual retained TU1.5 native frame clone");
    }
    return memory;
}
template<class T>T get(const SourceRuntimeMemory& memory,std::uint32_t address) {
    T value;std::memcpy(&value,memory.view(address,sizeof(value)).data(),sizeof(value));return value;
}
template<class T>void put(SourceRuntimeMemory& memory,std::uint32_t address,const T& value) {
    memory.write(address,std::as_bytes(std::span(&value,1)));
}
Json controls=Json::array();
template<class F>void reject(const char* name,F fn,const char* expected) {
    try{fn();}catch(const std::exception& error) {
        check(std::string(error.what()).find(expected)!=std::string::npos,"negative control hit an unrelated rejection");
        controls.push_back({{"name",name},{"rejected",true},{"reason",error.what()}});return;
    }
    throw std::runtime_error(std::string("unsupported live phase control admitted: ")+name);
}
struct Selected {
    SourceShadowPhase phase;unsigned vertex_class;
    MenuWorldSourcePass pass;SourceShadowMaterial material;
};
std::vector<Selected> selected(const Cache& cache,const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);check(bool(file),"actual selected-descriptor fixture missing");
    const auto header=input<std::array<std::uint32_t,7>>(file);
    check(header[0]==0x53524853 && header[1]==1 && header[2]==18,"actual selected descriptor schema changed");
    file.seekg(240+header[4]*4,std::ios::cur);std::vector<Selected> result;
    for(unsigned n=0;n<header[2];++n) {
        const auto info=input<std::array<std::uint32_t,5>>(file);
        const auto descriptor=input<std::array<std::uint32_t,6>>(file);
        const auto command=input<std::array<std::uint32_t,6>>(file);
        (void)input<std::array<std::uint32_t,8>>(file);file.seekg(info[4]*48,std::ios::cur);
        const auto phase=info[0]?SourceShadowPhase::Apply:SourceShadowPhase::Generate;
        const MenuWorldSourcePassChoice choice{command[5]>>8&255u,info[0]?11u:10u,command[5]&255u,info[0]?4u:0u};
        auto pass=load_menu_world_source_pass(cache,command[2],choice);
        check(pass.implementation_address()==descriptor[4],"actual descriptor lost literal cache implementation identity");
        auto authored=cache.load_material(command[2]);authored.selected_shader_lod=choice.shader_lod;
        auto prepared=prepare_shadow_material(cache,authored,phase,true,false,std::bit_cast<float>(header[6]));
        const auto found=std::find_if(prepared.begin(),prepared.end(),[&](const auto& material) {
            const auto& m=material.material;const auto& p=m.passes.at(m.forward_pass_index);
            return p.template_pass==pass.template_pass() && p.implementation==choice.implementation;
        });check(found!=prepared.end(),"actual selected pass absent from completed material");
        result.push_back({phase,descriptor[2]&65535u,std::move(pass),std::move(*found)});
    }
    check(file.peek()==std::ifstream::traits_type::eof(),"actual descriptor fixture has extra data");return result;
}
std::vector<std::byte> file_bytes(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);check(bool(f),"map missing");
    std::vector<std::byte> b(std::size_t(f.tellg()));f.seekg(0);check(bool(f.read(reinterpret_cast<char*>(b.data()),b.size())),"map truncated");return b;
}
void file_bytes(const std::filesystem::path& path,std::span<const std::byte> bytes) {
    std::ofstream f(path,std::ios::binary);check(bool(f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size())),"negative fixture write failed");
}
std::size_t file_offset(std::span<const std::byte> raw,std::uint32_t address) {
    //Exact Cache constructor address conversion; diagnostic modified assets
    //never stand in for actual reference bytes or the unmodified positive.
    const Bytes b(raw);const auto index=b.u32(16);
    const auto header_magic=std::int64_t(b.u32(index))-(std::int64_t(index)+32);
    const auto table=std::int64_t(b.u32(index+8))-header_magic;
    const auto magic=std::int64_t(b.u32(std::size_t(table)+8))-(std::int64_t(index)+b.u32(20));
    const auto offset=std::int64_t(address)-magic;check(offset>=0 && std::uint64_t(offset)<raw.size(),"negative metadata pointer escapes actual file");return std::size_t(offset);
}
int run(int argc,char** argv) {
    check(argc==7,"usage: test map actual640-bank actual960-bank actual-v21-descriptors shader-directory output-directory");
    const std::filesystem::path output=argv[6];check(!std::filesystem::exists(output),"fresh probe output required");std::filesystem::create_directories(output);
    auto memory=snapshot(argv[2]);auto unsupported=snapshot(argv[3]);Cache cache(argv[1]);auto passes=selected(cache,argv[4]);
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());struct Sdl{~Sdl(){SDL_Quit();}} sdl;
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("Actual TU1.5 shadow phase owner",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    check(bool(window),SDL_GetError());Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{64},std::byte{96},std::byte{128},std::byte{255}}, {}};mesh.materials.push_back(material);
    mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{0,0},0},{{4,0,2},{1,1,1},{0,0},0}};
    auto renderer=std::make_unique<Renderer>(window.get(),mesh,true,std::span<const SourceShadowPlanMaterial>{},nullptr,std::function<void()>{},nullptr,argv[5]);
    auto targets=renderer->target_resources();auto use=targets.acquire();const auto& bank=use.bank();
    SourceLiveShadowPhaseResidueTu15 residue;
    for(unsigned i=0;i<residue.constants.size();++i)for(unsigned lane=0;lane<4;++lane)residue.constants[i][lane]=float(1000+i*4+lane);
    residue.fog_parameters={.25f,.125f}; //Explicit UNUSED residue, not sourceSDK1,1.
    auto owner=source_live_shadow_phase_inputs_tu15(memory,use,residue);
    reject("actual960 source against genuine640 attachment",[&]{(void)source_live_shadow_phase_inputs_tu15(unsupported,use,residue);},"full640x480");
    const auto camera=owner.camera(use);const auto projection=get<SourceViewProjectionBank>(memory,0x4c6634u);
    const auto expected=source_view_shader_bank(projection,get<float>(memory,0x491e7cu),get<Vec3>(memory,0x491e38u),{0,0,640,480},false);
    check(std::memcmp(camera.data(),expected.data(),sizeof(camera))==0,"actual main camera composition changed");
    Json report{{"authority","OG Xbox TU1.5"},{"actual640_bank",argv[2]},{"actual960_negative_bank",argv[3]},
        {"native_device",use.device_address().value},{"generation",bank.generation},{"color_format",bank.color.front().format},
        {"depth_format",bank.depth_stencil.format},{"depth_samples",bank.depth_stencil.samples},{"selected_records",passes.size()},
        {"timed_effect_source",get<std::uint32_t>(memory,0x515ec0u)},
        {"timed_effect_native",memory.native_address(get<std::uint32_t>(memory,0x515ec0u),0x3fc).value},
        {"camera_words",Json::array()},{"phases",Json::array()},{"live_gpu_submission",false},
        {"reference_framebuffer_parity",false},{"phase_field_patching_in_positive",false}};
    for(const auto& vector:camera)for(float value:vector)report["camera_words"].push_back(std::bit_cast<std::uint32_t>(value));
    for(unsigned n=0;n<passes.size();++n) {
        const auto& selected=passes[n];const bool apply=selected.phase==SourceShadowPhase::Apply;
        const auto& phase=owner.phase(use,cache,selected.phase,selected.pass,selected.vertex_class,selected.material);
        check(phase.controls.cull==0x900 && phase.controls.depth_clamp,"actual source cull/W-clamp changed");
        check(phase.controls.surface_clip==(apply?std::array<std::uint32_t,4>{0,0,640,480}:std::array<std::uint32_t,4>{0,0,128,128}),"actual source surface clip changed");
        check(phase.surface_size==(apply?std::array<float,2>{640,480}:std::array<float,2>{128,128}),"actual source surface dimensions changed");
        check(!phase.raster.window_exclusive && phase.raster.surface_scale==std::array<std::int32_t,2>{1,1} && !phase.raster.stencil.enabled && phase.raster.source_stencil_mode==0,"actual source raster controls changed");
        const std::array<std::int32_t,4> region=apply?std::array<std::int32_t,4>{0,0,641,481}:std::array<std::int32_t,4>{0,0,129,129};
        for(const auto& r:phase.raster.regions)check(r==region,"source inclusive window is confused with physical surface clip");
        check(phase.raster.depth_factor_offset==(apply?std::array<float,2>{0,0}:std::array<float,2>{2,550}),"actual source offset teardown changed");
        check(phase.clip_range==std::array<float,4>{0,16777215,255.99998474121094f,16777215},"actual ordered source W/D24 clip changed");
        check(phase.fog_parameters==residue.fog_parameters && phase.constants[37]==residue.constants[37],"explicit unused residue was silently replaced");
        check(std::memcmp(phase.constants.data(),camera.data(),sizeof(camera))==0,"phase replaced actual main camera with shadow viewport constants");
        report["phases"].push_back({{"record",n},{"phase",apply?"Apply":"Generate"},{"class",selected.vertex_class},
            {"surface_clip",phase.controls.surface_clip},{"surface_size",phase.surface_size},{"window",region},
            {"offset",phase.raster.depth_factor_offset},{"clip",phase.clip_range}});
    }
    const auto change=[&](const char* label,std::uint32_t address,const auto& value,const char* reason) {
        const auto previous=get<std::remove_cvref_t<decltype(value)>>(memory,address);put(memory,address,value);
        try{reject(label,[&]{(void)source_live_shadow_phase_inputs_tu15(memory,use,residue);},reason);}catch(...){put(memory,address,previous);throw;}
        put(memory,address,previous);
    };
    change("nonordinary mode",0x491e22u,std::int16_t(1),"ordinary unscaled");change("scaled view",0x491e27u,std::uint8_t(1),"ordinary unscaled");
    change("global viewport alias changed",0x4c65f0u,std::array<std::int16_t,4>{0,0,480,639},"full640x480");
    change("submitting viewport alias changed",0x491e68u,std::array<std::int16_t,4>{0,0,240,640},"full640x480");
    change("split players",0x4c686cu,std::int32_t(2),"full640x480");
    change("submitting eye alias changed",0x491e38u,Vec3{},"aliases differ");
    change("global near alias changed",0x4c6600u,0.f,"aliases differ");
    change("config far alias changed",0x4922f8u,512.f,"aliases differ");
    change("missing timed-effect body",0x515ec0u,std::uint32_t(0),"allocated game arena");
    const auto effect=get<std::uint32_t>(memory,0x515ec0u);
    for(unsigned offset:{0xacu,0x3cu,0x40u,0x190u,0x194u})change("unresolved timed boundary/override",effect+offset,std::uint32_t(1),"unresolved source owner");
    change("requested main AA changed",0x4117e0u,std::uint32_t(2),"AA zero");
    change("retained offscreen AA changed",0x4117e4u,std::uint32_t(2),"AA zero");
    change("nonfinite projection bank",0x4c6634u,std::numeric_limits<float>::quiet_NaN(),"projection bank is not finite");
    change("projection/eye alias changed",0x4c6634u+0x34u+0x28u,Vec3{},"projection and submitting eye");
    auto bad_residue=residue;bad_residue.constants[37][0]=std::numeric_limits<float>::quiet_NaN();
    reject("nonfinite constant residue",[&]{(void)source_live_shadow_phase_inputs_tu15(memory,use,bad_residue);},"constant residue");
    bad_residue=residue;bad_residue.fog_parameters[1]=std::numeric_limits<float>::infinity();
    reject("nonfinite fog residue",[&]{(void)source_live_shadow_phase_inputs_tu15(memory,use,bad_residue);},"FogParams residue");
    const auto& first=passes.front();const auto access=[&](const SourceShadowMaterial& m) {return owner.phase(use,cache,first.phase,first.pass,first.vertex_class,m);};
    reject("unknown phase enum",[&]{(void)owner.phase(use,cache,static_cast<SourceShadowPhase>(2),first.pass,first.vertex_class,first.material);},"two-phase");
    reject("unknown class",[&]{(void)owner.phase(use,cache,first.phase,first.pass,7,first.material);},"identity");
    for(unsigned state:{146u,147u,154u,155u}) {
        auto m=first.material;m.material.passes.at(m.material.forward_pass_index).states.push_back({std::uint8_t(state),0x901});
        reject("completed cull/frontface/AA override",[&]{(void)access(m);},"completed material changed");
    }
    const auto fog_control=[&](unsigned word,unsigned shift) {
        auto m=first.material;auto& value=m.material.forward_program.words[word];value=(value&~(15u<<shift))|(3u<<shift);
        reject("completed fog-register read",[&]{(void)access(m);},"unowned fog residue");
    };
    for(unsigned i=0;i<(first.material.material.forward_program.words[53]&255u);++i)
        for(unsigned shift=0;shift<32;shift+=8){fog_control(i,shift);fog_control(34+i,shift);}
    for(unsigned shift=0;shift<32;shift+=8)fog_control(8,shift);
    for(unsigned shift:{8u,16u,24u})fog_control(9,shift);
    //Actual authored default-state147 would otherwise evade a MaterialPass
    //main-only check. Modify ONLY that literal index in a separately declared
    //negative cache, reload its actual pass/material, and require this guard.
    const auto original_map=file_bytes(argv[1]);const auto negative_map=output/"declared-negative.map";
    const Bytes impl(first.pass.implementation_record());
    const auto descriptor=read_tag_block_descriptor(Bytes(cache.read_tag_bytes(first.pass.pass_postprocess_address()+16,8)),0);
    for(unsigned range_offset:{2u,0x11cu}) {
        const ShaderRange range{impl.u16(range_offset)};check(range.count()>0,"negative requires actual nonempty main/default range");
        const auto physical=file_offset(original_map,descriptor.address+std::uint32_t(range.first()*5));
        for(unsigned state:{146u,147u}) {
            auto bytes=original_map;bytes.at(physical)=std::byte(state);file_bytes(negative_map,bytes);
            Cache negative(negative_map,cache.shared_directory());auto negative_passes=selected(negative,argv[4]);
            const auto& p=negative_passes.front();reject(range_offset==2?"actual main-range override":"actual default-range override",[&] {
                (void)owner.phase(use,negative,p.phase,p.pass,p.vertex_class,p.material);
            },"selected pass overrides");
            controls.back()["declared_asset_file_offset"]=physical;controls.back()["original_state"]=std::to_integer<unsigned>(original_map[physical]);controls.back()["declared_state"]=state;
        }
    }
    //Mutate a real consumed constant index in the actual selected program,
    //leaving all instruction opcodes/outputs and metadata ownership intact.
    const auto program=cache.load_vertex_shader_code(first.pass.vertex_shader(),first.vertex_class);
    const auto decoded=decode_vertex_program(program);unsigned instruction=decoded.count;
    for(unsigned n=0;n<decoded.count;++n) {
        const auto& t=decoded.instructions[n];const auto mac=t[1]>>21&15u,ilu=t[1]>>25&7u;
        if(!(t[3]&2u) && ((mac && (t[2]>>26&3u)==3u) ||
           ((mac==2 || (mac>=4 && mac<=12)) && (t[2]>>11&3u)==3u) ||
           ((ilu || mac==3 || mac==4) && (t[3]>>28&3u)==3u))){instruction=n;break;}
    }
    check(instruction<decoded.count,"negative requires real nonrelative constant read");
    for(unsigned constant:{12u,17u,37u,38u,39u,49u,173u,191u}) {
        auto bytes=original_map;unsigned replacements=0;auto it=bytes.begin();
        while((it=std::search(it,bytes.end(),program.begin(),program.end()))!=bytes.end()) {
            const auto at=std::size_t(it-bytes.begin())+4+instruction*16+4;auto word=Bytes(bytes).u32(at);word=(word&~(255u<<13))|(constant<<13);std::memcpy(bytes.data()+at,&word,4);it+=program.size();++replacements;
        }check(replacements>0,"negative program does not resolve in actual map file");file_bytes(negative_map,bytes);
        Cache negative(negative_map,cache.shared_directory());auto negative_passes=selected(negative,argv[4]);const auto& p=negative_passes.front();
        reject("actual program consumes unowned residue",[&]{(void)owner.phase(use,negative,p.phase,p.pass,p.vertex_class,p.material);},"unowned constant residue");
        controls.back()["declared_constant"]=constant;controls.back()["modified_program_copies"]=replacements;
    }
    const auto saved=get<std::uint32_t>(memory,0x4117e4u);put(memory,0x4117e4u,std::uint32_t(2));
    reject("stale captured phase inputs",[&]{(void)owner.camera(use);},"changed before synchronous");put(memory,0x4117e4u,saved);
    check(owner.camera(use)==camera,"source input restoration changed camera output");
    std::string other_thread_error;std::thread thread([&]{try{(void)owner.camera(use);}catch(const std::exception& e){other_thread_error=e.what();}});thread.join();
    check(!other_thread_error.empty(),"other thread admitted Renderer lease");controls.push_back({{"name","other thread lease"},{"rejected",true},{"reason",other_thread_error}});
    //Actual attachment recreation, not a forged Use/format/dimension token.
    SDL_SetWindowSize(window.get(),960,540);SDL_PumpEvents();renderer->resized();
    for(unsigned n=0;n<5 && use.valid();++n)(void)renderer->draw(Camera{});
    check(!use.valid(),"actual attachment generation did not retire");auto wide=targets.acquire();
    reject("actual960 attachments",[&]{(void)source_live_shadow_phase_inputs_tu15(memory,wide,residue);},"single-sample640x480");
    reject("retired640 attachment lease",[&]{(void)owner.camera(use);},"live native Renderer");
    renderer.reset();reject("retired native Renderer",[&]{(void)source_live_shadow_phase_inputs_tu15(memory,wide,residue);},"live native Renderer");
    report["rejection_controls"]=controls;report["passed"]=true;
    std::ofstream json(output/"probe.json");json<<report.dump(2)<<'\n';
    std::cout<<"PASS actual retained640 source bank + genuine native Renderer lease;18 actual selected descriptors;"<<controls.size()<<" rejection controls; no phase-field patching, sourceSDKdevice, liveGPU activation or framebuffer parity claim\n";
    return 0;
}
}
int main(int argc,char** argv) {
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    try{const auto result=run(argc,argv);std::cerr.rdbuf(previous);check(diagnostics.str().find("Vulkan validation:")==std::string::npos,"phase-owner probe generated Vulkan validation messages");if(!diagnostics.str().empty())std::cout<<diagnostics.str();return result;}
    catch(const std::exception& error){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str()<<error.what()<<'\n';return 1;}
}
