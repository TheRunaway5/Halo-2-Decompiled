#pragma once
#include <algorithm>
#include <cerrno>
#include <exception>
#include <limits>
#include <map>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <dispatch/dispatch.h>
#include <fcntl.h>
#include <sys/event.h>
#include <time.h>
#include <unistd.h>

namespace halo2::native_events {
inline void failure(const char *operation){throw std::system_error(errno,std::generic_category(),operation);}
inline std::uint64_t clock_nanoseconds(Clock clock){
    timespec value{};
    // libc++ steady_clock includes sleep. Dispatch's relative timer uses Mach
    // uptime, so its deadline queries and the source interrupt clock must use
    // CLOCK_UPTIME_RAW instead of borrowing that different epoch/domain.
    if(::clock_gettime(clock==Clock::Realtime?CLOCK_REALTIME:CLOCK_UPTIME_RAW,&value))
        failure("query Darwin native timer clock");
    return std::uint64_t(value.tv_sec)*1000000000ull+std::uint64_t(value.tv_nsec);
}
inline void descriptor_flags(int fd){
    const auto flags=::fcntl(fd,F_GETFL);
    if(flags<0 || ::fcntl(fd,F_SETFD,FD_CLOEXEC)<0 || ::fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0)
        failure("configure Darwin event descriptor");
}
class Signal {
    int reader_=-1,writer_=-1;
public:
    Signal(){int pipe[2];if(::pipe(pipe))failure("create Darwin event pipe");
        try{descriptor_flags(pipe[0]);descriptor_flags(pipe[1]);}catch(...){::close(pipe[0]);::close(pipe[1]);throw;}
        reader_=pipe[0];writer_=pipe[1];}
    ~Signal(){::close(reader_);::close(writer_);}
    Signal(const Signal&)=delete;
    Signal& operator=(const Signal&)=delete;
    int descriptor()const{return reader_;}
    void signal(){const std::uint64_t one=1;ssize_t n;do{n=::write(writer_,&one,sizeof(one));}while(n<0&&errno==EINTR);
        if(n!=sizeof(one))failure("signal Darwin event pipe");}
    std::optional<std::uint64_t> take(){std::uint64_t total=0;
        for(;;){std::uint64_t count{};ssize_t n;do{n=::read(reader_,&count,sizeof(count));}while(n<0&&errno==EINTR);
            if(n<0 && errno==EAGAIN)break;if(n<0)failure("consume Darwin event pipe");
            if(n!=sizeof(count)||!count||count>UINT64_MAX-total)throw std::runtime_error("Darwin event receipt is incomplete");
            total+=count;}
        return total?std::optional(total):std::nullopt;}
};
class Timer {
    Signal receipt_;
    const Clock clock_;
    dispatch_queue_t queue_=nullptr;
    dispatch_source_t source_=nullptr;
    dispatch_semaphore_t cancelled_=nullptr;
    bool armed_=false;
    std::uint64_t deadline_{};
    std::exception_ptr fault_;
    std::uint64_t now()const{return clock_nanoseconds(clock_);}
    template<class F>void synchronize(F &&function)const {
        struct Call {F &function;std::exception_ptr error;};Call call{function,{}};
        ::dispatch_sync_f(queue_,&call,[](void *context){auto &c=*static_cast<Call*>(context);
            try{c.function();}catch(...){c.error=std::current_exception();}});
        if(call.error)std::rethrow_exception(call.error);
    }
    void require_live()const{if(fault_)std::rethrow_exception(fault_);}
    static std::uint64_t nanoseconds(std::uint64_t units){
        if(!units || units>std::uint64_t(INT64_MAX)/100)throw std::invalid_argument("Darwin timer deadline exceeds its nanosecond clock domain");
        return units*100;
    }
    bool set(std::uint64_t deadline,dispatch_time_t start){bool previous{};
        if(start==DISPATCH_TIME_FOREVER)throw std::invalid_argument("Darwin timer deadline became infinite");
        synchronize([&]{require_live();previous=armed_&&deadline_>now();
            ::dispatch_source_set_timer(source_,start,DISPATCH_TIME_FOREVER,0);
            (void)receipt_.take();deadline_=deadline;armed_=true;});return previous;
    }
public:
    explicit Timer(Clock clock):clock_(clock){
        queue_=::dispatch_queue_create("halo2.native-timer",DISPATCH_QUEUE_SERIAL);
        if(!queue_)throw std::runtime_error("Darwin timer queue allocation failed");
        cancelled_=::dispatch_semaphore_create(0);
        if(!cancelled_){::dispatch_release(queue_);throw std::runtime_error("Darwin timer cancellation allocation failed");}
        source_=::dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,DISPATCH_TIMER_STRICT,queue_);
        if(!source_){::dispatch_release(cancelled_);::dispatch_release(queue_);throw std::runtime_error("Darwin timer source allocation failed");}
        ::dispatch_set_context(source_,this);
        ::dispatch_source_set_event_handler_f(source_,[](void *context){auto &timer=*static_cast<Timer*>(context);
            // set_timer clears pending data from the old schedule. Its calls
            // and this handler share one serial queue, fencing any in-flight
            // handler before re-arm/cancel. Trust the native expiry receipt:
            // comparing wall time again could discard it after a clock jump.
            try{if(timer.armed_){timer.receipt_.signal();timer.armed_=false;}}
            catch(...){timer.fault_=std::current_exception();timer.armed_=false;}});
        ::dispatch_source_set_cancel_handler_f(source_,[](void *context){auto &timer=*static_cast<Timer*>(context);
            ::dispatch_semaphore_signal(timer.cancelled_);});
        ::dispatch_source_set_timer(source_,DISPATCH_TIME_FOREVER,DISPATCH_TIME_FOREVER,0);
        ::dispatch_activate(source_);
    }
    ~Timer(){
        ::dispatch_sync_f(queue_,this,[](void *context){auto &timer=*static_cast<Timer*>(context);
            timer.armed_=false;::dispatch_source_cancel(timer.source_);});
        // Cancel handler runs after every in-flight native callback. Its signal
        // fences receipt storage before the pipe and callback context disappear.
        ::dispatch_semaphore_wait(cancelled_,DISPATCH_TIME_FOREVER);
        ::dispatch_release(source_);::dispatch_release(cancelled_);::dispatch_release(queue_);
    }
    Timer(const Timer&)=delete;
    Timer& operator=(const Timer&)=delete;
    int descriptor()const{return receipt_.descriptor();}
    bool arm_relative_100ns(std::uint64_t duration){
        if(clock_!=Clock::Monotonic)throw std::invalid_argument("Darwin relative timer has the wrong clock owner");
        const auto interval=nanoseconds(duration);const auto current=now();
        if(current>UINT64_MAX-interval)throw std::invalid_argument("Darwin monotonic timer deadline overflow");
        return set(current+interval,::dispatch_time(DISPATCH_TIME_NOW,std::int64_t(interval)));
    }
    bool arm_realtime_100ns(std::uint64_t time){
        if(clock_!=Clock::Realtime)throw std::invalid_argument("Darwin wall timer has the wrong clock owner");
        const auto deadline=nanoseconds(time);timespec absolute{};
        absolute.tv_sec=deadline/1000000000ull;absolute.tv_nsec=deadline%1000000000ull;
        return set(deadline,::dispatch_walltime(&absolute,0));
    }
    bool cancel(){bool previous{};
        synchronize([&]{require_live();previous=armed_&&deadline_>now();
            ::dispatch_source_set_timer(source_,DISPATCH_TIME_FOREVER,DISPATCH_TIME_FOREVER,0);
            armed_=false;deadline_=0;(void)receipt_.take();});return previous;
    }
    std::optional<std::uint64_t> take(){std::optional<std::uint64_t> result;
        synchronize([&]{require_live();result=receipt_.take();});return result;}
    std::uint64_t remaining_100ns()const {std::uint64_t result{};
        synchronize([&]{require_live();const auto current=now();result=armed_&&deadline_>current?(deadline_-current)/100:0;});return result;}
    bool disarmed()const{bool result{};
        synchronize([&]{require_live();result=!armed_||deadline_<=now();});return result;}
};
class Poller {
    int fd_;
    std::map<int,std::uint64_t> watched_;
public:
    Poller():fd_(::kqueue()){if(fd_<0)failure("create Darwin event dispatcher");
        if(::fcntl(fd_,F_SETFD,FD_CLOEXEC)<0){const auto error=errno;::close(fd_);throw std::system_error(error,std::generic_category(),"configure Darwin dispatcher");}}
    ~Poller(){::close(fd_);}
    Poller(const Poller&)=delete;
    Poller& operator=(const Poller&)=delete;
    int descriptor()const{return fd_;}
    void add(int descriptor,std::uint64_t tag){
        if(!watched_.emplace(descriptor,tag).second)throw std::system_error(EEXIST,std::generic_category(),"duplicate Darwin event registration");
        struct kevent change;EV_SET(&change,descriptor,EVFILT_READ,EV_ADD|EV_ENABLE,0,0,reinterpret_cast<void*>(std::uintptr_t(tag)));
        if(::kevent(fd_,&change,1,nullptr,0,nullptr)){watched_.erase(descriptor);failure("register Darwin event");}}
    void remove(int descriptor){struct kevent change;EV_SET(&change,descriptor,EVFILT_READ,EV_DELETE,0,0,nullptr);
        if(::kevent(fd_,&change,1,nullptr,0,nullptr))failure("disconnect Darwin event");watched_.erase(descriptor);}
    bool ready(std::uint64_t tag)const{std::vector<struct kevent> events(std::max<std::size_t>(watched_.size(),1));const timespec zero{};int n;
        do{n=::kevent(fd_,nullptr,0,events.data(),int(events.size()),&zero);}while(n<0&&errno==EINTR);
        if(n<0)failure("poll Darwin events");
        for(int i=0;i<n;++i){if(events[i].flags&EV_ERROR)throw std::system_error(int(events[i].data),std::generic_category(),"Darwin event dispatcher");
            if(std::uintptr_t(events[i].udata)==tag)return true;}return false;}
};
}
