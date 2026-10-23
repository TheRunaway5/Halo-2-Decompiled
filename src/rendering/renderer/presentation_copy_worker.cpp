#include "rendering/renderer/presentation_copy_worker.hpp"
#include <chrono>
#include <exception>
#include <utility>

namespace halo2 {
struct PresentationCopyWorker::Impl {
    std::uint64_t serial{};
    Result result;
    static std::uint64_t now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    Result finish(std::uint64_t) {return result;}
    void stop()noexcept{}
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
    const auto serial=++impl_->serial;
    const auto begin=Impl::now();copy(source,destination,count);
    impl_->result={std::this_thread::get_id(),count,begin,Impl::now()};
    return Ticket{impl_,serial};
}
}
