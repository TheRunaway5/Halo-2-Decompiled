#pragma once
#include <cstdint>
#include <stdexcept>
#include <system_error>
#if defined(__linux__) || defined(__APPLE__)
#include <cerrno>
#include <sched.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace halo2 {
// Existing host scheduling policy for the no-argument NtYieldExecution owner.
// POSIX success maps to NTSTATUS success; Windows returns the real NTSTATUS.
// This does not reproduce the Xbox scheduler or complete an I/O request.
inline std::uint32_t native_kernel_yield_status() {
#if defined(__linux__) || defined(__APPLE__)
    if(::sched_yield()!=0)throw std::system_error(errno,std::generic_category(),"Native source yield");
    return 0;
#elif defined(_WIN32)
    using Yield=LONG (NTAPI*)();
    static const auto yield=reinterpret_cast<Yield>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtYieldExecution"));
    if(!yield)throw std::runtime_error("Native Windows kernel-yield entry point unavailable");
    return std::uint32_t(yield());
#else
    throw std::runtime_error("Native source kernel-yield status owner unavailable");
#endif
}
} // namespace halo2
