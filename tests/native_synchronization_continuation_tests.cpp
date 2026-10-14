#include "halo2/native_synchronization.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
constexpr auto acquired=NativeSynchronizationWaitState::Acquired;
constexpr auto timeout=NativeSynchronizationWaitState::TimedOut;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f) {
  bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}
  check(rejected,"Invalid or nonquiescent synchronization import was accepted");
}
void release(const NativeSynchronizationServices &services,std::uint32_t mutex) {
  check(!services.release_mutex(mutex),"Real mutex release failed");
}
}
int main() try {
  NativeSynchronization source({0xf6000001u,0xf6010000ull}),target({0xf6000001u,0xf6010000ull});
  const auto original=source.services(),resumed=target.services();
  const auto mutex=original.create_mutex(false).handle;
  const auto semaphore=original.create_semaphore(3,7).handle;
  check(original.wait(semaphore,0).state==acquired,"Initial semaphore consumption failed");
  const auto saved=source.checkpoint();
  const auto old_mutex=resumed.create_mutex(false).handle;
  const auto old=target.checkpoint();
  {
    auto staged=target.prepare_restore(saved);
    check(!source.commit_restore(std::move(staged)),"Foreign owner accepted staged synchronization");
    auto moved=std::move(staged);
    check(moved.for_owner(target) && !staged.for_owner(target),"Prepared synchronization move lost owner");
  }
  check(target.checkpoint()==old,"Cancelled staging changed target synchronization");
  auto bad=saved;bad.version=2;rejects([&]{target.prepare_restore(bad);});
  bad=saved;++bad.first;rejects([&]{target.prepare_restore(bad);});
  bad=saved;++bad.limit;rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.next=bad.limit+1;rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.objects.pop_back();rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.objects[1].handle=bad.objects[0].handle;rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.objects[1].count=8;rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.objects[1].count=-1;rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.objects[1].limit=0;rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.objects[0].count=1;rejects([&]{target.prepare_restore(bad);});
  bad=saved;bad.objects[1].kind=static_cast<NativeSynchronization::ObjectKind>(99);
  rejects([&]{target.prepare_restore(bad);});
  check(target.checkpoint()==old,"Rejected staging changed target synchronization");
  check(resumed.wait(old_mutex,0).state==acquired,"Target mutex setup failed");
  rejects([&]{target.checkpoint();});rejects([&]{target.prepare_restore(saved);});
  release(resumed,old_mutex);
  auto staged=target.prepare_restore(saved);
  check(target.commit_restore(std::move(staged)) && !target.commit_restore(std::move(staged)),"Prepared synchronization publication was reusable");
  check(target.checkpoint()==saved,"Imported synchronization aliases/counts differ");
  // Services captured before import must resolve new real native objects.
  for(const auto *services:{&original,&resumed}) {
    check(services->wait(mutex,0).state==acquired && services->wait(mutex,0).state==acquired,"Restored recursive mutex failed");
    auto wrong_thread=std::async(std::launch::async,[&] {
      const auto wrong=services->release_mutex(mutex);
      const auto waited=services->wait(mutex,1);
      if(waited.state==acquired)release(*services,mutex);
      return wrong==std::errc::operation_not_permitted && waited.state==timeout;
    });
    check(wrong_thread.get(),"Imported mutex lost actual thread ownership/exclusion");
    release(*services,mutex);release(*services,mutex);
    check(services->wait(semaphore,0).state==acquired && services->wait(semaphore,0).state==acquired &&
        services->wait(semaphore,0).state==timeout,"Imported semaphore fabricated or lost permits");
    const auto filled=services->release_semaphore(semaphore,7);
    check(filled.previous_count==0 && !filled.error,"Imported semaphore lost its maximum");
    check(services->release_semaphore(semaphore,1).error==std::errc::value_too_large,"Imported semaphore overflow succeeded");
    for(int i=0;i<7;++i)check(services->wait(semaphore,0).state==acquired,"Imported semaphore permit missing");
  }
  check(source.checkpoint()==target.checkpoint(),"Future real synchronization execution diverged");
  // A live infinite wait is an outstanding native owner. Reject immediately
  // rather than deadlocking export or silently transferring the waiter.
  std::promise<void> entered;auto ready=entered.get_future();
  auto waiter=std::async(std::launch::async,[&] {
    entered.set_value();return resumed.wait(semaphore,0xffffffffu);
  });
  ready.wait();bool observed_busy=false;
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(std::chrono::steady_clock::now()<deadline && !observed_busy) {
    try{(void)target.checkpoint();}catch(const std::runtime_error &){observed_busy=true;}
    std::this_thread::yield();
  }
  const auto released=resumed.release_semaphore(semaphore,1);
  const auto waited=waiter.get();
  check(observed_busy && !released.error && waited.state==acquired,"Active wait was exported or could not finish after rejection");
  check(source.checkpoint()==target.checkpoint(),"Rejected waiter snapshot changed semaphore continuation");
  for(int i=0;i<3;++i) {
    const auto state=source.checkpoint();auto restore=target.prepare_restore(state);
    check(target.commit_restore(std::move(restore)),"Repeated synchronization handoff failed");
    check(original.create_mutex(false).handle==resumed.create_mutex(false).handle,"Imported handle cursor diverged");
  }
  NativeSynchronization exhausted({0xffffffffu,0x100000000ull}),exhausted_copy({0xffffffffu,0x100000000ull});
  const auto end=exhausted.services();check(end.create_mutex(false).handle==0xffffffffu,"Last 32-bit alias unavailable");
  auto last=exhausted_copy.prepare_restore(exhausted.checkpoint());
  check(exhausted_copy.commit_restore(std::move(last)),"Exhausted handle namespace import failed");
  rejects([&]{exhausted_copy.services().create_mutex(false);});
  std::cout<<"PASS real mutex/semaphore continuation, captured services, staged rejection/cancellation, active-wait/held-mutex guards, repeated restore and 32-bit alias exhaustion\n";
  return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
