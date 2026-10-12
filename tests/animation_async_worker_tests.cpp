#include "halo2/animation_async_worker.hpp"
#include "halo2/native_synchronization.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__linux__)
#include <unistd.h>
#endif
using namespace halo2;

#if defined(__linux__)
namespace {
struct Stop{};
std::uint32_t hex(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string&s){
  if(s.size()%2)throw std::runtime_error("Odd fixture bytes");std::vector<std::byte>b;
  for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(hex(s.substr(i,2))));return b;
}
std::uint32_t word(std::span<const std::byte>b){std::uint32_t v=0;for(unsigned i=0;i<b.size();++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(8*i);return v;}
std::string h(std::uint32_t v,unsigned n=4){std::ostringstream s;for(unsigned i=0;i<n;++i)s<<std::hex<<std::setfill('0')<<std::setw(2)<<((v>>(8*i))&255);return s.str();}
std::string number(std::uint32_t v){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(8)<<v;return s.str();}
struct Case {
  std::string name,kind;std::uint32_t entry{},argument{},callback{},returned{};bool stop{};
  std::map<std::string,std::vector<std::byte>> before,after;std::vector<std::string> events;
};
struct Memory {
  std::map<std::uint32_t,std::vector<std::byte>> banks;
  std::span<std::byte> get(std::uint32_t p,std::size_t n){
    auto it=banks.upper_bound(p);if(it==banks.begin())throw std::out_of_range("Unowned raw async address");--it;
    const std::uint64_t off=std::uint64_t(p)-it->first;if(off>it->second.size()||n>it->second.size()-off)throw std::out_of_range("Raw async span escapes backing");
    return std::span(it->second).subspan(off,n);
  }
};
std::string call(const std::string&kind,std::initializer_list<std::uint32_t> values){
  std::string s="CALL "+kind;for(auto v:values)s+=' '+number(v);return s;
}
std::size_t run(const Case&c,const std::filesystem::path&maps){
  constexpr std::uint32_t bank=0x4e7924,current=0x518000,done=0xa103000,count=0xa103010,buffer=0x82947000;
  Memory m;const std::array<std::pair<const char*,std::uint32_t>,5> names{{{"bank_hex",bank},{"current_hex",current},{"done_hex",done},{"count_hex",count},{"buffer_hex",buffer}}};
  for(auto[name,p]:names)m.banks[p]=c.before.at(name);
  SourceAnimationAsyncWorkerServices s;s.memory.read=[&](auto p,auto n){return std::span<const std::byte>(m.get(p,n));};
  s.memory.write=[&](auto p,auto b){auto out=m.get(p,b.size());std::copy(b.begin(),b.end(),out.begin());};
  s.bindings={{0x4e7938,0x4e9eb8,0x4e9ebc,0x4e7928,0x4e792c,0x4e7930,0x4e7924,0x5098a0,0x5018a0,0x122350},0x4e7934,0x4e9ec0,current,0x1a47d0};
  std::vector<std::string> actual;std::size_t stores=0;
  auto observe=[&](const SourceAnimationAsyncStore&q){++stores;actual.push_back("STORE "+number(q.instruction)+' '+number(q.address)+' '+std::to_string(q.bytes)+' '+h(q.before,q.bytes)+' '+h(q.after,q.bytes));};
  NativeSynchronization sync({0xf3000001,0xf3000004});auto native=sync.services();
  const auto a=native.create_mutex(false),b=native.create_mutex(false),sem=native.create_semaphore(c.entry==0x122360?1:0,150);
  if(a.error||b.error||sem.error||a.handle!=0xf3000001||b.handle!=0xf3000002||sem.handle!=0xf3000003)throw std::runtime_error("Actual native sync creation failed");
  s.wait=[&](auto handle,auto ms){
    // Bounded observation provider: try actual exclusion/permit, and unwind
    // before source post-wait stores when no actual permit exists. No source
    // successful wait/status or shutdown notification is fabricated.
    const auto result=native.wait(handle,0);
    if(result.state==NativeSynchronizationWaitState::TimedOut)throw Stop{};
    if(result.state!=NativeSynchronizationWaitState::Acquired||result.error)throw std::runtime_error("Actual native wait failed");actual.push_back(call("wait",{handle,ms}));
  };
  s.release_mutex=[&](auto handle){if(native.release_mutex(handle))throw std::runtime_error("Actual native mutex release failed");actual.push_back(call("release",{handle}));};
  s.release_semaphore=[&](auto handle,auto n){const auto result=native.release_semaphore(handle,n);if(result.error||!result.previous_count)throw std::runtime_error("Actual native semaphore release failed");actual.push_back(call("signal",{handle,n,std::uint32_t(*result.previous_count)}));};
  s.yield=[] {throw Stop{};};
  std::array<SourceAsyncOwnedFile,4> files{{{0xf4000001,maps/"lockout.map"},{0xf4000002,maps/"mainmenu.map"},{0xf4000003,maps/"shared.map"},{0xf4000004,maps/"single_player_shared.map"}}};
  int pipe_read=-1;std::unique_ptr<SourceAsyncOwnedFiles> owned;
  if(c.kind=="file_nonseek"){
    int fd[2];if(pipe(fd))throw std::runtime_error("Actual native pipe creation failed");pipe_read=fd[0];close(fd[1]);files[0].path="/proc/self/fd/"+std::to_string(pipe_read);
  }
  if(c.kind=="file"||c.kind=="file_nonseek")owned=std::make_unique<SourceAsyncOwnedFiles>(std::span(files).first(1));
  s.callback=[&](auto callback,auto payload)->std::int32_t{
    if(c.kind=="callback_control") {actual.push_back(call("callback_control",{callback,payload,c.callback}));return std::bit_cast<std::int32_t>(c.callback);}
    if(!owned||callback!=0x1a47d0)throw std::runtime_error("Actual file callback binding missing");
    actual.push_back(call("callback",{callback,payload}));
    const auto result=read_source_animation_async_file(payload,0x40000,{buffer,m.banks.at(buffer)},*owned,s.memory,observe,[&](const SourceAsyncReadExecution&q){
      actual.push_back(call("seek",{q.step.file,q.step.offset,0,q.io.seek_low}));
      if(q.io.seek_low!=0xffffffffu)actual.push_back(call("read",{q.step.file,q.step.buffer,q.step.size,q.io.bytes_read}));
    });return result.completion.callback_result;
  };
  std::uint32_t returned=0;bool stopped=false;
  try {
    switch(c.entry){
    case 0x121fd0:returned=lease_source_animation_async_node(s,observe);break;
    case 0x122020:insert_source_animation_async_node(c.argument,s,observe);break;
    case 0x1220e0:returned=publish_source_animation_async_node(c.argument,s,observe);break;
    case 0x122150:unlink_source_animation_async_node(c.argument,s,observe);break;
    case 0x122360:run_source_animation_async_worker(s,observe);
    case 0x19d8c0:returned=submit_source_animation_async_read({c.argument,0x7a00,buffer,count,done,7,3},{0xf4000001,0xf4000002,0xf4000003,0xf4000004},s,observe);break;
    default:throw std::runtime_error("Unknown original entry");
    }
  }catch(const Stop&){stopped=true;}
  owned.reset();if(pipe_read>=0)close(pipe_read);
  if(stopped!=c.stop)throw std::runtime_error(c.name+": stop differs");
  if(!stopped&&(c.entry==0x121fd0||c.entry==0x1220e0||c.entry==0x19d8c0)&&returned!=c.returned)throw std::runtime_error(c.name+": result differs");
  if(actual!=c.events){
    std::size_t i=0;while(i<std::min(actual.size(),c.events.size())&&actual[i]==c.events[i])++i;
    throw std::runtime_error(c.name+": event "+std::to_string(i)+" expected "+(i<c.events.size()?c.events[i]:"END")+" got "+(i<actual.size()?actual[i]:"END"));
  }
  for(auto[name,p]:names)if(m.banks.at(p)!=c.after.at(name))throw std::runtime_error(c.name+": complete buffer differs: "+name);
  // Raw host malformed/missing-provider stops must not manufacture ownership.
  if(c.entry==0x121fd0&&!c.stop){auto bad=s;bad.wait={};const auto before=m.banks;bool rejected=false;try{lease_source_animation_async_node(bad);}catch(const std::runtime_error&){rejected=true;}if(!rejected||before!=m.banks)throw std::runtime_error("Missing wait mutated source storage");}
  return stores;
}
} // namespace
#endif
int main(int argc,char**argv){
#if !defined(__linux__)
  (void)argc;(void)argv;std::cerr<<"Linux owned-pipe fixture provider unavailable\n";return 77;
#else
try{
  if(argc!=3){std::cerr<<"fixture and actual map directory required\n";return 77;}
  std::ifstream in(argv[1]);if(!in||!std::filesystem::is_regular_file(std::filesystem::path(argv[2])/"lockout.map"))return 77;
  std::vector<Case> cases;Case current;std::string line;
  while(std::getline(in,line)){
    if(line.empty()||line[0]=='#')continue;std::istringstream s(line);std::string op;s>>op;
    if(op=="SOURCE")continue;
    if(op=="CASE"){std::string entry,arg,callback;s>>current.name>>entry>>arg>>current.kind>>callback;current.entry=hex(entry);current.argument=hex(arg);current.callback=hex(callback);}
    else if(op=="BEFORE"||op=="AFTER"){std::string name,raw;s>>name>>raw;(op=="BEFORE"?current.before:current.after)[name]=bytes(raw);}
    else if(op=="CALL"||op=="STORE")current.events.push_back(line);
    else if(op=="RETURN"){std::string v,b;s>>v>>b;current.returned=hex(v);current.stop=b=="true";}
    else if(op=="END"){cases.push_back(std::move(current));current=Case{};}
    else throw std::runtime_error("Unknown fixture row");
  }
  std::size_t stores=0;for(const auto&c:cases)stores+=run(c,argv[2]);
  std::cout<<cases.size()<<" complete original queue/worker/file buffers; "<<stores<<" ordered stores and all interleaved native service calls PASS\n";
  std::cout<<"Real Lockout/full/multichunk/short/EOF/zero/nonseekable outcomes; source TLS/worker creation/residency remain explicit\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
#endif
}
