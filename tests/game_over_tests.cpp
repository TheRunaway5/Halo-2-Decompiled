#include "halo2/source_game_over.hpp"
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
std::vector<unsigned char> bytes(const std::string& hex) {
  std::vector<unsigned char> out;
  for(unsigned i=0;i<hex.size();i+=2)out.push_back(std::stoul(hex.substr(i,2),nullptr,16));
  return out;
}
float literal(const std::string& hex) {
  const auto raw=bytes(hex);check(raw.size()==4,"Original float width differs");
  float value;std::memcpy(&value,raw.data(),4);return value;
}
float word(const Json& value) {
  return std::bit_cast<float>(std::uint32_t(std::stoul(value.get<std::string>(),nullptr,16)));
}
SourceGameOverState state(const Json& row) {
  return {row.at("active"),row.at("fade_started"),row.at("countdown")};
}
void compare(const SourceGameOverState& actual,const Json& row) {
  check(actual.active==row.at("active") && actual.fade_started==row.at("fade_started") &&
      actual.countdown==row.at("countdown") && actual.complete()==row.at("complete"),
      "Original game-over record/completion differs");
}
SoundClassFades saved_audio(const Json& value) {
  const auto raw=bytes(value);check(raw.size()==54*16,"Original saved audio width differs");
  SoundClassFades out;
  // Declared saved-record API inputs. This mutable test object supplies the
  // original target/current/time array, without changing production access.
  auto& records=const_cast<std::array<SoundClassFades::Fade,54>&>(out.state());
  for(unsigned i=0;i<54;++i)std::memcpy(&records[i],raw.data()+i*16,12);
  return out;
}
void compare(const SoundClassFades& actual,const Json& value) {
  const auto expected=bytes(value);check(expected.size()==54*16,"Original audio output width differs");
  for(unsigned i=0;i<54;++i)check(std::memcmp(&actual.state()[i],expected.data()+i*16,12)==0,
      "Original class target/current/time bits differ");
}
}
int main(int argc,char**argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original game-over fixture");
    Json data;std::ifstream(argv[1])>>data;
    check(std::endian::native==std::endian::little,"Original byte fixture requires little-endian host");
    check(data.at("instruction_patches").empty() && data.at("supplied_returns").empty(),"Original returns were supplied");
    unsigned starts=0,boundaries=0,sequence_ticks=0,fades=0;
    for(const auto& version:data.at("versions")) {
      SourceGameOverConstants constants{literal(version.at("constants")[0]),
          literal(version.at("constants")[1]),literal(version.at("constants")[2]),1.f};
      for(unsigned i=0;i<54;++i)check(sound_class_names()[i]==version.at("names")[i].get<std::string>(),"Original class table differs");
      for(const auto& row:version.at("starts")) {
        auto actual=state(row.at("before"));source_game_over_begin(actual,row.at("rate"),constants);
        compare(actual,row.at("after"));source_game_over_begin(actual,row.at("rate"),constants);
        compare(actual,row.at("after"));++starts;
      }
      for(const auto& row:version.at("boundaries")) {
        auto actual=state(row.at("before"));SoundClassFades classes;
        source_game_over_tick(actual,word(row.at("dt_bits")),classes,row.at("engine_present"),constants);
        compare(actual,row.at("after"));compare(classes,row.at("classes"));++boundaries;
      }
      SourceGameOverState actual;SoundClassFades classes;source_game_over_begin(actual,30,constants);
      for(const auto& row:version.at("sequence")) {
        if(row.at("step").get<unsigned>()) {
          source_game_over_tick(actual,1.f/30,classes,true,constants);
          classes.advance(word(row.at("audio_dt_bits")));++sequence_ticks;
        }
        compare(actual,row.at("record"));compare(classes,row.at("classes"));
      }
      for(const auto& row:version.at("fades")) {
        auto classes=saved_audio(row.at("before"));source_game_over_fade(classes,row.at("engine_present"),constants);
        compare(classes,row.at("after_request"));classes.advance(word(row.at("audio_dt_bits")));
        compare(classes,row.at("after_advance"));++fades;
      }
    }
    check(starts==48 && boundaries==1320 && sequence_ticks==675 && fades==72,"Original countdown/fade coverage truncated");
    std::cout<<checks<<" comparisons: three original images,48 starts,1320 boundary ticks,675 countdown/audio ticks and72 interrupted class fades passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
