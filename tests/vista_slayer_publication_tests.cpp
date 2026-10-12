#include "halo2/vista_slayer_publication.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const std::string& message) {
  if(!ok)throw std::runtime_error(message);
}
std::uint32_t word(const std::string& text,std::uint32_t maximum=0xffffffffu) {
  std::size_t used{};
  const auto value=std::stoull(text,&used,16);
  check(used==text.size() && value<=maximum,"Invalid original word: "+text);
  return std::uint32_t(value);
}
std::array<std::byte,vista_slayer_activation_storage_size> bytes(const std::string& text) {
  check(text.size()==2*vista_slayer_activation_storage_size,"Invalid original event storage length");
  std::array<std::byte,vista_slayer_activation_storage_size> result{};
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(word(text.substr(i*2,2),0xff));
  return result;
}
std::vector<VistaPlayerActivationService> calls(const std::string& text) {
  std::vector<VistaPlayerActivationService> result;
  if(text=="-")return result;
  std::istringstream input(text);
  std::string token;
  while(std::getline(input,token,',')) {
    if(token=="engine")result.push_back(VistaPlayerActivationService::engine_activation);
    else if(token=="notify")result.push_back(VistaPlayerActivationService::player_notification);
    else throw std::runtime_error("Unknown original activation intent: "+token);
  }
  return result;
}
}
int main(int argc,char** argv) {
  try {
    check(argc==2,"Supply original Vista Slayer publication fixture");
    std::ifstream file(argv[1]);
    if(!file){std::cerr<<"Missing original Vista Slayer publication fixture\n";return 77;}
    unsigned publisher{},activation{},suppressed{},padding{},no_query{},unchanged{},falling{},rising{},nonboolean{},retained_count{};
    bool version{};
    std::set<unsigned> phases;
    std::optional<std::uint16_t> retained;
    std::string line;
    while(std::getline(file,line)) {
      if(line=="# H2V_SLAYER_PUBLICATION_ORIGINAL_V1")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream input(line);
      char kind{};std::string label,phase_text;
      check(bool(input>>kind>>label>>phase_text),"Truncated original Slayer fixture row");
      const auto phase=std::uint8_t(word(phase_text,0xff));
      if(kind=='P') {
        std::string recipient_text,before_text,after_text,extra;
        unsigned expected_intent{};
        check(bool(input>>recipient_text>>before_text>>after_text>>expected_intent) &&
              expected_intent<=1 && !(input>>extra),label+": invalid original publisher row");
        const auto before=bytes(before_text),expected=bytes(after_text);
        // Guard both ends of the supplied storage; no interpretation of padding.
        std::array<std::byte,vista_slayer_activation_storage_size+4> guarded;
        guarded.fill(std::byte{0x6d});
        std::copy(before.begin(),before.end(),guarded.begin()+2);
        const auto intent=vista_slayer_activation_publication(phase,word(recipient_text),
            std::span<std::byte,vista_slayer_activation_storage_size>{guarded.data()+2,vista_slayer_activation_storage_size});
        check(intent==bool(expected_intent),label+": publication intent differs");
        check(std::equal(expected.begin(),expected.end(),guarded.begin()+2),label+": exact original storage differs");
        check(guarded[0]==std::byte{0x6d} && guarded[1]==std::byte{0x6d} &&
              guarded[guarded.size()-2]==std::byte{0x6d} && guarded.back()==std::byte{0x6d},
              label+": publisher wrote outside supplied storage");
        check(guarded[2+0x22]==before[0x22] && guarded[2+0x23]==before[0x23],label+": opaque padding changed");
        phases.insert(phase);++publisher;suppressed+=!intent;
        padding+=label.starts_with("publisher_padding_");
      } else if(kind=='A') {
        std::string before_text,query_text,after_text,order_text,extra;
        unsigned query_present{},engine{},notify{};
        check(bool(input>>before_text>>query_present>>query_text>>after_text>>engine>>notify>>order_text) &&
              query_present<=1 && engine<=1 && notify<=1 && !(input>>extra),label+": invalid original activation row");
        auto before=std::uint16_t(word(before_text,0xffff));
        const auto query=std::uint8_t(word(query_text,0xff));
        const bool sequence=label.starts_with("retained_");
        if(sequence) {
          check(label=="retained_"+std::to_string(retained_count),"Original retained sequence is out of order");
          if(retained){check(before==*retained,label+": retained flags differ from prior native state");before=*retained;}
        } else check(!retained,"Non-retained row followed original retained sequence");
        const auto admitted=vista_player_activation_query_admitted(phase,before);
        check(admitted==bool(query_present),label+": original query admission differs");
        VistaPlayerActivationUpdate update;
        update.flags=before;
        if(admitted)update=vista_player_activation_after_query(before,query);
        check(update.flags==word(after_text,0xffff),label+": original player flag word differs");
        const auto expected=calls(order_text);
        check(update.call_count==expected.size() && update.call_count<=update.calls.size(),label+": activation intent count differs");
        check(std::equal(expected.begin(),expected.end(),update.calls.begin()),label+": ordered activation intents differ");
        check(std::count(expected.begin(),expected.end(),VistaPlayerActivationService::engine_activation)==engine &&
              std::count(expected.begin(),expected.end(),VistaPlayerActivationService::player_notification)==notify,
              label+": original fixture intent counters disagree");
        check((update.flags&0xfffeu)==(before&0xfffeu),label+": unrelated player flag bits changed");
        ++activation;no_query+=!admitted;unchanged+=admitted && expected.empty();
        falling+=admitted && query==0 && notify;rising+=admitted && query!=0 && engine;
        nonboolean+=admitted && query>1;
        if(sequence){retained=update.flags;++retained_count;}
      } else throw std::runtime_error("Unknown original Slayer fixture row");
    }
    check(file.eof(),"Original Slayer fixture read failed");
    check(version && publisher==771 && activation==341 && phases.size()==256 &&
          suppressed==3 && padding==3 && no_query && unchanged && falling && rising &&
          nonboolean && retained_count==5,"Original Slayer fixture lacks required coverage");
    std::cout<<publisher<<" original Vista Slayer publications and "<<activation
             <<" player-activation decisions matched; exact padding, flags and ordered intents preserved\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
