#include "halo2/frontend_matchmaking_presentation.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> bytes(const std::string &s) {
  rust::Vec<std::uint8_t> out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;
}
std::string string(rust::Slice<const std::uint8_t> s) {return {reinterpret_cast<const char *>(s.data()),s.size()};}
bridge::PresentationFacts facts(const FrontendTextFacts &f) {
  bridge::PresentationFacts out{};out.screen=f.screen;out.selected_row=f.selected_row;out.selected_id=bytes(f.selected_id);
  for(const auto &p:f.playlists) {
    bridge::PresentationPlaylist q{};q.id=bytes(p.id);q.name=bytes(p.name);q.description=bytes(p.description);
    q.eligible=p.eligible;q.ranked=p.ranked;q.clan=p.clan;q.minimum=p.minimum;q.maximum=p.maximum;
    q.level_present=p.level.has_value();q.level=p.level.value_or(0);out.playlists.push_back(std::move(q));
  }
  out.status=f.status;out.ui=f.ui;out.pane=f.pane;out.title=f.title;out.countdown=f.countdown;out.pc_progress=f.pc_progress;
  out.matched_present=f.matched.has_value();out.matched=f.matched.value_or(0);
  out.gathering_present=f.gathering.has_value();out.gathering=f.gathering.value_or(0);out.minimum=f.minimum;out.queued=f.queued;
  out.variant=bytes(f.variant);out.map=bytes(f.map);out.message=bytes(f.message);out.party=f.party;out.leader=f.leader;
  out.party_message=bytes(f.party_message);return out;
}
struct Io final:bridge::NativeFrontendTextIo {
  const FrontendTextIo &native;mutable std::exception_ptr error;
  explicit Io(const FrontendTextIo &io):native(io){}
  template<class F> auto capture(F &&f) const -> std::invoke_result_t<F> {
    try{return f();}catch(...){error=std::current_exception();throw std::runtime_error("Native matchmaking text IO failed");}
  }
  rust::Vec<std::uint8_t> text_original_text(std::uint32_t s,std::uint32_t p,std::uint32_t i)const override{return capture([&]{return bytes(native.original_text(s,p,i));});}
  rust::Vec<std::uint8_t> text_original_string(std::uint32_t s,std::uint32_t id)const override{return capture([&]{return bytes(native.original_string(s,id));});}
  rust::Vec<std::uint8_t> text_pc_string(std::uint32_t id)const override{return capture([&]{return bytes(native.pc_string(id));});}
  rust::Vec<std::uint8_t> text_pc_authored(std::uint32_t p)const override{return capture([&]{return bytes(native.pc_authored(p));});}
  rust::Vec<std::uint8_t> text_pc_match_line()const override{return capture([&]{return bytes(native.pc_match_line());});}
  rust::Vec<std::uint8_t> text_map_name()const override{return capture([&]{return bytes(native.map_name());});}
  rust::Vec<std::uint8_t> text_quick_option(std::uint32_t i)const override{return capture([&]{return bytes(native.quick_option(i));});}
  std::uint64_t text_party_count()const override{return capture([&]{return native.party_count();});}
  rust::Vec<std::uint8_t> text_expand(rust::Slice<const std::uint8_t> s)const override{return capture([&]{return bytes(native.expand(string(s)));});}
};
} // namespace
std::string frontend_matchmaking_text_rust(const FrontendTextFacts &f,bool original,unsigned pane,unsigned index,const FrontendTextIo &native) {
  Io io(native);
  try {
    auto result=bridge::frontend_presentation_text(facts(f),original,pane,index,io);
    return {reinterpret_cast<const char *>(result.data()),result.size()};
  }catch(...){if(io.error)std::rethrow_exception(io.error);throw;}
}
} // namespace halo2
