#include "halo2/animation_async_worker.hpp"
#include "halo2/native_threads.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>
using namespace halo2;
namespace {
struct PriorityUnavailable{};
std::uint32_t hex(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(hex(s.substr(i,2))));return b;}
std::string num(std::uint32_t v){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(8)<<v;return s.str();}
std::string raw(std::uint32_t v,unsigned n){std::ostringstream s;for(unsigned i=0;i<n;++i)s<<std::hex<<std::setfill('0')<<std::setw(2)<<((v>>(8*i))&255);return s.str();}
struct Memory {
 std::map<std::uint32_t,std::vector<std::byte>> banks;
 std::span<std::byte> span(std::uint32_t p,std::size_t n){auto i=banks.upper_bound(p);if(i==banks.begin())throw std::out_of_range("Missing raw native worker backing");--i;const auto off=std::uint64_t(p)-i->first;if(off>i->second.size()||n>i->second.size()-off)throw std::out_of_range("Raw native worker span escapes");return std::span(i->second).subspan(off,n);}
};
struct Fixture {std::map<std::string,std::vector<std::byte>> before,after;std::vector<std::string> stores;};
Fixture fixture(const char*path){std::ifstream f(path);if(!f)throw std::runtime_error("Source composition fixture missing");Fixture out;std::string line;while(std::getline(f,line)){std::istringstream s(line);std::string op,key,value;s>>op;if(op=="BEFORE"||op=="AFTER"){s>>key>>value;(op=="BEFORE"?out.before:out.after)[key]=bytes(value);}else if(op=="STORE")out.stores.push_back(line);}return out;}
std::vector<std::byte> constructor_input(const char*path){std::ifstream f(path);std::string line;while(std::getline(f,line)){std::istringstream s(line);std::string op,at,value;s>>op>>at>>value;if(op=="MEM"&&at=="004e7924")return bytes(value);}throw std::runtime_error("Actual source constructor input missing");}
void run(const char*source,const char*constructor,const std::filesystem::path&map){
 const auto expected=fixture(source);Memory m;const std::map<std::string,std::uint32_t> addresses{{"bank_hex",0x4e7924},{"current_hex",0x518000},{"done_hex",0xa103000},{"count_hex",0xa103010},{"buffer_hex",0x82947000}};
 for(auto&[name,p]:addresses)m.banks[p]=expected.before.at(name);
 m.banks[0x4e7924]=constructor_input(constructor);m.banks[0x5098a0]=std::vector<std::byte>(12);
 // Actual host-owned storage requested by the original descriptor. Poison is
 // explicitly a host storage input, not an Xbox stack initialization claim.
 m.banks[0x5018a0]=std::vector<std::byte>(0x8000,std::byte{0xcd});
 SourceAnimationResourceMemory memory;
 memory.read=[&](auto p,auto n){return std::span<const std::byte>(m.span(p,n));};
 memory.write=[&](auto p,auto b){auto out=m.span(p,b.size());std::copy(b.begin(),b.end(),out.begin());};
 NativeSynchronization sync({0xf3000001,0xf3000004});const auto native=sync.services();
 auto files=std::make_shared<SourceAsyncOwnedFiles>(std::array<SourceAsyncOwnedFile,1>{{{0xf4000001,map}}});
 std::mutex log_mutex;std::vector<std::string> stores;std::atomic<bool> collect=false;
 SourceAnimationAsyncObserver observe=[&](const SourceAnimationAsyncStore&q){if(!collect.load())return;std::lock_guard lock(log_mutex);stores.push_back("STORE "+num(q.instruction)+' '+num(q.address)+' '+std::to_string(q.bytes)+' '+raw(q.before,q.bytes)+' '+raw(q.after,q.bytes));};
 SourceAnimationAsyncWorkerServices base;base.memory=memory;base.bindings={{0x4e7938,0x4e9eb8,0x4e9ebc,0x4e7928,0x4e792c,0x4e7930,0x4e7924,0x5098a0,0x5018a0,0x122350},0x4e7934,0x4e9ec0,0x518000,0x1a47d0};
 base.wait=[&](auto h,auto ms){acquire_native_synchronization(native,h,ms,{},5);};
 base.release_mutex=[&](auto h){if(auto error=native.release_mutex(h))throw std::system_error(error);};
 base.release_semaphore=[&](auto h,auto n){const auto result=native.release_semaphore(h,n);if(result.error||!result.previous_count)throw std::system_error(result.error?result.error:std::make_error_code(std::errc::io_error));};
 base.yield=[] {std::this_thread::yield();};
 std::vector<SourceAsyncReadExecution> reads;
 base.callback=[&](auto callback,auto payload){if(callback!=0x1a47d0)throw std::runtime_error("Unowned source callback");auto out=read_source_animation_async_file(payload,0x40000,{0x82947000,m.banks.at(0x82947000)},*files,memory,observe);reads.push_back(out);return out.completion.callback_result;};
 std::promise<void> retired;auto complete=retired.get_future();
 // Declared after all captured storage so actual final-owner stop/join occurs
 // before any captured raw/file/promise lifetime is released on exceptions.
 NativeThreads threads({0xf5000001,0xf5000002});const auto workers=threads.services();
 std::uint32_t worker=0;std::error_code creation_error;
 SourceAnimationAsyncConstructionServices ctor;ctor.memory=memory;ctor.bindings=base.bindings.queue;
 ctor.native_create_mutex=[&](auto security,auto initial,auto name){if(security||initial||name)throw std::runtime_error("Unexpected source mutex request");auto out=native.create_mutex(false);if(out.error)throw std::system_error(out.error);return out.handle;};
 ctor.native_create_semaphore=[&](auto security,auto initial,auto maximum,auto name){if(security||initial||maximum!=150||name)throw std::runtime_error("Unexpected source semaphore request");auto out=native.create_semaphore(initial,maximum);if(out.error)throw std::system_error(out.error);return out.handle;};
 ctor.native_create_worker=[&](auto security,auto stack,auto entry,auto context,auto flags,auto tid){
  if(security||stack!=0x4000||entry!=0x122350||context||flags||tid)throw std::runtime_error("Unexpected source worker request");
  const auto out=workers.create([&](std::stop_token stop){
   auto owned=base;owned.wait=[&](auto h,auto ms){acquire_native_synchronization(native,h,ms,stop,5);};bool terminal=false,notified=false;
   owned.callback=[&](auto callback,auto payload){const auto result=base.callback(callback,payload);terminal=result!=0;return result;};
   owned.release_mutex=[&](auto h){base.release_mutex(h);if(terminal&&!notified&&h==0xf3000001){notified=true;retired.set_value();}};
   run_source_animation_async_worker(owned,observe);
  });worker=out.handle;creation_error=out.error;return out.handle;
 };
 ctor.native_set_worker_priority=[&](auto handle,auto priority){if(handle!=worker||priority!=1)throw std::runtime_error("Unexpected source priority request");throw PriorityUnavailable{};};
 bool priority_stop=false;try{construct_source_animation_async_queue(ctor);}catch(const PriorityUnavailable&){priority_stop=true;}
 if(creation_error)throw std::system_error(creation_error);
 if(!priority_stop||worker!=0xf5000001||sync.object_count()!=3||threads.worker_count()!=1)throw std::runtime_error("Actual constructor/native worker boundary differs");
 collect=true;
 const auto id=submit_source_animation_async_read({0xbb4800,0x7a00,0x82947000,0xa103010,0xa103000,7,3},{0xf4000001,0xf4000002,0xf4000003,0xf4000004},base,observe);
 const auto status=complete.wait_for(std::chrono::seconds(5));
 if(auto error=workers.request_stop(worker))throw std::system_error(error);
 if(auto error=workers.join(worker))throw std::system_error(error);
 const auto outcome=workers.outcome(worker);
 if(outcome.state==NativeThreadState::Failed&&outcome.failure)std::rethrow_exception(outcome.failure);
 if(status!=std::future_status::ready||outcome.state!=NativeThreadState::Stopped||id!=0)throw std::runtime_error("Actual native worker completion/stop differs");
 if(reads.size()!=1||reads[0].io.bytes_read!=0x7a00||reads[0].completion.callback_result!=1)throw std::runtime_error("Actual owned-file outcome differs");
 if(stores!=expected.stores){std::size_t i=0;while(i<std::min(stores.size(),expected.stores.size())&&stores[i]==expected.stores[i])++i;throw std::runtime_error("Actual thread source store sequence differs at "+std::to_string(i));}
 for(auto&[name,p]:addresses){auto want=expected.after.at(name);if(name=="bank_hex")for(unsigned i=0;i<4;++i)want[i]=std::byte((worker>>(8*i))&255);if(m.banks.at(p)!=want)throw std::runtime_error("Actual native thread complete buffer differs: "+name);}
 const auto descriptor=m.banks.at(0x5098a0);const std::array<std::byte,12> want{{std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0xa0},std::byte{0x18},std::byte{0x50},std::byte{0},std::byte{0},std::byte{0x80},std::byte{0},std::byte{0}}};
 if(!std::equal(descriptor.begin(),descriptor.end(),want.begin())||!std::all_of(m.banks.at(0x5018a0).begin(),m.banks.at(0x5018a0).end(),[](auto b){return b==std::byte{0xcd};}))throw std::runtime_error("Source worker storage descriptor/backing differs");
 std::cout<<"Actual native thread "+num(worker)+": source request/read/done/unlink/free/stop+join PASS; "<<stores.size()<<" original ordered stores/full buffers; actual bytes="<<reads[0].io.bytes_read<<'\n';
 std::cout<<"Three actual synchronization owners; constructor stops at unsupported source/native priority. Xbox TLS/SEH/stack/priority/file-open/residency remain explicit\n";
}
} // namespace
int main(int argc,char**argv){try{if(argc!=4)return 77;if(!std::filesystem::is_regular_file(argv[3]))return 77;run(argv[1],argv[2],argv[3]);return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
