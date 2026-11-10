#include <iostream>
#define _MSC_VER 1
#include "physics/runtime/native_tu15_camera_instance_query_frame.hpp"
#undef _MSC_VER
struct UnavailableState {};
int main(){
    UnavailableState state;unsigned children{};
    const auto entered=halo2::native_camera_instance_query_detail::bridge(state,[&](auto&,auto,auto){++children;});
    if(entered||children)return 1;
    std::cout<<"unsupported backend preserved the retained route\n";return 0;
}
