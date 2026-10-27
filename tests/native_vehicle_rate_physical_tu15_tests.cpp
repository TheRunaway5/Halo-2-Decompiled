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
    std::function<void(std::uint32_t,std::size_t)> read_guard,write_guard;
    auto at(std::uint32_t p)const{
        auto i=banks.upper_bound(p);if(i==banks.begin())throw std::runtime_error("Missing contact bank @ "+h(p));--i;
        check(std::uint64_t(p-i->first)<i->second.size(),"Missing contact extent @ "+h(p));return i;
    }
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        if(read_guard)read_guard(p,b.size());
        for(auto &x:b){auto i=at(p);x=i->second[p++-i->first];}
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        if(write_guard)write_guard(p,b.size());
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
 check(argc==2,"Supply physical original rate fixture");std::ifstream input(argv[1]);const auto f=J::parse(input);check(f.at("image")=="1.5"&&f.at("oracle_backend")=="original_i386_hardware_single_step","Required physical original rate authority missing");std::size_t cases{},writes{},before_publication_faults{},after_publication_faults{},rounded_up_publications{};
 for(const auto&c:f.at("cases")){
  const auto name="physical_rate_"+std::to_string(c.at("id").get<unsigned>());Memory memory;for(const auto&b:c.at("initial_banks"))memory.banks.emplace(b.at("address"),bytes(b.at("hex")));auto state=abi(c.at("entry_abi"));const auto expected=abi(c.at("exit"));std::uint32_t phase{};std::vector<halo2::SourcePhysicsContactRecordStore>stores;halo2::NativeVehicleRateActivityTu15 activity;halo2::native_vehicle_rate_activity_reset_tu15(memory);
  halo2::SourcePhysicsContactRuntimeServices services;services.instruction=[](auto){throw std::runtime_error("Scoped physical rate unexpectedly entered literal instruction path");};services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Physical scalar reached unowned callee");};
  {
   halo2::NativeVehicleRateConsumerScopeTu15 permission(memory,{f.at("scope_stack_begin"),f.at("scope_stack_end"),[&](auto pc){phase=pc;},nullptr,nullptr,true,&activity});
   halo2::source_run_physics_vehicle_runtime(memory,c.at("entry"),state,halo2::SourcePhysicsContactImage::TitleUpdate15,{c.at("entry_abi").at("x87_control_word"),c.at("entry_abi").at("mxcsr")},services,[&](const auto&e){check(phase==e.instruction+0x4900u,name+" actual store phase differs");stores.push_back(e);});
  }
  for(const auto&snapshot:c.at("rate_fp_snapshots")){
   const auto site=snapshot.at("instruction").get<std::uint32_t>();if(site!=0x208da8u&&site!=0x208db3u)continue;
   if(snapshot.at("after").at("x87_status").get<unsigned>()&0x200u)++rounded_up_publications;
   for(bool completed:{false,true}){
    Memory partial,expected_memory;for(const auto&b:c.at("initial_banks")){partial.banks.emplace(b.at("address"),bytes(b.at("hex")));expected_memory.banks.emplace(b.at("address"),bytes(b.at("hex")));}
    auto partial_state=abi(c.at("entry_abi"));std::uint32_t partial_phase{};bool stopped{};
    for(const auto&store:c.at("writes")){if(store.at("instruction")==site&&!completed)break;expected_memory.write(store.at("address"),bytes(store.at("written")));if(store.at("instruction")==site)break;}
    partial.write_guard=[&](auto,auto){if(!completed&&partial_phase==site+0x4900u)throw StoreStop{};};
    halo2::native_vehicle_rate_activity_reset_tu15(partial);
    {
     halo2::NativeVehicleRateConsumerScopeTu15 permission(partial,{f.at("scope_stack_begin"),f.at("scope_stack_end"),[&](auto pc){partial_phase=pc;},nullptr,nullptr,true,nullptr});
     try{halo2::source_run_physics_vehicle_runtime(partial,c.at("entry"),partial_state,halo2::SourcePhysicsContactImage::TitleUpdate15,{c.at("entry_abi").at("x87_control_word"),c.at("entry_abi").at("mxcsr")},services,[&](const auto&e){if(completed&&e.instruction==site)throw StoreStop{};});}catch(const StoreStop&){stopped=true;}
    }
    const auto&expected_json=snapshot.at(completed?"after":"before");const auto expected_state=abi(expected_json);
    check(stopped&&partial.banks==expected_memory.banks,name+" physical publication fault retained memory differs");
    check(partial_state.gp==expected_state.gp&&partial_state.xmm==expected_state.xmm&&(partial_state.eflags&0xcd5u)==(expected_state.eflags&0xcd5u),name+" physical publication fault GP/XMM/defined flags differs");
    check(partial_state.x87_depth==expected_state.x87_depth&&partial_state.x87_status==expected_state.x87_status,name+" physical publication fault depth/SW differs "+std::to_string(site)+" completed "+std::to_string(completed)+" actual "+h(partial_state.x87_status)+" original "+h(expected_state.x87_status));
    for(unsigned i=0;i<partial_state.x87_depth;++i)check(std::memcmp(&partial_state.x87[i],&expected_state.x87[i],10)==0,name+" physical publication fault live x87 differs");
    check(partial_state.floating_point&&partial_state.floating_point->x87_control_word==expected_json.at("x87_control_word")&&partial_state.floating_point->mxcsr==expected_json.at("mxcsr"),name+" physical publication fault CW/MXCSR differs");
    halo2::native_vehicle_rate_activity_reset_tu15(partial);if(completed)++after_publication_faults;else ++before_publication_faults;
   }
  }
  const auto&original=c.at("writes");check(stores.size()==original.size(),name+" physical store count differs");for(unsigned i=0;i<stores.size();++i){const auto&a=stores[i];const auto&b=original[i];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" physical ordered store differs at "+std::to_string(i)+" native "+hex(a.written)+" original "+b.at("written").get<std::string>());}
  for(const auto&b:c.at("final_banks")){const auto p=b.at("address").get<std::uint32_t>();const auto raw=bytes(b.at("hex"));std::vector<std::byte>actual(raw.size());memory.read(p,actual);check(actual==raw,name+" physical retained bank differs");}
  for(unsigned i=0;i<8;++i)check(state.gp[i]==expected.gp[i],name+" physical GP differs "+std::to_string(i));check(state.xmm==expected.xmm,name+" physical XMM differs");check((state.eflags&0xcd5u)==(expected.eflags&0xcd5u),name+" physical defined flags differ");check(state.x87_depth==expected.x87_depth&&state.fs_base==expected.fs_base,name+" physical live state differs");for(unsigned i=0;i<state.x87_depth;++i)check(std::memcmp(&state.x87[i],&expected.x87[i],10)==0,name+" physical live x87 value differs");
  check(state.x87_status==expected.x87_status,name+" physical full x87 status differs native "+h(state.x87_status)+" original "+h(expected.x87_status));check(state.floating_point&&state.floating_point->x87_control_word==c.at("exit").at("x87_control_word")&&state.floating_point->mxcsr==c.at("exit").at("mxcsr"),name+" physical CW/MXCSR differs");check(activity.acceleration_calls+activity.approach_calls>0&&activity.live_cursor_calls>0&&!activity.observation_refusals,name+" physical case did not execute scoped native math");halo2::native_vehicle_rate_activity_reset_tu15(memory);++cases;writes+=stores.size();
 }
 check(before_publication_faults&&after_publication_faults&&rounded_up_publications,"Physical FSTP pre/post rounding fault corpus missing");
 std::cout<<before_publication_faults<<" rejected-write and "<<after_publication_faults<<" committed-observer physical prefixes with "<<rounded_up_publications<<" rounded-up FSTPs; "<<cases<<" physical original native vehicle rate calls and "<<writes<<" exact stores/full retained banks/GP/XMM/defined flags/full CW/SW/MXCSR passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
