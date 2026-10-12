#include "halo2/unit_locomotion.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool value,const std::string &context) {
  if(!value)throw std::runtime_error("Original locomotion differs: "+context);
}
std::vector<std::byte> bytes(const std::string &hex) {
  check(hex.size()%2==0,"hex extent");std::vector<std::byte> out;
  for(std::size_t n=0;n<hex.size();n+=2)out.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
  return out;
}
template<std::size_t N> std::array<std::byte,N> packet(const std::string &hex) {
  const auto value=bytes(hex);check(value.size()==N,"complete packet extent");
  std::array<std::byte,N> out;std::copy(value.begin(),value.end(),out.begin());return out;
}
std::vector<std::string> split(const std::string &text,char delimiter) {
  std::vector<std::string> out;std::istringstream stream(text);
  for(std::string part;std::getline(stream,part,delimiter);)out.push_back(part);
  return out;
}
void unavailable_contract() {
  std::array<std::byte,0x154> request{};std::array<std::byte,0x70> result;
  result.fill(std::byte{0xa5});const auto before=result;
  for(unsigned n=0;n<256;++n) {
    if(n==1)continue;
    const std::array state={std::byte(n)};
    const auto update=halo2::dispatch_source_locomotion(request,state,result);
    check(update.path==halo2::SourceLocomotionPath::unavailable_physical_state && update.store_count==0 && result==before,
        "unmeasured physical dispatch remains unavailable");
  }
  bool threw=false;
  try { halo2::dispatch_source_locomotion(request,{},result); }
  catch(const std::invalid_argument &) { threw=true; }
  check(threw,"missing physical byte rejected");
  // The literal flag branch precedes the physical-state access.
  request[0x14]=std::byte{0x20};
  const auto flag=halo2::dispatch_source_locomotion(request,{},result);
  check(flag.path==halo2::SourceLocomotionPath::forced_class_one && flag.store_count==2,
      "forced path needs no physical dispatch");
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply frozen original locomotion fixture export");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream file(argv[1]);unsigned rows=0,expected_count=0,stores=0,neutral=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream stream(line);std::vector<std::string> r;
      for(std::string value;stream>>value;)r.push_back(value);
      if(r.empty())continue;
      if(r[0]=="M") { check(r.size()==2 && expected_count==0,"fixture count declaration");expected_count=std::stoul(r[1]);continue; }
      check(r.size()==9 && r[0]=="C","fixture schema");
      const auto context="case "+r[1]+" "+r[2];
      check(std::stoul(r[1])==rows,context+" original correlation");
      auto request=packet<0x154>(r[3]);auto result=packet<0x70>(r[4]);
      const auto expected=packet<0x70>(r[5]);auto physical=packet<0x10>(r[6]);
      const auto original_request=request;
      const auto original_physical=physical;
      const auto update=r[2]=="leaf"?halo2::classify_source_locomotion_request(request,result):
          halo2::dispatch_source_locomotion(request,physical,result);
      check(result==expected,context+" all 0x70 result bytes");
      check(request==original_request && physical==original_physical,context+" input buffers untouched");
      const auto path=r[2]=="wrapper_flag20"?halo2::SourceLocomotionPath::forced_class_one:halo2::SourceLocomotionPath::normal_request;
      check(update.path==path && update.store_count==std::stoul(r[7]),context+" measured path/store count");
      const auto captured=r[8]=="-"?std::vector<std::string>{}:split(r[8],';');
      check(captured.size()==update.store_count,context+" original store trace extent");
      for(std::size_t n=0;n<captured.size();++n) {
        const auto fields=split(captured[n],',');check(fields.size()==4,context+" store schema");
        const auto &store=update.stores[n];
        check(store.instruction==std::stoul(fields[0],nullptr,16) && store.offset==std::stoul(fields[1],nullptr,16) &&
            store.width==std::stoul(fields[2]) && store.bits==std::stoul(fields[3],nullptr,16),
            context+" original store order/site/offset/width/value");
      }
      stores+=update.store_count;neutral+=update.store_count==0;++rows;
    }
    check(rows==expected_count && rows>0,"complete independently executed suite");
    unavailable_contract();
    std::cout<<rows<<" original classifier/wrapper packets and "<<stores<<" ordered stores matched; "<<neutral<<" no-write paths preserved\n";
  }catch(const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
