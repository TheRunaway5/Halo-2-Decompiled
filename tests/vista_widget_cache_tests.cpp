#include "halo2/vista_widget_cache.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string_view>

using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> load(const char* path) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);
  if(!in || in.tellg()<0)throw FormatError("Missing actual Vista widget test cache");
  std::vector<std::byte> data(std::size_t(in.tellg()));in.seekg(0);
  if(!in.read(reinterpret_cast<char*>(data.data()),std::streamsize(data.size())))
    throw FormatError("Short actual Vista widget test cache read");
  return data;
}
std::vector<std::byte> unhex(std::string_view raw) {
  const auto digit=[](char c)->unsigned {
    if(c>='0'&&c<='9')return unsigned(c-'0');
    if(c>='a'&&c<='f')return unsigned(c-'a')+10;
    throw FormatError("Invalid original Vista widget cache hex");
  };
  if(raw.size()%2)throw FormatError("Invalid original cache hex extent");
  std::vector<std::byte> data(raw.size()/2);
  for(std::size_t i=0;i<data.size();++i)data[i]=std::byte{std::uint8_t(digit(raw[i*2])*16+digit(raw[i*2+1]))};
  return data;
}
std::uint32_t little(std::span<const std::byte> data) {
  std::uint32_t value{};
  for(std::size_t i=0;i<data.size();++i)value|=std::uint32_t(std::to_integer<unsigned>(data[i]))<<(8*i);
  return value;
}
std::string fnv(std::span<const std::byte> data) {
  std::uint64_t value=14695981039346656037ull;
  for(const auto b:data)value=(value^std::to_integer<std::uint8_t>(b))*1099511628211ull;
  constexpr char digits[]="0123456789abcdef";std::string result(16,'0');
  for(std::size_t i=0;i<16;++i)result[15-i]=digits[(value>>(i*4))&15];
  return result;
}
template<class F> void rejects(F&& f) {
  try{f();}catch(const FormatError&){return;}
  throw FormatError("Invalid Vista widget cache boundary was admitted");
}
void same(std::span<const std::byte> actual,std::span<const std::byte> expected) {
  if(actual.size()!=expected.size() || !std::equal(actual.begin(),actual.end(),expected.begin()))
    throw FormatError("Retained Vista widget cache bytes differ from actual source");
}
void store(std::span<std::byte> data,std::size_t at,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)data[at+i]=std::byte{std::uint8_t(value>>(i*8))};
}
} // namespace

