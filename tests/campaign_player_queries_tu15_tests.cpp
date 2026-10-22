#include "halo2/source_campaign_player_queries_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_script_lists.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F fn) {
  try {fn();}catch(const std::exception &){return;}
  throw std::runtime_error("Missing/invalid campaign owner was accepted");
}
void put(SourceRuntimeMemory &memory,std::uint32_t p,std::uint32_t value,unsigned width=4) {
  std::array<std::byte,4> bytes{};
  for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8*i));
  memory.write(p,std::span(bytes).first(width));
}
void mount(SourceRuntimeMemory &memory,std::uint32_t p,unsigned size) {
  memory.mount(p,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,"campaign query component input");
}
std::string hex(std::span<const std::uint8_t> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string result;
  for(const auto byte:bytes){result+=digits[byte>>4];result+=digits[byte&15];}
  return result;
}
void players(const nlohmann::json &input) {
  SourceRuntimeMemory memory;
  constexpr unsigned pool=0x2001000,rows=0x2002000,bitmap=0x2005000;
  mount(memory,0x4f54f4,4);mount(memory,pool,0x4c);mount(memory,rows,16*0x21c);mount(memory,bitmap,4);
  put(memory,0x4f54f4,pool);put(memory,pool+0x20,16);put(memory,pool+0x24,0x21c);
  put(memory,pool+0x38,input.at("high_water"));put(memory,pool+0x44,rows);put(memory,pool+0x48,bitmap);
  put(memory,bitmap,input.at("mask"));
  for(unsigned slot=0;slot<16;++slot) {
    put(memory,rows+slot*0x21c,0xec70+slot,2);
    put(memory,rows+slot*0x21c+2,std::array{0u,1u,2u,0xffffu}[slot%4],2);
    put(memory,rows+slot*0x21c+0x2c,input.at("units").at(slot));
  }
  const auto before=memory.checkpoint();SourceHsListBank bank;unsigned allocations{};
  const auto list=source_campaign_script_players_tu15(memory,{
      [&]{++allocations;return bank.allocate();},[&](auto id,auto unit){bank.append(id,unit);}});
  check(allocations==1 && list==input.at("result"),"players() reused a list or changed its salted identity");
  check(bank.objects(list)==input.at("objects").get<std::vector<std::uint32_t>>(),
      "players() changed original order, dead/NONE handling, duplicates or full unit identity");
  check(hex(bank.headers().elements().first(12))==input.at("header").get<std::string>(),"players() header differs from complete original allocation");
  check(hex(bank.references().elements().first(bank.references().count()*12))==input.at("references").get<std::string>(),
      "players() reference bytes differ from complete original allocations");
  check(memory.checkpoint()==before,"Campaign query modified the source world");
  // A returned list is a snapshot; source unit retirement must not rewrite it.
  const auto saved=bank;const auto objects=bank.objects(list);
  for(unsigned slot=0;slot<16;++slot)put(memory,rows+slot*0x21c+0x2c,0xffffffffu);
  const auto next=source_campaign_script_players_tu15(memory,{
      [&]{bank.roots(std::array{list});return bank.allocate();},[&](auto id,auto unit){bank.append(id,unit);}});
  check(next!=list && bank.count(next)==0 && bank.objects(list)==objects,"players() is a mutable singleton view");
  bank=saved;check(bank.objects(list)==objects,"Checkpoint lost player-list identity or links");
}
}
int main(int argc,char **argv) {try {
  check(argc==2,"Expected independently executed original fixture");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Wrong original campaign authority");
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_dependency_returns").empty(),
      "Original query fixture supplied a dependency outcome");
  for(const auto &input:fixture.at("cooperative")) {
    SourceRuntimeMemory memory;mount(memory,0x4f3218,4);mount(memory,0x2008000,0x135);
    put(memory,0x4f3218,0x2008000);put(memory,0x2008008,input.at("mode"));
    put(memory,0x2008134,input.at("flag"),1);
    check(source_campaign_is_cooperative_tu15(memory)==(input.at("result").get<unsigned>()!=0),
        "Cooperative query changed the original game-type/byte predicate");
  }
  for(const auto &input:fixture.at("players"))players(input);
  // Non-campaign short-circuits the absent cooperative byte, as the original does.
  SourceRuntimeMemory short_bank;mount(short_bank,0x4f3218,4);mount(short_bank,0x2008000,12);
  put(short_bank,0x4f3218,0x2008000);put(short_bank,0x2008008,2);
  check(!source_campaign_is_cooperative_tu15(short_bank),"Non-campaign query read the cooperative byte");
  put(short_bank,0x2008008,1);rejects([&]{source_campaign_is_cooperative_tu15(short_bank);});
  put(short_bank,0x4f3218,0);rejects([&]{source_campaign_is_cooperative_tu15(short_bank);});
  put(short_bank,0x4f3218,0xfffffff8u);rejects([&]{source_campaign_is_cooperative_tu15(short_bank);});
  SourceRuntimeMemory missing;
  rejects([&]{source_campaign_script_players_tu15(missing,{});});
  rejects([&]{source_campaign_script_players_tu15(missing,{[]{return 0u;},[](auto,auto){}});});
  SourceRuntimeMemory invalid;mount(invalid,0x4f54f4,4);mount(invalid,0x2001000,0x4c);
  put(invalid,0x4f54f4,0x2001000);put(invalid,0x2001020,16);put(invalid,0x2001024,0x21c);
  SourceHsObjectListServices lists{[]{return 0u;},[](auto,auto){throw std::runtime_error("Invalid query appended a unit");}};
  put(invalid,0x2001038,17);rejects([&]{source_campaign_script_players_tu15(invalid,lists);});
  put(invalid,0x2001038,0);put(invalid,0x2001024,0x218);rejects([&]{source_campaign_script_players_tu15(invalid,lists);});
  put(invalid,0x2001024,0x21c);put(invalid,0x2001020,0);rejects([&]{source_campaign_script_players_tu15(invalid,lists);});
  std::cout<<"PASS "<<fixture.at("cooperative").size()<<" original cooperative queries, "
      <<fixture.at("players").size()<<" complete original player lists, snapshot/restore and required-owner controls\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
