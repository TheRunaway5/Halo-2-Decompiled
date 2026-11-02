#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
}
int main(int argc,char** argv) {
  Json report{{"passed",false},{"scope","Declared loop-controller requests with real imported metadata, resources and PCM"},
      {"live_input_or_full_vehicle_parity",false},{"cases",Json::array()},{"required_output_profiles",Json::array()}};
  try {
    need(argc==4,"Supply prepared MCC MAP, original profile inventory, and REPORT");
    auto cache=std::make_shared<const Cache>(argv[1]);need(cache->is_mcc(),"Vehicle profiles require imported MCC data");
    const auto fixture=Json::parse(std::ifstream(argv[2]));
    need(fixture.at("schema")=="H2_VEHICLE_LOOP_PROFILES_TU15_V1" && fixture.at("loops").size()==39,
        "Original vehicle loop inventory differs");
    const auto metadata=MccSoundMetadataPublication::create(cache);
    const auto atmosphere=cache->load_bsp_atmosphere(0);
    need(!cache->spawns().empty(),"Prepared vehicle map has no real listener spawn");
    const auto point=cache->spawns().front().position+Vec3{0,0,1};
    const auto location=atmosphere.location_at(point);
    SourceObjectAudioViewsTu15 views;
    views[0]=SourceObjectAudioViewTu15{point,std::int16_t(location.cluster),
        SourceObjectAudioViewTu15::Orientation{{1,0,0},{0,0,1}},location.leaf};
    unsigned failures{};
    for(const auto& expected:fixture.at("loops"))for(unsigned type=0;type<2;++type) {
      auto& row=report["cases"].emplace_back(Json{{"name",expected.at("name")},{"location_type",type},{"passed",false}});
      try {
        const auto tag=cache->query_tag({.group=TagGroup::from_string("lsnd"),.name=expected.at("name").get<std::string>()});
        need(tag!=nullptr,"Imported authored vehicle loop is absent");
        const auto definition=cache->load_looping_sound(tag->datum);
        row["flags"]=definition.flags;row["original_flags"]=expected.at("flags");
        std::set<std::uint32_t> authored;
        for(const auto& track:definition.tracks)for(const auto sound:{track.start,track.loop,track.end,track.alternate,
            track.alternate_end,track.alternate_transition_in,track.alternate_transition_out})
          if(sound!=0xffffffffu)authored.insert(sound);
        for(const auto& detail:definition.details)if(detail.sound!=0xffffffffu)authored.insert(detail.sound);
        unsigned packets{},emitted_type=type;double peak{};std::uint32_t random=0x76489b21u;
        MultiplayerAudio audio(cache,0x76108bu,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
            [&](const auto& event){
              need(event.frames>0 && event.voice && event.owner==0x12340003u && authored.contains(event.sound),
                  "Vehicle profile lost its authored sound, native voice or declared emitter");
              const bool detail=std::ranges::any_of(definition.details,[&](const auto& row){return row.sound==event.sound;});
              need(event.position.has_value()==(detail || bool(emitted_type)),"Vehicle loop changed its centered/spatial location");++packets;
            },&random);
        audio.bind_native_loop_metadata(metadata);
        audio.bind_object_loop_random_source([]{return 0x3152bd71u;}); // Declared component platform-seed input.
        audio.object_loop_listeners(0,views);
        SourceLoopSoundRequestTu15 request{};
        request.arguments={tag->datum,0,0x12340001u,0x12340002u,0xffffffffu,0,0,0};
        request.emitter_owner=0x12340003u;request.location[3]=std::byte(type);
        const auto put=[&](unsigned at,std::uint32_t value){for(unsigned n=0;n<4;++n)request.location[at+n]=std::byte(value>>(8*n));};
        put(4,std::bit_cast<std::uint32_t>(1.f));
        put(0xc,std::bit_cast<std::uint32_t>(point.x));put(0x10,std::bit_cast<std::uint32_t>(point.y));
        put(0x14,std::bit_cast<std::uint32_t>(point.z));put(0x28,std::bit_cast<std::uint32_t>(location.leaf));
        request.location[0x2c]=std::byte(location.cluster);request.location[0x2d]=std::byte(unsigned(location.cluster)>>8);
        const bool alternate_only=std::ranges::any_of(definition.tracks,[](const auto& track){
          return track.loop==0xffffffffu && track.alternate!=0xffffffffu;});
        request.arguments[6]=alternate_only?1:0;
        if(type && (definition.flags&0x80u)) {
          bool rejected=false;try{audio.object_loop_request(request);}catch(const FormatError& error){
            rejected=std::string(error.what()).find("four-bin playback owner")!=std::string::npos;
          }
          need(rejected && audio.voices()==0 && packets==0,"Combined playback lost its explicit four-bin output requirement");
          row["output_required"]="native four-bin playback/DSP";
          report["required_output_profiles"].push_back(expected.at("name"));row["passed"]=true;continue;
        }
        unsigned tick{};
        for(;tick<990;++tick) {
          request.arguments[5]=tick>=90?2:tick?1:0;
          if(!(definition.flags&0x80u))emitted_type=tick>=45 && tick<60?1u-type:type;
          request.location[3]=std::byte(emitted_type);
          put(4,std::bit_cast<std::uint32_t>(tick<30?.2f:1.f));
          audio.object_loop_request(request);audio.object_loop_frame(1.f/30.f);
          for(const auto value:audio.render(1600)) {
            need(std::isfinite(value),"Imported vehicle profile produced nonfinite PCM");peak=std::max(peak,std::abs(double(value)));
          }
          audio.update();
          if(tick>=90 && !audio.voices())break;
        }
        row["packets"]=packets;row["peak"]=peak;row["active_after_stop"]=audio.voices();
        row["stop_ticks"]=tick>=90?tick-90:0;
        row["centered_spatial_switch_exercised"]=!(definition.flags&0x80u);
        need(packets>0 && peak>0,"Authored vehicle loop produced no PCM");
        need(audio.voices()==0,"Authored vehicle loop did not finish its stop");
        row["passed"]=true;
      }catch(const std::exception& error){row["error"]=error.what();++failures;}
    }
    report["failures"]=failures;report["passed"]=failures==0;
    report["PCM_cases"]=std::ranges::count_if(report["cases"],[](const auto& row){return row.contains("peak") && row.at("peak")>0;});
    report["required_output_cases"]=report["required_output_profiles"].size();
    report["complete_vehicle_output_verified"]=failures==0 && report["required_output_profiles"].empty();
  }catch(const std::exception& error){report["error"]=error.what();}
  if(argc==4){std::ofstream file(argv[3]);file<<report.dump(2)<<'\n';}
  std::cout<<report.dump(2)<<'\n';return report["passed"].get<bool>()?0:1;
}
