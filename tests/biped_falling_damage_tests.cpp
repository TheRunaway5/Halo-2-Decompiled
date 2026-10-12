#include "halo2/physics_vehicle_update.hpp"
#include "halo2/biped_falling_damage.hpp"
#include "halo2/biped_phase_runtime.hpp"
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
}
int main(int argc,char**argv){try{
    check(argc==3,"Supply original falling fixture and raw/joined mode");if(!std::filesystem::exists(argv[1]))return 77;
    const bool joined=std::string(argv[2])=="joined";check(joined||std::string(argv[2])=="raw","Unknown falling mode");
    std::ifstream in(argv[1]);const auto fixture=J::parse(in);std::size_t cases=0,writes=0,instructions=0,fullbytes=0,logical=0;
    const auto image=fixture.at("image")=="1.5"?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11;
    check(!joined||image==halo2::SourcePhysicsContactImage::MapPack11,"Joined phase only authenticates TU1.1");
    for(const auto&c:fixture.at("cases")){
        // The paired raw fixture stops before DEF10; its existing TU-only
        // joined implementation has an additional original capture suite.
        if(joined&&c.at("declared_input").value("buoyancy",false))continue;
        const auto name="falling_"+std::to_string(c.at("id").get<unsigned>());Memory m;
        for(const auto&b:c.at("initial_banks"))check(m.banks.emplace(b.at("address").get<std::uint32_t>(),bytes(b.at("hex"))).second,"Duplicate contact bank");
        const auto initial=m;auto state=abi(c.at("entry_abi"));std::vector<halo2::SourcePhysicsContactRecordStore> stores;std::vector<std::uint32_t> trace;std::size_t estimates=0,timestamps=0;
        halo2::SourcePhysicsContactRuntimeServices services;
        services.instruction=[&](auto p){trace.push_back(p);};
        services.reciprocal_sqrt=[&](auto input){check(estimates<c.at("estimates").size(),name+" unexpected CPU estimate");const auto&e=c.at("estimates")[estimates++];check(input==e.at("input").get<std::uint32_t>(),name+" CPU input differs");return e.at("output").get<std::uint32_t>();};
        services.timestamp_counter=[&](){
            check(c.contains("timestamps")&&timestamps<c.at("timestamps").size(),name+" unexpected CPU timestamp");
            return c.at("timestamps")[timestamps++].at("output").get<std::uint64_t>();
        };
        const halo2::SourcePhysicsFactoryFloatingPoint fp{c.at("entry_abi").at("x87_control_word").get<std::uint16_t>(),c.at("entry_abi").at("mxcsr").get<std::uint32_t>()};
        services.required_owner=[&](auto&bank,const auto&call,auto&state,const auto&observer){
            const auto velocity=image==halo2::SourcePhysicsContactImage::MapPack11?0xbb0c0u:0xbc200u;
            if(call.target==velocity){halo2::source_run_physics_vehicle_update_runtime(bank,0xbb0c0,state,image,fp,services,observer);return;}
            throw Stop{call};
        };
        auto animation=[&](const auto&s){halo2::SourceUnitAnimationRuntimeServices result;result.instruction=s.instruction;
            result.required_owner=[&](auto&,const auto&call,auto&,const auto&){throw Stop{call};};return result;};
        auto run=[&](auto&memory,auto&a,const auto&s,const auto&o){
            if(joined){halo2::SourceUnitAnimationRuntimeAbi full{};static_cast<halo2::SourcePhysicsContactRuntimeAbi&>(full)=a;
                try{halo2::source_run_biped_falling_damage_1_1(memory,c.at("entry").template get<std::uint32_t>(),full,fp,animation(s),o);}
                catch(...){a=static_cast<const halo2::SourcePhysicsContactRuntimeAbi&>(full);throw;}
                a=static_cast<const halo2::SourcePhysicsContactRuntimeAbi&>(full);
            }else halo2::source_run_biped_falling_damage_runtime(memory,c.at("entry").template get<std::uint32_t>(),a,image,fp,s,o);
        };
        bool stopped=false;
        try{run(m,state,services,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){stores.push_back(s);}));}
        catch(const Stop&e){stopped=true;check(!c.at("complete").get<bool>()&&e.call.target==c.at("exit_entry").get<std::uint32_t>(),name+" unknown owner differs @ "+h(e.call.target));
            const auto&p=c.at("pending");check(e.call.instruction==p.at("instruction")&&e.call.tail==p.at("tail"),name+" pending call site/tail differs");
            check(state.gp[4]==p.at("frame_esp"),name+" pending frame differs");std::array<std::byte,32> frame;m.read(state.gp[4],frame);
            check(hex(frame)==p.at("stack32").get<std::string>(),name+" pending stack differs");}
        catch(const std::exception&e){throw std::runtime_error(name+" last site "+(trace.empty()?"none":h(trace.back()))+": "+e.what());}
        check(stopped==!c.at("complete").get<bool>(),name+" completion differs");
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
        check(timestamps==c.value("timestamps",J::array()).size(),name+" unused CPU timestamps");
        check(estimates==c.at("estimates").size(),name+" unused CPU estimates");
        if(joined&&stopped){
            auto bank=initial;halo2::SourceUnitAnimationRuntimeAbi full{};static_cast<halo2::SourcePhysicsContactRuntimeAbi&>(full)=abi(c.at("entry_abi"));
            auto missing=animation(services);missing.required_owner={};trace.clear();std::vector<halo2::SourcePhysicsContactRecordStore>prefix;bool rejected=false;
            try{halo2::source_run_biped_falling_damage_1_1(bank,c.at("entry").get<std::uint32_t>(),full,fp,missing,[&](const auto&s){prefix.push_back(s);});}
            catch(const std::runtime_error&e){rejected=std::string(e.what())=="Required original falling lower owner unavailable";}
            check(rejected&&trace==expected_trace&&prefix==stores&&bank.banks==m.banks&&full.gp==state.gp&&full.xmm==state.xmm,
                name+" absent owner changed the original pending prefix");
        }
        if(!c.at("estimates").empty()){
            auto absent=initial;auto a=abi(c.at("entry_abi"));auto missing=services;missing.reciprocal_sqrt={};trace.clear();std::vector<halo2::SourcePhysicsContactRecordStore>prefix;bool rejected=false;
            try{run(absent,a,missing,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){prefix.push_back(s);}));}catch(const std::runtime_error&e){rejected=std::string(e.what()).starts_with("Required original contact RSQRTSS");}
            const auto end=std::find(expected_trace.begin(),expected_trace.end(),c.at("estimates")[0].value("instruction",0x30bb22u))+1;
            check(rejected&&trace==std::vector<std::uint32_t>(expected_trace.begin(),end),name+" absent CPU owner changed instruction prefix");
            check(prefix.size()<=stores.size()&&std::equal(prefix.begin(),prefix.end(),stores.begin()),name+" absent CPU owner changed store prefix");
            auto expected_prefix=initial;for(const auto&s:prefix)expected_prefix.write(s.address,s.written);
            check(absent.banks==expected_prefix.banks,name+" absent CPU owner changed retained state");
        }
        if(joined&&c.at("logical").get<bool>()&&c.at("complete").get<bool>()){
            auto bank=initial;trace.clear();std::vector<halo2::SourcePhysicsContactRecordStore>actual;
            halo2::SourceRuntimeCall call;call.entry=c.at("entry");call.entry_esp=c.at("entry_abi").at("gp")[4];
            call.arguments=c.at("arguments").get<std::vector<std::uint32_t>>();if(call.entry==0xe4d30)call.eax=c.at("entry_abi").at("gp")[0];
            const auto value=halo2::source_call_biped_runtime_1_1(bank,call,fp,animation(services),[&](const auto&s){actual.push_back(s);});
            check(value==state.gp[0]&&trace==expected_trace&&actual==stores&&bank.banks==m.banks,name+" biped phase falling adapter differs");++logical;
        }
        ++cases;writes+=stores.size();instructions+=expected_trace.size();
    }
    check(!joined||logical==8,"Missing original falling adapter coverage");
    for(unsigned variant=0;variant<6;++variant){
        Memory bank;halo2::SourceRuntimeCall call;call.entry=variant<3?0xe4d30u:0xe5700u;call.entry_esp=0x12340000;
        if(variant==1){call.eax=0;call.arguments={1};}
        if(variant==2){call.eax=0;call.argument_masks={0xffffffff};}
        if(variant==4)call.arguments={1};
        if(variant==5){call.arguments={1,2};call.argument_masks={0xff,0xffffffff};}
        bool rejected=false;try{halo2::source_call_biped_falling_damage_1_1(bank,call,{0x37f,0x1f80},{});}
        catch(const std::runtime_error&e){rejected=std::string(e.what())=="Missing defined original falling producer ABI";}
        check(rejected&&bank.banks.empty(),"Invalid falling ABI reached memory");
    }
    std::cout<<"PASS original biped falling "<<(joined?"joined":"raw")<<": "<<cases<<" cases, "<<instructions<<" instructions, "<<writes<<" ordered writes, "<<fullbytes<<" full-state bytes, "<<logical<<" logical adapters\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
