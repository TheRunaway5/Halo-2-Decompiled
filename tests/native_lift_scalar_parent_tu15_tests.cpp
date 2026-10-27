#include "halo2/physics_phantom_force_tu15.hpp"
#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <stdexcept>
namespace {
using J=nlohmann::json;
void check(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string& text){std::vector<std::byte> out;for(unsigned i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> input){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:input){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::string address(std::uint32_t value){std::ostringstream out;out<<std::hex<<value;return out.str();}
struct Bank final:halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::byte> data;
  explicit Bank(const J& rows){for(const auto& row:rows){const auto p=row.at("address").get<std::uint32_t>();const auto raw=bytes(row.at("hex"));for(unsigned i=0;i<raw.size();++i)check(data.emplace(p+i,raw[i]).second,"Overlapping original bank");}}
  void read(std::uint32_t p,std::span<std::byte> out)const override{for(auto& b:out){const auto at=data.find(p);check(at!=data.end(),"Missing parent retained read at "+address(p));b=at->second;++p;}}
  void write(std::uint32_t p,std::span<const std::byte> input)override{for(auto b:input){const auto at=data.find(p);check(at!=data.end(),"Missing parent retained store at "+address(p));at->second=b;++p;}}
};
halo2::SourcePhysicsContactRuntimeAbi abi(const J& row){
  halo2::SourcePhysicsContactRuntimeAbi result{};result.gp=row.at("gp");result.eflags=row.at("eflags");result.fs_base=row.at("fs_base");result.x87_status=row.at("x87_status");result.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{row.at("x87_control_word"),row.at("mxcsr")};
  result.x87_depth=row.at("x87_values").size();for(unsigned i=0;i<result.x87_depth;++i){const auto raw=bytes(row.at("x87_values")[i]);std::memcpy(&result.x87[i],raw.data(),10);}
  for(unsigned i=0;i<8;++i){const auto raw=row.at("xmm")[i].get<std::string>();for(unsigned lane=0;lane<4;++lane)result.xmm[i][lane]=std::stoul(raw.substr(24-lane*8,8),nullptr,16);}return result;
}
}
int main(int argc,char**argv)try{
  check(argc==2,"Supply complete original1DA750 consumer fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
  check(fixture.at("image")=="1.5"&&fixture.at("oracle_backend")=="original_i386_hardware_single_step","Untouched TU1.5 physical original required");
  std::array<std::uint64_t,4> calls{};std::size_t cases{},writes{},compared_bytes{},private_bytes{},private_before_bytes{};
  for(const auto& row:fixture.at("cases")){
    const auto label="parent "+std::to_string(row.at("id").get<unsigned>());Bank memory(row.at("initial_banks"));auto state=abi(row.at("entry_abi"));const auto original=abi(row.at("exit"));
    halo2::SourcePhysicsFactoryFloatingPoint fp=*state.floating_point;halo2::NativeLiftScalarActivityTu15 owned_activity;std::uint32_t phase{};
    halo2::NativeLiftScalarConsumerPolicyTu15 policy{row.at("stack_begin"),row.at("stack_end"),[&](auto pc){phase=pc;},nullptr,nullptr,true,&owned_activity};
    halo2::NativeLiftScalarConsumerScopeTu15 scope(memory,std::move(policy));check(halo2::native_lift_scalar_consumer_policy_tu15(memory),"Missing memory/thread-bound native scope");
    halo2::SourcePhysicsContactRuntimeServices services;services.instruction=[&](auto pc){phase=pc;};
    services.required_owner=[](auto&,const auto& call,auto&,const auto&){throw std::runtime_error("Required original parent dependency "+address(call.target));};
    J stores=J::array();
    try{halo2::source_run_physics_phantom_force_tu15_runtime(memory,0x1da750u,state,fp,services,[&](const auto& event){stores.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});});}
    catch(const std::exception& error){throw std::runtime_error(label+" at "+address(phase)+": "+error.what());}
    check(state.gp[4]==original.gp[4],label+" caller stack protocol differs");for(auto i:{3u,5u,6u,7u})check(state.gp[i]==original.gp[i],label+" callee-saved register differs");
    check((state.eflags&0xcd5u)==(original.eflags&0xcd5u),label+" caller defined flags differ");
    check(fp.x87_control_word==original.floating_point->x87_control_word&&fp.mxcsr==original.floating_point->mxcsr&&state.x87_status==original.x87_status,label+" complete caller FP differs: nativeSW="+address(state.x87_status)+" originalSW="+address(original.x87_status)+" nativeMX="+address(fp.mxcsr)+" originalMX="+address(original.floating_point->mxcsr));
    check(state.fs_base==original.fs_base&&state.x87_depth==original.x87_depth,label+" caller FS/x87 depth differs");for(unsigned i=0;i<state.x87_depth;++i)check(std::memcmp(&state.x87[i],&original.x87[i],10)==0,label+" live x87 value differs");
    const auto expected_calls=row.at("scalar_calls").get<std::array<std::uint64_t,4>>();check(owned_activity.caller_calls==expected_calls,label+" native callsite activity differs");check(halo2::native_lift_scalar_activity_tu15(memory).caller_calls==expected_calls,label+" memory keyed activity differs");for(unsigned i=0;i<4;++i)calls[i]+=expected_calls[i];
    const auto& expected_stores=row.at("semantic_writes");
    check(stores.size()==expected_stores.size(),label+" semantic store count differs");
    for(unsigned i=0;i<stores.size();++i){
      const auto& actual=stores[i];const auto& expected=expected_stores[i];bool same=actual.at("instruction")==expected.at("instruction")&&actual.at("address")==expected.at("address")&&actual.at("written")==expected.at("written");
      const auto actual_before=bytes(actual.at("before")),expected_before=bytes(expected.at("before"));const auto mask=row.at("private_before_byte_masks")[i].get<std::uint64_t>();check(actual_before.size()==expected_before.size(),label+" semantic store width differs");
      for(unsigned n=0;n<expected_before.size();++n){if(mask&(1ull<<n))++private_before_bytes;else same=same&&actual_before[n]==expected_before[n];}
      if(!same)throw std::runtime_error(label+" semantic store "+std::to_string(i)+" differs: native="+actual.dump()+" original="+expected.dump());
    }
    std::set<std::uint32_t> private_final;for(const auto& region:row.at("original_private_leaf_final_bytes")){const auto first=region.at("address").get<std::uint32_t>();for(unsigned i=0;i<region.at("bytes").get<unsigned>();++i)private_final.insert(first+i);}private_bytes+=private_final.size();
    for(const auto& bank:row.at("final_banks")){const auto p=bank.at("address").get<std::uint32_t>();const auto expected=bytes(bank.at("hex"));for(unsigned i=0;i<expected.size();++i){if(private_final.contains(p+i))continue;const auto found=memory.data.find(p+i);check(found!=memory.data.end()&&found->second==expected[i],label+" caller retained bank differs at "+address(p+i));++compared_bytes;}}
    halo2::native_lift_scalar_activity_reset_tu15(memory);writes+=stores.size();++cases;
  }
  for(auto count:calls)check(count,"An original1DA750 scalar callsite was not exercised");
  std::cout<<J{{"passed",true},{"original_parent_calls",cases},{"native_scalar_calls",calls},{"semantic_writes",writes},{"compared_retained_bytes",compared_bytes},{"private_original_helper_bytes",private_bytes},{"private_leaf_last_writer_before_bytes",private_before_bytes},{"complete_caller_fp_status_and_control",true},{"full_machine_postimage_claimed",false},{"live_multiplayer_proven",false}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
