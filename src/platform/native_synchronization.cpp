#include "halo2/native_synchronization.hpp"
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <shared_mutex>
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
      :first(configuration.first_handle_alias),next(first),limit(configuration.handle_alias_limit) {
    if(!next||next>=limit||limit>(std::uint64_t{1}<<32))
      throw std::invalid_argument("Native synchronization alias namespace unavailable");
  }
  mutable std::mutex objects_mutex;
  mutable std::shared_mutex operation_gate;
  const std::uint32_t first;
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
    std::shared_lock operation(operation_gate);
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
    std::shared_lock operation(operation_gate);
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
    std::shared_lock operation(operation_gate);
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
    std::shared_lock operation(operation_gate);
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
    std::shared_lock operation(operation_gate);
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
  // Both the exclusive operation gate and objects_mutex must be held.
  void require_unowned_mutexes() const {
    for(const auto &[handle,object]:objects) {
      if(object->kind==Kind::Mutex && std::static_pointer_cast<Mutex>(object)->depth)
        throw std::runtime_error("Native synchronization continuation requires released mutexes");
    }
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
struct NativeSynchronization::PreparedRestore::Impl {
  std::shared_ptr<State> state;
  std::unique_lock<std::shared_mutex> operation;
  std::unique_lock<std::mutex> registry;
  std::uint64_t next{};
  std::map<std::uint32_t,std::shared_ptr<Object>> objects;
  explicit Impl(std::shared_ptr<State> state)
      :state(std::move(state)),operation(this->state->operation_gate,std::try_to_lock),
       registry(this->state->objects_mutex,std::defer_lock) {
    if(!operation.owns_lock())throw std::runtime_error("Native synchronization continuation has active calls or waits");
    registry.lock();this->state->require_unowned_mutexes();
  }
};
NativeSynchronization::PreparedRestore::PreparedRestore()=default;
NativeSynchronization::PreparedRestore::~PreparedRestore()=default;
NativeSynchronization::PreparedRestore::PreparedRestore(PreparedRestore &&) noexcept=default;
NativeSynchronization::PreparedRestore &NativeSynchronization::PreparedRestore::operator=(PreparedRestore &&) noexcept=default;
bool NativeSynchronization::PreparedRestore::for_owner(const NativeSynchronization &owner) const noexcept {
  return impl_ && impl_->state==owner.state_;
}
NativeSynchronization::Checkpoint NativeSynchronization::checkpoint() const {
  std::unique_lock operation(state_->operation_gate,std::try_to_lock);
  if(!operation.owns_lock())throw std::runtime_error("Native synchronization checkpoint has active calls or waits");
  std::lock_guard registry(state_->objects_mutex);state_->require_unowned_mutexes();
  Checkpoint saved{1,state_->first,state_->next,state_->limit,{}};
  saved.objects.reserve(state_->objects.size());
  for(const auto &[handle,object]:state_->objects) {
    if(object->kind==Kind::Mutex)saved.objects.push_back({handle,ObjectKind::Mutex,0,0});
    else {
      const auto semaphore=std::static_pointer_cast<Semaphore>(object);
      saved.objects.push_back({handle,ObjectKind::Semaphore,semaphore->count,semaphore->limit});
    }
  }
  return saved;
}
NativeSynchronization::PreparedRestore NativeSynchronization::prepare_restore(const Checkpoint &saved) const {
  if(saved.version!=1 || saved.first!=state_->first || saved.limit!=state_->limit ||
      saved.next<saved.first || saved.next>saved.limit || saved.objects.size()!=saved.next-saved.first)
    throw std::invalid_argument("Invalid native synchronization checkpoint namespace");
  PreparedRestore prepared;prepared.impl_=std::make_unique<PreparedRestore::Impl>(state_);
  prepared.impl_->next=saved.next;
  auto expected=std::uint64_t(saved.first);
  for(const auto &object:saved.objects) {
    if(object.handle!=expected++)throw std::invalid_argument("Invalid native synchronization checkpoint alias");
    if(object.kind==ObjectKind::Mutex) {
      if(object.count || object.limit)throw std::invalid_argument("Mutex checkpoint cannot carry semaphore counts");
      prepared.impl_->objects.emplace(object.handle,std::make_shared<Mutex>());
    } else if(object.kind==ObjectKind::Semaphore) {
      if(object.count<0 || object.limit<=0 || object.count>object.limit)
        throw std::invalid_argument("Invalid native semaphore checkpoint count");
      prepared.impl_->objects.emplace(object.handle,std::make_shared<Semaphore>(object.count,object.limit));
    } else throw std::invalid_argument("Invalid native synchronization checkpoint kind");
  }
  return prepared;
}
bool NativeSynchronization::commit_restore(PreparedRestore &&prepared) noexcept {
  if(!prepared.for_owner(*this))return false;
  state_->objects.swap(prepared.impl_->objects);state_->next=prepared.impl_->next;
  prepared.impl_.reset();return true;
}
} // namespace halo2
