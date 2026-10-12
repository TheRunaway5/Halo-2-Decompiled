#include "halo2/animation_async_worker.hpp"
#include "halo2/native_execution_gate.hpp"
#include "halo2/native_sound_storage.hpp"
#include "halo2/native_threads.hpp"
#include "halo2/sound_cache_request.hpp"
#include "halo2/sound_create.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace halo2;
namespace {
struct PriorityUnavailable{};
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t number(const std::string&s){return std::stoul(s,nullptr,16);}
std::string hex(std::uint32_t v,unsigned n=4){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(n*2)<<(v&(n==4?0xffffffffu:((1u<<(n*8))-1)));return s.str();}
std::vector<std::string> fields(const std::string &s){std::istringstream in(s);std::string v;std::vector<std::string> out;while(std::getline(in,v,'\t'))out.push_back(v);return out;}
std::vector<std::byte> bytes(const std::string &s){require(s.size()%2==0,"Odd original fixture bytes");std::vector<std::byte> out(s.size()/2);for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(number(s.substr(i*2,2)));return out;}
struct Stage{std::string name;std::uint32_t result;std::map<std::uint32_t,std::vector<std::byte>> before,after;std::vector<std::string> stores;};
struct Case{std::string name;std::uint32_t sound,chunk;SoundCacheRequestState state;std::vector<Stage> stages;SoundCreateState create_state;SoundCreateConstants create_constants;std::vector<std::byte> raw_request;std::uint32_t association,promotion;};
std::vector<Case> load(const char *path){
 std::ifstream in(path);require(bool(in),"Frozen source cache fixture missing");std::string line;std::getline(in,line);require(line=="SOUND_CREATE_CACHE_NATIVE_V1","Original fixture schema differs");std::vector<Case> out;Case c;Stage s;
 while(std::getline(in,line)){const auto q=fields(line);if(q.empty())continue;
  if(q[0]=="CREATE"){require(q.size()==16,"Complete original create ABI missing");c={};c.create_state={number(q[1]),number(q[2]),number(q[3]),number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),number(q[10]),number(q[11]),number(q[12])};c.raw_request=bytes(q[13]);c.association=number(q[14]);c.promotion=number(q[15]);}
  else if(q[0]=="CONSTANTS"){require(q.size()==12,"Complete original create operands missing");c.create_constants={number(q[1]),number(q[2]),number(q[3]),number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),number(q[10]),number(q[11])};}
  else if(q[0]=="CASE"){c.name=q[1];c.sound=number(q[2]);c.chunk=number(q[3]);c.state={number(q[4]),number(q[5]),number(q[6])};}
  else if(q[0]=="STAGE")s={q[1],number(q[3]),{},{},{}};
  else if(q[0]=="BEFORE"||q[0]=="AFTER")(q[0]=="BEFORE"?s.before:s.after).emplace(number(q[1]),bytes(q[2]));
  else if(q[0]=="STORE")s.stores.push_back(line);
  else if(q[0]=="ENDSTAGE"){c.stages.push_back(std::move(s));s={};}
  else if(q[0]=="ENDCASE"){out.push_back(std::move(c));c={};}
  else require(q[0]=="CALL","Unknown original fixture row");
 }return out;
}
struct Result{std::size_t bytes=0,source_stores=0,cache_yields=0,reads=0;};
Result run(const Case &c,const std::filesystem::path &shared){
 NativeExecutionGate gate;NativeSoundStorage storage({0x480d68,0xb000000,0xc000000});
 const auto &regions=c.stages.at(0).before;
 auto memory=[&](std::uint32_t a,std::size_t n){
  gate.require_owned();auto i=regions.upper_bound(a);require(i!=regions.begin(),"Missing owned cache region");--i;
  // The constructor's requested host backing has a separate explicit owner.
  if(a>=0x5018a0&&std::uint64_t(a)+n<=0x5098a0)return storage.memory(a,n);
  require(std::uint64_t(a)+n<=std::uint64_t(i->first)+i->second.size(),"Cache region extent escapes");return storage.memory(a,n);
 };
 gate.execute([&]{
  std::vector<std::pair<std::uint64_t,std::uint64_t>> ranges;
  for(const auto &[a,b]:regions)ranges.emplace_back(a&~4095u,(std::uint64_t(a)+b.size()+4095)&~std::uint64_t(4095));
  ranges.emplace_back(0x501000,0x50a000);std::ranges::sort(ranges);std::uint64_t lo=0,hi=0;
  for(auto [a,b]:ranges){if(a<=hi){hi=std::max(hi,b);continue;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);lo=a;hi=b;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);
  for(const auto &[a,b]:regions)std::ranges::copy(b,memory(a,b.size()).begin());
  std::ranges::fill(memory(0x5018a0,0x8000),std::byte{0xcd});
 });
 bool outside_rejected=false;try{(void)memory(c.state.entry_pool,4);}catch(const std::logic_error&){outside_rejected=true;}require(outside_rejected,"Raw cache span escaped execution ownership");
 SourceAnimationResourceMemory raw;raw.read=[&](auto a,auto n){return std::span<const std::byte>(memory(a,n));};raw.write=[&](auto a,auto b){std::ranges::copy(b,memory(a,b.size()).begin());};
 std::vector<std::string> constructor_stores,request_stores,worker_stores,pending_stores,blocking_stores,completed_stores;std::vector<std::string> *cache_output=&request_stores;Result result;
 auto record=[&](std::vector<std::string> &out,std::uint32_t pc,std::uint32_t a,unsigned n,std::uint32_t before,std::uint32_t after){gate.require_owned();out.push_back("STORE\t"+hex(pc)+'\t'+hex(a)+'\t'+std::to_string(n)+'\t'+hex(before,n)+'\t'+hex(after,n));++result.source_stores;};
 auto queue_observe=[&](const SourceAnimationAsyncStore &s){record(request_stores,s.instruction,s.address,s.bytes,s.before,s.after);};
 auto worker_observe=[&](const SourceAnimationAsyncStore &s){record(worker_stores,s.instruction,s.address,s.bytes,s.before,s.after);};
 auto cache_observe=[&](const SoundCacheStartupStore &s){record(*cache_output,s.instruction,s.address,s.bytes,s.before,s.after);};
 NativeSynchronization synchronization({0xf3100001,0xf3100004});const auto native=synchronization.services();
 SourceAsyncOwnedFiles files(std::array<SourceAsyncOwnedFile,1>{{{0xf4100001,shared}}});
 std::promise<void> retired;auto retirement=retired.get_future();
 // Threads are destroyed first, so final-owner cancellation/join cannot outlive
 // any memory, gate, file, synchronization, observer or promise captured below.
 NativeThreads threads({0xf5100001,0xf5100002});const auto workers=threads.services();
 SourceAnimationAsyncBindings binding{0x4e7938,0x4e9eb8,0x4e9ebc,0x4e7928,0x4e792c,0x4e7930,0x4e7924,0x5098a0,0x5018a0,0x122350};
 SourceAnimationAsyncWorkerServices worker;worker.memory=raw;worker.bindings={binding,0x4e7934,0x4e9ec0,0x518000,0x1a47d0};
 worker.wait=[&](auto handle,auto ms){gate.suspend([&]{acquire_native_synchronization(native,handle,ms,{},5);});};
 worker.release_mutex=[&](auto h){gate.require_owned();if(auto e=native.release_mutex(h))throw std::system_error(e);};
 worker.release_semaphore=[&](auto h,auto n){gate.require_owned();const auto q=native.release_semaphore(h,n);if(q.error||!q.previous_count)throw std::system_error(q.error?q.error:std::make_error_code(std::errc::io_error));};
 worker.yield=[&]{gate.suspend([]{std::this_thread::yield();});};
 worker.callback=[&](auto callback,auto payload){
  gate.require_owned();require(callback==0x1a47d0,"Missing actual source file callback owner");
  const auto io=read_source_animation_async_file(payload,0x40000,{c.state.physical_buffer,memory(c.state.physical_buffer,0x300000)},files,raw,worker_observe);
  require(io.io.bytes_read==io.step.size,"Unexpected actual shared-file transfer");++result.reads;return io.completion.callback_result;
 };
 SourceAnimationAsyncConstructionServices ctor;ctor.memory=raw;ctor.bindings=binding;std::uint32_t thread=0;
 ctor.native_create_mutex=[&](auto security,auto owner,auto name){gate.require_owned();require(!security&&!owner&&!name,"Original mutex request differs");const auto q=native.create_mutex(false);if(q.error)throw std::system_error(q.error);return q.handle;};
 ctor.native_create_semaphore=[&](auto security,auto count,auto maximum,auto name){gate.require_owned();require(!security&&!count&&maximum==150&&!name,"Original semaphore request differs");const auto q=native.create_semaphore(count,maximum);if(q.error)throw std::system_error(q.error);return q.handle;};
 ctor.native_create_worker=[&](auto security,auto stack,auto entry,auto context,auto flags,auto output){
  gate.require_owned();require(!security&&stack==0x4000&&entry==0x122350&&!context&&!flags&&!output,"Original worker request differs");
  const auto q=workers.create([&](std::stop_token stop){
   auto owned=worker;bool terminal=false,notified=false;
   owned.wait=[&](auto handle,auto ms){gate.suspend([&]{acquire_native_synchronization(native,handle,ms,stop,5);});};
   owned.callback=[&](auto callback,auto payload){const auto value=worker.callback(callback,payload);terminal=value!=0;return value;};
   owned.release_mutex=[&](auto h){worker.release_mutex(h);if(terminal&&!notified&&h==0xf3100001){notified=true;retired.set_value();}};
   gate.execute([&]{run_source_animation_async_worker(owned,worker_observe);});
  });if(q.error)throw std::system_error(q.error);thread=q.handle;return q.handle;
 };
 ctor.native_set_worker_priority=[&](auto h,auto priority){gate.require_owned();require(h==thread&&priority==1,"Original priority request differs");throw PriorityUnavailable{};};
 bool priority_stop=false;try{gate.execute([&]{construct_source_animation_async_queue(ctor,[&](const AnimationResourceStore&s){record(constructor_stores,s.instruction,s.address,4,s.before,s.after);});});}catch(const PriorityUnavailable&){priority_stop=true;}
 require(priority_stop&&thread==0xf5100001,"Missing explicit actual-thread/priority boundary");
 auto compare=[&](const Stage &stage){gate.require_owned();for(const auto &[a,b]:stage.after){
  auto actual=memory(a,b.size());for(std::size_t i=0;i<b.size();++i){const auto wanted=a==0x4e7924&&i<4?std::byte(thread>>(i*8)):b[i];if(actual[i]!=wanted)throw std::runtime_error("Complete source cache region differs at "+hex(a+std::uint32_t(i)));}result.bytes+=b.size();
 }};
 gate.execute([&]{
  require(constructor_stores.size()==c.stages[0].stores.size()+1,"Constructor store count differs");
  require(std::equal(c.stages[0].stores.begin(),c.stages[0].stores.end(),constructor_stores.begin()),"Constructor source store prefix differs");compare(c.stages[0]);
 });
 SoundCacheRequestServices cache;cache.memory=memory;
 cache.async_read=[&](const SourceBitmapReadRequest&r){gate.require_owned();submit_source_animation_async_read(r,{0,0,0xf4100001,0},worker,queue_observe);};
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
 cache.yield=[&]{gate.require_owned();++result.cache_yields;gate.suspend([&]{
  const auto outcome=workers.outcome(thread);
  if(outcome.state==NativeThreadState::Failed&&outcome.failure)std::rethrow_exception(outcome.failure);
  if(std::chrono::steady_clock::now()>=deadline)throw std::runtime_error("Native test observation deadline, no source completion supplied");
  std::this_thread::yield();
 });};
 const SoundCacheStartupConstants constants{0x21aaf0,0x21aab0};
 SoundCreateServices create;create.memory=memory;
 create.cache_request=[&](auto sound,auto chunk,auto flags){gate.require_owned();return source_sound_cache_request(c.state,sound,chunk,flags,constants,cache,cache_observe);};
 // Effect/controller callbacks remain absent. Complete SourceCreate must throw
 // if these genuine inputs reach an unavailable owner; no success defaults.
 gate.execute([&]{
  require(std::ranges::equal(c.raw_request,memory(0xa400000,160)),"Retained source160-byte request differs");
  const auto created=source_sound_create(c.create_state,c.sound,c.association,0xa400000,c.promotion,c.create_constants,{0x027f,0x1f80},create,[&](const SoundStartupStore &s){record(request_stores,s.instruction,s.address,s.bytes,s.before,s.after);});
  require(created==c.stages[1].result,"Whole original source-create return differs");
  require(result.reads==0&&worker_stores.empty(),"Worker changed pending creation before its caller return");
  require(request_stores==c.stages[1].stores,"Whole source-create/publication/pending pin stores differ");compare(c.stages[1]);
  cache_output=&pending_stores;require(source_sound_cache_request(c.state,c.sound,c.chunk,0,constants,cache,cache_observe)==1,"Gated pending cache observation differs");
  require(pending_stores.size()==1&&pending_stores[0]==c.stages[3].stores[0],"Pending source touch store differs");
  cache_output=&blocking_stores;require(source_sound_cache_request(c.state,c.sound,c.chunk,1,constants,cache,cache_observe)==2,"Concurrent source completion observation differs");
  require(blocking_stores==c.stages[3].stores,"Blocking source completion stores differ");
  // Completion was read by the source cache loop while the immortal worker is
  // still alive, before native retirement observation, stop or join.
  require(result.cache_yields>0,"Source blocking yield never admitted live worker");
 });
 gate.require_outside_entry();require(retirement.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Actual worker did not retire source node");
 gate.execute([&]{require(worker_stores==c.stages[2].stores,"Concurrent worker source stores differ");compare(c.stages[3]);
  for(unsigned k=4;k<6;++k){cache_output=&completed_stores;completed_stores.clear();const auto flags=k==4?4:5;require(source_sound_cache_request(c.state,c.sound,c.chunk,flags,constants,cache,cache_observe)==c.stages[k].result,"Completed cache flags differ");require(completed_stores==c.stages[k].stores,"Completed cache ordered stores differ");compare(c.stages[k]);}
 });
 std::size_t retired_store_count=0;gate.execute([&]{retired_store_count=result.source_stores;});
 gate.require_outside_entry();if(auto e=workers.request_stop(thread))throw std::system_error(e);if(auto e=workers.join(thread))throw std::system_error(e);
 const auto outcome=workers.outcome(thread);if(outcome.state==NativeThreadState::Failed&&outcome.failure)std::rethrow_exception(outcome.failure);require(outcome.state==NativeThreadState::Stopped,"Actual worker did not stop at wait boundary");
 gate.execute([&]{require(result.source_stores==retired_store_count,"Cancelled wait admitted source post-wait stores");compare(c.stages[5]);});
 require(result.reads==1,"Actual owned-file callback count differs");
 std::cout<<"PASS "<<c.name<<": whole source-create pending pin before IO; concurrent pending1/blocking2/pin6/6, full original regions="<<result.bytes<<", original stores="<<result.source_stores<<", actual reads=1, before-stop completion, gated yield observed\n";
 return result;
}
} // namespace
int main(int argc,char **argv){try{if(argc!=3||!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;const auto cases=load(argv[1]);require(cases.size()==3,"Missing actual original cases");for(const auto &c:cases)run(c,argv[2]);std::cout<<"Named native serialization and explicit captured SourceCreate ABI only; no original admission/voice/FP-route/TLS/priority/file-open/residency or live Unit/Q claim\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
