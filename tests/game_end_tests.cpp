#include "halo2/source_game_end.hpp"
#include "halo2/full_scoreboard.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
SourceGameEndStats bank(const std::string& hex) {
  check(hex.size()==0x254*2,"Original stat-bank width changed");
  std::array<unsigned char,0x254> raw{};
  for(unsigned i=0;i<raw.size();++i)raw[i]=std::stoul(hex.substr(i*2,2),nullptr,16);
  SourceGameEndStats out;std::memcpy(&out,raw.data(),raw.size());return out;
}
void compare(const SourceGameEndStats& actual,const std::string& expected) {
  const auto original=bank(expected);
  check(std::memcmp(&actual,&original,sizeof(actual))==0,"Full original stat bank differs");
}
}
int main(int argc,char**argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original game-end fixture");
    Json data;std::ifstream(argv[1])>>data;
    check(std::endian::native==std::endian::little,"Raw fixture requires little-endian host");
    check(data.at("source_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    check(data.at("instruction_patches").empty() && data.at("supplied_game_or_platform_returns").empty(),"Supplied original results");
    for(const auto& row:data.at("endings")) {
      auto stats=bank(row.at("before_bank"));std::vector<SourceGameEndParticipant> players;
      for(const auto& p:row.at("players"))players.push_back({p.at("datum"),std::int8_t(p.at("membership").get<int>()),
          std::uint16_t(p.at("flags").get<unsigned>())});
      source_game_end_award(stats,players,row.at("team"),std::stoul(row.at("winner").get<std::string>(),nullptr,16));
      compare(stats,row.at("after_award_bank"));
      // Original15EFA0 lifecycle2 clears only the header's low byte.
      stats.header&=0xffffff00u;source_game_end_accumulate(stats,true);
      compare(stats,row.at("after_final_bank"));
      check(row.at("end_record").at("game_over")==1 && row.at("end_record").at("countdown")==210 &&
          row.at("final_record").at("finalized")==1 && row.at("final_record").at("lifecycle")==2,
          "Original game-over/finalization records changed");
    }
    for(const auto& row:data.at("carries")) {
      auto stats=bank(row.at("before_bank"));source_game_end_accumulate(stats,row.at("game_over"));
      compare(stats,row.at("after_bank"));
    }
    for(const auto& row:data.at("rounds"))if(!row.contains("mode")) {
      const auto winner=std::stoul(row.at("winner").get<std::string>(),nullptr,16);
      check(source_round_finishes(row.at("kind"),std::int16_t(row.at("index").get<int>()),winner,
          std::int16_t(row.at("winner_wins_after").get<int>()),row.at("force"))==bool(row.at("record").at("game_over").get<int>()),
          "Original round-completion selector differs");
    }
    FullScoreboardConstants constants;constants.counter_seconds=std::bit_cast<std::uint32_t>(.25f);
    constants.scroll_axis_scale=std::bit_cast<std::uint32_t>(0.f);constants.scroll_pixels=std::bit_cast<std::uint32_t>(0.f);
    for(const auto& group:data.at("automatic_scoreboard")) {
      FullScoreboardHeldState state;
      for(const auto& row:group.at("sequence")) {
        FullScoreboardHeldInput input;input.local_view=row.at("view");input.input_user=input.local_view;
        input.processed_record_present=true;input.engine_present=true;input.lifecycle=2;input.engine_status=2;
        input.current_unit=0xe2700000u+input.local_view;input.player_flags=1;
        check(full_scoreboard_update_held(state,input,{30,1.f/30,row.at("tick")},
            FullScoreboardRounding::NearestEven,constants),"Automatic display input rejected");
        check(state.inactive_counter==row.at("inactive"),"Shared original scoreboard delay differs");
        for(unsigned i=0;i<4;++i)check(state.counters[i]==row.at("counters").at(i),"Automatic view fade differs");
      }
    }
    check(data.at("endings").size()==70 && data.at("carries").size()==128 && data.at("rounds").size()==902,
        "Original end/carry/round fixture coverage truncated");
    std::cout<<checks<<" comparisons: full original win/final-score banks, round selectors and400 automatic scoreboard calls passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
