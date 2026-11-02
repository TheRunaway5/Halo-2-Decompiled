#include "halo2/source_workshop_unit_controls_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &text) {
    std::vector<std::byte> out;need(text.size()%2==0,"Incomplete unit-control reference bytes");
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<unsigned,std::byte> bytes;
    unsigned pc{};bool tracking{};mutable Json reads=Json::array();Json writes=Json::array();
    void read(unsigned p,std::span<std::byte> out)const override {
        if(tracking && !(p>=0x21ff700 && p<0x2200000))reads.push_back(Json::array({pc,p,out.size()}));
        for(auto &v:out)v=bytes.at(p++);
    }
    void write(unsigned p,std::span<const std::byte> in)override {
        if(tracking && !(p>=0x21ff700 && p<0x2200000)) {
            need(in.size()<=4,"Unexpected unit-control write width");unsigned value{};
            for(unsigned n=0;n<in.size();++n)value|=std::to_integer<unsigned>(in[n])<<(n*8);
            writes.push_back(Json::array({pc,p,in.size(),value}));
        }
        for(auto v:in)bytes[p++]=v;
    }
    void put(unsigned p,unsigned value,unsigned size=4){for(unsigned n=0;n<size;++n)bytes[p+n]=std::byte(value>>(n*8));}
    std::vector<std::byte> bank(unsigned p,unsigned size)const {std::vector<std::byte> out(size);read(p,out);return out;}
};
}
int main(int argc,char **argv)try {
    need(argc==2,"Supply original unit-control reference");std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        (reference.at("cases").size()==130 ||
         (reference.at("cases").size()==20 && reference.at("owners").size()==1 &&
          reference.at("owners").at(0).at("entry")==0x2b2d80)) && reference.at("instruction_patches").empty() &&
        reference.at("effects_intercepted").empty(),"Unit-control reference lost its complete original owner");
    unsigned cases{};
    for(const auto &row:reference.at("cases")) {
        const unsigned entry=row.at("entry"),seed=row.at("seed");
        need(source_workshop_unit_controls_tu15_owns_entry(entry),"Missing complete native unit-control owner");
        Memory memory;memory.write(0x2106000,unhex(row.at("bank_before")));
        memory.write(0x21ff800,unhex(row.at("stack_before")));
        memory.put(0x4ecbd0,0x2104000);memory.put(0x2104044,0x2105000);memory.put(0x2105020,0x2106000);
        memory.put(0x4f9b54,0x2107000);memory.put(0x2107004,row.at("flags"));
        if(row.contains("actions_before"))memory.write(0x2107000,unhex(row.at("actions_before")));
        memory.put(0x2103000,row.at("object"));memory.put(0x2103004,row.at("enabled"));
        memory.put(0x480c00+210*4,0x457b58);memory.put(0x457b64,2,2);
        memory.put(0x457b66,51,2);memory.put(0x457b68,5,2);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("initial_registers").get<decltype(abi.gp)>();abi.eflags=row.at("initial_flags");
        for(unsigned n=0;n<8;++n)for(unsigned lane=0;lane<4;++lane) {
            unsigned value{};for(unsigned k=0;k<4;++k)value|=((seed+n+lane*4+k)&255)<<(k*8);
            abi.xmm[n][lane]=value;
        }
        const auto before_xmm=abi.xmm;std::vector<unsigned> trace;unsigned index{};
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](unsigned pc){memory.pc=pc;trace.push_back(pc);};
        services.required_owner=[&](auto &,const auto &call,auto &state,auto &) {
            need(index<row.at("calls").size(),"Unit control introduced an unowned dependency");
            const auto &expected=row.at("calls").at(index++);
            need(call.target==expected.at("target") && state.gp==expected.at("registers").get<decltype(state.gp)>() &&
                memory.bank(state.gp[4],24)==unhex(expected.at("stack")),"Unit-control parser/completion ABI differs");
            if(call.target==0x212010) {state.gp[0]=row.at("parsed").get<bool>()?0x2103000:0;state.gp[4]+=20;}
            else {need(call.target==0x211da0,"Unexpected unit-control dependency");state.gp[4]+=4;}
        };
        memory.tracking=true;
        source_run_workshop_unit_controls_tu15(memory,entry,abi,{0x27f,0x1f80},services);
        memory.tracking=false;
        need(memory.bank(0x2106000,0x800)==unhex(row.at("bank_after")),"Native unit-control ordinary bank differs");
        if(row.contains("actions_after"))need(memory.bank(0x2107000,64)==unhex(row.at("actions_after")),
            "Native unit-control action reset bank differs");
        need(memory.bank(0x21ff800,2048)==unhex(row.at("stack_after")),"Native unit-control stack differs");
        need(abi.gp==row.at("registers").get<decltype(abi.gp)>() && abi.eflags==row.at("eflags"),"Native unit-control return/flags differ");
        need(abi.xmm==before_xmm && abi.x87_status==0,"Native unit-control FP/SIMD state differs");
        need(memory.reads==row.at("reads") && memory.writes==row.at("writes"),"Native unit-control ordered memory accesses differ");
        need(trace==row.at("trace").get<std::vector<unsigned>>() && index==row.at("calls").size(),"Native unit-control instruction/dependency path differs");
        ++cases;
    }
    need(!source_workshop_unit_controls_tu15_owns_entry(0xffffffffu),"Unknown unit-control owner was admitted");
    std::cout<<"PASS complete original unit-control/jump-test cases="<<cases<<"; World scheduling/consumer acceptance remains separate\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
