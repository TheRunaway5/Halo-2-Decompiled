#include "halo2/native_player_effect_tick_tu15.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> decode(const std::string& text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    bool recording{};mutable Json reads=Json::array();
    auto locate(std::uint32_t at,std::size_t bytes)const {
        auto i=banks.upper_bound(at);check(i!=banks.begin(),"Missing player-effect storage");--i;
        check(std::uint64_t(at-i->first)+bytes<=i->second.size(),"Player-effect storage extent differs");return i;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        const auto i=locate(at,out.size());if(recording)reads.push_back(Json::array({at,out.size()}));
        std::copy_n(i->second.begin()+at-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        const auto i=locate(at,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+at-i->first);
    }
};
}
int main(int argc,char** argv)try {
    check(argc==2,"Supply physical original player-effect fixture");std::ifstream f(argv[1]);const auto original=Json::parse(f);
    check(original.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        original.at("instruction_patches").empty() && !original.at("supplied_original_outcomes").get<bool>(),"Player-effect original authority changed");
    std::size_t cases{},reads{},stores{},bindings{};
    for(const auto& row:original.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified") && row.at("completed"),name+": incomplete original");
        for(bool binding:{false,true})for(bool reading:{false,true}) {
            Memory memory;for(const auto& b:row.at("banks"))memory.banks.emplace(b.at("address"),decode(b.at("before")));
            memory.recording=reading;std::vector<SourcePhysicsContactRecordStore> events;
            SourcePhysicsContactRecordObserver observe;
            if(!reading)observe=[&](const auto& e){events.push_back(e);};
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
            const auto sp=abi.gp[4];memory.banks.emplace(sp,std::vector<std::byte>(4));
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0;fnstsw %1":"=m"(host_cw),"=m"(host_sw));
            if(binding) {
                native_player_effect_tick_tu15_run(memory,abi,observe);++bindings;
                fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
                check(abi.gp[4]==sp+4,name+": native tick RET differs");
            } else native_player_effect_tick_tu15(memory,fp,observe);
            std::uint16_t cw{},sw{};asm volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));
            check(cw==host_cw && sw==host_sw && _mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(Json::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp"),name+": source FP differs");
            for(const auto& b:row.at("banks"))check(memory.banks.at(b.at("address"))==decode(b.at("after")),name+": retained player-effect bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto& r:row.at("reads"))expected.push_back(Json::array({r.at("address"),r.at("bytes")}));
                if(binding)expected.push_back(Json::array({sp,4}));
                check(memory.reads==expected,name+": ordered source reads differ\nnative="+memory.reads.dump()+"\noriginal="+expected.dump());reads+=memory.reads.size();
            } else {
                check(events.size()==row.at("writes").size(),name+": source store count differs");
                for(std::size_t n=0;n<events.size();++n) {
                    const auto& a=events[n];const auto& b=row.at("writes")[n];
                    check(a.instruction==b.at("instruction") && a.address==b.at("address") &&
                        a.before==decode(b.at("before")) && a.written==decode(b.at("written")),name+": ordered source store differs at "+std::to_string(n));
                }
                stores+=events.size();
            }
        }
        ++cases;
    }
    check(cases==60 && reads==4502 && stores==28424 && bindings==120,"Incomplete physical player-effect tick matrix");
    std::cout<<cases<<" original player-effect tick cases, "<<reads<<" ordered reads and "<<stores<<" stores; complete bytes and source/host FP passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
