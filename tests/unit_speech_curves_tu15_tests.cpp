#include "halo2/source_unit_speech_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text) {
    check(text.size()%2==0,"Invalid original speech fixture bytes");std::vector<std::byte> result;
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto locate(std::uint32_t address)const {
        auto it=banks.upper_bound(address);
        check(it!=banks.begin(),"Unowned original speech address");--it;
        check(address-it->first<it->second.size(),"Original speech escaped its retained bank");return it;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(auto &value:out){const auto it=locate(at);value=it->second[at++-it->first];}
    }
    void write(std::uint32_t at,std::span<const std::byte> input)override {
        for(const auto value:input){const auto it=locate(at);banks.at(it->first)[at++-it->first]=value;}
    }
    void put(std::uint32_t at,std::uint32_t word) {
        std::array<std::byte,4> encoded;for(unsigned i=0;i<4;++i)encoded[i]=std::byte(word>>(i*8));write(at,encoded);
    }
};
}
int main(int argc,char **argv)try {
    if(argc!=2 && argc!=3)throw std::runtime_error("Supply original facial curve JSON [--no-update]");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const bool baseline=argc==3 && std::string(argv[2])=="--no-update";
    if(argc==3 && !baseline)throw std::runtime_error("Unknown original curve probe mode");
    Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Original speech fixture has another executable");
    constexpr std::uint32_t descriptor=0x2100000,curve=0x2101000,record=0x2104000,stack=0x21fff00,stop=0x3000000;
    Memory m;
    for(const auto &[at,data]:fixture.at("constants").items())m.banks.emplace(std::stoul(at,nullptr,16),bytes(data));
    m.banks.emplace(0x5564d8,std::vector<std::byte>(4));
    m.banks.emplace(descriptor,std::vector<std::byte>(8));m.banks.emplace(curve,std::vector<std::byte>(0x2000));
    m.banks.emplace(record,std::vector<std::byte>(48));m.banks.emplace(0x21fe000,std::vector<std::byte>(0x2000));
    SourcePhysicsWorldRuntimeAbi init{};init.gp[4]=stack;m.put(stack,stop);
    source_run_unit_speech_tu15(m,0x380dd0,init,{0x27f,0x1f80},{});
    std::array<std::byte,4> normalization;m.read(0x5564d8,normalization);
    for(unsigned i=0;i<4;++i)check(normalization[i]==std::byte(fixture.at("normalization").get<std::uint32_t>()>>(i*8)),
        "Original mouth normalization initializer differs");
    std::map<std::string,std::vector<std::byte>> curves;
    for(const auto &value:fixture.at("curves"))curves.emplace(value.at("sound"),bytes(value.at("data")));
    check(curves.size()==38,"Incomplete original opening facial curve coverage");
    unsigned compared{},moving{};
    for(const auto &row:fixture.at("cases")) {
        const auto &input=curves.at(row.at("sound"));const auto before=bytes(row.at("before")),after=bytes(row.at("after"));
        m.write(curve,input);m.put(descriptor,std::uint32_t(input.size()));m.put(descriptor+4,curve);m.write(record,before);
        m.put(stack,stop);m.put(stack+4,row.at("time"));m.put(stack+8,row.at("blend"));m.put(stack+12,row.at("head"));
        SourcePhysicsWorldRuntimeAbi a{};a.gp[0]=descriptor;a.gp[2]=record;a.gp[4]=stack;a.eflags=2;
        if(!baseline)source_run_unit_speech_tu15(m,0x110520,a,{0x27f,0x1f80},{});
        std::array<std::byte,48> actual;m.read(record,actual);
        const bool changed=!std::equal(after.begin()+0x14,after.end(),before.begin()+0x14);
        if(changed) {
            ++moving;
            check(std::equal(after.begin()+0x14,after.end(),actual.begin()+0x14),
                  "Native speech controls remain frozen or differ from the original curve");
        }
        check(std::ranges::equal(actual,after),"Native whole speech sampler record differs from original110520");
        check(a.gp[4]==stack+16,"Native speech sampler caller stack differs");++compared;
    }
    check(compared>1000 && moving>1000,"Original facial fixture does not exercise moving controls");
    std::cout<<"PASS "<<compared<<" unchanged-original sampler cases over38 actual opening curves, "
        <<moving<<" moving-control comparisons, full retained records and caller stack\n";
    std::cout<<"LIMIT curve sampling/quantization; full live audio binding and rendered mouth motion require separate evidence\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
