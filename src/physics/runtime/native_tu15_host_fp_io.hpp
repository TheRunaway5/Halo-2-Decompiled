#pragma once
#include <type_traits>
#include <utility>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#endif

namespace halo2::native_host_fp_io_detail {
// Preserve the host's opaque FP image across an observation/provider callback.
// This owner neither imports nor publishes State's software x87/MMX file.
// Genuine child execution belongs outside this boundary so its returned FP
// outcome remains visible to the caller.
template<class Callback>void preserve(Callback&&callback){
    static_assert(std::is_void_v<std::invoke_result_t<Callback>>);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto image=native_raw_x87_detail::capture();
    try{std::forward<Callback>(callback)();}
    catch(...){native_raw_x87_detail::restore(image);throw;}
    native_raw_x87_detail::restore(image);
#else
    std::forward<Callback>(callback)();
#endif
}
} // namespace halo2::native_host_fp_io_detail
