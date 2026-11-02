#if defined(HALO2_CAMERA_SHARED_MODULE)
#include "halo2/source_workshop_script_builtins_tu15.hpp"
#define source_workshop_camera_start_tu15_owns_entry source_workshop_script_builtins_tu15_owns_entry
#define source_run_workshop_camera_start_tu15 source_run_workshop_script_builtins_tu15
#else
#include "halo2/source_workshop_camera_start_tu15.hpp"
#endif
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string& text){
    need(text.size()%2==0,"Incomplete camera reference bytes");std::vector<std::byte> result;
    for(unsigned n=0;n<text.size();n+=2)result.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return result;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<unsigned,std::byte> bytes;unsigned pc{};bool tracking{};mutable Json reads=Json::array();
    void read(unsigned at,std::span<std::byte> out)const override{
        if(tracking)reads.push_back(Json::array({pc,at,out.size()}));
        for(auto& v:out)v=bytes.at(at++);
    }
    void write(unsigned at,std::span<const std::byte> in)override{for(auto v:in)bytes[at++]=v;}
    void word(unsigned at,unsigned value){for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(8*n));}
    std::vector<std::byte> bank(unsigned at,unsigned size)const{std::vector<std::byte> result(size);read(at,result);return result;}
};
}
int main(int argc,char** argv)try{
    need(argc==2,"Supply original camera-wrapper reference");std::ifstream input(argv[1]);const auto ref=Json::parse(input);
    need(ref.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        ref.at("entry")==0x2b27b0&&ref.at("cases").size()==32&&ref.at("instruction_patches").empty()&&
        ref.at("effects_intercepted")==Json::array({0x16edd0})&&ref.at("all_sites_executed").get<bool>(),
        "Camera wrapper reference lost its unchanged original scope");
    for(const auto& row:ref.at("cases")){
        need(source_workshop_camera_start_tu15_owns_entry(0x2b27b0),"Missing complete native camera wrapper");
        const unsigned seed=row.at("seed");Memory memory;
        memory.write(0x21ff800,unhex(row.at("stack_before")));memory.write(0x458e90,unhex(ref.at("descriptor")));
        memory.word(row.at("table"),0x458e90);memory.word(0x2103000,row.at("graph"));memory.word(0x2103004,row.at("name"));
        Memory expected;expected.bytes=memory.bytes;expected.write(0x21ff800,unhex(row.at("stack_after")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("initial_registers").get<decltype(abi.gp)>();abi.eflags=row.at("initial_flags");
        for(unsigned n=0;n<8;++n)for(unsigned lane=0;lane<4;++lane){
            unsigned value{};for(unsigned k=0;k<4;++k)value|=((seed+n+lane*4+k)&255)<<(k*8);abi.xmm[n][lane]=value;
        }
        abi.x87_depth=3;abi.x87_status=0x1800;abi.x87[0]=.5L;abi.x87[1]=-1.L;abi.x87[2]=2.L;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto before=abi;
        unsigned calls{};std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](unsigned pc){memory.pc=pc;trace.push_back(pc);};
        services.required_owner=[&](auto&,const auto& call,auto& state,auto&){
            need(calls<row.at("calls").size(),"Camera wrapper introduced a dependency");const auto& boundary=row.at("calls").at(calls++);
            memory.tracking=false;
            need(call.target==boundary.at("target")&&state.gp==boundary.at("registers").get<decltype(state.gp)>()&&
                memory.bank(state.gp[4],28)==unhex(boundary.at("stack")),"Camera wrapper parser/effect/completion ABI differs");
            memory.tracking=true;
            if(call.target==0x212010){state.gp[0]=row.at("parsed").get<bool>()?0x2103000:0;state.gp[4]+=20;}
            else if(call.target==0x16edd0){state.gp[0]=0xabc00000|seed;state.gp[4]+=12;}
            else{need(call.target==0x211da0,"Camera wrapper called an unknown owner");state.gp[0]=0xabc00000|seed;state.gp[4]+=4;}
            state.gp[1]=0xdef00000|seed;state.gp[2]=0x98700000|seed;
        };
        memory.tracking=true;source_run_workshop_camera_start_tu15(memory,0x2b27b0,abi,{0x27f,0x1f80},services);memory.tracking=false;
        need(abi.gp==row.at("registers").get<decltype(abi.gp)>()&&abi.eflags==row.at("eflags"),"Camera wrapper GP/flags/RET differ");
        need(memory.bytes==expected.bytes&&memory.reads==row.at("reads"),"Camera wrapper full ordinary banks or ordered reads differ");
        need(trace==row.at("trace").get<std::vector<unsigned>>()&&calls==row.at("calls").size(),"Camera wrapper deferred path differs");
        need(abi.xmm==before.xmm&&abi.x87_depth==before.x87_depth&&abi.x87_status==before.x87_status&&
            abi.x87[0]==before.x87[0]&&abi.x87[1]==before.x87[1]&&abi.x87[2]==before.x87[2]&&
            abi.floating_point&&abi.floating_point->x87_control_word==0x27f&&abi.floating_point->mxcsr==0x1f80&&!abi.active_mmx,
            "Camera wrapper live FP/SIMD state differs");
    }
    need(!source_workshop_camera_start_tu15_owns_entry(0xffffffffu),"Unknown camera wrapper owner admitted");
    std::cout<<"PASS original camera-start wrapper:32 cases,26 sites, signed descriptor index, full banks/GP/flags/live FP/SIMD and deferred native ABI\n";
    std::cout<<"LIMIT parser/camera/completion are declared ABI inputs; actual native camera effects, HS composition and MCC admission remain required\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
