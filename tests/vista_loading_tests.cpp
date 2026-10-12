#include "halo2/vista_loading.hpp"
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

using namespace halo2;
namespace {
void check(bool ok,const std::string& message) {
  if(!ok)throw std::runtime_error(message);
}
std::uint32_t word(const std::string& text) {
  std::size_t used{};
  const auto value=std::stoull(text,&used,16);
  check(used==text.size() && value<=0xffffffffu,"Invalid original32-bit word");
  return std::uint32_t(value);
}
unsigned count(const std::string& text) {
  std::size_t used{};
  const auto value=std::stoul(text,&used,10);
  check(used==text.size() && value<=5,"Invalid original service count");
  return unsigned(value);
}
std::vector<std::string> fields(const std::string& line) {
  std::istringstream input(line);
  std::vector<std::string> result;
  std::string value;
  while(input>>value)result.push_back(value);
  return result;
}
VistaLoadingFadeState fade_state(const std::vector<std::string>& row,unsigned offset) {
  const auto active=word(row[offset]);
  check(active<=255,"Invalid original active byte");
  return {std::uint8_t(active),word(row[offset+1]),word(row[offset+2])};
}
std::string progress_words(const VistaLoadingProgressState& state) {
  std::ostringstream out;
  out<<std::hex<<state.baseline_bits<<','<<state.progress_bits<<','<<state.previous_clock;
  return out.str();
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
std::uint16_t control_word() {
  std::uint16_t result;
  asm volatile("fnstcw %0":"=m"(result));
  return result;
}
struct HostFloatingPoint {
  std::uint16_t cw=control_word();
  unsigned mxcsr=_mm_getcsr();
  int exceptions=std::fetestexcept(FE_ALL_EXCEPT);
  void unchanged(const std::string& label) const {
    check(cw==control_word() && mxcsr==_mm_getcsr() &&
        exceptions==std::fetestexcept(FE_ALL_EXCEPT),label+": host FP state leaked");
  }
};
void rejection_controls() {
  const VistaLoadingProgressQueryResults query{0x3f000000,1000,1,0x3f000000};
  for(const VistaLoadingFloatingPoint context:std::array<VistaLoadingFloatingPoint,4>{
      VistaLoadingFloatingPoint{0x027e,0x1f80}, {0x017f,0x1f80},
      {0x027f,0x1f00}, {0x027f,0x11f80}}) {
    VistaLoadingProgressState progress{0xbf800000,0,0};
    const auto before=progress;
    const HostFloatingPoint host;
    bool rejected{};
    try { (void)vista_loading_progress_update(progress,query,context); }
    catch(const std::runtime_error&) { rejected=true; }
    check(rejected && progress==before,"Unsupported progress FP context mutated state");
    host.unchanged("unsupported progress context");
    VistaLoadingFadeState fade{255,0x3f000000,0x3f4ccccd};
    const auto before_fade=fade;
    rejected=false;
    try { (void)vista_loading_fade_begin(fade,0,1,context); }
    catch(const std::runtime_error&) { rejected=true; }
    check(rejected && fade==before_fade,"Unsupported begin FP context mutated state");
    host.unchanged("unsupported begin context");
    rejected=false;
    try { (void)vista_loading_fade_update(fade,0,1,context); }
    catch(const std::runtime_error&) { rejected=true; }
    check(rejected && fade==before_fade,"Unsupported fade FP context mutated state");
    host.unchanged("unsupported fade context");
  }
}
#endif
}

int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original Vista loading fixture");
#if !((defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER))
    throw std::runtime_error("Original Vista loading x87/SSE test backend unavailable");
#else
    std::ifstream file(argv[1]);
    check(bool(file),"Missing original Vista loading fixture");
    unsigned progress_cases{},begin_cases{},fade_cases{},raw_cases{},sequences{};
    unsigned inactive{},complete{},zero_gain_incomplete{},active255{},wraps{},query_boundary_changes{};
    std::set<std::pair<std::uint16_t,std::uint32_t>> contexts;
    std::map<std::pair<std::uint16_t,std::uint32_t>,VistaLoadingProgressState> retained;
    std::map<std::pair<std::uint16_t,std::uint32_t>,unsigned> sequence_indices;
    bool version{};
    std::string line;
    while(std::getline(file,line)) {
      if(line=="# H2V_LOADING_ORIGINAL_V2")version=true;
      if(line.empty() || line.front()=='#')continue;
      const auto row=fields(line);
      check(row.size()>=4,"Truncated original loading row");
      const auto cw=word(row[2]);
      check(cw<=0xffff,"Invalid original x87 control word");
      const VistaLoadingFloatingPoint fp{std::uint16_t(cw),word(row[3])};
      const auto key=std::pair(fp.x87_control_word,fp.mxcsr);
      contexts.insert(key);
      const HostFloatingPoint host;
      const auto& label=row[1];
      if(row[0]=="P") {
        check(row.size()==17,label+": invalid original progress row (requires V2 post-spill input)");
        VistaLoadingProgressState state{word(row[4]),word(row[5]),word(row[6])};
        const bool sequence=label.starts_with("sequence_");
        if(sequence) {
          const auto index=sequence_indices[key]++;
          check(label.ends_with("_"+std::to_string(index)),label+": sequence order differs");
          if(index) {
            check(state==retained.at(key),label+": retained original input differs from prior native output");
            state=retained.at(key);
          }
        }
        // V2 preserves V1's declared pre-thunk query word at7 and supplies
        // the independently observed original647F7D post-spill input at8.
        query_boundary_changes+=word(row[7])!=word(row[8]);
        const VistaLoadingProgressQueryResults query{word(row[8]),
            std::bit_cast<std::int32_t>(word(row[9])),word(row[10]),word(row[11])};
        const auto update=vista_loading_progress_update(state,query,fp);
        const VistaLoadingProgressState expected{word(row[12]),word(row[13]),word(row[14])};
        check(state==expected,label+": retained loading progress fields differ (native "+
            progress_words(state)+", original "+progress_words(expected)+")");
        check(count(row[15])==5 && row[16]=="path,map,duration,clock,secondary" &&
            update.calls==std::array{VistaLoadingProgressService::path_conversion,
              VistaLoadingProgressService::map_progress,VistaLoadingProgressService::duration,
              VistaLoadingProgressService::clock,VistaLoadingProgressService::secondary_progress},
            label+": ordered progress service intents differ");
        if(sequence){retained[key]=state;++sequences;}
        raw_cases+=label.starts_with("raw_");
        wraps+=query.clock<word(row[6]);
        ++progress_cases;
      } else if(row[0]=="B") {
        check(row.size()==14,label+": invalid original begin row");
        auto state=fade_state(row,4);
        const auto before=state;
        const auto begin=vista_loading_fade_begin(state,word(row[7]),word(row[8]),fp);
        check(state==fade_state(row,9),label+": fade-begin retained fields differ");
        check(state.active==1 && state.gain_bits==before.gain_bits && count(row[12])==1 &&
            row[13]=="clock" && begin.calls==std::array{VistaLoadingFadeService::clock},
            label+": original fade-begin clock/gain contract differs");
        wraps+=word(row[8])<word(row[7]);
        ++begin_cases;
      } else if(row[0]=="F") {
        check(row.size()==17,label+": invalid original fade row");
        auto state=fade_state(row,4);
        const auto before=state;
        const auto update=vista_loading_fade_update(state,word(row[7]),word(row[8]),fp);
        check(state==fade_state(row,9),label+": fade retained fields differ");
        check(update.elapsed_seconds_bits==word(row[12]) &&
            unsigned(update.complete)==count(row[13]),label+": fade elapsed/completion differs");
        if(before.active) {
          check(count(row[15])==1 && row[16].starts_with("gain:") &&
              update.gain_service_bits==std::optional{word(row[16].substr(5))},
              label+": ordered gain service argument differs");
          check(row[14]!="-" && update.packed_argb==std::optional{word(row[14])},
              label+": original ARGB8 vertex packing differs");
          zero_gain_incomplete+=state.gain_bits==0 && !update.complete;
        } else {
          check(count(row[15])==0 && row[16]=="-" && row[14]=="-" &&
              !update.gain_service_bits && !update.packed_argb && state==before && !update.complete,
              label+": inactive fade produced a draw/service/state change");
          ++inactive;
        }
        complete+=update.complete;
        active255+=before.active==255 && state.active==255;
        ++fade_cases;
      } else throw std::runtime_error("Unknown original loading row kind");
      host.unchanged(label);
    }
    check(file.eof(),"Original Vista loading fixture read failed");
    check(version && progress_cases==6146 && begin_cases==28 && fade_cases==924 &&
        contexts.size()==7 && raw_cases==224 && sequences==42 && inactive && complete &&
        zero_gain_incomplete && active255 && wraps && query_boundary_changes==2,
        "Original loading fixture lacks required coverage");
    for(const auto& [key,index]:sequence_indices) {
      (void)key;
      check(index==6,"Original retained progress sequence lacks six updates");
    }
    rejection_controls();
    std::cout<<progress_cases<<" original Vista progress updates, "<<begin_cases
        <<" fade begins and "<<fade_cases<<" fade/color updates matched across seven explicit FP contexts\n";
    return 0;
#endif
  } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
