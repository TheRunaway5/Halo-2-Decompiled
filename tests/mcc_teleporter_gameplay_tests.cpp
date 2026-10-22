#include "halo2/mcc_map_gameplay_vanilla.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &text) {
  std::vector<std::byte> result;
  for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return result;
}
struct Fixture {
  static constexpr std::uint32_t root=0x1000000u,flags=root+0x200u;
  std::string path;std::vector<std::byte> memory,target;
  TagHandle handle{0xe0010000u};bool missing{},foreign{};
  explicit Fixture(const nlohmann::json &row):path(row.at("path").get<std::string>()),target(unhex(row.at("target_flags_hex"))) {
    const auto bank=unhex(row.at("donor_flags_hex"));memory.resize(0x200u+bank.size());
    std::copy(bank.begin(),bank.end(),memory.begin()+0x200u);put(root+0x118u,row.at("count"));put(root+0x11cu,flags);
  }
  void put(std::uint32_t address,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)memory.at(address-root+n)=std::byte(value>>(8*n));
  }
  std::span<const std::byte> read(std::uint32_t address,std::size_t bytes) {
    check(address>=root && std::uint64_t(address-root)+bytes<=memory.size(),"Fixture read escapes its real owner");
    return std::span<const std::byte>(memory).subspan(address-root,bytes);
  }
  MccMapGameplaySource source() {
    const auto identity=[this]{return MccMapGameplayIdentity{foreign?TagHandle{handle.value^0x10000u}:handle,
        tag_groups::scenario,path,root,std::uint32_t(memory.size())};};
    return {path,handle,{},[this,identity](TagHandle id)->std::optional<MccMapGameplayIdentity>{
          if(missing || id!=handle)return {};return identity();},
        [this,identity](TagGroup group,std::string_view name)->std::optional<MccMapGameplayIdentity>{
          if(missing || group!=tag_groups::scenario || name!=path)return {};return identity();},
        [this](auto address,auto bytes){return read(address,bytes);}};
  }
};
}
int main(int argc,char **argv)try {
  if(argc!=2)return 2;std::ifstream stream(argv[1]);if(!stream)return 77;
  nlohmann::json fixture;stream>>fixture;unsigned words{};
  for(const auto &row:fixture.at("profiles")) {
    Fixture f(row);const auto before=f.memory;
    const auto plan=plan_mcc_teleporter_gameplay_vanilla(f.source());
    check(plan.status==MccMapGameplayStatus::Ready && plan.writes.size()==row.at("patches").size(),
        "Known original-backed teleporter tuple was not planned");
    check(f.memory==before,"Planning changed cache bytes");
    preflight_mcc_map_gameplay_vanilla(plan,f.source().read);
    for(const auto &write:plan.writes) {
      check(write.before.size()==4 && write.after.size()==4,"Teleporter correction escaped its authored word");
      std::copy(write.after.begin(),write.after.end(),f.memory.begin()+write.source_token-Fixture::root);++words;
    }
    check(std::ranges::equal(f.read(Fixture::flags,f.target.size()),f.target),"Corrected flag bank differs from original-backed target");
    const auto again=plan_mcc_teleporter_gameplay_vanilla(f.source());
    check(again.status==MccMapGameplayStatus::AlreadyEquivalent && again.writes.empty(),"Teleporter conversion is not idempotent");
    Fixture modified(row);modified.memory.back()^=std::byte{1};const auto changed=modified.memory;
    const auto unknown=plan_mcc_teleporter_gameplay_vanilla(modified.source());
    check(unknown.status==MccMapGameplayStatus::AuthoredTupleUnrecognized && unknown.writes.empty() &&
        unknown.guards.empty() && modified.memory==changed,"Custom flag authoring was not preserved");
    Fixture stale(row);const auto queued=plan_mcc_teleporter_gameplay_vanilla(stale.source());
    stale.memory.back()^=std::byte{1};bool rejected{};
    try{preflight_mcc_map_gameplay_vanilla(queued,stale.source().read);}catch(const std::exception&){rejected=true;}
    check(rejected,"Stale teleporter admission was accepted");
    Fixture escaped(row);escaped.put(Fixture::root+0x11cu,Fixture::root+std::uint32_t(escaped.memory.size()));
    check(plan_mcc_teleporter_gameplay_vanilla(escaped.source()).status==MccMapGameplayStatus::InvalidMetadata,
        "Foreign teleporter bank extent was accepted");
    Fixture wrong(row);wrong.foreign=true;
    check(plan_mcc_teleporter_gameplay_vanilla(wrong.source()).status==MccMapGameplayStatus::RequiredDependencyUnavailable,
        "Foreign scenario salt was accepted");
  }
  check(words==4,"Original-backed teleporter correction coverage changed");
  std::cout<<"PASS original TU1.5-loaded teleporter tuples: four words, complete-bank guards, idempotency, custom preservation and atomic stale/foreign rejection\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
