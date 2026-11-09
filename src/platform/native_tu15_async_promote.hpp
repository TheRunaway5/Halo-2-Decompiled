#pragma once
#include "platform/native_tu15_async_promotion_abi.hpp"
#include <utility>

namespace halo2::native_async_promote_detail {
template<class State,class Insert>bool promote(State&state,Insert&&insert){
    return native_async_promotion_abi_detail::promote(state,std::forward<Insert>(insert));
}
} // namespace halo2::native_async_promote_detail
