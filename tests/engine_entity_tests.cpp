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
template<class T>void equal(const T& value,std::span<const std::uint8_t> raw,const char* message) {
  check(raw.size()==sizeof(T) && std::memcmp(&value,raw.data(),sizeof(T))==0,message);
}
std::uint16_t word(std::span<const std::uint8_t> raw,unsigned at){return std::uint16_t(raw[at]|unsigned(raw[at+1])<<8);}
void word(std::span<std::uint8_t> raw,unsigned at,std::uint16_t value){raw[at]=std::uint8_t(value);raw[at+1]=std::uint8_t(value>>8);}
SourceEngineReplicationState shared(std::span<const std::uint8_t> raw) {
  SourceEngineReplicationState state;
  for(unsigned i=0;i<14;++i)state.words_06_21[i]=word(raw,6+i*2);
  state.phase=std::bit_cast<std::int16_t>(word(raw,0x6c));state.round=word(raw,0x6e);
  state.clock_seconds=word(raw,0xe0);state.game_over=raw[0xc08];return state;
}
void encode(std::span<std::uint8_t> raw,const SourceEngineReplicationState& state) {
  for(unsigned i=0;i<14;++i)word(raw,6+i*2,state.words_06_21[i]);
  word(raw,0x6c,std::uint16_t(state.phase));word(raw,0x6e,state.round);
  word(raw,0xe0,state.clock_seconds);raw[0xc08]=state.game_over;
}
std::array<std::uint8_t,8> encode(const SourceGameOverState& state,const std::array<std::uint8_t,8>& original) {
  auto raw=original;raw[0]=std::uint8_t(state.active);raw[1]=std::uint8_t(state.fade_started);
  std::memcpy(raw.data()+4,&state.countdown,4);return raw;
}
struct Event {
  std::string kind;std::vector<std::uint8_t> globals,bank;
  std::array<std::uint8_t,8> game_over;std::uint32_t mask{},opaque{};unsigned phase{};
};
void missing_services() {
  SourceEngineEntityCallbacks engine;engine.identifier=3;std::array<std::uint8_t,40> bank{};
  std::uint32_t flags=7,entity=0x12340007;SourceEngineEntityCreation creation{0xdead0009,3,0xa55a};
  // Rejection precedes provider validation, including absent engines.
  check(!source_engine_entity_initialize(nullptr,3,0,bank),"Absent engine initialized King");
  check(!source_engine_entity_difference(&engine,2,flags,0,bank) && flags==7,"Mismatch changed difference mask");
  check(!source_engine_entity_apply(&engine,2,flags,0,bank),"Mismatched definition applied");
  auto rejected=[&](auto action){try{action();return false;}catch(const std::runtime_error&){return true;}};
  check(rejected([&]{source_engine_entity_initialize(&engine,3,0,bank);}),"Missing initializer silently succeeded");
  check(rejected([&]{source_engine_entity_difference(&engine,3,flags,0,bank);}),"Missing difference silently succeeded");
  check(rejected([&]{source_engine_entity_apply(&engine,3,0,0,bank);}),"Missing application silently succeeded");
  check(rejected([&]{source_engine_entity_create(&engine,entity,creation,1,0,bank);}) && entity==creation.entity,
      "Creation provider failure did not retain preceding adoption");
  engine.apply=[&](std::uint32_t mask,std::uint32_t opaque,std::span<const std::uint8_t> input) {
    check(mask==7 && opaque==0x12345678 && input.data()==bank.data() && entity==creation.entity,
        "Creation application ran before adoption or lost forwarded inputs");return false;
  };
  // Generic callback return contracts are also explicit in the full owners:
  // application propagates false; creation ignores it and returns true.
  check(!source_engine_entity_apply(&engine,3,7,0x12345678,bank),"Application discarded callback result");
  check(source_engine_entity_create(&engine,entity,creation,7,0x12345678,bank),"Creation propagated callback false");
  creation.identifier=-1;creation.entity=0xabcdef09;entity=1;
  check(source_engine_entity_create(nullptr,entity,creation,0,0,bank) && entity==creation.entity,
      "Service-free NONE creation was over-rejected");
  entity=1;check(rejected([&]{source_engine_entity_create(nullptr,entity,creation,1,0,bank);}) && entity==creation.entity,
      "Absent-engine failure changed adoption order");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply unchanged-original entity dispatch fixture");std::ifstream input(argv[1]);Json fixture;input>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original dispatch proof supplies replacement results");
  check(std::fesetround(FE_TONEAREST)==0,"Unable to select original nearest rounding");
  const auto marker_bytes=bytes(fixture.at("map").at("rows_hex"));check(marker_bytes.size()%32==0,"Scenario row width differs");
  std::vector<SourceKingMarker> markers(marker_bytes.size()/32);
  if(!markers.empty())std::memcpy(markers.data(),marker_bytes.data(),marker_bytes.size());
  unsigned calls=0,faults=0,callbacks=0,shared_callbacks=0,builds=0,rejections=0,adoptions=0;
  for(const auto& [version,data]:fixture.at("versions").items()) {
    SourceGameOverConstants game_constants;game_constants.seconds=decode<float>(bytes(data.at("game_over_seconds_hex")));
    for(unsigned n=0;n<fixture.at("inputs").size();++n)try {
      const auto& row=fixture.at("inputs")[n];const auto& expected=data.at("results")[n];
      auto raw=blob(fixture,row.at("globals"));auto bank=blob(fixture,row.at("bank"));auto state=shared(raw);
      auto king=decode<SourceKingState>(std::span(raw).subspan(0xfc,sizeof(SourceKingState)));const auto initial_king=king;
      std::uint32_t entity;std::memcpy(&entity,raw.data()+0x24,4);
      auto game_raw=decode<std::array<std::uint8_t,8>>(bytes(row.at("game_over_hex")));
      SourceGameOverState game;game.active=game_raw[0]!=0;game.fade_started=game_raw[1]!=0;
      std::memcpy(&game.countdown,game_raw.data()+4,4);
      const auto snapshot=[&] {
        auto result=raw;encode(result,state);std::memcpy(result.data()+0xfc,&king,sizeof(king));
        std::memcpy(result.data()+0x24,&entity,4);return result;
      };
      std::vector<Event> events;
      const auto observe=[&](const char* kind,std::uint32_t mask=0,std::uint32_t opaque=0,unsigned phase=0) {
        events.push_back({kind,snapshot(),bank,encode(game,game_raw),mask,opaque,phase});
      };
      SourceEngineReplicationServices services;
      // These observations stand for whole original empty-roster/disabled
      // recorder paths. They are not providers for an occupied native world.
      services.refresh_player_properties=[&]{observe("refresh_player_properties");};
      services.refresh_recording=[&]{observe("refresh_recording");};
      services.phase_changed=[&](std::uint8_t value){check(value!=1,"Phase1 requires a world provider");observe("phase_changed",0,0,value);};
      services.begin_game_over=[&]{observe("begin_game_over");source_game_over_begin(game,row.at("rate"),game_constants);};
      SourceEngineEntityCallbacks engine;const auto& identity=data.at("identities").at(row.at("selected").get<unsigned>());
      engine.identifier=identity.at("identifier");
      engine.initialize=[&](std::uint32_t opaque,std::span<std::uint8_t> storage) {
        check(engine.identifier==3,"Non-King callback needs a recovered owner");observe("bank_initialize",0,opaque);
        auto value=decode<SourceKingReplicationBank>(storage);source_king_replication_initialize(value);std::memcpy(storage.data(),&value,sizeof(value));
      };
      engine.difference=[&](std::uint32_t& mask,std::uint32_t opaque,std::span<std::uint8_t> storage) {
        check(engine.identifier==3,"Non-King callback needs a recovered owner");observe("bank_difference",mask,opaque);
        auto value=decode<SourceKingReplicationBank>(storage);source_king_replication_difference(state,king,mask,value);std::memcpy(storage.data(),&value,sizeof(value));
      };
      engine.apply=[&](std::uint32_t mask,std::uint32_t opaque,std::span<const std::uint8_t> storage) {
        check(engine.identifier==3,"Non-King callback needs a recovered owner");observe("bank_apply",mask,opaque);
        auto value=decode<SourceKingReplicationBank>(storage);return source_king_replication_apply(state,king,markers,mask,value,services);
      };
      const SourceEngineEntityCallbacks* selected=identity.at("present").get<bool>() ? &engine : nullptr;
      auto mask=row.at("mask").get<std::uint32_t>();const auto opaque=row.at("opaque").get<std::uint32_t>();
      const auto operation=row.at("operation").get<std::string>();bool returned=false,failed=false;
      try {
        if(operation=="initialize")returned=source_engine_entity_initialize(selected,data.at("definition_identifier"),opaque,bank);
        else if(operation=="difference")returned=source_engine_entity_difference(selected,data.at("definition_identifier"),mask,opaque,bank);
        else if(operation=="apply")returned=source_engine_entity_apply(selected,data.at("definition_identifier"),mask,opaque,bank);
        else {
          const auto creation=decode<SourceEngineEntityCreation>(bytes(row.at("creation_hex")));
          returned=source_engine_entity_create(selected,entity,creation,mask,opaque,bank);
        }
      }catch(const std::runtime_error&){if(operation!="create_fault")throw;failed=true;}
      check(failed==!expected.at("fault").is_null(),"Original fault boundary differs");
      if(failed)++faults;else {check(unsigned(returned)==expected.at("returned_al"),"Original dispatch return differs");++calls;if(!returned)++rejections;}
      check(snapshot()==blob(fixture,expected.at("globals")),"Whole original dispatched MP record differs");
      check(bank==blob(fixture,expected.at("bank")),"Original dispatched bank differs");
      equal(encode(game,game_raw),bytes(expected.at("game_over_hex")),"Original dispatched game-over effects differ");
      check(mask==expected.at("flags"),"Original dispatched difference mask differs");
      unsigned index=0;
      for(const auto& event:expected.at("events")) {
        if(event.at("kind")=="build") {
          check(index==events.size(),"Hill build preceded a shared callback");auto before=snapshot();auto before_king=initial_king;
          before_king.hill=decode<SourceKingReplicationBank>(bank).hill;std::memcpy(before.data()+0xfc,&before_king,sizeof(before_king));
          check(before==blob(fixture,event.at("globals")),"Original hill store/rebuild order differs");++builds;continue;
        }
        check(index<events.size(),"Native skipped a dispatched callback");const auto& actual=events[index++];
        check(actual.kind==event.at("kind").get<std::string>() && actual.globals==blob(fixture,event.at("globals")),"Dispatched callback order/pre-state differs");
        check(actual.bank==blob(fixture,event.at("bank")),"Dispatched callback bank pre-state differs");
        equal(actual.game_over,bytes(event.at("game_over_hex")),"Dispatched callback game pre-state differs");
        if(event.contains("phase"))check(actual.phase==event.at("phase"),"Original received phase argument differs");
        if(event.contains("opaque"))check(actual.opaque==event.at("opaque"),"Opaque callback argument was not forwarded");
        if(event.contains("mask"))check(actual.mask==event.at("mask"),"Callback mask value differs");
        if(actual.kind.starts_with("bank_"))++callbacks;else ++shared_callbacks;
      }
      check(index==events.size(),"Native added a dispatched callback");
      if(!expected.at("entity_stores").empty()) {
        const auto& stores=expected.at("entity_stores");check(operation.starts_with("create"),"Unexpected entity store owner");
        check(stores.back().at("value")==entity && stores.back().at("size")==4,"Original entity adoption differs");
        if(stores.size()==2)check(stores.front().at("value")==0xffffffffu,"Original prior entity clear differs");
        ++adoptions;
      }
    }catch(const std::exception&){std::cerr<<version<<" input="<<n<<'\n';throw;}
  }
  missing_services();
  std::cout<<"Original engine entity dispatch: "<<calls<<" completed calls, "<<faults<<" expected absent-engine faults, "
    <<rejections<<" identity rejections, "<<adoptions<<" adoptions, "<<callbacks<<" real bank callbacks, "
    <<shared_callbacks<<" shared services, "<<builds<<" changed-hill rebuilds; provider/return ordering checks pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
