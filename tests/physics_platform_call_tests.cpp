#include "halo2/physics_world_runtime.hpp"
#include "halo2/physics_platform_call.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <thread>
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
    check(argc==3,"Supply original construction and authenticated call binding fixtures");if(!std::filesystem::exists(argv[1]))return 77;
    std::size_t retained_boundaries=0;
    std::ifstream binding_input(argv[2]);const auto binding_fixture=J::parse(binding_input);
    std::ifstream in(argv[1]);const auto fixture=J::parse(in);std::size_t cases=0,writes=0,instructions=0,fullbytes=0,packets=0;
    for(const auto&c:fixture.at("cases")){
        const auto retained_boundary=c.at("complete").get<bool>()?0u:c.at("exit_entry").get<std::uint32_t>();
        const auto name="contact_"+std::to_string(c.at("id").get<unsigned>());Memory m;
        for(const auto&b:c.at("initial_banks"))check(m.banks.emplace(b.at("address").get<std::uint32_t>(),bytes(b.at("hex"))).second,"Duplicate contact bank");
        const auto initial=m;auto state=abi(c.at("entry_abi"));std::vector<halo2::SourcePhysicsContactRecordStore> stores;std::vector<std::uint32_t> trace;std::size_t estimates=0,timestamps=0;
        halo2::SourcePhysicsContactRuntimeServices services;
        services.instruction=[&](auto p){
            // Earlier sealed fixtures deliberately stop at the old boundary.
            // This read-only test observer preserves their exact prefix while
            // the production runtime now owns that complete body as well.
            if(retained_boundary&&p==retained_boundary) {
                ++retained_boundaries;
                throw Stop{{0,retained_boundary,false}};
            }
            trace.push_back(p);
        };
        services.reciprocal_sqrt=[&](auto input){check(estimates<c.at("estimates").size(),name+" unexpected CPU estimate");const auto&e=c.at("estimates")[estimates++];check(input==e.at("input").get<std::uint32_t>(),name+" CPU input differs");return e.at("output").get<std::uint32_t>();};
        services.timestamp_counter=[&](){
            check(c.contains("timestamps")&&timestamps<c.at("timestamps").size(),name+" unexpected CPU timestamp");
            return c.at("timestamps")[timestamps++].at("output").get<std::uint64_t>();
        };
        halo2::PhysicsPlatformLocks platform_locks;
        std::size_t platform_calls=0;
        const auto owner_thread=std::this_thread::get_id();
        halo2::PhysicsPlatformLockServices platform_services;
        platform_services.current_thread=[&] {
            check(std::this_thread::get_id()==owner_thread,"Native source call changed identity owner");
            return 0x96f0000u; // Declared opaque equality token from the independent host fixture.
        };
        services.required_owner=[&](auto &bank,const auto &call,auto &a,const auto&) {
            if(call.target!=0x80000115u&&call.target!=0x80000126u)throw Stop{call};
            check(platform_calls<c.at("platform_calls").size(),"Unexpected native platform call");
            const auto &expected=c.at("platform_calls")[platform_calls];
            check(call.instruction==expected.at("instruction")&&call.target==expected.at("target"),"Native platform caller differs");
            const auto entry=abi(expected.at("entry_abi"));
            check(a.gp==entry.gp&&a.xmm==entry.xmm,"Native platform actual input ABI differs");
            const auto pointer=expected.at("pointer").get<std::uint32_t>();
            std::vector<std::byte> actual(28);bank.read(pointer,actual);
            check(actual==bytes(expected.at("before_hex")),"Native platform actual object input differs");
            check(halo2::source_try_native_physics_lock_call(bank,call,a,halo2::SourcePhysicsContactImage::MapPack11,platform_locks,platform_services),"Authenticated native platform call was not handled");
            bank.read(pointer,actual);check(actual==bytes(expected.at("after_hex")),"Independent compatibility object result differs");
            const auto exit=abi(expected.at("exit_abi"));
            check(a.gp==exit.gp&&a.xmm==exit.xmm&&a.eflags==exit.eflags,"Declared native platform return ABI differs");
            ++platform_calls;
        };
        auto run=[&](auto&memory,auto&a,const auto&s,const auto&o){halo2::source_run_physics_world_runtime(memory,c.at("entry").get<std::uint32_t>(),a,
            fixture.value("image",std::string("1.1"))=="1.5"?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11,
            {c.at("entry_abi").at("x87_control_word").get<std::uint16_t>(),c.at("entry_abi").at("mxcsr").get<std::uint32_t>()},s,o);};
        bool stopped=false;
        try{run(m,state,services,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){stores.push_back(s);}));}
        catch(const Stop&e){stopped=true;check(!c.at("complete").get<bool>()&&e.call.target==c.at("exit_entry").get<std::uint32_t>(),name+" unknown owner differs @ "+h(e.call.target));}
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
        check((state.eflags&0xcd5u)==(endabi.eflags&0xcd5u),name+" defined flags differ native "+h(state.eflags)+" original "+h(endabi.eflags));
        check(state.fs_base==endabi.fs_base&&state.x87_depth==endabi.x87_depth,name+" FS/x87 state differs");
        check((state.x87_status&0x7d00u)==(endabi.x87_status&0x7d00u),name+" x87 comparison/TOP differs");
        for(unsigned n=0;n<state.x87_depth;++n)check(std::memcmp(&state.x87[n],&endabi.x87[n],10)==0,name+" active extended precision value differs");
        for(const auto&b:c.at("final_banks")){const auto p=b.at("address").get<std::uint32_t>();const auto e=bytes(b.at("hex"));std::vector<std::byte>actual(e.size());m.read(p,actual);check(actual==e,name+" full retained bank differs @ "+h(p));fullbytes+=e.size();}
        check(timestamps==c.value("timestamps",J::array()).size(),name+" unused CPU timestamps");
        check(estimates==c.at("estimates").size(),name+" unused CPU estimates");
        if(!c.at("estimates").empty()){
            auto absent=initial;auto a=abi(c.at("entry_abi"));auto missing=services;missing.reciprocal_sqrt={};trace.clear();std::vector<halo2::SourcePhysicsContactRecordStore>prefix;bool rejected=false;
            try{run(absent,a,missing,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){prefix.push_back(s);}));}catch(const std::runtime_error&e){rejected=std::string(e.what()).starts_with("Required original contact RSQRTSS");}
            const auto end=std::find(expected_trace.begin(),expected_trace.end(),c.at("estimates")[0].value("instruction",0x30bb22u))+1;
            check(rejected&&trace==std::vector<std::uint32_t>(expected_trace.begin(),end),name+" absent CPU owner changed instruction prefix");
            check(prefix.size()<=stores.size()&&std::equal(prefix.begin(),prefix.end(),stores.begin()),name+" absent CPU owner changed store prefix");
            auto expected_prefix=initial;for(const auto&s:prefix)expected_prefix.write(s.address,s.written);
            check(absent.banks==expected_prefix.banks,name+" absent CPU owner changed retained state");
        }
        check(platform_calls==c.at("platform_calls").size(),"Unused native platform call evidence");
        {
            auto joined_memory=initial;auto joined_state=abi(c.at("entry_abi"));auto joined_services=services;
            joined_services.required_owner=[](auto&,const auto&call,auto&,const auto&){throw Stop{call};};
            std::vector<halo2::SourcePhysicsContactRecordStore> joined_stores;trace.clear();
            bool stopped=false;
            try {
                halo2::source_run_physics_world_with_native_platform(joined_memory,c.at("entry").get<std::uint32_t>(),joined_state,
                    halo2::SourcePhysicsContactImage::MapPack11,
                    {c.at("entry_abi").at("x87_control_word").get<std::uint16_t>(),c.at("entry_abi").at("mxcsr").get<std::uint32_t>()},
                    joined_services,platform_locks,platform_services,[&](const auto&s){joined_stores.push_back(s);});
            } catch(const Stop &e){stopped=true;check(e.call.target==c.at("exit_entry"),"Joined world delegated wrong owner");}
            check(stopped==!c.at("complete").get<bool>()&&trace==expected_trace,"Joined world source completion/path differs");
            check(joined_stores==stores&&joined_memory.banks==m.banks&&joined_state.gp==state.gp&&joined_state.xmm==state.xmm,"Joined world changed source state");
            check(joined_state.fs_base==state.fs_base&&joined_state.x87_depth==state.x87_depth&&
                (joined_state.x87_status&0x7d00u)==(state.x87_status&0x7d00u)&&
                (joined_state.eflags&0xcd5u)==(state.eflags&0xcd5u),"Joined world changed source FS/FP/flags");
            for(unsigned n=0;n<state.x87_depth;++n)
                check(std::memcmp(&joined_state.x87[n],&state.x87[n],10)==0,"Joined world changed an active extended value");
            check(joined_state.active_mmx.has_value()==state.active_mmx.has_value(),"Joined world changed MMX ownership");
            if(state.active_mmx)check(joined_state.active_mmx->known_mask==state.active_mmx->known_mask&&
                joined_state.active_mmx->physical==state.active_mmx->physical,"Joined world changed physical MMX aliases");
        }
        {
            auto absent=initial;auto a=abi(c.at("entry_abi"));auto missing=services;missing.required_owner={};trace.clear();
            std::vector<halo2::SourcePhysicsContactRecordStore>prefix;bool rejected=false;
            try{run(absent,a,missing,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){prefix.push_back(s);}));}
            catch(const std::runtime_error&e){rejected=std::string(e.what()).starts_with("Required original contact runtime callee");}
            const auto &first=c.at("platform_calls")[0];
            const auto cut=std::find(expected_trace.begin(),expected_trace.end(),first.at("instruction").get<std::uint32_t>())+1;
            check(rejected&&trace==std::vector<std::uint32_t>(expected_trace.begin(),cut),"Missing platform callback changed source instruction prefix");
            check(prefix.size()<=stores.size()&&std::equal(prefix.begin(),prefix.end(),stores.begin()),"Missing platform callback changed source store prefix");
            auto expected_memory=initial;for(const auto&s:prefix)expected_memory.write(s.address,s.written);
            check(absent.banks==expected_memory.banks&&a.gp==abi(first.at("entry_abi")).gp,"Missing platform callback fabricated a return");
        }
        // Independently authenticated source return words exercise all three
        // callers in both image selections, including the cleanup funclet.
        for(const auto &binding:binding_fixture.at("bindings"))for(bool later:{false,true}) {
            const auto &first=c.at("platform_calls")[0];auto bank=initial;auto a=abi(first.at("entry_abi"));
            const auto p=first.at("pointer").get<std::uint32_t>();const auto sp=a.gp[4];
            const auto expected_return=binding.at(later?"return15":"return11").get<std::uint32_t>();
            std::array<std::byte,8> stack;
            for(unsigned k=0;k<4;++k){stack[k]=std::byte(expected_return>>(8*k));stack[k+4]=std::byte(p>>(8*k));}
            bank.write(sp,stack);bank.write(p,bytes(first.at("before_hex")));
            if(binding.at("target").get<std::uint32_t>()==0x80000126u) {
                platform_locks.enter(bank,p,platform_services);
            }
            const auto input_abi=a;
            halo2::SourcePhysicsContactRuntimeCall call{binding.at("instruction"),binding.at("target"),false};
            check(halo2::source_try_native_physics_lock_call(bank,call,a,later?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11,platform_locks,platform_services),"Paired source caller rejected");
            auto expected_abi=input_abi;expected_abi.gp[4]+=8;
            check(a.gp==expected_abi.gp&&a.xmm==expected_abi.xmm&&a.eflags==expected_abi.eflags,"Void-call binding changed preserved state");
            auto wrong=initial;wrong.write(sp,stack);stack[0]^=std::byte{1};wrong.write(sp,stack);auto wrong_state=input_abi;
            const auto original_banks=wrong.banks;bool rejected=false;
            try{halo2::source_try_native_physics_lock_call(wrong,call,wrong_state,later?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11,platform_locks,platform_services);}
            catch(const std::runtime_error&e){rejected=std::string(e.what()).find("return owner mismatch")!=std::string::npos;}
            check(rejected&&wrong.banks==original_banks&&wrong_state.gp==input_abi.gp,"Wrong source return owner fabricated a call");
            if(call.target==0x80000115u) {
                auto missing=initial;missing.write(sp,std::span<const std::byte>(stack));
                stack[0]^=std::byte{1};missing.write(sp,stack);missing.write(p,bytes(first.at("before_hex")));
                auto a=input_abi;auto no_identity=platform_services;no_identity.current_thread={};
                const auto before=missing.banks;bool stopped=false;
                try{halo2::source_try_native_physics_lock_call(missing,call,a,later?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11,platform_locks,no_identity);}
                catch(const halo2::PhysicsPlatformLockBoundary &b){stopped=b.kind==halo2::PhysicsPlatformLockBoundaryKind::CurrentThread;}
                check(stopped&&missing.banks==before&&a.gp==input_abi.gp,"Missing native identity consumed the source return");
                call.instruction^=1u;
                check(!halo2::source_try_native_physics_lock_call(missing,call,a,later?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11,platform_locks,platform_services),"Unbound source caller was handled");
                check(missing.banks==before&&a.gp==input_abi.gp,"Unbound source caller changed state");
            }
        }
        ++cases;writes+=stores.size();instructions+=expected_trace.size();packets+=c.at("packets").size();
    }
    std::cout<<"PASS native physics platform construction: "<<cases<<" cases, "<<packets<<" packets, "<<instructions<<" instructions, "<<writes<<" ordered writes, "<<fullbytes<<" full-state bytes"<<(retained_boundaries?", retained earlier prefix boundary":"")<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
