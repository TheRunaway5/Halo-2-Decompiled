#include "halo2/native_tu15_sound_ep_input.hpp"
#include "halo2/native_tu15_sound_ep_stereo_numeric.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include <sstream>
#include <type_traits>
using namespace halo2;
namespace {
using Json=nlohmann::json;using Word24=tu15_sound::Word24;
void need(bool v,const char* s){if(!v)throw std::runtime_error(s);}
std::uint32_t read(){std::array<unsigned char,4>b{};std::cin.read(reinterpret_cast<char*>(b.data()),4);need(bool(std::cin),"Short GP value fixture");return unsigned(b[0])|(unsigned(b[1])<<8)|(unsigned(b[2])<<16)|(unsigned(b[3])<<24);}
std::string hash(const tu15_ep_input::Inputs& values){std::uint64_t h=0xcbf29ce484222325ull;for(const auto& c:values)for(auto w:c)for(unsigned b=0;b<4;++b){h^=(w.bits>>(b*8))&255u;h*=0x100000001b3ull;}std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(16)<<h;return out.str();}
void verify(const tu15_ep_input::InputWindow& window,const Json& row){
 need(window.half_word_offset()==row.at("half_phase").get<unsigned>()&&hash(window.values())==row.at("input_hash").get<std::string>(),"Original EP channel/half acquisition differs");
 tu15_ep_stereo_numeric::Controls controls{Word24(row.at("numeric_entry_SR").get<unsigned>()),{}};controls.address_modes.fill(tu15_ep_stereo_numeric::AddressingMode::linear);
 const auto result=tu15_ep_stereo_numeric::process(controls,{Word24(row.at("scale_enabled").get<unsigned>()),Word24(row.at("scale").get<unsigned>())},window.values());
 std::vector<unsigned> words;for(const auto& c:result.output)for(auto w:c)words.push_back(w.bits);for(auto w:result.peak)words.push_back(w.bits);for(auto w:result.energy_root)words.push_back(w.bits);for(auto w:result.coefficients)words.push_back(w.bits);
 need(words==row.at("words").get<decltype(words)>(),"Original full EP stereo outputs/diagnostics differ");
 std::ostringstream hex;for(auto b:tu15_ep_stereo_numeric::pack_interleaved_s16(result))hex<<std::hex<<std::setfill('0')<<std::setw(2)<<std::to_integer<unsigned>(b);
 need(hex.str()==row.at("fifo_hex").get<std::string>(),"Original EP FIFO bytes differ");
}
}
int main(int argc,char** argv)try{
 static_assert(!std::is_default_constructible_v<tu15_ep_input::InputWindow>);
 need(argc==2,"Supply original EP acquisition reference");const auto ref=Json::parse(std::ifstream(argv[1]));
 need(ref.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&ref.at("original_program_patches").empty()&&ref.at("supplied_callee_or_clock_results").empty(),"Unqualified EP acquisition authority");
 need(read()==0x47504f31u&&read()==36,"Original GP value fixture shape");std::unique_ptr<tu15_gp_output::OutputOwner> owner;unsigned joined{},snapshots{},guards{},lifetime{},current_views{};
 std::optional<tu15_gp_output::LogicalCopyReceipt> previous;
 std::vector<tu15_ep_input::InputWindow> retained;std::vector<Json> retained_reference;
 for(unsigned record=0;record<36;++record){
  const auto execute=read();std::array<unsigned,6> cursors{};for(auto& c:cursors)c=read();std::array<Word24,3712> expected{};for(auto& w:expected)w=Word24(read());std::array<Word24,1024> mix{};for(auto& w:mix)w=Word24(read());
  if(!execute){previous.reset();owner=tu15_gp_output::OutputOwner::retained_phase({expected,cursors});++snapshots;continue;}
  const auto receipt=owner->copy_values(mix);
  need(owner->byte_cursors()==cursors&&std::ranges::equal(expected,owner->retained_values()),"Actual native GP resource differs from unchanged original output");
  need(reinterpret_cast<std::uintptr_t>(receipt.retained_values().data())>0xffffffffull,"GP resource did not use native64 storage");
  for(const auto& row:ref.at("cases"))if(row.at("record")==record){
   auto window=tu15_ep_input::copy_input_window(receipt,row.at("half_phase"));verify(window,row);++joined;
   retained.push_back(window);retained_reference.push_back(row);
   if(previous){verify(tu15_ep_input::copy_input_window(*previous,row.at("half_phase")),row);++current_views;}
   auto heap=std::make_unique<tu15_ep_input::InputWindow>(window);need(reinterpret_cast<std::uintptr_t>(&heap->values())>0xffffffffull,"EP value copy did not use native64 ownership");verify(*heap,row);++lifetime;
  }
  for(unsigned invalid:{1u,255u,257u,511u,512u,0xffffffffu}){bool refused=false;try{(void)tu15_ep_input::copy_input_window(receipt,invalid);}catch(const std::invalid_argument&){refused=true;}need(refused,"Unproved EP half selection admitted");++guards;}
  need(owner->byte_cursors()==cursors&&std::ranges::equal(expected,owner->retained_values()),"EP acquisition mutated GP resources or advanced its cursors");previous=receipt;
 }
 previous.reset();owner.reset();need(std::cin.peek()==EOF&&joined==66&&snapshots==3&&current_views==60,"Incomplete original GP/EP join matrix");
 for(unsigned i=0;i<retained.size();++i){verify(retained[i],retained_reference[i]);++lifetime;}
 need(ref.at("original_full_module_returns")==joined&&ref.at("original_input_DMA_requests")==joined*5,"Original EP request matrix incomplete");
 std::cout<<Json{{"passed",true},{"original_GP_copies",33},{"original_EP_returns",joined},{"original_input_reads",joined*5},{"owned_snapshot_lifetime_checks",lifetime},{"half_admission_guards",guards},
  {"older_receipts_read_current_resources",current_views},{"GP_resource_no_mutation_checks",33},{"snapshots_after_source_destruction",66},
  {"original_EP_numeric_words",526*joined},{"original_FIFO_bytes",1024*joined},{"hardware_frame_DMA_or_endpoint_receipt",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
