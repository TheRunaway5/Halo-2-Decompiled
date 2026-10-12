#include "halo2/vista_session_birth.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const std::string& message) {if(!ok)throw std::runtime_error(message);}
struct Words {
  std::string label;
  std::vector<std::uint32_t> words;
  std::size_t cursor{};
  std::uint32_t get() {
    check(cursor<words.size(),label+": truncated original row");return words[cursor++];
  }
  bool flag() {const auto value=get();check(value<2,label+": invalid original boolean");return value!=0;}
  template<std::size_t N> std::array<std::byte,N> bytes() {
    static_assert(N%4==0);std::array<std::byte,N> result;
    for(std::size_t i=0;i<N;i+=4){const auto value=get();for(unsigned j=0;j<4;++j)result[i+j]=std::byte(value>>(j*8));}
    return result;
  }
  void end() {check(cursor==words.size(),label+": extra original words");}
};
std::uint32_t get(std::span<const std::byte> bank,std::size_t offset) {
  std::uint32_t result{};
  for(unsigned i=0;i<4;++i)result|=std::uint32_t(std::to_integer<unsigned char>(bank[offset+i]))<<(i*8);
  return result;
}
void put(std::span<std::byte> bank,std::size_t offset,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)bank[offset+i]=std::byte(value>>(i*8));
}
template<class F> void rejects(F function,const std::string& label) {
  try {function();}catch(const FormatError&){return;}
  throw std::runtime_error(label+": unsupported source context was admitted");
}
template<std::size_t N> void compare(const std::array<std::byte,N>& actual,
    const std::array<std::byte,N>& expected,const std::string& label) {
  for(std::size_t i=0;i<N;++i)if(actual[i]!=expected[i]) {
    std::ostringstream message;message<<label<<": original field bank differs at +0x"<<std::hex<<i;
    throw std::runtime_error(message.str());
  }
}
void controls() {
  std::array<std::byte,vista_session_source_size> source{};
  VistaSessionBirthQueries query{true,true,true,VistaSessionBirthValues{}};
  for(const auto size:{std::size_t(0),vista_session_source_size-1}) {
    const auto before=source;
    rejects([&]{(void)vista_session_local_birth(std::span(source).first(size),query);},"truncated owner");
    check(source==before,"Truncated owner rejection partially wrote the bank");
  }
  for(const auto value:{1u,0x80u,0xffu}) {
    source.fill(std::byte{});source[0x48]=std::byte(value);const auto before=source;
    rejects([&]{(void)vista_session_local_birth(source,query);},"active transport");
    check(source==before,"Unsupported transport partially wrote the bank");
  }
  for(const auto slot:{1u,0xffffffffu}) {
    source.fill(std::byte{});put(source,0x5c,slot);const auto before=source;
    rejects([&]{(void)vista_session_local_birth(source,query);},"nonprimary transport");
    check(source==before,"Unsupported slot partially wrote the bank");
  }
  source.fill(std::byte{});query.values.reset();const auto before=source;
  rejects([&]{(void)vista_session_local_birth(source,query);},"missing consumed service output");
  check(source==before,"Missing consumed values partially wrote the bank");
  // The earlier original 5AD7D0 state guard must win over unconsumed inputs.
  source.fill(std::byte{0x6d});put(source,0x73a0,5);const auto guarded=source;
  const auto inactive=vista_session_local_birth(source,query);
  check(!inactive.accepted && !inactive.call_count && source==guarded,
      "Existing-session guard consumed unsupported constructor context");
  // Failed ordinary queries consume no successful service values.
  for(unsigned failure=0;failure<3;++failure) {
    source.fill(std::byte{0x6d});source[0x48]=std::byte{};put(source,0x5c,0);put(source,0x73a0,0);
    auto expected=source;std::fill(expected.begin()+0x28,expected.begin()+0x48,std::byte{});
    query={failure!=0,failure!=1,failure!=2,std::nullopt};
    const auto result=vista_session_local_birth(source,query);
    check(!result.accepted && result.call_count==failure+1 && source==expected,
        "Failed source query consumed absent success values or lost its partial name clear");
  }
  std::array<std::byte,vista_session_source_size+16> extended{};
  std::fill(extended.begin()+vista_session_source_size,extended.end(),std::byte{0x6d});
  query={true,true,true,VistaSessionBirthValues{}};
  check(vista_session_local_birth(extended,query).accepted,"Admitted larger owner bank rejected");
  check(std::all_of(extended.begin()+vista_session_source_size,extended.end(),
      [](auto byte){return byte==std::byte{0x6d};}),"Birth wrote beyond its source owner extent");
  std::array<std::byte,0x54> manager;manager.fill(std::byte{0x6d});const auto manager_before=manager;
  rejects([&]{vista_session_lifecycle_initialize(std::span(manager).first(0x43),{});},"truncated lifecycle owner");
  check(manager==manager_before,"Truncated manager initialization partially wrote the bank");
  vista_session_lifecycle_initialize(manager,{1,2,3,4,5});
  check(std::equal(manager.begin()+0x44,manager.end(),manager_before.begin()+0x44),"Manager wrote beyond source extent");
  check(std::equal(manager.begin()+0x37,manager.begin()+0x44,manager_before.begin()+0x37),
      "Manager initialization changed pending request or padding");
}
}

