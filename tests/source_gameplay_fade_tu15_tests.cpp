#include "halo2/source_gameplay_fade_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <bit>
using namespace halo2;
namespace {
using J=nlohmann::json;
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
SourceGameplayFadeStateTu15 state(const J &s){return {s.at("opacity"),s.at("mask"),s.at("frames"),s.at("counter")};}
J record(const SourceGameplayFadeStateTu15 &s){return {{"opacity",s.opacity_bits},{"mask",s.activation_mask},{"frames",s.rendered_frames},{"counter",s.phase_counter}};}
}
int main(int argc,char **argv)try {
  need(argc==2,"Supply unchanged TU1.5 gameplay fade fixture");std::ifstream file(argv[1]);need(bool(file),"Original fade fixture missing");
  const auto fixture=J::parse(file);need(fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original TU1.5 fade pin changed");
  unsigned ticks{},renders{},index{};
  for(const auto &c:fixture.at("cases")){++index;auto s=state(c.at("before"));const auto &q=c.at("input");
    const SourcePhysicsFactoryFloatingPoint fp{q.value("cw",std::uint16_t(0x27f)),q.value("mxcsr",0x1f80u)};
    if(c.at("kind")=="tick"){
      SourceGameplayFadeTickTu15 in;in.engine_present=q.value("engine",true);in.raw_mode=q.value("mode",1);
      in.phase=q.value("phase",1);in.pending_phase=q.value("pending",1);in.rate=q.value("rate",30);
      in.tick=q.value("tick",100u);in.round_start_tick=q.value("start",0u);in.life_limit=q.value("limit",0);
      for(auto it=q.at("users").begin();it!=q.at("users").end();++it){const auto view=std::stoul(it.key());const auto &p=it.value();
        in.users.at(view)={p.value("present",true),p.value("unit",true)?0x20000u+unsigned(view):0xffffffffu,
          p.value("flags",std::uint16_t(1)),p.value("team",std::int8_t(0)),p.value("lives",std::int16_t(1)),p.value("respawn",0)};
      }
      source_gameplay_fade_tick_tu15(s,in,fp);++ticks;
    }else{
      SourceGameplayFadeRenderTu15 in;in.engine_present=q.value("engine",true);in.actual_view=q.value("user",0u);
      in.unresolved_world_geometry=q.value("pending_geometry",0);in.rate=q.value("rate",30);
      const auto color=source_gameplay_fade_render_tu15(s,in,fp);
      need(bool(color)==c.at("overlay").get<bool>(),"Original TU1.5 black-quad admission mismatch");
      if(color)need(std::bit_cast<std::uint32_t>(*color)==(in.actual_view==0xffffffffu?0x3f800000u:s.opacity_bits[in.actual_view]),
          "Original binary32 viewport alpha was quantized or retimed");
      ++renders;
    }
    if(record(s)!=c.at("after")){std::cerr<<"case="<<index<<" actual="<<record(s)<<" expected="<<c.at("after")<<'\n';
      throw std::runtime_error("Original TU1.5 fade state mismatch");}
  }
  need(ticks>=654&&renders>=108,"Original TU1.5 fade control coverage fell");
  std::cout<<"PASS original TU1.5 gameplay fade:"<<ticks<<" native tick controls,"<<renders<<" render-prefix controls; exact binary32 alpha/mask/countdown and source player eligibility\n";
  // The network carries inputs and source lifecycle events, never another
  // machine's opacity/mask. Different actual local player slots must project
  // to independent view0, with complete exact histories across packet gaps.
  SourceGameplayFadeJournalTu15 journal;journal.context_available=journal.engine_present=true;
  journal.rate=30;journal.x87_control_word=0x27f;journal.mxcsr=0x1f80;
  for(unsigned slot=0;slot<16;++slot)journal.current[slot]={0x10000u+slot,1};
  SourceGameplayFadePresentationTu15 host,joined;
  const std::array<std::uint32_t,1> host_ids{0x10000u};
  const std::array<std::uint32_t,2> joined_ids{0x10004u,0x10005u};
  SourceGameplayFadeStateTu15 expected;
  source_gameplay_fade_activate_tu15(expected,0);
  host.consume(journal,host_ids);joined.consume(journal,joined_ids);
  host.render(0,0);joined.render(0,0);joined.render(1,0);
  source_gameplay_fade_render_tu15(expected,{true,0,0,30},{0x27f,0x1f80});
  for(unsigned tick=1;tick<=240;++tick) {
    SourceGameplayFadeFrameTu15 frame;frame.sequence=++journal.newest;frame.tick=tick;frame.engine_present=true;
    frame.raw_mode=1;frame.rate=30;frame.phase=tick<=72?1:2;frame.pending_phase=frame.phase;
    frame.phase_counter_before=expected.phase_counter;frame.x87_control_word=0x27f;frame.mxcsr=0x1f80;
    frame.identities=journal.current;
    for(auto &p:frame.players)p={true,0x20000u,1,0,1,0};
    journal.slots.push_back(frame);
    SourceGameplayFadeTickTu15 input;input.engine_present=true;input.raw_mode=1;input.phase=frame.phase;
    input.pending_phase=frame.pending_phase;input.rate=30;input.tick=tick;input.users[0]=frame.players[0];
    source_gameplay_fade_tick_tu15(expected,input,{0x27f,0x1f80});
    const auto encoded=encode_source_gameplay_fade_journal_tu15(journal);
    const auto decoded=decode_source_gameplay_fade_journal_tu15(encoded);need(decoded==journal,"Fade observation wire changed source inputs");
    host.consume(decoded,host_ids);host.render(0,0);
    source_gameplay_fade_render_tu15(expected,{true,0,0,30},{0x27f,0x1f80});
    need(host.state()==expected,"Host presentation diverged from exact original fade projection");
    if(tick%7==0||tick==240){joined.consume(decoded,joined_ids);joined.render(0,0);joined.render(1,0);
      need(joined.state().opacity_bits[0]==expected.opacity_bits[0]&&joined.state().opacity_bits[1]==expected.opacity_bits[0],
        "Joined local slots4/5 failed exact dropped-snapshot fade catchup");}
  }
  need(host.state().opacity_bits[0]==0x3f800000u,"Original ending did not become fully black");
  const auto unchanged=joined.state();auto older=journal;older.newest=0;older.slots.clear();joined.consume(older,joined_ids);
  need(joined.state()==unchanged,"Older source fade snapshot rewound local presentation");
  const auto rejects=[](auto call){bool failed=false;try{call();}catch(const std::exception&){failed=true;}need(failed,"Invalid fade observation was accepted");};
  auto bad=journal;bad.slots[0].sequence=2;rejects([&]{validate_source_gameplay_fade_journal_tu15(bad);});
  bad=journal;bad.slots[0].identities[4].player=0x10005u;rejects([&]{validate_source_gameplay_fade_journal_tu15(bad);});
  auto bytes=encode_source_gameplay_fade_journal_tu15(journal);bytes.push_back(std::byte{});rejects([&]{decode_source_gameplay_fade_journal_tu15(bytes);});
  auto gap=journal;gap.newest=513;gap.slots.resize(512);
  for(unsigned i=0;i<512;++i){gap.slots[i]=journal.slots.back();gap.slots[i].sequence=i?i+1:513;}
  const auto full=encode_source_gameplay_fade_journal_tu15(gap);
  need(full.size()==source_gameplay_fade_journal_header_wire_tu15+512*source_gameplay_fade_frame_wire_tu15 && full.size()<256u*1024u,
    "Full gameplay fade history exceeds its bounded wire budget");
  need(decode_source_gameplay_fade_journal_tu15(full)==gap,"Wrapped complete fade history lost canonical ring slots");
  SourceGameplayFadePresentationTu15 late;rejects([&]{late.consume(gap,host_ids);});
  need(late.sequence()==0,"Missing fade history fabricated a starting state");
  // A new physical owner can join an older world only when its full salted
  // player did not yet exist in the earliest retained real input frame.
  auto new_player=gap;
  for(auto &f:new_player.slots)if(f.sequence<300){f.identities[0]={};f.players[0]={};}
  SourceGameplayFadePresentationTu15 admitted_late;admitted_late.consume(new_player,host_ids);
  need(admitted_late.sequence()==513&&admitted_late.state().activation_mask==1,
    "Actual newly admitted local player could not consume its source init history");

  std::cout<<"PASS pointer-free fade publication:240 source input frames, distinct host/joined local views, seven-tick packet gaps, canonical bounded wire, stale/reordered/foreign/trailing/history-gap rejection\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
