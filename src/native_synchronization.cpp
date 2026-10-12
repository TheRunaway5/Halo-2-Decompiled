#include "halo2/native_synchronization.hpp"
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
enum class Kind { Mutex, Semaphore };
struct Object {
  explicit Object(Kind input):kind(input) {}
  virtual ~Object()=default;
  const Kind kind;
};
struct Mutex final:Object {
  Mutex():Object(Kind::Mutex) {}
  std::recursive_timed_mutex native;
  std::mutex ownership;
  std::thread::id owner;
  std::size_t depth{};
};
struct Semaphore final:Object {
  Semaphore(std::int32_t initial,std::int32_t maximum)
      :Object(Kind::Semaphore),count(initial),limit(maximum) {}
  std::mutex native;
  std::condition_variable available;
  std::int32_t count;
  const std::int32_t limit;
};
std::error_code error(std::errc value) {return std::make_error_code(value);}
NativeSynchronizationWait failed(std::errc value) {
  return {NativeSynchronizationWaitState::Failed,error(value)};
}
constexpr std::uint32_t infinite=0xffffffffu;
}
struct NativeSynchronization::State {
  explicit State(const NativeSynchronizationConfiguration &configuration)
      :next(configuration.first_handle_alias),limit(configuration.handle_alias_limit) {
    if(!next||next>=limit||limit>(std::uint64_t{1}<<32))
      throw std::invalid_argument("Native synchronization alias namespace unavailable");
  }
  mutable std::mutex objects_mutex;
  std::uint64_t next;
  const std::uint64_t limit;
  std::map<std::uint32_t,std::shared_ptr<Object>> objects;
  NativeSynchronizationCreation retain(std::shared_ptr<Object> object) {
    std::lock_guard lock(objects_mutex);
    if(next>=limit)throw std::out_of_range("Native synchronization alias namespace exhausted");
    const auto handle=static_cast<std::uint32_t>(next);
    objects.emplace(handle,std::move(object));++next;
    return {handle,{}};
  }
  NativeSynchronizationCreation create_mutex(bool initially_owned) {
    try {
      auto object=std::make_shared<Mutex>();
      std::unique_lock initial(object->native,std::defer_lock);
      if(initially_owned) {
        initial.lock();
        object->owner=std::this_thread::get_id();object->depth=1;
      }
      // A failed retain unwinds the real initial acquisition before freeing
      // the backing. Publish the alias only after ownership is established.
      const auto result=retain(object);
      (void)initial.release();
      return result;
    }catch(const std::system_error &exception){return {0,exception.code()};}
     catch(const std::bad_alloc &){return {0,error(std::errc::not_enough_memory)};}
  }
  NativeSynchronizationCreation create_semaphore(std::int32_t initial,std::int32_t maximum) {
    if(initial<0||maximum<=0||initial>maximum)return {0,error(std::errc::invalid_argument)};
    try {return retain(std::make_shared<Semaphore>(initial,maximum));}
    catch(const std::system_error &exception){return {0,exception.code()};}
    catch(const std::bad_alloc &){return {0,error(std::errc::not_enough_memory)};}
  }
  std::shared_ptr<Object> find(std::uint32_t handle) const {
    std::lock_guard lock(objects_mutex);const auto found=objects.find(handle);
    return found==objects.end()?nullptr:found->second;
  }
  NativeSynchronizationWait wait(std::uint32_t handle,std::uint32_t milliseconds) {
    const auto object=find(handle);
    if(!object)return failed(std::errc::bad_file_descriptor);
    try {
      if(object->kind==Kind::Mutex) {
        const auto mutex=std::static_pointer_cast<Mutex>(object);bool acquired;
        if(milliseconds==infinite){mutex->native.lock();acquired=true;}
        else if(!milliseconds)acquired=mutex->native.try_lock();
        else acquired=mutex->native.try_lock_for(std::chrono::milliseconds(milliseconds));
        if(!acquired)return {NativeSynchronizationWaitState::TimedOut,{}};
        std::lock_guard lock(mutex->ownership);
        mutex->owner=std::this_thread::get_id();++mutex->depth;
      }else {
        const auto semaphore=std::static_pointer_cast<Semaphore>(object);
        std::unique_lock lock(semaphore->native);
        const auto ready=[&]{return semaphore->count!=0;};
        if(milliseconds==infinite)semaphore->available.wait(lock,ready);
        else if(!milliseconds){if(!ready())return {NativeSynchronizationWaitState::TimedOut,{}};}
        else if(!semaphore->available.wait_for(lock,std::chrono::milliseconds(milliseconds),ready))
          return {NativeSynchronizationWaitState::TimedOut,{}};
        --semaphore->count;
      }
      return {NativeSynchronizationWaitState::Acquired,{}};
    }catch(const std::system_error &exception){return {NativeSynchronizationWaitState::Failed,exception.code()};}
  }
  std::error_code release_mutex(std::uint32_t handle) {
    const auto object=find(handle);
    if(!object)return error(std::errc::bad_file_descriptor);
    if(object->kind!=Kind::Mutex)return error(std::errc::invalid_argument);
    const auto mutex=std::static_pointer_cast<Mutex>(object);
    std::lock_guard lock(mutex->ownership);
    if(!mutex->depth||mutex->owner!=std::this_thread::get_id())
      return error(std::errc::operation_not_permitted);
    --mutex->depth;if(!mutex->depth)mutex->owner={};
    mutex->native.unlock();return {};
  }
  NativeSemaphoreRelease release_semaphore(std::uint32_t handle,std::int32_t count) {
    const auto object=find(handle);
    if(!object)return {{},error(std::errc::bad_file_descriptor)};
    if(object->kind!=Kind::Semaphore)return {{},error(std::errc::invalid_argument)};
    const auto semaphore=std::static_pointer_cast<Semaphore>(object);
    std::unique_lock lock(semaphore->native);
    if(count<=0)return {{},error(std::errc::invalid_argument)};
    if(count>semaphore->limit-semaphore->count)
      return {{},error(std::errc::value_too_large)};
    const auto previous=semaphore->count;semaphore->count+=count;
    lock.unlock();semaphore->available.notify_all();return {previous,{}};
  }
};
NativeSynchronization::NativeSynchronization(const NativeSynchronizationConfiguration &configuration)
    :state_(std::make_shared<State>(configuration)) {}
NativeSynchronizationServices NativeSynchronization::services() const {
  const auto state=state_;
  return {
    [state](bool owned){return state->create_mutex(owned);},
    [state](std::int32_t initial,std::int32_t maximum){return state->create_semaphore(initial,maximum);},
    [state](std::uint32_t handle,std::uint32_t timeout){return state->wait(handle,timeout);},
    [state](std::uint32_t handle){return state->release_mutex(handle);},
    [state](std::uint32_t handle,std::int32_t count){return state->release_semaphore(handle,count);}
  };
}
std::size_t NativeSynchronization::object_count() const {
  std::lock_guard lock(state_->objects_mutex);return state_->objects.size();
}
} // namespace halo2
