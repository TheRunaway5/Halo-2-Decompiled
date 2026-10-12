#include "halo2/vista_pcr_publication.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool pass,const std::string& message) {
  if(!pass)throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
  bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}
  check(rejected,"Unqualified native PCR input admitted");
}
std::vector<std::byte> unhex(const std::string& s) {
  check(s.size()%2==0,"Invalid original hexadecimal buffer");
  std::vector<std::byte> out;
  for(std::size_t i=0;i<s.size();i+=2)
    out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return out;
}
template<class B> void pattern(B& b,unsigned seed,unsigned multiplier=37) {
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte((i*multiplier+seed*13)&255u);
}
void compare_variant(const SourceGameVariant& v,const std::string& raw) {
  const auto expected=unhex(raw);const auto pc=vista_pcr_slayer_variant(v);
  check(std::equal(expected.begin(),expected.end(),pc.bytes.begin(),pc.bytes.end()),
        "Complete304 native Slayer bytes differ from original PC");
}
std::vector<std::byte> consumed(std::span<const std::byte> b) {
  std::vector<std::byte> out;
  const auto append=[&](unsigned at,unsigned size){out.insert(out.end(),b.begin()+at,b.begin()+at+size);};
  append(0x10,2);append(0x20,0x130);
  for(unsigned i=0;i<16;++i) {
    const auto at=0x370+i*0x94;
    append(at,1);append(at+2,8);append(at+0xc,0x84);append(at+0x90,1);append(at+0x92,2);
  }
  for(unsigned i=0;i<16;++i)append(0xcb0+i*0x18,4);
  append(0xe30,0x4040);return out;
}
constexpr std::array<std::int32_t,12> values{
  -2147483647-1,-65537,-32769,-1,0,1,2,255,256,32768,65535,2147483647};
