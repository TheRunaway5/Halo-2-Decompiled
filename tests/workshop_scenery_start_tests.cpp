#include "halo2/source_workshop_script_builtins_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string& text){
    std::vector<std::byte> out;need(text.size()%2==0,"Incomplete scenery reference bytes");
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<unsigned,std::byte> bytes;unsigned pc{};bool tracking{};mutable Json reads=Json::array();
    void read(unsigned p,std::span<std::byte> out)const override{
        if(tracking&&!(p>=0x21ff700&&p<0x2200000))reads.push_back(Json::array({pc,p,out.size()}));
        for(auto& v:out)v=bytes.at(p++);
    }
    void write(unsigned p,std::span<const std::byte> in)override{for(auto v:in)bytes[p++]=v;}
    void put(unsigned p,unsigned v,unsigned size=4){for(unsigned n=0;n<size;++n)bytes[p+n]=std::byte(v>>(n*8));}
    std::vector<std::byte> bank(unsigned p,unsigned size)const{std::vector<std::byte> out(size);read(p,out);return out;}
};
}
int main(int argc,char** argv)try{
    need(argc==2,"Supply original scenery-wrapper reference");std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        reference.at("entry")==0x2accf0&&reference.at("cases").size()==32&&reference.at("instruction_patches").empty()&&
        reference.at("effects_intercepted")==Json::array({0x10f710})&&reference.at("all_sites_executed").get<bool>(),
        "Scenery wrapper reference lost its declared original scope");
    for(const auto& row:reference.at("cases")){
        need(source_workshop_script_builtins_tu15_owns_entry(0x2accf0),"Missing complete native scenery wrapper");
        const unsigned seed=row.at("seed");Memory memory;memory.write(0x21ff800,unhex(row.at("stack_before")));
        memory.write(0x457914,unhex(reference.at("descriptor")));memory.put(0x480c00+184*4,0x457914);
        memory.put(0x2103000,row.at("object"));memory.put(0x2103004,row.at("graph"));memory.put(0x2103008,row.at("name"));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("initial_registers").get<decltype(abi.gp)>();abi.eflags=row.at("initial_flags");
        for(unsigned n=0;n<8;++n)for(unsigned lane=0;lane<4;++lane){
            unsigned value{};for(unsigned k=0;k<4;++k)value|=((seed+n+lane*4+k)&255)<<(k*8);abi.xmm[n][lane]=value;
        }
        const auto before_xmm=abi.xmm;unsigned calls{};std::vector<unsigned> trace;
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](unsigned pc){memory.pc=pc;trace.push_back(pc);};
        services.required_owner=[&](auto&,const auto& call,auto& state,auto&){
            need(calls<row.at("calls").size(),"Scenery wrapper introduced a dependency");const auto& expected=row.at("calls").at(calls++);
            memory.tracking=false;
            need(call.target==expected.at("target")&&state.gp==expected.at("registers").get<decltype(state.gp)>()&&
                memory.bank(state.gp[4],28)==unhex(expected.at("stack")),"Scenery wrapper parser/effect/completion ABI differs");
            memory.tracking=true;
            if(call.target==0x212010){state.gp[0]=row.at("parsed").get<bool>()?0x2103000:0;state.gp[4]+=20;}
            else if(call.target==0x10f710){state.gp[0]=row.at("effect_return");state.gp[4]+=28;}
            else{need(call.target==0x211da0,"Scenery wrapper called an unknown owner");state.gp[0]=0xabc00000|seed;state.gp[4]+=4;}
            state.gp[1]=0xdef00000|seed;state.gp[2]=0x98700000|seed;
        };
        memory.tracking=true;source_run_workshop_script_builtins_tu15(memory,0x2accf0,abi,{0x27f,0x1f80},services);memory.tracking=false;
        need(abi.gp==row.at("registers").get<decltype(abi.gp)>()&&abi.eflags==row.at("eflags"),"Scenery wrapper return/flags differ");
        need(memory.bank(0x21ff800,2048)==unhex(row.at("stack_after")),"Scenery wrapper stack differs");
        need(memory.reads==row.at("reads"),"Scenery wrapper ordered ordinary reads differ");
        need(abi.xmm==before_xmm&&abi.x87_status==0,"Scenery wrapper FP/SIMD state differs");
        need(trace==row.at("trace").get<std::vector<unsigned>>()&&calls==row.at("calls").size(),"Scenery wrapper path differs");
    }
    need(!source_workshop_script_builtins_tu15_owns_entry(0xffffffffu),"Unknown scenery owner admitted");
    std::cout<<"PASS original scenery-start wrapper:32 cases,29 sites, exact deferred parser/effect/VOID completion ABI\n";
    std::cout<<"LIMIT effect returns are declared ABI inputs; actual animation, full HS composition and MCC admission remain unqualified\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
