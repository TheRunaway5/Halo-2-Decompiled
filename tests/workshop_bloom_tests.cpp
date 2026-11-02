#include "halo2/bloom.hpp"
#include "halo2/source_workshop_script_builtins_tu15.hpp"
#include "halo2/source_object_vector_contact.hpp"
#include <nlohmann/json.hpp>
#include <bit>
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
        for(auto &byte:out)byte=bytes.at(at++);
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(auto byte:in)bytes[at++]=byte;
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4){
        for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(n*8));
    }
    std::uint32_t word(std::uint32_t at)const {
        std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes.at(at+n))<<(8*n);return value;
    }
};
std::vector<std::byte> decode(const std::string &text){
    std::vector<std::byte> out;
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original Workshop bloom fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Workshop bloom reference differs");
    check(fixture.at("cases").size()==64 && fixture.at("setters").size()==36,"Incomplete Workshop bloom fixture");
    for(const auto &row:fixture.at("setters")) {
        Memory memory;memory.write(0x2100000,decode(row.at("before")));
        memory.put(0x515ec4,0x2100000);memory.put(0x2103000,row.at("value"));
        const unsigned opcode=row.at("opcode"),descriptor=opcode==891?0x45afc4:opcode==894?0x45b000:0x45b028;
        memory.put(0x480c00+opcode*4,descriptor);memory.put(descriptor+12,1,2);
        memory.put(descriptor+14,opcode==896?5:6,2);
        memory.put(0x21fff00,0x3000000);memory.put(0x21fff04,opcode);
        memory.put(0x21fff08,0x12340001);memory.put(0x21fff0c,0x23450002);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x21fff00;abi.eflags=2;
        std::vector<unsigned> calls;
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &,const auto &call,auto &state,auto &) {
            calls.push_back(call.target);const auto sp=state.gp[4];
            if(call.target==0x212010) {
                check(memory.word(sp+4)==0x12340001 && memory.word(sp+8)==1 &&
                    memory.word(sp+12)==descriptor+14 && memory.word(sp+16)==0x23450002,
                    "Native bloom changed its actual argument parser ABI");
                state.gp[0]=row.at("parsed").get<bool>()?0x2103000:0;state.gp[4]+=20;
            }else if(call.target==0x211da0) {
                check(state.gp[0]==0 && state.gp[1]==0x12340001,"Native bloom completion differs");
                state.gp[4]+=4;
            }else throw std::runtime_error("Unexpected Workshop bloom dependency");
        };
        source_run_workshop_script_builtins_tu15(memory,row.at("entry"),abi,{0x27f,0x1f80},services);
        std::vector<std::byte> actual(36);memory.read(0x2100000,actual);
        check(actual==decode(row.at("after")),"Native bloom override bank differs from original");
        check(calls==row.at("calls").get<std::vector<unsigned>>(),"Native bloom parser/completion schedule differs");
        check(abi.gp==row.at("registers").get<std::array<unsigned,8>>(),"Native bloom register/stack ABI differs");
    }
    for(const auto &row:fixture.at("cases")) {
        CinematicBloomOverride override;override.enabled=row.at("enabled");
        override.blur_amount=std::bit_cast<float>(row.at("blur").get<unsigned>());
        override.box_factor=std::bit_cast<float>(row.at("box").get<unsigned>());
        override.silver_bullet=row.at("silver");
        const auto actual=cinematic_bloom(.9f,.375f,row.at("cinematic"),override);
        unsigned p{};for(const auto *program:{&actual.threshold,&actual.blur,&actual.composite}) {
            check(program->words==row.at("programs")[p++].get<std::array<unsigned,60>>(),
                "Workshop bloom program differs from original");
        }
        check(actual.radii.size()==row.at("radii").size(),"Workshop bloom pass count differs");
        for(unsigned i=0;i<actual.radii.size();++i)
            check(std::bit_cast<unsigned>(actual.radii[i])==row.at("radii")[i].get<unsigned>(),
                "Workshop bloom radius differs from original");
        check((actual.silver_bullet?775u:1u)==row.at("blends")[1][1].get<unsigned>(),
            "Workshop bloom blend selector differs");
    }
    check(fixture.at("collision").size()==32,"Incomplete cinematic-collision fixture");
    for(const auto &row:fixture.at("collision")) {
        Memory memory;memory.put(0x4ecbd0,0x2104000);memory.put(0x2104044,0x2105000);
        memory.put(0x2105020,0x2106000);memory.put(0x2106004,row.at("before"));
        memory.put(0x21fff00,0x3000000);memory.put(0x21fff04,row.at("enabled"));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=row.at("object");abi.gp[4]=0x21fff00;abi.eflags=2;
        std::vector<unsigned> calls;
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &,const auto &call,auto &state,auto &) {
            check(call.target==0xbaba0 && state.gp[6]==row.at("object").get<unsigned>(),
                "Cinematic collision lost the actual spatial-update owner ABI");
            check(memory.word(0x2106004)==row.at("after").get<unsigned>(),
                "Cinematic collision called spatial update before committing the flag");
            calls.push_back(call.target);state.gp[4]+=4;
        };
        source_run_workshop_script_builtins_tu15(memory,0xbe130,abi,{0x27f,0x1f80},services);
        check(memory.word(0x2106004)==row.at("after").get<unsigned>(),"Cinematic collision changed unrelated object flags");
        check(calls==row.at("calls").get<std::vector<unsigned>>() &&
            abi.gp==row.at("registers").get<std::array<unsigned,8>>(),"Cinematic collision call/return differs");
        SourceObjectVectorOwner owner;owner.full_handle=0x12340002;owner.header_type=6;
        owner.flags=memory.word(0x2106004)&0x100000u;
        SourceObjectVectorInput query;query.flags=0x80000u|(1u<<10);
        check(source_object_vector_child_admitted(owner.full_handle,owner,query)==!(owner.flags&0x100000u),
            "Cinematic collision flag did not reach the actual contact-query filter");
    }
    std::cout<<"PASS 36 native HS setters, 64 original bloom comparisons and 32 cinematic collision cases\n";
    std::cout<<"LIMIT collision flags/query admission and spatial-update boundary; no complete scene collision or framebuffer parity\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
