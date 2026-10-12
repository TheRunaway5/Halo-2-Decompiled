#include "halo2/vista_lobby_status.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::array<std::int16_t,8> indices{-1,1,2,45,52,53,54,55};
void put32(VistaLobbyStatusRecord& r,unsigned at,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)r.bytes[at+i]=std::uint8_t(value>>(8*i));
}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream input(argv[1]);Json rows;input>>rows;unsigned count{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");const auto op=q.at("op").get<std::string>();
    const auto byte=[&](const char* key){return q.at(key).get<std::uint8_t>();};
    const auto integer=[&](const char* key){return q.at(key).get<std::int32_t>();};
    const auto uint=[&](const char* key){return q.at(key).get<std::uint32_t>();};
    VistaLobbyStatusRecord record;VistaLobbyStatusPeers peers;peers.mask=uint("mask");
    for(unsigned i=0;i<record.bytes.size();++i)record.bytes[i]=std::uint8_t(i*37+uint("seed")*13);
    put32(record,4,uint("kind"));put32(record,0x88,uint("error"));
    for(unsigned i=0;i<16;++i)for(unsigned j=0;j<8;++j)peers.identities[i][j]=std::uint8_t(i*17+j*29+uint("seed"));
    for(unsigned k=0;k<2;++k) {
      const auto match=integer(k?"match1":"match0");const auto offset=k?0x8cu:8u;
      if(match<0)std::fill_n(record.bytes.begin()+offset,8,std::uint8_t(k?254:255));
      else std::copy(peers.identities[unsigned(match)].begin(),peers.identities[unsigned(match)].end(),record.bytes.begin()+offset);
    }
    if(integer("duplicate")>=0)std::copy_n(record.bytes.begin()+8,8,peers.identities[unsigned(integer("duplicate"))].begin());
    VistaLobbyStatusState state{std::int16_t(integer("pane")),byte("dialog"),integer("previous_kind")};
    std::array<VistaLobbyStatusText,8> texts;
    for(unsigned i=0;i<texts.size();++i)texts[i]={q.at("visible").at(i),q.at("bounds").at(i).get<VistaLobbyLayoutRect>(),q.at("buffer_flags").at(i)};
    const auto index=[&](const VistaLobbyStatusText& t){return indices[std::size_t(&t-texts.data())];};
    Json events=Json::array(),tuple_out,success;
    VistaLobbyStatusQueryServices query;
    query.record=[&]{events.push_back({"record",q.at("available"),q.at("session_nonzero")});
      return VistaLobbyStatusRecordSource{std::uint8_t(byte("available") && uint("session_nonzero")),&record};};
    query.peers=[&]{events.push_back({"peers"});return peers;};
    VistaLobbyStatusServices s;
    s.text=[&](std::int16_t id)->VistaLobbyStatusText* {
      const auto it=std::find(indices.begin(),indices.end(),id);
      if(it==indices.end())throw FormatError("Unexpected source runtime text index");
      const auto i=std::size_t(it-indices.begin());events.push_back({"text",id,q.at("text_exists").at(i)});
      return q.at("text_exists").at(i).get<unsigned>()?&texts[i]:nullptr;
    };
    s.set_string=[&](auto& t,auto sid){events.push_back({"set",index(t),sid});};
    s.append_string=[&](auto& t,auto sid){events.push_back({"append",index(t),sid});};
    s.clear_string=[&](auto& t){events.push_back({"clear",index(t)});};
    s.set_raw=[&](auto& t,const auto& value){
      if(value!=u"original controlled custom string")throw FormatError("Unexpected controlled string");
      events.push_back({"raw",index(t),"original controlled custom string"});};
    s.refresh_button_key=[&]{events.push_back({"refresh"});};
    s.countdown_active=[&]{events.push_back({"active",q.at("active")});return byte("active");};
    s.favorite=[&]{events.push_back({"favorite",q.at("favorite")});return byte("favorite");};
    s.identity=[&](auto& id){events.push_back({"identity",id[0],id[1],q.at("identity")});id={0x12345678,0x9abcdef0};return byte("identity");};
    s.favorite_identity=[&](const auto& id){if(id!=std::array<std::uint32_t,2>{0x12345678,0x9abcdef0})throw FormatError("Original identity not forwarded");
      events.push_back({"favorite_identity",q.at("favorite_identity")});return byte("favorite_identity");};
    s.multiplayer=[&]{events.push_back({"multiplayer",q.at("multiplayer")});return byte("multiplayer");};
    s.query=[&](auto& out){events.push_back({"query"});return vista_lobby_status_tuple(out,query);};
    s.readiness=[&]{events.push_back({"readiness",q.at("readiness")});return integer("readiness");};
    s.reset_dialog=[&]{events.push_back({"reset_dialog",q.at("reset_dialog")});return byte("reset_dialog");};
    s.status7_dialog=[&](auto& st){events.push_back({"status7_dialog"});st.dialog_shown=byte("dialog_result");};
    s.status_string=[&](auto code){events.push_back({"status_string",code,q.at("status_sid")});return uint("status_sid");};
    s.custom_map=[&](auto identity){events.push_back({"custom_map",identity,q.at("custom_map")});return byte("custom_map");};
    s.custom_text=[&](auto first,auto second)->std::optional<std::u16string>{events.push_back({"custom_text",first,second,q.at("custom_text")});
      return byte("custom_text")?std::optional<std::u16string>{u"original controlled custom string"}:std::nullopt;};
    s.count=[&]{events.push_back({"count",q.at("count")});return integer("count");};
    s.maximum=[&]{events.push_back({"maximum",q.at("maximum")});return integer("maximum");};
    s.overflow_string=[&]{events.push_back({"overflow_string",q.at("overflow_sid")});return uint("overflow_sid");};
    s.user=[&]{events.push_back({"user",q.at("user")});return integer("user");};
    s.user_ready=[&](auto user){events.push_back({"user_ready",user,q.at("user_ready")});return byte("user_ready");};
    s.state2=[&]{events.push_back({"state2",q.at("state2")});return byte("state2");};
    s.campaign=[&]{events.push_back({"campaign",q.at("campaign")});return byte("campaign");};
    s.state9=[&]{events.push_back({"state9",q.at("state9")});return byte("state9");};
    s.team_change=[&]{events.push_back({"team_change",q.at("team_change")});return byte("team_change");};
    s.map_team_limit=[&]{events.push_back({"map_team_limit",q.at("map_team_limit")});return integer("map_team_limit");};
    s.screen_a0=[&](auto v){events.push_back({"screen_a0",v});};
    s.factory_flag=[&]{events.push_back({"factory",q.at("factory")});return byte("factory");};
    if(op=="tuple") {
      VistaLobbyStatusTuple out{-7,-8,nullptr,nullptr,-9,-10,0x1111,0x2222,0x3333,0x44444444};
      events.push_back({"query"});success=unsigned(vista_lobby_status_tuple(out,query));
      tuple_out=Json::array({out.field04,out.peer08,out.field08?Json(out.field08-record.bytes.data()):Json(),
          out.field48?Json(out.field48-record.bytes.data()):Json(),out.field88,out.peer8c,out.field94,out.field96,out.field98,out.field9c});
      std::array<std::byte,0x73a4> bank{};
      for(unsigned i=0;i<record.bytes.size();++i)bank[0x4ae0+i]=std::byte(record.bytes[i]);
      for(unsigned i=0;i<4;++i) {
        bank[0x73a0+i]=std::byte(uint("session_nonzero")>>(8*i));bank[0x1258+i]=std::byte(peers.mask>>(8*i));
      }
      for(unsigned i=0;i<16;++i)for(unsigned j=0;j<8;++j)bank[0x125c+i*0x128+j]=std::byte(peers.identities[i][j]);
      VistaLobbyStatusRecord retained;retained.bytes.fill(0x55);
      VistaLobbyStatusTuple from_bank{-7,-8,nullptr,nullptr,-9,-10,0x1111,0x2222,0x3333,0x44444444};
      const auto ok=vista_lobby_status_session_tuple(bank,byte("available"),retained,from_bank);
      const auto bank_output=Json::array({from_bank.field04,from_bank.peer08,from_bank.field08?Json(from_bank.field08-retained.bytes.data()):Json(),
          from_bank.field48?Json(from_bank.field48-retained.bytes.data()):Json(),from_bank.field88,from_bank.peer8c,from_bank.field94,from_bank.field96,from_bank.field98,from_bank.field9c});
      if(Json(unsigned(ok))!=success || bank_output!=tuple_out ||
          (ok?retained.bytes!=record.bytes:std::any_of(retained.bytes.begin(),retained.bytes.end(),[](auto v){return v!=0x55;})))
        throw FormatError("Original session-bank status adapter mismatch");
    } else if(op=="caption")vista_lobby_button_key(byte("variant_present")?std::optional{byte("variant48")}:std::nullopt,byte("host"),s);
    else vista_lobby_status(state,byte("host"),uint("stack_seed"),s);
    Json actual={{"visible",Json::array()},{"bounds",Json::array()},{"buffer_flags",Json::array()},
      {"dialog",state.dialog_shown},{"previous_kind",state.previous_kind}};
    for(const auto& t:texts){actual["visible"].push_back(t.visible);actual["bounds"].push_back(t.bounds);actual["buffer_flags"].push_back(t.buffer_flags);}
    Json expected_events=Json::array();for(const auto& e:row.at("events"))if(e.at(0)!="bounds")expected_events.push_back(e);
    if(events!=expected_events || actual!=row.at("state") || tuple_out!=row.at("tuple") || success!=row.at("success")) {
      std::cerr<<"case="<<count<<" input="<<q.dump()<<"\nexpected="<<row.dump()<<"\nactual="<<actual.dump()<<"\nevents="<<events.dump()
        <<" tuple="<<tuple_out.dump()<<" success="<<success.dump()<<'\n';throw FormatError("Original Vista lobby status mismatch");
    }
    ++count;
  }
  if(count!=4587)throw FormatError("Incomplete original Vista lobby status corpus");
  bool missing{};try{VistaLobbyStatusTuple out;vista_lobby_status_tuple(out,{});}catch(const FormatError&){missing=true;}
  if(!missing)throw FormatError("Missing consumed query service accepted");
  std::cout<<"PASS original Vista lobby status cases="<<count<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
