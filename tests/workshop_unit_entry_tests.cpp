#include "halo2/source_workshop_script_builtins_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/kernels.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
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
        for(auto &b:out) {
            const auto found=bytes.find(at);
            if(found==bytes.end())throw std::runtime_error("Unowned unit-entry fixture byte at "+std::to_string(at));
            b=found->second;++at;
        }
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {for(auto b:in)bytes[at++]=b;}
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(8*n));
    }
    void zero(std::uint32_t at,unsigned size){write(at,std::vector<std::byte>(size));}
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
    check(argc==2,"Supply original unit-entry fixture");std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Unit-entry reference differs");
    check(fixture.at("effects").size()==32 && fixture.at("wrappers").size()==8 &&
          fixture.at("consumers").size()==4 && fixture.at("immortality").size()==64,"Incomplete object-action fixture");
    for(const char *kind:{"effects","wrappers","consumers","immortality"})for(const auto &row:fixture.at(kind)) {
        const bool immortal=std::string_view(kind)=="immortality";
        const bool wrapper=std::string_view(kind)=="wrappers" || immortal,consumer=std::string_view(kind)=="consumers";
        Memory memory;memory.zero(0x2105000,0x100);memory.zero(0x2106000,0x400);memory.zero(0x2107000,0x400);
        memory.zero(0x2108000,0x200);memory.zero(0x210a000,0xb0);
        memory.put(0x4ecbd0,0x2104000);memory.put(0x2104044,0x2105000);
        memory.put(0x2105014,0x2106000);memory.put(0x2105020,0x2107000);memory.put(0x210501b,1,1);
        memory.put(0x2107000,3);memory.put(0x2107010,0xffffffff);
        memory.put(0x4f0414,0x2109000);memory.put(0x2109038,0x2108000);
        memory.put(0x21081c8,1);memory.put(0x21081cc,0x210a000);memory.put(0x210a000,0x30000);
        memory.put(immortal?0x210710b:0x2107134,row.at("before"),immortal?1:4);
        memory.write(0x21ff800,decode(row.at("stack_before")));memory.put(0x21fff00,0x3000000);
        unsigned offset=4;for(unsigned value:row.at("arguments").get<std::vector<unsigned>>()) {
            memory.put(0x21fff00+offset,value);offset+=4;
        }
        if(wrapper) {
            memory.put(0x2103000,row.at("object"));memory.put(0x2103004,row.at("enabled"));
            const unsigned opcode=immortal?85:213,descriptor=immortal?0x457174:0x457b94;
            memory.put(0x480c00+opcode*4,descriptor);memory.put(descriptor+12,2,2);
            memory.put(descriptor+14,immortal?50:51,2);memory.put(descriptor+16,5,2);
        }
        if(consumer)memory.put(0x210613c,row.at("player"));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("registers_before").get<std::array<unsigned,8>>();abi.eflags=2;
        unsigned index{};std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](unsigned pc){trace.push_back(pc);};
        services.required_owner=[&](auto &,const auto &call,auto &state,auto &) {
            check(index<row.at("calls").size(),"Unit entry introduced an unowned dependency");
            const auto &expected=row.at("calls")[index++];
            check(call.target==expected.at("target").get<unsigned>(),"Unit-entry dependency schedule differs");
            check(state.gp==expected.at("registers").get<std::array<unsigned,8>>() &&
                  memory.bank(state.gp[4],24)==decode(expected.at("stack")),"Unit-entry dependency ABI differs");
            if(call.target==0x212010){state.gp[0]=row.at("parsed").get<bool>()?0x2103000:0;state.gp[4]+=20;}
            else {check(call.target==0x211da0,"Unexpected unit-entry dependency");state.gp[4]+=4;}
            // The oracle hook records the boundary instruction too.
            trace.push_back(call.target);
        };
        if(consumer)source_run_physics_live_support_tu15(memory,0xcb090,abi,{0x27f,0x1f80},services);
        else source_run_workshop_script_builtins_tu15(memory,row.at("entry"),abi,{0x27f,0x1f80},services);
        const unsigned width=immortal?1:4;std::vector<std::byte> expected_flags(width);
        const auto flags=row.at("after").get<unsigned>();
        for(unsigned n=0;n<width;++n)expected_flags[n]=std::byte(flags>>(8*n));
        check(memory.bank(immortal?0x210710b:0x2107134,width)==expected_flags,"Object action changed unrelated flags");
        check(memory.bank(0x21ff800,2048)==decode(row.at("stack_after")),"Unit-entry private stack differs from original");
        check(abi.gp==row.at("registers").get<std::array<unsigned,8>>(),"Unit-entry return ABI differs");
        check(index==row.at("calls").size() && trace==row.at("trace").get<std::vector<unsigned>>(),
              "Unit-entry original execution path differs");
        if(consumer) {
            const bool declined=std::find(trace.begin(),trace.end(),0xcb167u)==trace.end();
            check(declined==(row.at("player").get<unsigned>()!=0xffffffffu && !row.at("enabled").get<bool>()),
                  "Script enterability did not reach the actual player-only vehicle entry gate");
        }
        if(immortal) {
            BodyDamageState vitality;vitality.flags=std::uint16_t((flags&0x40u)<<8);
            BodyDamageInput damage;damage.damage=2;damage.maximum_body=damage.death_maximum_body=1;
            const auto result=body_damage(vitality,damage);
            check(bool(vitality.flags&4u)==!(flags&0x40u) && result.depleted==!(flags&0x40u),
                  "Object cannot-die flag did not reach the actual body-damage clamp");
        }
    }
    std::cout<<"PASS 32 unit flag effects, 8 HS evaluator calls and 4 whole seat-selection consumers\n";
    std::cout<<"PASS 64 original object-immortality evaluator/flag cases and actual body-damage clamp consumers\n";
    std::cout<<"LIMIT authored seat flags bound consumers before geometry; no successful seat-entry acceptance\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
