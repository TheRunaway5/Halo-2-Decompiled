#include "halo2/native_tu15_main_view_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_shadow_frame_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/native_tu15_camera_frame_policy.hpp"
#include "helpers/source_shadow_policy_2c470_reference_20261029.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
using Json=nlohmann::json;unsigned checks{},negatives{};
struct Bank{std::uint32_t address;unsigned bytes;};
constexpr std::array banks{Bank{0x4c65c0u,116u},Bank{0x4c6634u,192u},Bank{0x4c66f4u,21u},Bank{0x4c670cu,4u},Bank{0x4c6830u,1u},Bank{0x4c6839u,1u},Bank{0x4c683cu,4u},Bank{0x4c6840u,6u},Bank{0x4c6848u,8u},Bank{0x473628u,4u},Bank{0x473644u,4u}};
void check(bool ok,const char*why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void reject(F&&f,const char*why){bool bad{};try{f();}catch(const std::exception&){bad=true;}check(bad,why);++negatives;}
std::vector<std::byte> hex(std::string_view s){check(!(s.size()%2),"odd original hex");std::vector<std::byte>b;for(unsigned i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(std::string(s.substr(i,2)),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){unsigned n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
void word(SourceRuntimeMemory&m,unsigned p,unsigned v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
void portable(){
 SourceRuntimeMemory m;mount_native_tu15_main_view_data(m);check(m.checkpoint().regions.size()==11,"main-view exact owner count differs");static_assert(sizeof(SourceViewProjectionBank)==192);
 for(const auto[p,n]:banks){check(!m.is_read_only(p,n),"main-view source destination is immutable");check(m.native_address(p,n).value>UINT32_MAX,"main-view backing narrowed to32 bits");const auto b=m.view(p,n);if(p==0x473644u)check(hex(b)=="9a99593f","main-view authored mutable cold operand differs");else if(p==0x473628u)check(hex(b)=="0000803f","main-view original render scale differs");else check(std::ranges::all_of(b,[](auto v){return v==std::byte{};}),"main-view cold storage presets a source result");}
 for(const auto p:{0x4c65bfu,0x4c6709u,0x4c670au,0x4c670bu,0x4c6710u,0x4c6831u,0x4c6838u,0x4c683au,0x4c683bu,0x4c6846u,0x4c6847u,0x4c6850u,0x4c6854u})reject([&]{m.view(p,1);},"main-view helper claimed an existing owner or unknown gap");
 for(const auto[p,n]:banks)for(const auto delta:{0u,n/2u,n-1u}){SourceRuntimeMemory foreign;foreign.mount(p+delta,std::array{std::byte{0xa5}},SourceRuntimeMemory::Access::ReadWrite,"foreign main-view overlap");const auto old=foreign.checkpoint();reject([&]{mount_native_tu15_main_view_data(foreign);},"main-view cold helper accepted partial foreign backing");check(foreign.checkpoint()==old,"overlap rejection partially published a main-view bank");}
 for(auto user:{0xffffffffu,0u,3u}){word(m,0x4c66f8u,user);check(Bytes(m.view(0x4c66f8u,4)).u32(0)==user,"main-view owner changed actual current-user input");}
 const auto old=m.checkpoint();reject([&]{mount_native_tu15_main_view_data(m);},"main-view cold mount reset a live bank");check(m.checkpoint()==old,"duplicate main-view mount mutated source values");
 auto factory=native_tu15_world_memory({0x60212000u,{},"isolated main-view factory clock"});for(const auto[p,n]:banks)check(std::ranges::equal(factory.view(p,n),m.view(p,n))||p==0x4c66f4u,"normal source-free factory lacks complete main-view ownership");check(factory.view(0x4c6709u,1).size()==1&&factory.view(0x4c6710u,288).size()==288&&factory.view(0x4c6854u,8).size()==8,"main-view helper displaced old policy/environment/frame ownership");
 for(auto p:{0x10000u,0x2b9f0u})reject([&]{factory.view(p,4);},"main-view cold factory imported code/header backing");
}
unsigned original(const Json&proof){
 check(proof.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&proof.at("instruction_patches").empty()&&proof.at("supplied_callee_returns").empty(),"main-view primary authority differs");check(proof.at("whole_policy_cases").size()==12&&proof.at("main_prefix_cases").size()==3&&!proof.at("whole_main_function_claim").get<bool>(),"main-view original coverage differs");unsigned bytes{};
 for(const auto&expected:proof.at("whole_policy_cases")){
  SourceRuntimeMemory m;mount_native_tu15_main_view_data(m);for(const auto[p,n]:banks)std::ranges::fill(m.mutable_view(p,n),std::byte(expected.at("fill").get<unsigned>()));for(const auto&o:proof.at("operands")){const auto b=hex(o.at("bytes").get<std::string>());m.mount(o.at("address"),b,SourceRuntimeMemory::Access::ReadOnly,"authenticated original main-policy operand");}
  constexpr unsigned stack=0x61008000u;m.mount(stack-4096u,std::array<std::byte,8192>{},SourceRuntimeMemory::Access::ReadWrite,"isolated actual policy caller stack");word(m,stack,0x70000000u);SourcePhysicsWorldRuntimeAbi a{};a.gp=expected.at("initial_gp").get<decltype(a.gp)>();a.eflags=0x202;Json stores=Json::array();unsigned instructions{};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++instructions;};services.required_owner=[](auto&,auto&,auto&,auto&){throw std::runtime_error("main-view policy unexpectedly needs supplied callee");};
  test::shadow_policy_2c470_reference(m,a,{0x27f,0x1f80},services,[&](const auto&s){if(std::ranges::any_of(banks,[&](auto b){return s.address>=b.address&&std::uint64_t(s.address)+s.written.size()<=std::uint64_t(b.address)+b.bytes;}))stores.push_back({{"pc",s.instruction},{"address",s.address},{"before",hex(s.before)},{"after",hex(s.written)}});});
  check(a.gp==expected.at("returned_gp").get<decltype(a.gp)>(),"main-view original policy GP differs");check((a.eflags&0xcc5u)==(expected.at("returned_eflags").get<unsigned>()&0xcc5u),"main-view original policy flags differ");check(instructions==expected.at("instructions"),"main-view original policy instruction count differs");check(stores==expected.at("stores"),"main-view original policy ordered stores differ");for(const auto&i:expected.at("postimages")){const auto b=hex(i.at("bytes").get<std::string>());check(std::ranges::equal(m.view(i.at("address"),b.size()),b),"main-view whole policy postimage differs");bytes+=b.size();}
 }
 // Original CPU execution supplies these store bytes. This checks backing
 // ownership/ranges for the complete producer, not a second camera algorithm.
 for(const auto&expected:proof.at("main_prefix_cases")){
  SourceRuntimeMemory m;mount_native_tu15_main_view_data(m);for(const auto&s:expected.at("new_owner_write_extents")){const auto b=hex(s.at("value_hex").get<std::string>());check(b.size()==s.at("bytes"),"original main-prefix store width differs");m.write(s.at("address"),b);}
  for(const auto&i:expected.at("postimages")){const auto b=hex(i.at("bytes").get<std::string>());check(std::ranges::equal(m.view(i.at("address"),b.size()),b),"main-view owner omits genuine caller/projection field bytes");bytes+=b.size();}
 }
 return bytes;
}

// This is a fresh cohort from the sealed physical policy fixture. It does not
// replace, rename or claim the same inputs as the optional historical12+3 test.
unsigned sealed_policy_original(const Json&proof,const Json&flags){
 check(proof.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&proof.at("instruction_patches").empty()&&!proof.at("supplied_original_outcomes").get<bool>(),"sealed policy primary authority differs");
 check(flags.at("passed").get<bool>()&&flags.at("independent_repeat_identical").get<bool>()&&flags.at("all_existing_canonical_outcomes_identical").get<bool>()&&flags.at("instruction_patches").empty()&&!flags.at("supplied_original_outcomes").get<bool>()&&flags.at("cases").size()==12,"fresh physical policy flag authority differs");
 unsigned compared{};
 for(const auto&returned:flags.at("cases")){
  const auto it=std::ranges::find_if(proof.at("cases"),[&](const auto&row){return row.at("id")==returned.at("id");});
  check(it!=proof.at("cases").end(),"fresh policy case is absent from its sealed fixture");const auto&row=*it;
  check(row.at("owner")==0x2c470u&&row.at("entry")==0x2c470u&&!row.at("caller").get<unsigned>()&&row.at("completed").get<bool>()&&row.at("boundary").is_null()&&row.at("physical_verified").get<bool>()&&row.at("input_changes").empty()&&row.at("name")==returned.at("name"),"fresh policy case is not an independent whole original entry");
  auto prepare=[&](SourceRuntimeMemory&memory){
   for(const auto&bank:row.at("banks")){const auto raw=hex(bank.at("before").template get<std::string>());memory.mount(bank.at("address"),raw,SourceRuntimeMemory::Access::ReadWrite,"fresh sealed original policy input");}
   const auto stack=row.at("gp")[4].get<unsigned>();memory.mount(stack,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"actual original policy RET word");word(memory,stack,row.at("stop"));
  };
  auto observe=[](Json&stores){return [&stores](const SourcePhysicsContactRecordStore&store){stores.push_back({{"instruction",store.instruction},{"address",store.address},{"before",hex(store.before)},{"written",hex(store.written)}});};};
  auto postimages=[&](const SourceRuntimeMemory&memory){for(const auto&bank:row.at("banks")){const auto raw=hex(bank.at("after").template get<std::string>());check(std::ranges::equal(memory.view(bank.at("address"),raw.size()),raw),"fresh policy original postimage differs");compared+=raw.size();}};
  SourceRuntimeMemory reference;prepare(reference);SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
  for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=hex(row.at("live_x87")[n].get<std::string>());check(raw.size()==10,"fresh physical extended value width differs");std::memcpy(&abi.x87[n],raw.data(),10);}
  unsigned instructions{};Json reference_stores=Json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++instructions;};services.required_owner=[](auto&,auto&,auto&,auto&){throw std::runtime_error("historical policy unexpectedly requested a supplied callee");};
  test::shadow_policy_2c470_reference(reference,abi,*abi.floating_point,services,observe(reference_stores));
  check(abi.gp==row.at("boundary_gp").get<decltype(abi.gp)>(),"fresh policy reference full GP differs");
  check((abi.eflags&0xcc5u)==(returned.at("returned_eflags").get<unsigned>()&0xcc5u),"fresh policy reference defined flags differ");
  check(instructions==row.at("original_instructions"),"fresh policy reference instruction count differs");
  check(reference_stores==row.at("writes"),"fresh policy reference ordered stores differ");postimages(reference);
  SourceRuntimeMemory native;prepare(native);Json native_stores=Json::array();
  native_tu15_camera_frame_policy(native,row.at("gp")[0],observe(native_stores));
  check(native_stores==row.at("writes"),"fresh semantic policy ordered stores differ");postimages(native);
 }
 return compared;
}
}
int main(int argc,char**argv)try{check(argc==1||argc==2||(argc==4&&std::string_view(argv[1])=="--sealed-policy"),"usage: main-view-test [original-proof.json] | --sealed-policy policy.json flags.json");portable();unsigned compared{};if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream f(argv[1]);compared=original(Json::parse(f));}else if(argc==4){std::ifstream f(argv[2]),g(argv[3]);check(bool(f)&&bool(g),"fresh policy reference inputs are required");compared=sealed_policy_original(Json::parse(f),Json::parse(g));}std::cout<<"PASS "<<checks<<" checks / "<<negatives<<" negatives / "<<compared<<" original bytes; exact main-view owners, genuine policy and caller storage\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
