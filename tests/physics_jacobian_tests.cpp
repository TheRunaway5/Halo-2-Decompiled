#include "halo2/physics_jacobian.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace {
using J=nlohmann::json;
void check(bool b,const std::string &s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte> bytes(const std::string &s){
    check(s.size()%2==0,"Invalid solver fixture bytes");std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;
}
std::string hex(std::span<const std::byte> b){
    static constexpr char d[]="0123456789abcdef";std::string s;
    for(auto v:b){auto x=std::to_integer<unsigned>(v);s+=d[x>>4];s+=d[x&15];}return s;
}
struct Memory:halo2::SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto at(std::uint32_t p,std::size_t n)const{
        auto i=banks.upper_bound(p);
        if(i==banks.begin())throw std::runtime_error("Missing solver bank @ "+std::to_string(p));--i;
        check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing solver extent @ "+std::to_string(p));return i;
    }
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        const auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        const auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);
    }
};
struct Stop{halo2::SourcePhysicsContactBatchOwnerCall call;};
std::uint32_t run(const J &c,Memory &m,const halo2::SourcePhysicsSolverServices &services,
    const halo2::SourcePhysicsContactRecordObserver &observe){
    const auto a=c.at("args").get<std::vector<std::uint32_t>>(),b=c.at("abi").get<std::vector<std::uint32_t>>();
    return halo2::source_build_physics_jacobians(m,c.at("entry").get<std::uint32_t>(),c.at("receiver").get<std::uint32_t>(),a,
        c.at("entry_esp").get<std::uint32_t>(),c.at("image")=="1.5"?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11,
        {c.at("fp")[0].get<std::uint16_t>(),c.at("fp")[1].get<std::uint32_t>()},services,observe,{b.at(0),b.at(1),b.at(2),b.at(3)});
}
}
int main(int argc,char **argv){try{
    check(argc==2,"Supply full original constraint producer fixture");if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream in(argv[1]);auto fixture=J::parse(in);std::size_t cases=0,write_count=0,instructions=0,byte_count=0,boundaries=0;
    for(const auto &c:fixture.at("cases")){
        const auto name=c.at("name").get<std::string>();Memory m;
        for(const auto &b:c.at("banks"))check(m.banks.emplace(b[0].get<std::uint32_t>(),bytes(b[1])).second,"Duplicate solver bank");
        const auto initial=m;std::vector<halo2::SourcePhysicsContactRecordStore> stores;std::vector<std::uint32_t> trace;std::size_t estimate_index=0;
        halo2::SourcePhysicsSolverServices services;
        services.instruction=[&](auto p){trace.push_back(p);};
        services.reciprocal_sqrt=[&](auto input){
            check(estimate_index<c.at("estimates").size(),name+" unexpected CPU estimate");
            const auto &e=c.at("estimates")[estimate_index++];
            check(input==e.at("input").get<std::uint32_t>(),name+" CPU estimate input differs");
            check(!trace.empty()&&trace.back()==e.at("instruction").get<std::uint32_t>(),name+" CPU estimate site differs");
            return e.at("output").get<std::uint32_t>();
        };
        services.required_owner=[](auto &,const auto &call,const auto &)->std::uint32_t{throw Stop{call};};
        bool stopped=false;
        // These are the only component entries whose EAX output is consumed
        // by a verified caller. Other producers/callbacks are called for stores;
        // their discarded EAX can retain host-dependent x87 exception bits.
        try{const auto result=run(c,m,services,[&](const auto &s){stores.push_back(s);});if((c.at("entry")==0x1d2d70||c.at("entry")==0x326820)&&result!=c.at("returned").get<std::uint32_t>()){std::ostringstream out;out<<name<<" return differs native "<<std::hex<<result<<" original "<<c.at("returned").get<std::uint32_t>();throw std::runtime_error(out.str());}}
        catch(const Stop &e){
            stopped=true;const auto &b=c.at("boundary");check(!b.is_null(),name+" unexpected required owner");
            check(e.call.instruction==b.at("instruction").get<std::uint32_t>()&&e.call.entry==b.at("entry").get<std::uint32_t>()&&
                e.call.entry_esp==b.at("entry_esp").get<std::uint32_t>()&&!e.call.ecx&&
                e.call.arguments==b.at("arguments").get<std::vector<std::uint32_t>>(),name+" required owner ABI differs");++boundaries;
        }catch(const std::exception &e){throw std::runtime_error(name+": "+e.what());}
        check(stopped==!c.at("boundary").is_null(),name+" completion boundary differs");
        check(estimate_index==c.at("estimates").size(),name+" CPU estimate was not consumed");
        const auto expected_trace=c.at("trace").get<std::vector<std::uint32_t>>();
        if(trace!=expected_trace){auto i=std::mismatch(trace.begin(),trace.end(),expected_trace.begin(),expected_trace.end()).first-trace.begin();
            std::ostringstream s;s<<name<<" instruction "<<i<<" counts "<<trace.size()<<'/'<<expected_trace.size();
            if(std::size_t(i)<trace.size())s<<" native "<<std::hex<<trace[i];if(std::size_t(i)<expected_trace.size())s<<" original "<<std::hex<<expected_trace[i];throw std::runtime_error(s.str());}
        const auto &expected=c.at("writes");std::size_t i=0;
        while(i<stores.size()&&i<expected.size()){
            const auto &s=stores[i];const auto &e=expected[i];
            if(s.instruction!=e.at("instruction").get<std::uint32_t>()||s.address!=e.at("address").get<std::uint32_t>()||
                s.before!=bytes(e.at("before"))||s.written!=bytes(e.at("written")))break;++i;
        }
        if(i!=stores.size()||i!=expected.size()){
            std::ostringstream s;s<<name<<" store "<<i<<" counts "<<stores.size()<<'/'<<expected.size();
            if(i<stores.size())s<<" native "<<std::hex<<stores[i].instruction<<" @ "<<stores[i].address<<" "<<hex(stores[i].written)<<" prior "<<hex(stores[i].before);
            if(i<expected.size())s<<" original "<<std::hex<<expected[i].at("instruction").get<std::uint32_t>()<<" @ "<<expected[i].at("address").get<std::uint32_t>()<<" "<<expected[i].at("written").get<std::string>()<<" prior "<<expected[i].at("before").get<std::string>();throw std::runtime_error(s.str());
        }
        for(const auto &b:c.at("after_banks")){
            const auto p=b[0].get<std::uint32_t>();const auto e=bytes(b[1]);check(m.banks.at(p)==e,name+" full bank differs @ "+std::to_string(p));byte_count+=e.size();
        }
        if(!c.at("estimates").empty()){
            auto absent=initial;auto missing=services;missing.reciprocal_sqrt={};trace.clear();
            std::vector<halo2::SourcePhysicsContactRecordStore> prefix;bool rejected=false;
            try{run(c,absent,missing,[&](const auto &s){prefix.push_back(s);});}
            catch(const std::runtime_error &e){rejected=std::string(e.what()).starts_with("Required original Jacobian RSQRTSS");}
            const auto site=c.at("estimates")[0].at("instruction").get<std::uint32_t>();
            const auto end=std::find(expected_trace.begin(),expected_trace.end(),site)+1;
            check(rejected&&trace==std::vector<std::uint32_t>(expected_trace.begin(),end),name+" absent CPU owner changed trace prefix");
            check(prefix.size()<=stores.size()&&std::equal(prefix.begin(),prefix.end(),stores.begin()),name+" absent CPU owner changed store prefix");
            auto expected_prefix=initial;
            for(const auto &s:prefix)expected_prefix.write(s.address,s.written);
            check(absent.banks==expected_prefix.banks,name+" absent CPU owner changed retained state");
        }
        if(stopped){
            auto absent=initial;auto missing=services;missing.required_owner={};std::vector<halo2::SourcePhysicsContactRecordStore> prefix;trace.clear();bool rejected=false;
            try{run(c,absent,missing,[&](const auto &s){prefix.push_back(s);});}catch(const std::runtime_error &e){rejected=std::string(e.what()).starts_with("Required original");}
            check(rejected&&prefix==stores&&trace==expected_trace,name+" absent owner fabricated completion or changed prefix");
            check(absent.banks==m.banks,name+" absent owner changed retained state");
        }
        ++cases;write_count+=stores.size();instructions+=expected_trace.size();
    }
    std::cout<<"PASS original constraint producers: "<<cases<<" cases, "<<instructions<<" instructions, "<<write_count<<" ordered writes, "<<byte_count<<" full-state bytes, "<<boundaries<<" required stops\n";return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
