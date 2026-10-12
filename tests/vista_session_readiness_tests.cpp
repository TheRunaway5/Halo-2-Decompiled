#include "halo2/vista_session_readiness.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

using namespace halo2;
namespace {
void check(bool ok,const std::string& message) { if(!ok)throw std::runtime_error(message); }
struct Words {
  std::vector<std::uint32_t> data;
  std::size_t cursor{};
  std::uint32_t get() {
    check(cursor<data.size(),"Truncated original readiness record");return data[cursor++];
  }
  std::int32_t integer() {return std::bit_cast<std::int32_t>(get());}
  bool flag() {const auto value=get();check(value<2,"Invalid original boolean");return value!=0;}
  std::uint8_t byte() {const auto value=get();check(value<256,"Invalid original byte");return std::uint8_t(value);}
  VistaSessionMapPath path() {
    VistaSessionMapPath result;
    for(auto& value:result){const auto word=get();check(word<65536,"Invalid original code unit");value=std::uint16_t(word);}
    return result;
  }
  VistaSessionMapReadinessState state() {return {integer(),integer(),byte(),integer()};}
  template<std::size_t N> std::array<std::uint8_t,N> bytes() {
    static_assert(N%4==0);std::array<std::uint8_t,N> result;
    for(unsigned i=0;i<N;i+=4){const auto value=get();for(unsigned j=0;j<4;++j)result[i+j]=std::uint8_t(value>>(j*8));}
    return result;
  }
  VistaSessionPeerMapRecord peer() {return {bytes<4>(),bytes<0xC8>()};}
  void end() {check(cursor==data.size(),"Extra original readiness words");}
};
std::uint32_t word(const std::string& text) {
  std::size_t used{};const auto value=std::stoull(text,&used,16);
  check(used==text.size() && value<=0xffffffffu,"Invalid original readiness word");return std::uint32_t(value);
}
std::uint32_t get(const VistaSessionPeerMapPayload& payload,unsigned offset) {
  std::uint32_t result{};for(unsigned i=0;i<4;++i)result|=std::uint32_t(payload[offset+i])<<(i*8);return result;
}
void patch_check(const VistaSessionMapReadinessState& state) {
  VistaSessionPeerMapPayload payload;
  for(unsigned i=0;i<payload.size();++i)payload[i]=std::uint8_t(i*37+11);
  const auto before=payload;
  vista_session_map_readiness_payload(payload,state);
  check(get(payload,0x60)==std::bit_cast<std::uint32_t>(state.status) &&
      get(payload,0x64)==std::bit_cast<std::uint32_t>(state.progress),"Readiness payload lost original output words");
  for(unsigned i=0;i<payload.size();++i)if(i<0x60 || i>=0x68)
    check(payload[i]==before[i],"Readiness payload changed an unowned byte");
}
template<class F> void rejects(F&& function) {
  bool rejected{};try{function();}catch(const FormatError&){rejected=true;}
  check(rejected,"Unsupported original readiness context was admitted");
}
void controls() {
  VistaSessionMapReadinessQueries query;
  query.lifecycle_slot=1;query.mxcsr=0x1f80;
  query.session=VistaSessionMapReadinessSession{5,{1,17,{}}};
  query.session->selected.path[0]='a';
  const VistaSessionMapReadinessState before{7,88,0x80,19};
  for(const auto invalid:{0x1f00u,0x11f80u}) {
    auto state=before;query.mxcsr=invalid;
    rejects([&]{(void)vista_session_map_readiness(state,query);});
    check(state==before,"Unsupported FP context partially wrote readiness");
  }
  query.mxcsr=0x1f80;query.loaded_world_valid=true;
  auto state=before;
  rejects([&]{(void)vista_session_map_readiness(state,query);});
  check(state==before,"Missing consumed loaded metadata partially wrote readiness");
  query.loaded_world_valid=false;
  for(bool empty_path:{false,true}) {
    query.session->selected.id=empty_path?17:-1;
    query.session->selected.path[0]=empty_path?0:'a';
    state=before;rejects([&]{(void)vista_session_map_readiness(state,query);});
    check(state==before,"Unsupported alternate selection partially wrote readiness");
  }
  std::array<VistaSessionPeerMapRecord,2> peers{};const auto original=peers;
  VistaSessionPeerMapPayload payload{};
  for(auto session_state:{0u,3u,4u,9u,0xffffffffu}) {
    std::uint32_t revision=0xffffffffu,change=7;
    rejects([&]{(void)vista_session_publish_local_map_readiness(session_state,0,peers,revision,change,payload);});
    check(peers==original && revision==0xffffffffu && change==7,"Remote rejection changed peer bank");
  }
  for(auto host:{2u,0xffffffffu}) {
    std::uint32_t revision=0xffffffffu,change=7;
    rejects([&]{(void)vista_session_publish_local_map_readiness(5,host,peers,revision,change,payload);});
    check(peers==original && revision==0xffffffffu && change==7,"Invalid host changed peer bank");
  }
}
}

