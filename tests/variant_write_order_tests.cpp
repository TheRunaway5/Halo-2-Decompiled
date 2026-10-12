#include "halo2/frontend.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/multiplayer_network.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
using namespace halo2;
namespace {
void check(bool pass,const std::string &message) { if(!pass)throw FormatError(message); }
void choose(MenuState &menu,unsigned row,unsigned screen) {
  menu.select(row);menu.accept();check(menu.frame().screen==screen,"ordered variant menu route changed");
}
unsigned entry(const MenuState &menu,int value) {
  const auto &rows=menu.frame().entries;
  const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto &row){return row.value==value;});
  check(found!=rows.end(),"ordered variant missing from actual menu");
  return unsigned(found-rows.begin());
}
void sign_in(MenuState &menu) {
  menu.accept();check(menu.frame().screen==10,"title did not open profile picker");
  const auto &rows=menu.frame().entries;
  const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto &row){return row.label==menu.profile_name();});
  if(found==rows.end()) {
    choose(menu,0,167);menu.text("Ordered Source");menu.commit_text();
    check(menu.frame().screen==38,"new profile skipped customization");menu.back();
  } else choose(menu,unsigned(found-rows.begin()),6);
  check(menu.frame().screen==6 && !menu.signed_in_profile().empty(),"ordered variant profile failed actual sign-in");
}
void lobby(MenuState &menu,unsigned protocol) {
  sign_in(menu);choose(menu,protocol?3:2,30);choose(menu,0,14);
}
void select_variant(MenuState &menu,unsigned index) {
  choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,0),208);
  choose(menu,entry(menu,int(index)),14);
}
SourceGameVariant from_hex(const std::string &hex) {
  SourceGameVariant v;check(hex.size()==v.bytes.size()*2,"truncated ordered source buffer");
  for(unsigned n=0;n<v.bytes.size();++n)v.bytes[n]=std::uint8_t(std::stoul(hex.substr(n*2,2),nullptr,16));
  return v;
}
void oracle(const char *fixture) {
  std::ifstream file(fixture);check(bool(file),"missing original ordered-write fixture");unsigned rows=0;
  for(std::string line;std::getline(file,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream in(line);char kind;unsigned engine,count;in>>kind>>engine>>count;
    check(kind=='O',"unknown ordered source row");UiVariant v{"Source order",engine,{}};
    GameVariantSettings compact;
    for(unsigned n=0;n<count;++n) {
      unsigned parameter;int value;in>>parameter>>value;
      v.values.append(parameter,value);compact.set(parameter,value);
    }
    std::string hex;int valid;unsigned queries;in>>hex>>valid>>queries;
    const auto expected=from_hex(hex),actual=source_game_variant(v);
    check(actual.bytes==expected.bytes,"ordered setters differ at source row "+std::to_string(rows));
    check(source_game_variant({v.name,engine,compact}).bytes==expected.bytes,
          "same-ID edit coalescing changes chronological source writes");
    check(source_game_variant_valid(actual)==bool(valid),"ordered source validation differs");
    for(unsigned n=0;n<queries;++n) {
      unsigned parameter;int value;in>>parameter>>value;
      check(source_game_variant_get(actual,std::bit_cast<std::int32_t>(std::uint32_t(parameter)))==value,
            "ordered source getter differs");
    }
    check(!in.fail(),"truncated ordered source row");++rows;
  }
  check(rows==3195,"incomplete ordered original source fixture");
  std::cout<<"PASS 3195 complete original retail/TU ordered buffers, getters and validators; 150 overlapping descriptor pairs across seven families\n";
}
struct Temporary {
  std::filesystem::path path=std::filesystem::temp_directory_path()/
      ("halo2-write-order-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Temporary(){check(std::filesystem::create_directory(path),"cannot create disposable source-order directory");}
  ~Temporary(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
void match_udp(const FrontendLaunch &launch,const std::filesystem::path &maps) {
  Cache cache(maps/launch.map);const auto content=cache.load_content(0,true);
  auto rules=slayer_rules(launch.variant);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  check(rules.score_limit==2 && rules.starting_weapon==5 && rules.starting_grenades,
        "actual native rules lost ordered alias/byte/bit values");
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("Source host");
  LanHost host(match,0,0x3195);LanClient client("127.0.0.1:"+std::to_string(host.port()),0x3195,2);
  std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,2> remote{};
  for(unsigned attempt=0;attempt<100 && !client.ready();++attempt) {
    client.send(remote);host.receive(controls);host.broadcast();client.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(client.ready(),"ordered source settings failed actual UDP join");
  for(unsigned tick=0;tick<90;++tick) {
    client.send(remote);host.receive(controls);match.step(controls);host.broadcast();client.receive();
    check(client.state().tick==match.state().tick && client.state().score_limit==2 &&
          client.state().variant_name==rules.name,"native UDP lost ordered source rules/clock");
  }
  // Exercise the existing authoritative score callback and replication owner.
  // This is score-limit flow coverage, not a live weapon-combat playthrough.
  auto death=[&](unsigned victim) {
    auto &unit=const_cast<Unit &>(match.unit(victim));
    unit.vitality.dead=true;unit.last_damager=-1; // Established native non-SMG credit to slot0.
    match.step(controls);
  };
  death(client.slots()[0]);
  check(!match.finished() && match.player(0).score==1,"first callback ignored source score limit2");
  death(client.slots()[1]);host.broadcast();client.receive();
  check(match.finished() && match.state().winner==0 && client.state().finished &&
        client.state().winner==0 && client.state().players[0].score==2,
        "ordered source score limit did not finish/replicate through authoritative callback");
  std::cout<<"PASS actual tag/menu -> selected authored SMG -> three-participant UDP90 ticks and replicated score-callback completion\n";
}
void cache_and_launch(const std::filesystem::path &maps) {
  std::ifstream file(maps/"mainmenu.map",std::ios::binary|std::ios::ate);
  check(bool(file),"original menu cache missing");const auto size=file.tellg();
  std::vector<std::byte> raw(static_cast<std::size_t>(size));file.seekg(0);
  check(bool(file.read(reinterpret_cast<char*>(raw.data()),size)),"short original cache read");
  Bytes bytes(raw);const auto index=bytes.u32(16);
  const auto header_magic=std::int64_t(bytes.u32(index))-std::int64_t(index+32);
  const auto table=std::size_t(std::int64_t(bytes.u32(index+8))-header_magic);
  const auto magic=std::int64_t(bytes.u32(table+8))-std::int64_t(index+bytes.u32(20));
  Cache cache(maps/"mainmenu.map");
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &t){return t.group=="goof";});
  check(tag!=cache.tags().end(),"goof tag missing");
  const auto goof=std::size_t(std::int64_t(tag->address)-magic);
  const auto rows=std::size_t(std::int64_t(bytes.u32(goof+44))-magic);
  check(bytes.u32(goof+40)==39 && bytes.u32(rows+20+8)==2 && bytes.u32(rows+40+8)==5,
        "original ordered tag layout changed");
  const auto two=std::size_t(std::int64_t(bytes.u32(rows+20+12))-magic);
  const auto five=std::size_t(std::int64_t(bytes.u32(rows+40+12))-magic);
  Temporary temporary;const auto path=temporary.path/"mainmenu.map";
  const auto original=raw;
  auto write=[&](std::size_t at,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)raw.at(at+n)=std::byte((value>>(8*n))&255);
  };
  auto load=[&] {
    {std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(raw.data()),raw.size());
      check(bool(out),"cannot save disposable tag data");}
    return Cache(path,maps).load_frontend();
  };
  write(two,2);write(two+4,32768);write(two+8,1);write(two+12,2);
  const auto accepted=load();
  check(accepted.variants.size()==39 && source_game_variant_get(source_game_variant(accepted.variants[1]),2)==2,
        "tag loader sorted aliases before source validation");
  write(two,1);write(two+4,2);write(two+8,2);write(two+12,32768);
  check(load().variants.size()==38,"tag loader admitted reversed source-invalid final write");
  raw=original;
  const GameVariantSettings authored{{2,7},{1,4},{2,2},{89,261},{88,-1}};
  unsigned offset=0;for(const auto &[parameter,value]:authored) {
    write(five+offset,parameter);write(five+offset+4,std::uint32_t(value));offset+=8;
  }
  const auto content=load();
  check(content.variants.size()==39 && content.variants[2].values==authored,
        "actual cache deduplicated/reordered the authored settings block");
  for(unsigned protocol=0;protocol<2;++protocol) {
    MenuState menu(content);lobby(menu,protocol);select_variant(menu,2);
    const auto quick=menu.lobby_view()->quick_options;
    std::map<unsigned,int> getters{{2,2},{89,5},{88,1}};
    check(quick==frontend_lobby_quick_options(content,0,getters,
          content.localized(content.screens.at(14),0x1a001820)),"lobby displays literal writes instead of original getter results");
    menu.select(0);menu.accept();check(menu.launch && menu.launch->variant.values==authored,
          "ordinary signed-in Start lost exact tag override order/duplicates");
    if(!protocol)match_udp(*menu.launch,maps);
  }
  std::cout<<"PASS disposable real-tag alias order, repeated rows, native canonical lobby text and both ordinary Start routes\n";
}
void preferences(const std::filesystem::path &maps) {
  const auto content=Cache(maps/"mainmenu.map").load_frontend();Temporary temporary;
  const auto path=temporary.path/"menu.cfg";
  {std::ofstream file(path);file<<"halo2-native-menu-v1\nactive \"Ordered Source\"\nprofile \"Ordered Source\"\n"
      <<"variant_name \"Ordered Custom\" 0\nvariant 2 10\nvariant 1 2\nvariant 10 32767\n"
      <<"variant 89 261\nvariant 88 -1\nvariant 4294967295 -2147483648\n"
      <<"saved_variant \"Ordered Custom\" 0\nsaved_value \"Ordered Custom\" 2 10\n"
      <<"saved_value \"Ordered Custom\" 1 2\nsaved_value \"Ordered Custom\" 10 32767\n"
      <<"saved_value \"Ordered Custom\" 89 261\nsaved_value \"Ordered Custom\" 88 -1\n"
      <<"saved_value \"Ordered Custom\" 4294967295 -2147483648\n";
    check(bool(file),"cannot create native ordered preference fixture");}
  auto start=[&](MenuState &menu,int score) {
    lobby(menu,0);choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,0),208);
    check(menu.frame().entries.back().label=="Ordered Custom","native saved variant is absent from selection");
    choose(menu,unsigned(menu.frame().entries.size()-1),14);menu.select(0);menu.accept();
    check(menu.launch.has_value(),"native saved ordered variant did not launch");
    const auto rules=slayer_rules(menu.launch->variant);
    check(rules.score_limit==score && rules.time_ticks==983010 && rules.starting_weapon==5 &&
          rules.starting_grenades,"native persistence narrowed/reordered source settings");
  };
  MenuState menu(content,path);start(menu,2);
  const auto parameter=std::find_if(content.parameters.begin(),content.parameters.end(),
                                  [](const auto &p){return p.second.parameter==2;});
  check(parameter!=content.parameters.end(),"authored score selector missing");
  choose(menu,1,23);choose(menu,2,222);
  choose(menu,entry(menu,int(parameter->first)),223);choose(menu,entry(menu,10),222);
  menu.back();check(menu.frame().screen==14,"edited rule did not close the setup channel back to lobby");
  MenuState reloaded(content,path);start(reloaded,10);
  choose(reloaded,1,23);choose(reloaded,2,222);
  choose(reloaded,entry(reloaded,int(parameter->first)),223);
  check(reloaded.frame().entries[reloaded.frame().selected].value==10,
        "editor reads stale alias metadata instead of canonical source getter");
  std::cout<<"PASS native preference read/edit/save/reload through actual signed-in saved-variant selection; full source integer widths and chronological aliases retained\n";
}
}
int main(int argc,char **argv) try {
  check(argc==3,"ordered variant check needs original fixture and map directory");
  const std::filesystem::path maps=argv[2];
  if(!std::filesystem::exists(maps/"mainmenu.map"))return 77;
  oracle(argv[1]);cache_and_launch(maps);preferences(maps);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
