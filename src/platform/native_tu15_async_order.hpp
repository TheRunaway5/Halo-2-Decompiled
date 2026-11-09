#pragma once
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "platform/native_tu15_async_queue_abi.hpp"
#include <utility>

namespace halo2::native_async_order_detail {
inline bool reference(){return native_async_queue_abi_detail::reference();}
template<class State>bool admitted(State&s){return native_async_queue_abi_detail::admitted(s);}
template<class State>bool compare(State&s,std::uint32_t site_base){
    return native_async_queue_abi_detail::compare(s,site_base);
}
template<class State,class Compare>bool insert(State&s,std::uint32_t site_base,Compare&&compare){
    return native_async_queue_abi_detail::insert(s,site_base,std::forward<Compare>(compare));
}
} // namespace halo2::native_async_order_detail
