#include "halo2/renderer.hpp"
#include "halo2/source_lens_draw_geometry_tu15.hpp"
#include "halo2/source_lens_program_tu15.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
using namespace halo2;
namespace {
void check(bool v,const char *why){if(!v)throw FormatError(why);}
std::vector<std::byte> bytes(const std::string &hex){std::vector<std::byte>b;for(unsigned n=0;n<hex.size();n+=2)b.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return b;}
std::vector<char> file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);check(bool(f),"Lens capture absent");return {std::istreambuf_iterator<char>(f),{}};}
void admit_program(const Cache &cache) {
    const Bytes globals(cache.tag_payload(cache.globals_tag()));
    check(globals.u32(0x108)==1,"Original flare rasterizer globals differ");
    const Bytes raster(cache.read_tag_bytes(globals.u32(0x10c),0x60));
    check(raster.u32(0x58)>10,"Original flare vertex reference slot absent");
    const Bytes shaders(cache.read_tag_bytes(raster.u32(0x5c),raster.u32(0x58)*8));
    check(shaders.u32(0x50)==0x76727478,"Original flare vertex slot is not vrtx");
    const auto program=cache.load_vertex_shader_code(shaders.u32(0x54),0);
    check(program.size()==source_lens_vertex_bytes.size(),"Original flare vertex program size differs");
    for(unsigned n=0;n<program.size();++n)check(std::to_integer<unsigned char>(program[n])==source_lens_vertex_bytes[n],"Original flare vertex MOV program differs");
}
SourceLensDraw draw(const nlohmann::json &j,const char *variant,bool depth) {
    const auto c=std::find_if(j.at("cases").begin(),j.at("cases").end(),[&](const auto &c){return c.at("variant")==variant;});
    check(c!=j.at("cases").end(),"Original lens primitive fixture absent");
    for(const auto &index:c->at("final_banks")) {
        const auto &b=j.at("banks").at(index.template get<unsigned>());
        if(b.at("address")==0x2003000){const auto data=bytes(b.at("hex"));return source_lens_draw_geometry_tu15(std::span(data).first(c->at("fifo_bytes").template get<unsigned>()),1,.5f,depth);}
    }
    throw FormatError("Original lens FIFO absent");
}
}
int main(int argc,char **argv)try {
    check(argc==4 || argc==5,"Supply hardware geometry fixture, Headlong cache, output directory and optional final-callback fixture");std::ifstream input(argv[1]);if(!input)return 77;
    const auto fixture=nlohmann::json::parse(input);const auto near=draw(fixture,"near:primitive-plain",true);
    const auto far=draw(fixture,"far:primitive-plain",true);const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path());
    admit_program(cache);
    const auto lens=cache.query_tag({.group=TagGroup::from_string("lens"),.name="effects\\scenery\\flares\\light_flashing_synchronized_red_big"});
    check(lens,"Actual authored Headlong flare absent");const Bytes definition(cache.tag_payload(lens->handle()));
    Material flare;flare.diffuse=cache.load_bitmap(definition.u32(0x24),0);flare.name="actual authored lens bitmap";
    std::vector<Material> resident{Material{},flare};
    struct Callback {std::string label;std::vector<SourceLensDraw> draws;};std::vector<Callback> callbacks;
    std::map<std::string,std::vector<unsigned>> bitmap_slots;
    if(argc==5) {
        std::ifstream f(argv[4]);check(bool(f),"Original final-callback hardware fixture absent");const auto original=nlohmann::json::parse(f);
        const auto assets=std::filesystem::path(argv[2]).parent_path().parent_path().parent_path();
        for(const auto &c:original.at("cases")) {
            Callback callback;callback.label=c.at("variant");const auto first=callback.label.find(':'),last=callback.label.rfind(':');
            const auto map=callback.label.substr(0,first),tag=callback.label.substr(first+1,last-first-1),key=map+":"+tag;
            if(!bitmap_slots.contains(key)) {
                auto path=assets/"disc/maps"/(map+".map");if(!std::filesystem::exists(path))path=assets/"map-pack/maps"/(map+".map");
                Cache authored(path,path.parent_path());admit_program(authored);const auto lens=authored.query_tag({.group=TagGroup::from_string("lens"),.name=tag});check(lens,"Callback lacks its actual authored lens definition");
                const auto bitmap=Bytes(authored.tag_payload(lens->handle())).u32(0x24);std::vector<unsigned> slots;
                for(unsigned image=0;image<authored.bitmap_count(bitmap);++image) {
                    Material material;material.diffuse=authored.load_bitmap(bitmap,image);material.name=key+":"+std::to_string(image);
                    slots.push_back(resident.size());resident.push_back(std::move(material));
                }
                bitmap_slots.emplace(key,std::move(slots));
            }
            const auto &slots=bitmap_slots.at(key);unsigned selected=~0u;float secondary=-1;
            for(const auto &index:c.at("final_banks")) {
                const auto &bank=original.at("banks").at(index.get<unsigned>());if(bank.at("address")!=0x2003000)continue;
                const auto raw=bytes(bank.at("hex"));const auto count=c.at("fifo_bytes").get<unsigned>();const Bytes b{std::span<const std::byte>(raw).first(count)};
                for(unsigned at=0;at<count;) {
                    const auto header=b.u32(at),method=header&0x1fffu,n=(header>>18)&2047u;
                    check(n && at+4+n*4<=count,"Original callback has a truncated GPU packet");
                    if(header==0x417fc && b.u32(at+4)==7) {
                        check(selected<slots.size() && secondary>=0,"Original callback primitive lacks bitmap/attribute8 state");
                        callback.draws.push_back(source_lens_draw_geometry_tu15(std::span(raw).subspan(at,136),slots[selected],secondary,false));at+=136;continue;
                    }
                    if(method==0x1b00 && n==2) {
                        const auto alias=b.u32(at+4);check(alias>=0x2540000 && (alias-0x2540000)%4096==0,"Original callback bitmap alias differs");selected=(alias-0x2540000)/4096;
                    }else if(method==0x18c0 && n==2)secondary=b.f32(at+4);
                    else check((method==0x1b08 && n==2) || ((method==0x1b0c || method==0x1b14 || method==0x1e70) && n==1),"Original callback requires an undeclared GPU state method");
                    at+=4+n*4;
                }
            }
            callbacks.push_back(std::move(callback));
        }
    }
    SDL_setenv("HALO2_DEPTH_DIAGNOSTICS","1",1);SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Original final flare draw",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);check(window,SDL_GetError());
    std::ostringstream diagnostics;auto *old=std::cerr.rdbuf(diagnostics.rdbuf());nlohmann::json report;
    try {
        Mesh mesh;Material plane;plane.diffuse={1,1,11,{std::byte{16},std::byte{32},std::byte{64},std::byte{255}}};resident[0]=plane;mesh.materials=resident;
        for(const auto p:std::array<Vec3,12>{{{4,-100,-100},{4,100,-100},{4,100,100},{4,-100,-100},{4,100,100},{4,-100,100},{4,100,100},{4,100,-100},{4,-100,-100},{4,-100,100},{4,100,100},{4,-100,-100}}})mesh.vertices.push_back({p,{1,1,1},{0,0},0});
        const auto *directory=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,directory?directory:std::filesystem::path{});
        renderer.require_lens_bitmap(1,flare.diffuse);
        for(unsigned n=2;n<resident.size();++n)renderer.require_lens_bitmap(n,resident[n].diffuse);
        bool rejected=false;try{renderer.require_lens_bitmap(0,flare.diffuse);}catch(const FormatError&){rejected=true;}check(rejected,"Foreign bitmap residency was admitted");
        BspAtmosphere atmosphere;atmosphere.gradient={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};renderer.scene_bsp_atmosphere(atmosphere);
        Camera camera;camera.field_of_view=70;ModelView view;view.camera=camera;view.width=640;view.height=480;view.full_surface=true;view.vertices=mesh.vertices;view.atmosphere=BspFogSample{};
        auto capture=[&](const char *name,std::span<const SourceLensDraw> draws){view.lens_draws=draws;check(renderer.draw(camera,output/name,{},false,{},{},{},std::span{&view,1}),"Final flare view was not submitted");};
        capture("baseline.ppm",{});capture("visible.ppm",std::span{&near,1});capture("visible-repeat.ppm",std::span{&near,1});capture("occluded.ppm",std::span{&far,1});
        check(file(output/"baseline.ppm")!=file(output/"visible.ppm"),"Original final flare produced no visible pixels");
        check(file(output/"visible.ppm")==file(output/"visible-repeat.ppm"),"Retained final flare pixels changed on repeated input");
        check(file(output/"baseline.ppm")==file(output/"occluded.ppm"),"Depth-tested flare leaked through original world depth");
        auto zero=near;for(auto &v:zero.vertices)v.rgba&=0xffffffu;capture("zero-alpha.ppm",std::span{&zero,1});
        check(file(output/"baseline.ppm")==file(output/"zero-alpha.ppm"),"Zero source alpha changed additive flare presentation");
        for(const auto *suffix:{".depth-d24-low24.bin",".stencil-u8.bin"})check(file(output/(std::string("baseline.ppm")+suffix))==file(output/(std::string("visible.ppm")+suffix)),"Final flare altered original depth/stencil");
        auto no_depth=far;no_depth.depth_test=false;capture("depth-disabled.ppm",std::span{&no_depth,1});
        check(file(output/"baseline.ppm")!=file(output/"depth-disabled.ppm"),"Original disabled depth mode still occluded a flare");
        report={{"passed",true},{"gpu",renderer.device_name()},{"actual_authored_bitmap",true},{"source_pixel_words",source_lens_pixel_words},{"visible_repeat_exact",true},{"depth_stencil_unchanged",true},{"full_goal_complete",false},{"boundary","Original primitive packets and authored bitmap through the native final shader consumer. Ordinary final-callback scheduling and all-map acceptance remain separate."}};
        unsigned callback_quads{};for(unsigned n=0;n<callbacks.size();++n) {
            const auto name="callback-"+std::to_string(n)+".ppm";capture(name.c_str(),callbacks[n].draws);callback_quads+=callbacks[n].draws.size();
        }
        report["complete_callback_cases"]=callbacks.size();report["complete_callback_quads"]=callback_quads;
        report["actual_resident_bitmap_images"]=resident.size()-1;
    }catch(...){std::cerr.rdbuf(old);std::ofstream(output/"validation.log")<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(old);std::ofstream(output/"validation.log")<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();
    check(diagnostics.str().find("VUID-")==std::string::npos && diagnostics.str().find("Validation Error")==std::string::npos,"Original flare GPU validation failed");
    std::ofstream(output/"report.json")<<report.dump(2)<<'\n';std::cout<<"PASS original flare primitive: visible pixels, original combiner, real bitmap, alpha/depth gates and immutable depth/stencil\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
