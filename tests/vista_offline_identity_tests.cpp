#include "halo2/vista_offline_identity.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const std::string& message) {
  if(!ok)throw std::runtime_error(message);
}
std::vector<std::string> split(const std::string& text) {
  std::istringstream input(text);std::vector<std::string> result;std::string item;
  while(std::getline(input,item,'\t'))result.push_back(item);
  return result;
}
std::uint64_t integer(const std::string& text,int base=10) {
  std::size_t used{};const auto value=std::stoull(text,&used,base);
  check(used==text.size(),"Invalid fixture number");return value;
}
std::vector<std::byte> bytes(const std::string& text) {
  if(text=="-")return {};
  check(text.size()%2==0,"Invalid fixture hex size");
  std::vector<std::byte> result(text.size()/2);
  for(std::size_t i=0;i<result.size();++i)
    result[i]=std::byte(integer(text.substr(i*2,2),16));
  return result;
}
template<std::size_t N> std::array<std::byte,N> bank(const std::string& text) {
  const auto b=bytes(text);check(b.size()==N,"Invalid original bank size");
  std::array<std::byte,N> result;std::copy(b.begin(),b.end(),result.begin());return result;
}
void compare(std::span<const std::byte> actual,std::span<const std::byte> expected,
    const std::string& label) {
  check(actual.size()==expected.size(),label+": byte counts differ");
  for(std::size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i]) {
    std::ostringstream message;message<<label<<": source bank differs at +0x"<<std::hex<<i
      <<" (native "<<std::to_integer<unsigned>(actual[i])
      <<", original "<<std::to_integer<unsigned>(expected[i])<<')';
    throw std::runtime_error(message.str());
  }
}
std::uint32_t get(std::span<const std::byte> data,std::size_t offset) {
  std::uint32_t result{};
  for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(data[offset+i])<<(8*i);
  return result;
}
class Services final:public VistaOfflineIdentityServices {
public:
  VistaOfflineIdentityState& state;
  const std::vector<std::string>& fixture;
  std::vector<std::string> calls;
  unsigned random_count{};
  explicit Services(VistaOfflineIdentityState& value,const std::vector<std::string>& row)
    :state(value),fixture(row) {}
  void random_bytes(std::span<std::byte> destination) override {
    check(random_count<2,"Unexpected extra random request");
    check(destination.size()==(random_count?20u:6u),"Source RNG request order differs");
    check(std::all_of(destination.begin(),destination.end(),[](auto b){return b==std::byte{0x6d};}),
        "Offline orchestration silently cleared the caller's random scratch");
    const auto output=bytes(fixture[random_count?7:6]);
    std::copy(output.begin(),output.end(),destination.begin());
    calls.push_back(random_count?"random20":"random6");++random_count;
  }
  std::int32_t crt_rand() override {
    calls.push_back("rand");return std::int32_t(std::stoll(fixture[2]));
  }
  std::uint32_t interface_ipv4() override {
    calls.push_back("interface");
    check(std::all_of(state.machine.begin(),state.machine.end(),[](auto b){return b==std::byte{};}),
        "Machine owner was not cleared before interface service");
    check(get(state.user_info,12)==1,"Interface observed wrong user sign-in state");
    return std::uint32_t(integer(fixture[5],16));
  }
  std::uint32_t inet_addr(std::string_view text) override {
    calls.push_back(std::string(text));
    std::istringstream input{std::string(text)};std::string part;std::uint32_t result{};
    for(unsigned i=0;i<4;++i) {
      check(bool(std::getline(input,part,'.')),"Invalid source inet_addr string");
      const auto octet=integer(part);check(octet<256,"Invalid source IPv4 byte");
      result|=std::uint32_t(octet)<<(8*i);
    }
    return result;
  }
  void initialize_machine_context(std::span<std::byte> context) override {
    calls.push_back("context");check(context.size()==0x218,"Wrong source machine context extent");
    check(state.machine[0x30]==std::byte{},"Machine became valid before required context owner");
    check(std::all_of(context.begin(),context.end(),[](auto b){return b==std::byte{};}),
        "Context was not cleared by original whole-bank initialization");
    // Declared original-oracle external service output, not an OS context claim.
    const std::array<std::byte,4> marker={std::byte{0x77},std::byte{0x66},std::byte{0x55},std::byte{0x44}};
    std::copy(marker.begin(),marker.end(),context.begin());
  }
  void master_status_changed(std::uint32_t signin_state) override {
    calls.push_back("status");check(signin_state==1,"Wrong source status argument");
    check(state.machine[0x30]==std::byte{1},"Status preceded actual machine validity write");
    check(get(state.machine,0x90)==0x44556677,"Required context owner output was dropped");
    check(state.login_token=="prior token storage","Token ownership changed before source status service");
  }
};
void account(const std::vector<std::string>& row) {
  check(row.size()==14,"Invalid account fixture row");
  VistaOfflineIdentityState state{bank<40>(row[8]),bank<0x2a8>(row[9]),"prior token storage"};
  VistaOfflineIdentityScratch scratch;scratch.enet.fill(std::byte{0x6d});scratch.online.fill(std::byte{0x6d});
  Services services(state,row);
  const auto result=vista_offline_identity_configure(state,scratch,
      std::uint32_t(integer(row[3],16)),std::uint16_t(integer(row[4])),services);
  check(result==(row[12]=="1"),row[1]+": account return differs");
  compare(state.user_info,bytes(row[10]),row[1]+": whole user bank");
  compare(state.machine,bytes(row[11]),row[1]+": whole machine bank");
  compare(scratch.enet,bytes(row[6]),row[1]+": caller enet scratch");
  compare(scratch.online,bytes(row[7]),row[1]+": caller online scratch");
  check(state.login_token==row[13],row[1]+": retained token differs");
  std::vector<std::string> expected={"random6","random20","rand"};
  if(result) {
    const std::vector<std::string> ips={"interface","192.168.0.0","255.255.0.0","10.0.0.0",
      "255.0.0.0","172.16.0.0","255.240.0.0","127.0.0.0","255.0.0.0"};
    expected.insert(expected.end(),ips.begin(),ips.end());
    // Compare the fixture-declared branch, independently of native mask arithmetic.
    if(row[1]=="account-2" || row[1]=="account-3" || row[1]=="account-5" ||
        row[1]=="account-8")expected.push_back("127.0.0.1");
    expected.push_back("context");expected.push_back("status");
  }
  check(services.calls==expected,row[1]+": external owner service order/branch differs");
}
template<class F> void rejects(F function,const char* label) {
  try {function();}catch(const std::invalid_argument&){return;}
  throw std::runtime_error(std::string(label)+": missing owner/input was silently accepted");
}
void controls() {
  std::array<std::byte,40> user{};std::array<std::byte,8> xuid;xuid.fill(std::byte{0x55});
  const auto xuid_before=xuid;
  rejects([&]{vista_offline_identity_xuid(std::span(user).first(39),xuid);},"truncated user");
  check(xuid==xuid_before,"Truncated user rejection modified output");
  std::array<std::byte,0x2a8> machine{};std::array<std::byte,36> title;title.fill(std::byte{0x7f});
  const auto title_before=title;
  rejects([&]{vista_offline_identity_machine(std::span(machine).first(0x2a7),title);},"truncated machine");
  check(title==title_before,"Truncated machine rejection modified output");
  VistaXnetRandomState random;random.permutation.fill(0xa5);
  const auto random_before=random;std::array<std::byte,6> data;data.fill(std::byte{0x33});
  const auto data_before=data;
  rejects([&]{vista_xnet_random(random,data,{});},"missing actual counter owner");
  check(random.permutation==random_before.permutation && random.i==random_before.i &&
      random.j==random_before.j && random.initialized==random_before.initialized && data==data_before,
      "Missing actual counter changed retained RNG state or destination");
  rejects([&]{vista_xnet_random(random,{},{});},"empty first random call still requires counter");
  unsigned calls{};vista_xnet_random(random,{},[&]{++calls;return 0x123456789abcdef0ull;});
  check(random.initialized && calls==1,"Empty first call did not initialize exact retained RNG");
  vista_xnet_random(random,data,{});check(calls==1 && data!=data_before,"Retained RNG requested a new seed");
}
} // namespace

