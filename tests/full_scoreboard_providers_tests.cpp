#include "halo2/full_scoreboard_providers.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks=0;
void check(bool ok,const char* message) {++checks;if(!ok)throw std::runtime_error(message);}
std::vector<std::uint8_t> bytes(const std::string& s) {
  check(s.size()%2==0,"invalid original hex");std::vector<std::uint8_t> out;
  for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::uint8_t(std::stoul(s.substr(i,2),nullptr,16)));
  return out;
}
std::u16string wide(const std::string& s) {
  auto b=bytes(s);check(b.size()%2==0,"invalid original UTF16");std::u16string out;
  for(std::size_t i=0;i<b.size();i+=2)out.push_back(char16_t(b[i]|unsigned(b[i+1])<<8));
  return out;
}
FullScoreboardColorBits color(const std::string& s) {
  const auto b=bytes(s);check(b.size()==12,"invalid original palette row");FullScoreboardColorBits out{};
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)out[i]|=std::uint32_t(b[i*4+j])<<(8*j);
  return out;
}
} // namespace
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply the original scoreboard providers report.json");
    if(!std::filesystem::exists(argv[1]))return 77;
    Json r;std::ifstream input(argv[1]);input>>r;
    check(r.at("source_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","wrong original executable");
    check(r.at("oracle_sha256")=="492c77bed874eb8def07371a509df4c767cded35a629e2d2f476ca05e653033e","wrong original provider oracle");
    check(r.at("instruction_patches").empty() && r.at("supplied_gameplay_graphics_readiness_clock_returns").empty(),"patched/supplied original results");
    FullScoreboardNameMappings m;
    for(const auto& t:r.at("tokens"))m.tokens.push_back({wide(t.at("text_utf16_hex")),char16_t(std::stoul(t.at("glyph").get<std::string>(),nullptr,16))});
    for(const auto& g:r.at("private_glyphs"))m.glyphs.push_back({std::uint32_t(std::stoul(g.at("glyph").get<std::string>(),nullptr,16)),std::uint32_t(std::stoul(g.at("handler").get<std::string>(),nullptr,16))});
    for(const auto& row:r.at("name_cases")) {
      check(row.at("completion")=="complete_original_return","original sanitizer did not return");
      const auto a=full_scoreboard_sanitize_name(wide(row.at("input_utf16_hex")),m);
      check(a.status==FullScoreboardNameStatus::Ready,"literal name provider rejected");
      check(a.text==wide(row.at("output_utf16_hex")),"sanitizer differs from original");
    }
    const auto& stopped=r.at("private_callback_stop").at("table_record");
    const std::u16string ch(1,char16_t(std::stoul(stopped.at("glyph").get<std::string>(),nullptr,16)));
    auto unavailable=full_scoreboard_sanitize_name(ch,m);
    check(unavailable.status==FullScoreboardNameStatus::UnavailableGlyphCallback,"dynamic original callback invented");
    check(unavailable.unavailable_handler==std::stoul(stopped.at("handler").get<std::string>(),nullptr,16),"wrong stopped callback");
    std::vector<FullScoreboardColorBits> palette;
    for(const auto& p:r.at("palette").at("records12_hex"))palette.push_back(color(p));
    const FullScoreboardPalette p{true,palette,color(r.at("palette").at("default12_hex"))};
    for(const auto& row:r.at("palette_cases")) {
      check(row.at("completion")=="complete_original_return","original palette did not return");
      check(full_scoreboard_palette_color(std::int8_t(row.at("signed_index").get<int>()),p)==color(row.at("output12_hex")),"signed palette lookup differs from original");
    }
    const FullScoreboardPalette cold{false,palette,p.fallback};
    check(full_scoreboard_palette_color(10,cold)==color(r.at("palette_cold_case").at("output12_hex")),"absent scenario fallback differs from original");
    const auto raw=bytes(r.at("profile_constructor").at("appearance4_hex"));
    std::array<std::uint8_t,4> appearance;std::copy(raw.begin(),raw.end(),appearance.begin());
    check(full_scoreboard_default_appearance()==appearance,"default profile appearance differs from owning constructor");
    const auto resolved=full_scoreboard_ffa_colors(appearance,p);check(r.at("ordinary_color").at("completion")=="complete_original_return","original FFA provider did not return");
    for(unsigned i=0;i<4;++i) {
      std::string slice=r.at("ordinary_color").at("output48_hex").get<std::string>().substr(i*24,24);
      check(resolved[i]==color(slice),"full FFA appearance provider differs from original");
    }
    // Host bounds, not additional admitted source cases.
    auto invalid=m;invalid.tokens.pop_back();check(full_scoreboard_sanitize_name(u"Ada",invalid).status==FullScoreboardNameStatus::InvalidMappings,"missing literal mappings admitted");
    invalid=m;invalid.tokens[0].text.clear();check(full_scoreboard_sanitize_name(u"Ada",invalid).status==FullScoreboardNameStatus::InvalidMappings,"empty malformed token admitted");
    std::cout<<checks<<" checks: original name, signed palette, default appearance and explicit callback boundary passed\n";
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
