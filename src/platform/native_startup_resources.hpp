#pragma once
#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <system_error>

#if defined(__APPLE__)
#include "macos/startup_resources.hpp"
#elif defined(_WIN32)
#include "platform/windows_api.hpp"
#include <io.h>
#include <sys/stat.h>
namespace halo2::native_startup_resources {
struct Worker {DWORD id{};explicit operator bool()const noexcept{return id!=0;}};
inline Worker current_worker(){return {::GetCurrentThreadId()};}
inline bool raise_worker_priority(const Worker& worker) {
    if(!worker)throw std::invalid_argument("Native Windows priority requires an actual worker identity");
    const auto thread=::OpenThread(THREAD_SET_INFORMATION,FALSE,worker.id);
    if(!thread)return false;
    const auto result=::SetThreadPriority(thread,THREAD_PRIORITY_ABOVE_NORMAL);::CloseHandle(thread);return result!=FALSE;
}
inline bool same_file_extent_and_time(const struct stat& before,const struct stat& after) {
    return before.st_size==after.st_size && before.st_mtime==after.st_mtime;
}
inline int synchronize_file(int descriptor){return ::_commit(descriptor);}
}
#elif defined(__linux__)
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace halo2::native_startup_resources {
struct Worker {
    std::int64_t id{};
    explicit operator bool() const noexcept {return id>0;}
};
inline Worker current_worker() {
    const auto id=::syscall(SYS_gettid);
    if(id<=0)throw std::system_error(errno?errno:EIO,std::generic_category(),"Query native Linux worker identity");
    return {id};
}
inline bool raise_worker_priority(const Worker& worker) {
    if(!worker)throw std::invalid_argument("Native Linux priority requires an actual worker identity");
    // Retain the existing Linux nice(-1) policy and its actual failure BOOL.
    return ::setpriority(PRIO_PROCESS,id_t(worker.id),-1)==0;
}
inline bool same_file_extent_and_time(const struct stat& before,const struct stat& after) {
    return before.st_size==after.st_size && before.st_mtim.tv_sec==after.st_mtim.tv_sec &&
        before.st_mtim.tv_nsec==after.st_mtim.tv_nsec;
}
inline int synchronize_file(int descriptor) {return ::fdatasync(descriptor);}
} // namespace halo2::native_startup_resources
#else
namespace halo2::native_startup_resources {
struct Worker {explicit operator bool() const noexcept {return false;}};
inline Worker current_worker() {throw std::runtime_error("Native startup worker platform unavailable");}
inline bool raise_worker_priority(const Worker&) {throw std::runtime_error("Native startup priority platform unavailable");}
} // namespace halo2::native_startup_resources
#endif
