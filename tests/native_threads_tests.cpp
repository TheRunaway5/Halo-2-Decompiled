#include "halo2/native_threads.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
void require(bool condition,const char *message) {
  if(!condition)throw std::runtime_error(message);
}
void joined(const NativeThreadServices &threads,std::uint32_t handle) {
  require(!threads.join(handle),"Real native worker could not be joined");
}
}
int main() {
  try {
    NativeSynchronization synchronization({0x41000000,0x41000100});
    const auto sync=synchronization.services();
    const auto semaphore=sync.create_semaphore(0,150);
    const auto mutex=sync.create_mutex(true);
    require(semaphore.handle&&mutex.handle&&!semaphore.error&&!mutex.error,
            "Real synchronization backing unavailable");
    NativeThreadServices threads;
    std::uint32_t worker_handle=0;
    std::atomic<unsigned> post_wait_stores=0;
    std::promise<void> entered,acquired;
    auto entered_future=entered.get_future(),acquired_future=acquired.get_future();
    {
      NativeThreads owner({0x42000000,0x42000100});threads=owner.services();
      const auto worker=threads.create([&](std::stop_token stop) {
        entered.set_value();
        acquire_native_synchronization(sync,semaphore.handle,0xffffffffu,stop,2);
        ++post_wait_stores;acquired.set_value();
        acquire_native_synchronization(sync,semaphore.handle,0xffffffffu,stop,2);
        ++post_wait_stores;
      });
      require(worker.handle&&!worker.error&&owner.worker_count()==1,
              "Real worker was not retained");worker_handle=worker.handle;
    }
    // Captured services keep the actual thread alive after facade destruction.
    entered_future.wait();
    require(post_wait_stores==0&&threads.outcome(worker_handle).state==NativeThreadState::Running,
            "An empty semaphore allowed source post-wait writes");
    const auto released=sync.release_semaphore(semaphore.handle,1);
    require(!released.error&&released.previous_count==0,"Real permit release failed");
    acquired_future.wait();
    require(!threads.request_stop(worker_handle),"Host stop request failed");
    joined(threads,worker_handle);
    require(post_wait_stores==1&&threads.outcome(worker_handle).state==NativeThreadState::Stopped,
            "Timeout or shutdown became a completed source acquisition");
    require(sync.wait(semaphore.handle,0).state==NativeSynchronizationWaitState::TimedOut,
            "Actual acquired permit was not consumed");
    auto blocked=threads.create([&](std::stop_token stop) {
      acquire_native_synchronization(sync,mutex.handle,0xffffffffu,stop,2);
      ++post_wait_stores;
      require(!sync.release_mutex(mutex.handle),"Worker mutex release failed");
    });
    require(!threads.request_stop(blocked.handle),"Blocked worker stop failed");
    joined(threads,blocked.handle);
    require(threads.outcome(blocked.handle).state==NativeThreadState::Stopped&&post_wait_stores==1,
            "Stopping a blocked worker granted mutex ownership");
    require(!sync.release_mutex(mutex.handle),"Original actual mutex ownership was lost");
    auto failed=threads.create([&](std::stop_token stop) {
      acquire_native_synchronization(sync,0,0xffffffffu,stop,2);++post_wait_stores;
    });
    joined(threads,failed.handle);const auto failure=threads.outcome(failed.handle);
    require(failure.state==NativeThreadState::Failed&&failure.failure&&post_wait_stores==1,
            "Native wait failure became success");
    bool actual_error=false;
    try{std::rethrow_exception(failure.failure);}
    catch(const std::system_error &error){actual_error=error.code()==std::errc::bad_file_descriptor;}
    require(actual_error,"Actual native failure was not retained");
    auto returned=threads.create([](std::stop_token){});joined(threads,returned.handle);
    require(threads.outcome(returned.handle).state==NativeThreadState::Returned,
            "A returned native task was classified as source shutdown");
    bool timed_out=false;
    try{acquire_native_synchronization(sync,semaphore.handle,0,{},2);}
    catch(const std::system_error &error){timed_out=error.code()==std::errc::timed_out;}
    require(timed_out,"A finite native timeout granted ownership");
    // Stop racing after actual acquisition cannot discard the consumed permit
    // or skip source writes belonging to that acquisition.
    std::promise<void> raced_acquired,proceed;auto raced_future=raced_acquired.get_future();
    auto proceed_future=proceed.get_future();
    require(!sync.release_semaphore(semaphore.handle,1).error,"Race permit release failed");
    auto raced=threads.create([&](std::stop_token stop) {
      acquire_native_synchronization(sync,semaphore.handle,0xffffffffu,stop,2);
      raced_acquired.set_value();proceed_future.wait();++post_wait_stores;
      acquire_native_synchronization(sync,semaphore.handle,0xffffffffu,stop,2);
    });
    raced_future.wait();const auto stopped=threads.request_stop(raced.handle);
    proceed.set_value();joined(threads,raced.handle);
    require(!stopped&&post_wait_stores==2&&threads.outcome(raced.handle).state==NativeThreadState::Stopped,
            "An acquired permit was lost during host teardown");
    std::promise<void> final_entered;auto final_future=final_entered.get_future();
    std::atomic<bool> final_stopped=false;
    {
      NativeThreads final_owner({3,4});
      const auto final_services=final_owner.services();
      require(final_services.create([&](std::stop_token stop) {
        final_entered.set_value();
        try{acquire_native_synchronization(sync,semaphore.handle,0xffffffffu,stop,2);}
        catch(const NativeWorkerStopped &){final_stopped=true;throw;}
      }).handle==3,"Explicit native alias was not used");
      final_future.wait();bool exhausted=false;
      try{(void)final_services.create([](std::stop_token){});}
      catch(const std::out_of_range &){exhausted=true;}
      require(exhausted,"Unavailable alias became a fake native failure");
    }
    require(final_stopped,"Dropping final owner did not stop and join its real worker");
    require(threads.join(0)==std::errc::bad_file_descriptor&&
            threads.request_stop(0)==std::errc::bad_file_descriptor,
            "Unknown native thread alias succeeded");
    bool unavailable=false;
    try{NativeThreads invalid({0,100});}catch(const std::invalid_argument &){unavailable=true;}
    require(unavailable,"Missing native thread namespace was accepted");
    std::cout<<"realWorkers=6 realPermitConsumptions=2 postWaitStores=2 "
             <<"blockedStopAndFailure=noSourceStores retainedServices=yes finalOwner=stopAndJoin "
             <<"sourceThreadTlsPriorityAndNtStatus=separate\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
