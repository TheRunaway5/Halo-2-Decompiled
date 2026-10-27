#include "halo2/physics_vehicle_runtime.hpp"
#include "halo2/source_vehicle_native_rates.hpp"
#include "halo2/native_vehicle_rate_consumer_tu15.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/vehicles/native_vehicle_rate_math_tu15.hpp"
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
// Plain native value ownership exercises the complete rate algorithms without
// a guest register file, stack, address dispatcher, or instruction callback.
struct NativeRateValues:halo2::native_vehicle_rate_math_tu15::RateEvaluationObserver {
    using Rate=halo2::native_vehicle_rate_math_tu15::ScalarRate;
    float &value;
    const std::array<float,4> &rates;
    float direction_value,seconds_value,target_value,unit_value;
    std::vector<std::uint32_t> &publications;
    float child_direction{},child_seconds{};
    NativeRateValues(float &v,const std::array<float,4> &r,float d,float seconds,float t,float one,
        std::vector<std::uint32_t> &p):value(v),rates(r),direction_value(d),seconds_value(seconds),
        target_value(t),unit_value(one),publications(p) {}
    Rate direction()const {return {direction_value};}
    Rate seconds()const {return {seconds_value};}
    Rate acceleration_coefficient()const {return {rates[2]};}
    Rate deceleration_coefficient()const {return {rates[3]};}
    Rate acceleration_step()const {return {direction_value};}
    Rate deceleration_step()const {return {seconds_value};}
    Rate current()const {return {value};}
    Rate positive_limit()const {return {rates[0]};}
    Rate negative_limit()const {return {rates[1]};}
    Rate unit()const {return {unit_value};}
    Rate target()const {return {target_value};}
    void publish_acceleration(const halo2::native_vehicle_rate_math_tu15::RateDelta&next) {direction_value=next.published();}
    void publish_deceleration(const halo2::native_vehicle_rate_math_tu15::RateDelta&next) {seconds_value=next.published();}
    void publish(Rate next) {value=next.value;publications.push_back(std::bit_cast<std::uint32_t>(value));}
    void publish_increased(Rate next) {publish(next);}
    void publish_positive_limit(Rate next) {publish(next);}
    void publish_decreased(Rate next) {publish(next);}
    void publish_negative_acceleration(Rate next) {publish(next);}
    void publish_final(Rate next) {publish(next);}
    void publish_target(Rate next) {publish(next);}
    void begin_approach() {}
    Rate backward_seconds()const {return seconds();}
    Rate backward_direction()const {return direction();}
    void prepare_backward(Rate seconds,Rate direction) {child_seconds=seconds.value;child_direction=direction.value;}
    Rate forward_seconds()const {return seconds();}
    void prepare_forward_seconds(Rate seconds) {child_seconds=seconds.value;}
    Rate forward_direction()const {return direction();}
    void prepare_forward_direction(Rate direction) {child_direction=direction.value;}
    void advance() {
        NativeRateValues child{value,rates,child_direction,child_seconds,target_value,unit_value,publications};
        halo2::native_vehicle_rate_math_tu15::accelerate_rate(child);
    }
};
float read_float(const Memory &memory,std::uint32_t address) {
    std::array<std::byte,4> raw;memory.read(address,raw);return std::bit_cast<float>(raw);
}
}
int main(int argc,char**argv){try{
    // Scope lookup is executor-local and chooses the nearest matching memory.
    Memory outer_memory,inner_memory;halo2::NativeVehicleRateActivityTu15 outer_activity,inner_activity;
    check(!halo2::native_vehicle_rate_consumer_policy_tu15(outer_memory),"Unexpected initial native rate scope");
    {
        halo2::NativeVehicleRateConsumerScopeTu15 outer(outer_memory,{0x1000,0x2000,[](auto){},nullptr,nullptr,true,&outer_activity});
        const auto*outer_policy=halo2::native_vehicle_rate_consumer_policy_tu15(outer_memory);check(outer_policy&&outer_policy->world_activity==&outer_activity,"Outer native rate scope missing");
        std::thread other([&]{check(!halo2::native_vehicle_rate_consumer_policy_tu15(outer_memory),"Native rate scope leaked to another executor");});other.join();
        {
            halo2::NativeVehicleRateConsumerScopeTu15 inner(inner_memory,{0x3000,0x4000,[](auto){},nullptr,nullptr,true,&inner_activity});
            check(halo2::native_vehicle_rate_consumer_policy_tu15(outer_memory)==outer_policy&&halo2::native_vehicle_rate_consumer_policy_tu15(inner_memory)->world_activity==&inner_activity,"Nested distinct-memory native rate scope changed owner");
            {
                halo2::NativeVehicleRateConsumerScopeTu15 same(outer_memory,{0x5000,0x6000,[](auto){},nullptr,nullptr,true,&inner_activity});
                check(halo2::native_vehicle_rate_consumer_policy_tu15(outer_memory)->caller_stack_begin==0x5000,"Nested same-memory scope did not select nearest owner");
            }
            check(halo2::native_vehicle_rate_consumer_policy_tu15(outer_memory)==outer_policy,"Nested same-memory scope did not restore owner");
        }
        check(!halo2::native_vehicle_rate_consumer_policy_tu15(inner_memory),"Ended nested scope remains admitted");
    }
    check(!halo2::native_vehicle_rate_consumer_policy_tu15(outer_memory),"Ended outer scope remains admitted");
    check(argc==2,"Supply original vehicle runtime fixture");if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream in(argv[1]);const auto fixture=J::parse(in);std::size_t cases=0,writes=0,instructions=0,fullbytes=0,cursor_kernels=0,store_faults=0,read_faults=0,cursor_fallbacks=0,mapped_kernels=0,mapped_fallbacks=0,observed_kernels=0,observed_store_faults=0,mutating_observer_refusals=0,typed_value_cases=0;
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
        {
            auto memory=initial;auto candidate=abi(c.at("entry_abi"));auto own_services=services;std::uint32_t actual_phase{};std::vector<halo2::SourcePhysicsContactRecordStore> actual_stores;bool trace1=false,trace2=false;halo2::NativeVehicleRateActivityTu15 activity;
            halo2::native_vehicle_rate_activity_reset_tu15(memory);
            {
                halo2::NativeVehicleRateConsumerScopeTu15 permission(memory,{fixture.at("scope_stack_begin"),fixture.at("scope_stack_end"),[&](auto pc){actual_phase=pc;},&trace1,&trace2,true,&activity});
                estimates=0;timestamps=0;bool ended{};
                try{run(memory,candidate,own_services,[&](const auto&store){if(store.instruction>=0x208d80&&store.instruction<=0x208ec4)check(actual_phase==store.instruction+0x4900u,name+" scope published wrong actual store phase");actual_stores.push_back(store);});}catch(const Stop&){ended=true;}
                check(ended==stopped&&memory.banks==m.banks&&actual_stores==stores&&same_abi(candidate,state),name+" scoped native rate changed complete ABI/stores/retained memory");
                if(c.at("entry")==0x208d80u||c.at("entry")==0x208e70u){check(activity.acceleration_calls+activity.approach_calls>0&&activity.live_cursor_calls>0&&activity.observation_refusals==0,name+" scoped scalar did not execute native kernel");check(actual_phase==expected_trace.back()+0x4900u,name+" scope final actual phase differs");}
                const auto keyed=halo2::native_vehicle_rate_activity_tu15(memory);check(keyed.acceleration_calls==activity.acceleration_calls&&keyed.approach_calls==activity.approach_calls&&keyed.live_cursor_calls==activity.live_cursor_calls,name+" per-world rate activity differs");
            }
            halo2::native_vehicle_rate_activity_reset_tu15(memory);
        }
        if(c.at("entry")==0x208d80u||c.at("entry")==0x208e70u){
            for(unsigned refusal=0;refusal<4;++refusal){
                auto memory=initial;auto candidate=abi(c.at("entry_abi"));auto before=candidate;before.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{c.at("entry_abi").at("x87_control_word"),c.at("entry_abi").at("mxcsr")};bool trace1=refusal==0,trace2=refusal==1;halo2::NativeVehicleRateActivityTu15 activity;
                halo2::native_vehicle_rate_activity_reset_tu15(memory);bool refused{};
                {
                    const auto stack_begin=refusal==3?before.gp[4]+4u:fixture.at("scope_stack_begin").get<std::uint32_t>();
                    halo2::NativeVehicleRateConsumerScopeTu15 permission(memory,{stack_begin,fixture.at("scope_stack_end"),[](auto){},&trace1,&trace2,refusal!=2,&activity});
                    try{run(memory,candidate,services,[](const auto&){});}catch(const std::runtime_error&e){refused=std::string_view(e.what()).starts_with("Native live vehicle rate");}
                }
                check(refused&&activity.observation_refusals==1&&activity.acceleration_calls+activity.approach_calls==0&&same_abi(candidate,before)&&memory.banks==initial.banks,name+" scoped unsupported observer/trace/stack silently fell back");halo2::native_vehicle_rate_activity_reset_tu15(memory);
            }
            for(std::size_t stop=0;stop<stores.size();++stop){
                auto literal_memory=initial,scoped_memory=initial;auto literal=abi(c.at("entry_abi")),candidate=literal;std::size_t literal_ordinal{},scoped_ordinal{};bool literal_ended{},scoped_ended{};std::uint32_t literal_phase{},actual_phase{};std::vector<halo2::SourcePhysicsContactRecordStore> left,right;
                auto literal_services=services;literal_services.instruction=[&](auto pc){literal_phase=pc;};
                try{run(literal_memory,literal,literal_services,[&](const auto&store){left.push_back(store);if(literal_ordinal++==stop)throw StoreStop{};});}catch(const StoreStop&){literal_ended=true;}
                halo2::native_vehicle_rate_activity_reset_tu15(scoped_memory);
                {
                    halo2::NativeVehicleRateConsumerScopeTu15 permission(scoped_memory,{fixture.at("scope_stack_begin"),fixture.at("scope_stack_end"),[&](auto pc){actual_phase=pc;},nullptr,nullptr,true,nullptr});
                    try{run(scoped_memory,candidate,services,[&](const auto&store){right.push_back(store);if(scoped_ordinal++==stop)throw StoreStop{};});}catch(const StoreStop&){scoped_ended=true;}
                }
                check(literal_ended&&scoped_ended&&left==right&&literal_memory.banks==scoped_memory.banks&&same_abi(literal,candidate)&&actual_phase==literal_phase+0x4900u,name+" scoped completed-store rejection prefix differs");++store_faults;halo2::native_vehicle_rate_activity_reset_tu15(scoped_memory);
            }
            std::vector<std::pair<std::uint32_t,std::size_t>> reads;auto read_memory=initial;auto read_state=abi(c.at("entry_abi"));
            read_memory.read_guard=[&](auto p,auto n){reads.emplace_back(p,n);};run(read_memory,read_state,services,halo2::SourcePhysicsContactRecordObserver{});
            for(std::size_t stop=0;stop<reads.size();++stop){
                auto literal_memory=initial,scoped_memory=initial;auto literal=abi(c.at("entry_abi")),candidate=literal;std::size_t left{},right{};bool literal_ended{},scoped_ended{};
                literal_memory.read_guard=[&](auto,auto){if(left++==stop)throw ReadStop{};};scoped_memory.read_guard=[&](auto,auto){if(right++==stop)throw ReadStop{};};
                try{run(literal_memory,literal,services,halo2::SourcePhysicsContactRecordObserver{});}catch(const ReadStop&){literal_ended=true;}
                halo2::native_vehicle_rate_activity_reset_tu15(scoped_memory);
                {
                    halo2::NativeVehicleRateConsumerScopeTu15 permission(scoped_memory,{fixture.at("scope_stack_begin"),fixture.at("scope_stack_end"),[](auto){},nullptr,nullptr,true,nullptr});
                    try{run(scoped_memory,candidate,services,halo2::SourcePhysicsContactRecordObserver{});}catch(const ReadStop&){scoped_ended=true;}
                }
                check(literal_ended&&scoped_ended&&literal_memory.banks==scoped_memory.banks&&same_abi(literal,candidate),name+" scoped operand-read fault prefix differs");++read_faults;halo2::native_vehicle_rate_activity_reset_tu15(scoped_memory);
            }
        }
        ++cases;writes+=stores.size();instructions+=trace.size();
    }
    std::cout<<cases<<" original TU1.5 cases, "<<writes<<" original stores, "<<store_faults<<" scoped store-observer prefixes, "<<read_faults<<" scoped read-fault prefixes, full ABI/banks and explicit live refusal checks passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
