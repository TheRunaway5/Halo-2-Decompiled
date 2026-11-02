#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include "halo2/script_null_global.hpp"
#include <bit>
#include <iostream>
#include <vector>
using namespace halo2;
namespace {
void word(std::span<std::byte> bytes, unsigned at, std::uint32_t value, unsigned count = 4) {
  for (unsigned n = 0; n < count; ++n) bytes[at+n] = std::byte(value >> (n*8));
}
}
int main() { try {
  // The actual Earth City call reduced to its full salted parent/name/boolean
  // syntax identities. Changing unrelated engine startup cannot make this pass.
  const std::string name = "rasterizer_bloom_override_silver_bullet";
  std::vector<std::byte> strings;
  for (const char c : name) strings.push_back(std::byte(c));
  strings.push_back(std::byte{});
  std::vector<std::byte> nodes(60);
  word(nodes,0,0x9000,2); word(nodes,2,0x36c,2); word(nodes,4,4,2); word(nodes,6,8,2);
  word(nodes,8,0xffffffffu); word(nodes,16,0x90010001u);
  word(nodes,20,0x9001,2); word(nodes,22,2,2); word(nodes,24,2,2); word(nodes,26,9,2);
  word(nodes,28,0x90020002u);
  word(nodes,40,0x9002,2); word(nodes,42,5,2); word(nodes,44,5,2); word(nodes,46,9,2);
  word(nodes,48,0xffffffffu); word(nodes,56,0xffffff00u);
  const auto layout = project_mcc_script_layout(nodes,strings,{});
  layout.require_native_ready();
  if (Bytes(layout.nodes).u16(2) != 0x380)
    throw FormatError("Earth City bloom command did not select its exact TU1.5 boolean evaluator");
  for(const auto [name,donor,target,type]:std::vector<std::tuple<std::string,unsigned,unsigned,unsigned>>{
      {"ssc_light_diffuse",203,211,6},{"global_playtest_mode",991,1011,5}}) {
    std::vector<std::byte> text;
    for(char c:name)text.push_back(std::byte(c));text.push_back(std::byte{});
    std::vector<std::byte> global(20);
    word(global,0,0x9000,2);word(global,4,type,2);word(global,6,5,2);
    word(global,8,0xffffffffu);word(global,16,0x12348000u|donor);
    const auto converted=project_mcc_script_layout(global,text,{}, {.convert_external_globals=true});
    converted.require_native_ready();
    if(Bytes(converted.nodes).u32(16)!=(0x12348000u|target))
      throw FormatError("Workshop null global lost its index or compiler bits");
    const ScriptValue input{std::uint16_t(type),type==6?std::bit_cast<unsigned>(.6f):1u,{}};
    if(source_null_campaign_global(target,input)!=input ||
        source_null_campaign_global(target,std::nullopt)!=ScriptValue{std::uint16_t(type),0,{}})
      throw FormatError("Workshop null global stored a value or lost its assignment result");
  }
  std::cout << "Earth City bloom script admission passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; } }
