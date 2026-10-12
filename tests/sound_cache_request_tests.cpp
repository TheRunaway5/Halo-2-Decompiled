#include "halo2/sound_cache_request.hpp"
#include "halo2/animation_async_constructor.hpp"
#include "halo2/animation_async_worker.hpp"
#include "halo2/native_sound_storage.hpp"
#include "halo2/native_synchronization.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
struct Stop{};
void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::uint32_t num(const std::string&s){return std::stoul(s,nullptr,16);}
std::string hex(std::uint32_t v,unsigned n=4){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(n*2)<<(v&(n==4?0xffffffffu:((1u<<(n*8))-1)));return s.str();}
std::vector<std::string> fields(const std::string&s){std::istringstream in(s);std::string v;std::vector<std::string>r;while(std::getline(in,v,'\t'))r.push_back(v);return r;}
std::vector<std::byte> decode(const std::string&s){require(s.size()%2==0,"Odd fixture bytes");std::vector<std::byte>b(s.size()/2);for(std::size_t k=0;k<b.size();++k)b[k]=std::byte(num(s.substr(k*2,2)));return b;}
struct Stage{std::string name;std::uint32_t entry,result;bool returned;std::map<std::uint32_t,std::vector<std::byte>>before,after;std::vector<std::string>events;};
struct Case{std::string name;std::uint32_t sound,chunk;SoundCacheRequestState state;std::vector<Stage>stages;};
std::string call(const std::string&name,std::initializer_list<std::uint32_t>args){std::string out="CALL\t"+name;for(auto n:args)out+='\t'+hex(n);return out;}
struct Count{std::size_t stages=0,stores=0,bytes=0,reads=0;};
Count run(const Case &c,const std::filesystem::path&maps){
 NativeSoundStorage storage({0x480d68,0xb000000,0xc000000});
 const auto &regions=c.stages.front().before;std::vector<std::pair<std::uint64_t,std::uint64_t>> ranges;
 for(const auto&[a,b]:regions)ranges.emplace_back(a&~4095u,(std::uint64_t(a)+b.size()+4095)&~std::uint64_t(4095));
 std::ranges::sort(ranges);
 std::uint64_t lo=0,hi=0;for(auto[a,b]:ranges){if(a<=hi){hi=std::max(hi,b);continue;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);lo=a;hi=b;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);
 auto memory=[&](std::uint32_t a,std::size_t n){auto it=regions.upper_bound(a);require(it!=regions.begin(),"Unowned cache address");--it;require(std::uint64_t(a)+n<=std::uint64_t(it->first)+it->second.size(),"Unowned cache extent");return storage.memory(a,n);};
 for(const auto&[a,b]:regions)std::ranges::copy(b,memory(a,b.size()).begin());
 SourceAnimationResourceMemory raw;raw.read=[&](auto a,auto n){return std::span<const std::byte>(memory(a,n));};raw.write=[&](auto a,auto b){std::ranges::copy(b,memory(a,b.size()).begin());};
 std::vector<std::string>actual;Count count;
 auto store=[&](std::uint32_t pc,std::uint32_t a,unsigned n,std::uint32_t before,std::uint32_t after){++count.stores;actual.push_back("STORE\t"+hex(pc)+'\t'+hex(a)+'\t'+std::to_string(n)+'\t'+hex(before,n)+'\t'+hex(after,n));};
 auto queue_observe=[&](const SourceAnimationAsyncStore&q){store(q.instruction,q.address,q.bytes,q.before,q.after);};
 auto cache_observe=[&](const SoundCacheStartupStore&q){store(q.instruction,q.address,q.bytes,q.before,q.after);};
 NativeSynchronization sync({0xf3100001,0xf3100004});auto native=sync.services();
 SourceAnimationAsyncBindings binding{0x4e7938,0x4e9eb8,0x4e9ebc,0x4e7928,0x4e792c,0x4e7930,0x4e7924,0x5098a0,0x5018a0,0x122350};
 SourceAnimationAsyncConstructionServices ctor;ctor.memory=raw;ctor.bindings=binding;
 ctor.native_create_mutex=[&](auto security,auto owner,auto name){auto result=native.create_mutex(owner!=0);require(!result.error&&result.handle,"Actual mutex create failure");actual.push_back(call("native_create_mutex",{security,owner,name,result.handle}));return result.handle;};
 ctor.native_create_semaphore=[&](auto security,auto initial,auto maximum,auto name){auto result=native.create_semaphore(initial,maximum);require(!result.error&&result.handle,"Actual semaphore create failure");actual.push_back(call("native_create_semaphore",{security,initial,maximum,name,result.handle}));return result.handle;};
 ctor.native_create_worker=[](auto,auto,auto,auto,auto,auto)->std::uint32_t{throw Stop{};};
 SourceAnimationAsyncWorkerServices worker;worker.memory=raw;worker.bindings={binding,0x4e7934,0x4e9ec0,0x518000,0x1a47d0};
 worker.wait=[&](auto handle,auto ms){auto result=native.wait(handle,0);if(result.state==NativeSynchronizationWaitState::TimedOut)throw Stop{};require(result.state==NativeSynchronizationWaitState::Acquired&&!result.error,"Actual wait failure");actual.push_back(call("native_wait_acquired",{handle,ms}));};
 worker.release_mutex=[&](auto handle){require(!native.release_mutex(handle),"Actual mutex release failure");actual.push_back(call("native_release_mutex",{handle}));};
 worker.release_semaphore=[&](auto handle,auto n){auto result=native.release_semaphore(handle,n);require(!result.error&&result.previous_count,"Actual semaphore release failure");actual.push_back(call("native_release_semaphore",{handle,n,std::uint32_t(*result.previous_count)}));};
 worker.yield=[]{throw Stop{};};
 const std::array<SourceAsyncOwnedFile,1> filebindings{{{0xf4100001,maps/"shared.map"}}};SourceAsyncOwnedFiles files(filebindings);
 worker.callback=[&](auto callback,auto payload){require(callback==0x1a47d0,"Unowned callback");auto result=read_source_animation_async_file(payload,0x40000,{c.state.physical_buffer,memory(c.state.physical_buffer,0x300000)},files,raw,queue_observe,[&](const SourceAsyncReadExecution&q){actual.push_back(call("native_file_seek",{q.step.file,q.step.offset,0,q.io.seek_low}));if(q.io.seek_low!=0xffffffffu){actual.push_back(call("native_file_read",{q.step.file,q.step.buffer,q.step.size,q.io.bytes_read}));++count.reads;}});return result.completion.callback_result;};
 SoundCacheRequestServices cache;cache.memory=memory;cache.async_read=[&](const SourceBitmapReadRequest&r){submit_source_animation_async_read(r,{0,0,0xf4100001,0},worker,queue_observe);};cache.yield=[] {throw std::runtime_error("Unexpected original cache blocking yield");};
 for(const auto&s:c.stages){
  for(const auto&[a,b]:s.before)require(std::ranges::equal(b,memory(a,b.size())),"Source phase before bytes differ");
  actual.clear();bool stopped=false;std::uint32_t returned=0;
  try{
   if(s.name=="complete_queue_constructor_prefix")construct_source_animation_async_queue(ctor,[&](const AnimationResourceStore&q){store(q.instruction,q.address,4,q.before,q.after);});
   else if(s.name=="actual_cache_request_publishes_read")returned=source_sound_cache_request(c.state,c.sound,c.chunk,2,{0x21aaf0,0x21aab0},cache,cache_observe);
   else if(s.name=="real_owned_file_worker_completion")run_source_animation_async_worker(worker,queue_observe);
   else {const auto flags=std::stoul(s.name.substr(s.name.find_last_of('_')+1));returned=source_sound_cache_request(c.state,c.sound,c.chunk,flags,{0x21aaf0,0x21aab0},cache,cache_observe);}
  }catch(const Stop&){stopped=true;}
  const bool expected_stop=s.name=="complete_queue_constructor_prefix"||s.name=="real_owned_file_worker_completion";
  require(stopped==expected_stop,"Phase stop differs");if(!stopped)require(returned==s.result,"Original cache return differs");
  if(actual!=s.events){std::size_t k=0;while(k<std::min(actual.size(),s.events.size())&&actual[k]==s.events[k])++k;std::cerr<<c.name<<' '<<s.name<<" firstevent "<<k<<" actual "<<(k<actual.size()?actual[k]:"END")<<" expected "<<(k<s.events.size()?s.events[k]:"END")<<'\n';throw std::runtime_error("Cache interleaved event differs");}
  for(const auto&[a,b]:s.after){require(std::ranges::equal(b,memory(a,b.size())),"Source phase full bytes differ");count.bytes+=b.size();}++count.stages;
 }
 return count;
}
}
int main(int argc,char**argv)try{
 require(argc==3,"Usage: sound_cache_request_tests <fixtures.tsv> <map-directory>");std::ifstream in(argv[1]);require(bool(in),"Original fixtures unavailable");std::string line;std::getline(in,line);require(line=="SOUND_CACHE_REQUEST_NATIVE_V1","Fixture schema");std::vector<Case>cases;Case c;Stage s;
 while(std::getline(in,line)){const auto q=fields(line);if(q.empty())continue;
  if(q[0]=="CASE"){c={q[1],num(q[2]),num(q[3]),{num(q[4]),num(q[5]),num(q[6])},{}};}
  else if(q[0]=="STAGE"){s={q[1],num(q[2]),num(q[3]),q[4]=="true",{},{},{}};}
  else if(q[0]=="BEFORE"||q[0]=="AFTER")(q[0]=="BEFORE"?s.before:s.after).emplace(num(q[1]),decode(q[2]));
  else if(q[0]=="CALL"||q[0]=="STORE")s.events.push_back(line);
  else if(q[0]=="ENDSTAGE"){c.stages.push_back(std::move(s));s={};}
  else if(q[0]=="ENDCASE"){cases.push_back(std::move(c));c={};}
  else throw std::runtime_error("Unknown original fixture row");
 }
 Count total;for(const auto&x:cases){const auto n=run(x,argv[2]);total.stages+=n.stages;total.stores+=n.stores;total.bytes+=n.bytes;total.reads+=n.reads;}
 require(cases.size()==3&&total.stages==18&&total.reads==3,"Incomplete cache request suite");
 std::cout<<"PASS "<<cases.size()<<" original cache requests; "<<total.stages<<" full phases; "<<total.stores<<" ordered stores; "<<total.bytes<<" whole bytes; "<<total.reads<<" actual owned shared.map reads; no admission/voice/thread claim.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
