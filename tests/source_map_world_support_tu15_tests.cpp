#include "halo2/source_map_world_support_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;using Json=nlohmann::json;
namespace {
std::vector<std::byte> bytes(const std::string &s){std::vector<std::byte> out;for(unsigned i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> b){const char *digits="0123456789abcdef";std::string out;for(auto v:b){auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto at(std::uint32_t p)const{auto i=banks.upper_bound(p);if(i==banks.begin())throw std::runtime_error("Missing original map callback bank");--i;if(p-i->first>=i->second.size())throw std::runtime_error("Map callback exceeds its retained bank");return i;}
    void read(std::uint32_t p,std::span<std::byte> b)const override{for(auto &v:b){auto i=at(p);v=i->second[p++-i->first];}}
    void write(std::uint32_t p,std::span<const std::byte> b)override{for(auto v:b){auto i=at(p);banks.at(i->first)[p++-i->first]=v;}}
    void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));write(p,b);}
};
Json replay(const Json &c){
    constexpr unsigned pool=0x2100000,headers=0x2101000,body=0x2103000,stack=0x2110000,stop=0x3000000;
    Memory m;m.banks[0x4ecbd0]=std::vector<std::byte>(4);m.banks[0x46a0d0]=std::vector<std::byte>(4);m.banks[pool]=std::vector<std::byte>(0x20000);m.banks[0x4f0414]=std::vector<std::byte>(4);m.banks[0x2200000]=std::vector<std::byte>(0x100000);m.banks[0x2310000]=std::vector<std::byte>(0x200);m.banks[0x2311000]=std::vector<std::byte>(0x100);m.banks[0x2312000]=std::vector<std::byte>(0x100);
    m.put(0x4ecbd0,pool);m.put(0x46a0d0,c.at("constant"));m.put(pool+0x44,headers);
    const auto datum=c.at("datum").get<std::uint32_t>();m.put(headers+(datum&65535)*12+8,body);m.write(body,bytes(c.at("before")));
    const auto raw=bytes(c.at("before"));unsigned definition=0;for(unsigned j=0;j<4;++j)definition|=std::to_integer<unsigned>(raw[j])<<(j*8);
    m.put(0x4f0414,0x2200000);m.put(0x2200000+(definition&65535)*16+8,0x2310000);m.put(0x2310000+0x11e,c.value("machine_flags",0u));m.put(0x2310000+0x38,0xffffffffu);
    if(c.at("entry")==0x10c0b0u){const auto mode=c.at("reset_mode").get<unsigned>();m.put(0x2310000+0x38,mode==0?0xffffffffu:0xabcdfffeu);m.put(0x2200000+65534*16+8,0x2311000);m.put(0x2311000+4,mode==1?0xffffffffu:0xabcdfffdu);m.put(0x2200000+65533*16+8,0x2312000);}
    m.put(stack,stop);m.put(stack+4,datum);m.put(stack+8,0x13579bdf);m.put(stack+12,0x2468ace0);
    SourcePhysicsWorldRuntimeAbi a{};a.gp=c.at("gp").get<decltype(a.gp)>();a.gp[4]=stack;a.xmm=c.at("xmm").get<decltype(a.xmm)>();a.eflags=2;
    SourcePhysicsWorldRuntimeServices services;Json trace=Json::array(),writes=Json::array();services.instruction=[&](auto pc){trace.push_back(pc);};services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Unexpected map constructor callee");};
    source_run_map_world_support_tu15(m,c.at("entry"),a,{0x37f,0x1f80},services,[&](const auto&w){writes.push_back(Json::array({w.address,hex(w.written)}));});
    std::vector<std::byte> after(0x400);m.read(body,after);
    return {{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"body",hex(after)},{"instructions",trace},{"writes",writes}};
}
}
int main(int argc,char **argv){try{
    std::ifstream fixture;std::istream *input=&std::cin;if(argc==2){fixture.open(argv[1]);if(!fixture)throw std::runtime_error("Missing original map callback fixtures");input=&fixture;}
    unsigned cases=0;std::string line;while(std::getline(*input,line)){const auto c=Json::parse(line);const auto got=replay(argc==2?c.at("input"):c);if(argc==2){if(got!=c.at("original"))throw std::runtime_error("Original map callback mismatch "+std::to_string(cases));}else std::cout<<got.dump()<<'\n';++cases;}
    if(argc==2){if(cases!=768)throw std::runtime_error("Incomplete original map callback bank");std::cout<<"PASS 768 complete original map callbacks, full body/register/SIMD/flags/write/trace comparison\n";}
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
