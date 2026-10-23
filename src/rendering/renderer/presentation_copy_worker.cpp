#include "rendering/renderer/presentation_copy_worker.hpp"
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <utility>

namespace halo2 {
struct PresentationCopyWorker::Impl {
    std::mutex mutex;
    std::condition_variable ready,done;
    std::uint64_t serial{};
    bool occupied{},pending{},complete{},stopping{};
    const void *source{};
    void *destination{};
    std::size_t count{};
    Copy copy{};
    Result result;
    std::exception_ptr error;
    std::thread thread;
    Impl():thread([this]{run();}){}
    ~Impl(){stop();}
    static std::uint64_t now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    void run()noexcept {
        std::unique_lock lock(mutex);
        for(;;) {
            ready.wait(lock,[&]{return pending || stopping;});
            if(!pending)return;
            const auto job_source=source;auto *job_destination=destination;
            const auto job_count=count;const auto job_copy=copy;
            pending=false;
            lock.unlock();
            const auto begin=now();std::exception_ptr failure;
            try{job_copy(job_source,job_destination,job_count);}catch(...){failure=std::current_exception();}
            const Result finished{std::this_thread::get_id(),job_count,begin,now()};
            lock.lock();
            result=finished;error=failure;complete=true;
            done.notify_all();
        }
    }
    Result finish(std::uint64_t expected) {
        std::unique_lock lock(mutex);
        if(!occupied || serial!=expected)throw FormatError("presentation copy ticket differs from its outstanding job");
        done.wait(lock,[&]{return complete;});
        const auto finished=result;const auto failure=error;
        occupied=false;source=nullptr;destination=nullptr;copy=nullptr;
        lock.unlock();
        if(failure)std::rethrow_exception(failure);
        return finished;
    }
    void stop()noexcept {
        {
            std::lock_guard lock(mutex);stopping=true;
        }
        ready.notify_one();
        if(thread.joinable())thread.join();
    }
};
PresentationCopyWorker::Ticket::Ticket(std::shared_ptr<Impl> p,std::uint64_t n):impl_(std::move(p)),serial_(n){}
PresentationCopyWorker::Ticket::Ticket(Ticket&& b)noexcept:impl_(std::move(b.impl_)),serial_(b.serial_){}
auto PresentationCopyWorker::Ticket::operator=(Ticket&& b)noexcept->Ticket& {
    if(this!=&b){join();impl_=std::move(b.impl_);serial_=b.serial_;}return *this;
}
PresentationCopyWorker::Ticket::~Ticket(){join();}
auto PresentationCopyWorker::Ticket::finish()->Result {
    auto p=std::exchange(impl_,{});return p?p->finish(serial_):Result{};
}
void PresentationCopyWorker::Ticket::join()noexcept {
    if(impl_)try{(void)finish();}catch(...){/* Explicit finish observes errors; teardown still joins. */}
}
PresentationCopyWorker::PresentationCopyWorker():impl_(std::make_shared<Impl>()){}
PresentationCopyWorker::~PresentationCopyWorker(){impl_->stop();}
auto PresentationCopyWorker::enqueue(const void *source,void *destination,std::size_t count,Copy copy)->Ticket {
    std::unique_lock lock(impl_->mutex);
    if(impl_->stopping)throw FormatError("presentation copy worker has stopped");
    if(impl_->occupied)throw FormatError("presentation copy still owns an outstanding ticket");
    const auto serial=++impl_->serial;
    impl_->source=source;impl_->destination=destination;impl_->count=count;impl_->copy=copy;
    impl_->occupied=true;impl_->pending=true;impl_->complete=false;impl_->error=nullptr;
    lock.unlock();impl_->ready.notify_one();
    return Ticket{impl_,serial};
}
}
