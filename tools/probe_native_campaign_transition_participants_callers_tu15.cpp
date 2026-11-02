#include "halo2/native_campaign_transition_participants_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;using Json=nlohmann::json;
namespace {
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> raw){constexpr char d[]="0123456789abcdef";std::string out;for(auto b:raw){const auto n=std::to_integer<unsigned>(b);out+=d[n>>4];out+=d[n&15];}return out;}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;mutable unsigned reads{};unsigned stores{};
    void read(std::uint32_t at,std::span<std::byte> out)const override{
        auto p=banks.upper_bound(at);if(p==banks.begin())throw std::runtime_error("Missing caller source");--p;
        if(std::uint64_t(at-p->first)+out.size()>p->second.size())throw std::runtime_error("Wrong caller source extent");
        ++reads;std::copy_n(p->second.begin()+at-p->first,out.size(),out.begin());
    }
    void write(std::uint32_t,std::span<const std::byte>)override{++stores;throw std::runtime_error("Private native options unexpectedly wrote persistent source");}
};
}
int main(int argc,char **argv)try{
    if(argc!=2)throw std::runtime_error("Supply actual caller input profiles");std::ifstream stream(argv[1]);const auto inputs=Json::parse(stream);Json result=Json::array();
    for(const auto &row:inputs){
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto unchanged=memory.banks;const auto initial=bytes(row.at("options_value"));NativeCampaignTransitionOptionsTu15 options;
        if(initial.size()!=options.bytes.size())throw std::runtime_error("Wrong actual private options size");std::copy(initial.begin(),initial.end(),options.bytes.begin());
        // Real191841 publishes the already-produced seed before either child.
        // Its production is an explicit upstream boundary, not supplied here.
        const auto seed=row.at("seed").get<std::uint32_t>();for(unsigned n=0;n<4;++n)options.bytes[0x10u+n]=std::byte(seed>>(8u*n));
        NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
        native_campaign_transition_capture_participants_value_tu15(memory,options);
        native_campaign_transition_prepare_participants_value_tu15(memory,options,fp,row.at("occupied"));
        result.push_back(Json{{"profile",row.at("profile")},{"options",hex(options.bytes)},{"after_fp",Json::array({fp.control_word,fp.status,fp.mxcsr})},{"reads",memory.reads},{"stores",memory.stores},{"source_unchanged",memory.banks==unchanged}});
    }
    std::cout<<result.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
