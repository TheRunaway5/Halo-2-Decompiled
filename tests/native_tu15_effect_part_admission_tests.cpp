#include "halo2/source_effect_part_admission_tu15.hpp"
#include <fstream>
#include <array>
#include <bit>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char** argv)try{
 if(argc!=3)throw std::runtime_error("Supply original part admission reference and actual destruction part census");
 const auto ref=nlohmann::json::parse(std::ifstream(argv[1]));
 if(ref.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"||
    !ref.at("original_instruction_patches").empty()||!ref.at("substituted_callees").empty())throw std::runtime_error("Unqualified effect admission authority");
 unsigned selected{},rejected{},cases{};
 for(const auto& row:ref.at("cases")){
  const bool admitted=source_effect_part_reference_admitted_tu15(row.at("location"),row.at("count"),row.at("reference"))&&
      source_effect_part_selector_admitted_tu15(row.at("selector"),row.at("flags"));
  if(admitted!=row.at("selected_for_location_iteration").get<bool>())throw std::runtime_error(row.at("name").get<std::string>()+": Original part selection differs");
  std::array<std::byte,0x38> bytes{};
  const auto put=[&](unsigned at,std::uint32_t value,unsigned n=4){for(unsigned i=0;i<n;++i)bytes[at+i]=std::byte(value>>(i*8u));};
  put(0,row.at("environment"),2);put(2,row.at("selector"),2);
  put(4,std::bit_cast<std::uint16_t>(row.at("location").get<std::int16_t>()),2);put(8,0x736e6421u);put(0x10,row.at("reference"));
  put(0x30,0x12345678u);put(0x34,0x87654321u);
  const auto part=source_effect_sound_part_tu15(bytes,row.at("count"));
  const bool adapter_selected=part&&source_effect_part_selector_admitted_tu15(part->selector,row.at("flags"));
  if(adapter_selected!=admitted || (part&&(part->sound!=row.at("reference").get<unsigned>()||part->location!=row.at("location").get<std::int16_t>()||
      part->selector!=row.at("selector").get<unsigned>()||part->scale_a!=0x12345678u||part->scale_b!=0x87654321u)))throw std::runtime_error("Actual shared sound-part adapter lost literal field ownership");
  put(8,0x6c696768u);if(source_effect_sound_part_tu15(bytes,row.at("count")))throw std::runtime_error("Sound adapter accepted another dispatcher group");
  ++cases;if(admitted)++selected;else ++rejected;
 }
 if(cases!=1024||!selected||!rejected)throw std::runtime_error("Incomplete part admission matrix");
 for(unsigned n:{0u,0x37u,0x39u}){std::array<std::byte,0x39> bytes{};bool refused=false;
  try{(void)source_effect_sound_part_tu15(std::span<const std::byte>(bytes).first(n),1);}catch(const std::invalid_argument&){refused=true;}
  if(!refused)throw std::runtime_error("Incomplete sound part was accepted");}
 const auto stock=nlohmann::json::parse(std::ifstream(argv[2]));unsigned stock_parts{};
 if(!stock.at("passed").get<bool>()||stock.at("conditional_selector_parts")!=0||stock.at("invalid_location_parts")!=0)
  throw std::runtime_error("Unqualified current destruction part census");
 for(const auto& row:stock.at("cases")){
  const auto hex=row.at("part_hex").get<std::string>();if(hex.size()!=0x70)throw std::runtime_error("Actual destruction part extent");
  std::array<std::byte,0x38> raw{};for(unsigned i=0;i<raw.size();++i)raw[i]=std::byte(std::stoul(hex.substr(i*2,2),nullptr,16));
  const auto before=raw;const auto part=source_effect_sound_part_tu15(raw,row.at("location_count"));
  if(!part||part->location!=row.at("location").get<std::int16_t>()||part->selector!=0||
     !source_effect_part_selector_admitted_tu15(part->selector,0)||!source_effect_part_selector_admitted_tu15(part->selector,32)||raw!=before)
   throw std::runtime_error("Actual destruction sound part changed admission or source bytes");
  ++stock_parts;
 }
 if(stock_parts!=368)throw std::runtime_error("Incomplete current destruction part census");
 std::cout<<"PASS "<<cases<<" original part admission boundaries; "<<selected<<" iterator selections and "<<rejected
  <<" natural rejections; "<<stock_parts<<" actual MCC destruction sound parts retain both selector branches; signed locations/counts, NONE, part+2 and actual effect flag bit5; no environment or playback result supplied\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
