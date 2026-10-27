#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
namespace {
using J=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string hex(std::span<const std::byte> input){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:input){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
struct Bank final:halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::byte> data;
  explicit Bank(const J& rows){for(const auto& row:rows)apply(row.at("address"),row.at("hex"));}
  void apply(std::uint32_t p,const std::string& raw){for(unsigned n=0;n<raw.size()/2;++n)data[p+n]=std::byte(std::stoul(raw.substr(n*2,2),nullptr,16));}
  void read(std::uint32_t p,std::span<std::byte> out)const override{for(auto& b:out){const auto at=data.find(p++);check(at!=data.end(),"Missing alias retained read");b=at->second;}}
  void write(std::uint32_t p,std::span<const std::byte> input)override{for(auto b:input)data[p++]=b;}
};
constexpr std::array callers{0x1dabbeu,0x1daf99u,0x1db2d0u,0x1db4dbu};
const std::set<std::uint32_t> sites{0x1da530u,0x1da54eu,0x1da559u,0x1da56eu,0x1da5aeu,0x1da5e1u,0x1da643u,0x1da6eau,0x32e9b7u,0x32e9c3u};
}
int main(int argc,char**argv)try{
  check(argc==2,"Supply original scalar alias fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);check(fixture.at("oracle_backend")=="original_i386_hardware_single_step"&&fixture.at("image")=="1.5","Physical TU1.5 alias observer required");
  std::size_t cases{},mutations{},writes{};
  for(const auto& row:fixture.at("cases")){
    Bank memory(row.at("initial_banks"));const auto& entry=row.at("entry_abi");halo2::NativeLiftScalarConsumerTu15 c;c.call_site=callers[cases%4];c.entry_stack=c.stack_pointer=entry.at("gp")[4];c.scratch_word=entry.at("gp")[1];c.fs_base=entry.at("fs_base");c.continuation=row.at("return_address");c.executor=std::this_thread::get_id();c.allow_reverse=(entry.at("gp")[3].get<std::uint32_t>()&255u)!=0;const auto sw=entry.at("x87_status").get<std::uint16_t>();c.floating_point={entry.at("x87_control_word"),entry.at("mxcsr"),std::uint8_t(sw&63u),sw};
    J stores=J::array();std::set<unsigned> applied;std::uint32_t phase{};
    const auto result=halo2::native_lift_scalar_consumer_tu15(memory,c,[&](auto pc){phase=pc;},[&](const auto& store){stores.push_back(J{{"instruction",store.instruction},{"address",store.address},{"before",hex(store.before)},{"written",hex(store.written)}});unsigned index{};for(const auto& mutation:row.at("observer_mutations")){if(mutation.at("instruction")==store.instruction&&applied.insert(index).second){memory.apply(mutation.at("address"),mutation.at("written"));++mutations;}++index;}});
    const auto& exit=row.at("exit");const auto xmm=exit.at("xmm")[0].get<std::string>();
    if(std::bit_cast<std::uint32_t>(result.result.velocity_delta)!=std::stoul(xmm.substr(24,8),nullptr,16)||result.x87_status!=exit.at("x87_status").get<std::uint16_t>()||result.mxcsr!=exit.at("mxcsr").get<std::uint32_t>()||result.control_word!=exit.at("x87_control_word").get<std::uint16_t>()){std::cerr<<"case="<<row.at("id")<<" name="<<row.at("alias_name")<<" lastphase="<<std::hex<<phase<<std::dec<<'\n';throw std::runtime_error("Original alias result/full FP differs");}
    check(applied.size()==row.at("observer_mutations").size(),"An alias mutation source phase was not reached");check(c.stack_pointer==exit.at("gp")[4].get<std::uint32_t>()&&c.scratch_word==exit.at("gp")[1].get<std::uint32_t>(),"Alias return stack/scratch differs");
    J expected_stores=J::array();for(const auto& event:row.at("writes"))if(sites.contains(event.at("instruction").get<std::uint32_t>()))expected_stores.push_back(J{{"instruction",event.at("instruction")},{"address",event.at("address")},{"before",event.at("before")},{"written",event.at("written")}});
    if(stores!=expected_stores){std::cerr<<"case="<<row.at("id")<<" actual="<<stores.dump()<<" original="<<expected_stores.dump()<<'\n';throw std::runtime_error("Original alias semantic stores differ");}
    Bank expected(row.at("final_banks"));for(const auto&[p,b]:expected.data){if(p>=c.entry_stack-0x1000u&&p<c.entry_stack-4u)continue;check(memory.data.contains(p)&&memory.data.at(p)==b,"Original alias caller retained bank differs");}
    halo2::native_lift_scalar_activity_reset_tu15(memory);writes+=stores.size();++cases;
  }
  check(cases>=16&&mutations>=cases,"Insufficient independent alias cases");std::cout<<J{{"passed",true},{"cases",cases},{"observer_mutations",mutations},{"semantic_writes",writes},{"full_fp_status_and_control",true},{"mutating_live_caller_observer_admitted",false}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
