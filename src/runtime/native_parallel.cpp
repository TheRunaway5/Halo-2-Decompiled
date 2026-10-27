#include "halo2/native_parallel.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>
#if defined(__x86_64__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if (defined(__x86_64__) || defined(_M_X64)) && !defined(_MSC_VER)
constexpr bool host_fp_state=true;
unsigned sse_csr(){return _mm_getcsr();}
void set_sse_csr(unsigned value){_mm_setcsr(value);}
std::uint16_t x87_control(){std::uint16_t word;asm volatile("fnstcw %0":"=m"(word));return word;}
void set_x87_control(std::uint16_t word){asm volatile("fldcw %0"::"m"(word));}
std::uint16_t x87_status(){std::uint16_t word;asm volatile("fnstsw %0":"=m"(word));return word;}
void clear_x87_flags(){asm volatile("fnclex");}
// Only the sticky exception flags change; tags, TOP, control word and
// pointers keep their live values (SDM Vol.1 Figure 8-9: SW at +4).
void raise_x87_flags(std::uint16_t flags) {
    std::array<std::uint32_t,7> environment;
    asm volatile("fnstenv %0":"=m"(environment)::"memory");
    environment[1]|=flags&0x3fu;
    asm volatile("fldenv %0"::"m"(environment):"memory");
}
#else
constexpr bool host_fp_state=false;
unsigned sse_csr(){return 0;}
void set_sse_csr(unsigned){}
std::uint16_t x87_control(){return 0;}
void set_x87_control(std::uint16_t){}
std::uint16_t x87_status(){return 0;}
void clear_x87_flags(){}
void raise_x87_flags(std::uint16_t){}
#endif
thread_local bool inside_task=false;
const bool reference_serial=[]{const auto* p=std::getenv("HALO2_REFERENCE_SERIAL_NATIVE");return p && *p;}();

struct Job {
    const std::function<void(std::size_t)>* task{};
    std::size_t count{};
    unsigned control_sse{};
    std::uint16_t control_x87{};
    std::atomic<std::size_t> next{0},done{0};
    std::atomic<unsigned> sse_flags{0},x87_flags{0};
    std::vector<std::exception_ptr> errors;
    std::vector<int> errno_values;
    std::vector<char> errno_changed;
};
// Each task leaves errno as it found it; its own change is recorded so the
// caller can apply the last change in task order after the join.
void run_task(Job& job,std::size_t index) {
    const int before=errno;
    try{(*job.task)(index);}catch(...){job.errors[index]=std::current_exception();}
    if(errno!=before){job.errno_values[index]=errno;job.errno_changed[index]=1;}
    errno=before;
}
class Pool {
    std::mutex submit_,mutex_;
    std::condition_variable wake_,finished_;
    std::vector<std::thread> threads_;
    Job job_;
    std::uint64_t generation_{};
    unsigned active_{};
    bool stopping_{};
    void work() {
        inside_task=true;
        std::uint64_t seen=0;
        for(;;) {
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock,[&]{return stopping_ || generation_!=seen;});
                if(stopping_)return;
                seen=generation_;++active_;
            }
            set_sse_csr(job_.control_sse);set_x87_control(job_.control_x87);clear_x87_flags();
            for(std::size_t index;(index=job_.next.fetch_add(1,std::memory_order_relaxed))<job_.count;) {
                run_task(job_,index);
                job_.sse_flags.fetch_or(sse_csr()&0x3fu,std::memory_order_relaxed);
                job_.x87_flags.fetch_or(x87_status()&0x3fu,std::memory_order_relaxed);
                job_.done.fetch_add(1,std::memory_order_release);
            }
            std::lock_guard lock(mutex_);
            if(!--active_)finished_.notify_all();
        }
    }
