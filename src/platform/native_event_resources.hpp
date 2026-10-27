#pragma once
#include <cstdint>
#include <optional>

namespace halo2::native_events {enum class Clock {Monotonic,Realtime};}
#if defined(__APPLE__)
#include "macos/native_event_resources.hpp"
#elif defined(_WIN32)
#include "platform/windows_event_resources.hpp"
#elif defined(__linux__)
#include <algorithm>
#include <cerrno>
#include <map>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace halo2::native_events {
inline void failure(const char *operation){throw std::system_error(errno,std::generic_category(),operation);}
class Signal {
    int fd_;
public:
    Signal():fd_(::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC)){if(fd_<0)failure("create native event");}
    ~Signal(){::close(fd_);}
    Signal(const Signal&)=delete;
    Signal& operator=(const Signal&)=delete;
    int descriptor()const{return fd_;}
    void signal(){const std::uint64_t one=1;ssize_t n;do{n=::write(fd_,&one,sizeof(one));}while(n<0&&errno==EINTR);
        if(n!=sizeof(one))failure("signal native event");}
    std::optional<std::uint64_t> take(){std::uint64_t count{};ssize_t n;
        do{n=::read(fd_,&count,sizeof(count));}while(n<0&&errno==EINTR);
        if(n<0&&errno==EAGAIN)return {};if(n<0)failure("consume native event");
        if(n!=sizeof(count)||!count)throw std::runtime_error("Native event receipt is incomplete");return count;}
};
class Timer {
    const Clock clock_;
    int fd_;
    bool set(std::uint64_t value,int flags){itimerspec next{},before{};
        next.it_value.tv_sec=value/10000000ull;next.it_value.tv_nsec=(value%10000000ull)*100;
        if(::timerfd_settime(fd_,flags,&next,&before))failure("set native one-shot timer");
        return before.it_value.tv_sec||before.it_value.tv_nsec;}
public:
    explicit Timer(Clock clock):clock_(clock),fd_(::timerfd_create(clock==Clock::Realtime?CLOCK_REALTIME:CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC)){
        if(fd_<0)failure("create native one-shot timer");}
    ~Timer(){::close(fd_);}
    Timer(const Timer&)=delete;
    Timer& operator=(const Timer&)=delete;
    int descriptor()const{return fd_;}
    bool arm_relative_100ns(std::uint64_t duration){if(!duration||clock_!=Clock::Monotonic)throw std::invalid_argument("Native relative timer requires its monotonic owner and a nonzero interval");return set(duration,0);}
    bool arm_realtime_100ns(std::uint64_t time){if(!time||clock_!=Clock::Realtime)throw std::invalid_argument("Native absolute timer requires its realtime owner and a nonzero Unix deadline");return set(time,TFD_TIMER_ABSTIME);}
    bool cancel(){return set(0,0);}
    std::optional<std::uint64_t> take(){std::uint64_t count{};ssize_t n;
        do{n=::read(fd_,&count,sizeof(count));}while(n<0&&errno==EINTR);
        if(n<0&&errno==EAGAIN)return {};if(n<0)failure("consume native timer expiration");
        if(n!=sizeof(count)||!count)throw std::runtime_error("Native timer receipt is incomplete");return count;}
    std::uint64_t remaining_100ns()const{itimerspec value{};if(::timerfd_gettime(fd_,&value))failure("query native timer");
        return std::uint64_t(value.it_value.tv_sec)*10000000ull+std::uint64_t(value.it_value.tv_nsec)/100;}
    bool disarmed()const{itimerspec value{};if(::timerfd_gettime(fd_,&value))failure("verify native timer retirement");
        return !value.it_value.tv_sec&&!value.it_value.tv_nsec&&!value.it_interval.tv_sec&&!value.it_interval.tv_nsec;}
};
class Poller {
    int fd_;
    std::map<int,std::uint64_t> watched_;
public:
    Poller():fd_(::epoll_create1(EPOLL_CLOEXEC)){if(fd_<0)failure("create native event dispatcher");}
    ~Poller(){::close(fd_);}
    Poller(const Poller&)=delete;
    Poller& operator=(const Poller&)=delete;
    int descriptor()const{return fd_;}
    void add(int descriptor,std::uint64_t tag){
        if(!watched_.emplace(descriptor,tag).second)throw std::system_error(EEXIST,std::generic_category(),"duplicate native event registration");
        epoll_event event{};event.events=EPOLLIN;event.data.u64=tag;
        if(::epoll_ctl(fd_,EPOLL_CTL_ADD,descriptor,&event)){watched_.erase(descriptor);failure("register native event");}}
    void remove(int descriptor){if(::epoll_ctl(fd_,EPOLL_CTL_DEL,descriptor,nullptr))failure("disconnect native event");watched_.erase(descriptor);}
    bool ready(std::uint64_t tag)const{std::vector<epoll_event> events(std::max<std::size_t>(watched_.size(),1));int n;
        do{n=::epoll_wait(fd_,events.data(),int(events.size()),0);}while(n<0&&errno==EINTR);
        if(n<0)failure("poll native events");return std::any_of(events.begin(),events.begin()+n,[&](const auto &event){return event.data.u64==tag;});}
};
}
#else
#include <stdexcept>
namespace halo2::native_events {
[[noreturn]] inline void unavailable(){throw std::runtime_error("Native event/timer platform unavailable");}
class Signal {public:Signal(){unavailable();}int descriptor()const{return -1;}void signal(){unavailable();}
    std::optional<std::uint64_t> take(){unavailable();}};
class Timer {public:explicit Timer(Clock){unavailable();}int descriptor()const{return -1;}
    bool arm_relative_100ns(std::uint64_t){unavailable();}bool arm_realtime_100ns(std::uint64_t){unavailable();}
    bool cancel(){unavailable();}std::optional<std::uint64_t> take(){unavailable();}std::uint64_t remaining_100ns()const{unavailable();}bool disarmed()const{unavailable();}};
class Poller {public:Poller(){unavailable();}int descriptor()const{return -1;}void add(int,std::uint64_t){unavailable();}
    void remove(int){unavailable();}bool ready(std::uint64_t)const{unavailable();}};
}
#endif
