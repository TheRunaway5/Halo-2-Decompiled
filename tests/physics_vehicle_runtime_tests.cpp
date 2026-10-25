#include "halo2/physics_vehicle_runtime.hpp"
#include "halo2/source_vehicle_native_rates.hpp"
#include "physics/runtime/physics_instruction_remap_tu15.hpp"
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
    bool offer_views=false;
    std::function<void(std::uint32_t,std::size_t)> read_guard;
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t p,std::size_t n)const noexcept override{
        if(!offer_views)return {};auto i=banks.upper_bound(p);if(i==banks.begin())return {};--i;
        if(std::uint64_t(p-i->first)+n>i->second.size())return {};
        return std::span<const std::byte>(i->second).subspan(p-i->first,n);
    }
    auto at(std::uint32_t p)const{
        auto i=banks.upper_bound(p);if(i==banks.begin())throw std::runtime_error("Missing contact bank @ "+h(p));--i;
        check(std::uint64_t(p-i->first)<i->second.size(),"Missing contact extent @ "+h(p));return i;
    }
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        if(read_guard)read_guard(p,b.size());
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
struct StoreStop{};
struct ReadStop{};
bool same_abi(const halo2::SourcePhysicsContactRuntimeAbi &a,const halo2::SourcePhysicsContactRuntimeAbi &b){
    if(a.gp!=b.gp || a.xmm!=b.xmm || a.eflags!=b.eflags || a.fs_base!=b.fs_base ||
       a.x87_depth!=b.x87_depth || a.x87_status!=b.x87_status)return false;
    if(a.floating_point.has_value()!=b.floating_point.has_value())return false;
    if(a.floating_point && (a.floating_point->x87_control_word!=b.floating_point->x87_control_word ||
       a.floating_point->mxcsr!=b.floating_point->mxcsr))return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;
    return true;
}
J describe_abi(const halo2::SourcePhysicsContactRuntimeAbi &a){
    return {{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status}};
}
Memory padded(const Memory &input,const halo2::SourcePhysicsContactRuntimeAbi &state,const J &immutable){
    // Ordinary explicitly declared backing for cursor-capability tests. The
    // original reference executes against the identical padded bank too.
    std::map<std::uint32_t,std::vector<std::byte>> pages;
    for(const auto &[p,n]:std::array<std::pair<std::uint32_t,unsigned>,3>{{
        {state.gp[1],4},{state.gp[2],16},{state.gp[4]-16,28}}})
        for(unsigned i=0;i<n;++i)pages.try_emplace((p+i)&~0xfffu,0x1000);
    for(const auto &bank:immutable){
        const auto p=bank.at("address").get<std::uint32_t>();const auto raw=bytes(bank.at("hex"));
        for(std::size_t n=0;n<raw.size();++n){const auto at=p+std::uint32_t(n),page=at&~0xfffu;
            auto &b=pages[page];if(b.empty())b.resize(0x1000);b[at-page]=raw[n];}
    }
    for(const auto &[p,bytes]:input.banks)for(std::size_t n=0;n<bytes.size();++n){
        const auto at=p+std::uint32_t(n),page=at&~0xfffu;
        auto &b=pages[page];if(b.empty())b.resize(0x1000);b[at-page]=bytes[n];
    }
    Memory result;result.offer_views=true;
    for(const auto &[p,bytes]:pages){
        if(!result.banks.empty()){
            auto last=std::prev(result.banks.end());
            if(std::uint64_t(last->first)+last->second.size()==p){last->second.insert(last->second.end(),bytes.begin(),bytes.end());continue;}
        }
        result.banks.emplace(p,bytes);
    }
    return result;
}
}
int main(int argc,char**argv){try{
    check(argc==2,"Supply original vehicle runtime fixture");if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream in(argv[1]);const auto fixture=J::parse(in);std::size_t cases=0,writes=0,instructions=0,fullbytes=0,cursor_kernels=0,store_faults=0,read_faults=0,cursor_fallbacks=0,mapped_kernels=0,mapped_fallbacks=0;
    for(const auto&c:fixture.at("cases")){
        const auto name="vehicle_"+std::to_string(c.at("id").get<unsigned>());Memory m;
        for(const auto&b:c.at("initial_banks"))check(m.banks.emplace(b.at("address").get<std::uint32_t>(),bytes(b.at("hex"))).second,"Duplicate contact bank");
        const auto initial=m;auto state=abi(c.at("entry_abi"));std::vector<halo2::SourcePhysicsContactRecordStore> stores;std::vector<std::uint32_t> trace;std::size_t estimates=0,timestamps=0;
        halo2::SourcePhysicsContactRuntimeServices services;
        services.instruction=[&](auto p){trace.push_back(p);};
        services.reciprocal_sqrt=[&](auto input){check(estimates<c.at("estimates").size(),name+" unexpected CPU estimate");const auto&e=c.at("estimates")[estimates++];check(input==e.at("input").get<std::uint32_t>(),name+" CPU input differs");return e.at("output").get<std::uint32_t>();};
        services.timestamp_counter=[&](){
            check(c.contains("timestamps")&&timestamps<c.at("timestamps").size(),name+" unexpected CPU timestamp");
            return c.at("timestamps")[timestamps++].at("output").get<std::uint64_t>();
        };
        services.required_owner=[](auto&,const auto&call,auto&,const auto&){throw Stop{call};};
        auto run=[&](auto&memory,auto&a,const auto&s,const auto&o){halo2::source_run_physics_vehicle_runtime(memory,c.at("entry").get<std::uint32_t>(),a,
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
        // Exercise the production structured kernels without instruction
        // observation as well as the original instruction-trace reference.
        // Compare the complete ABI, ordered stores and all memory banks.
        {
            auto native_memory=initial;auto native_state=abi(c.at("entry_abi"));
            auto native_services=services;native_services.instruction={};
            estimates=0;timestamps=0;bool native_stopped=false;
            std::vector<halo2::SourcePhysicsContactRecordStore> native_stores;
            try{run(native_memory,native_state,native_services,
                halo2::SourcePhysicsContactRecordObserver([&](const auto&s){native_stores.push_back(s);}));}
            catch(const Stop&e){native_stopped=true;check(e.call.target==c.at("exit_entry").get<std::uint32_t>(),name+" native kernel owner differs");}
            check(native_stopped==stopped&&native_memory.banks==m.banks&&native_stores==stores,
                name+" native kernel changed stores or full memory");
            check(same_abi(native_state,state),
                name+" native kernel changed full ABI");
            for(unsigned n=0;n<state.x87_depth;++n)
                check(std::memcmp(&native_state.x87[n],&state.x87[n],10)==0,
                    name+" native kernel changed extended precision state");
            check(timestamps==c.value("timestamps",J::array()).size()&&estimates==c.at("estimates").size(),
                name+" native kernel changed required CPU inputs");
        }
        if(fixture.contains("native_cursor_immutable_inputs") &&
           (c.at("entry")==0x208d80u || c.at("entry")==0x208e70u)){
            auto native_state=abi(c.at("entry_abi")),reference_state=native_state;
            auto memory=padded(initial,native_state,fixture.at("native_cursor_immutable_inputs")),reference_memory=memory;
            auto reference_services=services;reference_services.instruction=[](auto){};
            run(reference_memory,reference_state,reference_services,halo2::SourcePhysicsContactRecordObserver{});
            std::uint32_t cursor=0;bool first=false,second=false;std::size_t callbacks=0;
            constexpr std::array<std::uint32_t,3> service_sites{0x38846eu,0x21f590u,0x33b595u};
            auto cursor_services=services;
            cursor_services.instruction=[&](auto pc){cursor=pc;++callbacks;};
            cursor_services.instruction_cursor={&cursor,&first,&second,service_sites,std::this_thread::get_id(),
                &cursor_services.instruction.target_type(),true};
            const auto before=halo2::source_vehicle_native_rate_activity();
            run(memory,native_state,cursor_services,halo2::SourcePhysicsContactRecordObserver{});
            const auto after=halo2::source_vehicle_native_rate_activity();
            check(after.live_cursor_calls>before.live_cursor_calls,name+" live cursor did not execute native vehicle kernel");
            cursor_kernels+=after.live_cursor_calls-before.live_cursor_calls;
            check(callbacks==0&&cursor==expected_trace.back(),name+" native kernel changed live final cursor or called a non-service hook");
            check(memory.banks==reference_memory.banks&&same_abi(native_state,reference_state),
                name+" live cursor native kernel changed complete state");
            for(unsigned n=0;n<native_state.x87_depth;++n)
                check(std::memcmp(&native_state.x87[n],&reference_state.x87[n],10)==0,name+" cursor extended state differs");
            if(fixture.contains("native_cursor_site_bindings")) {
                using Remap=halo2::PhysicsInstructionRemapTu15;
                std::vector<Remap::Binding> bindings;
                for(const auto &row:fixture.at("native_cursor_site_bindings"))
                    bindings.push_back({row.at("key"),row.at("value")});
                const auto delta=fixture.at("image")=="1.5"?0x4900u:0u;
                for(unsigned delivery=0;delivery<11;++delivery) {
                    auto table=bindings;auto underlying=services;
                    std::vector<std::uint32_t> delivered;
                    std::uint32_t mapped_cursor{};bool trace1=delivery==1,trace2=delivery==2;
                    underlying.instruction=[&](auto pc){mapped_cursor=pc;delivered.push_back(pc);};
                    underlying.instruction_cursor={&mapped_cursor,&trace1,&trace2,service_sites,
                        std::this_thread::get_id(),&underlying.instruction.target_type(),true};
                    if(delivery==3)underlying.instruction_cursor.executor={};
                    if(delivery==4)underlying.instruction_cursor.callback_type=&typeid(int);
                    if(delivery==5)underlying.instruction_cursor.literal_service_sites=false;
                    if(delivery==6)underlying.instruction_cursor.position=nullptr;
                    if(delivery==7)underlying.instruction_cursor.service_sites={};
                    if(delivery==8) {
                        // Even an unexecuted branch must have its authenticated
                        // mapping; skipping that check can hide a real fault.
                        const auto absent=std::ranges::find_if(table,[&](const auto &row){
                            return std::ranges::find(expected_trace,row.key)==expected_trace.end();});
                        check(absent!=table.end(),name+" mapped refusal needs an unexecuted original instruction");
                        table.erase(absent);
                    }
                    if(delivery==9)table.front().value=0x33b595u;
                    if(delivery==10)table.erase(table.begin());
                    auto mapped_services=underlying;
                    mapped_services.instruction=Remap{&underlying,table};
                    auto mapped_state=abi(c.at("entry_abi"));
                    auto mapped_memory=padded(initial,mapped_state,fixture.at("native_cursor_immutable_inputs"));
                    const auto activity=halo2::source_vehicle_native_rate_activity();
                    bool failed=false;
                    try{run(mapped_memory,mapped_state,mapped_services,halo2::SourcePhysicsContactRecordObserver{});}
                    catch(const std::runtime_error &e){failed=std::string_view(e.what())=="Unbound TU1.5 phantom/world source observation";if(!failed)throw;}
                    if(delivery==0) {
                        const auto actual=halo2::source_vehicle_native_rate_activity();
                        check(actual.live_cursor_calls>activity.live_cursor_calls,name+" mapped live cursor did not execute native vehicle kernel");
                        mapped_kernels+=actual.live_cursor_calls-activity.live_cursor_calls;
                        check(!failed && mapped_cursor==expected_trace.back()+delta &&
                            same_abi(mapped_state,reference_state) && mapped_memory.banks==reference_memory.banks,
                            name+" mapped native kernel changed complete state or final cursor");
                        check(delivered.size()<expected_trace.size(),name+" mapped kernel still delivered every scalar instruction");
                    } else {
                        check(halo2::source_vehicle_native_rate_activity().live_cursor_calls==activity.live_cursor_calls,
                            name+" invalid mapped cursor entered native kernel");
                        auto literal=underlying;literal.instruction_cursor={};
                        auto literal_state=abi(c.at("entry_abi"));
                        auto literal_memory=padded(initial,literal_state,fixture.at("native_cursor_immutable_inputs"));
                        std::vector<std::uint32_t> literal_trace;
                        literal.instruction=[&](auto pc){literal_trace.push_back(pc);};
                        auto composed=literal;composed.instruction=Remap{&literal,table};bool reference_failed=false;
                        try{run(literal_memory,literal_state,composed,halo2::SourcePhysicsContactRecordObserver{});}
                        catch(const std::runtime_error &e){reference_failed=std::string_view(e.what())=="Unbound TU1.5 phantom/world source observation";if(!reference_failed)throw;}
                        check(failed==reference_failed && delivered==literal_trace &&
                            same_abi(mapped_state,literal_state) && mapped_memory.banks==literal_memory.banks,
                            name+" declined mapped cursor changed original trace or fault prefix");
                        ++mapped_fallbacks;
                    }
                }
            }
            for(unsigned delivery=0;delivery<7;++delivery){
                auto declined=padded(initial,abi(c.at("entry_abi")),fixture.at("native_cursor_immutable_inputs"));
                if(delivery==5)declined.offer_views=false;
                auto declined_state=abi(c.at("entry_abi"));auto declined_services=services;
                std::vector<std::uint32_t> declined_trace;bool trace1=delivery==0,trace2=delivery==1;
                declined_services.instruction=[&](auto pc){declined_trace.push_back(pc);};
                declined_services.instruction_cursor={&cursor,&trace1,&trace2,service_sites,std::this_thread::get_id(),
                    &declined_services.instruction.target_type(),delivery!=4};
                if(delivery==2)declined_services.instruction_cursor.executor={};
                if(delivery==3)declined_services.instruction_cursor.callback_type=&typeid(int);
                const auto activity=halo2::source_vehicle_native_rate_activity();
                const halo2::SourcePhysicsContactRecordObserver observed=delivery==6?
                    halo2::SourcePhysicsContactRecordObserver([](const auto&){}):halo2::SourcePhysicsContactRecordObserver{};
                run(declined,declined_state,declined_services,observed);
                check(halo2::source_vehicle_native_rate_activity().live_cursor_calls==activity.live_cursor_calls&&
                    declined_trace==expected_trace&&same_abi(declined_state,reference_state)&&declined.banks==reference_memory.banks,
                    name+" declined native cursor changed its original prefix or completion");
                ++cursor_fallbacks;
            }
            // A guard/observer can reject any original write. Compare both
            // implementations at every such boundary, including scratch and
            // return-address stores, without allowing a fabricated return.
            for(std::size_t stop=0;stop<stores.size();++stop){
                auto literal_memory=initial,native_memory=initial;
                auto literal_state=abi(c.at("entry_abi")),fault_state=literal_state;
                auto literal_services=services,native_services=services;
                literal_services.instruction=[](auto){};native_services.instruction={};
                std::size_t ordinal=0;bool literal_stopped=false,native_stopped=false;
                const halo2::SourcePhysicsContactRecordObserver guard=[&](const auto&){if(ordinal++==stop)throw StoreStop{};};
                try{run(literal_memory,literal_state,literal_services,guard);}catch(const StoreStop&){literal_stopped=true;}
                ordinal=0;
                try{run(native_memory,fault_state,native_services,guard);}catch(const StoreStop&){native_stopped=true;}
                check(literal_stopped&&native_stopped&&same_abi(literal_state,fault_state)&&
                    literal_memory.banks==native_memory.banks,name+" native write-fault prefix differs at store "+std::to_string(stop));
                ++store_faults;
            }
            std::vector<std::pair<std::uint32_t,std::size_t>> reads;
            auto read_memory=initial;auto read_state=abi(c.at("entry_abi"));auto read_services=services;
            read_services.instruction=[](auto){};
            read_memory.read_guard=[&](auto p,auto n){reads.emplace_back(p,n);};
            run(read_memory,read_state,read_services,halo2::SourcePhysicsContactRecordObserver{});
            for(std::size_t stop=0;stop<reads.size();++stop){
                auto literal_memory=initial,native_memory=initial;
                auto literal_state=abi(c.at("entry_abi")),fault_state=literal_state;
                auto literal_services=services,native_services=services;
                literal_services.instruction=[](auto){};native_services.instruction={};
                std::size_t ordinal=0;bool literal_stopped=false,native_stopped=false;
                const auto guard=[&](auto p,auto n){
                    check(ordinal<reads.size()&&reads[ordinal]==std::pair<std::uint32_t,std::size_t>{p,n},
                        name+" native memory read sequence differs at read "+std::to_string(ordinal));
                    if(ordinal++==stop)throw ReadStop{};
                };
                literal_memory.read_guard=guard;native_memory.read_guard=guard;
                try{run(literal_memory,literal_state,literal_services,halo2::SourcePhysicsContactRecordObserver{});}catch(const ReadStop&){literal_stopped=true;}
                ordinal=0;
                try{run(native_memory,fault_state,native_services,halo2::SourcePhysicsContactRecordObserver{});}catch(const ReadStop&){native_stopped=true;}
                if(!same_abi(literal_state,fault_state))std::cerr<<"read "<<stop<<" at "<<h(reads[stop].first)<<" size "<<reads[stop].second
                    <<"\nreference "<<describe_abi(literal_state)<<"\nnative "<<describe_abi(fault_state)<<'\n';
                check(literal_stopped&&native_stopped&&same_abi(literal_state,fault_state)&&
                    literal_memory.banks==native_memory.banks,name+" native read-fault prefix differs at read "+std::to_string(stop));
                ++read_faults;
            }
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
        if(stopped){
            auto absent=initial;auto a=abi(c.at("entry_abi"));auto missing=services;missing.required_owner={};trace.clear();std::vector<halo2::SourcePhysicsContactRecordStore>prefix;bool rejected=false;estimates=0;timestamps=0;
            try{run(absent,a,missing,halo2::SourcePhysicsContactRecordObserver([&](const auto&s){prefix.push_back(s);}));}
            catch(const std::runtime_error&e){rejected=std::string(e.what()).starts_with("Required original contact runtime callee");}
            check(rejected&&trace==expected_trace&&prefix==stores&&absent.banks==m.banks&&a.gp==state.gp&&a.xmm==state.xmm,name+" absent owner fabricated completion or changed prefix");
            check(a.fs_base==state.fs_base&&a.eflags==state.eflags&&a.x87_depth==state.x87_depth&&a.x87_status==state.x87_status,name+" absent owner changed flags/FP prefix");
            for(unsigned n=0;n<a.x87_depth;++n)check(std::memcmp(&a.x87[n],&state.x87[n],10)==0,name+" absent owner changed extended precision prefix");
        }
        ++cases;writes+=stores.size();instructions+=expected_trace.size();
    }
    std::cout<<"PASS original physics vehicle runtime: "<<cases<<" cases, "<<instructions<<" instructions, "<<writes<<" ordered writes, "<<fullbytes<<" full-state bytes; "<<cursor_kernels<<" live-cursor native rate calls, "<<cursor_fallbacks<<" cursor fallbacks, "<<mapped_kernels<<" mapped native calls, "<<mapped_fallbacks<<" mapped refusals, "<<store_faults<<" write-fault prefixes, "<<read_faults<<" read-fault prefixes\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
