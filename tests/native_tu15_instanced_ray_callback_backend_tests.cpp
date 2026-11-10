#include <iostream>
#define _MSC_VER 1
#include "physics/runtime/native_tu15_instanced_ray_callback.hpp"
#undef _MSC_VER
struct UnavailableState {};
int main(){
    UnavailableState state;unsigned children{};
    auto child=[&](auto&,auto,auto,bool){++children;};
    if(halo2::native_instanced_ray_callback_binding_detail::forward(state,child)||
       halo2::native_instanced_ray_callback_binding_detail::collect(state,child)||children)return 1;
    std::cout<<"unsupported callback backend preserved both retained routes\n";return 0;
}