int main(int argc,char** argv) {
  try {
    check(argc==2,"Supply original Vista session birth fixture");
    std::ifstream file(argv[1]);check(bool(file),"Missing original Vista birth fixture");
    std::string line;bool version{};unsigned births{},managers{},accepted{},guarded{},failure1{},failure2{},failure3{},value_controls{};
    while(std::getline(file,line)) {
      if(line=="# H2V_SESSION_BIRTH_ORIGINAL_V1")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream row(line);char kind{};Words words;std::size_t count{};
      check(bool(row>>kind>>words.label>>count) && count<20000,"Malformed original birth row");
      std::string token;
      while(row>>token) {
        std::size_t used{};const auto value=std::stoull(token,&used,16);
        check(used==token.size() && value<=0xffffffffu,"Invalid original birth word");
        words.words.push_back(std::uint32_t(value));
      }
      check(words.words.size()==count,words.label+": original word count differs");
      if(kind=='B') {
        VistaSessionBirthQueries query;
        query.local_machine_available=words.flag();query.network_created=words.flag();query.session_key_available=words.flag();
        VistaSessionBirthValues values;
        values.local_machine=words.bytes<36>();values.session_key_id=words.bytes<8>();values.session_key=words.bytes<16>();
        for(auto& clock:values.network_clocks)clock=words.get();values.language=words.get();
        values.random=words.bytes<8>();values.process_clock=words.get();query.values=values;
        auto bank=words.bytes<vista_session_source_size>();const auto expected=words.bytes<vista_session_source_size>();
        const auto original_state=get(bank,0x73a0);const bool expected_al=words.flag();
        const auto result=vista_session_local_birth(bank,query);
        compare(bank,expected,words.label);check(result.accepted==expected_al,words.label+": wrapper AL differs");
        const auto expected_count=words.get();check(result.call_count==expected_count,words.label+": source service count differs");
        for(const auto& call:result.call_intents()) {
          VistaSessionBirthCall expected_call;
          const auto service=words.get();check(service<=9,words.label+": invalid original service");
          expected_call.service=VistaSessionBirthService(service);expected_call.owner=words.get();expected_call.argument_count=words.get();
          check(expected_call.argument_count<=3,words.label+": invalid original scalar argument count");
          for(unsigned i=0;i<expected_call.argument_count;++i)expected_call.arguments[i]=words.get();
          check(call==expected_call,words.label+": source service order/owner/arguments differ");
        }
        accepted+=result.accepted;guarded+=original_state!=0;
        failure1+=result.call_count==1;failure2+=result.call_count==2;failure3+=result.call_count==3;
        value_controls+=words.label.starts_with("value_control_");++births;
      } else if(kind=='M') {
        std::array<std::uint32_t,5> owners;for(auto& owner:owners)owner=words.get();
        auto bank=words.bytes<0x44>();const auto expected=words.bytes<0x44>();
        vista_session_lifecycle_initialize(bank,owners);compare(bank,expected,words.label);++managers;
      } else throw std::runtime_error("Unknown original Vista birth row kind");
      words.end();
    }
    check(file.eof() && version && births==128 && managers==12 && accepted==36 && guarded==64 &&
        failure1==16 && failure2==8 && failure3==4 && value_controls==32,
        "Original Vista birth fixture lacks required coverage");
    controls();
    std::cout<<births<<" original Vista birth banks and ordered service intents, "<<managers
        <<" manager initialization banks matched; no live session activation\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
