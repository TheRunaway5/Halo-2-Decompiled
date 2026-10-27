#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
using namespace halo2;
namespace {
using J=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string hex(std::span<const std::byte> bytes){const char* digits="0123456789abcdef";std::string result;for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}return result;}
struct Bank final:SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::byte> bytes;
  std::function<void(std::uint32_t,std::span<const std::byte>)> guard;
  explicit Bank(const J& banks){for(const auto& row:banks){const auto p=row.at("address").get<std::uint32_t>();const auto raw=row.at("hex").get<std::string>();for(unsigned n=0;n<raw.size()/2;++n)bytes[p+n]=std::byte(std::stoul(raw.substr(n*2,2),nullptr,16));}}
  void read(std::uint32_t p,std::span<std::byte> out)const override{for(unsigned n=0;n<out.size();++n){const auto at=bytes.find(p+n);if(at==bytes.end()){std::cerr<<"missing read="<<std::hex<<p<<" bytes="<<out.size()<<std::dec<<'\n';throw std::runtime_error("Missing native scalar retained read");}out[n]=at->second;}}
  void write(std::uint32_t p,std::span<const std::byte> input)override{if(guard)guard(p,input);for(unsigned n=0;n<input.size();++n)bytes[p+n]=input[n];}
  void apply(const J& event){const auto raw=event.at("written").get<std::string>();const auto p=event.at("address").get<std::uint32_t>();for(unsigned n=0;n<raw.size()/2;++n)bytes[p+n]=std::byte(std::stoul(raw.substr(n*2,2),nullptr,16));}
};
constexpr std::array callers{0x1dabbeu,0x1daf99u,0x1db2d0u,0x1db4dbu};
const std::set<std::uint32_t> semantic_sites{0x1da530u,0x1da54eu,0x1da559u,0x1da56eu,0x1da5aeu,0x1da5e1u,0x1da643u,0x1da6eau,0x32e9b7u,0x32e9c3u};
NativeLiftScalarConsumerTu15 context(const J& row,unsigned index){
  const auto& abi=row.at("entry_abi");NativeLiftScalarConsumerTu15 c;
  c.call_site=callers[index%4];c.entry_stack=c.stack_pointer=abi.at("gp")[4];c.scratch_word=abi.at("gp")[1];c.fs_base=abi.at("fs_base");
  c.continuation=row.at("return_address");c.allow_reverse=(abi.at("gp")[3].get<std::uint32_t>()&255u)!=0;c.executor=std::this_thread::get_id();
  const auto status=abi.at("x87_status").get<std::uint16_t>();c.floating_point={abi.at("x87_control_word"),abi.at("mxcsr"),std::uint8_t(status&0x3fu),status};return c;
}
bool private_frame(std::uint32_t p,std::uint32_t stack){return p>=stack-0x1000u&&p<stack-4u;}
void compare_banks(const Bank& actual,const Bank& expected,std::uint32_t stack){
  for(const auto&[p,value]:expected.bytes)if(!private_frame(p,stack)){
    const auto at=actual.bytes.find(p);if(at==actual.bytes.end()||at->second!=value){std::cerr<<"bank mismatch at="<<std::hex<<p<<std::dec<<'\n';throw std::runtime_error("Native scalar caller-live retained bank mismatch");}
  }
}
}
int main(int argc,char**argv)try{
  check(argc==2,"Supply original TU1.5 scalar composition fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
  check(fixture.at("image")=="1.5"&&fixture.at("oracle_backend")=="original_i386_hardware_single_step","Untouched original hardware fixture required");
  std::size_t calls{},stores{},guard_prefixes{};
  for(const auto& row:fixture.at("cases")){
    check(row.at("complete"),"Complete original scalar return required");Bank memory(row.at("initial_banks"));native_lift_scalar_activity_reset_tu15(memory);
    auto c=context(row,unsigned(calls));J writes=J::array();std::uint32_t phase{};
    SourceLiftScalarOutcome result;
    try{result=native_lift_scalar_consumer_tu15(memory,c,[&](auto site){phase=site;},[&](const auto& event){writes.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});});}
    catch(...){std::cerr<<"case="<<row.at("id")<<" phase="<<std::hex<<phase<<std::dec<<'\n';throw;}
    const auto& exit=row.at("exit");const auto expected=std::stoul(exit.at("xmm")[0].get<std::string>().substr(24,8),nullptr,16);
    if(std::bit_cast<std::uint32_t>(result.result.velocity_delta)!=expected||result.mxcsr!=exit.at("mxcsr").get<std::uint32_t>()||result.x87_status!=exit.at("x87_status").get<std::uint16_t>()||result.control_word!=exit.at("x87_control_word").get<std::uint16_t>()){
      std::cerr<<"case="<<row.at("id")<<" SW="<<std::hex<<result.x87_status<<" original="<<exit.at("x87_status").get<std::uint16_t>()<<std::dec<<'\n';throw std::runtime_error("Native scalar consumer result/full FP mismatch");
    }
    check(c.stack_pointer==exit.at("gp")[4].get<std::uint32_t>()&&c.scratch_word==exit.at("gp")[1].get<std::uint32_t>(),"Native scalar real stack/scratch return mismatch");
    compare_banks(memory,Bank(row.at("final_banks")),c.entry_stack);
    J expected_writes=J::array();for(const auto& event:row.at("writes"))if(semantic_sites.contains(event.at("instruction").get<std::uint32_t>()))expected_writes.push_back(J{{"instruction",event.at("instruction")},{"address",event.at("address")},{"before",event.at("before")},{"written",event.at("written")}});
    if(writes!=expected_writes){std::cerr<<"case="<<row.at("id")<<" actual="<<writes.dump()<<" expected="<<expected_writes.dump()<<'\n';throw std::runtime_error("Native scalar semantic write order mismatch");}
    const auto activity=native_lift_scalar_activity_tu15(memory);check(activity.caller_calls[calls%4]==1,"Missing per-memory native scalar activity");
    stores+=writes.size();native_lift_scalar_activity_reset_tu15(memory);
    // A real retained write guard must reject before the target store, after
    // all earlier source semantic effects. Compare against original prefixes.
    if(calls<32){
      Bank guarded(row.at("initial_banks")),prefix(row.at("initial_banks"));auto g=context(row,unsigned(calls));std::uint32_t guard_phase{};bool rejected{};
      guarded.guard=[&](auto,std::span<const std::byte>){if(guard_phase==0x1da5aeu)throw std::runtime_error("owned step write guard");};
      try{(void)native_lift_scalar_consumer_tu15(guarded,g,[&](auto site){guard_phase=site;});}catch(const std::runtime_error& error){rejected=std::string(error.what())=="owned step write guard";}
      check(rejected,"Native scalar bypassed a retained write guard");
      for(const auto& event:row.at("writes")){if(event.at("instruction")==0x1da5aeu)break;prefix.apply(event);}
      compare_banks(guarded,prefix,g.entry_stack);native_lift_scalar_activity_reset_tu15(guarded);++guard_prefixes;
    }
    ++calls;
  }
  check(calls>100&&guard_prefixes,"Incomplete native scalar consumer coverage");
  std::cout<<J{{"passed",true},{"original_calls",calls},{"semantic_writes",stores},{"store_guard_prefixes",guard_prefixes},{"full_fp_status_and_control",true},{"private_frame_postimage_claimed",false},{"live_parent_1da750_proven",false}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
