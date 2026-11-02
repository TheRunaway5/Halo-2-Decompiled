#include "halo2/source_object_name_lookup_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Memory final:SourcePhysicsContactMemory {
    std::map<unsigned,std::byte> bytes;unsigned pc{};bool tracking{};
    mutable Json reads=Json::array();Json writes=Json::array();
    void read(unsigned address,std::span<std::byte> out)const override{
        if(tracking)reads.push_back(Json::array({pc,address,out.size()}));
        for(auto& b:out)b=bytes.at(address++);
    }
    void write(unsigned address,std::span<const std::byte> in)override{
        if(tracking)writes.push_back(Json::array({pc,address,in.size()}));
        for(auto b:in)bytes[address++]=b;
    }
    void word(unsigned address,unsigned value){for(unsigned n=0;n<4;++n)bytes[address+n]=std::byte(value>>(8*n));}
};
}
int main(int argc,char** argv)try{
    need(argc==2,"Supply original object-name reference");std::ifstream input(argv[1]);const auto ref=Json::parse(input);
    need(ref.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        ref.at("entry")==0xbd790&&ref.at("cases").size()==112&&ref.at("instruction_patches").empty()&&
        ref.at("intercepted_boundaries").empty()&&ref.at("all_sites_executed").get<bool>(),
        "Object-name reference lost its complete unchanged original scope");
    for(const auto& row:ref.at("cases")){
        need(source_object_name_lookup_tu15_owns_entry(0xbd790),"Missing complete native object-name owner");
        const unsigned seed=row.at("seed");Memory memory;memory.word(0x4eaba0,row.at("table"));
        memory.word(0x21fff00,ref.at("stop"));
        if(row.at("valid").get<bool>())memory.word(row.at("table").get<unsigned>()+row.at("index").get<unsigned>()*4u,row.at("slot"));
        const auto before_memory=memory.bytes;
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("initial_registers").get<decltype(abi.gp)>();abi.eflags=row.at("initial_flags");
        for(unsigned n=0;n<8;++n)for(unsigned lane=0;lane<4;++lane){
            unsigned value{};for(unsigned k=0;k<4;++k)value|=((seed+n+lane*4+k)&255)<<(k*8);abi.xmm[n][lane]=value;
        }
        abi.x87_depth=3;abi.x87_status=0x1800;abi.x87[0]=.5L;abi.x87[1]=-1.L;abi.x87[2]=2.L;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto before=abi;
        std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](unsigned pc){memory.pc=pc;trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,auto&){throw std::runtime_error("Object-name owner introduced a callee");};
        memory.tracking=true;source_run_object_name_lookup_tu15(memory,0xbd790,abi,{0x27f,0x1f80},services);memory.tracking=false;
        need(abi.gp==row.at("registers").get<decltype(abi.gp)>()&&abi.eflags==row.at("eflags"),"Object-name GP/flags/RET differ");
        need(memory.reads==row.at("reads")&&memory.writes.empty()&&memory.bytes==before_memory,"Object-name lazy ordered reads or memory effects differ");
        need(trace==row.at("trace").get<std::vector<unsigned>>(),"Object-name original path differs");
        need(abi.xmm==before.xmm&&abi.x87_depth==before.x87_depth&&abi.x87_status==before.x87_status&&
            abi.x87[0]==before.x87[0]&&abi.x87[1]==before.x87[1]&&abi.x87[2]==before.x87[2]&&
            abi.floating_point&&abi.floating_point->x87_control_word==0x27f&&abi.floating_point->mxcsr==0x1f80&&!abi.active_mmx,
            "Object-name live FP/SIMD state differs");
    }
    need(!source_object_name_lookup_tu15_owns_entry(0xffffffffu),"Unknown object-name owner admitted");
    std::cout<<"PASS original object-name lookup:112 cases,10 sites, full GP/flags/RET/live FP/SIMD and exact lazy reads; no intercepted callees\n";
    std::cout<<"LIMIT complete component only; actual World name-table initialization/association and native HS consumer remain required\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