int main(int argc,char** argv) {
  try {
    check(argc==2,"Supply original Vista readiness fixture");
    std::ifstream file(argv[1]);check(bool(file),"Missing original readiness fixture");
    unsigned selectors{},paths{},publications{},ready3{},ready4{},requests{},case_mismatch{},rounding{},inactive{};
    bool version{};std::string line;
    while(std::getline(file,line)) {
      if(line=="# H2V_SESSION_READINESS_ORIGINAL_V1")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream input(line);std::string kind,label,token;std::size_t count{};
      check(bool(input>>kind>>label>>count),"Invalid original readiness row");
      Words words;while(input>>token)words.data.push_back(word(token));
      check(words.data.size()==count,"Original readiness word count differs");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
      const auto host_mxcsr=_mm_getcsr();
#endif
      if(kind=="S") {
        VistaSessionMapReadinessQueries query;
        query.mxcsr=words.get();query.lifecycle_slot=words.get();
        const bool present=words.flag();
        VistaSessionMapReadinessSession session;
        session.state=words.get();session.selected.type=words.get();session.selected.id=words.integer();
        session.selected.path=words.path();if(present)query.session=session;
        query.loaded_world_valid=words.flag();const auto loaded_present=words.flag();
        VistaSessionSelectedMap loaded;
        loaded.type=words.get();loaded.id=words.integer();loaded.path=words.path();
        if(loaded_present)query.loaded_options=loaded;
        query.map_query=words.integer();query.map_progress_bits=words.get();
        auto state=words.state();const auto before=state;const auto expected=words.state();
        const auto result=vista_session_map_readiness(state,query);
        check(state==expected,label+": map readiness fields differ");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        // A declared original MXCSR must govern MULSS/CVTTSS2SI even when
        // the calling native thread has the opposite rounding direction.
        // Reuse independent unchanged-PE outputs rather than host arithmetic.
        for(const auto ambient:{0x1f80u,0x3f80u,0x5f80u,0x7f80u}) {
          _mm_setcsr(ambient);auto repeated=before;
          const auto calls=vista_session_map_readiness(repeated,query);
          check(_mm_getcsr()==ambient,label+": readiness changed ambient SSE context");
          _mm_setcsr(host_mxcsr);
          check(repeated==expected,label+": declared SSE context lost to ambient rounding");
          check(calls.call_count==result.call_count && std::equal(calls.call_intents().begin(),
            calls.call_intents().end(),result.call_intents().begin()),label+": ambient SSE context changed service intents");
        }
#endif
        const auto expected_count=words.get();check(result.call_count==expected_count,label+": query count differs");
        for(const auto& call:result.call_intents()) {
          check(unsigned(call.service)==words.get(),label+": query order differs");
          check(call.argument_count==words.get(),label+": scalar argument count differs");
          for(unsigned i=0;i<call.argument_count;++i)check(call.arguments[i]==words.get(),label+": query scalar argument differs");
          requests+=call.service==VistaSessionMapReadinessService::map_request;
        }
        patch_check(state);ready3+=state.status==3;ready4+=state.status==4;
        rounding+=query.mxcsr!=0x1f80;inactive+=state.status==0;++selectors;
      } else if(kind=="P") {
        const auto left=words.path(),right=words.path();const auto expected=words.get();
        check(vista_session_map_paths_equal(left,right)==(expected==0),label+": source path comparison differs");
        case_mismatch+=label.starts_with("different_") && expected!=0;++paths;
      } else if(kind=="W") {
        const auto session_state=words.get(),host=words.get(),peer_count=words.get();
        auto revision=words.get(),change=words.get();const auto payload=words.bytes<0xC8>();
        std::vector<VistaSessionPeerMapRecord> peers(peer_count),expected(peer_count);
        for(auto& peer:peers)peer=words.peer();
        for(auto& peer:expected)peer=words.peer();
        const auto expected_revision=words.get(),expected_change=words.get(),expected_return=words.get();
        const bool accepted=vista_session_publish_local_map_readiness(session_state,host,peers,revision,change,payload);
        check(peers==expected && revision==expected_revision && change==expected_change && unsigned(accepted)==expected_return,
            label+": local payload/presence/revision publication differs");++publications;
      } else throw std::runtime_error("Unknown original readiness row kind");
      words.end();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
      check(_mm_getcsr()==host_mxcsr,label+": readiness leaked SSE environment");
#endif
    }
    check(file.eof(),"Original readiness fixture read failed");
    check(version && selectors==734 && paths==15 && publications==192 && ready3 && ready4 && requests &&
        case_mismatch && rounding && inactive,"Original readiness fixture lacks required coverage");
    controls();
    std::cout<<selectors<<" original Vista map selectors, "<<paths<<" path comparisons and "
        <<publications<<" local payload publications matched\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