int main(int argc,char** argv) try {
  if(argc!=3)return 77;
  std::ifstream oracle(argv[2]);if(!oracle)return 77;Json fixture;oracle>>fixture;
  if(fixture.at("schema")!="H2_VISTA_WIDGET_CACHE_ORIGINAL_V1" ||
      fixture.at("map").at("sha256")!="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44")
    throw FormatError("Original Vista widget cache authority changed");
  std::vector<std::byte> expected_region,expected_header;
  std::shared_ptr<const VistaWidgetCacheBank> bank;
  std::size_t checks{},leaf_checks{},frame_count{},nonzero_frame_words{};
  {
    auto source=load(argv[1]);
    if(source.size()!=61063680 || fnv(source)!=fixture.at("map").at("fnv64").get<std::string>())
      throw FormatError("Actual Vista mainmenu input differs from pinned original");
    expected_header.assign(source.begin(),source.begin()+2048);
    expected_region.assign(source.begin()+55558656,source.end());
    bank=std::make_shared<const VistaWidgetCacheBank>(source);
    // Source storage is deliberately overwritten before destruction. The bank
    // must own the entire raw region rather than borrow PcCache.storage.
    std::fill(source.begin(),source.end(),std::byte{0xa5});
  }
  same(bank->file_header(),expected_header);same(bank->region(),expected_region);checks+=2;
  if(bank->source_index()!=55558656 || bank->source_file_size()!=61063680 || bank->region().size()!=5505024 ||
      bank->tag_count()!=1504 || bank->scenario_datum()!=0xe1790003u || bank->globals_datum()!=0xe1750000u ||
      fnv(bank->region())!=fixture.at("region").at("fnv64").get<std::string>())
    throw FormatError("Original Vista widget cache directory changed");
  ++checks;
  same(bank->index_header(),unhex(fixture.at("index_header").at("raw_hex").get<std::string>()));++checks;
  same(bank->tag_table(),unhex(fixture.at("tag_table").at("raw_hex").get<std::string>()));++checks;
  for(std::uint32_t i=0;i<bank->tag_count();++i) {
    same(bank->directory_row(i),bank->tag_table().subspan(i*16,16));++checks;
  }
  for(const auto& group:fixture.at("groups")) {
    const auto base=group.at("base").get<std::uint32_t>();const auto pointers=bank->loaded_pointers(base);
    Json actual=Json::array();
    for(const auto& [at,value]:pointers.bindings()) {
      actual.push_back(Json::array({at,value}));if(pointers.value(at)!=value)throw FormatError("Loaded-cache projection changed");++checks;
    }
    if(actual!=group.at("pointers"))throw FormatError("Declared loaded-cache source projections differ");
    ++checks;
    for(const auto& row:group.at("tag_results")) {
      const auto ordinal=row.at(0).get<std::uint32_t>();
      const auto raw=bank->directory_row(ordinal);const auto offset=little(raw.subspan(8,4));const auto size=little(raw.subspan(12,4));
      for(unsigned alias=0;alias<2;++alias) {
        const auto datum=row.at(alias?2:1).get<std::uint32_t>();
        for(const auto& result:row.at(3).at(alias)) {
          if(bank->tag_address(base,datum)!=result.get<std::uint32_t>())throw FormatError("Original low-WORD tag leaf differs");
          ++leaf_checks;
        }
        same(bank->tag_metadata(base,datum),std::span<const std::byte>(expected_region).subspan(offset,size));++checks;
      }
    }
    for(const auto& row:group.at("block_results")) {
      const auto at=row.at(0).get<std::uint32_t>();
      if(bank->block_address(base,base+at)!=row.at(1).get<std::uint32_t>())throw FormatError("Original40809B block relocation differs");
      ++leaf_checks;
    }
    same(bank->bytes(base,base,bank->region().size()),expected_region);++checks;
    if(bank->read(base,base+std::uint32_t(bank->region().size())-1,1)!=std::to_integer<unsigned>(expected_region.back()))
      throw FormatError("Last retained source byte changed");
    ++checks;
    rejects([&]{(void)bank->bytes(base,base,std::numeric_limits<std::size_t>::max());});++checks;
    rejects([&]{(void)bank->read(base,base,0);});++checks;
    rejects([&]{(void)bank->read(base,base,3);});++checks;
    rejects([&]{(void)bank->bytes(base,base-1,1);});++checks;
    rejects([&]{(void)bank->source_address(base,std::uint32_t(bank->region().size()),1);});++checks;
  }
  const auto check_snapshot=[&](const Json& row) {
    const auto offset=row.at("relative_offset").get<std::uint32_t>();const auto raw=unhex(row.at("raw_hex").get<std::string>());
    same(bank->bytes(0x3200000u,0x3200000u+offset,raw.size()),raw);++checks;
  };
  const auto track=[&](const Json& row) {
    check_snapshot(row.at("header"));
    for(const auto& frame:row.at("frames")) {
      check_snapshot(frame);const auto at=frame.at("relative_offset").get<std::uint32_t>();
      // Full source stride20 includes leading DWORD omitted by semanticUiKey.
      const auto raw=unhex(frame.at("raw_hex").get<std::string>());
      if(frame.at("bytes")!=20 || bank->read(0x3200000u,0x3200000u+at,4)!=little(std::span<const std::byte>(raw).first(4)))
        throw FormatError("Original frame leading DWORD was dropped");
      nonzero_frame_words+=bank->read(0x3200000u,0x3200000u+at,4)!=0;++frame_count;++checks;
    }
  };
  if(fixture.at("animations").size()!=59)throw FormatError("Actual animation record count changed");
  for(const auto& row:fixture.at("animations")){check_snapshot(row.at("record"));for(const auto& t:row.at("tracks"))track(t);}
  for(const auto& skin:fixture.at("skins"))for(const auto& row:skin.at("records")){check_snapshot(row.at("record"));track(row.at("track"));}
  if(frame_count!=fixture.at("summary").at("frame_records"))throw FormatError("Actual raw frame inventory changed");
  ++checks;
  auto invalid=load(argv[1]);
  for(const auto& [at,value]:std::array<std::pair<std::size_t,std::uint32_t>,10>{{
      {0,0},{4,7},{8,2048},{2044,0},{16,0},{16,0xffffffffu},{0x12c,0},
      {55558656+24,0},{55558656+24,65537},{55558656+8,0xffffffffu}}}) {
    const auto old=little(std::span<const std::byte>(invalid).subspan(at,4));store(invalid,at,value);
    rejects([&]{VistaWidgetCacheBank rejected(invalid);});store(invalid,at,old);++checks;
  }
  rejects([&]{VistaWidgetCacheBank rejected(std::span<const std::byte>(invalid).first(2047));});++checks;
  rejects([&]{VistaWidgetCacheBank rejected(std::span<const std::byte>(invalid).first(invalid.size()-1));});++checks;
  rejects([&]{(void)bank->loaded_pointers(0xffac0001u);});++checks;
  rejects([&]{(void)bank->bytes(0xffac0000u,0xffffffffu,2);});++checks;
  rejects([&]{(void)bank->source_address(0xffac0000u,5505024,0);});++checks;
  rejects([&]{(void)bank->block_address(0xffac0000u,0xfffffffcu);});++checks;
  rejects([&]{(void)bank->directory_row(1504);});++checks;
  rejects([&]{(void)bank->loaded_pointers(0x3200000u).value(0x879e78u);});++checks;
  std::cout<<"PASS: "<<bank->region().size()<<" exact region bytes, "<<bank->tag_count()<<" tag rows, "
      <<frame_count<<" raw frames ("<<nonzero_frame_words<<" nonzero leading DWORDs), "<<leaf_checks
      <<" original leaf comparisons, "<<checks<<" data/boundary checks\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
