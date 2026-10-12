#include "halo2/native_synchronization.hpp"
#include <array>
#include <atomic>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using namespace halo2;
constexpr auto acquired=NativeSynchronizationWaitState::Acquired;
constexpr auto timeout=NativeSynchronizationWaitState::TimedOut;
void require(bool condition,const char *message) {
  if(!condition)throw std::runtime_error(message);
}
void lock(const NativeSynchronizationServices &services,std::uint32_t handle) {
  const auto result=services.wait(handle,0xffffffffu);
  require(result.state==acquired&&!result.error,"Native acquisition failed");
}
void unlock(const NativeSynchronizationServices &services,std::uint32_t handle) {
  require(!services.release_mutex(handle),"Native release failed");
}
}
int main() {
  try {
    NativeSynchronizationServices services;
    std::array<std::uint32_t,3> handles;
    {
      NativeSynchronization owner({0x40000000u,0x40000100ull});
      services=owner.services();
      const auto first=services.create_mutex(false),second=services.create_mutex(true);
      const auto semaphore=services.create_semaphore(0,150);
      require(first.handle&&second.handle&&semaphore.handle&&
              !first.error&&!second.error&&!semaphore.error,"Native startup objects unavailable");
      handles={first.handle,second.handle,semaphore.handle};
      require(first.handle!=second.handle&&second.handle!=semaphore.handle&&
              first.handle!=semaphore.handle&&owner.object_count()==3,"Native object aliases overlap");
      require(services.wait(second.handle,0).state==acquired,"Recursive native acquisition failed");
      unlock(services,second.handle); // One original initial acquisition remains.
      auto blocked=std::async(std::launch::async,[&] {
        const auto wrong_owner=services.release_mutex(second.handle);
        const auto timed=services.wait(second.handle,1);
        if(timed.state==acquired)unlock(services,second.handle);
        return wrong_owner==std::errc::operation_not_permitted&&timed.state==timeout;
      });
      require(blocked.get(),"Mutex ownership or genuine contention was lost");
      unlock(services,second.handle);
      require(services.wait(semaphore.handle,0).state==timeout,"Empty semaphore was acquired");
      const auto invalid=services.create_semaphore(151,150);
      require(!invalid.handle&&invalid.error==std::errc::invalid_argument&&owner.object_count()==3,
              "Invalid semaphore published a handle");
    }
    // The captured services own the actual objects after facade destruction.
    const auto mutex=handles[0],semaphore=handles[2];
    std::uint32_t protected_counter=0;
    std::array<std::thread,4> workers;
    for(auto &worker:workers)worker=std::thread([&] {
      for(unsigned i=0;i<200;++i){lock(services,mutex);++protected_counter;unlock(services,mutex);}
    });
    for(auto &worker:workers)worker.join();
    require(protected_counter==800,"Actual mutex failed concurrent exclusion");
    std::promise<void> entering;auto entered=entering.get_future();
    auto waiter=std::async(std::launch::async,[&] {
      entering.set_value();return services.wait(semaphore,0xffffffffu);
    });
    entered.wait();
    const auto released=services.release_semaphore(semaphore,1);
    require(!released.error&&released.previous_count==0,"Semaphore previous count differs");
    require(waiter.get().state==acquired,"Real waiter was not released");
    require(services.wait(semaphore,0).state==timeout,"Acquired semaphore count was not consumed");
    const auto full=services.release_semaphore(semaphore,150);
    require(!full.error&&full.previous_count==0,"Bounded semaphore could not reach actual maximum");
    const auto overflow=services.release_semaphore(semaphore,1);
    require(overflow.error==std::errc::value_too_large&&!overflow.previous_count,
            "Failed semaphore release changed output");
    for(unsigned i=0;i<150;++i)require(services.wait(semaphore,0).state==acquired,"Permit was lost");
    require(services.wait(semaphore,0).state==timeout,"Failed release increased the count");
    require(services.release_mutex(semaphore)==std::errc::invalid_argument&&
            services.release_semaphore(mutex,1).error==std::errc::invalid_argument&&
            services.wait(0,0).error==std::errc::bad_file_descriptor,
            "Missing handles or wrong kinds succeeded");
    bool unavailable=false;
    try{NativeSynchronization invalid({0,0x100});}catch(const std::invalid_argument &){unavailable=true;}
    require(unavailable,"Missing alias namespace was accepted");
    NativeSynchronization limited({7,8});const auto limited_services=limited.services();
    require(limited_services.create_mutex(true).handle==7,"Actual limited object was not retained");
    unlock(limited_services,7);unavailable=false;
    try{(void)limited_services.create_mutex(true);}catch(const std::out_of_range &){unavailable=true;}
    require(unavailable&&limited.object_count()==1,"Missing alias space became a fake Source failure");
    std::cout<<"nativeMutexes=2 boundedSemaphoreMaximum=150 concurrentProtectedWrites=800 "
             <<"realPermitConsumptions=151 retainedServices=yes sourceNtStatusTlsThreadAndIo=separate\n";
    return 0;
  }catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
