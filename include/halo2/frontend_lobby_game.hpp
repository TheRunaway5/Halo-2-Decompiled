#pragma once
#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
namespace halo2 {
// Rust owns selection and presentation. Native code supplies immutable cache
// observations and the shared gameplay source-buffer/descriptor getter.
std::optional<int> frontend_lobby_variant_type(unsigned engine);
int frontend_lobby_game_icon(int type);
std::optional<unsigned> frontend_lobby_quick_category(int type);
std::uint32_t frontend_lobby_difficulty_string(std::int16_t difficulty);
std::optional<int> frontend_lobby_parameter_default(unsigned engine,unsigned parameter);
class FrontendLobbyViewIo {
public:
  virtual ~FrontendLobbyViewIo()=default;
  virtual std::optional<std::vector<std::uint32_t>> category(unsigned)const=0;
  virtual std::optional<UiParameter> parameter(std::uint32_t)const=0;
  virtual std::optional<int> value(unsigned)const=0;
  virtual std::optional<int> default_value(unsigned,unsigned)const=0;
};
std::vector<std::optional<std::string>> frontend_variant_quick_options_rust(
    unsigned,std::string_view,std::size_t,const FrontendLobbyViewIo&);
std::optional<int> frontend_lobby_parameter_default_rust(unsigned,unsigned,const FrontendLobbyViewIo&);
std::vector<std::optional<std::string>> frontend_variant_quick_options_rows(
    const FrontendContent&,unsigned,const std::map<unsigned,int>&,std::string_view,std::size_t);
template<std::size_t Count>
inline std::array<std::optional<std::string>,Count> frontend_variant_quick_options(
    const FrontendContent &content,unsigned engine,const std::map<unsigned,int>& values,std::string_view separator) {
  const auto rows=frontend_variant_quick_options_rows(content,engine,values,separator,Count);
  std::array<std::optional<std::string>,Count> out;
  std::copy(rows.begin(),rows.end(),out.begin());return out;
}
inline std::array<std::optional<std::string>,5> frontend_lobby_quick_options(
    const FrontendContent &content,unsigned engine,const std::map<unsigned,int>& values,std::string_view separator) {
  return frontend_variant_quick_options<5>(content,engine,values,separator);
}
inline std::array<std::optional<std::string>,6> frontend_matchmaking_quick_options(
    const FrontendContent &content,unsigned engine,const std::map<unsigned,int>& values,std::string_view separator) {
  return frontend_variant_quick_options<6>(content,engine,values,separator);
}
struct FrontendLobbyHelp {
  std::optional<unsigned> text_index;
  bool visible{};
  std::optional<unsigned> value_index;
  std::uint32_t string_id{};
};
FrontendLobbyHelp frontend_lobby_help(int pane,int protocol,int machines,int privacy,int session);
int frontend_lobby_player_count(int previous,int count_a,int count_b);
std::optional<std::uint32_t> frontend_lobby_player_count_string(int count);
}
