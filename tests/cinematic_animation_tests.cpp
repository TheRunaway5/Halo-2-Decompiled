#include "halo2/cinematic_animation.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool result,const char *message){if(!result)throw FormatError(message);}
void frame(CinematicAnimationChannel &state,float value) {
  const auto word=std::bit_cast<std::uint32_t>(value);
  for(unsigned i=0;i<4;++i)state.manager[0x1c+i]=std::byte(word>>(i*8));
}
AnimationClip clip() {
  AnimationClip result;result.source_graph=1;result.source_index=0;result.frame_count=516;
  return result;
}
UnitAnimationDefinition definition(){UnitAnimationDefinition result;result.frame_count=516;return result;}
void same(const CinematicAnimationChannel &state,const nlohmann::json &expected) {
  check(std::bit_cast<std::uint32_t>(cinematic_animation_frame(state))==
      std::stoul(expected.at("bits").get<std::string>(),nullptr,16),"Original cinematic channel frame differs");
  check(cinematic_animation_completion(state)==expected.at("completion_flags").get<unsigned>(),
      "Original cinematic completion flags differ");
}
Transform transform(const nlohmann::json &words) {
  Transform result;result.scale=words.at(0);
  for(unsigned i=0;i<3;++i)result.basis[i]={words.at(1+i*3),words.at(2+i*3),words.at(3+i*3)};
  result.position={words.at(10),words.at(11),words.at(12)};return result;
}
}
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply the original cinematic channel fixture");
  std::ifstream input(argv[1]);nlohmann::json original;input>>original;
  check(original.at("xbe_sha256")=="03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d",
      "Cinematic original fixture has the wrong executable identity");
  unsigned checks=0;
  for(const auto &row:original.at("script_short_seeks")) {
    const auto state=start_cinematic_animation(clip(),definition(),false,false,false,
        row.at("requested_short_frame").get<std::int16_t>());
    same(state,row);++checks;
  }
  auto state=start_cinematic_animation(clip(),definition(),false,false,false);frame(state,513);
  for(const auto &row:original.at("nonloop_sequence")) {
    same(state,row);advance_cinematic_animation(state);++checks;
  }
  for(const auto &row:original.at("loop_wrap")) {
    if(row.at("delta").get<float>()!=1.f/30.f)continue;
    auto c=clip();auto d=definition();c.loop_frame=row.at("loop_frame");d.loop_frame=std::int16_t(c.loop_frame);
    state=start_cinematic_animation(c,d,false,true,false);frame(state,row.at("start"));
    advance_cinematic_animation(state);same(state,row);++checks;
  }
  for(const auto &row:original.at("duration_queries")) {
    for(bool unit:{false,true}) {
      state=start_cinematic_animation(clip(),definition(),unit,false,true);frame(state,row.at("frame"));
      check(std::uint16_t(cinematic_animation_time(state))==row.at(unit?"unit_ticks":"scenery_ticks").get<unsigned>(),
          "Original cinematic remaining-time rounding differs");++checks;
    }
  }
  for(const auto &row:original.at("live_anchor_composition")) {
    const auto local=transform(row.at("local"));
    const auto anchor=row.at("condition")=="destroyed_anchor"?std::optional<Transform>{}:
        std::optional(transform(row.at("live_anchor")));
    const auto actual=cinematic_animation_anchor(local,anchor);
    for(const auto key:{"unit_result","scenery_result"}) {
      check(std::bit_cast<std::array<std::uint32_t,13>>(actual)==
          std::bit_cast<std::array<std::uint32_t,13>>(transform(row.at(key))),"Original live-anchor matrix differs");++checks;
    }
  }
  for(const auto &row:original.at("sampler_index_fraction")) {
    state=start_cinematic_animation(clip(),definition(),false,false,false);frame(state,row.at("requested_frame"));
    if(row.at("interpolate").get<bool>())state.manager[0x12]|=std::byte{0x40};
    const float expected=row.at("sample_frame").get<float>()+row.at("fraction").get<float>();
    check(cinematic_animation_sample_frame(state)==expected,"Original cinematic sampler index/fraction differs");++checks;
  }
  state=start_cinematic_animation(clip(),definition(),true,false,true);
  for(unsigned i=0;i<520;++i)advance_cinematic_animation(state);
  check(state.paused && cinematic_animation_frame(state)==515 && cinematic_animation_time(state)==0,
      "Cinematic-held unit missed the original anticipatory final-frame pause");
  state=start_cinematic_animation(clip(),definition(),true,false,false);frame(state,514);
  check(advance_cinematic_animation(state)==CinematicAnimationAdvance::StopCustom && !state.paused,
      "Ordinary unit completion did not request its actual stop/idle owner");
  auto d=definition();d.timing={{2,0},{3,1}};
  state=start_cinematic_animation(clip(),d,false,false,false);unsigned events=0;
  const auto consume=[&](float f,const auto &event){check(f==float(events),"Animation event order changed");
    check(std::to_integer<unsigned>(event[8])==2+events,"Animation timing event ID changed");++events;};
  advance_cinematic_animation(state,consume);advance_cinematic_animation(state,consume);
  check(events==2,"Authored timing events were dropped");
  state=start_cinematic_animation(clip(),d,false,false,false);bool missing=false;
  try{advance_cinematic_animation(state);}catch(const FormatError&){missing=true;}
  check(missing,"Missing animation event consumer silently discarded authored events");
  std::cout<<"PASS "<<checks<<" original cinematic seeks/clock/query/anchor cases, unit hold/stop and event ownership\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
