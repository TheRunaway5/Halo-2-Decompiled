#pragma once
#include <cerrno>
#include <cstdint>
#include <limits>
#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace halo2::native_startup_resources {
struct Worker {
    pthread_t thread{};
    std::uint64_t id{};
    explicit operator bool() const noexcept {return thread && id;}
};
inline Worker current_worker() {
    std::uint64_t id{};
    const auto error=::pthread_threadid_np(nullptr,&id);
    if(error)throw std::system_error(error,std::generic_category(),"Query native Darwin worker identity");
    if(!id)throw std::runtime_error("Native Darwin worker has no thread identity");
    return {::pthread_self(),id};
}
inline bool raise_worker_priority(const Worker& worker) {
    if(!worker)throw std::invalid_argument("Native Darwin priority requires an actual worker identity");
    int policy{};sched_param parameters{};
    if(::pthread_getschedparam(worker.thread,&policy,&parameters))return false;
    // Named Darwin host policy: one scheduling-priority step for the actual
    // retained worker. The original constructor ignores this BOOL. This is
    // not a Windows/Xbox scheduler mapping or a process-wide nice operation.
    if(parameters.sched_priority==std::numeric_limits<int>::max())
        throw std::overflow_error("Native Darwin worker priority cannot be incremented");
    ++parameters.sched_priority;
    return ::pthread_setschedparam(worker.thread,policy,&parameters)==0;
}
inline bool same_file_extent_and_time(const struct stat& before,const struct stat& after) {
    return before.st_size==after.st_size && before.st_mtimespec.tv_sec==after.st_mtimespec.tv_sec &&
        before.st_mtimespec.tv_nsec==after.st_mtimespec.tv_nsec;
}
inline int synchronize_file(int descriptor) {
    int result;
    do {result=::fsync(descriptor);}while(result && errno==EINTR);
    return result;
}
} // namespace halo2::native_startup_resources
