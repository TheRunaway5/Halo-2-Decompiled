#include "halo2/source_game_variant.hpp"
#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool pass,const std::string &message) { if(!pass)throw FormatError(message); }
SourceGameVariant buffer(const std::string &hex) {
  SourceGameVariant result;
  check(hex.size()==result.bytes.size()*2,"invalid source normalization buffer length");
  for(unsigned n=0;n<result.bytes.size();++n)
    result.bytes[n]=std::uint8_t(std::stoul(hex.substr(n*2,2),nullptr,16));
  return result;
}
std::u16string title(const SourceGameVariant &v) {
  std::u16string result;
  for(unsigned n=0;n<32;++n) {
    const auto unit=std::uint16_t(v.bytes[4+n*2] | (std::uint16_t(v.bytes[5+n*2]) << 8));
    if(!unit)break;
    result += char16_t(unit);
  }
  return result;
}
void oracle(const FrontendContent &content,const std::filesystem::path &fixture) {
  std::ifstream file(fixture);check(bool(file),"missing original normalization fixture");
  unsigned defaults=0,rows=0;std::u16string reset_title;
  constexpr std::array<std::uint32_t,7> names{0x0e000780,0x0f000781,0x12000783,
      0x0c000782,0x0b00077f,0x0f000785,0x13000784};
  for(std::string line;std::getline(file,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream in(line);char kind;in>>kind;
    if(kind=='D') {
      unsigned engine;std::string hex;in>>engine>>hex;
      const auto expected=buffer(hex);const auto name=title(expected);
      if(engine==0)reset_title=name;
      check(source_game_variant_default(engine,name).bytes==expected.bytes,
            "loaded English source default differs for family"+std::to_string(engine));
      check(source_game_variant_title(expected)==name && !name.empty(),"source UTF16 title getter differs");
      const auto p=content.variant_strings.find(names.at(engine));
      check(p!=content.variant_strings.end(),"cache lost original default-name string ID");
      check(p->second==std::string(name.begin(),name.end()),"native cache default-name data differs from original lookup");
      ++defaults;
    } else if(kind=='N') {
      int locale,valid;std::string label,before,after;in>>locale>>label>>before>>after>>valid;
      auto v=buffer(before);const auto original=v;const auto expected=buffer(after);
      check(source_game_variant_valid(v)==bool(valid) && v.bytes==original.bytes,
            "copy validation differs or mutates caller: "+label);
      check(source_game_variant_normalize(v,locale?reset_title:std::u16string{})==bool(valid) &&
            v.bytes==expected.bytes,"complete source reset/preservation differs: "+label);
      ++rows;
    } else throw FormatError("unknown normalization fixture row");
    check(!in.fail(),"truncated original normalization fixture");
  }
  check(defaults==7 && rows==4249,"incomplete original normalization fixture");
  std::cout<<"PASS 4249 complete retail/TU validity/reset buffers and 7 literal localized defaults\n";
}
void runtime() {
  const auto wide=slayer_rules({"Source Ranges",0,{{2,32767},{10,32767},{17,32767},{18,32767}}});
  check(wide.score_limit==32767 && wide.time_ticks==983010 && wide.respawn_ticks==983010 &&
        wide.suicide_ticks==983010,"native range restriction overrides source scalar contract");
  check(slayer_rules({"Narrowed Weapon",0,{{89,261}}}).starting_weapon==5,
        "native weapon admission ignored original byte narrowing");
  check(slayer_rules({"Nonzero Bit",0,{{88,-1}}}).starting_grenades,
        "native grenade admission ignored source nonzero bit setter");
  check(slayer_rules({"Field Alias",0,{{92,0},{30,1}}}).death_loss,
        "native field alias failed to preserve ordered source writes");
  const auto alias=slayer_rules({"Field Alias",0,{{92,1}}});
  check(alias.death_loss,"native capability gate rejects original same-field alias");
  const auto ignored=slayer_rules({"Absent Descriptor",0,{{112,1},{0xffffffffu,1}}});
  check(ignored.score_limit==25,"absent source descriptor changed or rejected native settings");
  auto rejects=[](UiVariant variant) {
    try{(void)slayer_rules(variant);}catch(const FormatError&){return;}
    throw FormatError("native admission ignored source-invalid buffer");
  };
  rejects({"Bad Score",0,{{2,32768}}});
  rejects({"Bad Weapon Byte",0,{{89,255}}});
  rejects({"Unfinished Sensor",0,{{20,0}}});
  std::cout<<"PASS native canonical fields, source numeric ranges, aliases, bit/byte writes and retained unsupported gates\n";
}
struct TemporaryMap {
  std::filesystem::path directory;
  explicit TemporaryMap() {
    directory=std::filesystem::temp_directory_path()/
        ("halo2-variant-data-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    check(std::filesystem::create_directory(directory),"cannot create disposable variant map directory");
  }
  ~TemporaryMap(){std::error_code ignored;std::filesystem::remove_all(directory,ignored);}
};
void cache_contract(const std::filesystem::path &maps,const FrontendContent &content) {
  std::ifstream file(maps/"mainmenu.map",std::ios::binary|std::ios::ate);
  check(bool(file),"cannot open original menu map");
  const auto size=file.tellg();check(size>0,"empty original map");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);
  check(bool(file.read(reinterpret_cast<char*>(bytes.data()),size)),"short original map read");
  const Bytes data(bytes);const auto index=data.u32(16);
  const auto header_magic=std::int64_t(data.u32(index))-std::int64_t(index+32);
  const auto table=std::size_t(std::int64_t(data.u32(index+8))-header_magic);
  const auto metadata_magic=std::int64_t(data.u32(table+8))-std::int64_t(index+data.u32(20));
  Cache original(maps/"mainmenu.map");
  const auto tag=std::find_if(original.tags().begin(),original.tags().end(),
                             [](const auto &t){return t.group=="goof";});
  check(tag!=original.tags().end(),"goof tag missing");
  const auto root=std::size_t(std::int64_t(tag->address)-metadata_magic);
  check(data.u32(root+40)==39,"authored variant count changed");
  const auto entries=std::size_t(std::int64_t(data.u32(root+44))-metadata_magic);
  const auto first=entries;
  unsigned edited=0;
  while(edited<39 && !data.u32(entries+edited*0x14+8))++edited;
  check(edited<39,"authored variants have no source overrides");
  const auto settings=std::size_t(std::int64_t(data.u32(entries+edited*0x14+12))-metadata_magic);
  const auto original_bytes=bytes;
  TemporaryMap temporary;
  const auto path=temporary.directory/"mainmenu.map";
  auto write=[&](std::size_t offset,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)bytes.at(offset+n)=std::byte((value>>(n*8))&255);
  };
  auto load=[&] {
    {std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
      check(bool(output),"cannot write disposable variant map");}
    return Cache(path,maps).load_frontend();
  };
  write(first+4,7);
  check(load().variants.size()==38,"native tag loader admitted an invalid authored engine type");
  bytes=original_bytes;write(settings,2);write(settings+4,32768);
  const auto omitted=load();
  check(omitted.variants.size()==38 && std::none_of(omitted.variants.begin(),omitted.variants.end(),
      [&](const auto &v){return v.name==content.variants.at(edited).name;}),
      "native tag loader admitted a source-invalid authored settings buffer");
  bytes=original_bytes;write(settings,112);write(settings+4,1);
  check(load().variants.size()==39,"native tag loader rejected a no-op source descriptor");
  std::cout<<"PASS actual cache/tag admission using disposable engine, scalar and descriptor mutations\n";
}
}
int main(int argc,char **argv) {
  try {
    check(argc==3,"normalization check needs maps and source fixture");
    const std::filesystem::path maps(argv[1]);
    if(!std::filesystem::exists(maps/"mainmenu.map"))return 77;
    Cache cache(maps/"mainmenu.map");const auto content=cache.load_frontend();
    oracle(content,argv[2]);runtime();cache_contract(maps,content);return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