SourceResultStatistics statistics(unsigned seed) {
  SourceResultStatistics out;std::size_t byte=0;
  const auto next=[&]() {
    const auto lo=(byte++*19+seed*13)&255u;
    const auto hi=(byte++*19+seed*13)&255u;
    return std::uint16_t(lo|(hi<<8));
  };
  for(auto& p:out.players)for(auto& v:p)v=next();
  for(auto& p:out.pvp)for(auto& opponent:p)for(auto& v:opponent)v=next();
  for(auto& t:out.teams)for(auto& v:t)v=next();
  return out;
}
}
int main(int argc,char** argv) {
  try {
    check(argc==2,"Original PC publication fixture required");
    std::ifstream file(argv[1]);check(bool(file),"Missing PC publication fixture");
    unsigned defaults=0,descriptors=0,variants=0,writes=0,clears=0,ranks=0,snapshots=0,flags=0,scores=0;
    for(std::string line;std::getline(file,line);) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream input(line);char kind;input>>kind;
      if(kind=='D') {
        std::string raw;input>>raw;compare_variant(source_game_variant_default(0),raw);++defaults;
      } else if(kind=='Q') {
        unsigned p,expected_kind,offset,operand;input>>p>>expected_kind>>offset>>operand;
        const auto& d=source_game_variant_descriptors()[p];
        check(std::uint32_t(d.kind)==expected_kind && d.offset==offset && d.operand==operand,
              "Native variant descriptor differs from original PC");++descriptors;
      } else if(kind=='V') {
        int p,value,expected;std::string raw;input>>p>>value>>raw>>expected;
        auto v=source_game_variant_default(0);source_game_variant_set(v,p,value);
        compare_variant(v,raw);check(source_game_variant_get(v,p)==expected,"PC variant getter differs");++variants;
      } else if(kind=='P' || kind=='T') {
        int slot,enabled,final,occupied,score,placement;unsigned seed;std::string raw;
        input>>slot>>enabled>>final>>occupied>>score>>placement>>seed>>raw;
        std::array<std::byte,vista_pcr_source_size> bank;pattern(bank,seed);
        bank[0]=std::byte(enabled);bank[0x11]=std::byte(final);
        const auto at=kind=='P'?0x370+unsigned(std::max(slot,0))*0x94:0xcb0+unsigned(std::max(slot,0))*0x18;
        bank[at]=std::byte(occupied);auto expected=bank;const auto fields=unhex(raw);
        if(kind=='P') {
          expected[at+0x90]=fields[0];expected[at+0x92]=fields[1];expected[at+0x93]=fields[2];
          vista_pcr_write_player_result(bank,slot,score,placement);
        } else {
          std::copy(fields.begin(),fields.end(),expected.begin()+at+1);
          vista_pcr_write_team_result(bank,slot,score,placement);
        }
        check(bank==expected,"Original guarded PCR score/placement mutation differs");++writes;
      } else if(kind=='C') {
        unsigned seed;std::uint32_t clock;input>>seed>>clock;
        std::array<std::byte,vista_pcr_source_size> bank;pattern(bank,seed);auto expected=bank;
        std::fill(expected.begin()+0x10,expected.end(),std::byte{});
        std::fill(expected.begin()+4,expected.begin()+8,std::byte{});
        for(unsigned i=0;i<4;++i)expected[8+i]=std::byte(clock>>(8*i));
        vista_pcr_clear_publication(bank,clock);check(bank==expected,"Original PC clear boundary differs");++clears;
      } else if(kind=='R') {
        std::uint32_t datum,flag,round_flags;int expected;input>>datum>>flag>>round_flags>>expected;(void)flag;
        const std::array participants{datum};
        check(vista_pcr_single_player_placement(participants,datum,round_flags)==expected,"Original sole FFA placement differs");++ranks;
      } else if(kind=='F') {
        std::uint32_t value;int expected;input>>value>>expected;
        check(vista_pcr_ranking_available(value)==bool(expected),"Original rank flag differs");++flags;
      } else if(kind=='S') {
        std::uint32_t datum;int expected;input>>datum>>expected;
        SourceGameEndStats stats{};stats.players[datum&0xffffu].fields[1]=std::int16_t(expected);
        check(vista_pcr_accumulated_player_score(stats,datum)==expected,"Original accumulated PC score differs");++scores;
      } else if(kind=='B') {
        unsigned seed,enabled,final;std::string raw;input>>seed>>enabled>>final>>raw;
        auto numeric=statistics(seed);VistaPcrPublicationInput q;
        q.variant=vista_pcr_slayer_variant(source_game_variant_default(0));q.statistics=&numeric;
        q.recording={std::uint8_t(enabled),128};q.engine_present=true;q.current=1;q.finalized=std::uint8_t(final);
        for(unsigned i=0;i<16;++i) {
          auto& player=q.players[i];player.occupied=(i+seed)%3?0:(i%2?128:1);
          pattern(player.identity,seed+i,29);pattern(player.configuration,seed+i,17);
          player.score=values[(i+seed)%values.size()];player.placement=values[(i+seed+3)%values.size()];
        }
        const auto actual=vista_pcr_publish_consumed_snapshot(q);
        check(consumed(actual.source)==unhex(raw),"Original PC consumed snapshot differs");++snapshots;
        q.engine_present=false;
        const auto absent=vista_pcr_publish_consumed_snapshot(q);
        check(std::all_of(absent.source.begin()+0x10,absent.source.end(),[](auto b){return b==std::byte{};}),
              "Absent actual engine admitted PCR metadata");
      } else throw std::runtime_error("Unknown original PC publication fixture row");
      check(!input.fail(),"Malformed original PC publication fixture row");
    }
    check(defaults==1 && descriptors==112 && variants==276 && writes==2268 && clears==9 && ranks==6 && snapshots==18 && flags==7 && scores==80,
          "Incomplete original PC publication fixture");
    auto valid=source_game_variant_default(0,u"Actual source title");
    (void)vista_pcr_slayer_variant(valid);
    valid.bytes[0x100]^=1;rejects([&]{(void)vista_pcr_slayer_variant(valid);});
    rejects([&]{(void)vista_pcr_slayer_variant(source_game_variant_default(1));});
    const std::array<std::uint32_t,2> two{0x12340000,0x12340001};
    rejects([&]{(void)vista_pcr_single_player_placement(two,two[0],0x20);});
    rejects([&]{(void)vista_pcr_single_player_placement({},two[0],0x20);});
    rejects([&]{(void)vista_pcr_single_player_placement(std::span(two).first(1),two[1],0x20);});
    rejects([&]{(void)vista_pcr_single_player_placement(std::span(two).first(1),two[0],0);});
    const std::array none{0xffffffffu};rejects([&]{(void)vista_pcr_single_player_placement(none,none[0],0x20);});
    VistaPcrPublicationInput missing;missing.engine_present=true;
    rejects([&]{(void)vista_pcr_publish_consumed_snapshot(missing);});
    (void)vista_pcr_publish_consumed_snapshot({}); // Absent engine consumes no other owner.
    check(!VistaPcrConsumedSnapshot::complete_retained_metadata,"Unconsumed metadata claim lost");
    std::cout<<"Matched PC Slayer304/default,112 descriptors,276 writes/reads; "<<writes
             <<" guarded results,"<<clears<<" clears,"<<ranks<<" sole FFA ranks,"<<snapshots<<" consumed snapshots\n";
    return 0;
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
