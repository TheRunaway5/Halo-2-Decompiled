#include "halo2/source_direct_damage_runtime.hpp"
#ifdef HALO2_DAMAGE_DELIVERY_TEST
#include "halo2/source_damage_delivery_runtime.hpp"
#endif
#ifdef HALO2_DAMAGE_COMPOSITION_TEST
#include "halo2/source_damage_runtime.hpp"
#include "halo2/death_object_phases.hpp"
#endif
#include <algorithm>
#include <cstring>
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
    check(s.size()%2==0,"Invalid contact fixture bytes");std::vector<std::byte>b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;
}
std::string hex(std::span<const std::byte>b){static constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){auto x=std::to_integer<unsigned>(v);s+=d[x>>4];s+=d[x&15];}return s;}
std::string h(std::uint32_t x){std::ostringstream s;s<<std::hex<<x;return s.str();}
struct Memory:halo2::SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto at(std::uint32_t p)const{
        auto i=banks.upper_bound(p);if(i==banks.begin())throw std::runtime_error("Missing contact bank @ "+h(p));--i;
        check(std::uint64_t(p-i->first)<i->second.size(),"Missing contact extent @ "+h(p));return i;
    }
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        for(auto &x:b){auto i=at(p);x=i->second[p++-i->first];}
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        for(auto x:b){auto i=at(p);banks.at(i->first)[p++-i->first]=x;}
    }
};
halo2::SourcePhysicsContactRuntimeAbi abi(const J&j){
    halo2::SourcePhysicsContactRuntimeAbi a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.eflags=j.at("eflags");a.fs_base=j.at("fs_base");a.x87_status=j.at("x87_status");
    if(j.contains("x87_values")){
        a.x87_depth=j.at("x87_values").size();check(a.x87_depth<=8,"Invalid captured x87 depth");
        check(sizeof(long double)>=10,"Original extended precision backend unavailable");
        for(unsigned n=0;n<a.x87_depth;++n){const auto raw=bytes(j.at("x87_values")[n]);check(raw.size()==10,"Invalid captured extended precision value");std::memcpy(&a.x87[n],raw.data(),10);}
    }else check(j.at("x87_tag").get<unsigned>()==65535,"Uncaptured original x87 entry values");
    for(unsigned n=0;n<8;++n){const auto s=j.at("xmm")[n].get<std::string>();for(unsigned k=0;k<4;++k)a.xmm[n][k]=std::stoul(s.substr(24-k*8,8),nullptr,16);}
    return a;
}
struct Stop{halo2::SourcePhysicsContactRuntimeCall call;};
struct OriginalPrefixBoundary{};
}
int main(int argc,char**argv){try{
    check(argc==2,"Supply original direct damage fixture");if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream in(argv[1]);const auto fixture=J::parse(in);
    std::size_t cases=0,writes=0,instructions=0,fullbytes=0,pending=0,trajectory=0;
    std::map<std::uint32_t,std::vector<std::byte>> retained;
    const auto image=fixture.at("image")=="1.5"?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11;
    for(const auto&c:fixture.at("cases")){
        const auto name="direct damage_"+std::to_string(c.at("id").get<unsigned>());Memory m;
        for(const auto&b:c.at("initial_banks"))check(m.banks.emplace(b.at("address").get<std::uint32_t>(),bytes(b.at("hex"))).second,"Duplicate direct damage bank");
        if(c.at("declared_input").value("continuation",false)){
            check(!retained.empty(),"Missing native trajectory predecessor");
            for(const auto&[p,raw]:retained){std::vector<std::byte>original(raw.size());m.read(p,original);
                check(original==raw,name+" original continuation differs from previous native state");m.write(p,raw);}
        }else retained.clear();
        const auto initial=m;auto state=abi(c.at("entry_abi"));
        std::vector<halo2::SourcePhysicsContactRecordStore> stores;std::vector<std::uint32_t> trace;
        const auto cpu_estimates=c.value("cpu_estimates",J::array());std::size_t estimate_index=0;
        halo2::SourcePhysicsContactRuntimeServices services;
        services.instruction=[&](auto p){
            // A bounded original fixture can end at a helper now owned by the
            // latest source. Stop observation at that authenticated entry;
            // never execute it with banks the original fixture did not capture.
            // This supplies no return and still compares the entire CPU,
            // instruction/store prefix and captured memory at the boundary.
            if(c.contains("original_prefix_exit_site") &&
                p==c.at("original_prefix_exit_site").get<std::uint32_t>())
                throw OriginalPrefixBoundary{};
            trace.push_back(p);
        };
        services.required_owner=[](auto&,const auto&call,auto&,const auto&){throw Stop{call};};
        const halo2::SourcePhysicsFactoryFloatingPoint fp{c.at("entry_abi").at("x87_control_word").get<std::uint16_t>(),c.at("entry_abi").at("mxcsr").get<std::uint32_t>()};
#ifdef HALO2_DAMAGE_DELIVERY_TEST
        services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe) {
            if(image==halo2::SourcePhysicsContactImage::TitleUpdate15 && !call.tail && call.target==0x179610u &&
                (call.instruction==0xd8889u || call.instruction==0xd9016u || call.instruction==0xdb8b9u))
                halo2::source_run_damage_delivery_runtime(memory,0x1781f0u,state,image,fp,services,observe);
            else throw Stop{call};
        };
#endif
        const auto run=[&](auto&bank,auto&a,const auto&s,const auto&o){
#ifdef HALO2_DAMAGE_COMPOSITION_TEST
            check(image==halo2::SourcePhysicsContactImage::MapPack11,"Native composition requires TU1.1");
            halo2::SourceUnitAnimationRuntimeAbi full{};
            static_cast<halo2::SourcePhysicsContactRuntimeAbi &>(full)=a;
            for(unsigned n=0;n<8;++n)full.mmx[n]=0x123456789abc0000ull+n;
            const auto original_mmx=full.mmx;
            halo2::SourceDeathWorldSchedulerServices world;
            world.runtime.instruction=s.instruction;
            if(!cpu_estimates.empty())world.runtime.estimate=[&](auto kind,std::uint32_t input) {
                check(estimate_index<cpu_estimates.size(),name+" unexpected CPU estimate");
                const auto &e=cpu_estimates[estimate_index++];
                check(!trace.empty() && trace.back()==e.at("instruction").get<std::uint32_t>() &&
                    unsigned(kind==halo2::SourceUnitAnimationEstimate::ReciprocalSquareRoot)==e.at("kind").get<unsigned>() &&
                    input==e.at("input").get<std::uint32_t>(),name+" original CPU estimate input differs");
                return e.at("output").get<std::uint32_t>();
            };
            if(s.required_owner)world.runtime.required_owner=[&](auto &m,const auto &call,auto &state,const auto &observe) {
                s.required_owner(m,call,state,observe);
            };
            try {halo2::source_run_death_object_phases_1_1(bank,c.at("entry").template get<std::uint32_t>(),full,fp,world,o);}
            catch(...) {a=static_cast<const halo2::SourcePhysicsContactRuntimeAbi &>(full);check(full.mmx==original_mmx,"Damage composition changed untouched MMX");throw;}
            a=static_cast<const halo2::SourcePhysicsContactRuntimeAbi &>(full);
            check(full.mmx==original_mmx,"Damage composition changed untouched MMX");
#elif defined(HALO2_DAMAGE_DELIVERY_TEST)
            halo2::source_run_damage_delivery_runtime(bank,c.at("entry").template get<std::uint32_t>(),a,image,fp,s,o);
#else
            halo2::source_run_direct_damage_runtime(bank,c.at("entry").template get<std::uint32_t>(),a,image,fp,s,o);
#endif
        };
        bool stopped=false;
        try{run(m,state,services,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){stores.push_back(s);}));}
        catch(const Stop&e){stopped=true;check(!c.at("complete").get<bool>()&&e.call.target==c.at("exit_entry").get<std::uint32_t>(),name+" unknown owner differs @ "+h(e.call.target));
            const auto&p=c.at("pending");check(e.call.instruction==p.at("instruction")&&e.call.tail==p.at("tail"),name+" pending call site/tail differs");
            check(state.gp[4]==p.at("frame_esp"),name+" pending frame differs");std::array<std::byte,32> frame;m.read(state.gp[4],frame);
            check(hex(frame)==p.at("stack32").get<std::string>(),name+" pending stack differs");}
        catch(const OriginalPrefixBoundary&){
            stopped=true;check(!c.at("complete").get<bool>(),name+" complete case has a prefix boundary");
            const auto&p=c.at("pending");
            check(!trace.empty()&&trace.back()==p.at("instruction").get<std::uint32_t>(),name+" original prefix caller differs");
            check(state.gp[4]==p.at("frame_esp"),name+" original prefix frame differs");
            std::array<std::byte,32> frame;m.read(state.gp[4],frame);
            check(hex(frame)==p.at("stack32").get<std::string>(),name+" original prefix stack differs");
        }
        catch(const std::exception&e){throw std::runtime_error(name+" last site "+(trace.empty()?"none":h(trace.back()))+": "+e.what());}
        check(stopped==!c.at("complete").get<bool>(),name+" completion differs");
        check(estimate_index==cpu_estimates.size(),name+" unconsumed original CPU estimate");
        const auto expected_trace=c.at("instructions").get<std::vector<std::uint32_t>>();
        if(trace!=expected_trace){const auto i=std::mismatch(trace.begin(),trace.end(),expected_trace.begin(),expected_trace.end()).first-trace.begin();std::ostringstream s;s<<name<<" trace "<<i<<" counts "<<trace.size()<<'/'<<expected_trace.size();if(std::size_t(i)<trace.size())s<<" native "<<h(trace[i]);if(std::size_t(i)<expected_trace.size())s<<" original "<<h(expected_trace[i]);throw std::runtime_error(s.str());}
        const auto&expected=c.at("writes");std::size_t n=0;
        while(n<stores.size()&&n<expected.size()){
            const auto&s=stores[n];const auto&e=expected[n];if(s.instruction!=e.at("instruction").get<std::uint32_t>()||s.address!=e.at("address").get<std::uint32_t>()||s.before!=bytes(e.at("before"))||s.written!=bytes(e.at("written")))break;++n;
        }
        if(n!=stores.size()||n!=expected.size()){
            std::ostringstream s;s<<name<<" store "<<n<<" counts "<<stores.size()<<'/'<<expected.size();if(n<stores.size())s<<" native "<<h(stores[n].instruction)<<" @ "<<h(stores[n].address)<<" "<<hex(stores[n].written)<<" prior "<<hex(stores[n].before);if(n<expected.size())s<<" original "<<h(expected[n].at("instruction"))<<" @ "<<h(expected[n].at("address"))<<" "<<expected[n].at("written").get<std::string>()<<" prior "<<expected[n].at("before").get<std::string>();throw std::runtime_error(s.str());
        }
        const auto&exit=c.at("exit");
        for(unsigned i=0;i<8;++i)check(state.gp[i]==exit.at("gp")[i].get<std::uint32_t>(),name+" GP "+std::to_string(i)+" differs native "+h(state.gp[i])+" original "+h(exit.at("gp")[i]));
        const auto endabi=abi(exit);check(state.xmm==endabi.xmm,name+" full SSE state differs");
        check(state.floating_point&&state.floating_point->x87_control_word==exit.at("x87_control_word").get<std::uint16_t>()&&
            (state.floating_point->mxcsr&~0x3fu)==(exit.at("mxcsr").get<std::uint32_t>()&~0x3fu),name+" FP controls differ");
        check((state.eflags&0xcd5u)==(endabi.eflags&0xcd5u),name+" defined flags differ native "+h(state.eflags)+" original "+h(endabi.eflags));
        check(state.fs_base==endabi.fs_base&&state.x87_depth==endabi.x87_depth,name+" FS/x87 state differs");
        check((state.x87_status&0x7d00u)==(endabi.x87_status&0x7d00u),name+" x87 comparison/TOP differs");
        for(unsigned n=0;n<state.x87_depth;++n)check(std::memcmp(&state.x87[n],&endabi.x87[n],10)==0,name+" active extended precision value differs");
        for(const auto&b:c.at("final_banks")){const auto p=b.at("address").get<std::uint32_t>();const auto e=bytes(b.at("hex"));std::vector<std::byte>actual(e.size());m.read(p,actual);check(actual==e,name+" full retained bank differs @ "+h(p));fullbytes+=e.size();}
        
        
        if(stopped){
            ++pending;auto bank=initial;auto a=abi(c.at("entry_abi"));auto missing=services;missing.required_owner={};
            trace.clear();estimate_index=0;std::vector<halo2::SourcePhysicsContactRecordStore>prefix;bool rejected=false;
            try{run(bank,a,missing,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){prefix.push_back(s);}));}
            catch(const OriginalPrefixBoundary&){rejected=true;}
            catch(const std::runtime_error&e){
#ifdef HALO2_DAMAGE_COMPOSITION_TEST
                rejected=std::string(e.what())=="Required original death object phase owner unavailable";
#else
                rejected=std::string(e.what())=="Required original contact runtime callee unavailable";
#endif
            }
            check(rejected&&estimate_index==cpu_estimates.size()&&trace==expected_trace&&prefix==stores&&bank.banks==m.banks&&a.gp==state.gp&&a.xmm==state.xmm,
                name+" absent owner changed the original pending prefix");
        }
        if(c.at("declared_input").contains("trajectory")){
            ++trajectory;check(!stopped,"Continuous direct damage trajectory stopped at an unowned callee");
            for(const auto&r:c.at("retained_regions")){
                const auto p=r.at("address").get<std::uint32_t>();auto&raw=retained[p];raw.resize(r.at("size"));m.read(p,raw);
            }
        }
        ++cases;writes+=stores.size();instructions+=expected_trace.size();
    }
    std::cout<<"PASS original direct damage: "<<cases<<" cases, "<<instructions<<" instructions, "<<writes<<" ordered writes, "<<fullbytes<<" full-state bytes, "<<pending<<" required owner boundaries, "<<trajectory<<" continuous trajectory phases\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
