#include "halo2/cinematic_animation.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
std::vector<std::byte> decode(const std::string &hex) {
  std::vector<std::byte> result;
  for(std::size_t i=0;i<hex.size();i+=2)result.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
  return result;
}
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
void same(const CinematicAnimationChannel &state,const std::string &expected) {
  const auto bytes=decode(expected);
  check(bytes.size()==state.manager.size() && std::equal(bytes.begin(),bytes.end(),state.manager.begin()),
      "Original initialized animation manager bytes differ");
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply the original initialized animation fixture");
  std::ifstream file(argv[1]);nlohmann::json original;file>>original;
  check(original.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Initialized animation fixture has the wrong original image");
  unsigned managers=0,ticks=0,variants=0;
  for(const auto &row:original.at("cases")) {
    if(row.at("clip_metadata_6c").get<std::string>().empty())continue;
    const auto expected=decode(row.at("bound").at("manager"));
    const auto raw=decode(row.at("clip_metadata_6c"));const Bytes metadata(raw);
    AnimationClip clip;clip.source_graph=std::stoul(row.at("graph").get<std::string>(),nullptr,16);
    const Bytes manager(expected);clip.source_index=manager.u32(4)>>16;
    clip.frame_count=metadata.u16(0x14);clip.loop_frame=metadata.u16(0x2e);
    clip.animation_type=std::to_integer<unsigned char>(raw.at(0x10));
    check(metadata.u16(0x30)==0xffff && metadata.u16(0x32)==0xffff && metadata.u32(0x4c)==0 &&
        metadata.u32(0x54)==0 && metadata.u32(0x5c)==0,"Fixture leaves the admitted unlinked/event-free branch");
    UnitAnimationDefinition definition;definition.frame_count=std::int16_t(clip.frame_count);
    definition.loop_frame=std::int16_t(clip.loop_frame);definition.authored_flags=std::to_integer<unsigned char>(raw.at(0x18));
    std::array<std::uint32_t,4> identifiers;
    for(unsigned i=0;i<4;++i)identifiers[i]=manager.u32(0x70+i*4);
    std::array<std::byte,0x90> backing;backing.fill(std::byte(row.at("caller_buffer_fill").get<unsigned>()));
    auto state=start_initialized_object_animation(clip,definition,identifiers,backing);
    same(state,row.at("bound").at("manager"));++managers;
    check(!custom_cinematic_animation(state),"Default manager acquired a custom animation flag");
    check(cinematic_animation_time(state)==0,"Ordinary idle reported time for the original custom-only unit query");
    for(const auto &step:row.at("steps")) {
      unsigned called=0;
      advance_initialized_object_animation(state,[&](auto packed) {
        ++called;check(packed==manager.u32(4),"Loop requested a different source variant");return packed;
      });
      same(state,step.at("updated").at("manager"));
      check(called==step.at("variant_calls").get<unsigned>(),"Original loop variant call order differs");
      ++ticks;variants+=called;
    }
    state=start_initialized_object_animation(clip,definition,identifiers,backing);
    bool required=false;
    try{for(unsigned i=0;i<clip.frame_count;++i)advance_initialized_object_animation(state,{});}
    catch(const FormatError &e){required=std::string(e.what()).find("resident variant owner")!=std::string::npos;}
    check(required,"Loop silently substituted a missing resident variant owner");
  }
  std::cout<<managers<<" exact original initialized managers, "<<ticks<<" complete clock updates, "
      <<variants<<" ordered loop variants passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
