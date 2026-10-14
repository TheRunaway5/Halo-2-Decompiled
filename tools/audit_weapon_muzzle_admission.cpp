#include "halo2/content.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Supply MAP SHARED XBE OUTPUT_JSON");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  Json report={{"scope","Actual authored normal, misfire and empty firing references, particle materials and FP loader admission; this does not prove source spawning or rendered parity."},
    {"map",argv[1]},{"effects",Json::array()}};
  std::map<std::uint32_t,SourceTagMetadata> owners;
  const auto word=[&](std::uint32_t address) {
    const auto *owner=cache.query_tag({.contains=TagAddressRange{address,4},.require_address=true});
    if(!owner)throw FormatError("Firing reference has no resident source tag owner");
    auto found=owners.find(owner->datum);
    if(found==owners.end())found=owners.emplace(owner->datum,cache.load_source_tag_metadata(owner->datum)).first;
    return Bytes(found->second.bytes_at(address,4)).u32(0);
  };
  for(const auto &tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true})) {
    const auto count=word(tag.address+0x2d0);
    const auto barrels=count?word(tag.address+0x2d4):0;
    for(unsigned n=0;n<count;++n) {
      const auto b=barrels+n*0xec;
      const auto count_=word(b+0xe4);
      const auto first=count_?word(b+0xe8):0;
      for(unsigned e=0;e<count_;++e)for(unsigned selection=0;selection<3;++selection) {
        const auto effect=word(first+e*0x34+8+selection*8);
        if(effect==null_datum)continue;
        Json row={{"weapon",tag.name},{"barrel",n},{"entry",e},{"selection",selection},{"effect",effect}};
        const auto *definition_tag=cache.query_tag({.handle=TagHandle{effect},.require_address=true});
        if(!definition_tag) {row["boundary"]="Source firing reference is not resident";row["admitted"]=false;report["effects"].push_back(std::move(row));continue;}
        row["group"]=definition_tag->group;row["name"]=definition_tag->name;
        if(definition_tag->group!="effe") {row["admitted"]=false;row["boundary"]="Original non-particle firing reference";report["effects"].push_back(std::move(row));continue;}
        row["flags"]=word(definition_tag->address);
        row["particle_systems"]=Json::array();
        const auto events=word(definition_tag->address+0x14),event_rows=word(definition_tag->address+0x18);
        unsigned beams{},accelerations{};
        for(unsigned event=0;event<events;++event){beams+=word(event_rows+event*0x38+0x20);accelerations+=word(event_rows+event*0x38+0x28);}
        row["beam_count"]=beams;row["acceleration_count"]=accelerations;
        for(unsigned event=0;event<events;++event) {
          const auto event_at=event_rows+event*0x38;
          const auto systems=word(event_at+0x30);
          for(unsigned system=0;system<systems;++system) {
            Json particle={{"event",event},{"system",system},{"beam_count",word(event_at+0x20)},
              {"acceleration_count",word(event_at+0x28)}};
            try {
              const auto authored=cache.load_particle_system_render_definition(effect,event,system);
              particle["particle"]=authored.particle_datum;particle["coordinates"]=authored.coordinates;
              particle["flags"]=authored.flags;particle["emitters"]=authored.emitters.size();
              const auto *tag_=cache.query_tag({.handle=TagHandle{authored.particle_datum}});
              const auto material=cache.load_particle_material(authored.particle_datum,15,0,tag_ && tag_->group=="PRTM"?1:0);
              particle["classification"]=material.vertex.classification;
              particle["vertex_program_bytes"]=material.vertex.program.size();
              particle["bitmaps"]=Json::array();
              for(const auto &bitmap:material.bitmaps)if(bitmap)particle["bitmaps"].push_back(bitmap->datum);
            }catch(const std::exception &error){particle["boundary"]=error.what();}
            row["particle_systems"].push_back(std::move(particle));
          }
        }
        try {
          const auto definition=cache.load_muzzle_effect(effect);
          row["admitted"]=true;row["systems"]=definition.systems.size();
          row["events"]=definition.events.size();row["sounds"]=definition.sounds.size();
        }catch(const std::exception &error){row["admitted"]=false;row["boundary"]=error.what();}
        report["effects"].push_back(std::move(row));
      }
    }
  }
  report["starting_choices"]=Json::array();
  for(unsigned option=3;option<=20;++option) {
    try {
      const auto weapon=cache.slayer_starting_weapon(option);
      const auto *tag=cache.query_tag({.handle=TagHandle{weapon}});
      report["starting_choices"].push_back({{"option",option},{"weapon",weapon},{"name",tag->name}});
    }catch(const FormatError &error) {report["starting_choices"].push_back({{"option",option},{"boundary",error.what()}});}
  }
  std::ofstream output(argv[4]);if(!output)throw FormatError("Cannot create muzzle admission report");
  output<<report.dump(2)<<'\n';
  for(const auto &row:report["effects"])std::cout<<row["weapon"]<<" admitted="<<row["admitted"]
      <<(row.contains("boundary")?" "+row["boundary"].get<std::string>():"")<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
