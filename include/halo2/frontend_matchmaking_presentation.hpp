#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>
namespace halo2 {
struct FrontendTextPlaylist {
  std::string id,name,description;
  bool eligible{},ranked{},clan{};
  unsigned minimum{},maximum{};
  std::optional<unsigned> level;
};
struct FrontendTextFacts {
  unsigned screen{},selected_row{};
  std::string selected_id;
  std::vector<FrontendTextPlaylist> playlists;
  unsigned status{};
  int ui{},title{},countdown{-1};
  unsigned pane{};
  bool pc_progress{};
  std::optional<std::uint64_t> matched,gathering;
  unsigned minimum{},queued{};
  std::string variant,map,message;
  bool party{},leader{};
  std::string party_message;
};
// Genuine native locale/asset/profile reads. Rust selects and combines them.
// pc_match_line is the original native localized rendering expression; it
// retains its compiler's operand/read order without selecting a UI state.
struct FrontendTextIo {
  std::function<std::string(unsigned,unsigned,unsigned)> original_text;
  std::function<std::string(unsigned,std::uint32_t)> original_string;
  std::function<std::string(std::uint32_t)> pc_string;
  std::function<std::string(unsigned)> pc_authored;
  std::function<std::string()> pc_match_line,map_name;
  std::function<std::string(unsigned)> quick_option;
  std::function<std::uint64_t()> party_count;
  std::function<std::string(const std::string &)> expand;
};
std::string frontend_matchmaking_text_rust(const FrontendTextFacts &,bool original,
    unsigned pane,unsigned index,const FrontendTextIo &);
} // namespace halo2
