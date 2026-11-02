#include "halo2/source_workshop_script_builtins_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(auto &byte:out) {
            const auto found=bytes.find(at);
            if(found==bytes.end())throw std::runtime_error("Unowned AI fixture byte at "+std::to_string(at));
            byte=found->second;++at;
        }
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(auto byte:in)bytes[at++]=byte;
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(8*n));
    }
    std::vector<std::byte> bank(std::uint32_t at,unsigned size)const {
        std::vector<std::byte> out(size);read(at,out);return out;
    }
};
std::vector<std::byte> decode(const std::string &text) {
    std::vector<std::byte> out;
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original AI activation fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "AI activation reference differs");
    check(fixture.at("effects").size()==72 && fixture.at("wrappers").size()==12 &&
          fixture.at("consumers").size()==8,"Incomplete AI activation fixture");
    for(const char *kind:{"effects","wrappers","consumers"})for(const auto &row:fixture.at(kind)) {
        const bool wrapper=std::string_view(kind)=="wrappers",consumer=std::string_view(kind)=="consumers";
        Memory memory;const auto before=decode(row.at("before"));
        memory.write(0x2101000,before);memory.write(0x2102000,decode(fixture.at("groups")));
        memory.write(0x21ffe00,decode(row.at("stack_before")));
        memory.put(0x5020a8,0x2100000);memory.put(0x2100001,consumer || wrapper || row.at("active").get<bool>(),1);
        memory.put(0x2100014,0xffffffff);
        memory.put(0x52be7c,0x2100400);memory.put(0x2100444,0x2101000);
        memory.put(0x52be80,0x2100500);memory.put(0x2100544,0x2102000);
        memory.put(0x21fff00,0x3000000);
        if(wrapper) {
            memory.put(0x21fff04,317);memory.put(0x21fff08,0x12340001);memory.put(0x21fff0c,0x23450002);
            memory.put(0x2103000,row.at("reference"));memory.put(0x2103004,row.at("enabled"));
            memory.put(0x480c00+317*4,0x4583d0);memory.put(0x4583dc,2,2);
            memory.put(0x4583de,19,2);memory.put(0x4583e0,5,2);
        }else memory.put(0x21fff04,consumer?0u:row.at("enabled").get<unsigned>());
        if(consumer) {
            memory.put(0x51df54,0x2104000);memory.put(0x2104002,30,2);memory.put(0x2104008,1234);
            memory.put(0x4f3218,0x2108000);memory.put(0x4ecc18,0x2109000);
            memory.put(0x5020c8,0x2100700);
            memory.put(0x2100420,8);memory.put(0x2100424,0x98);memory.put(0x2100438,1);
            memory.put(0x2100448,0x2100600);memory.put(0x2100600,1);memory.put(0x474c24,0,2);
            memory.write(0x46a100,decode(fixture.at("activity_scale")));
        }
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("registers_before").get<std::array<unsigned,8>>();abi.eflags=2;
        unsigned call_index{};SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &,const auto &call,auto &state,auto &observe) {
            // Match the live world's real owner dispatch. Squad deactivation
            // is shared with scene-device support; it is not a fake return at
            // the test boundary, and its original stores/ABI are compared too.
            if(call.target==0x20bf80) {
                check(source_scene_device_constructor_tu15_owns_entry(call.target),"Missing live squad deactivation owner");
                source_run_scene_device_constructor_tu15(memory,call.target,state,{0x27f,0x1f80},services,observe);
                return;
            }
            if(call_index>=row.at("calls").size())throw std::runtime_error(
                "AI activation introduced an unowned dependency target="+std::to_string(call.target)+
                " site="+std::to_string(call.instruction));
            const auto &expected=row.at("calls")[call_index++];
            check(call.target==expected.at("target").get<unsigned>(),"AI activation dependency schedule differs");
            check(state.gp==expected.at("registers").get<std::array<unsigned,8>>(),"AI activation dependency ABI differs");
            check(memory.bank(state.gp[4],24)==decode(expected.at("stack")),"AI activation dependency arguments differ");
            unsigned pop{};
            if(call.target==0x212010){state.gp[0]=row.at("parsed").get<bool>()?0x2103000:0;pop=16;}
            else check(call.target==0x211da0,"Unexpected AI activation boundary");
            state.gp[4]+=4+pop;
        };
        if(consumer)source_run_ai_frame_tu15(memory,0x20a160,abi,{0x27f,0x1f80},services);
        else source_run_workshop_script_builtins_tu15(memory,wrapper?0x2af4c0:0x27f760,abi,{0x27f,0x1f80},services);
        check(memory.bank(0x2101000,before.size())==decode(row.at("after")),"AI activation squad state differs from original");
        check(memory.bank(0x2102000,280)==decode(fixture.at("groups")),"AI activation altered the group hierarchy");
        check(memory.bank(0x21ffe00,512)==decode(row.at("stack_after")),"AI activation private stack differs from original");
        check(abi.gp==row.at("registers").get<std::array<unsigned,8>>(),"AI activation return/register ABI differs");
        check(call_index==row.at("calls").size(),"AI activation lost a required dependency");
        if(consumer && row.at("force").get<bool>()) {
            const auto after=memory.bank(0x2101000,0x98);
            check(after[0x7c]==std::byte{} && after[0x7d]==std::byte{},"Force-active did not reset the actual idle timer");
            check(after[0x28]!=before[0x28],"Force-active did not refresh actual squad activity");
        }
    }
    std::cout<<"PASS 72 native AI activation effects, 12 HS evaluator calls, 8 whole-frame flag consumers\n";
    std::cout<<"LIMIT whole squad frame with empty actor lists; no full campaign AI acceptance\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
