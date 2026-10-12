#include "halo2/source_engine_entry.hpp"
#include "halo2/source_engine_entity.hpp"
#include "halo2/source_king_replication.hpp"
#include "halo2/source_game_over.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
std::vector<std::uint8_t> bytes(const std::string& hex) {
  check(hex.size()%2==0,"Odd original byte string");std::vector<std::uint8_t> out(hex.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return out;
}
std::vector<std::uint8_t> blob(const Json& fixture,const Json& key) {
  return bytes(fixture.at("raw_blobs").at(key.get<std::string>()));
}
template<class T>T decode(std::span<const std::uint8_t> raw) {
  check(raw.size()==sizeof(T),"Original object width differs");T value;
  std::memcpy(&value,raw.data(),sizeof(T));return value;
}
template<class T>std::vector<std::uint8_t> raw(const T& value) {
  std::vector<std::uint8_t> result(sizeof(value));std::memcpy(result.data(),&value,sizeof(value));return result;
}
std::uint16_t word(std::span<const std::uint8_t> data,unsigned at){return std::uint16_t(data[at]|unsigned(data[at+1])<<8);}
void word(std::span<std::uint8_t> data,unsigned at,std::uint16_t value){data[at]=std::uint8_t(value);data[at+1]=std::uint8_t(value>>8);}
SourceEngineReplicationState shared(std::span<const std::uint8_t> data) {
  SourceEngineReplicationState state;
  for(unsigned i=0;i<14;++i)state.words_06_21[i]=word(data,6+i*2);
  state.phase=std::bit_cast<std::int16_t>(word(data,0x6c));state.round=word(data,0x6e);
  state.clock_seconds=word(data,0xe0);state.game_over=data[0xc08];return state;
}
void encode(std::span<std::uint8_t> data,const SourceEngineReplicationState& state) {
  for(unsigned i=0;i<14;++i)word(data,6+i*2,state.words_06_21[i]);
  word(data,0x6c,std::uint16_t(state.phase));word(data,0x6e,state.round);
  word(data,0xe0,state.clock_seconds);data[0xc08]=state.game_over;
}
std::array<std::uint8_t,8> encode(const SourceGameOverState& state,const std::array<std::uint8_t,8>& original) {
  auto result=original;result[0]=std::uint8_t(state.active);result[1]=std::uint8_t(state.fade_started);
  std::memcpy(result.data()+4,&state.countdown,4);return result;
}
struct Event {
  std::string kind;std::vector<std::uint8_t> globals,entry,arena;
  std::array<std::uint8_t,8> game;std::uint32_t mask{},size{},opaque{};unsigned phase{};
};
void unavailable_inputs() {
  SourceEngineEntryRetail entry;entry.prefix.definition=0;entry.state_size=5;
  std::array<SourceEngineEntryDefinition,1> definitions;
  std::array<std::uint8_t,8> original{1,2,3,4,5,6,7,8},live{};
  SourceEngineEntryBlock block{0,-1,0,original};
  auto rejected=[&](auto action){try{action();return false;}catch(const std::runtime_error&){return true;}};
  check(rejected([&]{source_engine_entry_update_apply(entry,definitions,0,block,live);}) &&
      live==std::array<std::uint8_t,8>{1,2,3,4,5,0,0,0},"Missing provider did not preserve preceding copy");
  unsigned calls=0;definitions[0].apply=[&](auto input,std::uint32_t mask,std::uint32_t size,auto bank) {
    ++calls;check(input.data()==reinterpret_cast<std::uint8_t*>(&entry) && mask==0x80000000 && size==5 && bank.size()==5,
        "Entry callback did not receive raw entry/size/mask/live bank");return false;
  };
  source_engine_entry_update_apply(entry,definitions,0x80000000,block,live);
  check(calls==1,"False application result became an extra entry policy");
  live.fill(0);entry.state_size=9;
  check(rejected([&]{source_engine_entry_update_apply(entry,definitions,0,block,live);}) && live[0]==0,
      "Unavailable backing was substituted or partially copied");
  entry.state_size=5;entry.prefix.definition=-1;
  check(rejected([&]{source_engine_entry_update_apply(entry,definitions,0,block,live);}) && live[0]==0,
      "Unavailable definition table input did not fail at lookup");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply unchanged-original entry application fixture");std::ifstream input(argv[1]);Json fixture;input>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original entry proof supplies replacement results");
  check(std::fesetround(FE_TONEAREST)==0,"Unable to select original nearest rounding");
  const auto marker_bytes=bytes(fixture.at("map").at("rows_hex"));check(marker_bytes.size()%32==0,"Scenario row width differs");
  std::vector<SourceKingMarker> markers(marker_bytes.size()/32);
  if(!markers.empty())std::memcpy(markers.data(),marker_bytes.data(),marker_bytes.size());
  unsigned calls=0,rejections=0,bank_calls=0,services_count=0,builds=0,controls=0;
  for(const auto& [version,data]:fixture.at("versions").items()) {
    SourceGameOverConstants game_constants;game_constants.seconds=decode<float>(bytes(data.at("game_over_seconds_hex")));
    for(unsigned n=0;n<fixture.at("inputs").size();++n)try {
      const auto& row=fixture.at("inputs")[n];const auto& expected=data.at("results")[n];
      const auto original=blob(fixture,row.at("globals"));auto state=shared(original);
      auto king=decode<SourceKingState>(std::span(original).subspan(0xfc,sizeof(SourceKingState)));const auto initial_king=king;
      const auto snapshot=[&] {
        auto result=original;encode(result,state);std::memcpy(result.data()+0xfc,&king,sizeof(king));return result;
      };
      auto arena=bytes(row.at("arena_hex"));auto live=std::span(arena).subspan(row.at("live_offset").get<unsigned>(),80);
      const SourceEngineEntryBlock block{std::bit_cast<std::int16_t>(row.at("block_size_bits").get<std::uint16_t>()),
        std::bit_cast<std::int16_t>(row.at("block_type_bits").get<std::uint16_t>()),
        fixture.at("arena_address").get<std::uint32_t>()+row.at("source_offset").get<unsigned>(),
        std::span(arena).subspan(row.at("source_offset").get<unsigned>(),80)};
      const auto block_before=std::array<std::int64_t,3>{block.size,block.type,block.data_address};
      auto game_raw=decode<std::array<std::uint8_t,8>>(bytes(row.at("game_over_hex")));
      SourceGameOverState game;game.active=game_raw[0]!=0;game.fade_started=game_raw[1]!=0;
      std::memcpy(&game.countdown,game_raw.data()+4,4);
      auto entry=bytes(row.at("prefix_hex"));
      if(version!="retail") {
        const auto secondary=raw(row.at("secondary_pending").get<std::uint32_t>());
        entry.insert(entry.end(),secondary.begin(),secondary.end());
      }
      const auto tail=raw(std::array<std::uint32_t,4>{row.at("creation_size"),row.at("creation_address"),row.at("state_size"),
        fixture.at("arena_address").get<std::uint32_t>()+row.at("live_offset").get<unsigned>()});
      entry.insert(entry.end(),tail.begin(),tail.end());
      check(entry==bytes(expected.at("row_hex")),"Declared input entry does not match original unchanged metadata");
      std::vector<Event> events;
      const auto observe=[&](const char* kind,std::uint32_t mask=0,std::uint32_t size=0,std::uint32_t opaque=0,unsigned phase=0) {
        events.push_back({kind,snapshot(),entry,arena,encode(game,game_raw),mask,size,opaque,phase});
      };
      SourceEngineReplicationServices services;
      // Whole original empty-roster / disabled-recorder paths are separately
      // executed in the oracle. These observations do not bind a live world.
      services.refresh_player_properties=[&]{observe("refresh_player_properties");};
      services.refresh_recording=[&]{observe("refresh_recording");};
      services.phase_changed=[&](std::uint8_t value){check(value!=1,"Phase1 requires a world owner");observe("phase_changed",0,0,0,value);};
      services.begin_game_over=[&]{observe("begin_game_over");source_game_over_begin(game,row.at("rate"),game_constants);};
      std::uint32_t selected_index;std::memcpy(&selected_index,original.data()+0xc14,4);
      const auto& identity=data.at("identities").at(selected_index);SourceEngineEntityCallbacks engine;
      engine.identifier=identity.at("identifier");engine.apply=[&](std::uint32_t mask,std::uint32_t opaque,std::span<const std::uint8_t> storage) {
        observe("bank_apply",mask,0,opaque);auto bank=decode<SourceKingReplicationBank>(storage.first(40));
        return source_king_replication_apply(state,king,markers,mask,bank,services);
      };
      const auto* selected=identity.at("present").get<bool>() ? &engine : nullptr;
      std::array<SourceEngineEntryDefinition,4> definitions;bool inner_result=false;
      definitions[3].apply=[&](std::span<std::uint8_t> received_entry,std::uint32_t mask,std::uint32_t size,std::span<const std::uint8_t> bank) {
        check(std::vector<std::uint8_t>(received_entry.begin(),received_entry.end())==entry,"Callback received a different entry layout");
        check(bank.data()==live.data(),"Callback received the private rather than copied live bank");
        observe("definition_apply",mask,size);inner_result=source_engine_entity_apply(selected,data.at("definition_identifier"),mask,size,bank);
        return inner_result;
      };
      if(version=="retail") {
        auto value=decode<SourceEngineEntryRetail>(entry);source_engine_entry_update_apply(value,definitions,row.at("mask"),block,live);
        check(raw(value)==entry,"Native changed retail entry metadata");
      }else {
        auto value=decode<SourceEngineEntryTitleUpdate>(entry);source_engine_entry_update_apply(value,definitions,row.at("mask"),block,live);
        check(raw(value)==entry,"Native changed title-update entry metadata");
      }
      check(std::array<std::int64_t,3>{block.size,block.type,block.data_address}==block_before,"Native modified block metadata");
      check(unsigned(inner_result)==expected.at("observed_inner_al"),"Original observed inner callback AL differs");
      check(arena==bytes(expected.at("arena_hex")),"Original live copy/private bytes/guard storage differs");
      check(snapshot()==blob(fixture,expected.at("globals")),"Whole original entry-applied match record differs");
      check(raw(encode(game,game_raw))==bytes(expected.at("game_over_hex")),"Original entry-applied game-over effects differ");
      unsigned index=0;
      for(const auto& event:expected.at("events")) {
        if(event.at("kind")=="build") {
          check(index==events.size(),"Original hill build preceded a shared service");auto before=snapshot();auto before_king=initial_king;
          before_king.hill=decode<SourceKingReplicationBank>(std::span(live).first(40)).hill;
          std::memcpy(before.data()+0xfc,&before_king,sizeof(before_king));
          check(before==blob(fixture,event.at("globals")),"Entry-applied hill was not stored before rebuild");++builds;continue;
        }
        check(index<events.size(),"Native skipped an original entry callback");const auto& actual=events[index++];
        check(actual.kind==event.at("kind").get<std::string>() && actual.globals==blob(fixture,event.at("globals")),"Original entry callback pre-state/order differs");
        check(actual.entry==bytes(event.at("row_hex")) && actual.arena==bytes(event.at("arena_hex")),"Copy did not precede entry callback or entry layout differs");
        check(raw(actual.game)==bytes(event.at("game_over_hex")),"Entry callback game pre-state differs");
        if(event.contains("mask"))check(actual.mask==event.at("mask"),"Entry callback mask argument differs");
        if(event.contains("state_size"))check(actual.size==event.at("state_size"),"Entry callback state size differs");
        if(event.contains("opaque"))check(actual.opaque==event.at("opaque"),"Bank callback opaque size argument differs");
        if(event.contains("phase"))check(actual.phase==event.at("phase"),"Entry-applied phase argument differs");
        if(actual.kind=="bank_apply")++bank_calls;else if(actual.kind!="definition_apply")++services_count;
      }
      check(index==events.size(),"Native added an entry callback");++calls;if(!inner_result)++rejections;if(row.value("owner_control",false))++controls;
    }catch(const std::exception&){std::cerr<<version<<" input="<<n<<'\n';throw;}
  }
  unavailable_inputs();std::cout<<"Original entry application: "<<calls<<" whole calls, "<<rejections<<" identity rejections after copy, "
    <<bank_calls<<" real King bank callbacks, "<<services_count<<" shared services, "<<builds<<" hill rebuilds, "
    <<controls<<" copy/remainder/overlap controls; unavailable-provider ordering passes\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
