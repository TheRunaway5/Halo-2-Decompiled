#include "halo2/vista_announcer_queue.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const std::string& message) {
  if(!ok)throw std::runtime_error(message);
}
std::uint32_t word(const std::string& text) {
  std::size_t used{};
  const auto n=std::stoull(text,&used,16);
  check(used==text.size() && n<=0xffffffffu,"invalid original word");
  return std::uint32_t(n);
}
VistaAnnouncerQueueState bank(const std::string& text) {
  check(text.size()==128,"original queue bank is not64bytes");
  std::array<std::uint8_t,64> bytes{};
  for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::uint8_t(word(text.substr(i*2,2)));
  const auto u32=[&](unsigned at) {
    return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
        (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
  };
  VistaAnnouncerQueueState state;
  for(unsigned i=0;i<5;++i) {
    const unsigned at=i*12;
    state.slots[i]={u32(at),u32(at+4),bytes[at+8],bytes[at+9],
        std::bit_cast<std::int16_t>(std::uint16_t(bytes[at+10]|(unsigned(bytes[at+11])<<8)))};
  }
  state.count=u32(60);
  return state;
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original Vista announcer queue fixture");
    std::ifstream file(argv[1]);
    if(!file)return 77;
    unsigned cases{},sequence{},initialize{},enqueue{},completion{},updates{};
    unsigned played{},failed{},shifted{},waiting{},rejected_enqueue{},matching_completion{},wrong_completion{};
    bool version{};
    std::optional<VistaAnnouncerQueueState> retained;
    std::string line;
    while(std::getline(file,line)) {
      if(line=="# H2V_ANNOUNCER_QUEUE_ORIGINAL_V1")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream input(line);
      char kind{};std::string label,a,b,before,after,cue,scale,at_call,extra;unsigned call_count{};
      check(bool(input>>kind>>label>>a>>b>>before>>after>>call_count>>cue>>scale>>at_call) &&
          !(input>>extra),"malformed original queue fixture");
      auto state=bank(before);
      const auto previous=state;
      const bool sequential=label.starts_with("sequential_");
      if(sequential) {
        std::ostringstream expected;expected<<"sequential_";
        expected.width(3);expected.fill('0');expected<<sequence;
        check(label==expected.str(),"original queue sequence is out of order");
        if(retained) {
          check(state==*retained,label+": sequential input differs from prior native bank");
          state=*retained;
        }
      } else check(!retained,"Cartesian rows resumed after sequence");
      unsigned actual_calls{};
      const VistaAnnouncerPlayback play=[&](std::uint32_t sound,float request_scale) {
        ++actual_calls;
        check(call_count==1 && actual_calls==1,label+": unexpected playback request");
        check(sound==word(cue),label+": playback sound differs");
        check(std::bit_cast<std::uint32_t>(request_scale)==word(scale),label+": request scale differs");
        check(state==bank(at_call),label+": pre-playback state/write order differs");
        return word(a);
      };
      if(kind=='I') {
        vista_announcer_queue_initialize(state,std::bit_cast<std::int16_t>(std::uint16_t(word(a))));
        ++initialize;
      } else if(kind=='E') {
        vista_announcer_queue_enqueue(state,word(a),std::bit_cast<std::int32_t>(word(b)));
        rejected_enqueue+=previous.count>4;
        ++enqueue;
      } else if(kind=='C') {
        vista_announcer_queue_completed(state,word(a));
        matching_completion+=previous.slots[0].voice==word(a);
        wrong_completion+=previous.slots[0].voice!=word(a);
        ++completion;
      } else if(kind=='U') {
        vista_announcer_queue_update(state,play);
        played+=actual_calls && word(a)!=vista_announcer_none;
        failed+=actual_calls && word(a)==vista_announcer_none;
        shifted+=previous.count && previous.slots[0].delay<0 && !previous.slots[0].active;
        waiting+=previous.count && previous.slots[0].delay<0 && previous.slots[0].active;
        ++updates;
      } else throw std::runtime_error("unknown original queue operation");
      check(actual_calls==call_count,label+": playback call count differs");
      check(state==bank(after),label+": complete source queue bank differs");
      if(!call_count)check(cue=="-" && scale=="-" && at_call=="-","invalid empty original playback bank");
      if(sequential){retained=state;++sequence;}
      ++cases;
    }
    check(file.eof(),"original queue fixture read failed");
    check(version && cases==1860 && sequence==77 && initialize==8 && enqueue==218 &&
        completion==51 && updates==1583 && played && failed && shifted && waiting &&
        rejected_enqueue && matching_completion && wrong_completion,
        "original announcer queue fixture lacks branch coverage");
    VistaAnnouncerQueueState invalid;invalid.count=6;
    bool rejected{};
    try{vista_announcer_queue_update(invalid,{});}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && invalid.count==6,"out-of-capacity queue update not rejected");
    std::cout<<cases<<" original Vista announcer queue operations matched; "<<sequence
             <<" sequential operations, full retained slots and playback write order exact\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
