#include "halo2/vista_controller_profile.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string& message) {if(!value)throw std::runtime_error(message);}
std::vector<std::byte> decode(const std::string& s) {
  check((s.size()&1)==0,"odd fixture hex");
  std::vector<std::byte> out;
  for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return out;
}
std::string hex(std::span<const std::byte> value) {
  constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(value.size()*2);
  for(const auto b:value) {const auto x=std::to_integer<unsigned>(b);out+=digits[x>>4];out+=digits[x&15];}
  return out;
}
std::vector<std::byte> pattern(std::size_t size,unsigned salt) {
  std::vector<std::byte> out(size);
  for(std::size_t i=0;i<size;++i)out[i]=std::byte((i*37+salt)&255);
  return out;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[o+i]=std::byte(v>>(8*i));
}
template<std::size_t N> std::array<std::byte,N> value(const std::string& text) {
  const auto bytes=decode(text);check(bytes.size()==N,"fixture value size");
  std::array<std::byte,N> out;std::copy(bytes.begin(),bytes.end(),out.begin());return out;
}
struct Services final:VistaControllerProfileServices {
  const Json& test;std::size_t cursor{};std::vector<std::byte> context;
  explicit Services(const Json& row):test(row),context(decode(row.at("context_hex"))) {}
  void expect(Json actual) {
    check(cursor<test.at("calls").size(),"unexpected service "+actual.dump());
    const auto& expected=test.at("calls").at(cursor++);
    if(actual!=expected) {
      for(const auto* field:{"control_hex","before_hex"})
        if(actual.contains(field) && expected.contains(field) && actual.at(field)!=expected.at(field)) {
          const auto a=actual.at(field).get<std::string>(),e=expected.at(field).get<std::string>();
          const auto different=std::mismatch(a.begin(),a.end(),e.begin(),e.end());
          const auto pos=std::size_t(different.first-a.begin())&~std::size_t{1};
          throw std::runtime_error("service "+std::to_string(cursor-1)+" "+field+" first byte mismatch at "+std::to_string(pos/2)+
            " expected "+e.substr(pos,16)+" got "+a.substr(pos,16));
        }
      throw std::runtime_error("service "+std::to_string(cursor-1)+" expected "+expected.dump()+" got "+actual.dump());
    }
  }
  VistaPlatformXuidResult xuser_get_xuid(std::uint32_t controller) override {
    expect({{"service","XUserGetXUID"},{"controller",controller},
      {"result_word",test.at("hresult_word")},{"supplied_output_hex",test.at("xuid_hex")}});
    return {test.at("hresult_word").get<std::uint32_t>(),value<8>(test.at("xuid_hex"))};
  }
  void release_profile(std::uint32_t index) override {expect({{"service","profile_release"},{"profile_word",index}});}
  void acquire_profile(std::uint32_t index) override {expect({{"service","profile_acquire"},{"profile_word",index}});}
  std::uint8_t apply_input_bindings(std::span<std::byte> controls,VistaInputBindingArguments args) override {
    expect({{"service","binding_layout"},{"argument_words",Json::array({args.selector_5_word,
      args.selector_4_word,args.selector_8_word,args.argument_word})},{"control_hex",hex(controls)}});
    for(const auto& patch:test.at("layout_patches"))controls[patch.at(0).get<std::size_t>()]=std::byte(patch.at(1).get<unsigned>());
    return test.at("layout_result_al").get<std::uint8_t>();
  }
  void refresh_input_dependencies(std::span<const std::byte> controls,std::uint32_t controller) override {
    expect({{"service","input_dependencies"},{"controller",controller},{"control_hex",hex(controls)}});
  }
  VistaControllerPresentationContext presentation_context() override {
    expect({{"service","presentation_context"}});
    return {test.at("context_type").get<std::uint32_t>(),context};
  }
  void format_network_display_name(std::span<std::byte> dest,std::span<const std::byte> text) override {
    expect({{"service","network_display_format"},{"before_hex",hex(dest)},{"context_hex",hex(text)}});
    const auto output=decode(test.at("formatted_write_hex"));
    check(output.size()<=dest.size(),"formatter fixture overflow");
    std::copy(output.begin(),output.end(),dest.begin());
  }
};
void run(const Json& test) {
  const auto c=test.at("controller").get<std::uint32_t>();
  auto controllers=pattern(vista_controller_user_bank_size,0xa6);
  auto controls=pattern(vista_input_control_bank_size,0xb8);
  auto users=decode(test.at("before_users_hex"));
  const auto cr=decode(test.at("before_controller_hex"));
  const auto ic=decode(test.at("before_input_hex"));
  std::copy(cr.begin(),cr.end(),controllers.begin()+c*vista_controller_user_stride);
  std::copy(ic.begin(),ic.end(),controls.begin()+c*vista_input_control_stride);
  const auto original_controllers=controllers,original_controls=controls;
  // Detached guards prove that bank operations respect all three extents.
  controllers.resize(controllers.size()+8,std::byte{0xa5});
  controls.resize(controls.size()+8,std::byte{0xa5});users.resize(users.size()+8,std::byte{0xa5});
  VistaControllerProfileBanks banks{std::span(controllers).first(vista_controller_user_bank_size),
    std::span(users).first(vista_local_user_bank_size),std::span(controls).first(vista_input_control_bank_size)};
  Services services(test);
  const std::string operation=test.at("operation");
  if(operation=="assign") {
    const auto actual=vista_controller_profile_assign(banks,c,value<0x1208>(test.at("profile_hex")),
      test.at("storage_index").get<std::uint32_t>(),test.at("argument_word").get<std::uint32_t>(),services);
    check(actual==test.at("al").get<std::uint8_t>(),"raw returned AL");
  } else if(operation=="publish")vista_controller_profile_publish(banks,c,services);
  else if(operation=="remove")vista_controller_profile_remove(banks,c,services);
  else if(operation=="identity")vista_controller_identity_update(banks,c,value<8>(test.at("supplied_identity_hex")),services);
  else if(operation=="team")vista_controller_team_write(banks.controllers,c,test.at("raw_team_word").get<std::uint32_t>());
  else check(false,"unknown fixture action");
  check(services.cursor==test.at("calls").size(),"missing owner service");
  check(hex(banks.controllers.subspan(c*vista_controller_user_stride,vista_controller_user_stride))==test.at("after_controller_hex").get<std::string>(),"complete controller record");
  check(hex(banks.local_users)==test.at("after_users_hex").get<std::string>(),"complete local-user bank");
  check(hex(banks.input_controls.subspan(c*vista_input_control_stride,vista_input_control_stride))==test.at("after_input_hex").get<std::string>(),"complete input-control record");
  for(std::size_t i=0;i<4;++i)if(i!=c) {
    check(std::equal(controllers.begin()+i*vista_controller_user_stride,controllers.begin()+(i+1)*vista_controller_user_stride,
      original_controllers.begin()+i*vista_controller_user_stride),"unrelated controller record changed");
    check(std::equal(controls.begin()+i*vista_input_control_stride,controls.begin()+(i+1)*vista_input_control_stride,
      original_controls.begin()+i*vista_input_control_stride),"unrelated input-control record changed");
  }
  for(const auto* bank:{&controllers,&controls,&users})
    check(std::all_of(bank->end()-8,bank->end(),[](auto byte){return byte==std::byte{0xa5};}),"bank guard changed");
}
template<class Action> void rejected(Action action,const std::string& reason) {
  bool caught=false;try{action();}catch(const FormatError&){caught=true;}
  check(caught,reason);
}
void admission(const Json& specimen) {
  auto controllers=pattern(vista_controller_user_bank_size,0x33);
  auto users=pattern(vista_local_user_bank_size,0x44);
  auto controls=pattern(vista_input_control_bank_size,0x55);
  put(controllers,0,0);put(controllers,4,0xffffffffu);
  for(unsigned i=0;i<4;++i)users[i*vista_local_user_stride]=std::byte{1};
  const auto first=controllers,second=users,third=controls;
  VistaControllerProfileBanks b{controllers,users,controls};Services s(specimen);
  rejected([&]{vista_controller_profile_assign(b,0,{},1,0,s);},"full bank should reject");
  rejected([&]{vista_controller_identity_update(b,0,{},s);},"identity update full bank should reject");
  check(controllers==first && users==second && controls==third && s.cursor==0,"full-bank admission must be atomic");
  put(controllers,4,7);const auto invalid=controllers;
  rejected([&]{vista_controller_profile_publish(b,0,s);},"invalid mapping should reject");
  rejected([&]{vista_controller_profile_remove(b,0,s);},"invalid removal mapping should reject");
  check(controllers==invalid && users==second && controls==third && s.cursor==0,"invalid mapping must be atomic");
  put(controllers,4,0xffffffffu);const auto valid=controllers;
  rejected([&]{vista_controller_profile_publish(b,4,s);},"controller4 should reject");
  rejected([&]{vista_controller_team_write(b.controllers,4,9);},"team controller4 should reject");
  auto short_bank=b;short_bank.controllers=short_bank.controllers.first(vista_controller_user_bank_size-1);
  rejected([&]{vista_controller_profile_publish(short_bank,0,s);},"truncated controller bank should reject");
  rejected([&]{vista_controller_team_write(short_bank.controllers,0,9);},"truncated team controller bank should reject");
  short_bank=b;short_bank.local_users=short_bank.local_users.first(vista_local_user_bank_size-1);
  rejected([&]{vista_controller_profile_remove(short_bank,0,s);},"truncated user bank should reject");
  short_bank=b;short_bank.input_controls=short_bank.input_controls.first(vista_input_control_bank_size-1);
  rejected([&]{vista_controller_profile_assign(short_bank,0,{},1,0,s);},"truncated input bank should reject");
  auto alias=b;alias.local_users=alias.controllers.first(vista_local_user_bank_size);
  rejected([&]{vista_controller_profile_publish(alias,0,s);},"aliased bank should reject");
  check(controllers==valid && users==second && controls==third && s.cursor==0,"admission must preserve banks and callbacks");
}
}
int main(int argc,char** argv) {
  try {
    check(argc==2,"usage: vista_controller_profile_tests fixture.json");
    std::ifstream stream(argv[1]);check(bool(stream),"cannot open fixture");
    const auto fixture=Json::parse(stream);
    check(fixture.at("version")=="H2V_CONTROLLER_PROFILE_COMPLETE_CHAIN_V1","fixture version");
    check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","source pin");
    for(const auto& row:fixture.at("cases"))try{run(row);}catch(const std::exception& e){throw std::runtime_error(row.at("id").get<std::string>()+": "+e.what());}
    admission(fixture.at("cases").at(0));
    std::cout<<"PASS "<<fixture.at("cases").size()<<" unchanged Vista controller/profile actions; complete controller, user and input-control banks, raw AL and ordered owner calls\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
