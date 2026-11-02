#include "halo2/frontend_lobby_game.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
template<class T,class U> std::optional<T> optional(const U &value) {
  return value.present ? std::optional<T>{value.value} : std::nullopt;
}
rust::Vec<std::uint8_t> bytes(std::string_view value) {
  rust::Vec<std::uint8_t> out;out.reserve(value.size());for(unsigned char c:value)out.push_back(c);return out;
}
class NativeLobbyViewIo final:public FrontendLobbyViewIo {
public:
  NativeLobbyViewIo()=default;
  NativeLobbyViewIo(const FrontendContent &content,const std::map<unsigned,int>& values):content_(&content),values_(&values){}
  std::optional<std::vector<std::uint32_t>> category(unsigned id)const override {
    const auto found=content_->categories.find(id);
    return found==content_->categories.end()?std::nullopt:std::optional{found->second};
  }
  std::optional<UiParameter> parameter(std::uint32_t tag)const override {
    const auto found=content_->parameters.find(tag);
    return found==content_->parameters.end()?std::nullopt:std::optional{found->second};
  }
  std::optional<int> value(unsigned parameter)const override {
    const auto found=values_->find(parameter);
    return found==values_->end()?std::nullopt:std::optional{found->second};
  }
  std::optional<int> default_value(unsigned engine,unsigned parameter)const override {
    return source_game_variant_get(source_game_variant_default(engine),int(parameter));
  }
private:
  const FrontendContent *content_{};
  const std::map<unsigned,int> *values_{};
};
class Io final:public bridge::NativeFrontendLobbyViewIo {
public:
  const FrontendLobbyViewIo &native;mutable std::exception_ptr error;
  explicit Io(const FrontendLobbyViewIo &io):native(io){}
  template<class F> auto capture(F &&f)const->std::invoke_result_t<F> {
    try{return f();}catch(...){error=std::current_exception();throw std::runtime_error("Native lobby view IO failed");}
  }
  bridge::LobbyViewCategory lobby_view_category(std::uint32_t id)const override {
    return capture([&]{bridge::LobbyViewCategory out{};const auto rows=native.category(id);
      out.present=rows.has_value();if(rows)for(auto value:*rows)out.rows.push_back(value);return out;});
  }
  bridge::LobbyViewParameter lobby_view_parameter(std::uint32_t tag)const override {
    return capture([&]{bridge::LobbyViewParameter out{};const auto p=native.parameter(tag);
      out.present=p.has_value();if(p){out.parameter=p->parameter;out.title=bytes(p->title);
        for(const auto &choice:p->choices)out.choices.push_back({bytes(choice.label),choice.value});}return out;});
  }
  bridge::LobbyViewI32 lobby_view_value(std::uint32_t parameter)const override {
    return capture([&]{const auto value=native.value(parameter);return bridge::LobbyViewI32{value.has_value(),value.value_or(0)};});
  }
  bridge::LobbyViewI32 lobby_view_default(std::uint32_t engine,std::uint32_t parameter)const override {
    return capture([&]{const auto value=native.default_value(engine,parameter);return bridge::LobbyViewI32{value.has_value(),value.value_or(0)};});
  }
};
}
std::optional<int> frontend_lobby_variant_type(unsigned engine){return optional<int>(bridge::lobby_view_variant_type(engine));}
int frontend_lobby_game_icon(int type){return bridge::lobby_view_game_icon(type);}
std::optional<unsigned> frontend_lobby_quick_category(int type){return optional<unsigned>(bridge::lobby_view_quick_category(type));}
std::uint32_t frontend_lobby_difficulty_string(std::int16_t difficulty){return bridge::lobby_view_difficulty(difficulty);}
std::optional<int> frontend_lobby_parameter_default_rust(unsigned engine,unsigned parameter,const FrontendLobbyViewIo &native){
  Io io(native);try{return optional<int>(bridge::lobby_view_default_value(engine,parameter,io));}
  catch(...){if(io.error)std::rethrow_exception(io.error);throw;}
}
std::optional<int> frontend_lobby_parameter_default(unsigned engine,unsigned parameter){return frontend_lobby_parameter_default_rust(engine,parameter,NativeLobbyViewIo{});}
std::vector<std::optional<std::string>> frontend_variant_quick_options_rust(
    unsigned engine,std::string_view separator,std::size_t count,const FrontendLobbyViewIo &native){
  Io io(native);
  try {
    const auto raw=bridge::lobby_view_quick_options(engine,{reinterpret_cast<const std::uint8_t*>(separator.data()),separator.size()},count,io);
    std::vector<std::optional<std::string>> out;out.reserve(raw.size());
    for(const auto &row:raw) {
      if(row.present)out.emplace_back(std::string{reinterpret_cast<const char*>(row.value.data()),row.value.size()});
      else out.emplace_back(std::nullopt);
    }
    return out;
  }catch(...){if(io.error)std::rethrow_exception(io.error);throw;}
}
std::vector<std::optional<std::string>> frontend_variant_quick_options_rows(
    const FrontendContent&content,unsigned engine,const std::map<unsigned,int>&values,std::string_view separator,std::size_t count){
  return frontend_variant_quick_options_rust(engine,separator,count,NativeLobbyViewIo{content,values});
}
FrontendLobbyHelp frontend_lobby_help(int pane,int protocol,int machines,int privacy,int session){
  const auto p=bridge::lobby_view_help(pane,protocol,machines,privacy,session);
  return {optional<unsigned>(p.text),p.visible,optional<unsigned>(p.value),p.string_id};
}
int frontend_lobby_player_count(int previous,int count_a,int count_b){return bridge::lobby_view_player_count(previous,count_a,count_b);}
std::optional<std::uint32_t> frontend_lobby_player_count_string(int count){return optional<std::uint32_t>(bridge::lobby_view_player_count_string(count));}
}
