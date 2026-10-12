#include "halo2/native_execution_gate.hpp"
#include <array>
#include <atomic>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F> void rejected(F fn){bool threw=false;try{fn();}catch(const std::logic_error&){threw=true;}require(threw,"Missing execution lease rejection");}
struct Failure{};
void run(){
 NativeExecutionGate gate,other;
 rejected([&]{gate.require_owned();});rejected([&]{gate.suspend([]{});});
 gate.execute([&]{
  gate.require_owned();rejected([&]{other.require_owned();});
  rejected([&]{gate.execute([]{});});rejected([&]{other.execute([]{});});
  rejected([&]{gate.require_outside_entry();});
  gate.suspend([&]{rejected([&]{gate.require_owned();});rejected([&]{gate.execute([]{});});rejected([&]{gate.require_outside_entry();});std::this_thread::yield();});
  gate.require_owned();
 });gate.require_outside_entry();
 bool propagated=false;try{gate.execute([&]{gate.suspend([]{throw Failure{};});throw std::runtime_error("Post-failure source step admitted");});}catch(const Failure&){propagated=true;}
 require(propagated,"Native provider failure replaced");gate.require_outside_entry();
 rejected([&]{gate.execute([&]{
  try{gate.suspend([]{throw Failure{};});}catch(const Failure&){}
  rejected([&]{gate.require_owned();});
  rejected([&]{gate.suspend([]{});});
  rejected([&]{gate.execute([]{});});
  rejected([&]{other.execute([]{});});
 });});
 std::promise<void> suspended,release;auto ready=suspended.get_future(),proceed=release.get_future();
 std::atomic<bool> crossed=false;
 std::jthread worker([&]{gate.execute([&]{gate.suspend([&]{suspended.set_value();proceed.wait();});gate.require_owned();require(crossed.load(),"Suspended gate excluded real other entry");});});
 ready.get();gate.execute([&]{crossed=true;});release.set_value();gate.require_outside_entry();worker.join();
 std::uint32_t count=0;std::vector<std::jthread> writers;
 for(unsigned i=0;i<4;++i)writers.emplace_back([copy=gate,&count]{for(unsigned j=0;j<500;++j)copy.execute([&]{copy.require_owned();++count;});});
 for(auto &thread:writers){gate.require_outside_entry();thread.join();}
 require(count==2000,"Actual execution entries raced");
 std::cout<<"PASS native execution gate: nonrecursive ownership/revocation/suspension/retention; 2000 actual protected writes\n";
}
} // namespace
int main(){try{run();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
