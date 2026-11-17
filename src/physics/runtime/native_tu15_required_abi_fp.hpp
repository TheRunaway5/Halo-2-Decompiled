#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_host_fp_io.hpp"
#include <exception>

namespace halo2::native_required_abi_fp_detail {
inline constexpr bool available()noexcept{return true;}
// The provider owns its complete returned ABI, including source FP controls
// and status. Its incidental C++ host environment is a separate file. Restore
// the parked caller before State.load so a provider's pending host exception
// cannot fire in that load; then apply the actual returned source FP outcome.
// No ABI or observer is copied/wrapped by this helper. Callers retain the real
// required-owner descriptor and present/absent-owner policy.
template<class State,class Abi,class Provider>
void invoke(State&state,Abi&abi,Provider&&provider){
    std::exception_ptr original;
    try{native_host_fp_io_detail::preserve(std::forward<Provider>(provider));}
    catch(...){original=std::current_exception();}
    if(original){
        // Unsupported returned ABI shapes remain outside native resume. They
        // must not replace an already-thrown child's original exception.
        try{state.load(abi);}catch(...){std::rethrow_exception(original);}
        std::rethrow_exception(original);
    }
    state.load(abi);
}
} // namespace halo2::native_required_abi_fp_detail
#else
namespace halo2::native_required_abi_fp_detail {
inline constexpr bool available()noexcept{return false;}
}
#endif
