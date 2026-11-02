#pragma once
// Native observations and Rust decisions. No native matchmaking policy owner.
#include "halo2/frontend_matchmaking_progress.hpp"
#include <string>
namespace halo2 {
struct FrontendProgressMember {
  MatchmakingProgressPlayer key;
  std::optional<unsigned> level;
  bool self{};
  int local_controller{-1};
  std::string handle;
};
struct FrontendProgressFacts {
  unsigned pane{};
  int ui{};
  unsigned status{};
  bool ranked{},teams{},party_present{},match_present{},gathering_present{};
  std::vector<FrontendProgressMember> party,local,matched,gathering;
  unsigned maximum{};
  std::optional<unsigned> viewer_level;
  bool party_voice{},voice_joined{},microphone_muted{},transmitting{};
  std::vector<std::string> talking;
};
enum class FrontendProgressSource {none,party,local,match,gathering};
struct FrontendProgressRow {
  MatchmakingProgressRow kind{MatchmakingProgressRow::empty};
  FrontendProgressSource source{};
  unsigned index{};
  std::optional<unsigned> rank,speaker;
};
std::vector<FrontendProgressRow> frontend_matchmaking_project(const FrontendProgressFacts &);
std::optional<FrontendProgressRow> frontend_matchmaking_query(
    const FrontendProgressFacts &,int screen,const std::vector<int> &entries,unsigned row);
unsigned matchmaking_progress_leave_dialog(std::size_t party_players,std::size_t local_players);
std::vector<unsigned> matchmaking_progress_pane_order(
    const std::vector<MatchmakingProgressPlayer> &,unsigned pane,bool ranked,bool teams);
} // namespace halo2
