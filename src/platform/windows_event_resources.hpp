#pragma once
#include "platform/windows_api.hpp"
#include <climits>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <realtimeapiset.h>

namespace halo2::native_events {
namespace windows_events {
// Source-facing descriptors are stable aliases, never truncated Win64 HANDLEs.
struct Registry {std::mutex mutex;std::map<int,HANDLE> handles;int next=1;};
inline Registry& registry(){static Registry value;return value;}
class Resource {
    HANDLE handle_;
    int descriptor_;
public:
    explicit Resource(HANDLE handle):handle_(handle) {
        if(!handle_)windows_detail::failure("Create native Windows event resource");
        try {
            auto& r=registry();std::lock_guard lock(r.mutex);
            if(r.next==std::numeric_limits<int>::max())throw std::overflow_error("Native event descriptors exhausted");
            descriptor_=r.next;r.handles.emplace(descriptor_,handle_);++r.next;
        } catch(...){::CloseHandle(handle_);throw;}
    }
    ~Resource(){auto& r=registry();std::lock_guard lock(r.mutex);r.handles.erase(descriptor_);::CloseHandle(handle_);}
    Resource(const Resource&)=delete;
    Resource& operator=(const Resource&)=delete;
    HANDLE handle()const{return handle_;}
    int descriptor()const{return descriptor_;}
};
inline bool ready(int descriptor) {
    auto& r=registry();std::lock_guard lock(r.mutex);
    const auto at=r.handles.find(descriptor);
    if(at==r.handles.end())throw std::invalid_argument("Native Windows event descriptor has retired");
    const auto result=::WaitForSingleObject(at->second,0);
    if(result==WAIT_FAILED)windows_detail::failure("Poll native Windows event");
    return result==WAIT_OBJECT_0;
}
inline std::uint64_t now_100ns(Clock clock) {
    if(clock==Clock::Realtime) {
        FILETIME value;::GetSystemTimeAsFileTime(&value);
        return (std::uint64_t(value.dwHighDateTime)<<32|value.dwLowDateTime)-116444736000000000ull;
    }
    // Windows 8+ relative waitable timers pause during sleep/hibernation.
    // QPC/steady_clock includes that interval and would retire live timers
    // prematurely after resume. Use the kernel's matching awake-time clock.
    ULONGLONG value;
    if(!::QueryUnbiasedInterruptTime(&value))windows_detail::failure("Query native Windows timer clock");
    return value;
}
}
class Signal {
    windows_events::Resource event_{::CreateEventW(nullptr,TRUE,FALSE,nullptr)};
    std::mutex mutex_;
    std::uint64_t count_{};
public:
    int descriptor()const{return event_.descriptor();}
    void signal(){std::lock_guard lock(mutex_);
        if(count_==UINT64_MAX)throw std::overflow_error("Native Windows event count overflow");
        if(!::SetEvent(event_.handle()))windows_detail::failure("Signal native Windows event");++count_;}
    std::optional<std::uint64_t> take(){std::lock_guard lock(mutex_);
        if(!count_)return {};
        if(!::ResetEvent(event_.handle()))windows_detail::failure("Consume native Windows event");
        const auto value=count_;count_=0;return value;}
};
class Timer {
    windows_events::Resource timer_{::CreateWaitableTimerW(nullptr,TRUE,nullptr)};
    const Clock clock_;
    mutable std::mutex mutex_;
    std::uint64_t deadline_{};
    // CancelWaitableTimer stops scheduling but leaves an already signaled
    // timer signaled. A new distant schedule clears that receipt before cancel.
    void clear(){LARGE_INTEGER distant;distant.QuadPart=-std::numeric_limits<LONGLONG>::max();
        if(!::SetWaitableTimer(timer_.handle(),&distant,0,nullptr,nullptr,FALSE) ||
           !::CancelWaitableTimer(timer_.handle()))windows_detail::failure("Clear native Windows timer");}
    bool set(std::uint64_t deadline,LONGLONG due){const auto previous=deadline_>windows_events::now_100ns(clock_);
        LARGE_INTEGER value;value.QuadPart=due;
        if(!::SetWaitableTimer(timer_.handle(),&value,0,nullptr,nullptr,FALSE))windows_detail::failure("Arm native Windows timer");
        deadline_=deadline;return previous;}
public:
    explicit Timer(Clock clock):clock_(clock){}
    int descriptor()const{return timer_.descriptor();}
    bool arm_relative_100ns(std::uint64_t duration){std::lock_guard lock(mutex_);
        const auto now=windows_events::now_100ns(clock_);
        if(clock_!=Clock::Monotonic || !duration || duration>std::uint64_t(LLONG_MAX) || duration>UINT64_MAX-now)
            throw std::invalid_argument("Native relative timer interval is unavailable");
        return set(now+duration,-LONGLONG(duration));}
    bool arm_realtime_100ns(std::uint64_t time){std::lock_guard lock(mutex_);
        if(clock_!=Clock::Realtime || !time || time>std::uint64_t(LLONG_MAX)-116444736000000000ull)
            throw std::invalid_argument("Native absolute timer deadline is unavailable");
        return set(time,LONGLONG(time+116444736000000000ull));}
    bool cancel(){std::lock_guard lock(mutex_);const auto previous=deadline_>windows_events::now_100ns(clock_);
        clear();deadline_=0;return previous;}
    std::optional<std::uint64_t> take(){std::lock_guard lock(mutex_);
        if(!windows_events::ready(descriptor()))return {};clear();deadline_=0;return 1;}
    std::uint64_t remaining_100ns()const{std::lock_guard lock(mutex_);const auto now=windows_events::now_100ns(clock_);
        return deadline_>now?deadline_-now:0;}
    bool disarmed()const{return remaining_100ns()==0;}
};
class Poller {
    windows_events::Resource identity_{::CreateEventW(nullptr,TRUE,FALSE,nullptr)};
    std::map<int,std::uint64_t> watched_;
public:
    int descriptor()const{return identity_.descriptor();}
    void add(int descriptor,std::uint64_t tag){(void)windows_events::ready(descriptor);
        if(!watched_.emplace(descriptor,tag).second)throw std::invalid_argument("Duplicate native Windows event registration");}
    void remove(int descriptor){if(!watched_.erase(descriptor))throw std::invalid_argument("Native Windows event registration is unavailable");}
    bool ready(std::uint64_t tag)const{for(const auto& [descriptor,value]:watched_)
        if(value==tag && windows_events::ready(descriptor))return true;return false;}
};
}
