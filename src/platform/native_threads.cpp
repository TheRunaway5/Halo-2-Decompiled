#include "halo2/native_threads.hpp"
#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2 {
const char *NativeWorkerStopped::what() const noexcept {
  return "Native worker stopped before synchronization acquisition";
}
struct NativeThreads::State {
  struct Worker {
    std::mutex outcome_mutex,join_mutex;
    NativeThreadOutcome outcome{NativeThreadState::Running,{}};
    std::jthread thread;
    std::stop_source stop;
  };
  mutable std::mutex mutex;
  std::uint64_t next,limit;
  std::map<std::uint32_t,std::shared_ptr<Worker>> workers;
  explicit State(const NativeThreadConfiguration &configuration)
      :next(configuration.first_handle_alias),limit(configuration.handle_alias_limit) {
    if(!next||limit<=next||limit>0x100000000ull)
      throw std::invalid_argument("Native thread alias namespace unavailable");
  }
  ~State() {
    for(auto &[alias,worker]:workers)worker->stop.request_stop();
    for(auto &[alias,worker]:workers)if(worker->thread.joinable())worker->thread.join();
  }
  std::shared_ptr<Worker> find(std::uint32_t alias) const {
    std::lock_guard lock(mutex);
    const auto found=workers.find(alias);
    return found==workers.end()?nullptr:found->second;
  }
  NativeThreadCreation create(NativeThreadTask task) {
    if(!task)throw std::invalid_argument("Native worker task unavailable");
    std::lock_guard lock(mutex);
    if(next>=limit)throw std::out_of_range("Native thread alias namespace exhausted");
    auto worker=std::make_shared<Worker>();
    const auto alias=static_cast<std::uint32_t>(next);
    // Allocate retained storage before native thread creation. Failed native
    // creation removes the unpublished record and preserves the alias cursor.
    workers.emplace(alias,worker);
    try {
      worker->thread=std::jthread([target=worker.get(),task=std::move(task)](std::stop_token stop) {
        NativeThreadOutcome result{NativeThreadState::Returned,{}};
        try {task(stop);}
        catch(const NativeWorkerStopped &) {result.state=NativeThreadState::Stopped;}
        catch(...) {result={NativeThreadState::Failed,std::current_exception()};}
        std::lock_guard outcome_lock(target->outcome_mutex);
        target->outcome=result;
      });
      worker->stop=worker->thread.get_stop_source();
    }catch(const std::system_error &error) {
      workers.erase(alias);return {0,error.code()};
    }catch(...) {workers.erase(alias);throw;}
    ++next;
    return {alias,{}};
  }
};
NativeThreads::NativeThreads(const NativeThreadConfiguration &configuration)
    :state_(std::make_shared<State>(configuration)) {}
NativeThreadServices NativeThreads::services() const {
  const auto state=state_;
  return {
    [state](NativeThreadTask task){return state->create(std::move(task));},
    [state](std::uint32_t alias)->std::error_code {
      const auto worker=state->find(alias);
      if(!worker)return std::make_error_code(std::errc::bad_file_descriptor);
      worker->stop.request_stop();return {};
    },
    [state](std::uint32_t alias)->std::error_code {
      const auto worker=state->find(alias);
      if(!worker)return std::make_error_code(std::errc::bad_file_descriptor);
      std::lock_guard lock(worker->join_mutex);
      try {if(worker->thread.joinable())worker->thread.join();return {};}
      catch(const std::system_error &error) {return error.code();}
    },
    [state](std::uint32_t alias) {
      const auto worker=state->find(alias);
      if(!worker)throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor));
      std::lock_guard lock(worker->outcome_mutex);return worker->outcome;
    }
  };
}
std::size_t NativeThreads::worker_count() const {
  std::lock_guard lock(state_->mutex);return state_->workers.size();
}
void acquire_native_synchronization(const NativeSynchronizationServices &services,
    std::uint32_t handle,std::uint32_t milliseconds,std::stop_token stop,
    std::uint32_t polling_milliseconds) {
  if(!services.wait)throw std::invalid_argument("Native wait service unavailable");
  if(!polling_milliseconds||polling_milliseconds==0xffffffffu)
    throw std::invalid_argument("Native stop polling interval unavailable");
  using Clock=std::chrono::steady_clock;
  const auto deadline=Clock::now()+std::chrono::milliseconds(milliseconds);
  for(;;) {
    if(stop.stop_requested())throw NativeWorkerStopped();
    auto slice=polling_milliseconds;
    if(milliseconds!=0xffffffffu) {
      const auto remaining=std::chrono::ceil<std::chrono::milliseconds>(deadline-Clock::now()).count();
      slice=remaining<=0?0:static_cast<std::uint32_t>(std::min<std::int64_t>(remaining,slice));
    }
    const auto result=services.wait(handle,slice);
    if(result.state==NativeSynchronizationWaitState::Acquired) {
      if(result.error)throw std::logic_error("Native acquisition returned a failure");
      return;
    }
    if(result.state==NativeSynchronizationWaitState::Failed)
      throw std::system_error(result.error?result.error:std::make_error_code(std::errc::io_error));
    if(result.error)throw std::system_error(result.error);
    if(milliseconds!=0xffffffffu&&Clock::now()>=deadline)
      throw std::system_error(std::make_error_code(std::errc::timed_out));
  }
}
} // namespace halo2