public:
    Pool() {
        const auto hardware=std::thread::hardware_concurrency();
        const unsigned workers=hardware>1?std::min(hardware-1,7u):0u;
        for(unsigned n=0;n<workers;++n)threads_.emplace_back([this]{work();});
    }
    ~Pool() {
        {std::lock_guard lock(mutex_);stopping_=true;}
        wake_.notify_all();
        for(auto& thread:threads_)thread.join();
    }
    unsigned workers()const noexcept{return unsigned(threads_.size());}
    void run(std::size_t count,const std::function<void(std::size_t)>& task) {
        std::lock_guard serial(submit_);
        {
            // A worker that woke late for the previous job still reads its
            // fields outside the lock; it leaves before they change.
            std::unique_lock lock(mutex_);
            finished_.wait(lock,[&]{return !active_;});
            job_.task=&task;job_.count=count;
            job_.control_sse=sse_csr()&~0x3fu;job_.control_x87=x87_control();
            job_.next.store(0,std::memory_order_relaxed);job_.done.store(0,std::memory_order_relaxed);
            job_.sse_flags.store(0,std::memory_order_relaxed);job_.x87_flags.store(0,std::memory_order_relaxed);
            job_.errors.assign(count,nullptr);job_.errno_values.assign(count,0);job_.errno_changed.assign(count,0);
            ++generation_;
        }
        // Wake only as many workers as there are tasks beyond the caller's
        // own; the job completes on the caller either way, exactly as before.
        for(std::size_t n=0,wakes=std::min<std::size_t>(count-1,threads_.size());n<wakes;++n)wake_.notify_one();
        // The caller takes tasks too; its own sticky flags accumulate in place.
        inside_task=true;
        for(std::size_t index;(index=job_.next.fetch_add(1,std::memory_order_relaxed))<count;) {
            run_task(job_,index);job_.done.fetch_add(1,std::memory_order_release);
        }
        inside_task=false;
        {
            std::unique_lock lock(mutex_);
            finished_.wait(lock,[&]{return job_.done.load(std::memory_order_acquire)==count && !active_;});
        }
        if(const auto flags=job_.sse_flags.load(std::memory_order_relaxed))set_sse_csr(sse_csr()|flags);
        if(const auto flags=std::uint16_t(job_.x87_flags.load(std::memory_order_relaxed));flags&~x87_status())
            raise_x87_flags(flags);
        for(std::size_t index=count;index-->0;)
            if(job_.errno_changed[index]){errno=job_.errno_values[index];break;}
        for(const auto& error:job_.errors)if(error)std::rethrow_exception(error);
    }
};
Pool& pool(){static Pool shared;return shared;}
}
namespace {
class OverlapThread {
    std::mutex mutex_;
    std::condition_variable wake_,done_;
    const std::function<void()>* job_{};
    bool pending_{},finished_{},stopping_{};
    unsigned control_sse_{};std::uint16_t control_x87_{};int errno_before_{};
    unsigned sse_flags_{};std::uint16_t x87_flags_{};int errno_after_{};bool errno_changed_{};
    std::exception_ptr error_;
    std::thread thread_;
    void work() {
        for(;;) {
            std::unique_lock lock(mutex_);
            wake_.wait(lock,[&]{return stopping_ || pending_;});
            if(stopping_)return;
            pending_=false;
            const auto *job=job_;const auto csr=control_sse_;const auto cw=control_x87_;const int before=errno_before_;
            lock.unlock();
            set_sse_csr(csr);set_x87_control(cw);clear_x87_flags();errno=before;
            std::exception_ptr error;
            try{(*job)();}catch(...){error=std::current_exception();}
            const auto flags=sse_csr()&0x3fu;const auto status=std::uint16_t(x87_status()&0x3fu);const int after=errno;
            lock.lock();
            sse_flags_=flags;x87_flags_=status;errno_after_=after;errno_changed_=after!=before;error_=error;finished_=true;
            lock.unlock();
            done_.notify_all();
        }
    }
public:
    OverlapThread():thread_([this]{work();}){}
    ~OverlapThread(){{std::lock_guard lock(mutex_);stopping_=true;}wake_.notify_all();thread_.join();}
    void run(const std::function<void()>& job,const std::function<void()>& overlap) {
        {
            std::lock_guard lock(mutex_);
            job_=&job;control_sse_=sse_csr()&~0x3fu;control_x87_=x87_control();errno_before_=errno;
            pending_=true;finished_=false;error_=nullptr;
        }
        wake_.notify_one();
        const int before_overlap=errno;
        std::exception_ptr overlap_error;
        try{overlap();}catch(...){overlap_error=std::current_exception();}
        const bool overlap_changed=errno!=before_overlap;
        {
            std::unique_lock lock(mutex_);
            done_.wait(lock,[&]{return finished_;});
        }
        if(sse_flags_)set_sse_csr(sse_csr()|sse_flags_);
        if(x87_flags_&~x87_status())raise_x87_flags(x87_flags_);
        if(errno_changed_ && !overlap_changed)errno=errno_after_;
        if(error_)std::rethrow_exception(error_);
        if(overlap_error)std::rethrow_exception(overlap_error);
    }
};
}
void native_overlap_run(const std::function<void()>& job,const std::function<void()>& overlap) {
    if(reference_serial || !host_fp_state){job();overlap();return;}
    static OverlapThread shared;
    shared.run(job,overlap);
}
unsigned native_parallel_workers()noexcept {
    return reference_serial || !host_fp_state?0u:pool().workers();
}
void native_parallel_for(std::size_t count,const std::function<void(std::size_t)>& task) {
    if(count<2 || inside_task || !native_parallel_workers()) {
        std::exception_ptr first;
        for(std::size_t index=0;index<count;++index)
            try{task(index);}catch(...){if(!first)first=std::current_exception();}
        if(first)std::rethrow_exception(first);
        return;
    }
    pool().run(count,task);
}
}
