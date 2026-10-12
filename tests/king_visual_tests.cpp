#include "halo2/source_king_visual.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool pass,const std::string& message){if(!pass)throw std::runtime_error(message);}
SourceKingColor color(const std::string& hex) {
  check(hex.size()==24,"Original RGB must preserve twelve bytes");SourceKingColor out{};
  for(unsigned word=0;word<3;++word)for(unsigned byte=0;byte<4;++byte)
    out[word]|=std::uint32_t(std::stoul(hex.substr(word*8+byte*2,2),nullptr,16))<<(byte*8);
  return out;
}
std::string hex(const SourceKingColor& rgb) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto word:rgb)for(unsigned shift=0;shift<32;shift+=8){const auto b=(word>>shift)&255u;out+=digits[b>>4];out+=digits[b&15];}
  return out;
}
std::vector<SourceKingColor> palette(const char* path) {
  std::ifstream file(path);Json data;file>>data;
  check(data.at("group")=="mulg" && data.at("stride")==12,"Palette input is not the literal mulg RGB block");
  std::vector<SourceKingColor> out;
  for(const auto& row:data.at("colors_hex"))out.push_back(color(row));
  return out;
}
SourceKingColorInput input(const Json& row) {
  SourceKingColorInput in;
  // Unoccupied slots are explicitly zero-backed in the original probe.
  for(auto& player:in.players)player.current_unit=0;
  in.team_mode=row.at("team_mode");in.engine_present=row.value("engine",true);
  in.hill_occupants=row.at("occupants");in.render_view=row.at("view");
  const int local=row.at("local_slot");
  in.local_player=local==-1?0xffffffffu:0x9abc0000u+unsigned(local);
  for(const auto& p:row.at("players")) {
    const auto slot=p.at("slot").get<unsigned>();in.occupied_players|=std::uint16_t(1u<<slot);
    in.players.at(slot)={p.value("unit",true)?0x8aaa0000u+slot:0xffffffffu,
      p.at("team").get<std::int8_t>(),p.value("type",std::uint8_t(0))};
  }
  return in;
}
std::vector<std::byte> bytes(const std::string& value) {
  check(!(value.size()&1),"Original byte record has odd length");std::vector<std::byte> out;
  for(unsigned i=0;i<value.size();i+=2)out.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));
  return out;
}
}
int main(int argc,char** argv)try {
  check(argc==3,"Supply original visual report and authored team palette");
  std::ifstream file(argv[1]);Json report;file>>report;
  check(report.at("instruction_patches").empty() && report.at("supplied_returns").empty(),"Original color proof supplies game returns");
  const auto teams=palette(argv[2]);unsigned count{};
  for(const auto& [version,results]:report.at("results").items())
    for(unsigned n=0;n<report.at("color_inputs").size();++n)try {
      const auto& row=report.at("color_inputs")[n];auto in=input(row);
      const int query=row.at("query");
      in.query=query==-1?0xffffffffu:0x9abc0000u+unsigned(query);
      const auto& original=results.at("colors")[n];
      check(original.at("completion")=="complete_original_return" && original.at("stop").is_null(),"Color fixture stopped before return");
      check(hex(source_king_hill_color(in,teams))==original.at("rgb_hex").get<std::string>(),"RGB differs from whole original color owner");++count;
    }catch(const std::exception& error){throw std::runtime_error(version+" color "+std::to_string(n)+": "+error.what());}
  check(std::endian::native==std::endian::little,"Raw original state fixture requires little endian host");
  const auto geometry=bytes(report.at("geometry_hex"));check(geometry.size()==sizeof(SourceKingState),"Original King geometry size changed");
  unsigned marker_count{},handoffs{};
  for(const auto& [version,results]:report.at("results").items())for(const auto& row:results.at("markers"))try {
    SourceKingState state;std::memcpy(&state,geometry.data(),geometry.size());
    const auto& data=row.at("input");state.marker_count=data.at("count");state.occupants=data.at("occupants");
    auto in=input(data);std::array<std::byte,0x90> storage;storage.fill(std::byte{0xa5});unsigned reads{};
    const SourceKingPosition position=[&](std::uint32_t unit){++reads;
      check(unit==0x8aaa0000,"Marker changed original current-unit input");
      return data.at("players")[0].at("position").get<std::array<float,3>>();};
    const bool emitted=source_king_hill_marker(state,0,in.local_player,in,teams,position,storage);
    const bool expected=!row.at("stop").is_null();check(emitted==expected,"Original marker count gate differs");
    if(expected) {
      const auto& stop=row.at("stop");check(stop.at("entry")==report.at("versions").at(version).at("registered_entries").at("marker_projection"),
        "Original marker evidence stopped before a different service");
      const auto original=bytes(stop.at("marker90_hex"));check(original.size()==storage.size(),"Original marker record size changed");
      for(const auto& range:stop.at("defined_ranges"))for(unsigned at=range[0];at<range[1].get<unsigned>();++at)
        check(storage[at]==original[at],"Defined original marker field differs at "+std::to_string(at));
      check(reads==1,"Positive marker omitted actual unit position lookup");++handoffs;
    }else check(reads==0,"Invalid marker gate read current-unit position");
    for(unsigned at=0;at<storage.size();++at)if(!expected || at==2 || at==3 || at>=0x6c)
      check(storage[at]==std::byte{0xa5},"Marker initialized source opaque bytes");
    ++marker_count;
  }catch(const std::exception& error){throw std::runtime_error(version+" marker "+std::to_string(marker_count)+": "+error.what());}
  // Datum salts do not affect this source raw-consumer; missing raw slots do.
  SourceKingColorInput raw;raw.render_view=0;raw.local_player=0x1111000f;
  raw.players[15].appearance_type=3;
  const auto first=source_king_hill_color(raw,teams);raw.local_player=0xaaaa000f;
  check(first==source_king_hill_color(raw,teams),"Color owner incorrectly validates raw datum salt");
  raw.query=16;bool rejected=false;
  try{(void)source_king_hill_color(raw,teams);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"Absent raw player memory became a successful hill color");
  std::cout<<count<<" whole retail/TU1.1/TU1.5 King RGB results matched using authored mulg team colors\n";
  std::cout<<marker_count<<" original marker calls matched, including "<<handoffs<<" defined record prefixes at the real projection handoff\n";
  std::cout<<"LIMIT native hill/world/marker rendering and normal King admission remain separate\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
