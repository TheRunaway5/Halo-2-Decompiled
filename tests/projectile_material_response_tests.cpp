#include "halo2/projectile_material_response.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
struct Input {
  std::vector<std::byte> bytes;
  std::size_t cursor{};
  explicit Input(const char* path) {
    std::ifstream file(path,std::ios::binary);
    const std::string data((std::istreambuf_iterator<char>(file)),{});
    bytes.resize(data.size());std::memcpy(bytes.data(),data.data(),data.size());
  }
  std::span<const std::byte> take(std::size_t count) {
    if(cursor>bytes.size()||count>bytes.size()-cursor)throw FormatError("Truncated projectile response fixture");
    const auto result=std::span(bytes).subspan(cursor,count);cursor+=count;return result;
  }
  template<class T>T get(){static_assert(std::endian::native==std::endian::little);
    T result;const auto data=take(sizeof(T));std::memcpy(&result,data.data(),sizeof(T));return result;}
  void magic(const char* expected){if(std::memcmp(take(4).data(),expected,4))throw FormatError("Invalid projectile fixture schema");}
  void finish(){if(cursor!=bytes.size())throw FormatError("Trailing projectile fixture data");}
};
}
int main(int argc,char**argv){try{
  if(argc!=3||!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
  if(std::fesetround(FE_TONEAREST))throw FormatError("Original projectile rounding mode unavailable");
  Input materials(argv[1]);materials.magic("PMS1");
  const auto parent_count=materials.get<std::uint32_t>(),bank_count=materials.get<std::uint32_t>();
  std::vector<std::int16_t> parents(parent_count);for(auto&v:parents)v=materials.get<std::int16_t>();
  std::vector<std::vector<std::byte>> banks;
  for(unsigned i=0;i<bank_count;++i){const auto size=materials.get<std::uint32_t>();const auto row=materials.take(size);banks.emplace_back(row.begin(),row.end());}
  const auto lookup_count=materials.get<std::uint32_t>();
  for(unsigned i=0;i<lookup_count;++i){
    const auto bank=materials.get<std::uint32_t>();const auto material=materials.get<std::int16_t>();const auto expected=materials.get<std::int32_t>();
    const auto result=source_projectile_material_response(banks.at(bank),parents,material);
    if((result?std::int64_t(*result):-1)!=expected)throw FormatError("Original projectile material lookup differs at case "+std::to_string(i));
  }
  materials.finish();
  Input choices(argv[2]);choices.magic("PRS1");
  const auto one=choices.get<float>(),scale=choices.get<float>();const auto choice_count=choices.get<std::uint32_t>();
  unsigned rng_calls{},type_calls{};
  for(unsigned i=0;i<choice_count;++i){
    const auto row=choices.take(0x58);const auto collision=choices.get<std::int32_t>();const auto type=choices.get<std::uint8_t>();
    const auto angle=choices.get<float>(),speed=choices.get<float>();auto rng=choices.get<std::uint32_t>();
    const auto expected_rng=choices.get<std::uint32_t>();const auto expected=choices.get<std::int16_t>();
    const auto result=choose_source_projectile_response(row,{collision,angle,speed},{one,scale,
      [&]{++type_calls;return type;},[&]() ->std::uint32_t& {++rng_calls;return rng;}});
    if(result!=expected||rng!=expected_rng)throw FormatError("Original projectile response/RNG differs at case "+std::to_string(i));
  }
  choices.finish();
  // The actual plasma row's unit-only alternate response cannot ask for a
  // header or random stream on a terrain hit. A unit impact needs its owner.
  const auto plasma=std::span<const std::byte>(banks.at(1)).first(0x58);
  if(choose_source_projectile_response(plasma,{1,{},{}},{one,scale,{},{}})!=4)
    throw FormatError("Actual plasma terrain response is not the original bounce");
  bool rejected{};try{choose_source_projectile_response(plasma,{4,{},{}},{one,scale,{},{}});}catch(const FormatError&){rejected=true;}
  if(!rejected)throw FormatError("Missing unit target type owner was fabricated");
  if(lookup_count!=376||choice_count<3066||!rng_calls||!type_calls)throw FormatError("Incomplete original projectile fixture bank");
  std::cout<<"PASS "<<lookup_count<<" material lookups and "<<choice_count<<" response/RNG choices, independently measured in both update images\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