int main(int argc,char** argv) {
  try {
    check(argc==2,"Expected unchanged-DLL fixtures.tsv");std::ifstream input(argv[1]);
    check(bool(input),"Cannot open unchanged-DLL fixture");
    std::map<std::string,VistaXnetRandomState> states;std::map<std::string,unsigned> counter_calls;
    unsigned accounts{},randoms{},rands{},xuids{},titles{};std::string line;
    while(std::getline(input,line)) {
      const auto row=split(line);check(!row.empty(),"Empty fixture row");
      if(row[0]=="account") {account(row);++accounts;}
      else if(row[0]=="random") {
        check(row.size()==8,"Invalid random fixture row");auto data=bytes(row[4]);
        auto& state=states[row[1]];
        vista_xnet_random(state,data,[&]{++counter_calls[row[1]];return integer(row[2]);});
        compare(data,bytes(row[5]),row[1]+": original retained XOR stream");
        check(counter_calls[row[1]]==1,"Random first/empty call or retained seed history differs");
        if(row[3]=="4") {
          compare(std::as_bytes(std::span(state.permutation)),bytes(row[6]),row[1]+": final permutation");
          const std::array<std::byte,2> ij={std::byte(state.i),std::byte(state.j)};
          compare(ij,bytes(row[7]),row[1]+": final indices");
        }
        ++randoms;
      } else if(row[0]=="rand") {
        check(row.size()==4,"Invalid CRT fixture row");auto state=std::uint32_t(integer(row[1]));
        check(vista_crt_rand(state)==std::int32_t(integer(row[3])) && state==integer(row[2]),
            "Original CRT rand state/output differs");++rands;
      } else if(row[0]=="xuid") {
        check(row.size()==5,"Invalid XUID fixture row");std::array<std::byte,8> output;output.fill(std::byte{0xa5});
        const auto user=bytes(row[2]);const auto original=user;
        check(vista_offline_identity_xuid(user,output)==integer(row[4]),"Original XUID result differs");
        compare(output,bytes(row[3]),"Original selected-bank XUID output");
        check(user==original,"XUID read changed retained bank");++xuids;
      } else if(row[0]=="title") {
        check(row.size()==5,"Invalid title-address fixture row");std::array<std::byte,36> output;output.fill(std::byte{0xa5});
        const auto machine=bytes(row[2]);const auto original=machine;
        check(vista_offline_identity_machine(machine,output)==integer(row[4]),"Original title-address result differs");
        compare(output,bytes(row[3]),"Original title-address output");
        check(machine==original,"Title-address read changed retained bank");++titles;
      } else throw std::runtime_error("Unknown fixture row "+row[0]);
    }
    check(accounts==9 && randoms==15 && rands==5 && xuids==8 && titles==5,
        "Incomplete original fixture set");controls();
    std::cout<<"PASS supplied DLL offline identity: 9 complete user/machine banks, 15 retained XOR RNG calls, "
      "5 CRT states, 8 selected XUID tails, 5 title-address bodies; required external owners, "
      "signed high words, zero-XUID rejection and caller scratch preserved\n";
    std::cout<<"LIMIT account setup/machine/RNG use original unchanged instructions with explicit services; "
      "full protected XUserGetXUID dispatch, OS context, actual host RNG/interface/configuration bindings "
      "and original offline caller stack initialization remain separate owners\n";
    return 0;
  }catch(const std::exception& error) {std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
