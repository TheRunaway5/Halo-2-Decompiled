#include "halo2/source_shadow_frame_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_renderer_transparent_storage_tu15.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
Json read_json(const std::filesystem::path& path){std::ifstream file(path);check(bool(file),"Shadow replay JSON missing");return Json::parse(file);}
template<class T>void write(std::ostream& out,const T& value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));check(bool(out),"Shadow replay write failed");}
}
int main(int argc,char** argv)try {
    check(argc==4||argc==9,"Supply CAPTURE OWNER FRESH_OUTPUT [TOP LEFT BOTTOM RIGHT LOCAL_PLAYERS]");
    const std::filesystem::path capture=argv[1],out=argv[3];const auto owner=std::stoul(argv[2]);
    check(!std::filesystem::exists(out),"Shadow replay output must be fresh");std::filesystem::create_directories(out);
    const auto input=capture/"shadow-admission"/std::to_string(owner)/std::to_string(0x44710u);
    const auto call=read_json(input/"call.json"),caller=read_json(input/"before-abi.json");
    check(call.at("authority")=="OG Xbox TU1.5","Shadow replay authority differs");
    SourceRuntimeMemory memory;std::ifstream bank(input/"after.bin",std::ios::binary);check(bool(bank),"Shadow replay source bank missing");
    unsigned regions{};
    for(std::uint32_t address,size;bank.read(reinterpret_cast<char*>(&address),4);){
        bank.read(reinterpret_cast<char*>(&size),4);check(bool(bank)&&size&&size<0x40000000u&&++regions<4096,"Invalid source shadow region");
        std::vector<std::byte> bytes(size);bank.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(bank),"Truncated source shadow region");
        memory.mount(address,std::move(bytes),SourceRuntimeMemory::Access::ReadWrite,"Actual native secondary shadow replay input");
    }
    const auto word=[&](std::uint32_t address){std::uint32_t v;auto bytes=memory.view(address,4);std::memcpy(&v,bytes.data(),4);return v;};
    const auto put=[&](std::uint32_t address,const auto& value){memory.write(address,std::as_bytes(std::span(&value,1)));};
    const auto initial=[&] {
        SourcePhysicsWorldRuntimeAbi a{};a.gp=caller.at("gp").get<decltype(a.gp)>();a.xmm=caller.at("xmm").get<decltype(a.xmm)>();
        a.eflags=caller.at("eflags");a.fs_base=caller.at("fs_base");a.x87_status=caller.at("x87_status");
        a.floating_point=SourcePhysicsFactoryFloatingPoint{caller.at("x87_control"),caller.at("mxcsr")};return a;
    };
    SourcePhysicsPoolHostStorage host(memory,0x72000000u);
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto& m,const auto& c,auto& a,const auto& o) {
        if(host.try_call(m,c,a))return;
        const auto fp=*a.floating_point;
        if(source_renderer_transparent_storage_tu15_owns_entry(c.target)){source_run_renderer_transparent_storage_tu15(m,c.target,a,fp,services,o);return;}
        if(source_shadow_frame_tu15_owns_entry(c.target)){source_run_shadow_frame_tu15(m,c.target,a,fp,services,o);return;}
        if(source_model_render_tu15_owns_entry(c.target)){source_run_model_render_tu15(m,c.target,a,fp,services,o);return;}
        if(source_physics_live_support_tu15_owns_entry(c.target)){source_run_physics_live_support_tu15(m,c.target,a,fp,services,o);return;}
        std::ostringstream error;error<<"Unowned original shadow command call "<<std::hex<<c.instruction<<" -> "<<c.target;throw std::runtime_error(error.str());
    };
    check(word(0x4cd3a0u)==0x5598f8u&&word(0x5598f8u+0x2a78u)==2,"Replay lacks actual secondary context");
    if(!word(0x4c2aa8u)&&!word(0x4c2aacu)&&!word(0x52dfd0u)) {
        auto a=initial();source_run_renderer_transparent_storage_tu15(memory,0x1e0f0u,a,*a.floating_point,services);
        check((a.gp[0]&255u)==1,"Original transparent constructor failed");
    }
    check(word(0x4c2aa8u)&&word(0x4c2aacu)&&word(0x52dfd0u),"Source transparent storage incomplete");
    auto report=read_json(capture/"report.json");Json row;
    for(const auto& r:report.at("shadow_root_frames"))if(r.at("owner")==owner&&r.at("tick")==call.at("tick")&&r.at("view")==call.at("view"))row=r;
    check(!row.is_null(),"Shadow replay lacks actual selected root");
    put(0x4c6838u,std::uint8_t(1));put(0x4c6834u,std::uint32_t(0x14));
    put(0x4d1828u,std::uint32_t(0x48c5b8));put(0x492250u,row.at("frame_words").at(28).get<std::uint32_t>());
    const auto retain=[&](const std::filesystem::path& directory,const char* name,const SourcePhysicsWorldRuntimeAbi& a) {
        std::ofstream file(directory/(std::string(name)+".bin"),std::ios::binary);
        memory.visit_committed([&](auto address,auto bytes){write(file,address);write(file,std::uint32_t(bytes.size()));file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());});
        std::ofstream(directory/(std::string(name)+"-abi.json"))<<Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},
            {"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}}.dump(2)<<'\n';
    };
    if(argc==9) {
        std::array<std::int16_t,4> outer{};
        for(unsigned n=0;n<4;++n){const auto v=std::stoi(argv[4+n]);check(v>=-32768&&v<=32767,"Outer camera exceeds source short");outer[n]=v;}
        const auto players=std::stoul(argv[8]);check(players>=1&&players<=4&&outer[2]>outer[0]&&outer[3]>outer[1],"Invalid declared original caller inputs");
        put(0x4c65f0u,outer);
        for(const auto entry:{0x2c470u,0x1be90u,0x1e1a0u}) {
            const auto directory=out/"caller-inputs"/std::to_string(entry);std::filesystem::create_directories(directory);
            auto a=initial();if(entry==0x2c470u)a.gp[0]=players;
            retain(directory,"before",a);std::vector<std::uint32_t> trace;services.instruction=[&](auto pc){trace.push_back(pc);};
            std::ofstream(directory/"call.json")<<Json{{"authority","OG Xbox TU1.5"},{"entry",entry},{"owner",owner},{"tick",call.at("tick")},{"view",call.at("view")},
                {"boundary","Native replay with declared actual caller viewport/player count; actual source BSP planes. Live frame scheduling/GPU validation remains separate."}}.dump(2)<<'\n';
            try {source_run_shadow_frame_tu15(memory,entry,a,*a.floating_point,services);}
            catch(...){retain(directory,"after",a);throw;}
            retain(directory,"after",a);std::ofstream(directory/"native-instructions.json")<<Json(trace).dump()<<'\n';
            services.instruction={};
        }
    }
    auto a=initial();retain(out,"before",a);std::vector<std::uint32_t> trace;services.instruction=[&](auto pc){trace.push_back(pc);};
    std::ofstream(out/"call.json")<<Json{{"authority","OG Xbox TU1.5"},{"entry",0x40e80u},{"owner",owner},{"tick",call.at("tick")},{"view",call.at("view")},
        {"boundary","Native CPU replay on retained live geometry and actual native host allocation. Live runtime/GPU/framebuffer proof remains separate."}}.dump(2)<<'\n';
    try {source_run_shadow_frame_tu15(memory,0x40e80u,a,*a.floating_point,services);}
    catch(...){retain(out,"after",a);throw;}
    retain(out,"after",a);std::ofstream(out/"native-instructions.json")<<Json(trace).dump()<<'\n';
    std::cout<<"Native original40E80 replay completed "<<trace.size()<<" instructions; CPU differential verification pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
