#pragma once
#include "halo2/physics_game_arena_native.hpp"
#include "halo2/native_threads.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2::native_arena_schedule_detail {
// Private candidate only: the actual bootstrap and Arena producer are still
// unchanged. This exact fixed body owns only its idle-wait counter/barrier.
struct WorkerIdleBarrier {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned waits=0;
    void before_wait(std::uint32_t handle) {
        if(handle!=0xf3000003u)return; // This owner's actual semaphore namespace.
        std::lock_guard lock(mutex);++waits;changed.notify_all();
    }
    void finish_startup() {
        std::unique_lock lock(mutex);
        // Initial idle wait, then one after each of the two source jobs for
        // each savegame file. A completion byte precedes free-list recycling;
        // wait for the source worker itself to finish before stopping it.
        if(!changed.wait_for(lock,std::chrono::seconds(5),[&]{return waits>=5;}))
            throw std::runtime_error("Native arena worker did not retire its four startup file jobs");
    }
};

class WorkerIdleBeforeWait final {
    const std::shared_ptr<WorkerIdleBarrier> idle_;
    const std::thread::id parent_executor_;
public:
    explicit WorkerIdleBeforeWait(std::shared_ptr<WorkerIdleBarrier> idle)
        :idle_(std::move(idle)),parent_executor_(std::this_thread::get_id()) {
        if(!idle_)throw std::invalid_argument("Native arena idle schedule needs its actual barrier");
    }
    void operator()(std::uint32_t handle,std::stop_token)const {idle_->before_wait(handle);}
    bool preserves_parent_policy()const noexcept {
        return idle_&&parent_executor_==std::this_thread::get_id();
    }
};

// A declared feedback schedule gates the real worker at its actual idle wait.
// It never edits a source completion byte, grants a synchronization permit or
// supplies a file result. The real worker must finish/recycle its job before
// the main caller resumes from its genuine yield boundary.
struct WorkerJobBarrier final {
    std::mutex mutex;
    std::condition_variable_any changed;
    unsigned idle{},epoch{};
    void before_wait(std::uint32_t handle,std::stop_token stop){
        if(handle!=0xf3000003u)return;
        std::unique_lock lock(mutex);++idle;changed.notify_all();const auto seen=epoch;
        if(!changed.wait(lock,stop,[&]{return epoch>seen;}))throw NativeWorkerStopped();
    }
    void before_yield(){
        std::unique_lock lock(mutex);
        if(!changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>0;}))
            throw std::runtime_error("Real arena worker did not reach initial idle");
        const auto target=idle+1u;++epoch;changed.notify_all();
        if(!changed.wait_for(lock,std::chrono::seconds(5),[&]{return idle>=target;}))
            throw std::runtime_error("Real arena worker did not retire its actual file job");
    }
};
class WorkerJobBeforeWait final {
    const std::shared_ptr<WorkerJobBarrier> barrier_;
    const std::thread::id parent_executor_;
public:
    explicit WorkerJobBeforeWait(std::shared_ptr<WorkerJobBarrier> barrier)
        :barrier_(std::move(barrier)),parent_executor_(std::this_thread::get_id()){
        if(!barrier_)throw std::invalid_argument("Arena feedback schedule needs its actual barrier");
    }
    void operator()(std::uint32_t handle,std::stop_token stop)const{barrier_->before_wait(handle,stop);}
    const void*owner()const noexcept{return barrier_.get();}
    bool preserves_parent_policy()const noexcept{return parent_executor_==std::this_thread::get_id();}
};
class WorkerJobBeforeYield final {
    const std::shared_ptr<WorkerJobBarrier> barrier_;
    const std::thread::id parent_executor_;
public:
    explicit WorkerJobBeforeYield(std::shared_ptr<WorkerJobBarrier> barrier)
        :barrier_(std::move(barrier)),parent_executor_(std::this_thread::get_id()){
        if(!barrier_)throw std::invalid_argument("Arena feedback schedule needs its actual barrier");
    }
    void operator()()const{barrier_->before_yield();}
    const void*owner()const noexcept{return barrier_.get();}
    bool preserves_parent_policy()const noexcept{return parent_executor_==std::this_thread::get_id();}
};
inline SourcePhysicsGameArenaSchedule feedback_schedule(const std::shared_ptr<WorkerJobBarrier>&barrier){
    return {WorkerJobBeforeWait{barrier},WorkerJobBeforeYield{barrier}};
}

inline bool preserves_parent_policy(const SourcePhysicsGameArenaSchedule&schedule)noexcept {
    if(schedule.before_main_yield){
        const auto*wait=schedule.before_worker_wait.target<WorkerJobBeforeWait>();
        const auto*yield=schedule.before_main_yield.target<WorkerJobBeforeYield>();
        return wait&&yield&&wait->owner()==yield->owner()&&
            wait->preserves_parent_policy()&&yield->preserves_parent_policy();
    }
    if(!schedule.before_worker_wait)return true;
    const auto*owned=schedule.before_worker_wait.target<WorkerIdleBeforeWait>();
    return owned&&owned->preserves_parent_policy();
}
} // namespace halo2::native_arena_schedule_detail
