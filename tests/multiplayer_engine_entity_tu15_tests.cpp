#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_multiplayer_engine_entity_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string value;value.reserve(bytes.size()*2);
  for(const auto b:bytes){const auto v=std::to_integer<unsigned>(b);value+=digits[v>>4];value+=digits[v&15];}return value;
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned index) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Engine entity test requires owned scratch");
    path=std::filesystem::path(scratch)/("engine-entity-"+std::to_string(index));std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;check(argc==4 || argc==5,"Usage: multiplayer_engine_entity_tu15_tests MCC_MAP VARIANTS REPORT [PRESET]");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  Json report={{"passed",false},{"cases",Json::array()},
    {"scope","Read-only complete original engine entity construction/difference callbacks over canonical actual source world inputs. Native output, original callback ABI metadata and each observed source input byte are retained for an independent unchanged-Xbox comparison. Private caller storage is disjoint from all retained world reservations; no world writes, tick advancement, service returns or engine states are supplied by the query. Transport/client application and complete network matches remain separate."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain engine entity evidence");};
  unsigned index{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    const auto name=preset.at("name").get<std::string>();if(argc==5 && name!=argv[4])continue;
    Json row={{"name",name},{"passed",false},{"samples",Json::array()}};
    try {
      Files files(index++);MultiplayerOptions options;options.map=argv[1];options.source_object_world=true;
      options.source_object_savegame_directory=files.path;options.variant={name,preset.at("frontend_engine").get<unsigned>(),{}};
      for(const auto& w:preset.at("writes"))options.variant.values.append(w[0].get<unsigned>(),w[1].get<int>());
      const auto variant=source_game_variant(options.variant);const auto type=source_game_variant_type(variant);
      options.local_players=type==7?4:2;
      if(source_game_variant_get(variant,91))for(unsigned n=0;n<options.local_players;++n)options.selected_teams.push_back(n%2);
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& world=prepared.source_objects->tu15_world();auto& memory=world.memory();
      check(prepared.rules.requires_source_engine,"Engine entity selected a bounded adapter");
      // Obtain the actual native caller's retained FP controls. The query
      // itself receives only const world storage and never calls this owner.
      const auto caller=world.prepare_source_call({});check(caller.floating_point.has_value(),"Engine entity query has no actual FP owner");
      const auto fp=*caller.floating_point;
      const auto oracle_caller=memory.find_unowned_range(0x7fa00000u,0x7fe00000u,0x10000u,0x10000u);
      check(oracle_caller.has_value(),"Engine entity oracle lacks a separate unowned caller frame");
      std::array<Controls,multiplayer_capacity> controls{};
      for(const auto slot:prepared.local_slots){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
      const auto capture=[&](const char* stage) {
        const auto engine=word(memory,0x4f63b8u);const auto before_span=memory.view(engine,0xc34u);
        const std::vector<std::byte> before(before_span.begin(),before_span.end());const auto tick=match.state().tick;
        std::vector<SourceMultiplayerEngineEntityReadTu15> reads;
        const auto entity=source_multiplayer_engine_entity_tu15(memory,fp,&reads);
        const auto repeated=source_multiplayer_engine_entity_tu15(memory,fp);
        std::array<std::uint16_t,16> enemy_teams{};
        for(unsigned left=0;left<16;++left)for(unsigned right=0;right<16;++right)
          if(source_multiplayer_engine_entity_enemies_tu15(entity,left,right))enemy_teams[left]|=std::uint16_t(1u<<right);
        check(entity==repeated && entity.type==type && entity.bytes.size()==source_multiplayer_engine_entity_size_tu15(type),
            "Original engine entity projection is unstable or has the wrong schema");
        check(std::equal(before.begin(),before.end(),memory.view(engine,before.size()).begin()) && match.state().tick==tick,
            "Read-only engine entity query changed its world/tick");
        std::map<std::uint32_t,std::byte> inputs;
        for(const auto& read:reads)for(unsigned n=0;n<read.bytes.size();++n) {
          const auto [at,inserted]=inputs.emplace(read.address+n,read.bytes[n]);
          check(inserted || at->second==read.bytes[n],"Engine entity observed changing world input bytes");
        }
        Json spans=Json::array();
        while(!inputs.empty()) {
          const auto first=inputs.begin()->first;auto at=first;std::vector<std::byte> bytes;
          while(!inputs.empty() && inputs.begin()->first==at){bytes.push_back(inputs.begin()->second);inputs.erase(inputs.begin());++at;}
          spans.push_back({{"address",first},{"bytes",hex(bytes)}});
        }
        const auto object=word(memory,0x56dfa0u+unsigned(type)*4u),vtable=word(memory,object);
        row["samples"].push_back({{"stage",stage},{"tick",tick},{"type",type},{"changed",entity.changed},{"bytes",hex(entity.bytes)},
            {"object",object},{"initialize",word(memory,vtable+44u*4u)},{"difference",word(memory,vtable+45u*4u)},
            {"relationship_entry",word(memory,vtable+27u*4u)},{"relationship_mask",entity.relationship_mask},{"enemy_teams",enemy_teams},
            {"x87_control_word",fp.x87_control_word},{"mxcsr",fp.mxcsr},{"oracle_caller_base",*oracle_caller},{"world_inputs",std::move(spans)}});
      };
      capture("initial-active-frame");for(unsigned n=0;n<3;++n)match.step(controls);capture("after-three-ordinary-frames");
      row["passed"]=true;
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<Json{{"name",name},{"passed",row["passed"]},{"error",row.value("error",std::string{})}}.dump()<<'\n';
    report["cases"].push_back(std::move(row));save();
  }
  check(index==(argc==5?1u:39u),"Engine entity preset coverage changed");report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
